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

static QWidget *wrapForm(const QString &title, QFormLayout *form, QWidget *parent) {
    auto *group = new QGroupBox(title, parent);
    group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    group->setLayout(form);
    return group;
}

static void addBaudRateItem(QComboBox *box, const QString &text, unsigned value) {
    box->addItem(text, value);
}

static void addStandardCanBaudRates(QComboBox *box) {
    addBaudRateItem(box, QStringLiteral("125 kbps"), 125000);
    addBaudRateItem(box, QStringLiteral("250 kbps"), 250000);
    addBaudRateItem(box, QStringLiteral("500 kbps"), 500000);
    addBaudRateItem(box, QStringLiteral("1 Mbps"), 1000000);
    box->setCurrentIndex(box->findData(500000));
}

static void addCanChannels(QComboBox *box) {
    box->addItems(QStringList() << QStringLiteral("0")
                                << QStringLiteral("1")
                                << QStringLiteral("2")
                                << QStringLiteral("3"));
}

static void addCanAdapters(QComboBox *box, bool canFd) {
    if (!canFd) {
        box->addItem(QStringLiteral("广成科技 CAN 卡"), QStringLiteral("gc"));
    }
    box->addItem(canFd ? QStringLiteral("周立功 CANFD 卡")
                       : QStringLiteral("周立功 CAN 卡"),
                 QStringLiteral("zlg"));
}

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

void DevicePage::setConnected(bool connected) {
    deviceType_->setEnabled(!connected);
    deviceStack_->setEnabled(!connected);
    refreshButton_->setEnabled(!connected);
    connectButton_->setText(connected ? QStringLiteral("断开设备") : QStringLiteral("连接设备"));
}

void DevicePage::setDeviceType(UiDeviceType type) {
    const int index = deviceType_->findData(static_cast<int>(type));
    if (index >= 0) {
        deviceType_->setCurrentIndex(index);
    }
}

void DevicePage::setPortName(const QString &portName) {
    const int index = ports_->findData(portName);
    if (index >= 0) {
        ports_->setCurrentIndex(index);
    }
}

void DevicePage::setBaudRate(const QString &baudRate) {
    baud_->setCurrentText(baudRate);
}

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

void DevicePage::setCanChannel(const QString &channel) {
    canChannel_->setCurrentText(channel);
    canFdChannel_->setCurrentText(channel);
}

void DevicePage::setCanBaudRate(const QString &baudRate) {
    setComboByNumericData(canBaud_, baudRate);
    setComboByNumericData(canFdBaud_, baudRate);
}

void DevicePage::setCanFdDataBaudRate(const QString &baudRate) {
    setComboByNumericData(canFdDataBaud_, baudRate);
}

void DevicePage::setNetworkHost(const QString &host) {
    networkHost_->setText(host);
}

void DevicePage::setNetworkPort(const QString &port) {
    networkPort_->setText(port);
}

UiDeviceType DevicePage::deviceType() const {
    return static_cast<UiDeviceType>(deviceType_->currentData().toInt());
}

QString DevicePage::portName() const {
    return ports_->currentData().toString();
}

unsigned DevicePage::baudRate() const {
    return baud_->currentText().toUInt();
}

QString DevicePage::canAdapter() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdAdapter_->currentData().toString()
        : canAdapter_->currentData().toString();
}

QString DevicePage::canChannel() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdChannel_->currentText()
        : canChannel_->currentText();
}

unsigned DevicePage::canBaudRate() const {
    return deviceType() == UiDeviceType::CanFd
        ? canFdBaud_->currentData().toUInt()
        : canBaud_->currentData().toUInt();
}

unsigned DevicePage::canFdDataBaudRate() const {
    return canFdDataBaud_->currentData().toUInt();
}

QString DevicePage::networkHost() const {
    return networkHost_->text();
}

unsigned DevicePage::networkPort() const {
    return networkPort_->text().toUInt();
}
