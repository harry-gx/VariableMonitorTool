# 通信模块使用说明

## 构建

通信模块使用 make 独立构建，Windows、Git Bash 和 Linux 环境均应直接执行：

```sh
make -C vm_com all
```

构建产物：

- 静态库：`build/vm_com_make/libvm_communication.a`
- 对外头文件：`build/vm_com_make/include/vm_communication.h`、`vm_status.h`

UI 模块只能从 `build/vm_com_make/include` 和 `build/vm_com_make/libvm_communication.a` 使用通信模块，不能直接包含 `vm_com/src/<layer>/include` 下的内部头文件。

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

## 运行依赖

当前第一阶段只实现串口 + 自定义协议。UI 进程需要周期性调用 `vm_comm_poll()`，模块内部会执行 `PduR -> 下层路由 read -> Serial IF -> PduR -> Custom Service -> PduR -> COM` 的接收处理。

读写请求由 UI 调用 COM 对外接口发起。COM 只生成通用变量读写消息并交给 PduR；PduR 再路由到 custom 服务；custom 服务完成协议组包后仍通过 PduR 找到串口下层路由发送。协议服务不会直接调用 COM，COM 也不会直接调用 custom 服务。

通信模块内部使用静态对象和显式注册表。删除 `vm_service.c` 中的服务注册表项即可禁用对应协议；删除 COM 层下层路由注册表项或 PduR 设备输出路由表项即可禁用对应底层设备通道。连接和断开只会清状态、挂链和摘链，不在运行时创建或释放通信栈节点。

串口平台设备模板在 IF 层注册：Windows 构建只编译并注册 `src/mcal/windows/vm_windows_serial.c`，POSIX 构建只编译并注册 `src/mcal/posix/vm_posix_serial.c`；IF 层创建时自动挂接当前平台模板，并基于该模板创建运行时设备。CAN、CANFD 和以太网配置字段保留，后续接入时仍走同一 COM 业务入口。
