#ifndef ERRORWIDGET_H
#define ERRORWIDGET_H

#include <QFrame>
#include <QLabel>

enum ErrorType {
    networkError = 0,
    outOfStorageError
};

class ErrorWidget : public QFrame
{
    Q_OBJECT
public:
    ErrorWidget(QWidget *parent = nullptr);
    ~ErrorWidget();

public slots:
    void backPage();
    void retryPage();
    void themeChanged(int theme);
    void setErrorType(ErrorType type, int size = 0);

private:
    void initUI();

private:
    QLabel *titleLabel = nullptr;
    QLabel *promptLabel = nullptr;
};
#endif   // ERRORWIDGET_H
