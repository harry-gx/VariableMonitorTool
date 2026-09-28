/*
 * 文件说明：UI 界面模块Qt/C++ 实现文件。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "pages/vm_calibration_page.h"

#include <QtWidgets>


// 类型说明：枚举限定模块状态、事件或设备类型的取值范围。
enum CalibrationRoles {
    CalibrationRoleAddress = Qt::UserRole + 1,
    CalibrationRoleSize,
    CalibrationRoleTypeName,
    CalibrationRoleBitField,
    CalibrationRoleBitOffset,
    CalibrationRoleBitSize,
    CalibrationRoleKey,
    CalibrationRoleTarget,
    CalibrationRoleCurrentValue,
    CalibrationRoleStatus,
    CalibrationRoleRawData
};

// 函数说明：addressFromText，执行本模块对应功能逻辑。
// 输入：text：文本内容或输入字符串。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
static quint32 addressFromText(const QString &text) {
    bool ok = false;
    const quint64 value = text.toULongLong(&ok, 16);
    return ok ? static_cast<quint32>(value) : 0;
}

// 函数说明：QWidget，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
CalibrationPage::CalibrationPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setSpacing(6);

    calibrateAllButton_ = new QPushButton(QStringLiteral("一键标定"), this);
    toolbar->addWidget(calibrateAllButton_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    table_ = new QTableWidget(0, 4, this);
    table_->setHorizontalHeaderLabels(QStringList() << QStringLiteral("变量名")
                                                    << QStringLiteral("当前值")
                                                    << QStringLiteral("目标值")
                                                    << QStringLiteral("操作"));
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setColumnWidth(0, 180);
    table_->setColumnWidth(1, 100);
    table_->setColumnWidth(2, 140);
    table_->setColumnWidth(3, 78);
    layout->addWidget(table_, 1);

    auto *hint = new QLabel(QStringLiteral("目标值使用十进制输入；float 变量可输入小数，启停变量 cal_motor_run 使用 1/0"), this);
    layout->addWidget(hint);

    connect(table_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (!item || item->column() != 2) {
            return;
        }

        auto *nameItem = table_->item(item->row(), 0);
        if (nameItem) {
            nameItem->setData(CalibrationRoleTarget, item->text());
        }
    });

    connect(calibrateAllButton_, &QPushButton::clicked, this, [this] {
        if (calibrateAllCallback_) {
            calibrateAllCallback_();
        }
    });
}

// 函数说明：CalibrationPage::setReady，读取数据或发起读取请求。
// 输入：ready：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void CalibrationPage::setReady(bool ready) {
    ready_ = ready;
    const bool hasRows = table_->rowCount() > 0;
    table_->setEnabled(ready);
    calibrateAllButton_->setEnabled(ready && hasRows);
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (auto *button = table_->cellWidget(row, 3)) {
            button->setEnabled(ready);
        }
    }
}

// 函数说明：CalibrationPage::setCalibrateRowCallback，处理回调事件并更新相关状态。
// 输入：callback：事件回调函数指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void CalibrationPage::setCalibrateRowCallback(std::function<void(int)> callback) {
    calibrateRowCallback_ = std::move(callback);
}

// 函数说明：CalibrationPage::setCalibrateAllCallback，处理回调事件并更新相关状态。
// 输入：callback：事件回调函数指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void CalibrationPage::setCalibrateAllCallback(std::function<void()> callback) {
    calibrateAllCallback_ = std::move(callback);
}

// 函数说明：CalibrationPage::setRows，执行本模块对应功能逻辑。
// 输入：variables：函数输入参数，参与本函数的计算、查找或状态更新。；targets：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void CalibrationPage::setRows(const QVector<UiVariable> &variables, const QMap<QString, QString> &targets) {
    const QString selectedKey = keyAt(currentRow());
    QSignalBlocker blocker(table_);
    table_->setRowCount(0);
    rowsByAddress_.clear();

    int selectedRow = -1;
    for (const auto &variable : variables) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QString key = variable.name + variable.address;

        auto *item = new QTableWidgetItem(variable.name);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(CalibrationRoleAddress, variable.address);
        item->setData(CalibrationRoleSize, variable.size);
        item->setData(CalibrationRoleTypeName, variable.typeName);
        item->setData(CalibrationRoleBitField, variable.bitField);
        item->setData(CalibrationRoleBitOffset, variable.bitOffset);
        item->setData(CalibrationRoleBitSize, variable.bitSize);
        item->setData(CalibrationRoleKey, key);
        item->setData(CalibrationRoleTarget, targets.value(key));
        item->setData(CalibrationRoleCurrentValue, QStringLiteral("-"));
        item->setData(CalibrationRoleStatus, QStringLiteral("待操作"));
        item->setToolTip(QStringLiteral("地址：%1\n字节数：%2\n类型：%3%4")
                             .arg(variable.address,
                                  variable.size,
                                  variable.typeName.isEmpty() ? QStringLiteral("未知") : variable.typeName,
                                  variable.bitField
                                      ? QStringLiteral("\n位字段：bit[%1:%2]")
                                            .arg(variable.bitOffset)
                                            .arg(variable.bitOffset + variable.bitSize - 1)
                                      : QString()));
        table_->setItem(row, 0, item);
        rowsByAddress_[addressFromText(variable.address)].append(row);

        auto *currentItem = new QTableWidgetItem(QStringLiteral("-"));
        currentItem->setFlags(currentItem->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 1, currentItem);

        auto *targetItem = new QTableWidgetItem(targets.value(key));
        table_->setItem(row, 2, targetItem);

        auto *calibrateButton = new QPushButton(QStringLiteral("标定"), table_);
        calibrateButton->setProperty("row", row);
        calibrateButton->setCursor(Qt::PointingHandCursor);
        connect(calibrateButton, &QPushButton::clicked, this, [this, calibrateButton] {
            const int buttonRow = calibrateButton->property("row").toInt();
            table_->setCurrentCell(buttonRow, 2);
            if (calibrateRowCallback_) {
                calibrateRowCallback_(buttonRow);
            }
        });
        table_->setCellWidget(row, 3, calibrateButton);

        if (key == selectedKey) {
            selectedRow = row;
        }
    }

    if (selectedRow >= 0) {
        table_->setCurrentCell(selectedRow, 0);
    } else if (table_->rowCount() > 0) {
        table_->setCurrentCell(0, 0);
    }

    setReady(ready_);
}

// 函数说明：CalibrationPage::collectTargets，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QMap<QString, QString> CalibrationPage::collectTargets() const {
    QMap<QString, QString> targets;
    for (int row = 0; row < table_->rowCount(); ++row) {
        auto *item = table_->item(row, 0);
        if (item) {
            targets[item->data(CalibrationRoleKey).toString()] =
                table_->item(row, 2) ? table_->item(row, 2)->text() : QString();
        }
    }
    return targets;
}

// 函数说明：CalibrationPage::updateValue，刷新界面数据或内部状态。
// 输入：address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；value：函数输入参数，参与本函数的计算、查找或状态更新。；status：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void CalibrationPage::updateValue(quint32 address,
                                  quint16 size,
                                  const QString &value,
                                  const QString &status) {
    const auto it = rowsByAddress_.constFind(address);
    if (it == rowsByAddress_.constEnd()) {
        return;
    }

    for (int row : it.value()) {
        auto *item = table_->item(row, 0);
        if (!item) {
            continue;
        }

        const quint16 rowSize = static_cast<quint16>(item->data(CalibrationRoleSize).toString().toUShort());
        if (rowSize != size) {
            continue;
        }

        const QString typeName = item->data(CalibrationRoleTypeName).toString();
        const bool bitField = item->data(CalibrationRoleBitField).toBool();
        const quint8 bitOffset = static_cast<quint8>(item->data(CalibrationRoleBitOffset).toUInt());
        const quint8 bitSize = static_cast<quint8>(item->data(CalibrationRoleBitSize).toUInt());
        item->setData(CalibrationRoleCurrentValue, value);
        item->setData(CalibrationRoleStatus, status);
        item->setToolTip(QStringLiteral("地址：%1\n字节数：%2\n类型：%3%4\n当前值：%5\n状态：%6")
                             .arg(item->data(CalibrationRoleAddress).toString(),
                                  item->data(CalibrationRoleSize).toString(),
                                  typeName.isEmpty() ? QStringLiteral("未知") : typeName,
                                  bitField
                                      ? QStringLiteral("\n位字段：bit[%1:%2]")
                                            .arg(bitOffset)
                                            .arg(bitOffset + bitSize - 1)
                                      : QString(),
                                  value,
                                  status));
        if (table_->item(row, 1)) {
            table_->item(row, 1)->setText(value);
        }
    }
}

int CalibrationPage::rowCount() const { return table_->rowCount(); }
int CalibrationPage::currentRow() const { return table_->currentRow(); }

// 函数说明：CalibrationPage::addressAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint32 CalibrationPage::addressAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? addressFromText(item->data(CalibrationRoleAddress).toString()) : 0;
}

// 函数说明：CalibrationPage::sizeAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint16 CalibrationPage::sizeAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? static_cast<quint16>(item->data(CalibrationRoleSize).toString().toUShort()) : 0;
}

// 函数说明：CalibrationPage::nameAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString CalibrationPage::nameAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? item->text() : QString();
}

// 函数说明：CalibrationPage::typeNameAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString CalibrationPage::typeNameAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? item->data(CalibrationRoleTypeName).toString() : QString();
}

// 函数说明：CalibrationPage::bitFieldAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool CalibrationPage::bitFieldAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? item->data(CalibrationRoleBitField).toBool() : false;
}

// 函数说明：CalibrationPage::bitOffsetAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint8 CalibrationPage::bitOffsetAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? static_cast<quint8>(item->data(CalibrationRoleBitOffset).toUInt()) : 0;
}

// 函数说明：CalibrationPage::bitSizeAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint8 CalibrationPage::bitSizeAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? static_cast<quint8>(item->data(CalibrationRoleBitSize).toUInt()) : 0;
}


// 函数说明：CalibrationPage::targetTextAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString CalibrationPage::targetTextAt(int row) const {
    auto *item = table_->item(row, 2);
    return item ? item->text() : QString();
}

// 函数说明：CalibrationPage::variableAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
UiVariable CalibrationPage::variableAt(int row) const {
    auto *item = table_->item(row, 0);
    if (!item) {
        return UiVariable();
    }

    return UiVariable(item->text(),
                      item->data(CalibrationRoleAddress).toString(),
                      item->data(CalibrationRoleSize).toString(),
                      true,
                      item->data(CalibrationRoleTypeName).toString(),
                      item->data(CalibrationRoleBitField).toBool(),
                      static_cast<quint8>(item->data(CalibrationRoleBitOffset).toUInt()),
                      static_cast<quint8>(item->data(CalibrationRoleBitSize).toUInt()));
}

// 函数说明：CalibrationPage::keyAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 Qt 字符串结果。
QString CalibrationPage::keyAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? item->data(CalibrationRoleKey).toString() : QString();
}
