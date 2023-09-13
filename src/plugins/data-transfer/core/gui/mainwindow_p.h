#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#endif // MAINWINDOW_P_H

#include <QStackedLayout>

namespace data_transfer_core {

class MainWindow;
class MainWindowPrivate {
    friend class MainWindow;

public:
    explicit MainWindowPrivate(MainWindow *qq);
    virtual ~MainWindowPrivate();

protected:
    void initWindow();
    void initWidgets();
    void moveCenter();

protected:
    MainWindow *q { nullptr };
    QStackedLayout *mainLayout { nullptr };
};

}
