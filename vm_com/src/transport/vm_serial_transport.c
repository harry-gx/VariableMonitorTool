/*
 * 文件说明：串口传输适配实现，负责串口打开、关闭、发送和非阻塞接收。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_serial_transport.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
/* 常量说明：_DEFAULT_SOURCE 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_serial_transport
{
    /* 变量说明：base，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_t base;
    /* 变量说明：config，设备配置缓存。 */
    vm_serial_config_t config;
    /* 变量说明：rx，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_rx_fn rx;
    /* 变量说明：rx_context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *rx_context;
#ifndef _WIN32
    /* 变量说明：fd，保存当前对象运行所需的状态、参数或缓存数据。 */
    int fd;
#else
    /* 变量说明：handle，保存当前对象运行所需的状态、参数或缓存数据。 */
    HANDLE handle;
#endif
};
/**
 * 函数说明：serial_open，打开底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t serial_open(vm_transport_t *b)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
#ifdef _WIN32
    {
        DCB dcb = {0};
        COMMTIMEOUTS to = {0};
        char path[64];
        const char *dev = t->config.device;
        if (dev[0] == 'C' && dev[1] == 'O' && dev[2] == 'M' &&
            atoi(dev + 3) >= 10)
        {
            _snprintf(path, sizeof(path), "\\\\.\\%s", dev);
            dev = path;
        }
        t->handle = CreateFileA(
            dev, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (t->handle == INVALID_HANDLE_VALUE)
        {
            t->handle = NULL;
            return VM_IO;
        }
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(t->handle, &dcb))
        {
            CloseHandle(t->handle);
            t->handle = NULL;
            return VM_IO;
        }
        dcb.BaudRate = t->config.baudrate;
        dcb.ByteSize = t->config.data_bits;
        dcb.StopBits = t->config.stop_bits == 2 ? TWOSTOPBITS : ONESTOPBIT;
        dcb.Parity =
            t->config.parity == VM_SERIAL_PARITY_EVEN
                ? EVENPARITY
                : (t->config.parity == VM_SERIAL_PARITY_ODD ? ODDPARITY
                                                            : NOPARITY);
        dcb.fParity = t->config.parity != VM_SERIAL_PARITY_NONE;
        if (!SetCommState(t->handle, &dcb))
        {
            CloseHandle(t->handle);
            t->handle = NULL;
            return VM_IO;
        }
        to.ReadIntervalTimeout = MAXDWORD;
        to.ReadTotalTimeoutConstant = 0;
        to.ReadTotalTimeoutMultiplier = 0;
        to.WriteTotalTimeoutConstant = 100;
        to.WriteTotalTimeoutMultiplier = 2;
        if (!SetCommTimeouts(t->handle, &to))
        {
            CloseHandle(t->handle);
            t->handle = NULL;
            return VM_IO;
        }
        t->base.opened = 1;
        return VM_OK;
    }
#else
    {
        /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
        struct termios tio;
        speed_t speed;
        int flags = O_RDWR | O_NOCTTY | O_NONBLOCK;
        switch (t->config.baudrate)
        {
            case 9600:
                /* 变量说明：B9600，保存当前对象运行所需的状态、参数或缓存数据。 */
                speed = B9600;
                /* 变量说明：break，保存当前对象运行所需的状态、参数或缓存数据。 */
                break;
            case 19200:
                /* 变量说明：B19200，保存当前对象运行所需的状态、参数或缓存数据。 */
                speed = B19200;
                /* 变量说明：break，保存当前对象运行所需的状态、参数或缓存数据。 */
                break;
            case 38400:
                /* 变量说明：B38400，保存当前对象运行所需的状态、参数或缓存数据。 */
                speed = B38400;
                /* 变量说明：break，保存当前对象运行所需的状态、参数或缓存数据。 */
                break;
            case 57600:
                /* 变量说明：B57600，保存当前对象运行所需的状态、参数或缓存数据。 */
                speed = B57600;
                /* 变量说明：break，保存当前对象运行所需的状态、参数或缓存数据。 */
                break;
            case 115200:
                /* 变量说明：B115200，保存当前对象运行所需的状态、参数或缓存数据。 */
                speed = B115200;
                /* 变量说明：break，保存当前对象运行所需的状态、参数或缓存数据。 */
                break;
            default:
                /* 变量说明：VM_UNSUPPORTED，保存当前对象运行所需的状态、参数或缓存数据。 */
                return VM_UNSUPPORTED;
        }
        t->fd = open(t->config.device, flags);
        if (t->fd < 0)
        {
            return VM_IO;
        }
        if (tcgetattr(t->fd, &tio) != 0)
        {
            close(t->fd);
            t->fd = -1;
            return VM_IO;
        }
        cfmakeraw(&tio);
        cfsetispeed(&tio, speed);
        cfsetospeed(&tio, speed);
        tio.c_cflag |= CLOCAL | CREAD;
        tio.c_cflag &= ~CSIZE;
        tio.c_cflag |= (t->config.data_bits == 7 ? CS7 : CS8);
        if (t->config.stop_bits == 2)
        {
            tio.c_cflag |= CSTOPB;
        }
        else
        {
            tio.c_cflag &= ~CSTOPB;
        }
        if (t->config.parity == VM_SERIAL_PARITY_EVEN)
        {
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
        }
        else if (t->config.parity == VM_SERIAL_PARITY_ODD)
        {
            tio.c_cflag |= PARENB | PARODD;
        }
        else
        {
            tio.c_cflag &= ~PARENB;
        }
        if (tcsetattr(t->fd, TCSANOW, &tio) != 0)
        {
            close(t->fd);
            t->fd = -1;
            return VM_IO;
        }
        t->base.opened = 1;
        return VM_OK;
    }
