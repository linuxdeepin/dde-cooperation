#ifndef TRANSFERRWIDGET_H
#define TRANSFERRWIDGET_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

class TransferringWidget : public QFrame
{
    Q_OBJECT

public:
    TransferringWidget(QWidget *parent = nullptr);
    ~TransferringWidget();

public slots:
    void nextPage();

private:
    void initUI();
};

class ProgressBarLabel : public QLabel
{
public:
    ProgressBarLabel(QWidget *parent = nullptr)
        : QLabel(parent), m_progress(0)
    {
        setFixedSize(280, 8);
    }

    void setProgress(int progress)
    {
        m_progress = progress;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QLabel::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        // 绘制背景
        painter.setBrush(QColor(220, 220, 220));
        painter.drawRoundedRect(rect(), 5, 5);

        // 绘制进度条
        int width = static_cast<int>(rect().width() * (m_progress / 100.0));
        QRectF progressRect(rect().left(), rect().top(), width, rect().height());
        QLinearGradient gradient(progressRect.topLeft(), progressRect.topRight());
        QColor start;
        QColor mid;
        QColor end;
        start.setNamedColor("#0080FF");
        mid.setNamedColor("#0397FE");
        end.setNamedColor("#06BEFD");
        gradient.setColorAt(0, start);
        gradient.setColorAt(0.28, mid);
        gradient.setColorAt(1, end);

        painter.setBrush(gradient);
        painter.drawRoundedRect(progressRect, 5, 5);
    }

private:
    int m_progress;
};

#endif
