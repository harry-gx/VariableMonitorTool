#pragma once

#include <functional>

#include <QByteArray>
#include <QString>
#include <QVector>

#include "pages/vm_page_types.h"
#include "vm_communication.h"

struct MonitorVariable {
    QString name;
    QString typeName;
    quint64 address = 0;
    quint64 size = 0;
    bool writable = false;
    bool monitorable = true;
    bool calibratable = false;
    bool bitField = false;
    quint8 bitOffset = 0;
    quint8 bitSize = 0;
};

enum class MonitorDeviceType {
    Serial,
    Can,
    CanFd,
    Ethernet
};

struct MonitorDeviceConfig {
    MonitorDeviceType type = MonitorDeviceType::Serial;

    QString serialPort;
    unsigned serialBaud = 115200;

    QString canAdapter;
    unsigned canChannel = 0;
    unsigned canBaud = 500000;
    unsigned canFdDataBaud = 2000000;

    QString networkHost;
    unsigned networkPort = 0;
};

struct MonitorCommEvent {
    vm_comm_event_type_t type = VM_COMM_EVENT_ERROR;
    quint8 protocolCommand = 0;
    quint8 errorCode = 0;
    quint32 address = 0;
    quint16 size = 0;
    QString name;
    QString displayValue;
    QString message;
    double numericValue = 0.0;
    bool hasNumericValue = false;
};

class MonitorFacade {
public:
    MonitorFacade();
    ~MonitorFacade();

    bool connectDevice(const MonitorDeviceConfig &config, QString &error);
    bool connectPort(const QString &port, unsigned baud, QString &error);
    void disconnectPort();
    bool connected() const;

    bool load(const QString &path, QString &error);
    bool readVariable(const UiVariable &variable, QString &error);
    bool writeVariable(const UiVariable &variable, const QString &target, QString &error);
    void poll();

    void setResponseCallback(std::function<void(const MonitorCommEvent &)> callback);

    QVector<MonitorVariable> variables;

private:
    static void onEvent(void *context, const vm_comm_event_t *event);
    static bool fillCommVariable(const UiVariable &variable,
                                 vm_comm_variable_t &out,
                                 QByteArray &name,
                                 QByteArray &typeName,
                                 QString &error);

    vm_comm_t *client_ = nullptr;
    std::function<void(const MonitorCommEvent &)> responseCallback_;
};
