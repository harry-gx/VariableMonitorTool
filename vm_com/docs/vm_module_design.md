# 通信模块详细设计

## 对外接口

对外接口只位于 `include/vm_communication.h` 和 `include/vm_status.h`。UI 不能直接包含 `src/<layer>/include` 下的内部头文件。当前 COM 对外保留连接、断开、轮询、读取变量、写入变量和事件回调这些业务接口。

### `vm_comm_create(vm_comm_t **out)`

获取并初始化 COM 层静态通信对象。通信模块当前只提供一个 COM 实例，函数保持 create 命名是为了兼容 UI 侧已有调用。

- 输入：`out`，通信对象指针输出地址。
- 输出：成功时 `*out` 指向静态通信对象，默认设备类型为串口，默认串口参数为 115200/8N1。
- 返回：`VM_OK`、`VM_INVALID`、`VM_BUSY`。

### `vm_comm_destroy(vm_comm_t *comm)`

反初始化 COM 层静态通信对象。函数会先断开设备，注销 Services、PduR COM 入口和下层路由，清理 PduR、Serial IF、MCAL 串口设备和 pending 请求。

- 输入：`comm`，通信对象。
- 输出：所有静态节点从链表摘除并清零，不做堆内存释放。
- 返回：无。

### `vm_comm_set_device_config(vm_comm_t *comm, const vm_comm_device_config_t *config)`

设置设备连接参数。

- 输入：通信对象和设备配置。第一阶段只启用串口字段：`serial_device`、`serial_baudrate`、`serial_data_bits`、`serial_stop_bits`、`serial_parity`、`serial_flow_control`。
- 输出：COM 层保存一份配置副本，避免 UI 字符串释放后悬空。
- 返回：`VM_OK` 或 `VM_INVALID`。

### `vm_comm_connect(vm_comm_t *comm)`

按照已设置配置搭建通信链路。

- 输入：通信对象。
- 输出：串口路径依次初始化 `Serial IF 静态控制块 -> 当前平台 MCAL 设备模板 -> IF 层静态串口设备 -> PduR 静态控制块 -> 注册 COM 上报入口 -> 按显式下层路由表注册串口路由 -> 按 Services 显式服务表注册 Custom Service`，并打开串口。
- 返回：串口成功返回 `VM_OK`；CAN、CANFD、以太网当前返回 `VM_UNSUPPORTED`。

### `vm_comm_disconnect(vm_comm_t *comm)`

断开设备并反初始化内部通信栈对象。

- 输入：通信对象。
- 输出：注销 Services、注销 PduR COM 上报入口、注销 PduR 下层路由、清理 PduR、注销并关闭串口设备、清理 IF，清空 pending；所有节点均为静态节点，只摘链和清状态。
- 返回：无。

### `vm_comm_is_connected(const vm_comm_t *comm)`

查询连接状态。

- 输入：通信对象。
- 输出：无。
- 返回：`1` 表示串口 IF 中的当前设备已打开、PduR 下层路由已注册且 Services 已创建；`0` 表示未连接。

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
- 输出：请求被登记到 pending 表并通过 PduR 路由到协议服务；响应异步通过事件回调返回。
- 返回：`VM_OK`、`VM_INVALID`、`VM_BUSY`。

### `vm_comm_write_variable(...)`

发送变量写入请求。

- 输入：通信对象、变量描述、十进制目标文本、错误信息缓冲区。
- 输出：普通变量直接编码为 MCU 内存字节并发送写请求；位字段变量先读当前存储单元，再合并目标 bit 后发送写请求。
- 返回：成功发送返回 `VM_OK`；输入非法、范围错误、未连接或发送失败返回对应状态，并尽量通过 `error` 输出中文原因。

### `vm_comm_poll(vm_comm_t *comm, uint8_t budget)`

处理接收链路。

- 输入：通信对象；`budget` 表示本次最多读取并处理多少轮串口缓存。
- 输出：COM 触发 `PduR -> 下层路由 read -> Serial IF -> PduR -> Custom Service -> PduR -> COM`，可能产生 UI 事件回调。
- 返回：`VM_OK`、`VM_BUSY` 或底层错误。

## 内部公共基础设施

### 侵入式链表：`src/common/include/vm_list.h`

通信模块内所有节点挂接统一使用该链表，包括 IF 层运行时设备节点、PduR 下层路由节点和 PduR 服务节点。该文件从 `vm_elf_parser/private/vm_list.h` 拷贝到通信模块内部，链表节点由业务结构体内嵌，链表本身不拥有业务对象，避免每个层级重复实现 `next` 指针链。

- 主要内部接口：`INIT_LIST_HEAD()`、`LIST_ADD_TAIL()`、`LIST_DEL()`、`list_entry()`、`LIST_FOR_EACH_ENTRY()`、`LIST_FOR_EACH_ENTRY_SAFE()`。
- 输入：链表头、内嵌节点或遍历游标。
- 输出：节点被挂接、摘除或由节点还原外层结构体。
- 说明：该头文件只供通信模块内部使用，不安装给 UI。

## 内部子模块

### COM 层：`src/com/vm_communication.c`

