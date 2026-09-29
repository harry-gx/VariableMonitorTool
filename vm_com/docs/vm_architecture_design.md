# 通信模块架构设计

通信模块采用 AUTOSAR 风格分层，但目录不再包一层 `autosar` 或 `shal`，而是在 `src` 下直接按层放置。第一阶段实现串口 + 自定义变量监控协议；后续 XCP、UDS、CAN/CANFD、网口继续挂接到已有层次中。

模块对 UI 只安装 `include/vm_communication.h` 和 `include/vm_status.h`。其余 MCAL、IF、PduR、Services、协议和值编解码头文件都在各自 `src/<layer>/include` 下，仅供通信模块内部编译使用。

通信模块内部不在连接流程中使用 `malloc/calloc/free` 创建通信栈节点。COM 对象、PduR 控制块、Serial IF 控制块、串口运行时设备、下层路由节点、协议服务节点和 custom 协议解析器均为静态对象。初始化只负责清状态、填配置、挂接侵入式链表；反初始化只负责关闭设备、摘链和清状态。

## 分层关系

```text
UI(C++)
  │
  ▼
COM：连接管理、变量读写业务、pending 匹配、值编解码、事件上报
  │
  ▼
PduR：维护服务表、下层路由表和 COM 上报入口；COM、协议服务、IF/TP 之间只通过 PduR 交换数据
  ├─ 上层服务表：Services 用显式表注册 Custom、XCP、UDS 等协议静态节点
  │    └─ custom：自定义变量监控协议服务和协议帧编解码
  └─ 下层路由表：COM/设备适配层用显式表注册底层设备静态节点
       ├─ 串口：Serial IF -> Serial MCAL
       ├─ CAN/CANFD：后续 CAN TP -> CAN IF -> CAN MCAL
       └─ 以太网：后续 Ethernet IF -> Ethernet MCAL

IF：抽象层，当前实现 Serial IF，创建时自动挂接当前平台 MCAL 设备模板，并向上提供统一串口访问。
MCAL：平台驱动层，Windows/POSIX 串口驱动各自独立，构建时只编译当前平台驱动；运行时设备节点统一使用侵入式链表。
```

## 目录职责

| 目录 | 职责 | 对外可见性 |
| --- | --- | --- |
| `include/` | COM 层对 UI 暴露的 C ABI | 安装到 `build/vm_com_make/include` |
| `src/common/include/` | 通信模块内部公共基础设施，当前提供 `vm_list` 侵入式链表 | 内部使用 |
| `src/com/` | COM 层实现和值编解码，内部维护 COM 静态对象和显式下层路由注册表 | 内部实现 |
| `src/com/include/` | COM 内部头文件，例如值编解码接口 | 内部使用 |
| `src/mcal/` | 默认平台串口设备模板注册，只负责把平台设备模板挂接到 IF 层 | 内部实现 |
| `src/mcal/windows/` | Windows 串口 MCAL 平台设备模板 | 内部实现 |
| `src/mcal/posix/` | POSIX 串口 MCAL 平台设备模板 | 内部实现 |
| `src/mcal/include/` | MCAL 驱动描述结构、操作表和内部注册接口 | 内部使用 |
| `src/if/` | Serial IF，挂接平台设备模板，选择平台，校验配置，注册、打开、关闭、读写静态串口设备 | 内部实现 |
| `src/if/include/` | IF 内部接口 | 内部使用 |
| `src/pdur/` | PduR 路由，内部维护服务表、下层路由表、COM 上报入口和显式设备输出路由表；把 COM 请求转发到服务，把下层数据转发到服务，把服务输出送到下层，把服务逻辑消息上报 COM | 内部实现 |
| `src/pdur/include/` | PduR 内部接口 | 内部使用 |
| `src/services/` | Services 抽象层，用显式服务注册表初始化和注销协议静态节点，并把节点注册到 PduR | 内部实现 |
| `src/services/include/` | Services 总入口内部接口 | 内部使用 |
| `src/services/custom/` | 自定义协议服务、协议帧编码、CRC、解码、流式拆包 | 内部实现 |
| `src/services/custom/include/` | Custom 协议服务内部接口 | 内部使用 |
| `driver/vendor/` | 厂商 CAN/CANFD 文件，仅后续驱动适配使用 | 不处理、不安装给 UI |

## 串口数据流

读变量时，UI 调用 `vm_comm_read_variable()`。COM 层保存 pending 请求，并把通用读消息提交给 PduR；PduR 按 `设备类型 + 通道号 + 服务 ID` 找到 custom 服务节点；custom 服务把逻辑读消息编码成协议帧后再调用 PduR 输出；PduR 按 `设备类型 + 通道号` 查找下层路由节点，再经 Serial IF 调用已选中平台设备模板对应的操作表发送串口数据。

串口收到数据时，UI 周期调用 `vm_comm_poll()`。COM 层只触发 `vm_pdur_poll()`；PduR 轮询已注册的下层路由节点，通过节点的 read 回调从 Serial IF 读取字节流，再按设备类型、通道号和服务 match 函数分发到 custom 服务；custom 服务完成流式拆包和 CRC 校验，得到完整逻辑消息后通过 PduR 上报 COM；COM 根据 pending 表把原始字节格式化为十进制显示值和曲线数值，再通过 `vm_comm_event_fn` 上报 UI。

写变量时，UI 只传变量描述和目标文本。COM 层调用内部值编解码器把十进制文本转换为 MCU 内存字节，再把通用写消息交给 PduR；PduR 将写消息路由到 custom 服务，custom 服务组包后再经 PduR 路由到底层通道。位字段变量仍由 COM 层执行“先读原始存储单元、改目标 bit、再写回”的流程，UI 不需要理解位操作细节。

## 扩展原则

新增协议时，在 `src/services/<protocol>/` 新建服务子目录，并在 `src/services/vm_service.c` 中像 `tkd_shal` 的 `AsServiceInit()` 一样注册协议服务节点。COM 层只调用 PduR 的通用消息接口，不直接依赖 custom、XCP 或 UDS 的实现；协议服务也只通过 PduR 收发数据和上报 COM。

新增设备时，在 MCAL/IF/TP/PduR 对应分支中扩展。串口接收方向走 `MCAL -> IF -> PduR -> 服务节点 -> PduR -> COM`，发送方向走 `COM -> PduR -> 服务节点 -> PduR -> IF -> MCAL`；CAN/CANFD 后续在 IF 和 PduR 之间增加 TP；网口走 `IF -> PduR`。
