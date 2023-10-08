#ifndef SELECTMAINWIDGET_H
#define SELECTMAINWIDGET_H

#include <QFrame>
#include <QCheckBox>
class QLabel;
class SidebarWidget;
enum SelectItemName { FILES, APP, CONFIG };
class SelectItem : public QFrame
{
    Q_OBJECT
public:
    SelectItem(QString text, QIcon icon, SelectItemName name, QWidget *parent = nullptr);
    ~SelectItem() override;
    void updateSelectSize(int num);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void initEditFrame();

public:
    SelectItemName name;
    bool isOk{ false };
private:
    QFrame *editFrame{ nullptr };
    QLabel *sizeLabel{ nullptr };

public slots:
    void changeState(const bool &ok);
signals:
    void changePage();
};

class selectMainWidget : public QFrame
{
    Q_OBJECT
public:
    selectMainWidget(QWidget *parent = nullptr);
    ~selectMainWidget();
    void changeSelectframeState(const SelectItemName &name,const bool &ok);

private:
    void initUi();

public slots:
    void nextPage();
    void backPage();
    void selectPage();

private:
    SelectItem *fileItem;
    SelectItem *appItem;
    SelectItem *configItem;
};

#endif // SELECTMAINWIDGET_H
