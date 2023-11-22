#ifndef CHOOSEWIDGET_H
#define CHOOSEWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include "../type_defines.h"
class ModeItem;
class ChooseWidget : public QFrame
{
    Q_OBJECT

public:
    ChooseWidget(QWidget *parent = nullptr);
    ~ChooseWidget();

public slots:
    void nextPage();
    void themeChanged(int theme);

private:
    void initUI();
    void sendOptions();
    void changeAllWidgtText();
    void clearAllWidget();

private:
    QString transferMethod;
    QToolButton *nextButton = nullptr;
    int nextpage;
    ModeItem *winItem{nullptr};
    ModeItem *packageItem{nullptr};
    QString internetMethodName{ tr("From Windows PC") };
#ifdef WIN32
    QString localFileMethodName { tr("Export to local directory") };
    int selecPage1 = PageName::promptwidget;
    int selecPage2 = PageName::selectmainwidget;
#else
    QString localFileMethodName { tr("Import from backup files") };
    int selecPage1{ PageName::promptwidget };
    int selecPage2{ PageName::uploadwidget };
#endif
};

class ModeItem : public QFrame
{
    Q_OBJECT
    friend ChooseWidget;
public:
    ModeItem(QString text, QIcon icon, QWidget *parent = nullptr);
    ~ModeItem() override;

    void setEnable(bool able);
    void themeChanged(int theme);
signals:
    void clicked(bool checked);

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool enable{ true };
    bool checked{ false };
    QLabel *iconLabel{ nullptr };
    QString itemText;
    bool drak{false};
};

class IndexLabel : public QLabel
{
public:
    IndexLabel(int index, QWidget *parent = nullptr) : QLabel(parent), index(index)
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
