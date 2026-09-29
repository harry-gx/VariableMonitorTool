/*
 * 文件说明：Windows 平台串口 MCAL 设备模板和操作表实现。
 * 所属模块：通信模块 / MCAL 层 / Windows 平台。
 * 设计要点：本文件只处理 Windows 串口 API，并向 IF 层提供一个 vm_mcal_serial_device_t 平台模板。
 */

#include "vm_mcal_serial.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef struct
{
    HANDLE handle;
} vm_windows_serial_context_t;

static vm_status_t vm_windows_serial_open(vm_mcal_serial_device_t *device);
static vm_status_t vm_windows_serial_close(vm_mcal_serial_device_t *device);
static vm_status_t vm_windows_serial_read(vm_mcal_serial_device_t *device,
                                          uint8_t *buffer,
                                          size_t capacity,
                                          size_t *read_size);
static vm_status_t vm_windows_serial_write(vm_mcal_serial_device_t *device,
                                           const uint8_t *data,
                                           size_t size);

static const vm_mcal_serial_ops_t g_vm_windows_serial_ops =
{
    vm_windows_serial_open,
    vm_windows_serial_close,
    vm_windows_serial_read,
    vm_windows_serial_write
};

const vm_mcal_serial_device_t g_vm_windows_serial_device =
{
    { NULL, NULL },
    "windows_serial",
    { { '\0' }, 0u, 0u, 0u, 0u, 0u },
    &g_vm_windows_serial_ops,
    sizeof(vm_windows_serial_context_t),
    NULL,
    { 0u, 0u, 0u, 0u, 0u },
    0u
};

static uint8_t vm_windows_serial_make_name(const char *input,
                                           char *out,
                                           size_t out_size)
{
    uint8_t ok;

    ok = 0u;
    if ((input != NULL) && (out != NULL) && (out_size > 0u))
    {
        if (strncmp(input, "\\\\.\\", 4u) == 0)
        {
            (void)snprintf(out, out_size, "%s", input);
        }
        else
        {
            (void)snprintf(out, out_size, "\\\\.\\%s", input);
        }
        out[out_size - 1u] = '\0';
        ok = 1u;
    }

    return ok;
}

