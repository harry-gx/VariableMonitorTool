/*
 * 文件说明：POSIX 平台串口 MCAL 设备模板和操作表实现。
 * 所属模块：通信模块 / MCAL 层 / POSIX 平台。
 * 设计要点：本文件只处理 POSIX termios 串口 API，并向 IF 层提供一个 vm_mcal_serial_device_t 平台模板。
 */

#include "vm_mcal_serial.h"

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

typedef struct
{
    int fd;
} vm_posix_serial_context_t;

static vm_status_t vm_posix_serial_open(vm_mcal_serial_device_t *device);
static vm_status_t vm_posix_serial_close(vm_mcal_serial_device_t *device);
static vm_status_t vm_posix_serial_read(vm_mcal_serial_device_t *device,
                                        uint8_t *buffer,
                                        size_t capacity,
                                        size_t *read_size);
static vm_status_t vm_posix_serial_write(vm_mcal_serial_device_t *device,
                                         const uint8_t *data,
                                         size_t size);

static const vm_mcal_serial_ops_t g_vm_posix_serial_ops =
{
    vm_posix_serial_open,
    vm_posix_serial_close,
    vm_posix_serial_read,
    vm_posix_serial_write
};

const vm_mcal_serial_device_t g_vm_posix_serial_device =
{
    { NULL, NULL },
    "posix_serial",
    { { '\0' }, 0u, 0u, 0u, 0u, 0u },
    &g_vm_posix_serial_ops,
    sizeof(vm_posix_serial_context_t),
    NULL,
    { 0u, 0u, 0u, 0u, 0u },
    0u
};

