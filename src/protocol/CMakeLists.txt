cmake_minimum_required(VERSION 3.13)

project(uniapi)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# do not find, check system installed protobuf package
if(NOT EXISTS "${PROTOBUF_PROTOC_EXECUTABLE}")
    # 生成转换proto文件
    # protoc --cpp_out=./ message.proto
    set(PROTO_FILES ${PROJECT_SOURCE_DIR}/message.pb.cc)

# use the local customize protobuf package
else ()
    PROTOBUF_GENERATE_CPP(PROTO_RPC_UNIAPIS_SRC PROTO_RPC_UNIAPIS_HDR message.proto)
    # set_property(SOURCE ${PROTO_RPC_UNIAPIS_SRC} ${PROTO_RPC_UNIAPIS_HDR} PROPERTY SKIP_AUTOGEN ON)
    set(PROTO_FILES
        ${PROTO_RPC_UNIAPIS_SRC}
        ${PROTO_RPC_UNIAPIS_HDR}
    )
endif()

add_library(${PROJECT_NAME} SHARED 
    ${PROTO_FILES}
)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

#zrpc library which has include protobuf library
target_link_libraries(${PROJECT_NAME} zrpc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
