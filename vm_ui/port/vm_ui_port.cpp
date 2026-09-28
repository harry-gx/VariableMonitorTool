/*
 * 文件说明：UI 到 C 模块的适配层实现，隔离 Qt 类型和 C 模块接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_ui_port.h"

#include <QByteArray>

#include "vm_monitor_variables.h"

// 函数说明：addressFromText，执行本模块对应功能逻辑。
// 输入：text：文本内容或输入字符串。；ok：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
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

// 函数说明：MonitorFacade::MonitorFacade，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
MonitorFacade::MonitorFacade() {
    vm_comm_create(&client_);
}

// 函数说明：MonitorFacade::~MonitorFacade，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
MonitorFacade::~MonitorFacade() {
    vm_comm_destroy(client_);
    client_ = nullptr;
}

// 函数说明：MonitorFacade::connected，建立连接并准备收发链路。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool MonitorFacade::connected() const {
    return vm_comm_is_connected(client_) != 0u;
}

// 函数说明：MonitorFacade::disconnectPort，建立连接并准备收发链路。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorFacade::disconnectPort() {
    vm_comm_disconnect(client_);
}

// 函数说明：MonitorFacade::connectPort，建立连接并准备收发链路。
// 输入：port：函数输入参数，参与本函数的计算、查找或状态更新。；baud：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool MonitorFacade::connectPort(const QString &port, unsigned baud, QString &error) {
    MonitorDeviceConfig config;
    config.type = MonitorDeviceType::Serial;
    config.serialPort = port;
    config.serialBaud = baud;
    return connectDevice(config, error);
}

// 函数说明：MonitorFacade::connectDevice，建立连接并准备收发链路。
// 输入：config：设备连接参数或模块初始化参数。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
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

    // 关键步骤：连接成功后注册 C 回调，让通信模块把读值、写确认和错误统一推送回 UI。
    vm_comm_set_event_callback(client_, &MonitorFacade::onEvent, this);
    return true;
}

// 函数说明：MonitorFacade::setResponseCallback，处理回调事件并更新相关状态。
// 输入：callback：事件回调函数指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorFacade::setResponseCallback(std::function<void(const MonitorCommEvent &)> callback) {
    responseCallback_ = std::move(callback);
}

// 函数说明：MonitorFacade::fillCommVariable，执行本模块对应功能逻辑。
// 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；out：输出对象或结果指针，函数成功时写入有效值。；name：函数输入参数，参与本函数的计算、查找或状态更新。；typeName：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
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

// 函数说明：MonitorFacade::readVariable，读取数据或发起读取请求。
// 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
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

// 函数说明：MonitorFacade::writeVariable，写入数据或发起标定请求。
// 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；target：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
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

// 函数说明：MonitorFacade::poll，轮询处理异步收发事件。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorFacade::poll() {
    if (!client_ || !connected()) {
        return;
    }

    (void)vm_comm_poll(client_, 8u);
}

// 函数说明：MonitorFacade::onEvent，处理 UI 或通信事件。
// 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；event：UI 或通信模块事件对象。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
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

// 函数说明：MonitorFacade::load，加载外部文件或配置。
// 输入：path：待加载的文件路径。；error：错误信息输出缓冲区。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
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
