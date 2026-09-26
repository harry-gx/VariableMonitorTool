#include "pages/vm_file_page.h"

#include <QtWidgets>

FilePage::FilePage(QWidget *parent) : QWidget(parent) {
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *side = new QFrame(this);
    side->setObjectName(QStringLiteral("fileSidebar"));
    side->setMinimumWidth(150);
    side->setMaximumWidth(180);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(10, 12, 10, 12);
    sideLayout->setSpacing(8);

    newButton_ = makeButton(QStringLiteral("新建"), QStyle::SP_FileIcon);
    saveButton_ = makeButton(QStringLiteral("保存"), QStyle::SP_DialogSaveButton);
    loadButton_ = makeButton(QStringLiteral("加载"), QStyle::SP_DialogOpenButton);
    recentButton_ = makeButton(QStringLiteral("最近使用"), QStyle::SP_DirOpenIcon);
    exportButton_ = makeButton(QStringLiteral("导出"), QStyle::SP_DialogSaveButton);
    exitButton_ = makeButton(QStringLiteral("退出"), QStyle::SP_DialogCloseButton);

    sideLayout->addWidget(newButton_);
    sideLayout->addWidget(saveButton_);
    sideLayout->addWidget(loadButton_);
    sideLayout->addWidget(recentButton_);
    sideLayout->addWidget(exportButton_);
    sideLayout->addWidget(exitButton_);
    sideLayout->addStretch();
    commandButtons_ << newButton_ << saveButton_ << loadButton_ << recentButton_ << exportButton_ << exitButton_;
    for (auto *button : commandButtons_) {
        connect(button, &QToolButton::clicked, this, [this, button] {
            selectButton(button);
        });
    }

    auto *body = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(10, 8, 10, 10);
    bodyLayout->setSpacing(4);
    currentConfigLabel_ = new QLabel(QStringLiteral("当前配置：未打开工程"), body);
    auto *title = new QLabel(QStringLiteral("最近使用配置"), body);
    title->setStyleSheet(QStringLiteral("font-size:16px;font-weight:600;"));
    recentList_ = new QListWidget(body);
    recentList_->setAlternatingRowColors(true);
    recentList_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    recentList_->setStyleSheet(QStringLiteral(
        "QListWidget{background:white;border:1px solid #9aa8b8;}"
        "QListWidget::item{border:0;}"));

    bodyLayout->addWidget(currentConfigLabel_);
    bodyLayout->addWidget(title);
    bodyLayout->addWidget(recentList_, 1);

    root->addWidget(side);
    root->addWidget(body, 1);

    setStyleSheet(QStringLiteral(
        "#fileSidebar{background:#eef2f6;border-right:1px solid #cbd5df;}"
        "#fileSidebar QToolButton{color:#243241;border:0;text-align:left;padding:8px;}"
        "#fileSidebar QToolButton:hover{background:#dbe5ee;}"
        "#fileSidebar QToolButton[active=\"true\"]{background:#d4e3f1;color:#0f2f4f;font-weight:600;}"));
    selectButton(recentButton_);
}

QToolButton *FilePage::makeButton(const QString &text, int icon) {
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setIcon(style()->standardIcon(static_cast<QStyle::StandardPixmap>(icon)));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(42);
    return button;
}

void FilePage::selectButton(QToolButton *button) {
    for (auto *candidate : commandButtons_) {
        candidate->setProperty("active", candidate == button);
        candidate->style()->unpolish(candidate);
        candidate->style()->polish(candidate);
        candidate->update();
    }
}

QToolButton *FilePage::newButton() const { return newButton_; }
QToolButton *FilePage::saveButton() const { return saveButton_; }
QToolButton *FilePage::loadButton() const { return loadButton_; }
QToolButton *FilePage::recentButton() const { return recentButton_; }
QToolButton *FilePage::exportButton() const { return exportButton_; }
QToolButton *FilePage::exitButton() const { return exitButton_; }
QListWidget *FilePage::recentList() const { return recentList_; }

void FilePage::setProjectOpen(bool open) {
    saveButton_->setEnabled(open);
    exportButton_->setEnabled(open);
}

void FilePage::setRecentFiles(const QStringList &files,
                              const QString &activeFile,
                              std::function<void(const QString &)> openCallback,
                              std::function<void(const QString &)> removeCallback) {
    currentConfigLabel_->setText(activeFile.isEmpty()
        ? QStringLiteral("当前配置：未打开工程")
        : QStringLiteral("当前配置：%1").arg(QDir::toNativeSeparators(activeFile)));
    recentList_->clear();
    for (const auto &path : files) {
        auto *item = new QListWidgetItem(recentList_);
        item->setData(Qt::UserRole, path);
        item->setSizeHint(QSize(100, 30));

        auto *row = new QWidget(recentList_);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(6);

        auto *openButton = new QPushButton(style()->standardIcon(QStyle::SP_FileIcon), path, row);
        openButton->setFlat(true);
        openButton->setToolTip(path);
        openButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        openButton->setStyleSheet(QStringLiteral("QPushButton{text-align:left;border:0;padding:3px;} QPushButton:hover{background:#eef2f6;}"));

        auto *removeButton = new QToolButton(row);
        removeButton->setText(QStringLiteral("X"));
        removeButton->setToolTip(QStringLiteral("删除历史记录"));
        removeButton->setAutoRaise(true);
        removeButton->setFixedWidth(24);

        layout->addWidget(openButton, 1);
        layout->addWidget(removeButton);
        recentList_->setItemWidget(item, row);

        connect(openButton, &QPushButton::clicked, row, [openCallback, path] {
            if (openCallback) {
                openCallback(path);
            }
        });
        connect(removeButton, &QToolButton::clicked, row, [removeCallback, path] {
            if (removeCallback) {
                removeCallback(path);
            }
        });
    }
}
