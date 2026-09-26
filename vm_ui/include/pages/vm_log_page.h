#pragma once

#include <QStringList>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;

class LogPage : public QWidget {
public:
    explicit LogPage(QWidget *parent = nullptr);

    QPushButton *exportButton() const;
    void appendLine(const QString &line);
    bool exportAll(const QString &path) const;

private:
    bool lineMatchesFilter(const QString &line) const;
    void applyFilter();

    QStringList lines_;
    QLineEdit *filterEdit_ = nullptr;
    QTextEdit *view_ = nullptr;
    QPushButton *exportButton_ = nullptr;
};
