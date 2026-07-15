# tests/transfer_helper - cooperation_transfer::TransferHelper 单元测试
# 直接编译源码绕过 hidden visibility (参考 coop/dt 模式)
set(CD ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core)

file(GLOB TRANSFER_SRC
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/singleton/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/settings/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/logger.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    "${CD}/discover/deviceinfo.cpp"
    "${CD}/discover/deviceinfo.h"
    "${CD}/global_defines.h"
    "${CMAKE_SOURCE_DIR}/src/lib/cooperation/transfer/helper/transferhelper.cpp"
    "${CMAKE_SOURCE_DIR}/src/lib/cooperation/transfer/helper/transferhelper.h"
    "${CMAKE_SOURCE_DIR}/src/lib/cooperation/transfer/helper/transferhelper_p.h"
)

find_package(GTest REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network DBus Test)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)

set(CMAKE_AUTOMOC ON)
add_executable(transfer_helper_tests ${TRANSFER_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferhelper_ct_test.cpp)
target_compile_options(transfer_helper_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_include_directories(transfer_helper_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/lib/cooperation/transfer
    ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core
    ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core/discover
    ${CMAKE_SOURCE_DIR}/3rdparty/include
    ${CMAKE_SOURCE_DIR}/src/infrastructure/slotipc/include)
target_link_libraries(transfer_helper_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::DBus Qt6::Test
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    logging sessionmanager sslconf basekit fmt proto httpweb session
    slotipc)
target_link_options(transfer_helper_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME transfer_helper_tests COMMAND transfer_helper_tests)
