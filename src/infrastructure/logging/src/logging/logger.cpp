// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/logger.h"

#include "logging/config.h"

namespace Logging {

Logger::Logger() : _sink(Config::CreateLogger()._sink)
{
}

Logger::Logger(const std::string& name) : _name(name), _sink(Config::CreateLogger(name)._sink)
{
}

void Logger::Update()
{
    _sink = Config::CreateLogger(_name)._sink;
}

} // namespace Logging
