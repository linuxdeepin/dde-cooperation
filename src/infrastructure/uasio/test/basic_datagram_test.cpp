// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include <gtest/gtest.h>

class DatagramSocketTest : public ::testing::Test {
protected:
    uasio::io_context io;
    uasio::datagram_socket socket{io};
    uasio::error_code ec;
};

TEST_F(DatagramSocketTest, BasicOpen) {
    socket.open(uasio::address_family::ipv4, ec);
    ASSERT_FALSE(ec) << "打开socket失败: " << ec.message();
    ASSERT_TRUE(socket.is_open());
}

TEST_F(DatagramSocketTest, BasicClose) {
    socket.open(uasio::address_family::ipv4, ec);
    ASSERT_FALSE(ec);
    
    socket.close(ec);
    ASSERT_FALSE(ec) << "关闭socket失败: " << ec.message();
    ASSERT_FALSE(socket.is_open());
}