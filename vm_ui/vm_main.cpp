/*
 * 文件说明：Qt UI 程序入口，创建 QApplication 和主窗口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include <QApplication>
#include <QTimer>

#include "vm_main_window.h"

// 函数说明：main，执行本模块对应功能逻辑。
// 输入：argc：命令行参数数量。；argv：命令行参数数组。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回整数结果，具体含义由调用场景决定。
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("VariableMonitorTool"));

    MainWindow window;
    window.show();

    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        QTimer::singleShot(800, &app, &QApplication::quit);
    }

    return app.exec();
}
