#include "pages/vm_variable_load_page.h"

#include <QtWidgets>

#include "vm_ui_port.h"

enum VariableLoadRoles {
    VariableLoadRoleTypeName = Qt::UserRole + 1,
    VariableLoadRoleMonitorable,
    VariableLoadRoleCalibratable,
    VariableLoadRoleBitField,
    VariableLoadRoleBitOffset,
    VariableLoadRoleBitSize
};

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

void VariableLoadPage::clearVariables() {
    QSignalBlocker blocker(table_);
    table_->setRowCount(0);
}

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

QVector<UiVariable> VariableLoadPage::selectedMonitorVariables() const {
    return selectedVariables(4);
}

QVector<UiVariable> VariableLoadPage::selectedCalibrationVariables() const {
    return selectedVariables(5);
}

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

QString VariableLoadPage::nameAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->text() : QString();
}

QString VariableLoadPage::addressAt(int row) const {
    return table_->item(row, 1) ? table_->item(row, 1)->text() : QString();
}

QString VariableLoadPage::typeNameAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->data(VariableLoadRoleTypeName).toString() : QString();
}

bool VariableLoadPage::bitFieldAt(int row) const {
    return table_->item(row, 0) ? table_->item(row, 0)->data(VariableLoadRoleBitField).toBool() : false;
}

quint8 VariableLoadPage::bitOffsetAt(int row) const {
    return table_->item(row, 0) ? static_cast<quint8>(table_->item(row, 0)->data(VariableLoadRoleBitOffset).toUInt()) : 0;
}

quint8 VariableLoadPage::bitSizeAt(int row) const {
    return table_->item(row, 0) ? static_cast<quint8>(table_->item(row, 0)->data(VariableLoadRoleBitSize).toUInt()) : 0;
}
