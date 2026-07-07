# cooperation-core 直接编译测试 (参考 deepin-system-monitor 模式)
# 把 cooperation-core 源码直接编进测试 exe, 绕过隐藏可见性;
# -fno-access-control 允许测试访问私有成员。
# 照搬 cooperation-core 的精确 glob + 定义 + 依赖。
set(CD ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core)

# === 完全复制 cooperation-core 的源码 glob (PLUGIN + COMPAT + Linux BUSS) ===
file(GLOB COOP_SRC
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/singleton/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/settings/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/logger.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    "${CD}/cooperationcoreplugin.cpp"
    "${CD}/gui/*.cpp"
    "${CD}/transfer/*.cpp"
    "${CD}/cooperation/*.cpp"
    "${CD}/gui/dialogs/*.cpp"
    "${CD}/gui/widgets/*.cpp"
    "${CD}/gui/utils/*.cpp"
    "${CD}/utils/*.cpp"
    "${CD}/discover/*.cpp"
    "${CD}/share/*.cpp"
    "${CD}/net/networkutil*.cpp"
    "${CD}/net/helper/sharehelper*.cpp"
    "${CD}/net/helper/transferhelper*.cpp"
    "${CD}/net/helper/dialogs/*.cpp"
    "${CD}/net/compatwrapper*.cpp"
    "${CD}/net/transferwrapper*.cpp"
    # Linux BUSS_FILES:
    "${CMAKE_SOURCE_DIR}/src/base/reportlog/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/base/reportlog/*/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/dconfig/*.cpp"
    "${CD}/gui/linux/*.cpp"
    "${CD}/gui/phone/*.cpp"
    "${CD}/net/linux/*.cpp"
    "${CD}/net/helper/phonehelper*.cpp"
    "${CD}/net/*.h" "${CD}/net/*_p.h" "${CD}/net/helper/*_p.h"
    "${CD}/discover/*.h" "${CD}/discover/*_p.h"
    "${CD}/gui/*.h" "${CD}/gui/*_p.h" "${CD}/gui/*/*.h" "${CD}/*_p.h")

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Gui Network Concurrent DBus Test REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)
find_library(QRENCODE_LIBRARY qrencode)
find_library(VNCCLIENT_LIBRARY vncclient)

set(CMAKE_AUTOMOC ON)
add_executable(coop_tests ${COOP_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/discover_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helper_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coop_searchedit_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooconfig_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/historymanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationutil_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sharehelper_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferhelper_core_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compatwrapper_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferwrapper_test.cpp)
target_compile_options(coop_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_compile_definitions(coop_tests PRIVATE EXECUTABLE_PROG_DIR="${CMAKE_BINARY_DIR}" ENABLE_PHONE=1)
target_include_directories(coop_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CD}
    ${CMAKE_SOURCE_DIR}/src/lib/common
    ${CMAKE_SOURCE_DIR}/3rdparty/QtZeroConf
    ${CD}/gui/win)
target_link_libraries(coop_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Concurrent Qt6::DBus Qt6::Test
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    QtZeroConf slotipc
    logging sessionmanager sslconf basekit fmt proto httpweb session)
if(QRENCODE_LIBRARY)
    target_link_libraries(coop_tests PRIVATE ${QRENCODE_LIBRARY})
endif()
if(VNCCLIENT_LIBRARY)
    target_link_libraries(coop_tests PRIVATE ${VNCCLIENT_LIBRARY})
endif()
target_link_options(coop_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME coop_tests COMMAND coop_tests)
