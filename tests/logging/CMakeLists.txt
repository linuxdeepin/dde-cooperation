# logging appenders 单元测试 (gtest, 链静态 logging+basekit)
# 关键: AppendRecord 跳过 record.raw 为空的记录, 测试必须填 raw 字段。
set(LG ${CMAKE_SOURCE_DIR}/src/infrastructure/logging)

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Threads REQUIRED)

add_executable(logging_appender_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rolling_appender_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/misc_appender_test.cpp)
target_compile_options(logging_appender_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_include_directories(logging_appender_tests PRIVATE
    ${LG}/include
    ${CMAKE_SOURCE_DIR}/src/infrastructure/basekit/include)
target_link_libraries(logging_appender_tests PRIVATE
    GTest::GTest
    logging basekit
    Threads::Threads
    Qt6::Core)
target_link_options(logging_appender_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME logging_appender_tests COMMAND logging_appender_tests)
