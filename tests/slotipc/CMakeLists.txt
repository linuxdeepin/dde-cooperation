# slotipc 直接编译测试 (参考 tests/dt 模式)
#
# 为什么直接 glob 源码进测试 exe (而不是链接 libslotipc.so):
#   现有 src/infrastructure/slotipc/test 有链接错误 + interface_test 段错误
#   (见记忆 test-infra-overview, 默认 SLOTIPC_BUILD_TESTS=OFF)。
#   把源码直接 glob 进测试 exe 绕过这些坑, -fno-access-control 允许访问私有成员。
set(SI ${CMAKE_SOURCE_DIR}/src/infrastructure/slotipc)

file(GLOB SLOTIPC_SRC
    "${SI}/src/service.cpp"
    "${SI}/src/interface.cpp"
    "${SI}/src/marshaller.cpp"
    "${SI}/src/serviceconnection.cpp"
    "${SI}/src/interfaceconnection.cpp"
    "${SI}/src/message.cpp"
    "${SI}/src/signalhandler.cpp"
    "${SI}/src/interfaceworker.cpp"
    # 头文件 (AUTOMOC 扫描 Q_OBJECT)
    "${SI}/include/slotipc/service.h"
    "${SI}/include/slotipc/interface.h"
    "${SI}/include/slotipc/argdefine.h"
    "${SI}/src/service_p.h"
    "${SI}/src/interface_p.h"
    "${SI}/src/marshaller_p.h"
    "${SI}/src/message_p.h"
    "${SI}/src/signalhandler_p.h"
    "${SI}/src/serviceconnection_p.h"
    "${SI}/src/interfaceconnection_p.h"
    "${SI}/src/interfaceworker.h")

find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Network Gui Core5Compat Test REQUIRED)

# 镜像原 CMakeLists 的手动 moc 处理 (AUTOMOC OFF, 因为 service.cpp/interface.cpp
# 末尾 #include "moc_*.cpp" 是 merge-moc 风格, 与 AUTOMOC 冲突)
set(CMAKE_AUTOMOC OFF)
set(moc_headers
    "${SI}/src/serviceconnection_p.h"
    "${SI}/src/interfaceconnection_p.h"
    "${SI}/src/interfaceworker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/testsubject.h")
QT6_WRAP_CPP(MOC_SRCS ${moc_headers})
QT6_GENERATE_MOC("${SI}/include/slotipc/service.h" "${CMAKE_CURRENT_BINARY_DIR}/moc_service.cpp")
QT6_GENERATE_MOC("${SI}/include/slotipc/interface.h" "${CMAKE_CURRENT_BINARY_DIR}/moc_interface.cpp")

add_executable(slotipc_tests ${SLOTIPC_SRC} ${MOC_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/marshaller_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/message_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/signalhandler_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaceworker_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/service_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_test.cpp)
set_source_files_properties("${SI}/src/service.cpp" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/moc_service.cpp")
set_source_files_properties("${SI}/src/interface.cpp" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/moc_interface.cpp")
target_compile_options(slotipc_tests PRIVATE -fno-access-control -fprofile-arcs -ftest-coverage)
target_compile_definitions(slotipc_tests PRIVATE QT_USE_QSTRINGBUILDER)
target_include_directories(slotipc_tests PRIVATE
    ${SI}/include ${SI}/src
    ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(slotipc_tests PRIVATE
    GTest::GTest
    Qt6::Core Qt6::Network Qt6::Gui Qt6::Core5Compat Qt6::Test)
target_link_options(slotipc_tests PRIVATE -fprofile-arcs -ftest-coverage)
add_test(NAME slotipc_tests COMMAND slotipc_tests)
