cmake_minimum_required(VERSION 3.10)

project(cooperation-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/rpcwraper.h"
    "${CMAKE_SOURCE_DIR}/src/common/rpcwraper.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/ipcwraper.h"
    "${CMAKE_SOURCE_DIR}/src/common/ipcwraper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    )

#find_package(PkgConfig REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
# find_package(Boost 1.67 COMPONENTS system coroutine thread REQUIRED)
find_package(Boost COMPONENTS system coroutine thread REQUIRED)

# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_USE_STATIC_RUNTIME ON)

# find_package(Boost 1.55 COMPONENTS thread system filesystem program_options random atomic chrono)
# if (NOT Boost_FOUND)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost)
# endif()

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

include_directories(include)
# include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})
# include_directories(${CMAKE_CURRENT_BINARY_DIR}/third_party/tinyrpc/baseproto)
# include_directories(${CMAKE_SOURCE_DIR}/3rdparty/tinyrpc/include)
message("Boost: ${Boost_INCLUDE_DIRS}")

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/protocol)

link_libraries(uniapi)
link_libraries(co)

add_library(${PROJECT_NAME}
    SHARED
    ${PLUGIN_FILES}
    ${QRC_RES}
    # ${PROTO_RPC_UNIAPIS_SRC}
    # ${PROTO_RPC_UNIAPIS_HDR}
    # ${PROTO_RPC_SERVICE_PTL_SRC}
    # ${PROTO_RPC_SERVICE_PTL_HDRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
)

target_link_libraries(${PROJECT_NAME}
    deepin_cross::framework
)

#Qt libraries
# target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets)

#Boost libraries
target_link_libraries(${PROJECT_NAME} Boost::system Boost::coroutine Boost::thread)

#Protobuf libraris
# target_link_libraries(${PROJECT_NAME} protobuf::libprotobuf)

#System libraries
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
endif ()
target_link_libraries(${PROJECT_NAME} Threads::Threads)
target_link_libraries(${PROJECT_NAME} baseproto)

#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_DATA_TRANS_PLUGIN_DIR}
)

