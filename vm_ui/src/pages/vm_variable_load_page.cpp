/*
 * 文件说明：UI 界面模块Qt/C++ 实现文件。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "pages/vm_variable_load_page.h"

#include <QtWidgets>

#include "vm_ui_port.h"

// 类型说明：枚举限定模块状态、事件或设备类型的取值范围。
enum VariableLoadRoles {
    VariableLoadRoleTypeName = Qt::UserRole + 1,
    VariableLoadRoleMonitorable,
    VariableLoadRoleCalibratable,
    VariableLoadRoleBitField,
    VariableLoadRoleBitOffset,
    VariableLoadRoleBitSize
};

// 函数说明：QWidget，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
VariableLoadPage::VariableLoadPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    auto *pathRow = new QHBoxLayout;
    pathEdit_ = new QLineEdit(this);
    auto *browseButton = new QPushButton(QStringLiteral("浏览"), this);
    loadButton_ = new QPushButton(QStringLiteral("加载变量"), this);
    pathRow->addWidget(pathEdit_, 1);
    pathRow->addWidget(browseButton);
    pathRow->addWidget(loadButton_);
    layout->addLayout(pathRow);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("搜索变量名称"));
    layout->addWidget(searchEdit_);

    table_ = new QTableWidget(0, 6, this);
    table_->setHorizontalHeaderLabels(QStringList() << QStringLiteral("变量名")
                                                    << QStringLiteral("地址")
                                                    << QStringLiteral("字节数")
                                                    << QStringLiteral("属性")
                                                    << QStringLiteral("加入监控")
                                                    << QStringLiteral("加入标定"));
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table_, 1);

    connect(browseButton, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("选择 ELF/AXF"),
            QString(),
            QStringLiteral("ELF/AXF (*.elf *.axf);;All (*)"));
        if (!path.isEmpty()) {
            pathEdit_->setText(path);
        }
    });
    connect(searchEdit_, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int row = 0; row < table_->rowCount(); ++row) {
            table_->setRowHidden(row, !nameAt(row).contains(text, Qt::CaseInsensitive));
        }
    });
}

QPushButton *VariableLoadPage::loadButton() const { return loadButton_; }
QTableWidget *VariableLoadPage::table() const { return table_; }

QString VariableLoadPage::imagePath() const { return pathEdit_->text(); }
void VariableLoadPage::setImagePath(const QString &path) { pathEdit_->setText(path); }

// 函数说明：VariableLoadPage::clearVariables，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void VariableLoadPage::clearVariables() {
    QSignalBlocker blocker(table_);
    table_->setRowCount(0);
}

// 函数说明：VariableLoadPage::setVariables，执行本模块对应功能逻辑。
// 输入：variables：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void VariableLoadPage::setVariables(const QVector<MonitorVariable> &variables) {
    QSignalBlocker blocker(table_);
    table_->setRowCount(variables.size());
    for (int row = 0; row < variables.size(); ++row) {
        const auto &variable = variables.at(row);
        const QString typeText = variable.typeName.isEmpty()
            ? QStringLiteral("未知类型")
            : variable.typeName;
        const QString bitText = variable.bitField
            ? QStringLiteral(" / bit[%1:%2]")
                  .arg(variable.bitOffset)
                  .arg(variable.bitOffset + variable.bitSize - 1)
            : QString();
        const QStringList columns = QStringList()
            << variable.name
            << QStringLiteral("0x%1").arg(variable.address, 8, 16, QChar('0'))
            << QString::number(variable.size)
            << QStringLiteral("%1%2 / %3")
                   .arg(typeText, bitText, variable.writable ? QStringLiteral("节区可写") : QStringLiteral("只读"));
        for (int column = 0; column < columns.size(); ++column) {
            auto *item = new QTableWidgetItem(columns.at(column));
            item->setData(VariableLoadRoleTypeName, variable.typeName);
            item->setData(VariableLoadRoleMonitorable, variable.monitorable);
            item->setData(VariableLoadRoleCalibratable, variable.calibratable);
            item->setData(VariableLoadRoleBitField, variable.bitField);
            item->setData(VariableLoadRoleBitOffset, variable.bitOffset);
            item->setData(VariableLoadRoleBitSize, variable.bitSize);
            table_->setItem(row, column, item);
        }

        for (int column = 4; column < 6; ++column) {
            auto *item = new QTableWidgetItem;
            const bool enabled = column == 4 ? variable.monitorable : variable.calibratable;
            item->setData(VariableLoadRoleTypeName, variable.typeName);
            item->setData(VariableLoadRoleMonitorable, variable.monitorable);
            item->setData(VariableLoadRoleCalibratable, variable.calibratable);
            item->setData(VariableLoadRoleBitField, variable.bitField);
            item->setData(VariableLoadRoleBitOffset, variable.bitOffset);
            item->setData(VariableLoadRoleBitSize, variable.bitSize);
            item->setCheckState(Qt::Unchecked);
            if (!enabled) {
                item->setFlags(Qt::NoItemFlags);
                item->setToolTip(column == 4
                    ? QStringLiteral("该变量类型暂不支持直接监控，请选择可寻址的基础类型或结构体成员。")
                    : QStringLiteral("该变量类型或只读限定暂不支持标定，请选择可写基础类型或结构体成员。"));
            }
            table_->setItem(row, column, item);
        }
    }
}

// 函数说明：VariableLoadPage::selectedVariables，执行本模块对应功能逻辑。
// 输入：column：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QVector<UiVariable> VariableLoadPage::selectedVariables(int column) const {
    QVector<UiVariable> result;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto *check = table_->item(row, column);
        if (check && (check->flags() & Qt::ItemIsEnabled) && check->checkState() == Qt::Checked) {
            result.append(UiVariable(nameAt(row), addressAt(row), table_->item(row, 2)->text(),
                                     table_->item(row, 0)->data(VariableLoadRoleCalibratable).toBool(),
                                     typeNameAt(row),
                                     bitFieldAt(row),
                                     bitOffsetAt(row),
                                     bitSizeAt(row)));
        }
    }
    return result;
}

// 函数说明：VariableLoadPage::selectedMonitorVariables，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QVector<UiVariable> VariableLoadPage::selectedMonitorVariables() const {
    return selectedVariables(4);
}

// 函数说明：VariableLoadPage::selectedCalibrationVariables，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QVector<UiVariable> VariableLoadPage::selectedCalibrationVariables() const {
    return selectedVariables(5);
}

// 函数说明：VariableLoadPage::selectionJson，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QJsonArray VariableLoadPage::selectionJson() const {
    QJsonArray selected;
    for (int row = 0; row < table_->rowCount(); ++row) {
        QJsonObject object;
        object[QStringLiteral("name")] = nameAt(row);
        object[QStringLiteral("address")] = addressAt(row);
        object[QStringLiteral("monitor")] =
            (table_->item(row, 4)->flags() & Qt::ItemIsEnabled) &&
            table_->item(row, 4)->checkState() == Qt::Checked;
        object[QStringLiteral("calibration")] =
            (table_->item(row, 5)->flags() & Qt::ItemIsEnabled) &&
            table_->item(row, 5)->checkState() == Qt::Checked;
        selected.append(object);
    }
    return selected;
}

// 函数说明：VariableLoadPage::applySelection，执行本模块对应功能逻辑。
// 输入：selection：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void VariableLoadPage::applySelection(const QJsonArray &selection) {
    QSignalBlocker blocker(table_);
    for (const auto &value : selection) {
        const QJsonObject object = value.toObject();
        for (int row = 0; row < table_->rowCount(); ++row) {
            if (nameAt(row) == object[QStringLiteral("name")].toString() &&
                addressAt(row) == object[QStringLiteral("address")].toString()) {
                if (table_->item(row, 4)->flags() & Qt::ItemIsEnabled) {
                    table_->item(row, 4)->setCheckState(object[QStringLiteral("monitor")].toBool()
                        ? Qt::Checked : Qt::Unchecked);
                }
                if (table_->item(row, 5)->flags() & Qt::ItemIsEnabled) {
                    table_->item(row, 5)->setCheckState(object[QStringLiteral("calibration")].toBool()
                        ? Qt::Checked : Qt::Unchecked);
                }
            }
        }
    }
}

// 函数说明：VariableLoadPage::nameAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString VariableLoadPage::nameAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->text() : QString();
}

// 函数说明：VariableLoadPage::addressAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString VariableLoadPage::addressAt(int row) const {
    return table_->item(row, 1) ? table_->item(row, 1)->text() : QString();
}

// 函数说明：VariableLoadPage::typeNameAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString VariableLoadPage::typeNameAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->data(VariableLoadRoleTypeName).toString() : QString();
}

// 函数说明：VariableLoadPage::bitFieldAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool VariableLoadPage::bitFieldAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->data(VariableLoadRoleBitField).toBool() : false;
}

// 函数说明：VariableLoadPage::bitOffsetAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint8 VariableLoadPage::bitOffsetAt(int row) const {
    return table_->item(row, 0) ? static_cast<quint8>(table_->item(row, 0)->data(VariableLoadRoleBitOffset).toUInt()) : 0;
}

// 函数说明：VariableLoadPage::bitSizeAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint8 VariableLoadPage::bitSizeAt(int row) const {
    return table_->item(row, 0) ? static_cast<quint8>(table_->item(row, 0)->data(VariableLoadRoleBitSize).toUInt()) : 0;
}
