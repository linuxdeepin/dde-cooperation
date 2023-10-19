#ifndef LICENSEWIDGET_H
#define LICENSEWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QToolButton>

class LicenseWidget : public QFrame
{
    Q_OBJECT

public:
    LicenseWidget(QWidget *parent = nullptr);
    ~LicenseWidget();

public slots:
    void backPage();
    void nextPage();
    void themeChanged(int theme);

private:
    void initUI();
    QCheckBox *checkBox { nullptr };
};

#endif
