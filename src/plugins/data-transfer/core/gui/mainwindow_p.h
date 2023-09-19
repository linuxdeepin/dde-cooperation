#ifndef MAINWINDOW_P_H
#    define MAINWINDOW_P_H

#endif   // MAINWINDOW_P_H

#include <QDockWidget>
#include <QStackedLayout>

namespace data_transfer_core {

class MainWindow;
class MainWindowPrivate : public QObject
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit MainWindowPrivate(MainWindow *qq);
    virtual ~MainWindowPrivate();

protected:
    void initWindow();
    void initSideBar();
    void initWidgets();
    void moveCenter();

private slots:
    void handleCurrentChanged(int index);

protected:
    MainWindow *q { nullptr };
    QStackedLayout *mainLayout { nullptr };
    QDockWidget *sidebar { nullptr };
};

}
