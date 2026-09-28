/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <functional>

#include <QByteArray>
#include <QString>
#include <QVector>

#include "pages/vm_page_types.h"
#include "vm_communication.h"

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct MonitorVariable {
    /* 变量说明：name，变量名、页面名或节点名。 */
    QString name;
    /* 变量说明：typeName，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString typeName;
    quint64 address = 0;
    quint64 size = 0;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool writable = false;
    /* 变量说明：true，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool monitorable = true;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool calibratable = false;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool bitField = false;
    quint8 bitOffset = 0;
    quint8 bitSize = 0;
};

/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
enum class MonitorDeviceType {
    Serial,
    Can,
    CanFd,
    Ethernet
};

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct MonitorDeviceConfig {
    /* 变量说明：Serial，保存当前对象运行所需的状态、参数或缓存数据。 */
    MonitorDeviceType type = MonitorDeviceType::Serial;

    /* 变量说明：serialPort，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString serialPort;
    unsigned serialBaud = 115200;

    /* 变量说明：canAdapter，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString canAdapter;
    unsigned canChannel = 0;
    unsigned canBaud = 500000;
    unsigned canFdDataBaud = 2000000;

    /* 变量说明：networkHost，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString networkHost;
    unsigned networkPort = 0;
};

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct MonitorCommEvent {
    /* 变量说明：VM_COMM_EVENT_ERROR，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_comm_event_type_t type = VM_COMM_EVENT_ERROR;
    quint8 protocolCommand = 0;
    quint8 errorCode = 0;
    quint32 address = 0;
    quint16 size = 0;
    /* 变量说明：name，变量名、页面名或节点名。 */
    QString name;
    /* 变量说明：displayValue，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString displayValue;
    /* 变量说明：message，保存当前对象运行所需的状态、参数或缓存数据。 */
    QString message;
    double numericValue = 0.0;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool hasNumericValue = false;
};

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class MonitorFacade {
public:
    /**
     * 函数说明：MonitorFacade，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    MonitorFacade();
    /**
     * 函数说明：~MonitorFacade，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    ~MonitorFacade();

    /**
     * 函数说明：connectDevice，建立连接并准备收发链路。
     * 输入：config：设备连接参数或模块初始化参数。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool connectDevice(const MonitorDeviceConfig &config, QString &error);
    /**
     * 函数说明：connectPort，建立连接并准备收发链路。
     * 输入：port：函数输入参数，参与本函数的计算、查找或状态更新。；baud：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool connectPort(const QString &port, unsigned baud, QString &error);
    /**
     * 函数说明：disconnectPort，建立连接并准备收发链路。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void disconnectPort();
    /**
     * 函数说明：connected，建立连接并准备收发链路。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool connected() const;

    /**
     * 函数说明：load，加载外部文件或配置。
     * 输入：path：待加载的文件路径。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool load(const QString &path, QString &error);
    /**
     * 函数说明：readVariable，读取数据或发起读取请求。
     * 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool readVariable(const UiVariable &variable, QString &error);
    /**
     * 函数说明：writeVariable，写入数据或发起标定请求。
     * 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；target：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool writeVariable(const UiVariable &variable, const QString &target, QString &error);
    /**
     * 函数说明：poll，轮询处理异步收发事件。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void poll();

    /**
     * 函数说明：setResponseCallback，处理回调事件并更新相关状态。
     * 输入：callback：事件回调函数指针。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setResponseCallback(std::function<void(const MonitorCommEvent &)> callback);

    /* 变量说明：variables，保存当前对象运行所需的状态、参数或缓存数据。 */
    QVector<MonitorVariable> variables;

private:
    /**
     * 函数说明：onEvent，处理 UI 或通信事件。
     * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；event：UI 或通信模块事件对象。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    static void onEvent(void *context, const vm_comm_event_t *event);
    /**
     * 函数说明：fillCommVariable，执行本模块对应功能逻辑。
     * 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；out：输出对象或结果指针，函数成功时写入有效值。；name：函数输入参数，参与本函数的计算、查找或状态更新。；typeName：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    static bool fillCommVariable(const UiVariable &variable,
                                 vm_comm_variable_t &out,
                                 QByteArray &name,
                                 QByteArray &typeName,
                                 QString &error);

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_comm_t *client_ = nullptr;
    std::function<void(const MonitorCommEvent &)> responseCallback_;
};
