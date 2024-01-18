#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#    include <QMainWindow>
#    include <QPushButton>
typedef QMainWindow CrossMainWindow;
typedef QPushButton CooperationSuggestButton;
#else
#    include <DMainWindow>
#    include <DSuggestButton>
typedef DTK_WIDGET_NAMESPACE::DMainWindow CrossMainWindow;
typedef DTK_WIDGET_NAMESPACE::DSuggestButton CooperationSuggestButton;
#endif

#ifdef WIN32
enum PageName {
    startwidget = 0,
    choosewidget,
    promptwidget,
    readywidget,
    selectmainwidget,
    filewselectidget,
    configselectwidget,
    appselectwidget,
    transferringwidget,
    resultwidget,
    errorwidget,
    createbackupfilewidget,
    networkdisconnectwidget,
    zipfileprocesswidget,
    zipfileprocessresultwidget
};
#else
enum PageName {
    startwidget = 0,
    choosewidget,
    networkdisconnectwidget,
    promptwidget,
    connectwidget,
    uploadwidget,
    waitgwidget,
    errorwidget,
    transferringwidget,
    resultwidget
};
#endif

#include <QHBoxLayout>
class ButtonLayout : public QHBoxLayout
{
    Q_OBJECT

public:
    ButtonLayout(QWidget *parent = nullptr);
    ~ButtonLayout();

    QPushButton *getButton1() const;
    QPushButton *getButton2() const;

public slots:
    void themeChanged(int theme);

private:
    QPushButton *button1 { nullptr };
    QPushButton *button2 { nullptr };
};

class StyleHelper
{
public:
    enum TextStyle {
        normal = 0,
        error
    };

public:
    StyleHelper();

    static QFont font(int type);
    static QString longBtnStyle(int theme);
    static QString textStyle(TextStyle type);
};

#include <QLabel>
#include <QPushButton>
class IndexLabel : public QLabel
{
public:
    IndexLabel(int index, QWidget *parent = nullptr);

    void setIndex(int i);

private:
    int index;

protected:
    void paintEvent(QPaintEvent *event) override;
};
#endif   // TYPE_DEFINES_H
