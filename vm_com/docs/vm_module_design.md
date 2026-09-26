# 通信模块详细设计

## 对外接口

对外接口只位于 `include/vm_communication.h`。UI 不直接包含 `private/` 下的传输层、路由层、协议层和值编解码头文件。

### `vm_comm_create(vm_comm_t **out)`

创建 COM 层通信对象。

- 输入：`out`，通信对象指针的输出地址。
- 输出：成功时 `*out` 指向新对象。
- 返回：`VM_OK`、`VM_INVALID`、`VM_NOMEM`。

### `vm_comm_destroy(vm_comm_t *comm)`

销毁通信对象。函数会先断开设备并清理未完成请求。

- 输入：`comm`。
- 输出：无。

### `vm_comm_set_device_config(vm_comm_t *comm, const vm_comm_device_config_t *config)`

设置设备连接参数。当前可用路径是串口；CAN、CANFD、以太网的配置字段已经保留，底层驱动接入后在模块内部启用。

- 输入：通信对象、设备类型、串口端口/波特率、CAN 通道/波特率、网口地址等。
- 输出：通信对象保存一份配置副本。
- 返回：`VM_OK` 或 `VM_INVALID`。

### `vm_comm_connect(vm_comm_t *comm)`

按照已设置的设备配置连接设备。

- 输入：通信对象。
- 输出：创建对应传输对象、协议解析器和路由表。
- 返回：串口成功返回 `VM_OK`；未接入的 CAN/CANFD/以太网返回 `VM_UNSUPPORTED`。

### `vm_comm_disconnect(vm_comm_t *comm)`

断开设备，释放传输、路由、协议解析器和 pending 请求。

### `vm_comm_is_connected(const vm_comm_t *comm)`

返回当前连接状态，`1` 表示已连接，`0` 表示未连接。

### `vm_comm_set_event_callback(vm_comm_t *comm, vm_comm_event_fn callback, void *context)`

设置 COM 层事件回调。通信模块只在需要 UI 感知时上报事件。

事件类型：

| 事件 | 含义 |
| --- | --- |
| `VM_COMM_EVENT_VALUE` | 变量读响应已解析，`display_value` 是十进制显示字符串，`numeric_value` 可用于曲线。 |
| `VM_COMM_EVENT_WRITE_DONE` | MCU 已确认写请求。 |
| `VM_COMM_EVENT_ERROR` | 协议错误、编解码错误或 MCU 返回错误。 |

### `vm_comm_read_variable(vm_comm_t *comm, const vm_comm_variable_t *variable)`

按变量描述发送读请求。

- 输入：变量名、类型名、地址、字节数、位字段信息。
- 输出：无同步数据；响应经 `vm_comm_event_fn` 上报。
- 返回：`VM_OK`、`VM_INVALID`、`VM_BUSY`。

### `vm_comm_write_variable(...)`

按变量描述和目标文本发送写请求。

- 输入：变量描述、十进制目标文本、错误缓冲区。
- 输出：普通变量直接编码并写入；位字段变量先读当前值，再合并 bit 后写入。
- 返回：成功发送返回 `VM_OK`；输入非法或范围错误会返回错误码，并通过 `error` 输出中文原因。

### `vm_comm_poll(vm_comm_t *comm, uint8_t budget)`

由 UI 的定时器调用，用于接收串口数据、拆包、路由和协议处理。

- 输入：本次最多处理的接收轮数。
- 输出：可能触发事件回调。
- 返回：`VM_OK`、`VM_BUSY` 或底层接收错误。

## 内部子模块

### COM 层：`src/vm_communication.c`

负责设备配置、连接生命周期、请求上下文、值编解码、位字段读改写、事件上报。COM 层是 UI 唯一入口。

### 值编解码：`src/com/vm_value_codec.c`

根据变量类型名、字节数和位字段信息，将 MCU 内存字节转换为十进制字符串和 double 曲线值，或将 UI 输入的十进制目标值编码为 MCU 内存字节。该接口不对 UI 暴露。

### 协议层：`src/protocol/vm_custom_protocol.c`

负责自定义协议帧的编码、CRC、解码和流式拆包。后续 XCP、UDS 应以同层子模块方式扩展。

### PduR 层：`src/route/vm_router.c`

根据路由表把传输层收到的帧转给协议层，也把协议层发出的帧转给对应传输通道。

### TP/IF 层：`src/transport/`

串口传输负责打开、关闭、发送和非阻塞接收。loopback 传输用于模块测试。

### MCAL/驱动层：`src/driver/`

当前保留 CAN 适配占位，厂商库放在 `driver/vendor/`。后续接入时不得绕过 COM 层暴露给 UI。
