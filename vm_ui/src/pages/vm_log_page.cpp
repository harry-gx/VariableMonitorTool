#include "pages/vm_log_page.h"

#include <QtWidgets>

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

void LogPage::appendLine(const QString &line) {
    lines_.append(line);
    while (lines_.size() > 5000) {
        lines_.removeFirst();
    }
    if (lineMatchesFilter(line)) {
        view_->append(line.toHtmlEscaped());
    }
}

bool LogPage::exportAll(const QString &path) const {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const QByteArray bytes = lines_.join(QStringLiteral("\n")).toUtf8();
    return file.write(bytes) == bytes.size() && file.commit();
}

void LogPage::applyFilter() {
    view_->clear();
    for (const auto &line : lines_) {
        if (lineMatchesFilter(line)) {
            view_->append(line.toHtmlEscaped());
        }
    }
}

bool LogPage::lineMatchesFilter(const QString &line) const {
    const QString filter = filterEdit_->text();
    return filter.isEmpty() || line.contains(filter, Qt::CaseInsensitive);
}
