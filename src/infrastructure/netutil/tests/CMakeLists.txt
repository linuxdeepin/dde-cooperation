cmake_minimum_required(VERSION 3.13)

# 项目名称
project(netutil_tests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 FetchContent 获取 Catch2
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://gitee.com/wehux/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# 添加测试可执行文件
file(GLOB TEST_SOURCES "*.cpp" "asio/*.cpp" "http/*.cpp")
add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 链接netutil库和Catch2
target_link_libraries(${PROJECT_NAME} PRIVATE netutil Catch2::Catch2WithMain)

# 启用测试
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}) 