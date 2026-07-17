if(NOT TARGET compat)

if (NOT DEFINED DDE_COOPERATION_PLUGIN_ROOT_DIR)
    set(DDE_COOPERATION_PLUGIN_ROOT_DIR ${LIB_INSTALL_DIR}/dde-cooperation/plugins)
endif()

if (NOT DEFINED DEEPIN_DAEMON_PLUGIN_DIR)
    set(DEEPIN_DAEMON_PLUGIN_DIR ${DDE_COOPERATION_PLUGIN_ROOT_DIR}/daemon)
endif()

# build plugins dir
if(NOT DEFINED DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR)
    set(DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR ${CMAKE_CURRENT_BINARY_DIR}/plugins)
endif()

if (WIN32)
    set(DEPLOY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    message("   >>> DEPLOY_OUTPUT_DIRECTORY  ${DEPLOY_OUTPUT_DIRECTORY}")

    # windows runtime output defined
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})
else()
    # set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    # Disable the coroutine in coost.
    add_definitions(-D DISABLE_GO)
endif()

# The protoc tool not for all platforms, so use the generate out sources.
set(USE_PROTOBUF_FILES ON)
# 检查protobuf目录是否存在
if(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty/protobuf")
    set(PROTOBUF_DIR "${PROJECT_SOURCE_DIR}/3rdparty/protobuf")
    include_directories(${PROTOBUF_DIR}/src)

    if(MSVC)
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17")
    else()
        # protobuf.a 需要加"-fPIC"，否则无法连接到.so
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif()
    add_subdirectory("${PROTOBUF_DIR}" protobuf)
endif()


set(COOST_DIR "${PROJECT_SOURCE_DIR}/3rdparty/coost")
include_directories(${COOST_DIR}/include)

# coost 打开SSL和动态库
# cmake .. -DWITH_LIBCURL=ON -DWITH_OPENSSL=ON -DBUILD_SHARED_LIBS=ON
set(BUILD_SHARED_LIBS ON)

# 单元测试/覆盖率构建: 关闭 coost 的系统调用 hook。
# coost 通过符号介入 interpose 了 open/fcntl/close 等; 进程退出时 gcov 落盘
# (__gcov_exit -> __gcov_open) 会调用这些被 hook 的 syscall, 进而触发 co::gHook()
# 的惰性分配, 而此刻 coost 静态分配器正处于 teardown 之中 -> SIGSEGV, 并留下损坏
# 的 .gcda 导致 lcov 中途 abort、覆盖率大幅丢失。测试不依赖协程 socket hook,
# 关闭后这些调用直接走 libc, 从根上消除该崩溃。须在 add_subdirectory(coost) 之前
# 设置, 以覆盖 coost 自带 option(DISABLE_HOOK ... OFF) 的默认值。
if(DOTEST OR BUILD_TESTS)
    # 必须用 CACHE+FORCE: coost 子目录自带 cmake_minimum_required(VERSION 3.12),
    # 进入子作用域后 CMake policy CMP0077 退化为 OLD/WARN, option(DISABLE_HOOK OFF)
    # 会无视父级 set() 设置的 normal 变量, 直接写 cache 为 OFF, 导致
    # _CO_DISABLE_HOOK 编译宏不会生效。用 FORCE 强制写 cache 可确保子目录的
    # option() 看到已存在的 cache 值而不覆盖, 从而让 if(DISABLE_HOOK) 为真。
    set(DISABLE_HOOK ON CACHE BOOL "disable hooks for system APIs" FORCE)
endif()
add_subdirectory("${COOST_DIR}" coost)

# 单元测试构建：禁用 coost 在静态初始化阶段预热 hook（详见 log.cc 注释）。
# 仅影响 init_hook 的早调用时机，不改业务行为。
if(DOTEST OR BUILD_TESTS)
    if(TARGET co)
        target_compile_definitions(co PRIVATE CO_DEFER_HOOK_AT_MOD_INIT)
    endif()
endif()



endif()
