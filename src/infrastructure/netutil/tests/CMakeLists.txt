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

# asio 覆盖测试访问私有/保护成员，需要禁用访问控制
target_compile_options(${PROJECT_NAME} PRIVATE -fno-access-control)

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
include("${Catch2_SOURCE_DIR}/extras/Catch.cmake")
catch_discover_tests(${PROJECT_NAME}) 
# ---- netutil_http_tests: 仅 HTTP 消息解析（无网络、无 asio 阻塞）----
add_executable(netutil_http_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/http/http_message_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/http/http_parse_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/http/http_coverage_test.cpp")
target_include_directories(netutil_http_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(netutil_http_tests PRIVATE netutil Catch2::Catch2WithMain)
add_test(NAME netutil_http_tests COMMAND netutil_http_tests)

# ---- netutil_logic_tests: Timer + SSL/UDP/TCP non-network & localhost methods ----
add_executable(netutil_logic_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/timer_coverage_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/network_stub_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/tcp_coverage_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/extra_coverage_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/ssl_echo_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/extra_coverage2_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/ssl_tcp_connect_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asio/extra_coverage3_test.cpp")
target_include_directories(netutil_logic_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(netutil_logic_tests PRIVATE -fno-access-control)
target_link_libraries(netutil_logic_tests PRIVATE netutil Catch2::Catch2WithMain)
add_test(NAME netutil_logic_tests COMMAND netutil_logic_tests)
