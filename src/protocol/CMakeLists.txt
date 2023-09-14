cmake_minimum_required(VERSION 3.11)

project(uniapi)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# if (WIN32)
# 	set(PROTOBUF_PROTOC_EXECUTABLE "${TINYRPC_DIR}/third_party/protobuf/protoc/win32/protoc.exe")
# else()
# 	if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
# 		set(PROTOBUF_PROTOC_EXECUTABLE "${TINYRPC_DIR}/third_party/protobuf/protoc/linux-aarch_64/protoc")
# 	else()
# 		if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
# 			set(PROTOBUF_PROTOC_EXECUTABLE "${TINYRPC_DIR}/third_party/protobuf/protoc/Darwin-x86_64/protoc")
# 		else()
# 			set(PROTOBUF_PROTOC_EXECUTABLE "${TINYRPC_DIR}/third_party/protobuf/protoc/linux-x86_64/protoc")
# 		endif()
# 	endif()
# endif()

find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_RPC_UNIAPIS_SRC PROTO_RPC_UNIAPIS_HDR message.proto)
# set_property(SOURCE ${PROTO_RPC_UNIAPIS_SRC} ${PROTO_RPC_UNIAPIS_HDR} PROPERTY SKIP_AUTOGEN ON)

add_library(${PROJECT_NAME} STATIC 
    ${PROTO_RPC_UNIAPIS_SRC}
    ${PROTO_RPC_UNIAPIS_SRC}
    ${PROTO_RPC_SERVICE_PTL_SRC}
    ${PROTO_RPC_SERVICE_PTL_HDRS}
)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_libraries(baseproto)

#Protobuf libraris
target_link_libraries(${PROJECT_NAME} protobuf::libprotobuf)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
