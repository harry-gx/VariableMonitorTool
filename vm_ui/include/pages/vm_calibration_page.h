/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <functional>

#include <QHash>
#include <QMap>
#include <QVector>
#include <QWidget>

#include "vm_page_types.h"

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QPushButton;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QTableWidget;

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class CalibrationPage : public QWidget {
public:
    explicit CalibrationPage(QWidget *parent = nullptr);

    /**
     * 函数说明：setReady，读取数据或发起读取请求。
     * 输入：ready：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setReady(bool ready);
    /**
     * 函数说明：setRows，执行本模块对应功能逻辑。
     * 输入：variables：函数输入参数，参与本函数的计算、查找或状态更新。；targets：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setRows(const QVector<UiVariable> &variables, const QMap<QString, QString> &targets);
    /**
     * 函数说明：setCalibrateRowCallback，处理回调事件并更新相关状态。
     * 输入：callback：事件回调函数指针。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCalibrateRowCallback(std::function<void(int)> callback);
    /**
     * 函数说明：setCalibrateAllCallback，处理回调事件并更新相关状态。
     * 输入：callback：事件回调函数指针。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setCalibrateAllCallback(std::function<void()> callback);
    /**
     * 函数说明：collectTargets，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QMap<QString, QString> collectTargets() const;
    /**
     * 函数说明：updateValue，刷新界面数据或内部状态。
     * 输入：address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；value：函数输入参数，参与本函数的计算、查找或状态更新。；status：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void updateValue(quint32 address, quint16 size, const QString &value, const QString &status);

    /**
     * 函数说明：rowCount，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回整数结果，具体含义由调用场景决定。
     */
    int rowCount() const;
    /**
     * 函数说明：currentRow，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回整数结果，具体含义由调用场景决定。
     */
    int currentRow() const;
    /**
     * 函数说明：addressAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    quint32 addressAt(int row) const;
    /**
     * 函数说明：sizeAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    quint16 sizeAt(int row) const;
    /**
     * 函数说明：nameAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString nameAt(int row) const;
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
    /**
     * 函数说明：targetTextAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString targetTextAt(int row) const;
    /**
     * 函数说明：variableAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    UiVariable variableAt(int row) const;

private:
    /**
     * 函数说明：keyAt，执行本模块对应功能逻辑。
     * 输入：row：界面表格行号。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回 Qt 字符串结果。
     */
    QString keyAt(int row) const;

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QTableWidget *table_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QPushButton *calibrateAllButton_ = nullptr;
    /* 变量说明：rowsByAddress_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QHash<quint32, QVector<int>> rowsByAddress_;
    std::function<void(int)> calibrateRowCallback_;
    std::function<void()> calibrateAllCallback_;
    /* 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。 */
    bool ready_ = false;
};
