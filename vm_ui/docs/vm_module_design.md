# UI 模块详细模块设计

## 数据结构

| 类型 | 位置 | 说明 | 主要输入 | 主要输出 |
|---|---|---|---|---|
| `UiVariable` | `include/pages/vm_page_types.h` | UI 表格使用的变量快照 | 变量名、地址、长度、类型名、位字段信息 | 页面表格行数据 |
| `MonitorVariable` | `port/vm_ui_port.h` | C 解析结果转换后的变量描述 | `vm_monitor_variable_t` | Qt 页面和工作区保存用变量 |
| `MonitorDeviceConfig` | `port/vm_ui_port.h` | 设备连接配置 | 页面上的串口/CAN/CANFD/网口参数 | `MonitorFacade::connectDevice()` 输入 |
| `PendingBitFieldWrite` | `include/vm_main_window.h` | 位字段写入等待当前原始值时的暂存 | 变量名、目标值、地址、位偏移、位宽 | 读回当前值后合成写入数据 |

## 主窗口 `MainWindow`

| 函数 | 输入 | 输出/副作用 | 说明 |
|---|---|---|---|
| `MainWindow(QWidget *parent)` | 父窗口指针 | 创建完整主界面 | 初始化页面、工具栏、MDI 区域、状态栏、定时器和 C 模块回调。 |
| `closeEvent(QCloseEvent *event)` | Qt 关闭事件 | 保存最近文件状态，释放窗口 | 程序退出前保存 UI 状态。 |
| `addPageDock(const QString &name, QWidget *body)` | 页面名和页面控件 | 注册页面入口 | 将页面加入主工作区映射。 |
| `showPage(const QString &name)` | 页面名 | 切换主页面 | 文件、设备管理、变量加载、日志等主页面使用。 |
| `showMdiToolWindow(...)` | 窗口名、页面控件、窗口指针、尺寸、偏移 | 打开或激活 MDI 子窗口 | 监控和标定页使用，可拖拽但限制在软件内部。 |
| `log(const QString &message)` | 日志文本 | 写入日志页 | 为业务动作和错误统一记录日志。 |
| `refreshGate()` | 无 | 更新按钮可用性 | 根据设备连接和变量加载状态启用/禁用监控、标定、读取等动作。 |
| `syncVariableTables()` | 当前变量选择 | 刷新监控页和标定页 | 从变量加载页取勾选结果，并同步目标值缓存。 |
| `loadImage()` | 变量加载页路径 | 更新变量列表，刷新 UI | 调用 `MonitorFacade::load()` 加载 ELF/AXF。 |
| `readMonitor()` | 监控页勾选行 | 发送一轮读取请求 | 对当前监控表格中勾选变量调用 `sendMonitorReads()`。 |
| `startMonitor()` | 采样周期和勾选变量 | 启动轮询定时器 | 开始按周期刷新监控值和曲线。 |
| `stopMonitor()` | 无 | 停止轮询定时器 | 停止自动监控。 |
| `pollMonitor()` | 定时器事件 | 收包、发下一批读请求、更新页面 | 调用 `MonitorFacade::poll()` 并调度读请求。 |
| `expireMonitorPending()` | pending 表 | 清理超时请求 | 避免未响应请求长期占用 pending 状态。 |
| `sendMonitorReads(int maxRequests, bool logResult)` | 最大发送条数、是否记录日志 | 返回实际发送数量 | 按行轮询发送读取请求，限制单次发送量，降低 UI 卡顿。 |
| `completePendingBitFieldWrite(...)` | 地址和当前原始数据 | 生成位字段写入帧 | 对位字段先读原始值，再局部修改目标位。 |
| `readCalibration()` | 标定表变量 | 发送读取请求 | 刷新标定量当前值。 |
| `writeCalibration(bool confirm)` | 当前行目标值 | 发送当前行写请求 | 可带确认流程。 |
| `writeCalibrationRow(int row, bool confirm)` | 行号、确认标志 | 发送单个变量标定请求 | 行按钮调用。 |
| `sendCalibrationRow(int row, bool logSuccess)` | 行号、日志标志 | 返回是否发送成功 | 编码目标值并调用 facade 写变量。 |
| `writeAllCalibration()` | 全表目标值 | 逐行发送标定 | 一键标定入口。 |
| `connectOrDisconnectDevice()` | 设备页配置 | 连接或断开设备 | 根据当前状态调用 `MonitorFacade`。 |
| `newWorkspace()` | 无 | 清空当前工作区 | 重置项目打开状态、变量和选择。 |
| `saveCurrentWorkspace()` | 当前工作区状态 | 写配置文件 | 保存变量选择、目标值、设备参数。 |
| `saveWorkspace(const QString &file)` | 配置文件路径 | 写文件并更新最近列表 | 保存到指定文件。 |
| `writeWorkspace(const QString &file)` | 配置文件路径 | `true/false` | 执行 JSON 写入。 |
| `loadWorkspaceDialog()` | 用户选择 | 加载配置 | 弹出选择框并调用 `restoreWorkspace()`。 |
| `exportWorkspaceDialog()` | 用户选择 | 导出配置 | 保存工作区副本。 |
| `restoreWorkspace(const QString &file)` | 配置文件路径 | 恢复 UI 和变量选择 | 从 JSON 恢复工作区。 |
| `openRecentItem(QListWidgetItem *item)` | 最近文件列表项 | 打开对应配置 | 最近列表双击入口。 |
| `removeRecentFile(const QString &file)` | 文件路径 | 更新最近列表 | 移除不存在或用户删除的记录。 |
| `touchRecent(const QString &file)` | 文件路径 | 更新最近列表顺序 | 最近使用文件置顶。 |
| `updateRecentList()` | 最近文件状态 | 刷新文件页列表 | 同步 UI。 |
| `loadRecentState()` | 本地状态文件 | 恢复最近文件 | 程序启动时调用。 |
| `saveRecentState() const` | 最近文件状态 | 写本地状态文件 | 程序关闭时调用。 |
| `appStatePath() const` | 无 | 状态文件路径 | 返回本地 UI 状态文件路径。 |
| `connectionText() const` | 当前连接状态 | 状态栏文本 | 显示未连接/串口等状态。 |

