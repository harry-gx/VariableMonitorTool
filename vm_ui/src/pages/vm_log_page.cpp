/*
 * 文件说明：UI 界面模块Qt/C++ 实现文件。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "pages/vm_log_page.h"

#include <QtWidgets>

// 函数说明：QWidget，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
LogPage::LogPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    auto *row = new QHBoxLayout;
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(QStringLiteral("筛选运行日志"));
    exportButton_ = new QPushButton(QStringLiteral("导出全部日志"), this);
    auto *clearButton = new QPushButton(QStringLiteral("清空显示"), this);
    row->addWidget(filterEdit_, 1);
    row->addWidget(exportButton_);
    row->addWidget(clearButton);
    layout->addLayout(row);

    view_ = new QTextEdit(this);
    view_->setReadOnly(true);
    view_->document()->setMaximumBlockCount(5000);
    layout->addWidget(view_, 1);

    connect(filterEdit_, &QLineEdit::textChanged, this, [this] { applyFilter(); });
    connect(clearButton, &QPushButton::clicked, view_, &QTextEdit::clear);
}

QPushButton *LogPage::exportButton() const { return exportButton_; }

// 函数说明：LogPage::appendLine，追加数据到目标容器。
// 输入：line：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void LogPage::appendLine(const QString &line) {
    lines_.append(line);
    while (lines_.size() > 5000) {
        lines_.removeFirst();
    }
    if (lineMatchesFilter(line)) {
        view_->append(line.toHtmlEscaped());
    }
}

// 函数说明：LogPage::exportAll，执行本模块对应功能逻辑。
// 输入：path：待加载的文件路径。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool LogPage::exportAll(const QString &path) const {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const QByteArray bytes = lines_.join(QStringLiteral("\n")).toUtf8();
    return file.write(bytes) == bytes.size() && file.commit();
}

// 函数说明：LogPage::applyFilter，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void LogPage::applyFilter() {
    view_->clear();
    for (const auto &line : lines_) {
        if (lineMatchesFilter(line)) {
            view_->append(line.toHtmlEscaped());
        }
    }
}

// 函数说明：LogPage::lineMatchesFilter，执行本模块对应功能逻辑。
// 输入：line：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回 true 表示成功或条件成立，返回 false 表示失败或条件不成立。
bool LogPage::lineMatchesFilter(const QString &line) const {
    const QString filter = filterEdit_->text();
    return filter.isEmpty() || line.contains(filter, Qt::CaseInsensitive);
}
