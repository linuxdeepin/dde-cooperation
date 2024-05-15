// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ASIOSERVICE_H
#define ASIOSERVICE_H

#include "server/asio/service.h"

class AsioService : public CppServer::Asio::Service
{
public:
    AsioService();
protected:
    void onError(int error, const std::string& category, const std::string& message) override;
};

#endif // ASIOSERVICE_H
