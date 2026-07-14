# infrastructure/sslconf 单元测试
# 纯 C++ 库（无 Qt 依赖），仅链接 GTest + sslconf + OpenSSL
find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
if(TEST_SOURCES)
    add_executable(sslconf_tests ${TEST_SOURCES} main.cpp)
    target_compile_options(sslconf_tests PRIVATE -fno-access-control -Wno-shadow)
    target_link_libraries(sslconf_tests PRIVATE GTest::GTest sslconf ${OPENSSL_LIBRARIES})
    target_include_directories(sslconf_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/infrastructure/sslconf/include
        ${CMAKE_SOURCE_DIR}/src/infrastructure/sslconf/src
        ${CMAKE_SOURCE_DIR}/tests/common
        ${OPENSSL_INCLUDE_DIR})
    add_test(NAME sslconf_tests COMMAND sslconf_tests)
endif()
