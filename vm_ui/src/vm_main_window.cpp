/*
 * 文件说明：主窗口实现，负责页面切换、MDI 子窗口、变量加载、监控轮询、标定发送和工程状态管理。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_main_window.h"

#include <QtWidgets>

#include "pages/vm_calibration_page.h"
#include "pages/vm_device_page.h"
#include "pages/vm_file_page.h"
#include "pages/vm_log_page.h"
#include "pages/vm_monitor_page.h"
#include "pages/vm_variable_load_page.h"

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class ToolMdiSubWindow : public QMdiSubWindow {
public:
    explicit ToolMdiSubWindow(QWidget *parent = nullptr) : QMdiSubWindow(parent) {}

protected:
    void closeEvent(QCloseEvent *event) override {
        hide();
        if (widget()) {
            widget()->show();
        }
        event->ignore();
    }
};

// 函数说明：deviceTypeToString，执行本模块对应功能逻辑。
// 输入：type：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
static QString deviceTypeToString(UiDeviceType type) {
    switch (type) {
    case UiDeviceType::Can:
        return QStringLiteral("can");
    case UiDeviceType::CanFd:
        return QStringLiteral("canfd");
    case UiDeviceType::Ethernet:
        return QStringLiteral("ethernet");
    case UiDeviceType::Serial:
    default:
        return QStringLiteral("serial");
    }
}

// 函数说明：deviceTypeFromString，执行本模块对应功能逻辑。
// 输入：text：文本内容或输入字符串。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
static UiDeviceType deviceTypeFromString(const QString &text) {
    if (text == QStringLiteral("can")) {
        return UiDeviceType::Can;
    }
    if (text == QStringLiteral("canfd")) {
        return UiDeviceType::CanFd;
    }
    if (text == QStringLiteral("ethernet")) {
        return UiDeviceType::Ethernet;
    }
    return UiDeviceType::Serial;
}

// 常量说明：kMonitorMaxRequestsPerTick 用于控制界面刷新、请求节流或超时参数。
static constexpr int kMonitorMaxRequestsPerTick = 6;
// 常量说明：kMonitorMaxBackgroundRequestsPerTick 用于控制界面刷新、请求节流或超时参数。
static constexpr int kMonitorMaxBackgroundRequestsPerTick = 2;
// 常量说明：kMonitorMaxPendingRequests 用于控制界面刷新、请求节流或超时参数。
static constexpr int kMonitorMaxPendingRequests = 12;
// 常量说明：kMonitorRequestTimeoutMs 用于控制界面刷新、请求节流或超时参数。
static constexpr qint64 kMonitorRequestTimeoutMs = 1000;
// 常量说明：kSerialIoPollIntervalMs 用于控制界面刷新、请求节流或超时参数。
static constexpr int kSerialIoPollIntervalMs = 5;

// 函数说明：requestKey，执行本模块对应功能逻辑。
// 输入：address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
static quint64 requestKey(quint32 address, quint16 size) {
    return (static_cast<quint64>(address) << 16) | static_cast<quint64>(size);
}

// 函数说明：QMainWindow，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("变量监控与标定工具"));
    resize(1200, 780);

    workspace_ = new QStackedWidget(this);
    workspace_->setObjectName(QStringLiteral("workspace"));
    workspace_->setStyleSheet(QStringLiteral("#workspace{background:#f4f6f8;}"));
    setCentralWidget(workspace_);

    filePage_ = new FilePage(this);
    devicePage_ = new DevicePage(this);
    variablePage_ = new VariableLoadPage(this);
    monitorPage_ = new MonitorPage;
    calibrationPage_ = new CalibrationPage;
    logPage_ = new LogPage(this);
    mdiArea_ = new QMdiArea(this);
    mdiArea_->setObjectName(QStringLiteral("mdiWorkspace"));
    mdiArea_->setViewMode(QMdiArea::SubWindowView);
    mdiArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea_->setBackground(QBrush(QColor(244, 246, 248)));
    mdiArea_->setStyleSheet(QStringLiteral(
        "QMdiArea{background:#f4f6f8;}"
        "QMdiSubWindow{background:#ffffff;border:1px solid #2f80ed;}"
        "QMdiSubWindow::title{background:#eef1f5;color:#344054;padding:3px 6px;font-weight:600;"
        "border-bottom:1px solid #2f80ed;}"
        "QMdiSubWindow::minimize-button,"
        "QMdiSubWindow::normal-button,"
        "QMdiSubWindow::close-button{width:18px;height:16px;border:0;background:transparent;margin:2px;}"
        "QMdiSubWindow::minimize-button:hover,"
        "QMdiSubWindow::normal-button:hover,"
        "QMdiSubWindow::close-button:hover{background:#eef2f6;border-radius:2px;}"
        "QMdiSubWindow::minimize-button{image:url(:/icons/vm_window_minimize.svg);}"
        "QMdiSubWindow::normal-button{image:url(:/icons/vm_window_maximize.svg);}"
        "QMdiSubWindow::close-button{image:url(:/icons/vm_window_close.svg);}"));
    monitorPages_.append(monitorPage_);
    calibrationPages_.append(calibrationPage_);

    addPageDock(QStringLiteral("文件"), filePage_);
    addPageDock(QStringLiteral("设备管理"), devicePage_);
    addPageDock(QStringLiteral("变量加载"), variablePage_);
    addPageDock(QStringLiteral("日志"), logPage_);
    workspace_->addWidget(mdiArea_);

    auto *toolbar = addToolBar(QStringLiteral("功能"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->setStyleSheet(QStringLiteral(
        "QToolBar{background:#f7f7f7;border:0;border-bottom:1px solid #c4c8cc;spacing:0;}"
        "QToolButton{padding:12px 22px;font-size:15px;border:0;color:#1f2933;}"
        "QToolButton:hover{background:#e7edf3;}"
        "QToolButton:checked{background:#d4e3f1;color:#0f2f4f;font-weight:600;}"
        "QToolButton:disabled{color:#8a9299;background:#f7f7f7;}"));

    const QStringList actions = QStringList() << QStringLiteral("文件")
                                              << QStringLiteral("设备管理")
                                              << QStringLiteral("变量加载")
                                              << QStringLiteral("监控")
                                              << QStringLiteral("标定")
                                              << QStringLiteral("日志");
    for (const auto &name : actions) {
        auto *action = toolbar->addAction(name);
        action->setCheckable(true);
        toolbarActions_[name] = action;
        connect(action, &QAction::triggered, this, [this, name] { showPage(name); });
        if (name == QStringLiteral("监控")) {
            monitorAction_ = action;
        } else if (name == QStringLiteral("标定")) {
            calibrationAction_ = action;
        }
    }

    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, [this] {
        pollMonitor();
    });

    ioTimer_ = new QTimer(this);
    ioTimer_->setInterval(kSerialIoPollIntervalMs);
    connect(ioTimer_, &QTimer::timeout, this, [this] {
        facade_.poll();
    });

    facade_.setResponseCallback([this](const MonitorCommEvent &event) {
        monitorPendingSince_.remove(requestKey(event.address, event.size));
        if (event.type == VM_COMM_EVENT_ERROR) {
            for (auto it = monitorPendingSince_.begin(); it != monitorPendingSince_.end();) {
                if (static_cast<quint32>(it.key() >> 16) == event.address) {
                    it = monitorPendingSince_.erase(it);
                } else {
                    ++it;
                }
            }
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - lastProtocolErrorLogMs_ > 1000) {
                lastProtocolErrorLogMs_ = now;
                const QString message = event.message.isEmpty()
                    ? QStringLiteral("收到协议错误 address=0x%1 code=0x%2")
                          .arg(event.address, 8, 16, QChar('0'))
                          .arg(event.errorCode, 2, 16, QChar('0'))
                    : event.message;
                log(message);
            }
            return;
        }
        if (event.type == VM_COMM_EVENT_VALUE) {
            for (auto *monitor : monitorPages_) {
                monitor->updateValue(event.address,
                                     event.size,
                                     event.displayValue,
                                     event.numericValue,
                                     event.hasNumericValue);
            }
            for (auto *calibration : calibrationPages_) {
                calibration->updateValue(event.address,
                                         event.size,
                                         event.displayValue,
                                         event.message.isEmpty()
                                             ? QStringLiteral("读取成功")
                                             : event.message);
            }
        }
    });

    connect(filePage_->newButton(), &QToolButton::clicked, this, [this] { newWorkspace(); });
    connect(filePage_->saveButton(), &QToolButton::clicked, this, [this] { saveCurrentWorkspace(); });
    connect(filePage_->loadButton(), &QToolButton::clicked, this, [this] { loadWorkspaceDialog(); });
    connect(filePage_->recentButton(), &QToolButton::clicked, this, [this] {
        showPage(QStringLiteral("文件"));
        updateRecentList();
    });
    connect(filePage_->exportButton(), &QToolButton::clicked, this, [this] { exportWorkspaceDialog(); });
    connect(filePage_->exitButton(), &QToolButton::clicked, this, &QWidget::close);
    connect(filePage_->recentList(), &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) { openRecentItem(item); });
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this] { saveCurrentWorkspace(); });

    connect(devicePage_->refreshButton(), &QPushButton::clicked, this, [this] {
        if (!facade_.connected()) {
            devicePage_->refreshPorts();
        }
    });
    connect(devicePage_->connectButton(), &QPushButton::clicked, this,
            [this] { connectOrDisconnectDevice(); });

    connect(variablePage_->loadButton(), &QPushButton::clicked, this, [this] { loadImage(); });
    connect(variablePage_->table(), &QTableWidget::itemChanged, this,
            [this] { syncVariableTables(); });

    connect(monitorPage_->readButton(), &QPushButton::clicked, this, [this] { readMonitor(); });
    connect(monitorPage_->startButton(), &QPushButton::clicked, this, [this] { startMonitor(); });
    connect(monitorPage_->stopButton(), &QPushButton::clicked, this, [this] { stopMonitor(); });

    calibrationPage_->setCalibrateRowCallback([this](int row) {
        writeCalibrationRow(row, false);
    });
    calibrationPage_->setCalibrateAllCallback([this] {
        writeAllCalibration();
    });

    connect(logPage_->exportButton(), &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出日志"),
                                                          QStringLiteral("runtime.log"));
        if (!path.isEmpty() && !logPage_->exportAll(path)) {
            log(QStringLiteral("日志导出失败"));
        }
    });

    statusLabel_ = new QLabel(this);
    statusBar()->addWidget(statusLabel_);
    statusBar()->addPermanentWidget(new QLabel(QStringLiteral("v0.1.0"), this));

    loadRecentState();
    refreshGate();
    log(QStringLiteral("软件启动"));
    showPage(QStringLiteral("文件"));
}

// 函数说明：MainWindow::closeEvent，关闭底层资源。
// 输入：event：UI 或通信模块事件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::closeEvent(QCloseEvent *event) {
    saveRecentState();
    if (pollTimer_) {
        pollTimer_->stop();
    }
    if (ioTimer_) {
        ioTimer_->stop();
    }
    if (facade_.connected()) {
        facade_.disconnectPort();
    }
    QMainWindow::closeEvent(event);
}

// 函数说明：MainWindow::addPageDock，执行本模块对应功能逻辑。
// 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。；body：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::addPageDock(const QString &name, QWidget *body) {
    body->setObjectName(name);
    workspace_->addWidget(body);
    pages_[name] = body;
}

// 函数说明：MainWindow::showPage，显示指定页面或窗口。
// 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::showPage(const QString &name) {
    if (!projectOpen_ && name != QStringLiteral("文件")) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }

    const bool ready = projectOpen_ && facade_.connected() && loaded_;
    if (name == QStringLiteral("监控")) {
        if (!ready) {
            log(QStringLiteral("请先完成工程配置、设备连接和变量加载"));
            return;
        }
        showMdiToolWindow(name, monitorPage_, monitorWindow_, QSize(920, 520), QPoint(24, 24));
        return;
    }
    if (name == QStringLiteral("标定")) {
        if (!ready) {
            log(QStringLiteral("请先完成工程配置、设备连接和变量加载"));
            return;
        }
        showMdiToolWindow(name, calibrationPage_, calibrationWindow_, QSize(560, 460), QPoint(80, 72));
        return;
    }

    auto *page = pages_.value(name, nullptr);
    if (!page) {
        return;
    }
    workspace_->setCurrentWidget(page);
    for (auto it = toolbarActions_.begin(); it != toolbarActions_.end(); ++it) {
        it.value()->setChecked(it.key() == name);
    }
}

// 函数说明：MainWindow::showMdiToolWindow，显示指定页面或窗口。
// 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。；body：函数输入参数，参与本函数的计算、查找或状态更新。；window：函数输入参数，参与本函数的计算、查找或状态更新。；preferredSize：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::showMdiToolWindow(const QString &name,
                                   QWidget *body,
                                   QMdiSubWindow *&window,
                                   const QSize &preferredSize,
                                   const QPoint &offset) {
    workspace_->setCurrentWidget(mdiArea_);
    for (auto it = toolbarActions_.begin(); it != toolbarActions_.end(); ++it) {
        it.value()->setChecked(it.key() == name);
    }

    if (!window) {
        auto *toolWindow = new ToolMdiSubWindow;
        toolWindow->setWidget(body);
        window = toolWindow;
        mdiArea_->addSubWindow(window);
        window->setWindowTitle(name);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        window->setOption(QMdiSubWindow::RubberBandMove, true);
        window->setOption(QMdiSubWindow::RubberBandResize, true);
        window->resize(preferredSize);

        const QSize areaSize = mdiArea_->viewport()->size();
        const int x = qMax(0, (areaSize.width() - preferredSize.width()) / 2 + offset.x());
        const int y = qMax(0, (areaSize.height() - preferredSize.height()) / 2 + offset.y());
        window->move(x, y);
    }

    if (window->widget()) {
        window->widget()->show();
    }
    body->show();
    window->showNormal();
    window->raise();
    mdiArea_->setActiveSubWindow(window);
}

// 函数说明：MainWindow::log，执行本模块对应功能逻辑。
// 输入：message：协议解析后的消息对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::log(const QString &message) {
    const QString line = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
        + QStringLiteral("  ") + message;
    logPage_->appendLine(line);
    statusBar()->showMessage(message, 5000);
}

// 函数说明：MainWindow::refreshGate，刷新显示或使能状态。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::refreshGate() {
    const bool connected = facade_.connected();
    const bool ready = projectOpen_ && connected && loaded_;
    for (auto it = toolbarActions_.begin(); it != toolbarActions_.end(); ++it) {
        it.value()->setEnabled(it.key() == QStringLiteral("文件") || projectOpen_);
    }
    if (monitorAction_) {
        monitorAction_->setEnabled(ready);
    }
    if (calibrationAction_) {
        calibrationAction_->setEnabled(ready);
    }
    filePage_->setProjectOpen(projectOpen_);
    devicePage_->setConnected(connected);
    statusLabel_->setText(connectionText());
    for (auto *monitor : monitorPages_) {
        monitor->setReady(ready);
    }
    for (auto *calibration : calibrationPages_) {
        calibration->setReady(ready);
    }
    updateRecentList();
}

// 函数说明：MainWindow::syncVariableTables，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::syncVariableTables() {
    calibrationTargets_ = calibrationPage_->collectTargets();
    // 关键步骤：启动监控前清空未完成请求和扫描游标，避免上一次监控残留影响当前刷新。
    monitorPendingSince_.clear();
    monitorNextRow_ = 0;
    monitorNextCurveRow_ = 0;
    for (auto *monitor : monitorPages_) {
        monitor->setRows(variablePage_->selectedMonitorVariables());
    }
    for (auto *calibration : calibrationPages_) {
        calibration->setRows(variablePage_->selectedCalibrationVariables(), calibrationTargets_);
    }
    refreshGate();
}

// 函数说明：MainWindow::loadImage，加载外部文件或配置。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::loadImage() {
    if (!projectOpen_) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }
    QString error;
    if (!facade_.load(variablePage_->imagePath(), error)) {
        log(error);
        return;
    }
    loaded_ = true;
    loadedPath_ = variablePage_->imagePath();
    calibrationTargets_.clear();
    variablePage_->setVariables(facade_.variables);
    syncVariableTables();
    log(QStringLiteral("变量加载完成（当前为符号表变量）"));
}

// 函数说明：MainWindow::readMonitor，读取数据或发起读取请求。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::readMonitor() {
    const int sent = sendMonitorReads(monitorPage_->rowCount(), true);
    if (sent > 0) {
        log(QStringLiteral("监控读取请求已发送：%1 个变量").arg(sent));
    }
}

// 函数说明：MainWindow::startMonitor，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::startMonitor() {
    if (!facade_.connected() || monitorPage_->rowCount() <= 0) {
        log(QStringLiteral("当前没有可监控变量或设备未连接"));
        return;
    }

    monitorPendingSince_.clear();
    monitorNextRow_ = 0;
    monitorNextCurveRow_ = 0;
    pollTimer_->start(monitorPage_->periodSpinBox()->value());
    pollMonitor();
    log(QStringLiteral("开始监控"));
}

// 函数说明：MainWindow::stopMonitor，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::stopMonitor() {
    const bool wasActive = pollTimer_->isActive();
    pollTimer_->stop();
    monitorPendingSince_.clear();
    if (wasActive) {
        log(QStringLiteral("停止监控"));
    }
}

// 函数说明：MainWindow::pollMonitor，轮询处理异步收发事件。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::pollMonitor() {
    facade_.poll();
    if (!pollTimer_->isActive()) {
        return;
    }
    // 关键步骤：每次轮询先清理超时请求，再按窗口限制补发新请求，避免串口请求堆积造成卡顿。
    expireMonitorPending();
    sendMonitorReads(kMonitorMaxRequestsPerTick, false);
}

// 函数说明：MainWindow::expireMonitorPending，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::expireMonitorPending() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = monitorPendingSince_.begin(); it != monitorPendingSince_.end();) {
        if (now - it.value() >= kMonitorRequestTimeoutMs) {
            it = monitorPendingSince_.erase(it);
        } else {
            ++it;
        }
    }
}

// 函数说明：MainWindow::sendMonitorReads，读取数据或发起读取请求。
// 输入：maxRequests：函数输入参数，参与本函数的计算、查找或状态更新。；logResult：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回整数结果，具体含义由调用场景决定。
int MainWindow::sendMonitorReads(int maxRequests, bool logResult) {
    const int monitorRows = monitorPage_->rowCount();
    const int calibrationRows = calibrationPage_->rowCount();
    const int rows = monitorRows + calibrationRows;
    if (rows <= 0 || maxRequests <= 0) {
        return 0;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    int sent = 0;
    bool stopByError = false;

    expireMonitorPending();

    auto trySend = [&](bool calibrationRow, int pageRow) -> bool {
        if (pageRow < 0) {
            return false;
        }

        const UiVariable variable = calibrationRow
            ? calibrationPage_->variableAt(pageRow)
            : monitorPage_->variableAt(pageRow);
        const quint32 address = calibrationRow
            ? calibrationPage_->addressAt(pageRow)
            : monitorPage_->addressAt(pageRow);
        const quint16 size = calibrationRow
            ? calibrationPage_->sizeAt(pageRow)
            : monitorPage_->sizeAt(pageRow);
        if (size == 0) {
            return false;
        }

        const quint64 pendingKey = requestKey(address, size);
        if (monitorPendingSince_.contains(pendingKey)) {
            return false;
        }

        if (monitorPendingSince_.size() >= kMonitorMaxPendingRequests) {
            return false;
        }

        QString error;
        if (!facade_.readVariable(variable, error)) {
            if (logResult) {
                log(error);
            } else {
                pollTimer_->stop();
                monitorPendingSince_.clear();
                log(error);
                log(QStringLiteral("监控已停止"));
            }
            stopByError = true;
            return false;
        }

        monitorPendingSince_.insert(pendingKey, now);
        ++sent;
        return true;
    };

    const QVector<int> checkedRows = logResult ? QVector<int>() : monitorPage_->checkedRows();
    QSet<int> checkedSet;
    for (int row : checkedRows) {
        checkedSet.insert(row);
    }

    if (!checkedRows.isEmpty()) {
        int visited = 0;
        while (visited < checkedRows.size() && sent < maxRequests && !stopByError &&
               monitorPendingSince_.size() < kMonitorMaxPendingRequests) {
            const int index = monitorNextCurveRow_ % checkedRows.size();
            monitorNextCurveRow_ = (monitorNextCurveRow_ + 1) % checkedRows.size();
            ++visited;
            trySend(false, checkedRows.at(index));
        }
    }

    const int backgroundBudget = checkedRows.isEmpty()
        ? (maxRequests - sent)
        : qMin(kMonitorMaxBackgroundRequestsPerTick, maxRequests - sent);
    int backgroundSent = 0;
    int visited = 0;

    while (visited < rows && backgroundSent < backgroundBudget && !stopByError &&
           monitorPendingSince_.size() < kMonitorMaxPendingRequests) {
        const int row = monitorNextRow_ % rows;
        monitorNextRow_ = (monitorNextRow_ + 1) % rows;
        ++visited;

        const bool calibrationRow = row >= monitorRows;
        const int pageRow = calibrationRow ? row - monitorRows : row;
        if (!calibrationRow && checkedSet.contains(pageRow)) {
            continue;
        }

        const int before = sent;
        trySend(calibrationRow, pageRow);
        if (sent > before) {
            ++backgroundSent;
        }
    }

    return sent;
}

// 函数说明：MainWindow::readCalibration，读取数据或发起读取请求。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::readCalibration() {
    const int row = calibrationPage_->currentRow();
    if (row < 0) {
        log(QStringLiteral("请先选择标定变量"));
        return;
    }
    QString error;
    if (!facade_.readVariable(calibrationPage_->variableAt(row), error)) {
        log(error);
    } else {
        log(QStringLiteral("标定读取请求已发送"));
    }
}

// 函数说明：MainWindow::writeCalibration，写入数据或发起标定请求。
// 输入：confirm：是否执行确认流程。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::writeCalibration(bool confirm) {
    const int row = calibrationPage_->currentRow();
    writeCalibrationRow(row, confirm);
}

// 函数说明：MainWindow::writeCalibrationRow，写入数据或发起标定请求。
// 输入：row：界面表格行号。；confirm：是否执行确认流程。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::writeCalibrationRow(int row, bool confirm) {
    if (sendCalibrationRow(row, true) && confirm) {
        readCalibration();
    }
}

// 函数说明：MainWindow::sendCalibrationRow，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。；logSuccess：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool MainWindow::sendCalibrationRow(int row, bool logSuccess) {
    if (row < 0) {
        log(QStringLiteral("请先选择标定变量"));
        return false;
    }

    const UiVariable variable = calibrationPage_->variableAt(row);
    const QString target = calibrationPage_->targetTextAt(row).trimmed();
    if (target.isEmpty()) {
        log(QStringLiteral("请先输入目标值：%1").arg(variable.name));
        return false;
    }

    QString error;
    if (!facade_.writeVariable(variable, target, error)) {
        log(error);
        return false;
    }

    if (logSuccess) {
        log(QStringLiteral("标定请求已发送：%1 = %2").arg(variable.name, target));
    }
    return true;
}

// 函数说明：MainWindow::writeAllCalibration，写入数据或发起标定请求。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::writeAllCalibration() {
    if (calibrationPage_->rowCount() <= 0) {
        log(QStringLiteral("当前没有可标定变量"));
        return;
    }

    int skipped = 0;
    int success = 0;
    int failed = 0;

    // 关键步骤：一键标定逐行发送已有目标值，空目标值行跳过，避免误写未编辑变量。
    for (int row = 0; row < calibrationPage_->rowCount(); ++row) {
        if (calibrationPage_->targetTextAt(row).trimmed().isEmpty()) {
            ++skipped;
            continue;
        }

        if (sendCalibrationRow(row, false)) {
            ++success;
        } else {
            ++failed;
        }
    }

    if (success == 0 && failed == 0) {
        log(QStringLiteral("没有可标定的目标值"));
        return;
    }

    log(QStringLiteral("一键标定已发送：成功 %1 个，失败 %2 个，跳过空目标值 %3 个")
            .arg(success)
            .arg(failed)
            .arg(skipped));
}

// 函数说明：MainWindow::connectOrDisconnectDevice，建立连接并准备收发链路。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::connectOrDisconnectDevice() {
    if (!projectOpen_) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }
    if (facade_.connected()) {
        stopMonitor();
        if (ioTimer_) {
            ioTimer_->stop();
        }
        facade_.disconnectPort();
    } else {
        QString error;
        MonitorDeviceConfig config;
        switch (devicePage_->deviceType()) {
        case UiDeviceType::Can:
            config.type = MonitorDeviceType::Can;
            break;
        case UiDeviceType::CanFd:
            config.type = MonitorDeviceType::CanFd;
            break;
        case UiDeviceType::Ethernet:
            config.type = MonitorDeviceType::Ethernet;
            break;
        case UiDeviceType::Serial:
        default:
            config.type = MonitorDeviceType::Serial;
            break;
        }
        config.serialPort = devicePage_->portName();
        config.serialBaud = devicePage_->baudRate();
        config.canAdapter = devicePage_->canAdapter();
        config.canChannel = devicePage_->canChannel().toUInt();
        config.canBaud = devicePage_->canBaudRate();
        config.canFdDataBaud = devicePage_->canFdDataBaudRate();
        config.networkHost = devicePage_->networkHost();
        config.networkPort = devicePage_->networkPort();

        if (!facade_.connectDevice(config, error)) {
            log(error);
        } else if (ioTimer_) {
            ioTimer_->start();
        }
    }
    refreshGate();
}

// 函数说明：MainWindow::newWorkspace，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::newWorkspace() {
    QString path = QFileDialog::getSaveFileName(this,
                                                QStringLiteral("新建工程配置"),
                                                QStringLiteral("workspace.json"),
                                                QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        log(QStringLiteral("已取消新建工程配置"));
        return;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".json");
    }

    if (facade_.connected()) {
        pollTimer_->stop();
        if (ioTimer_) {
            ioTimer_->stop();
        }
        facade_.disconnectPort();
    }
    loaded_ = false;
    loadedPath_.clear();
    calibrationTargets_.clear();
    variablePage_->setImagePath(QString());
    variablePage_->clearVariables();
    syncVariableTables();
    pollTimer_->stop();
    activeConfigFile_ = QFileInfo(path).absoluteFilePath();
    projectOpen_ = true;
    saveWorkspace(activeConfigFile_);
    refreshGate();
    showPage(QStringLiteral("文件"));
    log(QStringLiteral("已新建工程配置"));
}

// 函数说明：MainWindow::saveWorkspace，保存当前状态或配置。
// 输入：file：文件路径或文件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::saveWorkspace(const QString &file) {
    if (!projectOpen_) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }
    if (writeWorkspace(file)) {
        activeConfigFile_ = QFileInfo(file).absoluteFilePath();
        touchRecent(file);
        log(QStringLiteral("配置已保存"));
    } else {
        log(QStringLiteral("保存失败"));
    }
}

// 函数说明：MainWindow::saveCurrentWorkspace，保存当前状态或配置。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::saveCurrentWorkspace() {
    if (!projectOpen_ || activeConfigFile_.isEmpty()) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }
    saveWorkspace(activeConfigFile_);
}

// 函数说明：MainWindow::writeWorkspace，写入数据或发起标定请求。
// 输入：file：文件路径或文件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool MainWindow::writeWorkspace(const QString &file) {
    QJsonObject object;
    object[QStringLiteral("version")] = 1;
    object[QStringLiteral("geometry")] = QString(saveGeometry().toBase64());
    object[QStringLiteral("layout")] = QString(saveState().toBase64());
    object[QStringLiteral("image")] = loadedPath_;
    object[QStringLiteral("period_ms")] = monitorPage_->periodSpinBox()->value();
    object[QStringLiteral("device_type")] = deviceTypeToString(devicePage_->deviceType());
    object[QStringLiteral("port")] = devicePage_->portName();
    object[QStringLiteral("baud")] = QString::number(devicePage_->baudRate());
    object[QStringLiteral("can_adapter")] = devicePage_->canAdapter();
    object[QStringLiteral("can_channel")] = devicePage_->canChannel();
    object[QStringLiteral("can_baud")] = QString::number(devicePage_->canBaudRate());
    object[QStringLiteral("canfd_data_baud")] = QString::number(devicePage_->canFdDataBaudRate());
    object[QStringLiteral("network_host")] = devicePage_->networkHost();
    object[QStringLiteral("network_port")] = QString::number(devicePage_->networkPort());
    object[QStringLiteral("variables")] = variablePage_->selectionJson();

    QJsonObject targets;
    const auto currentTargets = calibrationPage_->collectTargets();
    for (auto it = currentTargets.begin(); it != currentTargets.end(); ++it) {
        targets[it.key()] = it.value();
    }
    object[QStringLiteral("targets")] = targets;

    QSaveFile saveFile(file);
    const QByteArray bytes = QJsonDocument(object).toJson();
    return saveFile.open(QIODevice::WriteOnly) && saveFile.write(bytes) == bytes.size() && saveFile.commit();
}

// 函数说明：MainWindow::loadWorkspaceDialog，加载外部文件或配置。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::loadWorkspaceDialog() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("加载配置"),
                                                      QString(), QStringLiteral("JSON (*.json)"));
    if (!path.isEmpty()) {
        restoreWorkspace(path);
    }
}

// 函数说明：MainWindow::exportWorkspaceDialog，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::exportWorkspaceDialog() {
    if (!projectOpen_) {
        log(QStringLiteral("请先新建或加载工程配置"));
        showPage(QStringLiteral("文件"));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出配置"),
                                                      QStringLiteral("workspace.json"),
                                                      QStringLiteral("JSON (*.json)"));
    if (!path.isEmpty()) {
        if (writeWorkspace(path)) {
            log(QStringLiteral("工程配置已导出"));
        } else {
            log(QStringLiteral("导出失败"));
        }
    }
}

// 函数说明：MainWindow::restoreWorkspace，执行本模块对应功能逻辑。
// 输入：file：文件路径或文件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::restoreWorkspace(const QString &file) {
    QFile input(file);
    if (!input.open(QIODevice::ReadOnly)) {
        log(QStringLiteral("工程配置不存在或无法打开：%1").arg(QDir::toNativeSeparators(file)));
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || document.object()[QStringLiteral("version")].toInt() != 1) {
        log(QStringLiteral("配置格式无效"));
        return;
    }
    if (facade_.connected()) {
        log(QStringLiteral("请先断开设备再加载配置"));
        return;
    }

    const QJsonObject object = document.object();
    activeConfigFile_ = QFileInfo(file).absoluteFilePath();
    projectOpen_ = true;
    loaded_ = false;
    loadedPath_.clear();
    calibrationTargets_.clear();
    variablePage_->clearVariables();
    syncVariableTables();

    monitorPage_->periodSpinBox()->setValue(object[QStringLiteral("period_ms")].toInt(100));
    variablePage_->setImagePath(object[QStringLiteral("image")].toString());
    if (!variablePage_->imagePath().isEmpty()) {
        loadImage();
    }

    calibrationTargets_.clear();
    const QJsonObject targets = object[QStringLiteral("targets")].toObject();
    for (auto it = targets.begin(); it != targets.end(); ++it) {
        calibrationTargets_[it.key()] = it.value().toString();
    }
    variablePage_->applySelection(object[QStringLiteral("variables")].toArray());
    syncVariableTables();

    devicePage_->setDeviceType(deviceTypeFromString(object[QStringLiteral("device_type")].toString()));
    devicePage_->setPortName(object[QStringLiteral("port")].toString());
    devicePage_->setBaudRate(object[QStringLiteral("baud")].toString());
    devicePage_->setCanAdapter(object[QStringLiteral("can_adapter")].toString());
    devicePage_->setCanChannel(object[QStringLiteral("can_channel")].toString());
    devicePage_->setCanBaudRate(object[QStringLiteral("can_baud")].toString());
    devicePage_->setCanFdDataBaudRate(object[QStringLiteral("canfd_data_baud")].toString());
    devicePage_->setNetworkHost(object[QStringLiteral("network_host")].toString());
    devicePage_->setNetworkPort(object[QStringLiteral("network_port")].toString());
    restoreGeometry(QByteArray::fromBase64(object[QStringLiteral("geometry")].toString().toLatin1()));
    restoreState(QByteArray::fromBase64(object[QStringLiteral("layout")].toString().toLatin1()));

    touchRecent(file);
    refreshGate();
    showPage(QStringLiteral("文件"));
}

// 函数说明：MainWindow::openRecentItem，打开底层资源。
// 输入：item：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::openRecentItem(QListWidgetItem *item) {
    if (item) {
        restoreWorkspace(item->data(Qt::UserRole).toString());
    }
}

// 函数说明：MainWindow::removeRecentFile，执行本模块对应功能逻辑。
// 输入：file：文件路径或文件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::removeRecentFile(const QString &file) {
    recentFiles_.removeAll(QDir::toNativeSeparators(QFileInfo(file).absoluteFilePath()));
    recentFiles_.removeAll(file);
    saveRecentState();
    updateRecentList();
}

// 函数说明：MainWindow::touchRecent，执行本模块对应功能逻辑。
// 输入：file：文件路径或文件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::touchRecent(const QString &file) {
    const QString path = QDir::toNativeSeparators(QFileInfo(file).absoluteFilePath());
    recentFiles_.removeAll(path);
    recentFiles_.prepend(path);
    while (recentFiles_.size() > 10) {
        recentFiles_.removeLast();
    }
    saveRecentState();
    updateRecentList();
}

// 函数说明：MainWindow::updateRecentList，刷新界面数据或内部状态。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::updateRecentList() {
    if (filePage_) {
        filePage_->setRecentFiles(recentFiles_, activeConfigFile_,
                                  [this](const QString &path) { restoreWorkspace(path); },
                                  [this](const QString &path) { removeRecentFile(path); });
    }
}

// 函数说明：MainWindow::loadRecentState，加载外部文件或配置。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::loadRecentState() {
    QFile input(appStatePath());
    if (!input.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return;
    }
    recentFiles_.clear();
    for (const auto &value : document.object()[QStringLiteral("recent_files")].toArray()) {
        const QString path = value.toString();
        if (!path.isEmpty() && !recentFiles_.contains(path)) {
            recentFiles_.append(path);
        }
    }
}

// 函数说明：MainWindow::saveRecentState，保存当前状态或配置。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MainWindow::saveRecentState() const {
    QJsonArray recent;
    for (const auto &path : recentFiles_) {
        recent.append(path);
    }
    QJsonObject object;
    object[QStringLiteral("recent_files")] = recent;

    QSaveFile output(appStatePath());
    const QByteArray bytes = QJsonDocument(object).toJson();
    if (output.open(QIODevice::WriteOnly)) {
        output.write(bytes);
        output.commit();
    }
}

// 函数说明：MainWindow::appStatePath，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString MainWindow::appStatePath() const {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return directory + QStringLiteral("/recent.json");
}

// 函数说明：MainWindow::connectionText，建立连接并准备收发链路。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString MainWindow::connectionText() const {
    return facade_.connected()
        ? QStringLiteral("已连接：%1").arg(devicePage_->portName())
        : QStringLiteral("未连接");
}


