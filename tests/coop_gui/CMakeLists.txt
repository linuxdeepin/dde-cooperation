# cooperation GUI 单元测试 (深黑盒: QTest + QSignalSpy + stub.h + addr_pri.h)
set(CD ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core)

# === glob cooperation-core GUI 源码 (复用 tests/coop 的依赖集) ===
file(GLOB COOP_GUI_SRC
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/singleton/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/settings/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/logger.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    "${CD}/cooperationcoreplugin.cpp"
    "${CD}/gui/*.cpp"
    "${CD}/gui/dialogs/*.cpp"
    "${CD}/gui/widgets/*.cpp"
    "${CD}/gui/utils/*.cpp"
    "${CD}/gui/phone/*.cpp"
    "${CD}/transfer/*.cpp"
    "${CD}/utils/*.cpp"
    "${CD}/discover/*.cpp"
    "${CD}/share/*.cpp"
    "${CD}/net/networkutil*.cpp"
    "${CD}/net/helper/*.cpp"
    "${CD}/net/helper/dialogs/*.cpp"
    "${CD}/net/compatwrapper*.cpp"
    "${CD}/net/transferwrapper*.cpp"
    "${CD}/net/linux/*.cpp"
    "${CD}/gui/linux/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/base/reportlog/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/base/reportlog/*/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/dconfig/*.cpp"
    "${CD}/*.h" "${CD}/*_p.h" "${CD}/gui/*.h" "${CD}/gui/*/*.h"
    "${CD}/discover/*.h" "${CD}/net/*.h" "${CD}/net/*_p.h" "${CD}/net/helper/*_p.h")

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Gui Network Concurrent Test DBus REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)
find_library(QRENCODE_LIBRARY qrencode)
find_library(VNCCLIENT_LIBRARY vncclient)

set(CMAKE_AUTOMOC ON)
add_executable(coop_gui_tests ${COOP_GUI_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coop_searchedit_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qt2keysum_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationguihelper_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sortfilterworker_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/devicelistwidget_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filechooseredit_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationdialog_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationtaskdialog_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceitem_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationstatewidget_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vncviewer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/workspacewidget_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settingitem_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/firsttipwidget_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/phonewidget_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settingdialog_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/discovercontroller_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backgroundwidget_test.cpp)
target_compile_options(coop_gui_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_compile_definitions(coop_gui_tests PRIVATE EXECUTABLE_PROG_DIR="${CMAKE_BINARY_DIR}" ENABLE_PHONE=1)
target_include_directories(coop_gui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CD}
    ${CMAKE_SOURCE_DIR}/src/lib/common
    ${CMAKE_SOURCE_DIR}/3rdparty/QtZeroConf
    ${CD}/gui/win
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(coop_gui_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Concurrent Qt6::Test Qt6::DBus
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    QtZeroConf slotipc
    logging sessionmanager sslconf basekit fmt proto httpweb session)
if(QRENCODE_LIBRARY)
    target_link_libraries(coop_gui_tests PRIVATE ${QRENCODE_LIBRARY})
endif()
if(VNCCLIENT_LIBRARY)
    target_link_libraries(coop_gui_tests PRIVATE ${VNCCLIENT_LIBRARY})
endif()
target_link_options(coop_gui_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME coop_gui_tests COMMAND coop_gui_tests)
