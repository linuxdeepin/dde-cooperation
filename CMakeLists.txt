cmake_minimum_required(VERSION 3.13)

project(dde-cooperation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(PROTOBUF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/protobuf")
include_directories(${PROTOBUF_DIR}/src)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${PROJECT_SOURCE_DIR}/ui )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# define and use the local 3rdparty protobuf library
if (WIN32)
	set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/win32/protoc.exe")
else()
	if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
		set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/linux-aarch_64/protoc")
	else()
		if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
			set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/Darwin-x86_64/protoc")
		else()
			set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/linux-x86_64/protoc")
		endif()
	endif()
endif()

set(COOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/coost")
include_directories(${COOST_DIR}/include)

set(ZRPC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zrpc")
include_directories(${ZRPC_DIR}/include)

# build 3rd party libraries first

#add_subdirectory(3rdparty)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#include(protobuf-generate)


include_directories(include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIE")

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-need")

    #安全加固
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")
endif()

#安全加固
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -z relro")
    add_definitions(-D ARCH_MIPSEL)
endif()

include(GNUInstallDirs)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
endif()

if (NOT DEFINED DDE_COOPERATION_PLUGIN_ROOT_DIR)
    set(DDE_COOPERATION_PLUGIN_ROOT_DIR ${LIB_INSTALL_DIR}/dde-cooperation/plugins)
endif()

if (NOT DEFINED DEEPIN_DAEMON_PLUGIN_DIR)
    set(DEEPIN_DAEMON_PLUGIN_DIR ${DDE_COOPERATION_PLUGIN_ROOT_DIR}/daemon)
endif()

if (NOT DEFINED DEEPIN_DATA_TRANS_PLUGIN_DIR)
    set(DEEPIN_DATA_TRANS_PLUGIN_DIR ${DDE_COOPERATION_PLUGIN_ROOT_DIR}/data-transfer)
endif()

if (NOT DEFINED DEEPIN_COOPERATION_PLUGIN_DIR)
    set(DEEPIN_COOPERATION_PLUGIN_DIR ${DDE_COOPERATION_PLUGIN_ROOT_DIR}/cooperation)
endif()

# build plugins dir
if(NOT DEFINED DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR)
    set(DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/plugins)
endif()

message("   >>> LIB_INSTALL_DIR  ${LIB_INSTALL_DIR}")
message("   >>> DDE_COOPERATION_PLUGIN_ROOT_DIR  ${DDE_COOPERATION_PLUGIN_ROOT_DIR}")
message("   >>> DEEPIN_DATA_TRANS_PLUGIN_DIR  ${DEEPIN_DATA_TRANS_PLUGIN_DIR}")
message("   >>> DEEPIN_CROSS_PLATFORM_COLLABORATION_PLUGIN_DIR  ${DEEPIN_CROSS_PLATFORM_COLLABORATION_PLUGIN_DIR}")
message("   >>> DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR  ${DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR}")
message("   >>> COMPILE ON SYSTEM: ${CMAKE_SYSTEM}")

set(PROTOBUF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/protobuf")
include_directories(${PROTOBUF_DIR}/src)

# set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${PROJECT_SOURCE_DIR}/ui )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# define and use the local 3rdparty protobuf library
if (WIN32)
	set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/win32/protoc.exe")
else()
	if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
		set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/linux-aarch_64/protoc")
	else()
		if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
			set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/Darwin-x86_64/protoc")
		else()
			set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOBUF_DIR}/protoc/linux-x86_64/protoc")
		endif()
	endif()
endif()

include(protobuf-generate)

set(COOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/coost")
include_directories(${COOST_DIR}/include)

set(ZRPC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zrpc")
include_directories(${ZRPC_DIR}/include)

# build 3rd party libraries first
add_subdirectory(3rdparty)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)
