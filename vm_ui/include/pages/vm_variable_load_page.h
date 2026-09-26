#pragma once

#include <QJsonArray>
#include <QVector>
#include <QWidget>

#include "vm_page_types.h"

class QLineEdit;
class QPushButton;
class QTableWidget;

struct MonitorVariable;

class VariableLoadPage : public QWidget {
public:
    explicit VariableLoadPage(QWidget *parent = nullptr);

    QPushButton *loadButton() const;
    QTableWidget *table() const;

    QString imagePath() const;
    void setImagePath(const QString &path);
    void clearVariables();
    void setVariables(const QVector<MonitorVariable> &variables);

    QVector<UiVariable> selectedMonitorVariables() const;
    QVector<UiVariable> selectedCalibrationVariables() const;
    QJsonArray selectionJson() const;
    void applySelection(const QJsonArray &selection);

private:
    QVector<UiVariable> selectedVariables(int column) const;
    QString nameAt(int row) const;
    QString addressAt(int row) const;
    QString typeNameAt(int row) const;
    bool bitFieldAt(int row) const;
    quint8 bitOffsetAt(int row) const;
    quint8 bitSizeAt(int row) const;

    QLineEdit *pathEdit_ = nullptr;
    QLineEdit *searchEdit_ = nullptr;
    QPushButton *loadButton_ = nullptr;
    QTableWidget *table_ = nullptr;
};
