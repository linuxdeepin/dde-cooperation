# daemon 模块单元测试 (compat/plugins/daemon/core)
# 直接编译源码到测试 exe, 绕过隐藏可见性, -fno-access-control 访问私有成员.
set(CD ${CMAKE_SOURCE_DIR}/src/compat/plugins/daemon/core)
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/src/compat/common)

# daemon-core 源码 glob (照搬 core/CMakeLists.txt 的 glob)
file(GLOB DAEMON_SRC
    "${COMMON_DIR}/commonutils.h"
    "${COMMON_DIR}/commonutils.cpp"
    "${CD}/daemoncoreplugin.cpp"
    "${CD}/service/*.cpp"
    "${CD}/service/*/*.cpp"
)

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Network Concurrent Test REQUIRED)

set(CMAKE_AUTOMOC ON)
add_executable(daemon_tests ${DAEMON_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/comshare_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fsadapter_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/jobmanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlesendresult_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sendipc_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/discoveryjob_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handleipc_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlerpc_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sendrpc_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/remoteservice_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transferjob_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/searchlight_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/servicemanager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/daemonplugin_test.cpp
)
target_compile_options(daemon_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_compile_definitions(daemon_tests PRIVATE DOTEST DISABLE_GO)

target_include_directories(daemon_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/compat
    ${CMAKE_SOURCE_DIR}/src/compat/protocol
    ${CMAKE_SOURCE_DIR}/src/compat/ipc/proto
    ${CMAKE_SOURCE_DIR}/src/compat/common
    ${CD}
    ${CD}/service
    # coost
    ${CMAKE_SOURCE_DIR}/3rdparty/coost/include
    # protobuf
    ${CMAKE_SOURCE_DIR}/3rdparty/protobuf/src
    # zrpc
    ${CMAKE_SOURCE_DIR}/src/compat/zrpc/include
    ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src
    ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/include
    ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/net
    ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/spec
    # logging (for commonutils.cpp -> co/log -> logging/config.h)
    ${CMAKE_SOURCE_DIR}/src/infrastructure/logging/include
    # src last to avoid shadowing compat/common/constant.h with src/common/constant.h
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(daemon_tests PRIVATE
    GTest::GTest
    GTest::Main
    Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Test
    dde-cooperation-framework
    slotipc
    zrpc
    sslconf
    co
    uniapi
    proto
)

target_link_options(daemon_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME daemon_tests COMMAND daemon_tests)
