# lib 逻辑模块单元测试骨架。依赖全部可选，缺失则跳过，不阻断 configure。
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Network QUIET)
find_package(Dtk6Widget QUIET)
find_package(Protobuf QUIET)
find_package(ZLIB QUIET)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
if(TEST_SOURCES)
    add_executable(lib_logic_tests ${TEST_SOURCES} main.cpp)
    target_compile_options(lib_logic_tests PRIVATE -fno-access-control)
    target_link_libraries(lib_logic_tests PRIVATE GTest::GTest proto httpweb session cooperation-core sslconf netutil)
    if(TARGET Dtk6::Widget OR TARGET Dtk6Widget::Dtk6Widget)
        target_link_libraries(lib_logic_tests PRIVATE Dtk6::Widget)
        if(TARGET Dtk6Widget::Dtk6Widget)
            target_link_libraries(lib_logic_tests PRIVATE Dtk6Widget::Dtk6Widget)
        endif()
    endif()
    if(TARGET Qt6::Core)
        target_link_libraries(lib_logic_tests PRIVATE Qt6::Core Qt6::Widgets)
        if(TARGET Qt6::Network)
            target_link_libraries(lib_logic_tests PRIVATE Qt6::Network)
        endif()
    endif()
    if(Protobuf_FOUND AND TARGET protobuf::libprotobuf)
        target_link_libraries(lib_logic_tests PRIVATE protobuf::libprotobuf)
    endif()
    if(ZLIB_FOUND AND TARGET ZLIB::ZLIB)
        target_link_libraries(lib_logic_tests PRIVATE ZLIB::ZLIB)
    endif()
    target_include_directories(lib_logic_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/lib/common
        ${CMAKE_SOURCE_DIR}/src/lib/common/proto
        ${CMAKE_SOURCE_DIR}/src/lib/common/httpweb
        ${CMAKE_SOURCE_DIR}/src/lib/common/session
        ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core
        ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core/net
        ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core/net/helper
        ${CMAKE_SOURCE_DIR}/src/lib/cooperation/core/discover
        ${CMAKE_SOURCE_DIR}/src/lib/cooperation/dfmplugin
        ${CMAKE_SOURCE_DIR}/src/lib/data-transfer/quazip
        ${CMAKE_SOURCE_DIR}/3rdparty/QtZeroConf)
    add_test(NAME lib_logic_tests COMMAND lib_logic_tests)
endif()


