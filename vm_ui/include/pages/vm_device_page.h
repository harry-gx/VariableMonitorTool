/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <QString>
#include <QWidget>

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QComboBox;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QLineEdit;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QPushButton;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QStackedWidget;

/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
enum class UiDeviceType {
    Serial,
    Can,
    CanFd,
    Ethernet
};

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class DevicePage : public QWidget {
public:
    explicit DevicePage(QWidget *parent = nullptr);

    /**
     * 函数说明：refreshButton，刷新显示或使能状态。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QPushButton *refreshButton() const;
    /**
     * 函数说明：connectButton，建立连接并准备收发链路。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QPushButton *connectButton() const;

    /**
     * 函数说明：refreshPorts，刷新显示或使能状态。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void refreshPorts();
    /**
     * 函数说明：setConnected，建立连接并准备收发链路。
     * 输入：connected：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setConnected(bool connected);
    /**
     * 函数说明：setDeviceType，执行本模块对应功能逻辑。
     * 输入：type：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setDeviceType(UiDeviceType type);
    /**
     * 函数说明：setPortName，执行本模块对应功能逻辑。
     * 输入：portName：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setPortName(const QString &portName);
    /**
     * 函数说明：setBaudRate，执行本模块对应功能逻辑。
     * 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setBaudRate(const QString &baudRate);
    /**
     * 函数说明：setCanAdapter，执行本模块对应功能逻辑。
     * 输入：adapter：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCanAdapter(const QString &adapter);
    /**
     * 函数说明：setCanChannel，执行本模块对应功能逻辑。
     * 输入：channel：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCanChannel(const QString &channel);
    /**
     * 函数说明：setCanBaudRate，执行本模块对应功能逻辑。
     * 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCanBaudRate(const QString &baudRate);
    /**
     * 函数说明：setCanFdDataBaudRate，执行本模块对应功能逻辑。
     * 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCanFdDataBaudRate(const QString &baudRate);
    /**
     * 函数说明：setNetworkHost，执行本模块对应功能逻辑。
     * 输入：host：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setNetworkHost(const QString &host);
    /**
     * 函数说明：setNetworkPort，执行本模块对应功能逻辑。
     * 输入：port：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setNetworkPort(const QString &port);

    /**
     * 函数说明：deviceType，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    UiDeviceType deviceType() const;
    /**
     * 函数说明：portName，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString portName() const;
    /**
     * 函数说明：baudRate，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    unsigned baudRate() const;
    /**
     * 函数说明：canAdapter，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString canAdapter() const;
    /**
     * 函数说明：canChannel，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString canChannel() const;
    /**
     * 函数说明：canBaudRate，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    unsigned canBaudRate() const;
    /**
     * 函数说明：canFdDataBaudRate，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    unsigned canFdDataBaudRate() const;
    /**
     * 函数说明：networkHost，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString networkHost() const;
    /**
     * 函数说明：networkPort，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    unsigned networkPort() const;

private:
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *deviceType_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QStackedWidget *deviceStack_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *ports_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *baud_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canAdapter_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canChannel_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canBaud_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canFdAdapter_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canFdChannel_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canFdBaud_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QComboBox *canFdDataBaud_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLineEdit *networkHost_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLineEdit *networkPort_ = nullptr;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QPushButton *refreshButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QPushButton *connectButton_ = nullptr;
};