#endif
}
/**
 * 函数说明：serial_close，关闭底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t serial_close(vm_transport_t *b)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
#ifndef _WIN32
    if (t->fd >= 0)
    {
        close(t->fd);
        t->fd = -1;
    }
#else
    if (t->handle && t->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(t->handle);
        t->handle = NULL;
    }
#endif
    t->base.opened = 0;
    return VM_OK;
}
/**
 * 函数说明：serial_send，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；d：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。；c：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
serial_send(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t c)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    (void)c;
    if (!t || (!d && n))
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
    if (!t->base.opened)
    {
        return VM_BUSY;
    }
#ifdef _WIN32
    {
        DWORD written = 0;
        return WriteFile(t->handle, d, (DWORD)n, &written, NULL) && written == n
                   ? VM_OK
                   : VM_IO;
    }
#else
    {
        ssize_t w = write(t->fd, d, n);
        return w == (ssize_t)n ? VM_OK
                               : (w < 0 && errno == EAGAIN ? VM_BUSY : VM_IO);
    }
#endif
}
/**
 * 函数说明：serial_rx，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；f：函数输入参数，参与本函数的计算、查找或状态更新。；c：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t serial_rx(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
    t->rx = f;
    t->rx_context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {
    serial_open, serial_close, serial_send, serial_rx};
/**
 * 函数说明：vm_serial_transport_create，创建并初始化对象。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_create(const vm_serial_config_t *c,
                                       vm_serial_transport_t **out)
{
    vm_serial_transport_t *t;
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = vm_serial_config_validate(c);
    if (s != VM_OK)
    {
        return s;
    }
    *out = NULL;
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        return VM_NOMEM;
    }
    t->config = *c;
    t->base.ops = &ops;
    t->base.context = t;
#ifndef _WIN32
    t->fd = -1;
#else
    t->handle = NULL;
#endif
    *out = t;
    return VM_OK;
}
/**
 * 函数说明：vm_serial_transport_destroy，销毁对象并释放相关资源。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_destroy(vm_serial_transport_t *t)
{
    if (!t)
    {
        return VM_OK;
    }
    serial_close(&t->base);
    free(t);
    return VM_OK;
}
/**
 * 函数说明：vm_serial_transport_base，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_serial_transport_base(vm_serial_transport_t *t)
{
    return t ? &t->base : NULL;
}
/**
 * 函数说明：vm_serial_transport_receive_once，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。；wait_ms：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_receive_once(vm_serial_transport_t *t,
                                             uint32_t wait_ms)
{
    if (!t)
    {
        return VM_INVALID;
    }
    if (!t->base.opened)
    {
        return VM_BUSY;
    }
#ifdef _WIN32
    {
        uint8_t data[2048];
        DWORD n = 0;
        DWORD want = 0;
        DWORD errors = 0;
        COMSTAT stat;
        (void)wait_ms;
        if (!ClearCommError(t->handle, &errors, &stat))
        {
            return VM_IO;
        }
        if (stat.cbInQue == 0)
        {
            return VM_NOT_FOUND;
        }
        want = stat.cbInQue > (DWORD)sizeof(data) ? (DWORD)sizeof(data)
                                                  : stat.cbInQue;
        if (!ReadFile(t->handle, data, want, &n, NULL))
        {
            return VM_IO;
        }
        if (!n)
        {
            return VM_NOT_FOUND;
        }
        if (!t->rx)
        {
            return VM_NOT_FOUND;
        }
        {
            vm_transport_packet_t p = {data, (size_t)n, 0, 0};
            return t->rx(t->rx_context, &p);
        }
    }
#else
    {
        fd_set set;
        /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
        struct timeval tv;
        uint8_t data[2048];
        ssize_t n;
        FD_ZERO(&set);
        FD_SET(t->fd, &set);
        tv.tv_sec = (long)(wait_ms / 1000);
        tv.tv_usec = (long)((wait_ms % 1000) * 1000);
        if (select(t->fd + 1, &set, NULL, NULL, &tv) <= 0)
        {
            /* 变量说明：VM_NOT_FOUND，保存当前对象运行所需的状态、参数或缓存数据。 */
            return VM_NOT_FOUND;
        }
        n = read(t->fd, data, sizeof(data));
        if (n < 0)
        {
            return errno == EAGAIN ? VM_NOT_FOUND : VM_IO;
        }
        if (!n)
        {
            return VM_IO;
        }
        if (!t->rx)
        {
            return VM_NOT_FOUND;
        }
        {
            vm_transport_packet_t p = {data, (size_t)n, 0, 0};
            return t->rx(t->rx_context, &p);
        }
    }
#endif
}