## 页面类

### `FilePage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `FilePage(QWidget *parent)` | 父控件 | 创建文件页按钮和最近文件列表。 |
| `newButton/saveButton/loadButton/recentButton/exportButton/exitButton()` | 无 | 返回对应按钮指针，供主窗口连接信号。 |
| `recentList()` | 无 | 返回最近文件列表控件。 |
| `setProjectOpen(bool open)` | 是否打开项目 | 调整保存、导出等按钮状态。 |
| `setRecentFiles(...)` | 最近文件、当前文件、打开/删除回调 | 重建最近文件列表。 |

### `DevicePage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `refreshPorts()` | 无 | 枚举系统串口并刷新串口下拉框。 |
| `setConnected(bool connected)` | 连接状态 | 更新连接按钮文本和控件启用状态。 |
| `setDeviceType(UiDeviceType type)` | 设备类型 | 切换串口/CAN/CANFD/网口配置页。 |
| `setPortName/setBaudRate` | 串口参数 | 更新串口控件。 |
| `setCanAdapter/setCanChannel/setCanBaudRate` | CAN 参数 | 更新 CAN 控件。 |
| `setCanFdDataBaudRate` | CANFD 数据域波特率 | 更新 CANFD 控件。 |
| `setNetworkHost/setNetworkPort` | 网口参数 | 更新网口控件。 |
| `deviceType/portName/baudRate/canAdapter/canChannel/canBaudRate/canFdDataBaudRate/networkHost/networkPort` | 无 | 读取用户当前输入。 |

### `VariableLoadPage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `loadButton()` | 无 | 返回加载变量按钮。 |
| `table()` | 无 | 返回变量表格控件。 |
| `imagePath()` | 无 | 返回 ELF/AXF 路径。 |
| `setImagePath(const QString &path)` | 文件路径 | 更新路径输入框。 |
| `clearVariables()` | 无 | 清空变量表。 |
| `setVariables(const QVector<MonitorVariable> &variables)` | 变量列表 | 重建变量表。 |
| `selectedMonitorVariables()` | 表格勾选状态 | 返回加入监控的变量。 |
| `selectedCalibrationVariables()` | 表格勾选状态 | 返回加入标定的变量。 |
| `selectionJson()` | 当前勾选状态 | 导出为 JSON。 |
| `applySelection(const QJsonArray &selection)` | JSON 选择数据 | 恢复表格勾选状态。 |

