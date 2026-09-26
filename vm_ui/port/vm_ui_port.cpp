#include "vm_ui_port.h"

#include <QByteArray>

#include "vm_monitor_variables.h"

static quint32 addressFromText(const QString &text, bool &ok) {
    QString trimmed = text.trimmed();
    int base = 10;
    if (trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        trimmed = trimmed.mid(2);
        base = 16;
    }
    const quint64 value = trimmed.toULongLong(&ok, base);
    return ok ? static_cast<quint32>(value) : 0u;
}

MonitorFacade::MonitorFacade() {
    vm_comm_create(&client_);
}

MonitorFacade::~MonitorFacade() {
    vm_comm_destroy(client_);
    client_ = nullptr;
}

bool MonitorFacade::connected() const {
    return vm_comm_is_connected(client_) != 0u;
}

void MonitorFacade::disconnectPort() {
    vm_comm_disconnect(client_);
}

bool MonitorFacade::connectPort(const QString &port, unsigned baud, QString &error) {
    MonitorDeviceConfig config;
    config.type = MonitorDeviceType::Serial;
    config.serialPort = port;
    config.serialBaud = baud;
    return connectDevice(config, error);
}

bool MonitorFacade::connectDevice(const MonitorDeviceConfig &config, QString &error) {
    if (!client_) {
        error = QStringLiteral("通信模块创建失败");
        return false;
    }

    QByteArray serialPort = config.serialPort.toLocal8Bit();
    QByteArray canAdapter = config.canAdapter.toUtf8();
    QByteArray networkHost = config.networkHost.toUtf8();
    vm_comm_device_config_t commConfig = {};

    switch (config.type) {
    case MonitorDeviceType::Can:
        commConfig.type = VM_COMM_DEVICE_CAN;
        break;
    case MonitorDeviceType::CanFd:
        commConfig.type = VM_COMM_DEVICE_CANFD;
        break;
    case MonitorDeviceType::Ethernet:
        commConfig.type = VM_COMM_DEVICE_ETHERNET;
        break;
    case MonitorDeviceType::Serial:
    default:
        commConfig.type = VM_COMM_DEVICE_SERIAL;
        break;
    }

    commConfig.serial_device = serialPort.constData();
    commConfig.serial_baudrate = config.serialBaud;
    commConfig.serial_data_bits = 8u;
    commConfig.serial_stop_bits = 1u;
    commConfig.serial_parity = 0u;
    commConfig.serial_flow_control = 0u;
    commConfig.can_adapter = canAdapter.constData();
    commConfig.can_channel = config.canChannel;
    commConfig.can_baudrate = config.canBaud;
    commConfig.canfd_data_baudrate = config.canFdDataBaud;
    commConfig.network_host = networkHost.constData();
    commConfig.network_port = static_cast<uint16_t>(config.networkPort);

    if (vm_comm_set_device_config(client_, &commConfig) != VM_OK) {
        error = QStringLiteral("通信参数无效");
        return false;
    }

    const vm_status_t status = vm_comm_connect(client_);
    if (status == VM_UNSUPPORTED) {
        if (config.type == MonitorDeviceType::Can) {
            error = QStringLiteral("CAN 底层 C 适配接口已留好，驱动接入后在通信模块中完成连接。");
        } else if (config.type == MonitorDeviceType::CanFd) {
            error = QStringLiteral("CANFD 底层 C 适配接口已留好，驱动接入后在通信模块中完成连接。");
        } else {
            error = QStringLiteral("网口底层 C 适配接口已留好，TCP/UDP 接入后在通信模块中完成连接。");
        }
        return false;
    }
    if (status != VM_OK) {
        error = QStringLiteral("打开设备失败，请检查端口、参数和占用状态");
        return false;
    }

    vm_comm_set_event_callback(client_, &MonitorFacade::onEvent, this);
    return true;
}

void MonitorFacade::setResponseCallback(std::function<void(const MonitorCommEvent &)> callback) {
    responseCallback_ = std::move(callback);
}

