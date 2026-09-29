# 通信模块详细设计

## 对外接口

对外接口只位于 `include/vm_communication.h` 和 `include/vm_status.h`。UI 不能直接包含 `src/<layer>/include` 下的内部头文件。当前 COM 对外保留连接、断开、轮询、读取变量、写入变量和事件回调这些业务接口。

### `vm_comm_create(vm_comm_t **out)`

创建 COM 层通信对象。

- 输入：`out`，通信对象指针输出地址。
- 输出：成功时 `*out` 指向新对象，默认设备类型为串口，默认串口参数为 115200/8N1。
- 返回：`VM_OK`、`VM_INVALID`、`VM_NOMEM`。

### `vm_comm_destroy(vm_comm_t *comm)`

销毁通信对象。函数会先断开设备，释放 Services、PduR、Serial IF、MCAL 串口设备和 pending 请求。

- 输入：`comm`，通信对象。
- 输出：释放所有由通信对象持有的资源。
- 返回：无。

### `vm_comm_set_device_config(vm_comm_t *comm, const vm_comm_device_config_t *config)`

设置设备连接参数。

- 输入：通信对象和设备配置。第一阶段只启用串口字段：`serial_device`、`serial_baudrate`、`serial_data_bits`、`serial_stop_bits`、`serial_parity`、`serial_flow_control`。
- 输出：COM 层保存一份配置副本，避免 UI 字符串释放后悬空。
- 返回：`VM_OK` 或 `VM_INVALID`。

### `vm_comm_connect(vm_comm_t *comm)`

按照已设置配置搭建通信链路。

- 输入：通信对象。
- 输出：串口路径依次创建 `Serial IF -> 注册当前平台 MCAL 设备模板 -> IF 层创建串口设备 -> PduR -> Services -> Custom Service`，并打开串口。
- 返回：串口成功返回 `VM_OK`；CAN、CANFD、以太网当前返回 `VM_UNSUPPORTED`。

### `vm_comm_disconnect(vm_comm_t *comm)`

断开设备并释放内部通信栈对象。

- 输入：通信对象。
- 输出：销毁 Services、销毁 PduR、注销并关闭串口设备、释放 MCAL 和 IF，清空 pending。
- 返回：无。

### `vm_comm_is_connected(const vm_comm_t *comm)`

查询连接状态。

- 输入：通信对象。
- 输出：无。
- 返回：`1` 表示串口 IF 中的当前设备已打开且 Services 已创建；`0` 表示未连接。

### `vm_comm_set_event_callback(vm_comm_t *comm, vm_comm_event_fn callback, void *context)`

设置事件回调。

- 输入：通信对象、回调函数和用户上下文。
- 输出：COM 层保存回调，后续只在需要 UI 感知时上报。
- 返回：无。

事件类型：

| 事件 | 含义 |
| --- | --- |
| `VM_COMM_EVENT_VALUE` | 变量读响应已解析，`display_value` 是十进制显示字符串，`numeric_value` 可用于曲线。 |
| `VM_COMM_EVENT_WRITE_DONE` | MCU 已确认写请求。 |
| `VM_COMM_EVENT_ERROR` | 协议错误、编解码错误或 MCU 返回错误。 |

### `vm_comm_read_variable(vm_comm_t *comm, const vm_comm_variable_t *variable)`

发送变量读取请求。

- 输入：通信对象和变量描述，变量描述包含名称、类型名、目标地址、字节数、位字段信息。
- 输出：请求被登记到 pending 表并通过 Services 发送；响应异步通过事件回调返回。
- 返回：`VM_OK`、`VM_INVALID`、`VM_BUSY`。

### `vm_comm_write_variable(...)`

发送变量写入请求。

- 输入：通信对象、变量描述、十进制目标文本、错误信息缓冲区。
- 输出：普通变量直接编码为 MCU 内存字节并发送写请求；位字段变量先读当前存储单元，再合并目标 bit 后发送写请求。
- 返回：成功发送返回 `VM_OK`；输入非法、范围错误、未连接或发送失败返回对应状态，并尽量通过 `error` 输出中文原因。

### `vm_comm_poll(vm_comm_t *comm, uint8_t budget)`

处理接收链路。

- 输入：通信对象；`budget` 表示本次最多读取并处理多少轮串口缓存。
- 输出：收到数据后触发 `Serial IF -> PduR -> Services -> Custom Service -> COM`，可能产生 UI 事件回调。
- 返回：`VM_OK`、`VM_BUSY` 或底层错误。

## 内部公共基础设施

### 侵入式链表：`src/common/include/vm_list.h`

通信模块内所有节点挂接统一使用该链表，包括 IF 层平台设备模板节点、IF 层运行时设备节点和 PduR 服务节点。该文件从 `vm_elf_parser/private/vm_list.h` 拷贝到通信模块内部，链表节点由业务结构体内嵌，链表本身不拥有业务对象，避免每个层级重复实现 `next` 指针链。

