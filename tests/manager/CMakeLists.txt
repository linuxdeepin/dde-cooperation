# lib/common/manager 单元测试
# 链接 sessionmanager 静态库 (内含 SessionManager/SessionWorker/TransferWorker/FileSizeCounter)
find_package(GTest REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Test)

set(CMAKE_AUTOMOC ON)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
if(TEST_SOURCES)
    add_executable(manager_tests ${TEST_SOURCES} main.cpp)
    target_compile_options(manager_tests PRIVATE -fno-access-control -Wno-shadow)
    target_link_libraries(manager_tests PRIVATE
        GTest::GTest
        Qt6::Core Qt6::Network Qt6::Widgets Qt6::Test
        sessionmanager session httpweb sslconf logging)
    target_include_directories(manager_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/lib/common
        ${CMAKE_SOURCE_DIR}/src/lib/common/manager
        ${CMAKE_SOURCE_DIR}/src/lib/common/session
        ${CMAKE_SOURCE_DIR}/src/lib/common/httpweb
        ${CMAKE_SOURCE_DIR}/tests/common
        ${CMAKE_SOURCE_DIR}/3rdparty/include)
    add_test(NAME manager_tests COMMAND manager_tests)
endif()
