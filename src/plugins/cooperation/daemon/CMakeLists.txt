cmake_minimum_required(VERSION 3.13)

project(daemon-cooperation)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 插件相关的固定内容，或者公共头文件源文件，添加到这里。
FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.h"
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.h"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/configs/settings/*.h"
    "${CMAKE_SOURCE_DIR}/src/configs/settings/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    )

if (CMAKE_SYSTEM MATCHES "Linux")
    FILE(GLOB BUSS_FILES
        "${CMAKE_SOURCE_DIR}/src/configs/dconfig/*.h"
        "${CMAKE_SOURCE_DIR}/src/configs/dconfig/*.cpp"
    )
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui DBus Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui DBus Network)
find_package(Dtk COMPONENTS Core REQUIRED)

add_library(${PROJECT_NAME}
    SHARED
    ${PLUGIN_FILES}
    ${BUSS_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
)

# 公共依赖库，添加到这里
target_link_libraries(${PROJECT_NAME}
    deepin_cross::framework
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Network
    ${DtkCore_LIBRARIES}
    co
)

#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_DAEMON_PLUGIN_DIR}
)

include(${CMAKE_SOURCE_DIR}/install_dconfig.cmake)
INSTALL_DCONFIG("org.deepin.dde.cooperation.json")
