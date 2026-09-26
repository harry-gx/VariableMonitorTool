#pragma once

#include <functional>
#include <QList>
#include <QWidget>

class QLabel;
class QListWidget;
class QToolButton;

class FilePage : public QWidget {
public:
    explicit FilePage(QWidget *parent = nullptr);

    QToolButton *newButton() const;
    QToolButton *saveButton() const;
    QToolButton *loadButton() const;
    QToolButton *recentButton() const;
    QToolButton *exportButton() const;
    QToolButton *exitButton() const;
    QListWidget *recentList() const;

    void setProjectOpen(bool open);
    void setRecentFiles(const QStringList &files,
                        const QString &activeFile,
                        std::function<void(const QString &)> openCallback,
                        std::function<void(const QString &)> removeCallback);

private:
    QToolButton *makeButton(const QString &text, int icon);
    void selectButton(QToolButton *button);

    QToolButton *newButton_ = nullptr;
    QToolButton *saveButton_ = nullptr;
    QToolButton *loadButton_ = nullptr;
    QToolButton *recentButton_ = nullptr;
    QToolButton *exportButton_ = nullptr;
    QToolButton *exitButton_ = nullptr;
    QList<QToolButton *> commandButtons_;
    QListWidget *recentList_ = nullptr;
    QLabel *currentConfigLabel_ = nullptr;
};
