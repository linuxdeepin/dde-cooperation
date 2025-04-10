// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SSL_H
#define UASIO_SSL_H

/**
 * @file ssl.h
 * @brief SSL功能的主头文件
 * @details 包含所有SSL相关头文件
 */

#include "ssl/error.h"
#include "ssl/context.h"
#include "ssl/stream.h"
#include "ssl/stream_impl.h"

/**
 * @namespace uasio::ssl
 * @brief 提供SSL/TLS安全通信功能的命名空间
 */
namespace uasio {
namespace ssl {

// 基本功能类已在各自的头文件中定义

} // namespace ssl
} // namespace uasio

#endif // UASIO_SSL_H 