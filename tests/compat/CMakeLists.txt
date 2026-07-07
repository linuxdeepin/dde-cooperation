# compat 模块单元测试 (zrpc + framework/event)
# 仅当对应 target 存在时添加测试，缺失依赖不阻断 configure。
find_package(GTest REQUIRED)
find_package(Qt6 COMPONENTS Core Concurrent QUIET)

# ---- zrpc_tests: TcpBuffer / NetAddress / ZRpcCodeC / ZRpcController ----
if(TARGET zrpc)
    add_executable(zrpc_tests zrpc_test.cpp)
    target_link_libraries(zrpc_tests PRIVATE
        GTest::GTest
        GTest::Main
        zrpc
    )
    target_include_directories(zrpc_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/compat/zrpc/include
        ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src
        ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/include
        ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/net
        ${CMAKE_SOURCE_DIR}/src/compat/zrpc/src/spec
        # 传递性头文件：zrpc 头依赖 protobuf 与 coost
        ${CMAKE_SOURCE_DIR}/3rdparty/protobuf/src
        ${CMAKE_SOURCE_DIR}/3rdparty/coost/include
    )
    add_test(NAME zrpc_tests COMMAND zrpc_tests)
endif()

# ---- framework_tests: Event / EventChannel / EventSequence / EventDispatcher ----
if(TARGET dde-cooperation-framework)
    add_executable(framework_tests event_test.cpp lifecycle_test.cpp)
    target_link_libraries(framework_tests PRIVATE
        GTest::GTest
        GTest::Main
        dde-cooperation-framework
    )
    if(TARGET Qt6::Core)
        target_link_libraries(framework_tests PRIVATE Qt6::Core)
    endif()
    if(TARGET Qt6::Concurrent)
        target_link_libraries(framework_tests PRIVATE Qt6::Concurrent)
    endif()
    add_test(NAME framework_tests COMMAND framework_tests)
endif()
