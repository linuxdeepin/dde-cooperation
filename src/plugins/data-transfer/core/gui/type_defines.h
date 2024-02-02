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
    waitwidget,
    errorwidget,
    transferringwidget,
    resultwidget
};
#endif

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QItemDelegate>
#include <QListView>
class QSvgRenderer;
class ButtonLayout : public QHBoxLayout
{
    Q_OBJECT

public:
    ButtonLayout(QWidget *parent = nullptr);
    ~ButtonLayout();

    void setCount(int count);
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
    enum ButtonStyle {
        gray = 0,
        blue
    };

public:
    StyleHelper();

    static QFont font(int type);
    static QString textStyle(TextStyle type);
    static QString buttonStyle(int type);
    static QString textBrowserStyle(int type);
};

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

class MovieWidget : public QWidget
{
    Q_OBJECT
public:
    MovieWidget(QString filename, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void nextFrame();
    void loadFrames();

private:
    QString movie;
    QTimer *timer;
    QVector<QPixmap> frames;   // 存储图像帧
    int currentFrame = 0;   // 当前帧索引
};

class ProcessWindowItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ProcessWindowItemDelegate();
    ~ProcessWindowItemDelegate() override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setTheme(int newTheme);
    void addIcon(const QString &path);
    void setStageColor(QColor color);
private:
    void paintText(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintIcon(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

private:
    int theme { 1 };
    QVector<QSvgRenderer*> renders;
    QColor stageTextColor;
};
class ProcessDetailsWindow : public QListView
{
    Q_OBJECT
public:
    ProcessDetailsWindow(QFrame *parent = nullptr);
    ~ProcessDetailsWindow();
    void clear();
};
#endif   // TYPE_DEFINES_H
