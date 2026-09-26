#pragma once

#include <QString>
#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QStackedWidget;

enum class UiDeviceType {
    Serial,
    Can,
    CanFd,
    Ethernet
};

class DevicePage : public QWidget {
public:
    explicit DevicePage(QWidget *parent = nullptr);

    QPushButton *refreshButton() const;
    QPushButton *connectButton() const;

    void refreshPorts();
    void setConnected(bool connected);
    void setDeviceType(UiDeviceType type);
    void setPortName(const QString &portName);
    void setBaudRate(const QString &baudRate);
    void setCanAdapter(const QString &adapter);
    void setCanChannel(const QString &channel);
    void setCanBaudRate(const QString &baudRate);
    void setCanFdDataBaudRate(const QString &baudRate);
    void setNetworkHost(const QString &host);
    void setNetworkPort(const QString &port);

    UiDeviceType deviceType() const;
    QString portName() const;
    unsigned baudRate() const;
    QString canAdapter() const;
    QString canChannel() const;
    unsigned canBaudRate() const;
    unsigned canFdDataBaudRate() const;
    QString networkHost() const;
    unsigned networkPort() const;

private:
    QComboBox *deviceType_ = nullptr;
    QStackedWidget *deviceStack_ = nullptr;

    QComboBox *ports_ = nullptr;
    QComboBox *baud_ = nullptr;

    QComboBox *canAdapter_ = nullptr;
    QComboBox *canChannel_ = nullptr;
    QComboBox *canBaud_ = nullptr;

    QComboBox *canFdAdapter_ = nullptr;
    QComboBox *canFdChannel_ = nullptr;
    QComboBox *canFdBaud_ = nullptr;
    QComboBox *canFdDataBaud_ = nullptr;

    QLineEdit *networkHost_ = nullptr;
    QLineEdit *networkPort_ = nullptr;

    QPushButton *refreshButton_ = nullptr;
    QPushButton *connectButton_ = nullptr;
};
