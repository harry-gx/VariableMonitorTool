/*
 * 文件说明：UI 界面模块Qt/C++ 实现文件。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "pages/vm_device_page.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStackedWidget>
#include <QVBoxLayout>

// 函数说明：wrapForm，执行本模块对应功能逻辑。
// 输入：title：函数输入参数，参与本函数的计算、查找或状态更新。；form：函数输入参数，参与本函数的计算、查找或状态更新。；parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
static QWidget *wrapForm(const QString &title, QFormLayout *form, QWidget *parent) {
    auto *group = new QGroupBox(title, parent);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    group->setLayout(form);
    return group;
}

// 函数说明：addBaudRateItem，执行本模块对应功能逻辑。
// 输入：box：函数输入参数，参与本函数的计算、查找或状态更新。；text：文本内容或输入字符串。；value：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
static void addBaudRateItem(QComboBox *box, const QString &text, unsigned value) {
    box->addItem(text, value);
}

// 函数说明：addStandardCanBaudRates，执行本模块对应功能逻辑。
// 输入：box：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
static void addStandardCanBaudRates(QComboBox *box) {
    addBaudRateItem(box, QStringLiteral("125 kbps"), 125000);
    addBaudRateItem(box, QStringLiteral("250 kbps"), 250000);
    addBaudRateItem(box, QStringLiteral("500 kbps"), 500000);
    addBaudRateItem(box, QStringLiteral("1 Mbps"), 1000000);
    box->setCurrentIndex(box->findData(500000));
}

// 函数说明：addCanChannels，执行本模块对应功能逻辑。
// 输入：box：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
static void addCanChannels(QComboBox *box) {
    box->addItems(QStringList() << QStringLiteral("0")
                                << QStringLiteral("1")
                                << QStringLiteral("2")
                                << QStringLiteral("3"));
}

// 函数说明：addCanAdapters，执行本模块对应功能逻辑。
// 输入：box：函数输入参数，参与本函数的计算、查找或状态更新。；canFd：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
static void addCanAdapters(QComboBox *box, bool canFd) {
    if (!canFd) {
        box->addItem(QStringLiteral("广成科技 CAN 卡"), QStringLiteral("gc"));
    }
    box->addItem(canFd ? QStringLiteral("周立功 CANFD 卡")
                       : QStringLiteral("周立功 CAN 卡"),
                 QStringLiteral("zlg"));
}

// 函数说明：setComboByNumericData，执行本模块对应功能逻辑。
// 输入：box：函数输入参数，参与本函数的计算、查找或状态更新。；valueText：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
static void setComboByNumericData(QComboBox *box, const QString &valueText) {
    bool ok = false;
    const unsigned value = valueText.toUInt(&ok);
    if (ok) {
        const int index = box->findData(value);
        if (index >= 0) {
            box->setCurrentIndex(index);
            return;
        }
    }
    box->setCurrentText(valueText);
}

// 函数说明：QWidget，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
DevicePage::DevicePage(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto *top = new QHBoxLayout;
    deviceType_ = new QComboBox(this);
    deviceType_->addItem(QStringLiteral("串口"), static_cast<int>(UiDeviceType::Serial));
    deviceType_->addItem(QStringLiteral("CAN"), static_cast<int>(UiDeviceType::Can));
    deviceType_->addItem(QStringLiteral("CANFD"), static_cast<int>(UiDeviceType::CanFd));
    deviceType_->addItem(QStringLiteral("网口"), static_cast<int>(UiDeviceType::Ethernet));

    refreshButton_ = new QPushButton(QStringLiteral("刷新设备"), this);
    connectButton_ = new QPushButton(QStringLiteral("连接设备"), this);

    top->addWidget(new QLabel(QStringLiteral("设备类型"), this));
    top->addWidget(deviceType_);
    top->addWidget(refreshButton_);
    top->addStretch();
    top->addWidget(connectButton_);
    root->addLayout(top);

    deviceStack_ = new QStackedWidget(this);
    root->addWidget(deviceStack_, 1);

    auto *serialForm = new QFormLayout;
    ports_ = new QComboBox(this);
    baud_ = new QComboBox(this);
    baud_->addItems(QStringList() << QStringLiteral("9600")
                                  << QStringLiteral("19200")
                                  << QStringLiteral("38400")
                                  << QStringLiteral("57600")
                                  << QStringLiteral("115200")
                                  << QStringLiteral("230400")
                                  << QStringLiteral("460800")
                                  << QStringLiteral("921600"));
    baud_->setCurrentText(QStringLiteral("115200"));
    serialForm->addRow(QStringLiteral("串口"), ports_);
    serialForm->addRow(QStringLiteral("波特率"), baud_);
    deviceStack_->addWidget(wrapForm(QStringLiteral("串口设备"), serialForm, this));

    auto *canForm = new QFormLayout;
    canAdapter_ = new QComboBox(this);
    canChannel_ = new QComboBox(this);
    canBaud_ = new QComboBox(this);
    addCanAdapters(canAdapter_, false);
    addCanChannels(canChannel_);
    addStandardCanBaudRates(canBaud_);
    canForm->addRow(QStringLiteral("CAN 卡"), canAdapter_);
    canForm->addRow(QStringLiteral("通道"), canChannel_);
    canForm->addRow(QStringLiteral("波特率"), canBaud_);
    canForm->addRow(QStringLiteral("说明"),
                    new QLabel(QStringLiteral("底层 C 适配接口已留好，后续接入广成/周立功 CAN 驱动。"), this));
    deviceStack_->addWidget(wrapForm(QStringLiteral("CAN 设备"), canForm, this));

    auto *canFdForm = new QFormLayout;
    canFdAdapter_ = new QComboBox(this);
    canFdChannel_ = new QComboBox(this);
    canFdBaud_ = new QComboBox(this);
    canFdDataBaud_ = new QComboBox(this);
    addCanAdapters(canFdAdapter_, true);
    addCanChannels(canFdChannel_);
    addStandardCanBaudRates(canFdBaud_);
    addBaudRateItem(canFdDataBaud_, QStringLiteral("1 Mbps"), 1000000);
    addBaudRateItem(canFdDataBaud_, QStringLiteral("2 Mbps"), 2000000);
    addBaudRateItem(canFdDataBaud_, QStringLiteral("4 Mbps"), 4000000);
    addBaudRateItem(canFdDataBaud_, QStringLiteral("5 Mbps"), 5000000);
    addBaudRateItem(canFdDataBaud_, QStringLiteral("8 Mbps"), 8000000);
    canFdDataBaud_->setCurrentIndex(canFdDataBaud_->findData(2000000));
    canFdForm->addRow(QStringLiteral("CANFD 卡"), canFdAdapter_);
    canFdForm->addRow(QStringLiteral("通道"), canFdChannel_);
    canFdForm->addRow(QStringLiteral("仲裁域波特率"), canFdBaud_);
    canFdForm->addRow(QStringLiteral("数据域波特率"), canFdDataBaud_);
    canFdForm->addRow(QStringLiteral("说明"),
                      new QLabel(QStringLiteral("底层 C 适配接口已留好，后续接入周立功 CANFD 驱动。"), this));
    deviceStack_->addWidget(wrapForm(QStringLiteral("CANFD 设备"), canFdForm, this));

    auto *networkForm = new QFormLayout;
    networkHost_ = new QLineEdit(this);
    networkPort_ = new QLineEdit(this);
    networkHost_->setText(QStringLiteral("192.168.1.10"));
    networkPort_->setText(QStringLiteral("5555"));
    networkForm->addRow(QStringLiteral("IP 地址"), networkHost_);
    networkForm->addRow(QStringLiteral("端口"), networkPort_);
    networkForm->addRow(QStringLiteral("说明"),
                        new QLabel(QStringLiteral("网口传输层接口已留好，后续接入 TCP/UDP 底层。"), this));
    deviceStack_->addWidget(wrapForm(QStringLiteral("网口设备"), networkForm, this));

    connect(deviceType_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                deviceStack_->setCurrentIndex(index);
                refreshPorts();
            });

    refreshPorts();
}

QPushButton *DevicePage::refreshButton() const { return refreshButton_; }
QPushButton *DevicePage::connectButton() const { return connectButton_; }

// 函数说明：DevicePage::refreshPorts，刷新显示或使能状态。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::refreshPorts() {
    if (deviceType() != UiDeviceType::Serial) {
        return;
    }

    const QString old = ports_->currentData().toString();
    ports_->clear();
    for (const auto &port : QSerialPortInfo::availablePorts()) {
        ports_->addItem(port.portName() + QStringLiteral(" (") + port.description() + QStringLiteral(")"),
                        port.portName());
    }
    const int index = ports_->findData(old);
    if (index >= 0) {
        ports_->setCurrentIndex(index);
    }
}

// 函数说明：DevicePage::setConnected，建立连接并准备收发链路。
// 输入：connected：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setConnected(bool connected) {
    deviceType_->setEnabled(!connected);
    deviceStack_->setEnabled(!connected);
    refreshButton_->setEnabled(!connected);
    connectButton_->setText(connected ? QStringLiteral("断开设备") : QStringLiteral("连接设备"));
}

// 函数说明：DevicePage::setDeviceType，执行本模块对应功能逻辑。
// 输入：type：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setDeviceType(UiDeviceType type) {
    const int index = deviceType_->findData(static_cast<int>(type));
    if (index >= 0) {
        deviceType_->setCurrentIndex(index);
    }
}

// 函数说明：DevicePage::setPortName，执行本模块对应功能逻辑。
// 输入：portName：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setPortName(const QString &portName) {
    const int index = ports_->findData(portName);
    if (index >= 0) {
        ports_->setCurrentIndex(index);
    }
}

// 函数说明：DevicePage::setBaudRate，执行本模块对应功能逻辑。
// 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setBaudRate(const QString &baudRate) {
    baud_->setCurrentText(baudRate);
}

// 函数说明：DevicePage::setCanAdapter，执行本模块对应功能逻辑。
// 输入：adapter：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setCanAdapter(const QString &adapter) {
    const int canIndex = canAdapter_->findData(adapter);
    if (canIndex >= 0) {
        canAdapter_->setCurrentIndex(canIndex);
    }
    const int canFdIndex = canFdAdapter_->findData(adapter);
    if (canFdIndex >= 0) {
        canFdAdapter_->setCurrentIndex(canFdIndex);
    }
}

// 函数说明：DevicePage::setCanChannel，执行本模块对应功能逻辑。
// 输入：channel：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setCanChannel(const QString &channel) {
    canChannel_->setCurrentText(channel);
    canFdChannel_->setCurrentText(channel);
}

// 函数说明：DevicePage::setCanBaudRate，执行本模块对应功能逻辑。
// 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setCanBaudRate(const QString &baudRate) {
    setComboByNumericData(canBaud_, baudRate);
    setComboByNumericData(canFdBaud_, baudRate);
}

// 函数说明：DevicePage::setCanFdDataBaudRate，执行本模块对应功能逻辑。
// 输入：baudRate：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setCanFdDataBaudRate(const QString &baudRate) {
    setComboByNumericData(canFdDataBaud_, baudRate);
}

// 函数说明：DevicePage::setNetworkHost，执行本模块对应功能逻辑。
// 输入：host：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setNetworkHost(const QString &host) {
    networkHost_->setText(host);
}

// 函数说明：DevicePage::setNetworkPort，执行本模块对应功能逻辑。
// 输入：port：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void DevicePage::setNetworkPort(const QString &port) {
    networkPort_->setText(port);
}

// 函数说明：DevicePage::deviceType，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
UiDeviceType DevicePage::deviceType() const {
    return static_cast<UiDeviceType>(deviceType_->currentData().toInt());
}

// 函数说明：DevicePage::portName，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString DevicePage::portName() const {
    return ports_->currentData().toString();
}

// 函数说明：DevicePage::baudRate，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
unsigned DevicePage::baudRate() const {
    return baud_->currentText().toUInt();
}

// 函数说明：DevicePage::canAdapter，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString DevicePage::canAdapter() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdAdapter_->currentData().toString()
        : canAdapter_->currentData().toString();
}

// 函数说明：DevicePage::canChannel，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString DevicePage::canChannel() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdChannel_->currentText()
        : canChannel_->currentText();
}

// 函数说明：DevicePage::canBaudRate，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
unsigned DevicePage::canBaudRate() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdBaud_->currentData().toUInt()
        : canBaud_->currentData().toUInt();
}

// 函数说明：DevicePage::canFdDataBaudRate，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
unsigned DevicePage::canFdDataBaudRate() const {
    return canFdDataBaud_->currentData().toUInt();
}

// 函数说明：DevicePage::networkHost，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString DevicePage::networkHost() const {
    return networkHost_->text();
}

// 函数说明：DevicePage::networkPort，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
unsigned DevicePage::networkPort() const {
    return networkPort_->text().toUInt();
}
