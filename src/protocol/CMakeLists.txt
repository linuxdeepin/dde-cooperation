cmake_minimum_required(VERSION 3.13)

project(uniapi)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# find_package(Protobuf REQUIRED)
include(protobuf-generate)

# do not find, check system installed protobuf package
if(NOT EXISTS "${PROTOBUF_PROTOC_EXECUTABLE}")
    find_package(Protobuf REQUIRED)
    if (${PROTOBUF_FOUND})
        message("protobuf found")
    else ()
        message(FATAL_ERROR "Cannot find Protobuf")
    endif ()

    include_directories(${Protobuf_INCLUDE_DIRS})

# 生成转换proto文件
# protoc --cpp_out=./ message.proto
    set(PROTO_FILES ${PROJECT_SOURCE_DIR}/message.pb.cc)
    set(PROTOBUF_LIB
        ${Protobuf_LIBRARY}
    )

# use the local customize protobuf package
else ()
    include_directories(${PROTOBUF_DIR}/src)
    PROTOBUF_GENERATE_CPP(PROTO_RPC_UNIAPIS_SRC PROTO_RPC_UNIAPIS_HDR message.proto)
    # set_property(SOURCE ${PROTO_RPC_UNIAPIS_SRC} ${PROTO_RPC_UNIAPIS_HDR} PROPERTY SKIP_AUTOGEN ON)
    set(PROTO_FILES
        ${PROTO_RPC_UNIAPIS_SRC}
        ${PROTO_RPC_UNIAPIS_HDR}
    )
    set(PROTOBUF_LIB
        protobuf::libprotobuf
    )
endif()

add_library(${PROJECT_NAME} STATIC 
    ${PROTO_FILES}
)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

#Protobuf libraris
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIB})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
