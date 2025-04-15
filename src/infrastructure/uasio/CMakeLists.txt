# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# 兼容性问题说明：
# 1. uasio 库与 Boost.Asio 存在 API 不兼容问题，主要包括：
#    - 错误码处理：uasio 使用 std::error_code，不能直接与整数常量比较
#    - API 接口差异：如 make_work() vs make_work_guard(), remote_endpoint() 参数要求等
#    - 某些API不完全实现：如缺少 expires_after() 方法
# 2. 简单使用 -Dasio=uasio 无法解决所有问题，因为核心API行为不同
# 3. 示例和测试程序需要单独适配，不能直接通过命名空间替换兼容
#
# 解决方案：
# 1. 将主库构建为独立组件，不编译示例和测试（默认）
# 2. 若需要示例和测试，需转到单独的分支开发兼容性代码
# 3. 长期解决方案应修改 uasio 库的 API 以更好地兼容 Boost.Asio

cmake_minimum_required(VERSION 3.10)
project(uasio VERSION 0.1)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# 构建选项
option(BUILD_EXAMPLES "构建示例程序" OFF)
option(BUILD_TESTS "构建测试程序" OFF)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

# 源文件列表 - 包含基本网络功能和SSL文件
set(SOURCES
    src/error.cpp
    src/execution_context.cpp
    src/io_context.cpp
    src/socket.cpp
    src/socket_service.cpp
    src/resolver.cpp
    # SSL 相关文件
    src/ssl/error.cpp
    src/ssl/context.cpp
)

# 添加IP地址相关源文件
list(APPEND SOURCES
    src/ip/address_v4.cpp
    src/ip/address_v6.cpp
    src/datagram_socket.cpp
)

# 平台特定源文件
if(UNIX)
    list(APPEND SOURCES src/linux_epoll.cpp)
    message(STATUS "Building for Unix-like system.")
elseif(WIN32)
    list(APPEND SOURCES src/win_iocp.cpp)
    message(STATUS "Building for Windows.")
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

# 创建库
add_library(uasio STATIC ${SOURCES})

# 设置包含目录
target_include_directories(uasio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
        ${OPENSSL_INCLUDE_DIR}
)

# 链接OpenSSL库
target_link_libraries(uasio PRIVATE ${OPENSSL_LIBRARIES})

# 平台相关链接库
if(WIN32)
    target_link_libraries(uasio PRIVATE ws2_32 mswsock)
endif()

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(uasio PRIVATE Threads::Threads)

# 条件添加examples子目录
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 条件添加test子目录
if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# 设置CTest
enable_testing() 