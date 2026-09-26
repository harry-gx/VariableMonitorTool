#include <QApplication>
#include <QTimer>

#include "vm_main_window.h"

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
