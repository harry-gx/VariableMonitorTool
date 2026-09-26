# 通信模块使用说明

## 构建

通信模块使用 make 独立构建：

```sh
make -C vm_com all
```

构建产物：

- 静态库：`build/vm_com_make/libvm_communication.a`
- 对外头文件：`build/vm_com_make/include/vm_communication.h`、`vm_status.h`

UI 模块只能从 `build/vm_com_make/include` 和 `build/vm_com_make/libvm_communication.a` 使用通信模块，不能直接包含 `vm_com/private`。

## 最小调用流程

```c
vm_comm_t *comm = NULL;
vm_comm_device_config_t cfg = {0};

(void)vm_comm_create(&comm);

cfg.type = VM_COMM_DEVICE_SERIAL;
cfg.serial_device = "COM15";
cfg.serial_baudrate = 115200u;
cfg.serial_data_bits = 8u;
cfg.serial_stop_bits = 1u;

(void)vm_comm_set_device_config(comm, &cfg);
(void)vm_comm_set_event_callback(comm, on_event, user_context);
(void)vm_comm_connect(comm);
```

读取变量：

```c
vm_comm_variable_t var = {0};
var.name = "g_speed_pid.SetPoint";
var.type_name = "float";
var.address = 0x20000000u;
var.size = 4u;
(void)vm_comm_read_variable(comm, &var);
```

写变量：

```c
char error[VM_COMM_TEXT_MAX] = {0};
(void)vm_comm_write_variable(comm, &var, "400", error, sizeof(error));
```

主循环或 UI 定时器中轮询接收：

```c
(void)vm_comm_poll(comm, 8u);
```

退出：

```c
vm_comm_destroy(comm);
```

## 输入输出

输入来自 UI：设备连接参数、变量描述、读取请求、标定目标文本。

输出通过事件回调返回 UI：

- `VM_COMM_EVENT_VALUE`：读取成功，包含变量地址、长度、变量名、十进制显示值和曲线数值。
- `VM_COMM_EVENT_WRITE_DONE`：写入确认。
- `VM_COMM_EVENT_ERROR`：协议错误、目标值范围错误或底层通信错误。

## 依赖

当前串口路径依赖 Qt UI 进程提供的轮询定时器调用 `vm_comm_poll()`。CAN、CANFD、以太网的配置结构已保留，驱动接入后仍走同一 COM 接口。
