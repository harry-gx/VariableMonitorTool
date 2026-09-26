#include "vm_serial_transport.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif
struct vm_serial_transport
{
    vm_transport_t base;
    vm_serial_config_t config;
    vm_transport_rx_fn rx;
    void *rx_context;
#ifndef _WIN32
    int fd;
#else
    HANDLE handle;
#endif
};
static vm_status_t serial_open(vm_transport_t *b)
{
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
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
        struct termios tio;
        speed_t speed;
        int flags = O_RDWR | O_NOCTTY | O_NONBLOCK;
        switch (t->config.baudrate)
        {
            case 9600:
                speed = B9600;
                break;
            case 19200:
                speed = B19200;
                break;
            case 38400:
                speed = B38400;
                break;
            case 57600:
                speed = B57600;
                break;
            case 115200:
                speed = B115200;
                break;
            default:
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
static vm_status_t serial_close(vm_transport_t *b)
{
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
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
static vm_status_t
serial_send(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t c)
{
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    (void)c;
    if (!t || (!d && n))
    {
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
static vm_status_t serial_rx(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    struct vm_serial_transport *t = (struct vm_serial_transport *)b;
    if (!t)
    {
        return VM_INVALID;
    }
    t->rx = f;
    t->rx_context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {
    serial_open, serial_close, serial_send, serial_rx};
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
vm_transport_t *vm_serial_transport_base(vm_serial_transport_t *t)
{
    return t ? &t->base : NULL;
}
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
        struct timeval tv;
        uint8_t data[2048];
        ssize_t n;
        FD_ZERO(&set);
        FD_SET(t->fd, &set);
        tv.tv_sec = (long)(wait_ms / 1000);
        tv.tv_usec = (long)((wait_ms % 1000) * 1000);
        if (select(t->fd + 1, &set, NULL, NULL, &tv) <= 0)
        {
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
