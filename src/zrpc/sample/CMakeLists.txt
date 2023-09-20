cmake_minimum_required(VERSION 3.0)
project(rpctest)

SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
enable_language(ASM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable")

find_package(Protobuf REQUIRED)
if (${PROTOBUF_FOUND})
    message("protobuf found")
else ()
    message(FATAL_ERROR "Cannot find Protobuf")
endif ()

# 生成转换proto文件
# protoc --cpp_out=./ sample.proto

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(PATH_BIN bin)
set(LIBS
    zrpc
    ${Protobuf_LIBRARY}
    # dl
    # pthread
    co
)

# server
set(
    server_src
    ${PROJECT_SOURCE_DIR}/server.cpp
    ${PROJECT_SOURCE_DIR}/sample.pb.cc
)
add_executable(server ${server_src})
target_link_libraries(server ${LIBS})
install(TARGETS server DESTINATION ${PATH_BIN})

# client
set(
    client_src
    ${PROJECT_SOURCE_DIR}/client.cpp
    ${PROJECT_SOURCE_DIR}/sample.pb.cc
)
add_executable(client ${client_src})
target_link_libraries(client ${LIBS})
install(TARGETS client DESTINATION ${PATH_BIN})