# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# 包含CTest模块
include(CTest)
enable_testing()

# 查找gtest
find_package(GTest REQUIRED)

# 自动搜索所有的测试文件
file(GLOB TEST_SOURCES "*.cpp" EXCLUDE test_datagram_socket.cpp)

# 为每个测试文件创建兼容版本
foreach(TEST_SOURCE ${TEST_SOURCES})
    # 获取文件名(不含扩展名)作为可执行文件名
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    get_filename_component(TEST_FILENAME ${TEST_SOURCE} NAME)
    
    # 在构建目录中创建修改后的源文件
    set(MODIFIED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILENAME}")
    configure_file(${TEST_SOURCE} ${MODIFIED_SOURCE} @ONLY)
    
    # 添加可执行文件，使用修改后的源文件
    add_executable(${TEST_NAME} ${MODIFIED_SOURCE})
    
    # 链接 uasio 库
    target_link_libraries(${TEST_NAME} PRIVATE uasio)
    target_link_libraries(${TEST_NAME} PRIVATE GTest::GTest GTest::Main)
    
    # 添加包含路径
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
    
    # 添加替换命名空间的预处理定义
    target_compile_definitions(${TEST_NAME} PRIVATE "asio=uasio")
    
    # 添加测试
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    message(STATUS "Added test: ${TEST_NAME}")
endforeach() 