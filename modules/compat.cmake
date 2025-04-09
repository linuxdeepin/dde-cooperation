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
add_subdirectory("${COOST_DIR}" coost)



endif()
