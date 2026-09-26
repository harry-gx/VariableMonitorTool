#pragma once

#include <functional>

#include <QHash>
#include <QMap>
#include <QVector>
#include <QWidget>

#include "vm_page_types.h"

class QPushButton;
class QTableWidget;

class CalibrationPage : public QWidget {
public:
    explicit CalibrationPage(QWidget *parent = nullptr);

    void setReady(bool ready);
    void setRows(const QVector<UiVariable> &variables, const QMap<QString, QString> &targets);
    void setCalibrateRowCallback(std::function<void(int)> callback);
    void setCalibrateAllCallback(std::function<void()> callback);
    QMap<QString, QString> collectTargets() const;
    void updateValue(quint32 address, quint16 size, const QString &value, const QString &status);

    int rowCount() const;
    int currentRow() const;
    quint32 addressAt(int row) const;
    quint16 sizeAt(int row) const;
    QString nameAt(int row) const;
    QString typeNameAt(int row) const;
    bool bitFieldAt(int row) const;
    quint8 bitOffsetAt(int row) const;
    quint8 bitSizeAt(int row) const;
    QString targetTextAt(int row) const;
    UiVariable variableAt(int row) const;

private:
    QString keyAt(int row) const;

    QTableWidget *table_ = nullptr;
    QPushButton *calibrateAllButton_ = nullptr;
    QHash<quint32, QVector<int>> rowsByAddress_;
    std::function<void(int)> calibrateRowCallback_;
    std::function<void()> calibrateAllCallback_;
    bool ready_ = false;
};
