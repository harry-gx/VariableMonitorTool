#pragma once

#include <QMainWindow>
#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QVector>

#include "vm_ui_port.h"
#include "pages/vm_page_types.h"

class QAction;
class QCloseEvent;
class QLabel;
class QListWidgetItem;
class QMdiArea;
class QMdiSubWindow;
class QStackedWidget;
class QTimer;

class CalibrationPage;
class DevicePage;
class FilePage;
class LogPage;
class MonitorPage;
class VariableLoadPage;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void addPageDock(const QString &name, QWidget *body);
    void showPage(const QString &name);
    void showMdiToolWindow(const QString &name,
                           QWidget *body,
                           QMdiSubWindow *&window,
                           const QSize &preferredSize,
                           const QPoint &offset);
    void log(const QString &message);
    void refreshGate();
    void syncVariableTables();
    void loadImage();
    void readMonitor();
    void startMonitor();
    void stopMonitor();
    void pollMonitor();
    void expireMonitorPending();
    int sendMonitorReads(int maxRequests, bool logResult);
    void readCalibration();
    void writeCalibration(bool confirm);
    void writeCalibrationRow(int row, bool confirm);
    bool sendCalibrationRow(int row, bool logSuccess);
    void writeAllCalibration();
    void connectOrDisconnectDevice();
    void newWorkspace();
    void saveCurrentWorkspace();
    void saveWorkspace(const QString &file);
    bool writeWorkspace(const QString &file);
    void loadWorkspaceDialog();
    void exportWorkspaceDialog();
    void restoreWorkspace(const QString &file);
    void openRecentItem(QListWidgetItem *item);
    void removeRecentFile(const QString &file);
    void touchRecent(const QString &file);
    void updateRecentList();
    void loadRecentState();
    void saveRecentState() const;
    QString appStatePath() const;
    QString connectionText() const;


    MonitorFacade facade_;
    QMap<QString, QWidget *> pages_;
    QMap<QString, QAction *> toolbarActions_;
    QStackedWidget *workspace_ = nullptr;
    QMdiArea *mdiArea_ = nullptr;
    QMdiSubWindow *monitorWindow_ = nullptr;
    QMdiSubWindow *calibrationWindow_ = nullptr;

    FilePage *filePage_ = nullptr;
    DevicePage *devicePage_ = nullptr;
    VariableLoadPage *variablePage_ = nullptr;
    MonitorPage *monitorPage_ = nullptr;
    CalibrationPage *calibrationPage_ = nullptr;
    LogPage *logPage_ = nullptr;
    QVector<MonitorPage *> monitorPages_;
    QVector<CalibrationPage *> calibrationPages_;

    QAction *monitorAction_ = nullptr;
    QAction *calibrationAction_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QTimer *pollTimer_ = nullptr;
    QTimer *ioTimer_ = nullptr;

    QString loadedPath_;
    QString activeConfigFile_;
    QStringList recentFiles_;
    QMap<QString, QString> calibrationTargets_;
    QHash<quint64, qint64> monitorPendingSince_;
    int monitorNextRow_ = 0;
    int monitorNextCurveRow_ = 0;
    qint64 lastProtocolErrorLogMs_ = 0;
    bool projectOpen_ = false;
    bool loaded_ = false;
};
