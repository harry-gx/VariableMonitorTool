/*
 * 文件说明：UI 界面模块Qt/C++ 实现文件。
 * 所属模块：UI 界面模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "pages/vm_monitor_page.h"

#include <QtWidgets>


// 类型说明：枚举限定模块状态、事件或设备类型的取值范围。
enum MonitorRoles {
    MonitorRoleAddress = Qt::UserRole + 1,
    MonitorRoleSize,
    MonitorRoleTypeName,
    MonitorRoleBitField,
    MonitorRoleBitOffset,
    MonitorRoleBitSize,
    MonitorRoleCurrentValue,
    MonitorRoleStatus,
    MonitorRoleUpdatedAt
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

// 函数说明：colorIcon，执行本模块对应功能逻辑。
// 输入：color：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
static QIcon colorIcon(const QColor &color) {
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(120, 128, 136)));
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(1.0, 1.0, 12.0, 12.0), 2.0, 2.0);

    return QIcon(pixmap);
}

// 类型说明：结构体保存模块状态、配置、变量描述或解析结果。
struct CurveSample {
    qint64 timeMs = 0;
    double value = 0.0;
};

// 类型说明：类封装对应 UI 组件或窗口的状态与操作。
class CurveCanvas : public QWidget {
public:
    explicit CurveCanvas(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumWidth(420);
        setMinimumHeight(240);
        setAutoFillBackground(true);
    }

    void configureCurve(quint64 key, const QString &name, const QColor &color) {
        // 变量说明：name，变量名、页面名或节点名。
        names_[key] = name;
        // 变量说明：color，保存当前对象运行所需的状态、参数或缓存数据。
        colors_[key] = color;
        if (!series_.contains(key)) {
            series_[key] = QVector<CurveSample>();
        }
        scheduleUpdate();
    }

    void setCurveVisible(quint64 key, bool visible) {
        if (visible) {
            visible_.insert(key);
        } else {
            visible_.remove(key);
        }
        scheduleUpdate();
    }

    void clearCurves() {
        names_.clear();
        colors_.clear();
        series_.clear();
        visible_.clear();
        scheduleUpdate();
    }

    void appendValue(quint64 key, double value) {
        if (!series_.contains(key)) {
            series_[key] = QVector<CurveSample>();
        }

        // 变量说明：series_，保存当前对象运行所需的状态、参数或缓存数据。
        auto &points = series_[key];
        // 变量说明：sample，保存当前对象运行所需的状态、参数或缓存数据。
        CurveSample sample;
        sample.timeMs = QDateTime::currentMSecsSinceEpoch();
        // 变量说明：value，保存当前对象运行所需的状态、参数或缓存数据。
        sample.value = value;
        points.append(sample);
        while (points.size() > 300) {
            points.removeFirst();
        }
        scheduleUpdate();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);

        const QRect plot = rect().adjusted(64, 24, -22, -48);
        if (plot.width() <= 20 || plot.height() <= 20) {
            // 变量说明：return，保存当前对象运行所需的状态、参数或缓存数据。
            return;
        }

        double minValue = 0.0;
        double maxValue = 1.0;
        qint64 minTime = 0;
        qint64 maxTime = 1000;
        // 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。
        bool hasPoint = false;

        for (quint64 key : visible_) {
            for (const auto &sample : series_.value(key)) {
                if (!hasPoint) {
                    // 变量说明：value，保存当前对象运行所需的状态、参数或缓存数据。
                    minValue = maxValue = sample.value;
                    // 变量说明：timeMs，保存当前对象运行所需的状态、参数或缓存数据。
                    minTime = maxTime = sample.timeMs;
                    // 变量说明：true，保存当前对象运行所需的状态、参数或缓存数据。
                    hasPoint = true;
                } else {
                    minValue = qMin(minValue, sample.value);
                    maxValue = qMax(maxValue, sample.value);
                    minTime = qMin(minTime, sample.timeMs);
                    maxTime = qMax(maxTime, sample.timeMs);
                }
            }
        }

        if (qFuzzyCompare(minValue, maxValue)) {
            minValue -= 1.0;
            maxValue += 1.0;
        }
        if (maxTime <= minTime) {
            maxTime = minTime + 1000;
        }

        const int xDivisions = 10;
        const int yDivisions = 8;
        // 变量说明：minValue，保存当前对象运行所需的状态、参数或缓存数据。
        const double valueSpan = maxValue - minValue;
        // 变量说明：minTime，保存当前对象运行所需的状态、参数或缓存数据。
        const qint64 timeSpan = maxTime - minTime;

        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(218, 224, 230)));
        for (int i = 0; i <= xDivisions; ++i) {
            const int x = plot.left() + plot.width() * i / xDivisions;
            painter.drawLine(x, plot.top(), x, plot.bottom());
        }
        for (int i = 0; i <= yDivisions; ++i) {
            const int y = plot.top() + plot.height() * i / yDivisions;
            painter.drawLine(plot.left(), y, plot.right(), y);
        }

        painter.setPen(QPen(QColor(156, 166, 176)));
        painter.drawRect(plot);

        painter.setPen(QColor(70, 76, 82));
        const QFontMetrics fm(painter.font());
        for (int i = 0; i <= yDivisions; ++i) {
            const int y = plot.top() + plot.height() * i / yDivisions;
            // 变量说明：yDivisions，保存当前对象运行所需的状态、参数或缓存数据。
            const double value = maxValue - valueSpan * i / yDivisions;
            const QString label = QString::number(value, 'f', 2);
            painter.drawText(4, y + fm.ascent() / 2, label);
        }

        const bool millisecondAxis = timeSpan < 10000;
        for (int i = 0; i <= xDivisions; ++i) {
            const int x = plot.left() + plot.width() * i / xDivisions;
            // 变量说明：xDivisions，保存当前对象运行所需的状态、参数或缓存数据。
            const qint64 elapsed = timeSpan * i / xDivisions;
            const QString label = millisecondAxis
                ? QStringLiteral("%1 ms").arg(elapsed)
                : QStringLiteral("%1 s").arg(elapsed / 1000.0, 0, 'f', 1);
            const int labelWidth = fm.width(label);
            int labelX = x - labelWidth / 2;
            labelX = qBound(0, labelX, width() - labelWidth - 2);
            painter.drawText(labelX, plot.bottom() + fm.height() + 4, label);
        }

        painter.drawText(plot.left(), 16, QStringLiteral("曲线"));
        painter.drawText(plot.right() - 72, height() - 8,
                         millisecondAxis ? QStringLiteral("time/ms") : QStringLiteral("time/s"));

        painter.setRenderHint(QPainter::Antialiasing, true);
        for (quint64 key : visible_) {
            const QVector<CurveSample> points = series_.value(key);
            if (points.size() < 2) {
                // 变量说明：continue，保存当前对象运行所需的状态、参数或缓存数据。
                continue;
            }

            // 变量说明：path，保存当前对象运行所需的状态、参数或缓存数据。
            QPainterPath path;
            // 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。
            bool started = false;
            for (const auto &sample : points) {
                const double xRatio = static_cast<double>(sample.timeMs - minTime) / static_cast<double>(timeSpan);
                const double yRatio = (sample.value - minValue) / valueSpan;
                const QPointF point(
                    plot.left() + xRatio * plot.width(),
                    plot.bottom() - yRatio * plot.height());
                if (!started) {
                    path.moveTo(point);
                    // 变量说明：true，保存当前对象运行所需的状态、参数或缓存数据。
                    started = true;
                } else {
                    path.lineTo(point);
                }
            }

            painter.setPen(QPen(colors_.value(key, Qt::blue), 2));
            painter.drawPath(path);
        }
    }

private:
    void scheduleUpdate() {
        if (updatePending_) {
            // 变量说明：return，保存当前对象运行所需的状态、参数或缓存数据。
            return;
        }
        // 变量说明：true，保存当前对象运行所需的状态、参数或缓存数据。
        updatePending_ = true;
        QTimer::singleShot(33, this, [this] {
            // 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。
            updatePending_ = false;
            update();
        });
    }

    // 变量说明：names_，保存当前对象运行所需的状态、参数或缓存数据。
    QMap<quint64, QString> names_;
    // 变量说明：colors_，保存当前对象运行所需的状态、参数或缓存数据。
    QMap<quint64, QColor> colors_;
    // 变量说明：series_，保存当前对象运行所需的状态、参数或缓存数据。
    QMap<quint64, QVector<CurveSample>> series_;
    // 变量说明：visible_，保存当前对象运行所需的状态、参数或缓存数据。
    QSet<quint64> visible_;
    // 变量说明：false，保存当前对象运行所需的状态、参数或缓存数据。
    bool updatePending_ = false;
};

// 函数说明：QWidget，执行本模块对应功能逻辑。
// 输入：parent：Qt 父对象指针。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
MonitorPage::MonitorPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto *actions = new QHBoxLayout;
    actions->addWidget(new QLabel(QStringLiteral("采样周期"), this));
    periodSpinBox_ = new QSpinBox(this);
    periodSpinBox_->setRange(20, 60000);
    periodSpinBox_->setValue(100);
    periodSpinBox_->setSuffix(QStringLiteral(" ms"));
    readButton_ = new QPushButton(QStringLiteral("读取一次"), this);
    startButton_ = new QPushButton(QStringLiteral("开始监控"), this);
    stopButton_ = new QPushButton(QStringLiteral("停止监控"), this);
    actions->addWidget(periodSpinBox_);
    actions->addWidget(readButton_);
    actions->addWidget(startButton_);
    actions->addWidget(stopButton_);
    actions->addStretch();
    layout->addLayout(actions);

    table_ = new QTableWidget(0, 2, this);
    table_->setHorizontalHeaderLabels(QStringList() << QStringLiteral("变量名")
                                                    << QStringLiteral("当前值"));
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);

    curveCanvas_ = new CurveCanvas(this);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(table_);
    splitter->addWidget(curveCanvas_);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 4);
    layout->addWidget(splitter, 1);

    auto *hint = new QLabel(QStringLiteral("勾选变量后会按采样周期优先读取并绘制曲线；未勾选变量和标定当前值会后台刷新"), this);
    layout->addWidget(hint);

    connect(table_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (!item || item->column() != 0) {
            return;
        }
        curveCanvas_->setCurveVisible(static_cast<quint64>(item->row()), item->checkState() == Qt::Checked);
    });
}

QPushButton *MonitorPage::readButton() const { return readButton_; }
QPushButton *MonitorPage::startButton() const { return startButton_; }
QPushButton *MonitorPage::stopButton() const { return stopButton_; }
QSpinBox *MonitorPage::periodSpinBox() const { return periodSpinBox_; }

// 函数说明：MonitorPage::setReady，读取数据或发起读取请求。
// 输入：ready：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorPage::setReady(bool ready) {
    const bool hasRows = table_->rowCount() > 0;
    table_->setEnabled(ready);
    readButton_->setEnabled(ready && hasRows);
    startButton_->setEnabled(ready && hasRows);
    stopButton_->setEnabled(ready && hasRows);
}

// 函数说明：MonitorPage::setRows，执行本模块对应功能逻辑。
// 输入：variables：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorPage::setRows(const QVector<UiVariable> &variables) {
    QSignalBlocker blocker(table_);
    table_->setRowCount(0);
    rowsByAddress_.clear();
    curveCanvas_->clearCurves();

    for (const auto &variable : variables) {
        const int row = table_->rowCount();
        table_->insertRow(row);

        const QColor color = QColor::fromHsv((row * 47) % 360, 190, 210);
        auto *item = new QTableWidgetItem(colorIcon(color), variable.name);
        item->setCheckState(Qt::Unchecked);
        item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        item->setData(MonitorRoleAddress, variable.address);
        item->setData(MonitorRoleSize, variable.size);
        item->setData(MonitorRoleTypeName, variable.typeName);
        item->setData(MonitorRoleBitField, variable.bitField);
        item->setData(MonitorRoleBitOffset, variable.bitOffset);
        item->setData(MonitorRoleBitSize, variable.bitSize);
        item->setData(MonitorRoleCurrentValue, QStringLiteral("未读取"));
        item->setData(MonitorRoleStatus, QStringLiteral("待读取"));
        item->setData(MonitorRoleUpdatedAt, QStringLiteral("-"));
        item->setToolTip(QStringLiteral("地址：%1\n字节数：%2\n类型：%3%4\n当前值：未读取\n状态：待读取\n更新时间：-")
                             .arg(variable.address,
                                  variable.size,
                                  variable.typeName.isEmpty() ? QStringLiteral("未知") : variable.typeName,
                                  variable.bitField
                                      ? QStringLiteral("\n位字段：bit[%1:%2]")
                                            .arg(variable.bitOffset)
                                            .arg(variable.bitOffset + variable.bitSize - 1)
                                      : QString()));
        table_->setItem(row, 0, item);

        auto *valueItem = new QTableWidgetItem(QStringLiteral("未读取"));
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 1, valueItem);

        const quint32 address = addressFromText(variable.address);
        rowsByAddress_[address].append(row);
        curveCanvas_->configureCurve(static_cast<quint64>(row), variable.name, color);
    }
}

// 函数说明：MonitorPage::updateValue，刷新界面数据或内部状态。
// 输入：address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；value：函数输入参数，参与本函数的计算、查找或状态更新。；numericValue：函数输入参数，参与本函数的计算、查找或状态更新。；hasNumericValue：函数输入参数，参与本函数的计算、查找或状态更新。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：无返回值。
void MonitorPage::updateValue(quint32 address,
                              quint16 size,
                              const QString &value,
                              double numericValue,
                              bool hasNumericValue) {
    const auto it = rowsByAddress_.constFind(address);
    if (it == rowsByAddress_.constEnd()) {
        return;
    }

    for (int row : it.value()) {
        auto *item = table_->item(row, 0);
        if (!item) {
            continue;
        }

        const quint16 rowSize = static_cast<quint16>(item->data(MonitorRoleSize).toString().toUShort());
        if (rowSize != size) {
            continue;
        }

        const QString typeName = item->data(MonitorRoleTypeName).toString();
        const bool bitField = item->data(MonitorRoleBitField).toBool();
        const quint8 bitOffset = static_cast<quint8>(item->data(MonitorRoleBitOffset).toUInt());
        const quint8 bitSize = static_cast<quint8>(item->data(MonitorRoleBitSize).toUInt());
        const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
        item->setData(MonitorRoleCurrentValue, value);
        item->setData(MonitorRoleStatus, QStringLiteral("正常"));
        item->setData(MonitorRoleUpdatedAt, time);
        item->setToolTip(QStringLiteral("地址：%1\n字节数：%2\n类型：%3%4\n当前值：%5\n状态：正常\n更新时间：%6")
                             .arg(item->data(MonitorRoleAddress).toString(),
                                  item->data(MonitorRoleSize).toString(),
                                  typeName.isEmpty() ? QStringLiteral("未知") : typeName,
                                  bitField
                                      ? QStringLiteral("\n位字段：bit[%1:%2]")
                                            .arg(bitOffset)
                                            .arg(bitOffset + bitSize - 1)
                                      : QString(),
                                  value,
                                  time));
        if (table_->item(row, 1)) {
            table_->item(row, 1)->setText(value);
        }

        if (item->checkState() == Qt::Checked && hasNumericValue) {
            curveCanvas_->appendValue(static_cast<quint64>(row), numericValue);
        }
    }
}

int MonitorPage::rowCount() const { return table_->rowCount(); }

// 函数说明：MonitorPage::checkedRows，执行本模块对应功能逻辑。
// 输入：无。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
QVector<int> MonitorPage::checkedRows() const {
    QVector<int> rows;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto *item = table_->item(row, 0);
        if (item && item->checkState() == Qt::Checked) {
            rows.append(row);
        }
    }
    return rows;
}

// 函数说明：MonitorPage::addressAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint32 MonitorPage::addressAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? addressFromText(item->data(MonitorRoleAddress).toString()) : 0;
}

// 函数说明：MonitorPage::sizeAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
quint16 MonitorPage::sizeAt(int row) const {
    auto *item = table_->item(row, 0);
    return item ? static_cast<quint16>(item->data(MonitorRoleSize).toString().toUShort()) : 0;
}

// 函数说明：MonitorPage::variableAt，执行本模块对应功能逻辑。
// 输入：row：界面表格行号。
// 输出：通过返回值、对象成员或输出参数反馈处理结果。
// 返回：返回执行结果，具体含义由调用方按接口约定解释。
UiVariable MonitorPage::variableAt(int row) const {
    auto *item = table_->item(row, 0);
    if (!item) {
        return UiVariable();
    }

    return UiVariable(item->text(),
                      item->data(MonitorRoleAddress).toString(),
                      item->data(MonitorRoleSize).toString(),
                      false,
                      item->data(MonitorRoleTypeName).toString(),
                      item->data(MonitorRoleBitField).toBool(),
                      static_cast<quint8>(item->data(MonitorRoleBitOffset).toUInt()),
                      static_cast<quint8>(item->data(MonitorRoleBitSize).toUInt()));
}
