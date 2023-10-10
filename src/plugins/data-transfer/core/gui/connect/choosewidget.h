#ifndef CHOOSEWIDGET_H
#define CHOOSEWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QPainter>

class ChooseWidget : public QFrame
{
    Q_OBJECT

public:
    ChooseWidget(QWidget *parent = nullptr);
    ~ChooseWidget();

public slots:
    void nextPage();

private:
    void initUI();

private:
    int nextpage = 0;
};

class ModeItem : public QFrame
{
    Q_OBJECT

    friend ChooseWidget;

public:
    ModeItem(QString text, QIcon icon, QWidget *parent = nullptr);
    ~ModeItem() override;

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    QCheckBox *checkBox { nullptr };
};

class IndexLabel : public QLabel
{
public:
    IndexLabel(int index, QWidget *parent = nullptr)
        : QLabel(parent), index(index)
    {
        setFixedSize(50, 10);
    }

private:
    int index;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        int diam = 6;

        QColor brushColor;
        brushColor.setNamedColor("#0081FF");
        for (int i = 0; i < 4; i++) {
            if (i == index)
                brushColor.setAlpha(190);
            else
                brushColor.setAlpha(40);

            painter.setBrush(brushColor);
            painter.drawEllipse((diam + 5) * i, 0, diam, diam);
        }
    }
};

#endif
