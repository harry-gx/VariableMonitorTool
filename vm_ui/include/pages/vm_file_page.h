/*
 * 文件说明：UI 界面模块头文件，声明相关类型和函数接口。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#pragma once

#include <functional>
#include <QList>
#include <QWidget>

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QLabel;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QListWidget;
// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class QToolButton;

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class FilePage : public QWidget {
public:
    explicit FilePage(QWidget *parent = nullptr);

    /**
     * 函数说明：newButton，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *newButton() const;
    /**
     * 函数说明：saveButton，保存当前状态或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *saveButton() const;
    /**
     * 函数说明：loadButton，加载外部文件或配置。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *loadButton() const;
    /**
     * 函数说明：recentButton，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *recentButton() const;
    /**
     * 函数说明：exportButton，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *exportButton() const;
    /**
     * 函数说明：exitButton，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *exitButton() const;
    /**
     * 函数说明：recentList，执行本模块对应功能逻辑。
     * 输入：无。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QListWidget *recentList() const;

    /**
     * 函数说明：setProjectOpen，打开底层资源。
     * 输入：open：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setProjectOpen(bool open);
    /**
     * 函数说明：setRecentFiles，执行本模块对应功能逻辑。
     * 输入：files：函数输入参数，参与本函数的计算、查找或状态更新。；activeFile：函数输入参数，参与本函数的计算、查找或状态更新。；openCallback：函数输入参数，参与本函数的计算、查找或状态更新。；removeCallback：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void setRecentFiles(const QStringList &files,
                        const QString &activeFile,
                        std::function<void(const QString &)> openCallback,
                        std::function<void(const QString &)> removeCallback);

private:
    /**
     * 函数说明：makeButton，执行本模块对应功能逻辑。
     * 输入：text：文本内容或输入字符串。；icon：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    QToolButton *makeButton(const QString &text, int icon);
    /**
     * 函数说明：selectButton，执行本模块对应功能逻辑。
     * 输入：button：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：无返回值。
     */
    void selectButton(QToolButton *button);

    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *newButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *saveButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *loadButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *recentButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *exportButton_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QToolButton *exitButton_ = nullptr;
    /* 变量说明：commandButtons_，保存当前对象运行所需的状态、参数或缓存数据。 */
    QList<QToolButton *> commandButtons_;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QListWidget *recentList_ = nullptr;
    /* 变量说明：nullptr，保存当前对象运行所需的状态、参数或缓存数据。 */
    QLabel *currentConfigLabel_ = nullptr;
};
