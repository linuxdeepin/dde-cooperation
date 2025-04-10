# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# 自动搜索所有的 .cpp 文件
file(GLOB EXAMPLE_SOURCES "*.cpp")

# 为每个源文件创建兼容版本
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    # 获取文件名(不含扩展名)作为可执行文件名
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    get_filename_component(EXAMPLE_FILENAME ${EXAMPLE_SOURCE} NAME)
    
    # 在构建目录中创建修改后的源文件
    set(MODIFIED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE_FILENAME}")
    configure_file(${EXAMPLE_SOURCE} ${MODIFIED_SOURCE} @ONLY)
    
    # 添加可执行文件，使用修改后的源文件
    add_executable(${EXAMPLE_NAME} ${MODIFIED_SOURCE})
    
    # 链接 uasio 库
    target_link_libraries(${EXAMPLE_NAME} PRIVATE uasio)
    
    # 添加包含路径
    target_include_directories(${EXAMPLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    
    # 添加替换命名空间的预处理定义
    target_compile_definitions(${EXAMPLE_NAME} PRIVATE "asio=uasio")
    
    message(STATUS "Added example: ${EXAMPLE_NAME}")
endforeach() 