bool MonitorFacade::fillCommVariable(const UiVariable &variable,
                                      vm_comm_variable_t &out,
                                      QByteArray &name,
                                      QByteArray &typeName,
                                      QString &error) {
    bool ok = false;
    const quint32 address = addressFromText(variable.address, ok);
    if (!ok) {
        error = QStringLiteral("变量地址无效：%1").arg(variable.name);
        return false;
    }

    const quint64 sizeValue = variable.size.toULongLong(&ok, 10);
    if (!ok || sizeValue == 0u || sizeValue > 1024u) {
        error = QStringLiteral("变量长度无效：%1").arg(variable.name);
        return false;
    }

    name = variable.name.toUtf8();
    typeName = variable.typeName.toUtf8();
    out = {};
    out.name = name.constData();
    out.type_name = typeName.constData();
    out.address = address;
    out.size = static_cast<uint16_t>(sizeValue);
    out.writable = variable.writable ? 1u : 0u;
    out.monitorable = 1u;
    out.calibratable = variable.writable ? 1u : 0u;
    out.bit_field = variable.bitField ? 1u : 0u;
    out.bit_offset = variable.bitOffset;
    out.bit_size = variable.bitSize;
    return true;
}

bool MonitorFacade::readVariable(const UiVariable &variable, QString &error) {
    if (!client_ || !connected()) {
        error = QStringLiteral("设备未连接");
        return false;
    }

    QByteArray name;
    QByteArray typeName;
    vm_comm_variable_t commVariable = {};
    if (!fillCommVariable(variable, commVariable, name, typeName, error)) {
        return false;
    }

    if (vm_comm_read_variable(client_, &commVariable) != VM_OK) {
        error = QStringLiteral("读取请求发送失败");
        return false;
    }

    return true;
}

bool MonitorFacade::writeVariable(const UiVariable &variable, const QString &target, QString &error) {
    if (!client_ || !connected()) {
        error = QStringLiteral("设备未连接");
        return false;
    }

    QByteArray name;
    QByteArray typeName;
    QByteArray targetText = target.toUtf8();
    char errorBuffer[VM_COMM_TEXT_MAX] = {};
    vm_comm_variable_t commVariable = {};
    if (!fillCommVariable(variable, commVariable, name, typeName, error)) {
        return false;
    }

    if (vm_comm_write_variable(client_,
                               &commVariable,
                               targetText.constData(),
                               errorBuffer,
                               sizeof(errorBuffer)) != VM_OK) {
        error = QString::fromUtf8(errorBuffer[0] != '\0' ? errorBuffer : "写入请求发送失败");
        return false;
    }

    return true;
}

void MonitorFacade::poll() {
    if (!client_ || !connected()) {
        return;
    }

    (void)vm_comm_poll(client_, 8u);
}

void MonitorFacade::onEvent(void *context, const vm_comm_event_t *event) {
    auto *self = static_cast<MonitorFacade *>(context);

    if (self && self->responseCallback_ && event) {
        MonitorCommEvent next;
        next.type = event->type;
        next.protocolCommand = event->protocol_command;
        next.errorCode = event->error_code;
        next.address = event->address;
        next.size = event->size;
        next.name = QString::fromUtf8(event->name);
        next.displayValue = QString::fromUtf8(event->display_value);
        next.message = QString::fromUtf8(event->message);
        next.numericValue = event->numeric_value;
        next.hasNumericValue = event->has_numeric_value != 0u;
        self->responseCallback_(next);
    }
}

bool MonitorFacade::load(const QString &path, QString &error) {
    const QByteArray localPath = path.toLocal8Bit();
    char errorBuffer[VM_MONITOR_ERROR_MAX] = {};
    vm_monitor_variable_list_t *list = nullptr;

    if (vm_monitor_variables_load(localPath.constData(), &list, errorBuffer, sizeof(errorBuffer)) != VM_OK) {
        error = QString::fromUtf8(errorBuffer[0] != '\0' ? errorBuffer : "变量加载失败");
        return false;
    }

    QVector<MonitorVariable> next;
    const size_t count = vm_monitor_variable_count(list);
    next.reserve(static_cast<int>(count));

    for (size_t index = 0; index < count; ++index) {
        vm_monitor_variable_t source = {};
        if (vm_monitor_variable_at(list, index, &source) != VM_OK) {
            continue;
        }

        MonitorVariable item;
        item.name = QString::fromUtf8(source.name);
        item.typeName = QString::fromUtf8(source.type_name);
        item.address = source.address;
        item.size = source.size;
        item.writable = source.writable != 0;
        item.monitorable = source.monitorable != 0;
        item.calibratable = source.calibratable != 0;
        item.bitField = source.bit_field != 0;
        item.bitOffset = source.bit_offset;
        item.bitSize = source.bit_size;
        next.append(item);
    }

    vm_monitor_variable_list_destroy(list);
    variables = next;
    return true;
}
