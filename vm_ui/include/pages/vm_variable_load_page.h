/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <QJsonArray>
#include <QVector>
#include <QWidget>

#include "vm_page_types.h"

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QLineEdit;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QPushButton;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QTableWidget;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct MonitorVariable;

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class VariableLoadPage : public QWidget {
public:
    explicit VariableLoadPage(QWidget *parent = nullptr);

    /**
     * 函数说明：loadButton，加载外部文件或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QPushButton *loadButton() const;
    /**
     * 函数说明：table，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QTableWidget *table() const;

    /**
     * 函数说明：imagePath，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString imagePath() const;
    /**
     * 函数说明：setImagePath，执行本模块对应功能逻辑。
     * 输入：path：待加载的文件路径。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setImagePath(const QString &path);
    /**
     * 函数说明：clearVariables，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void clearVariables();
    /**
     * 函数说明：setVariables，执行本模块对应功能逻辑。
     * 输入：variables：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setVariables(const QVector<MonitorVariable> &variables);

    /**
     * 函数说明：selectedMonitorVariables，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QVector<UiVariable> selectedMonitorVariables() const;
    /**
     * 函数说明：selectedCalibrationVariables，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QVector<UiVariable> selectedCalibrationVariables() const;
    /**
     * 函数说明：selectionJson，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QJsonArray selectionJson() const;
    /**
     * 函数说明：applySelection，执行本模块对应功能逻辑。
     * 输入：selection：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void applySelection(const QJsonArray &selection);

private:
    /**
     * 函数说明：selectedVariables，执行本模块对应功能逻辑。
     * 输入：column：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QVector<UiVariable> selectedVariables(int column) const;
    /**
     * 函数说明：nameAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString nameAt(int row) const;
    /**
     * 函数说明：addressAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString addressAt(int row) const;
    /**
     * 函数说明：typeNameAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString typeNameAt(int row) const;
    /**
     * 函数说明：bitFieldAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
     */
    bool bitFieldAt(int row) const;
    /**
     * 函数说明：bitOffsetAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    quint8 bitOffsetAt(int row) const;
    /**
     * 函数说明：bitSizeAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    quint8 bitSizeAt(int row) const;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLineEdit *pathEdit_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLineEdit *searchEdit_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QPushButton *loadButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QTableWidget *table_ = nullptr;
};
