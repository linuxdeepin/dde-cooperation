// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DATATRANSFERCOREPLUGIN_H
#define DATATRANSFERCOREPLUGIN_H

#include <QObject>

namespace data_transfer_core {

class MainWindow;

class DataTransferCorePlugin : public QObject
{
    Q_OBJECT

public:
    explicit DataTransferCorePlugin(QObject *parent = nullptr);
    ~DataTransferCorePlugin();

    bool start();
    void stop();

private:
    void initialize();
    bool loadMainPage();

private:
    MainWindow *w { nullptr };
};

}
#endif   // DATATRANSFERCOREPLUGIN_H
