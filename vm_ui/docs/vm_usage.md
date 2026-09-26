# UI 模块使用说明

## 运行前依赖

- Qt 5.9 MinGW 32-bit，默认路径为 `C:\Qt\Qt5.9.0\5.9\mingw53_32\bin`。
- MinGW 工具链，默认路径为 `C:\Qt\Qt5.9.0\Tools\mingw530_32\bin`。
- C 模块源码位于同一工程根目录下的 `vm_elf_parser/` 和 `vm_com/`。
- Windows 串口驱动已安装，目标 MCU 已烧录支持自定义协议的固件。

## 构建输入

- qmake 工程：`vm_ui/vm_VariableMonitorTool.pro`
- UI 源码：`vm_ui/include/`、`vm_ui/src/`、`vm_ui/port/`、`vm_ui/resources/`
- C 模块构建产物：统一构建脚本会先用各模块 Makefile 生成 `build/vm_elf_parser_make/libvm_elf_parser.a`、`build/vm_com_make/libvm_communication.a` 以及 `build/*_make/include/` 下的公开头文件。UI 只引用 build 目录中的 C 模块头文件和静态库。

## 构建命令

整个工程统一使用 `scripts/vm_build_all.bat` 构建和打包。这个脚本可以直接双击执行；不要在顶层目录或 UI 目录手工运行 qmake，也不要使用顶层 Makefile；顶层 Makefile 已删除。

编译两个 C 模块、编译 UI 并完成打包：

直接双击 `scripts/vm_build_all.bat`。

清理 Qt 构建和打包目录后重新编译打包：

在命令行执行 `scripts\vm_build_all.bat clean`。

输出目录统一在顶层 `build/` 下：

- Qt 可执行文件：`build/qt/bin/VariableMonitorTool.exe`
- 打包目录：`build/package/VariableMonitorTool/`
- qmake/moc/rcc/object 等中间文件：`build/qt/`

## 运行输入

运行 UI 后需要输入或选择：

1. 设备参数：串口号和波特率；CAN、CANFD、网口界面参数当前保留。
2. ELF/AXF 文件路径：用于解析全局变量地址、长度和类型。
3. 变量选择：在变量加载页勾选加入监控或加入标定。
4. 监控周期：单位 ms。
5. 标定目标值：十进制整数或小数，位字段按整数输入。

## 运行输出

- 监控页表格显示当前值，曲线区域显示勾选变量的实时曲线。
- 标定页显示当前值、目标值和单行/一键标定按钮。
- 日志页显示软件启动、配置保存、连接、加载、读写请求和错误信息。
- 工作区配置文件保存变量选择、目标值、设备参数和最近文件记录。

## 常用操作流程

1. 打开软件。
2. 进入“设备管理”，选择串口和波特率，点击连接。
3. 进入“变量加载”，选择 MCU 工程编译出的 `.elf` 或 `.axf` 文件，点击加载变量。
4. 勾选变量加入监控或标定。
5. 点击“监控”打开监控子窗口，设置采样周期，点击开始监控。
6. 点击“标定”打开标定子窗口，输入目标值，点击行内“标定”或“一键标定”。

## 使用约束

- UI 模块只使用 `.pro` 编译。
- C 模块不通过 `.pro` 独立编译，独立编译时使用各自 `Makefile`。
- 若值显示为 `-`，表示当前变量没有读到数据或通信模块 COM 层无法按变量元信息解释该原始字节。
- 位字段标定由通信模块 COM 层自动先读当前存储单元，再只修改目标 bit 位。
