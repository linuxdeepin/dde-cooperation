#ifndef RESULTDISPLAYWIDGET_H
#define RESULTDISPLAYWIDGET_H

#include <QFrame>

class QTextBrowser;
class QLabel;

class ResultDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    ResultDisplayWidget(QWidget *parent = nullptr);
    ~ResultDisplayWidget();

    QString ellipsizedText(const QString &input, int maxLength, const QFont &font);
public slots:
    void themeChanged(int theme);
    void addResult(QString name, bool success, QString reason);
    void clear();
    void setStatus(bool success);

private:
    void initUI();
    void nextPage();

private:
    QTextBrowser *processTextBrowser { nullptr };
    QLabel *iconLabel { nullptr };
    QLabel *titileLabel { nullptr };
    QLabel *tiptextlabel { nullptr };
};

#endif
