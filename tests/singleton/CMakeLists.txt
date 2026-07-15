# tests/singleton - SingleApplication 单元测试
# SingleApplication 析构函数会调用 _exit(0), 必须用独立二进制隔离。
# ENABLE_AUTO_UNIT_TEST 让析构时先 __gcov_dump() 再 _exit, 覆盖率正常落盘。
find_package(GTest REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Test)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)

set(CMAKE_AUTOMOC ON)
add_executable(singleton_tests
    ${CMAKE_SOURCE_DIR}/src/singleton/singleapplication.cpp
    ${CMAKE_SOURCE_DIR}/src/singleton/singleapplication.h
    ${CMAKE_CURRENT_SOURCE_DIR}/singleapplication_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_compile_options(singleton_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_include_directories(singleton_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/singleton)
target_link_libraries(singleton_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Test
    Dtk6::Core Dtk6::Gui Dtk6::Widget)
target_link_options(singleton_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME singleton_tests COMMAND singleton_tests)
