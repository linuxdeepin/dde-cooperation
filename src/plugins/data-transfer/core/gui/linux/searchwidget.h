#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>

class SearchWidget : public QFrame
{
    Q_OBJECT

public:
    SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget();

    void setTip(bool status) const;

public slots:
    void nextPage();

private:
    QGridLayout *userlayout { nullptr };
    QLabel *tipLabel { nullptr };
    bool status = false;
    void initUI();
    void updateUserlayout();
};

class Useritem : public QWidget
{
    Q_OBJECT

public:
    Useritem(QString name, QWidget *parent = nullptr);
    ~Useritem() override;

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    QString name;
};

#endif
