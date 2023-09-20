#ifndef SELECTMAINWIDGET_H
#define SELECTMAINWIDGET_H

#include <QFrame>
#include <QCheckBox>
class QLabel;
class SidebarWidget;
enum SelectItemName { FILES, APP, DISPOSITION };
class SelectItem : public QFrame
{
    Q_OBJECT
public:
    SelectItem(QString text, QIcon icon, SelectItemName name, QWidget *parent = nullptr);
    ~SelectItem() override;
    void updateSelectSize(int num);

    SelectItemName name;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void initEditFrame();
    void changeStyle();
    QFrame *editFrame{ nullptr };
    QLabel *sizeLabel{ nullptr };
signals:
    void changePage();
};
class selectMainWidget : public QFrame
{
    Q_OBJECT
public:
    selectMainWidget(QWidget *parent = nullptr);
    ~selectMainWidget();

public slots:
    void nextPage();
    void backPage();
    void selectPage();
};

#endif // SELECTMAINWIDGET_H