### `MonitorPage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `readButton/startButton/stopButton/periodSpinBox()` | 无 | 返回控件指针供主窗口连接信号。 |
| `setReady(bool ready)` | 是否可用 | 启用/禁用监控按钮。 |
| `setRows(const QVector<UiVariable> &variables)` | 变量列表 | 重建监控表和曲线颜色映射。 |
| `updateValue(quint32 address, quint16 size, const QString &value, double numericValue, bool hasNumericValue)` | COM 层事件中的显示值和曲线值 | 更新表格当前值并追加曲线采样点。 |
| `rowCount()` | 无 | 返回监控表行数。 |
| `checkedRows()` | 无 | 返回勾选绘制曲线的行号。 |
| `addressAt/sizeAt` | 行号 | 返回该行变量地址和长度。 |

### `CalibrationPage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `setReady(bool ready)` | 是否可用 | 启用/禁用标定按钮。 |
| `setRows(...)` | 变量列表和目标值缓存 | 重建标定表。 |
| `setCalibrateRowCallback` | 行标定回调 | 单行按钮触发时调用。 |
| `setCalibrateAllCallback` | 一键标定回调 | 一键按钮触发时调用。 |
| `collectTargets()` | 表格目标值 | 返回变量名到目标值的映射。 |
| `updateValue(...)` | 地址、长度、显示值、状态文本 | 更新当前值和状态。 |
| `rowCount/currentRow/addressAt/sizeAt/nameAt/typeNameAt/bitFieldAt/bitOffsetAt/bitSizeAt/targetTextAt/variableAt` | 行号或当前选择 | 返回标定行数据和完整变量描述。 |

### `LogPage`

| 函数 | 输入 | 输出/副作用 |
|---|---|---|
| `exportButton()` | 无 | 返回导出按钮。 |
| `appendLine(const QString &line)` | 日志行 | 缓存并显示日志。 |
| `exportAll(const QString &path) const` | 文件路径 | 导出全部日志，返回成功状态。 |

## `MonitorFacade` 适配层

| 函数 | 输入 | 输出/副作用 | 调用的 C 模块 |
|---|---|---|---|
| `~MonitorFacade()` | 无 | 释放串口、路由器、协议解析器 | communication |
| `connectDevice(const MonitorDeviceConfig &config, QString &error)` | 设备配置 | 返回连接结果，失败写错误文本 | communication transport |
| `connectPort(const QString &port, unsigned baud, QString &error)` | 串口名、波特率 | 建立串口连接 | serial transport |
| `disconnectPort()` | 无 | 释放当前连接 | communication |
| `connected() const` | 无 | 返回连接状态 | communication |
| `load(const QString &path, QString &error)` | ELF/AXF 路径 | 填充 `variables` | elf_parser |
| `readVariable(const UiVariable &variable, QString &error)` | 变量描述 | 调用通信 COM 层发送读变量请求 | communication COM |
| `writeVariable(const UiVariable &variable, const QString &target, QString &error)` | 变量描述和目标文本 | 调用通信 COM 层发送标定请求 | communication COM |
| `poll()` | 无 | 从传输层读取一次数据并驱动协议解析 | transport + parser |
| `setResponseCallback(...)` | 响应回调 | 收到读/写响应时通知主窗口 | custom protocol |
| `onPacket` | 协议消息 | 静态回调，转发给 Qt 回调 | custom protocol |
| `onRouterProtocolReceive` | 路由上行帧 | 静态回调，喂给协议解析器 | router |
| `onRouterTransportSend` | 路由下行帧 | 静态回调，写传输层 | router/transport |
| `onTransportPacket` | 传输层收包 | 静态回调，送入路由器 | transport/router |

## 值处理边界

UI 模块不再包含值编解码接口。所有原始字节解析、十进制显示、曲线 double 转换、标定目标值编码和位字段读改写均由通信模块 COM 层处理。

## 错误处理

UI 层将 C 模块状态码转换为 `QString error` 或日志文本。页面类不直接弹出底层错误，统一由主窗口记录日志或更新状态栏。