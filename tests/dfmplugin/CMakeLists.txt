# dfmplugin 单元测试 (直接编译 src/lib/cooperation/dfmplugin 源码进测试 exe)
#
# 为什么独立 target:
#   dfmplugin 下的 configs/dconfig, configs/settings, reportlog 是 src/configs、
#   src/base 同名类的 vendored 副本 (同 include guard 同类名)。coop/coop_gui/dt 已
#   glob 了 src/ 版本, 若在同一 exe 再编 dfmplugin 版本会 ODR/符号冲突。故单独建 exe。
#
# 为什么直接编源码而不是链接 .so:
#   dfmplugin-cooperation 以 hidden visibility 构建, 外部链接拿不到符号 (同 coop 模式)。
#
# 依赖: dfm6-framework (dpf::Plugin / dpf 事件), dfm6-base (AbstractMenuScene /
#   SettingJsonGenerator), Dtk6 (DConfig / DSysInfo / DDialog 等)。

set(DFM ${CMAKE_SOURCE_DIR}/src/lib/cooperation/dfmplugin)

# === dfmplugin 全部业务源码 (排除 .json 资源与翻译) ===
file(GLOB DFPLUGIN_SRC
    "${DFM}/*.cpp"
    "${DFM}/*.h"
    "${DFM}/configs/*/*.cpp"
    "${DFM}/configs/*/*.h"
    "${DFM}/dialogs/*.cpp"
    "${DFM}/dialogs/*.h"
    "${DFM}/menu/*.cpp"
    "${DFM}/menu/*.h"
    "${DFM}/reportlog/*.cpp"
    "${DFM}/reportlog/*.h"
    "${DFM}/reportlog/*/*.cpp"
    "${DFM}/reportlog/*/*.h"
    "${DFM}/utils/*.cpp"
    "${DFM}/utils/*.h")

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Gui Network Test REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)
find_package(dfm6-framework REQUIRED)
find_package(dfm6-base REQUIRED)

set(CMAKE_AUTOMOC ON)
add_executable(dfmplugin_tests
    ${DFPLUGIN_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dconfigmanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/configmanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reportlog_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filetransfersettingsdialog_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationhelper_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationmenuscene_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cooperationplugin_test.cpp)
target_compile_options(dfmplugin_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_include_directories(dfmplugin_tests PRIVATE
    ${DFM}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dfmplugin_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Test
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    dfm6-framework
    dfm6-base)
target_link_options(dfmplugin_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME dfmplugin_tests COMMAND dfmplugin_tests)