- 主要内部接口：`INIT_LIST_HEAD()`、`LIST_ADD_TAIL()`、`LIST_DEL()`、`list_entry()`、`LIST_FOR_EACH_ENTRY()`、`LIST_FOR_EACH_ENTRY_SAFE()`。
- 输入：链表头、内嵌节点或遍历游标。
- 输出：节点被挂接、摘除或由节点还原外层结构体。
- 说明：该头文件只供通信模块内部使用，不安装给 UI。

## 内部子模块

### COM 层：`src/com/vm_communication.c`

负责 UI 业务请求汇总、连接生命周期、pending 表、值编解码调度、位字段读改写和事件上报。COM 层调用 Services 总入口、PduR 和 Serial IF 的内部接口，不调用平台串口 API，也不直接创建 custom 协议服务。

### 值编解码：`src/com/vm_value_codec.c`

根据变量类型名、字节数和位字段信息，将 MCU 内存字节转换为十进制字符串和 double 曲线值，或将 UI 输入的十进制目标值编码为 MCU 内存字节。该接口不对 UI 暴露。

### MCAL 层：`src/mcal/`

负责平台串口设备模板定义和默认模板注册。

- `src/mcal/include/vm_mcal_serial.h`：只定义串口配置、设备对象、统计信息、操作函数指针和平台设备模板结构，不提供 open/read/write 封装函数。
- `src/mcal/include/vm_mcal_serial_platform.h`：通信模块内部平台模板查询接口，供 IF 层获取当前编译平台的默认串口设备模板。
- `src/mcal/vm_mcal_serial.c`：默认平台设备模板注册文件，只把当前编译平台的设备模板注册到 Serial IF。
- `src/mcal/windows/vm_windows_serial.c`：Windows 串口平台设备模板，使用 Win32 串口 API 实现操作表。
- `src/mcal/posix/vm_posix_serial.c`：POSIX 串口平台设备模板，使用 termios 和非阻塞 fd 实现操作表。
- 主要内部接口：`vm_mcal_serial_default_device_get()`，Windows 平台文件提供 `g_vm_windows_serial_device`，POSIX 平台文件提供 `g_vm_posix_serial_device`，构建系统只编译当前平台对应文件。

### IF 层：`src/if/vm_serial_if.c`

负责平台设备模板挂接、平台选择、串口配置校验、设备生命周期和统一读写封装。

- 主要内部接口：`vm_serial_if_create()`、`vm_serial_if_register_device()`、`vm_serial_if_unregister_device()`、`vm_serial_if_open()`、`vm_serial_if_close()`、`vm_serial_if_read()`、`vm_serial_if_write()`。平台模板挂接和运行时设备创建已收敛到 IF 内部，不再向上暴露独立接口。
- 输入：串口配置、运行时串口设备、读写缓冲区。IF 层只维护一个运行时设备链表，当前平台模板通过指针保存，不单独挂链表。
- 输出：统一的串口设备对象和读写状态。
- 说明：COM、PduR 等上层模块最多调用到 IF 层，不直接调用 MCAL 封装函数或平台 API。

### PduR 层：`src/pdur/vm_pdur.c`

负责路由下层 PDU 和服务输出。

- 主要内部接口：`vm_pdur_register_service()`、`vm_pdur_input()`、`vm_pdur_output()`。
- 输入：`vm_pdur_context_t`，包含设备类型、服务 ID、下层句柄和收发数据。服务节点通过 `vm_list` 挂接。
- 输出：匹配服务被调用，或数据写回下层 IF/TP。
- 说明：串口当前直接路由到 Serial IF；CAN/CANFD 后续在 TP 层接入后再路由。

### Services 层：`src/services/vm_service.c`

负责协议服务抽象和挂接。

- 主要内部接口：`vm_service_manager_create()`、`vm_service_manager_destroy()`、`vm_service_custom_read()`、`vm_service_custom_write()`。
- 输入：PduR 控制块、下层 IF/设备、协议请求参数。
- 输出：已编码请求通过 PduR 发送；收到完整协议消息后回调 COM。
- 说明：后续 XCP、UDS 在 Services 层新增子服务，COM 层不直接调用具体协议服务。

### 自定义协议服务：`src/services/custom/vm_custom_service.c`

负责把自定义变量监控协议挂到 Services/PduR。

- 主要内部接口：`vm_custom_service_create()`、`vm_custom_service_send_read()`、`vm_custom_service_send_write()`。
- 输入：PduR 控制块、下层 IF/设备、请求序号、地址、长度或负载。
- 输出：协议帧经 PduR 发出；接收方向解析出完整消息后回调 Services/COM。

### 自定义协议帧：`src/services/custom/vm_custom_protocol.c`

负责自定义协议帧编码、CRC、解码和流式拆包。

- 输入：请求序号、目标地址、数据长度、负载字节，或下层收到的原始字节流。
- 输出：完整协议帧，或解析完成的 `vm_custom_message_t`。
- 说明：协议帧模块只处理帧格式，不直接读写设备。