if(NOT TARGET compat)



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
#    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})
#    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})

    # file(GLOB INNO_SCRIPTS ${CMAKE_SOURCE_DIR}/dist/inno/scripts)
    # file(COPY ${INNO_SCRIPTS} DESTINATION ${CMAKE_BINARY_DIR})
else()
    # Disable the coroutine in coost.
    add_definitions(-D DISABLE_GO)
endif()

# The protoc tool not for all platforms, so use the generate out sources.
set(USE_PROTOBUF_FILES ON)
set(PROTOBUF_DIR "${PROJECT_SOURCE_DIR}/3rdparty/protobuf")
include_directories(${PROTOBUF_DIR}/src)

# protobuf.a 需要加“-fPIC”， 否则无法连接到.so
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
add_subdirectory("${PROTOBUF_DIR}" protobuf)


set(COOST_DIR "${PROJECT_SOURCE_DIR}/3rdparty/coost")
include_directories(${COOST_DIR}/include)

# coost 打开SSL和动态库
# cmake .. -DWITH_LIBCURL=ON -DWITH_OPENSSL=ON -DBUILD_SHARED_LIBS=ON
set(WITH_OPENSSL ON CACHE BOOL "build with openssl")
set(BUILD_SHARED_LIBS ON CACHE BOOL "build shared lib")
set(BUILD_WITH_SYSTEMD ON CACHE BOOL "Build with systemd")
add_subdirectory("${COOST_DIR}" coost)


set(ZRPC_DIR "${PROJECT_SOURCE_DIR}/3rdparty/zrpc")
include_directories(${ZRPC_DIR}/include)
add_subdirectory("${ZRPC_DIR}" zrpc)

set(CUTEIPC_DIR "${PROJECT_SOURCE_DIR}/3rdparty/CuteIPC")
include_directories(${CUTEIPC_DIR}/include)
add_subdirectory("${CUTEIPC_DIR}" CuteIPC)

endif()
