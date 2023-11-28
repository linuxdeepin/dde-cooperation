
set(CMAKE_CXX_STANDARD 17)

# protobuf.a 需要加“-fPIC”， 否则无法连接到.so
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
add_subdirectory(protobuf)

# 检查coost目录是否存在 https://github.com/idealvin/coost
#if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/coost")
#    message(STATUS "coost目录不存在, 从GitHub下载")
    
#    # 使用execute_process命令执行git clone命令来克隆代码
#    execute_process(
#        COMMAND git clone --depth 1 --branch v3.0.1 git@github.com:idealvin/coost.git
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    )
    
#endif()

# coost 打开SSL和动态库
# cmake .. -DWITH_LIBCURL=ON -DWITH_OPENSSL=ON -DBUILD_SHARED_LIBS=ON
set(WITH_OPENSSL ON CACHE BOOL "build with openssl")
set(BUILD_SHARED_LIBS ON CACHE BOOL "build shared lib")
add_subdirectory(coost)

add_subdirectory(zrpc)


if (CMAKE_SYSTEM MATCHES "Windows")
    if (MINGW)
        # minGW 编译运行依赖的openssl和mingw库
        file(GLOB RUN_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mingw-runlibs/*)
        # 复制所有文件到目标目录
        file(COPY ${RUN_DEPENDS}
          DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
        message("   >>> copy mingw runtime depends files:  ${RUN_DEPENDS}")
    else()
        # 从QtZlib获取到的zlib头文件
        file(GLOB ZLIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/zlib-header/*)
        # 复制所有文件到目标目录
        file(COPY ${ZLIB_HEADERS}
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/quazip/quazip/)
    endif()
    add_subdirectory(quazip)
endif()

message("   >> include barrier...")
# barrier v2.4.0 release, upstream: https://github.com/debauchee/barrier
if (UNIX)
    add_subdirectory(barrier)
else()
    # 拷贝编译好的版本windows版本
    file(GLOB BARRIER_BINS ${CMAKE_CURRENT_SOURCE_DIR}/barrier/bins/*)
    file(COPY ${BARRIER_BINS}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
    message("   >>> copy barrier binary and depends:  ${BARRIER_BINS}")
endif()
