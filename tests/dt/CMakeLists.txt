# data-transfer-core 直接编译测试 (参考 tests/coop 模式)
#
# 为什么直接 glob 源码进测试 exe (而不是链接 data-transfer-core.so):
#   data-transfer-core 以 hidden visibility 构建, 且类无 Q_DECL_EXPORT 导出宏,
#   外部测试二进制链接时所有业务符号 undefined (见记忆 data-transfer-libs-hidden-visibility)。
#   把源码直接编进测试 exe 可绕过可见性限制, -fno-access-control 允许访问私有成员。
#
# NetworkUtil 斩链: networkutil.cpp 拉 sessionmanager/picojson 真网络栈,
#   用本地 networkutil_stub.cpp 替代, 提供 NetworkUtil 空实现以满足 transferhepler.cpp
#   等对 NetworkUtil::instance() 的链接引用, 不引入网络依赖。
set(CD ${CMAKE_SOURCE_DIR}/src/lib/data-transfer/core)

# === 业务源码 glob (Linux 分支) ===
file(GLOB DT_SRC
    # 公共依赖 (与 data-transfer-core PLUGIN_FILES 对齐)
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/logger.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    # utils
    "${CD}/utils/optionsmanager.cpp"
    "${CD}/utils/transferutil.cpp"
    "${CD}/utils/settinghepler.cpp"
    # net/helper (不含 networkutil.cpp, 用 stub 替代)
    "${CD}/net/helper/transferhepler.cpp"
    # gui (仅 Linux 分支: type_defines/mainwindow/connect/transfer/backupload/linux)
    "${CD}/gui/type_defines.cpp"
    "${CD}/gui/*.cpp"
    "${CD}/gui/connect/*.cpp"
    "${CD}/gui/transfer/*.cpp"
    "${CD}/gui/backupload/*.cpp"
    "${CD}/gui/linux/*.cpp"
    # 头文件 (AUTOMOC 扫描) — 严格对齐 Linux 源码, 排除 win/select/getbackup 头
    "${CD}/gui/*.h" "${CD}/gui/*_p.h"
    "${CD}/gui/connect/*.h" "${CD}/gui/transfer/*.h"
    "${CD}/gui/backupload/*.h" "${CD}/gui/linux/*.h"
    "${CD}/utils/*.h" "${CD}/net/*.h" "${CD}/net/*_p.h" "${CD}/net/helper/*.h"
    "${CD}/net/networkutil.h"
)
# 排除 compat (引用 slotipc) 与 networkutil 真实现 (用 stub 替代)
list(REMOVE_ITEM DT_SRC
    "${CD}/net/compatwrapper.cpp"
    "${CD}/net/compatwrapper.h"
    "${CD}/net/compatwrapper_p.h"
    "${CD}/net/compatstruct.h"
    "${CD}/net/networkutil.cpp"
    "${CD}/net/networkutill_p.h")

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Gui Network DBus Svg Test REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui Widget REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)

set(CMAKE_AUTOMOC ON)
add_executable(dt_tests ${DT_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/networkutil_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/type_defines_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferutil_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optionsmanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settinghepler_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferhepler_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upload_test.cpp)
target_compile_options(dt_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_compile_definitions(dt_tests PRIVATE
    EXECUTABLE_PROG_DIR="${CMAKE_BINARY_DIR}"
    APP_VERSION_MAJOR=1 APP_VERSION_MINOR=2 APP_VERSION_PATCH=6 APP_VERSION="1.2.6")
target_include_directories(dt_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CD}
    ${CMAKE_SOURCE_DIR}/3rdparty/include)
target_link_libraries(dt_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::DBus Qt6::Svg Qt6::Test
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    PkgConfig::libzip
    logging)
target_link_options(dt_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME dt_tests COMMAND dt_tests)
