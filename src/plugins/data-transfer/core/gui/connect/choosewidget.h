#ifndef CHOOSEWIDGET_H
#define CHOOSEWIDGET_H

#include <QCheckBox>
#include <QFrame>

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

#endif
