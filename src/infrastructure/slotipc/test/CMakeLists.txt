cmake_minimum_required(VERSION 3.13)

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS Test)

# 查找Google Test
find_package(GTest REQUIRED)
include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${Qt${QT_DESIRED_VERSION}Core_INCLUDE_DIRS}
    ${Qt${QT_DESIRED_VERSION}Gui_INCLUDE_DIRS}
    ${Qt${QT_DESIRED_VERSION}Test_INCLUDE_DIRS}
    ${Qt${QT_DESIRED_VERSION}Network_INCLUDE_DIRS}
)

# 测试源文件列表
set(TEST_SOURCES
    service_test.cpp
    interface_test.cpp
    marshaller_test.cpp
    serviceconnection_test.cpp
    interfaceconnection_test.cpp
    message_test.cpp
    signalhandler_test.cpp
    interfaceworker_test.cpp
)

# 为每个测试源文件创建可执行文件
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_compile_definitions(${test_name} PRIVATE QT_USE_QSTRINGBUILDER)
    target_link_libraries(${test_name}
        ${PROJECT_NAME}
        GTest::GTest
        GTest::Main
        Qt${QT_DESIRED_VERSION}::Core
        Qt${QT_DESIRED_VERSION}::Network
        Qt${QT_DESIRED_VERSION}::Gui
        Qt${QT_DESIRED_VERSION}::Test
        slotipc
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()