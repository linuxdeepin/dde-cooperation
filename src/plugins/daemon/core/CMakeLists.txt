cmake_minimum_required(VERSION 3.13)

project(daemon-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    # "${CMAKE_SOURCE_DIR}/src/common/rpcwraper.h"
    # "${CMAKE_SOURCE_DIR}/src/common/rpcwraper.cpp"
    # "${CMAKE_SOURCE_DIR}/src/common/ipcwraper.h"
    # "${CMAKE_SOURCE_DIR}/src/common/ipcwraper.cpp"
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

find_package(Threads REQUIRED)

include_directories(include)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/protocol)

add_library(${PROJECT_NAME}
    SHARED
    ${PLUGIN_FILES}
    ${QRC_RES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
)


target_link_libraries(${PROJECT_NAME}
    deepin_cross::framework
    uniapi
    co
)

#System libraries
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
endif ()
target_link_libraries(${PROJECT_NAME} Threads::Threads)

#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_DAEMON_PLUGIN_DIR}
)

