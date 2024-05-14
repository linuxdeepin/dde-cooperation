// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "asioservice.h"

AsioService::AsioService()
{

}

void AsioService::onError(int error, const std::string& category, const std::string& message)
{
    std::cout << "Asio service caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}
