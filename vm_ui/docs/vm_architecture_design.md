# UI 模块详细架构设计

## 模块定位

`vm_ui/` 是变量监控与标定工具的唯一 C++/Qt 界面模块。它负责窗口、菜单、表格、曲线、工作区配置、用户输入和状态展示，不实现 ELF/AXF 解析算法，也不实现通信协议解析算法。所有和 C 模块交互的入口集中在 `vm_ui/port/`，界面代码通过函数接口调用 `vm_elf_parser/` 和 `vm_com/`。

## 目录结构

```text
vm_ui/
├─ vm_main.cpp                       Qt 程序入口
├─ vm_VariableMonitorTool.pro        UI 唯一 qmake 工程
├─ include/                          UI 类声明
│  ├─ vm_main_window.h               主窗口编排
│  └─ pages/                         页面控件声明
├─ src/                              UI 类实现
│  └─ pages/                         各功能页实现
├─ port/                             C++ 调 C 模块的薄适配层
├─ resources/                        Qt 资源文件和图标
└─ docs/                             UI 模块文档
```

## 依赖方向

```text
Qt Widgets 事件
    ↓
MainWindow
    ↓
页面类 FilePage / DevicePage / VariableLoadPage / MonitorPage / CalibrationPage / LogPage
    ↓
MonitorFacade
    ↓
elf_parser C 接口 + communication C 接口
```

UI 模块可以包含 C 模块头文件，但 C 模块不能包含 Qt 头文件，也不能调用 UI 代码。`vm_ui/port/` 是边界层，它允许使用 Qt 类型，因为它属于 UI 模块；`vm_elf_parser/` 和 `vm_com/` 只暴露 C ABI。

## 子模块架构

### 主窗口编排层

`MainWindow` 负责创建顶部功能入口、MDI 工作区、状态栏、定时器和页面对象。它保存当前设备连接状态、变量加载状态、监控轮询状态、标定目标值和最近文件列表。主窗口不直接解析 ELF/AXF 或协议帧，而是调用 `MonitorFacade`。

### 页面层

页面层只负责控件和表格状态。

- `FilePage`：文件/工作区入口和最近文件列表。
- `DevicePage`：串口、CAN、CANFD、网口参数显示和输入。
- `VariableLoadPage`：ELF/AXF 路径、变量清单、加入监控/标定选择。
- `MonitorPage`：监控变量表、采样周期、读取/开始/停止按钮、曲线画布。
- `CalibrationPage`：标定变量表、目标值、单行标定和一键标定。
- `LogPage`：日志显示、过滤和导出。

页面之间不直接通信，状态由 `MainWindow` 分发。

### UI Port 适配层

`MonitorFacade` 管理三个 C 模块对象：串口传输、路由器和自定义协议流式解析器。它把 Qt 字符串、字节数组、回调转换为 C 模块输入输出。它还持有变量列表快照，用于 UI 表格生成。

### 值转换薄封装

`MonitorFacade` 是 UI 调用 C 通信模块的唯一薄封装，只做 Qt 字符串/结构体与 `vm_comm_*` C 接口之间的转换。变量值十进制格式化、曲线数值转换、目标值编码和位字段读改写都在通信模块 COM 层内部完成。

## 状态流

1. 用户在 `DevicePage` 选择设备并点击连接。
2. `MainWindow::connectOrDisconnectDevice()` 收集配置，调用 `MonitorFacade::connectDevice()`。
3. 用户加载 ELF/AXF 后，`MainWindow::loadImage()` 调用 `MonitorFacade::load()`，变量列表写入 `facade_.variables`。
4. `VariableLoadPage` 显示变量，用户勾选监控或标定。
5. `MainWindow::syncVariableTables()` 将选择同步到监控页和标定页。
6. 监控读请求通过 `MonitorFacade::readVariable()` 传入变量描述，通信模块 COM 层生成协议帧并经路由发送到传输层。
7. `MainWindow::pollMonitor()` 周期调用 `MonitorFacade::poll()`，传输层收到字节后经协议解析回调更新页面。
8. 标定写请求通过 `MonitorFacade::writeVariable()` 传入变量描述和目标文本，编码、范围校验和位字段读改写由通信模块 COM 层完成。

## 线程和时序

当前 UI 模块按单 UI 线程事件循环工作。串口读取在 UI 定时器驱动下非阻塞轮询，避免长时间阻塞界面。监控周期由 `MonitorPage::periodSpinBox()` 给出，`MainWindow` 会按当前表格和 pending 状态分批发送读请求。

## 扩展约束

- 新增页面只能依赖 `MainWindow` 或 `MonitorFacade` 提供的接口，不直接调用其他页面。
- 新增协议、解析、路由能力应先进入 C 模块，再由 `vm_ui/port/` 做薄适配。
- UI 里不得新增大量二进制协议解析、ELF/AXF 解析、DWARF 类型推导逻辑。
- C++ 文件命名继续使用 `vm_` 前缀。
