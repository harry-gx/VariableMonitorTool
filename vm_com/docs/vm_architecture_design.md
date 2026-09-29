# 通信模块架构设计

通信模块采用 AUTOSAR 风格分层，但目录不再包一层 `autosar` 或 `shal`，而是在 `src` 下直接按层放置。第一阶段实现串口 + 自定义变量监控协议；后续 XCP、UDS、CAN/CANFD、网口继续挂接到已有层次中。

模块对 UI 只安装 `include/vm_communication.h` 和 `include/vm_status.h`。其余 MCAL、IF、PduR、Services、协议和值编解码头文件都在各自 `src/<layer>/include` 下，仅供通信模块内部编译使用。

## 分层关系

```text
UI(C++)
  │
  ▼
COM：连接管理、变量读写业务、pending 匹配、值编解码、事件上报
  │
  ▼
Services：服务抽象层，统一挂接 Custom、XCP、UDS 等协议服务
  │
  └─ custom：自定义变量监控协议服务和协议帧编解码
  │
  ▼
PduR：按设备类型和服务 ID 路由 PDU
  │
  ├─ 串口：PduR 直接连接 Serial IF
  ├─ CAN/CANFD：后续为 TP → PduR → Services
  └─ 以太网：后续为 Ethernet IF → PduR → Services
  │
  ▼
IF：串口抽象层，创建时自动挂接当前平台 MCAL 设备模板，并向上提供统一串口访问
  │
  ▼
MCAL：平台驱动层，Windows/POSIX 串口驱动各自独立，构建时只编译当前平台驱动；运行时设备节点统一使用侵入式链表
```

## 目录职责

| 目录 | 职责 | 对外可见性 |
| --- | --- | --- |
| `include/` | COM 层对 UI 暴露的 C ABI | 安装到 `build/vm_com_make/include` |
| `src/common/include/` | 通信模块内部公共基础设施，当前提供 `vm_list` 侵入式链表 | 内部使用 |
| `src/com/` | COM 层实现和值编解码 | 内部实现 |
| `src/com/include/` | COM 内部头文件，例如值编解码接口 | 内部使用 |
| `src/mcal/` | 默认平台串口设备模板注册，只负责把平台设备模板挂接到 IF 层 | 内部实现 |
| `src/mcal/windows/` | Windows 串口 MCAL 平台设备模板 | 内部实现 |
| `src/mcal/posix/` | POSIX 串口 MCAL 平台设备模板 | 内部实现 |
| `src/mcal/include/` | MCAL 驱动描述结构、操作表和内部注册接口 | 内部使用 |
| `src/if/` | Serial IF，挂接平台设备模板，选择平台，校验配置，创建、销毁、注册、打开、关闭、读写串口设备 | 内部实现 |
| `src/if/include/` | IF 内部接口 | 内部使用 |
| `src/pdur/` | PduR 路由，把下层数据分发到服务，把服务输出送回下层 | 内部实现 |
| `src/pdur/include/` | PduR 内部接口 | 内部使用 |
| `src/services/` | Services 抽象层，管理各协议服务 | 内部实现 |
| `src/services/include/` | Services 总入口内部接口 | 内部使用 |
| `src/services/custom/` | 自定义协议服务、协议帧编码、CRC、解码、流式拆包 | 内部实现 |
| `src/services/custom/include/` | Custom 协议服务内部接口 | 内部使用 |
| `driver/vendor/` | 厂商 CAN/CANFD 文件，仅后续驱动适配使用 | 不处理、不安装给 UI |

## 串口数据流

读变量时，UI 调用 `vm_comm_read_variable()`。COM 层保存 pending 请求，并调用 Services 总入口发起 custom 读请求；custom 服务编码协议帧后通过 PduR 输出，PduR 根据 `VM_PDUR_DEVICE_SERIAL` 调用 Serial IF，Serial IF 调用已选中平台设备模板对应的操作表发送串口数据。

串口收到数据时，UI 周期调用 `vm_comm_poll()`。COM 层触发 Serial IF 读取；读取到的字节交给 PduR，PduR 路由到 Services 下挂接的 custom 服务；custom 服务完成流式拆包和 CRC 校验，得到完整消息后回调 COM；COM 根据 pending 表把原始字节格式化为十进制显示值和曲线数值，再通过 `vm_comm_event_fn` 上报 UI。

写变量时，UI 只传变量描述和目标文本。COM 层调用内部值编解码器把十进制文本转换为 MCU 内存字节，再交给 Services 总入口发送 custom 写请求。位字段变量仍由 COM 层执行“先读原始存储单元、改目标 bit、再写回”的流程，UI 不需要理解位操作细节。

## 扩展原则

新增协议时，在 `src/services/<protocol>/` 新建服务子目录，并在 `src/services/vm_service.c` 中挂接该服务。COM 层只调用 Services 总入口，不直接依赖 custom、XCP 或 UDS 的实现。

新增设备时，在 MCAL/IF/TP/PduR 对应分支中扩展。串口继续走 `MCAL -> IF -> PduR -> Services`；CAN/CANFD 后续增加 TP 后走 `MCAL -> IF -> TP -> PduR -> Services`；网口走 `MCAL -> IF -> PduR -> Services`。