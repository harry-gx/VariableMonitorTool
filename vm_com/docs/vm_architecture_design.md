# 通信模块架构设计

通信模块采用类似 AUTOSAR 的分层结构，但代码仍保持本工程已有的 C 风格实现。模块对 UI 只暴露 `include/vm_communication.h` 和 `include/vm_status.h`，其余头文件均放在 `private/`，只允许通信模块内部和通信模块测试使用。

## 分层

```
UI(C++)
  │
  ▼
COM 层：vm_comm_* 对外接口、变量读写请求汇总、值编解码、事件上报
  │
  ▼
协议层：自定义协议，后续可扩展 XCP、UDS
  │
  ▼
PduR 层：按 route_id / connection / channel 转发协议帧和传输帧
  │
  ▼
TP/IF 层：串口传输适配，后续扩展 CAN、CANFD、以太网
  │
  ▼
MCAL/驱动层：平台串口、厂商 CAN/CANFD 库、网口驱动适配
```

## 目录职责

| 目录 | 职责 | 对外可见性 |
| --- | --- | --- |
| `include/` | COM 层对 UI 暴露的稳定接口 | 只安装到 `build/vm_com_make/include` |
| `private/` | 传输、路由、协议、值编解码等内部接口 | 不安装，不允许 UI 直接包含 |
| `src/vm_communication.c` | COM 层实现，汇总 UI 请求和协议事件 | 内部实现 |
| `src/com/vm_value_codec.c` | 通信模块内部值编解码 | 内部实现 |
| `src/protocol/` | 自定义协议编解码与流式拆包 | 内部实现 |
| `src/route/` | PduR 风格路由 | 内部实现 |
| `src/transport/` | 串口/loopback 传输 | 内部实现 |
| `src/driver/` | CAN/CANFD 等驱动占位或适配 | 内部实现 |
| `driver/vendor/` | 厂商库文件，仅作为后续驱动适配依赖 | 不直接给 UI 使用 |

## 数据流

读变量时，UI 把变量名、类型、地址、长度、位字段信息传入 `vm_comm_read_variable()`。COM 层记录请求上下文并生成自定义协议读请求；串口收到响应后，TP 层把字节交给 PduR，PduR 路由到自定义协议层，自定义协议层完成拆包和 CRC 校验，最后 COM 层根据请求上下文把原始字节转换成十进制显示字符串和曲线数值，再通过 `vm_comm_event_fn` 上报 UI。

写变量时，UI 只传变量描述和目标文本。COM 层内部调用值编解码器转换成目标 MCU 内存字节。普通变量直接发送写请求；位字段变量由 COM 层先读取当前存储单元，再只替换目标 bit 位并发送写请求。UI 不需要知道原始字节、不需要调用值编解码接口。

## 扩展原则

新增 CAN、CANFD、以太网时，只新增或替换 IF/TP/MCAL 内部实现，并在 COM 层的设备配置分支中选择对应通道。UI 仍然只调用 `vm_comm_set_device_config()`、`vm_comm_connect()`、`vm_comm_read_variable()`、`vm_comm_write_variable()` 和 `vm_comm_poll()`。
