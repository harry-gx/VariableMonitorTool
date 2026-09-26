#pragma once

#include <QHash>
#include <QVector>
#include <QWidget>

#include "vm_page_types.h"

class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class CurveCanvas;

class MonitorPage : public QWidget {
public:
    explicit MonitorPage(QWidget *parent = nullptr);

    QPushButton *readButton() const;
    QPushButton *startButton() const;
    QPushButton *stopButton() const;
    QSpinBox *periodSpinBox() const;

    void setReady(bool ready);
    void setRows(const QVector<UiVariable> &variables);
    void updateValue(quint32 address, quint16 size, const QString &value, double numericValue, bool hasNumericValue);

    int rowCount() const;
    QVector<int> checkedRows() const;
    quint32 addressAt(int row) const;
    quint16 sizeAt(int row) const;
    UiVariable variableAt(int row) const;

private:
    QTableWidget *table_ = nullptr;
    CurveCanvas *curveCanvas_ = nullptr;
    QSpinBox *periodSpinBox_ = nullptr;
    QPushButton *readButton_ = nullptr;
    QPushButton *startButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QHash<quint32, QVector<int>> rowsByAddress_;
};