static void vm_posix_serial_make_raw(struct termios *options)
{
    if (options != NULL)
    {
        options->c_iflag &=
            (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
                        IGNCR | ICRNL | IXON);
        options->c_oflag &= (tcflag_t)~OPOST;
        options->c_lflag &=
            (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        options->c_cflag &= (tcflag_t)~(CSIZE | PARENB);
        options->c_cflag |= CS8;
    }
}

static speed_t vm_posix_serial_baud_to_speed(uint32_t baudrate,
                                             uint8_t *supported)
{
    speed_t speed;

    *supported = 1u;
    switch (baudrate)
    {
        case 9600u:
            speed = B9600;
            break;
        case 19200u:
            speed = B19200;
            break;
        case 38400u:
            speed = B38400;
            break;
        case 57600u:
            speed = B57600;
            break;
        case 115200u:
            speed = B115200;
            break;
#ifdef B230400
        case 230400u:
            speed = B230400;
            break;
#endif
#ifdef B460800
        case 460800u:
            speed = B460800;
            break;
#endif
#ifdef B921600
        case 921600u:
            speed = B921600;
            break;
#endif
        default:
            *supported = 0u;
            speed = B115200;
            break;
    }

    return speed;
}

static vm_status_t vm_posix_serial_apply_config(
    int fd,
    const vm_mcal_serial_config_t *config)
{
    struct termios options;
    speed_t speed;
    uint8_t supported;
    vm_status_t status;

    if ((fd < 0) || (config == NULL))
    {
        status = VM_INVALID;
    }
    else if (tcgetattr(fd, &options) != 0)
    {
        status = VM_IO;
    }
    else
    {
        speed = vm_posix_serial_baud_to_speed(config->baudrate, &supported);
        if (supported == 0u)
        {
            status = VM_UNSUPPORTED;
        }
        else
        {
            vm_posix_serial_make_raw(&options);
            (void)cfsetispeed(&options, speed);
            (void)cfsetospeed(&options, speed);
            options.c_cflag |= (CLOCAL | CREAD);
            options.c_cflag &= (tcflag_t)~CSIZE;
            if (config->data_bits == 7u)
            {
                options.c_cflag |= CS7;
            }
            else
            {
                options.c_cflag |= CS8;
            }
            if (config->stop_bits == 2u)
            {
                options.c_cflag |= CSTOPB;
            }
            else
            {
                options.c_cflag &= (tcflag_t)~CSTOPB;
            }
            if (config->parity == (uint8_t)VM_MCAL_SERIAL_PARITY_NONE)
            {
                options.c_cflag &= (tcflag_t)~PARENB;
            }
            else
            {
                options.c_cflag |= PARENB;
                if (config->parity == (uint8_t)VM_MCAL_SERIAL_PARITY_ODD)
                {
                    options.c_cflag |= PARODD;
                }
                else
                {
                    options.c_cflag &= (tcflag_t)~PARODD;
                }
            }
            options.c_cc[VMIN] = 0;
            options.c_cc[VTIME] = 0;
            if (tcsetattr(fd, TCSANOW, &options) != 0)
            {
                status = VM_IO;
            }
            else
            {
                (void)tcflush(fd, TCIOFLUSH);
                status = VM_OK;
            }
        }
    }

    return status;
}

static vm_posix_serial_context_t *vm_posix_serial_context(
    vm_mcal_serial_device_t *device)
{
    vm_posix_serial_context_t *context;

    context = NULL;
    if (device != NULL)
    {
        context = (vm_posix_serial_context_t *)device->platform_context;
    }

    return context;
}

static vm_status_t vm_posix_serial_open(vm_mcal_serial_device_t *device)
{
    vm_posix_serial_context_t *context;
    vm_status_t status;

    context = vm_posix_serial_context(device);
    if ((device == NULL) || (context == NULL))
    {
        status = VM_INVALID;
    }
    else if (device->opened != 0u)
    {
        status = VM_OK;
    }
    else
    {
        context->fd = open(device->config.device_name,
                           (O_RDWR | O_NOCTTY | O_NONBLOCK));
        if (context->fd < 0)
        {
            status = VM_IO;
        }
        else
        {
            status = vm_posix_serial_apply_config(context->fd,
                                                  &device->config);
            if (status == VM_OK)
            {
                device->opened = 1u;
            }
            else
            {
                (void)close(context->fd);
                context->fd = -1;
            }
        }
    }

    return status;
}

static vm_status_t vm_posix_serial_close(vm_mcal_serial_device_t *device)
{
    vm_posix_serial_context_t *context;

    context = vm_posix_serial_context(device);
    if ((device != NULL) && (context != NULL) && (device->opened != 0u))
    {
        (void)close(context->fd);
        context->fd = -1;
        device->opened = 0u;
    }

    return VM_OK;
}

static vm_status_t vm_posix_serial_read(vm_mcal_serial_device_t *device,
                                        uint8_t *buffer,
                                        size_t capacity,
                                        size_t *read_size)
{
    vm_posix_serial_context_t *context;
    ssize_t actual;
    vm_status_t status;

    if (read_size != NULL)
    {
        *read_size = 0u;
    }
    context = vm_posix_serial_context(device);
    if ((device == NULL) || (context == NULL) || (buffer == NULL) ||
        (capacity == 0u) || (read_size == NULL) || (device->opened == 0u))
    {
        status = VM_INVALID;
    }
    else
    {
        actual = read(context->fd, buffer, capacity);
        if (actual > 0)
        {
            *read_size = (size_t)actual;
            device->stats.rx_bytes += (uint64_t)actual;
            status = VM_OK;
        }
        else if ((actual == 0) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            status = VM_NOT_FOUND;
        }
        else
        {
            device->stats.rx_error_count++;
            status = VM_IO;
        }
    }

    return status;
}

static vm_status_t vm_posix_serial_write(vm_mcal_serial_device_t *device,
                                         const uint8_t *data,
                                         size_t size)
{
    vm_posix_serial_context_t *context;
    const uint8_t *cursor;
    size_t remain;
    ssize_t actual;
    vm_status_t status;

    context = vm_posix_serial_context(device);
    if ((device == NULL) || (context == NULL) ||
        ((data == NULL) && (size > 0u)) || (device->opened == 0u))
    {
        status = VM_INVALID;
    }
    else
    {
        cursor = data;
        remain = size;
        status = VM_OK;
        while (remain > 0u)
        {
            actual = write(context->fd, cursor, remain);
            if (actual > 0)
            {
                cursor = &cursor[(size_t)actual];
                remain -= (size_t)actual;
                device->stats.tx_bytes += (uint64_t)actual;
            }
            else if ((actual < 0) &&
                     ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
            {
                status = VM_BUSY;
                break;
            }
            else
            {
                device->stats.tx_error_count++;
                status = VM_IO;
                break;
            }
        }
    }

    return status;
}

