/*
 * 文件说明：UI 界面模块对外接口声明。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <QMainWindow>
#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QVector>

#include "vm_ui_port.h"
#include "pages/vm_page_types.h"

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QAction;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QCloseEvent;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QLabel;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QListWidgetItem;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QMdiArea;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QMdiSubWindow;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QStackedWidget;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QTimer;

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class CalibrationPage;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class DevicePage;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class FilePage;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class LogPage;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class MonitorPage;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class VariableLoadPage;

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    /**
     * 函数说明：addPageDock，执行本模块对应功能逻辑。
     * 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。；body：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void addPageDock(const QString &name, QWidget *body);
    /**
     * 函数说明：showPage，显示指定页面或窗口。
     * 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void showPage(const QString &name);
    /**
     * 函数说明：showMdiToolWindow，显示指定页面或窗口。
     * 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。；body：函数输入参数，参与本函数的计算、查找或状态更新。；window：函数输入参数，参与本函数的计算、查找或状态更新。；preferredSize：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void showMdiToolWindow(const QString &name,
                           QWidget *body,
                           QMdiSubWindow *&window,
                           const QSize &preferredSize,
                           const QPoint &offset);
    /**
     * 函数说明：log，执行本模块对应功能逻辑。
     * 输入：message：协议解析后的消息对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void log(const QString &message);
    /**
     * 函数说明：refreshGate，刷新显示或使能状态。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void refreshGate();
    /**
     * 函数说明：syncVariableTables，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void syncVariableTables();
    /**
     * 函数说明：loadImage，加载外部文件或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void loadImage();
    /**
     * 函数说明：readMonitor，读取数据或发起读取请求。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void readMonitor();
    /**
     * 函数说明：startMonitor，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void startMonitor();
    /**
     * 函数说明：stopMonitor，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void stopMonitor();
    /**
     * 函数说明：pollMonitor，轮询处理异步收发事件。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void pollMonitor();
    /**
     * 函数说明：expireMonitorPending，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void expireMonitorPending();
    /**
     * 函数说明：sendMonitorReads，读取数据或发起读取请求。
     * 输入：maxRequests：函数输入参数，参与本函数的计算、查找或状态更新。；logResult：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回整数结果，具体含义由调用场景决定。
     */
    int sendMonitorReads(int maxRequests, bool logResult);
    /**
     * 函数说明：readCalibration，读取数据或发起读取请求。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void readCalibration();
    /**
     * 函数说明：writeCalibration，写入数据或发起标定请求。
     * 输入：confirm：是否执行确认流程。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void writeCalibration(bool confirm);
    /**
     * 函数说明：writeCalibrationRow，写入数据或发起标定请求。
     * 输入：row：界面表格行号。；confirm：是否执行确认流程。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void writeCalibrationRow(int row, bool confirm);
    /**
     * 函数说明：sendCalibrationRow，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。；logSuccess：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool sendCalibrationRow(int row, bool logSuccess);
    /**
     * 函数说明：writeAllCalibration，写入数据或发起标定请求。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void writeAllCalibration();
    /**
     * 函数说明：connectOrDisconnectDevice，建立连接并准备收发链路。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void connectOrDisconnectDevice();
    /**
     * 函数说明：newWorkspace，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void newWorkspace();
    /**
     * 函数说明：saveCurrentWorkspace，保存当前状态或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void saveCurrentWorkspace();
    /**
     * 函数说明：saveWorkspace，保存当前状态或配置。
     * 输入：file：文件路径或文件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void saveWorkspace(const QString &file);
    /**
     * 函数说明：writeWorkspace，写入数据或发起标定请求。
     * 输入：file：文件路径或文件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool writeWorkspace(const QString &file);
    /**
     * 函数说明：loadWorkspaceDialog，加载外部文件或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void loadWorkspaceDialog();
    /**
     * 函数说明：exportWorkspaceDialog，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void exportWorkspaceDialog();
    /**
     * 函数说明：restoreWorkspace，执行本模块对应功能逻辑。
     * 输入：file：文件路径或文件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void restoreWorkspace(const QString &file);
    /**
     * 函数说明：openRecentItem，打开底层资源。
     * 输入：item：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void openRecentItem(QListWidgetItem *item);
    /**
     * 函数说明：removeRecentFile，执行本模块对应功能逻辑。
     * 输入：file：文件路径或文件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void removeRecentFile(const QString &file);
    /**
     * 函数说明：touchRecent，执行本模块对应功能逻辑。
     * 输入：file：文件路径或文件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void touchRecent(const QString &file);
    /**
     * 函数说明：updateRecentList，刷新界面数据或内部状态。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void updateRecentList();
    /**
     * 函数说明：loadRecentState，加载外部文件或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void loadRecentState();
    /**
     * 函数说明：saveRecentState，保存当前状态或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void saveRecentState() const;
    /**
     * 函数说明：appStatePath，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString appStatePath() const;
    /**
     * 函数说明：connectionText，建立连接并准备收发链路。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString connectionText() const;


    /* 变量说明：facade_，保存当前对象运行所需的状态、参数或缓存数据。 */
    MonitorFacade facade_;
    /* 变量说明：pages_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMap<QString, QWidget *> pages_;
    /* 变量说明：toolbarActions_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMap<QString, QAction *> toolbarActions_;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QStackedWidget *workspace_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMdiArea *mdiArea_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMdiSubWindow *monitorWindow_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMdiSubWindow *calibrationWindow_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    FilePage *filePage_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    DevicePage *devicePage_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    VariableLoadPage *variablePage_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    MonitorPage *monitorPage_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    CalibrationPage *calibrationPage_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    LogPage *logPage_ = nullptr;
    /* 变量说明：monitorPages_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QVector<MonitorPage *> monitorPages_;
    /* 变量说明：calibrationPages_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QVector<CalibrationPage *> calibrationPages_;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QAction *monitorAction_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QAction *calibrationAction_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLabel *statusLabel_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QTimer *pollTimer_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QTimer *ioTimer_ = nullptr;

    /* 变量说明：loadedPath_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString loadedPath_;
    /* 变量说明：activeConfigFile_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString activeConfigFile_;
    /* 变量说明：recentFiles_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QStringList recentFiles_;
    /* 变量说明：calibrationTargets_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QMap<QString, QString> calibrationTargets_;
    /* 变量说明：monitorPendingSince_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QHash<quint64, qint64> monitorPendingSince_;
    int monitorNextRow_ = 0;
    int monitorNextCurveRow_ = 0;
    qint64 lastProtocolErrorLogMs_ = 0;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool projectOpen_ = false;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool loaded_ = false;
};