static vm_status_t vm_windows_serial_apply_config(
    HANDLE handle,
    const vm_mcal_serial_config_t *config)
{
    DCB dcb;
    COMMTIMEOUTS timeouts;
    vm_status_t status;

    if ((handle == INVALID_HANDLE_VALUE) || (config == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        (void)memset(&dcb, 0, sizeof(dcb));
        dcb.DCBlength = sizeof(dcb);
        if (GetCommState(handle, &dcb) == 0)
        {
            status = VM_IO;
        }
        else
        {
            dcb.BaudRate = (DWORD)config->baudrate;
            dcb.ByteSize = config->data_bits;
            dcb.StopBits = (config->stop_bits == 2u) ? TWOSTOPBITS : ONESTOPBIT;
            if (config->parity == (uint8_t)VM_MCAL_SERIAL_PARITY_ODD)
            {
                dcb.Parity = ODDPARITY;
                dcb.fParity = TRUE;
            }
            else if (config->parity == (uint8_t)VM_MCAL_SERIAL_PARITY_EVEN)
            {
                dcb.Parity = EVENPARITY;
                dcb.fParity = TRUE;
            }
            else
            {
                dcb.Parity = NOPARITY;
                dcb.fParity = FALSE;
            }
            dcb.fBinary = TRUE;
            dcb.fDtrControl = DTR_CONTROL_ENABLE;
            dcb.fRtsControl = RTS_CONTROL_ENABLE;
            dcb.fOutxCtsFlow =
                (config->flow_control ==
                 (uint8_t)VM_MCAL_SERIAL_FLOW_RTS_CTS) ? TRUE : FALSE;
            dcb.fOutX =
                (config->flow_control ==
                 (uint8_t)VM_MCAL_SERIAL_FLOW_XON_XOFF) ? TRUE : FALSE;
            dcb.fInX = dcb.fOutX;

            if (SetCommState(handle, &dcb) == 0)
            {
                status = VM_IO;
            }
            else
            {
                timeouts.ReadIntervalTimeout = MAXDWORD;
                timeouts.ReadTotalTimeoutMultiplier = 0u;
                timeouts.ReadTotalTimeoutConstant = 0u;
                timeouts.WriteTotalTimeoutMultiplier = 0u;
                timeouts.WriteTotalTimeoutConstant = 100u;
                if (SetCommTimeouts(handle, &timeouts) == 0)
                {
                    status = VM_IO;
                }
                else
                {
                    status = VM_OK;
                }
            }
        }
    }

    return status;
}

static vm_windows_serial_context_t *vm_windows_serial_context(
    vm_mcal_serial_device_t *device)
{
    vm_windows_serial_context_t *context;

    context = NULL;
    if (device != NULL)
    {
        context = (vm_windows_serial_context_t *)device->platform_context;
    }

    return context;
}

static vm_status_t vm_windows_serial_open(vm_mcal_serial_device_t *device)
{
    vm_windows_serial_context_t *context;
    char win_name[VM_MCAL_SERIAL_NAME_MAX + 8u];
    vm_status_t status;

    context = vm_windows_serial_context(device);
    if ((device == NULL) || (context == NULL))
    {
        status = VM_INVALID;
    }
    else if (device->opened != 0u)
    {
        status = VM_OK;
    }
    else if (vm_windows_serial_make_name(device->config.device_name,
                                         win_name,
                                         sizeof(win_name)) == 0u)
    {
        status = VM_INVALID;
    }
    else
    {
        context->handle = CreateFileA(win_name,
                                      (GENERIC_READ | GENERIC_WRITE),
                                      0u,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
        if (context->handle == INVALID_HANDLE_VALUE)
        {
            status = VM_IO;
        }
        else
        {
            status = vm_windows_serial_apply_config(context->handle,
                                                    &device->config);
            if (status == VM_OK)
            {
                (void)PurgeComm(context->handle,
                                (PURGE_RXCLEAR | PURGE_TXCLEAR));
                device->opened = 1u;
            }
            else
            {
                (void)CloseHandle(context->handle);
                context->handle = INVALID_HANDLE_VALUE;
            }
        }
    }

    return status;
}

static vm_status_t vm_windows_serial_close(vm_mcal_serial_device_t *device)
{
    vm_windows_serial_context_t *context;

    context = vm_windows_serial_context(device);
    if ((device != NULL) && (context != NULL) && (device->opened != 0u))
    {
        (void)CloseHandle(context->handle);
        context->handle = INVALID_HANDLE_VALUE;
        device->opened = 0u;
    }

    return VM_OK;
}

static vm_status_t vm_windows_serial_read(vm_mcal_serial_device_t *device,
                                          uint8_t *buffer,
                                          size_t capacity,
                                          size_t *read_size)
{
    vm_windows_serial_context_t *context;
    DWORD errors;
    COMSTAT stat;
    DWORD to_read;
    DWORD actual;
    vm_status_t status;

    if (read_size != NULL)
    {
        *read_size = 0u;
    }
    context = vm_windows_serial_context(device);
    if ((device == NULL) || (context == NULL) || (buffer == NULL) ||
        (capacity == 0u) || (read_size == NULL) || (device->opened == 0u))
    {
        status = VM_INVALID;
    }
    else if (ClearCommError(context->handle, &errors, &stat) == 0)
    {
        device->stats.rx_error_count++;
        status = VM_IO;
    }
    else if (stat.cbInQue == 0u)
    {
        status = VM_NOT_FOUND;
    }
    else
    {
        to_read = (stat.cbInQue > capacity) ? (DWORD)capacity : stat.cbInQue;
        actual = 0u;
        if (ReadFile(context->handle, buffer, to_read, &actual, NULL) == 0)
        {
            device->stats.rx_error_count++;
            status = VM_IO;
        }
        else if (actual == 0u)
        {
            status = VM_NOT_FOUND;
        }
        else
        {
            *read_size = (size_t)actual;
            device->stats.rx_bytes += (uint64_t)actual;
            status = VM_OK;
        }
    }

    return status;
}

static vm_status_t vm_windows_serial_write(vm_mcal_serial_device_t *device,
                                           const uint8_t *data,
                                           size_t size)
{
    vm_windows_serial_context_t *context;
    DWORD written;
    vm_status_t status;

    context = vm_windows_serial_context(device);
    if ((device == NULL) || (context == NULL) ||
        ((data == NULL) && (size > 0u)) || (device->opened == 0u))
    {
        status = VM_INVALID;
    }
    else if (size == 0u)
    {
        status = VM_OK;
    }
    else
    {
        written = 0u;
        if ((size > (size_t)UINT32_MAX) ||
            (WriteFile(context->handle,
                       data,
                       (DWORD)size,
                       &written,
                       NULL) == 0) ||
            ((size_t)written != size))
        {
            device->stats.tx_error_count++;
            status = VM_IO;
        }
        else
        {
            device->stats.tx_bytes += (uint64_t)written;
            status = VM_OK;
        }
    }

    return status;
}

