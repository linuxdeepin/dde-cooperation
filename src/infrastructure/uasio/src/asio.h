#ifndef UASIO_ASIO_H
#define UASIO_ASIO_H

// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file asio.h
 * @brief asio头文件
 * @details 包含asio库的所有头文件
 */

#include "io_context.h"
#include "timer.h"
#include "error.h"
#include "socket.h"
#include "socket_service.h"
#include "resolver.h"
#include "buffer.h"
#include "streambuf.h"
#include "strand.h"
#include "signal_set.h"
#include "steady_timer.h"
#include "datagram_socket.h"
#include "ssl.h"
#include "file.h"
#include "ip/address.h"
#include "ip/address_v4.h"
#include "ip/address_v6.h"
#include "ip/network_v4.h"
#include "ip/network_v6.h"
#include "multiple_exceptions.h"
#include "cancellation_signal.h"
#include "system_context.h"
#include "thread_pool.h"
#include "wait_group.h"
#include "connection_pool.h"
// 移除有问题的头文件
// #include "future.h"
// #include "promise.h"
// #include "packaged_task.h"

/**
 * @namespace uasio
 * @brief 提供异步IO操作的命名空间
 */
namespace uasio {

// 基本功能类已在各自的头文件中定义

} // namespace uasio

#endif // UASIO_ASIO_H 