负责 UI 业务请求汇总、连接生命周期、pending 表、值编解码调度、位字段读改写和事件上报。COM 层负责打开/关闭串口 IF，把串口 IF 作为下层路由注册到 PduR，并把自己的事件接收函数注册为 PduR 的 COM 上报入口；业务读写请求只调用 PduR 通用消息接口，不直接调用任何协议服务。COM 层内部用 `g_vm_comm_lower_route_table[]` 显式列出启用的下层路由，删除某个表项即可禁用对应底层设备通道。

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

负责平台设备模板挂接、平台选择、串口配置校验、设备生命周期和统一读写封装。Serial IF 使用静态控制块、静态运行时设备节点和静态平台上下文存储，不在连接过程中申请堆内存。

- 主要内部接口：`vm_serial_if_create()`、`vm_serial_if_register_device()`、`vm_serial_if_unregister_device()`、`vm_serial_if_open()`、`vm_serial_if_close()`、`vm_serial_if_read()`、`vm_serial_if_write()`。平台模板挂接和运行时设备创建已收敛到 IF 内部，不再向上暴露独立接口。
- 输入：串口配置、运行时串口设备、读写缓冲区。IF 层只维护一个运行时设备链表，当前平台模板通过指针保存，不单独挂链表。
- 输出：统一的串口设备对象和读写状态。
- 说明：COM、PduR 等上层模块最多调用到 IF 层，不直接调用 MCAL 封装函数或平台 API。

### PduR 层：`src/pdur/vm_pdur.c`

负责路由下层 PDU、COM 逻辑请求、协议服务输出和协议服务上报。PduR 使用静态控制块，下层路由节点和协议服务节点由对应模块静态定义，PduR 注册时只把节点挂接到侵入式链表。

- 主要内部接口：`vm_pdur_register_lower_route()`、`vm_pdur_unregister_lower_route()`、`vm_pdur_register_com_route()`、`vm_pdur_unregister_com_route()`、`vm_pdur_register_service()`、`vm_pdur_unregister_service()`、`vm_pdur_poll()`、`vm_pdur_input()`、`vm_pdur_output()`、`vm_pdur_service_request()`、`vm_pdur_com_indicate()`。
- 输入：下层路由静态节点包含设备类型、通道号、下层对象和 read/write 回调；COM 注册配置包含上报回调；服务静态节点包含设备类型、通道号、服务 ID、匹配函数、下层处理函数和 COM 请求处理函数；`vm_pdur_context_t` 包含设备类型、服务 ID、通道号和收发数据；`vm_pdur_message_t` 表达 COM 与协议服务之间的通用读写语义。
- 输出：接收方向由 PduR 轮询下层路由并调用匹配服务；COM 下行方向由 PduR 按设备类型、通道号和服务 ID 找到协议服务；服务发送方向由 PduR 按设备类型和通道号查找下层路由并调用 write 回调；服务上行方向由 PduR 调用已注册的 COM 上报入口。
- 说明：`vm_pdur.c` 中用 `g_vm_pdur_device_info[]` 显式列出设备类型输出路由。串口当前注册 Serial IF 下层路由；CAN/CANFD 后续注册 CAN TP 下层路由；以太网后续注册 Ethernet IF 下层路由。删除设备路由表项或不注册下层静态节点时，对应设备不可用。

### Services 层：`src/services/vm_service.c`

负责协议服务节点的初始化和挂接，职责等同于 `tkd_shal` 中集中调用 `PduR_ServiceRegister()` 的 service 初始化文件。

- 主要内部接口：`vm_service_manager_create()`、`vm_service_manager_destroy()`。
- 输入：PduR 控制块、设备类型和通道号。
- 输出：当前阶段把 custom 静态服务节点注册到 PduR；反初始化时从 PduR 注销节点。
- 说明：该层不转发 COM 业务请求，也不向 COM 回调数据。`vm_service.c` 中用 `g_vm_service_table[]` 显式列出启用的协议服务，删除某个表项后，对应协议不会注册到 PduR，整条协议链路不可用。后续 XCP、UDS 在 Services 下新增子目录和服务节点，并在该表中统一注册到 PduR。

### 自定义协议服务：`src/services/custom/vm_custom_service.c`

负责把自定义变量监控协议挂到 Services/PduR。custom 服务对象、PduR 服务节点和流式解析器均为静态对象。

- 主要内部接口：`vm_custom_service_create()`、`vm_custom_service_destroy()`。PduR 的服务请求回调内直接完成读写命令判断、协议组包和 PduR 输出，不再额外拆出 read/write/send_frame 转发函数。
- 输入：PduR 控制块、设备类型、通道号，以及 PduR 转入的 `vm_pdur_message_t` 或下层原始字节流。
- 输出：COM 请求被编码为自定义协议帧并经 PduR 发出；接收方向解析出完整消息后转换成 `vm_pdur_message_t` 并通过 PduR 上报 COM。

### 自定义协议帧：`src/services/custom/vm_custom_protocol.c`

负责自定义协议帧编码、CRC、解码和流式拆包。

- 输入：请求序号、目标地址、数据长度、负载字节，或下层收到的原始字节流。
- 输出：完整协议帧，或解析完成的 `vm_custom_message_t`。
- 说明：协议帧模块只处理帧格式，不直接读写设备。
