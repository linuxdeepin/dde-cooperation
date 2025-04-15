cmake_minimum_required(VERSION 3.10)
project(fmt_refactor VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 添加编译器警告标志
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 确保源代码目录存在
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples)

# 源文件
set(SOURCES
    src/fmt.cpp
    src/fmt_utils.cpp
    src/core.cpp
    src/format.cpp
)

# 创建库目标
add_library(fmt ${SOURCES})

# 安装规则
install(TARGETS fmt
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 测试目标
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# 示例目标
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()