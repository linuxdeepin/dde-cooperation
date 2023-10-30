cmake_minimum_required(VERSION 3.13)

project(cooperation-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 插件相关的固定内容，或者公共头文件源文件，添加到这里。
FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.h"
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.cpp"
    "${CMAKE_SOURCE_DIR}/src/config/*.h"
    "${CMAKE_SOURCE_DIR}/src/config/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cooperationcoreplugin.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/cooperationcoreplugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_defines.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/dialogs/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/dialogs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/widgets/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/widgets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/events/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/maincontroller/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/maincontroller/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    )

# 在这里添加业务代码
if (CMAKE_SYSTEM MATCHES "Windows")
    FILE(GLOB BUSS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/win/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/win/*.cpp"
    )
   find_package(Qt5 COMPONENTS Sql REQUIRED)
elseif (CMAKE_SYSTEM MATCHES "Linux")
    FILE(GLOB BUSS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.cpp"
    )
    find_package(Dtk COMPONENTS Widget REQUIRED)
else()
    message (FATAL_ERROR "not support on ${CMAKE_SYSTEM} yet.")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

qt5_add_resources(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/resources.qrc)


add_library(${PROJECT_NAME}
    SHARED
    ${PLUGIN_FILES}
    ${BUSS_FILES}
    ${RESOURCE_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
)

# 公共依赖库，添加到这里
target_link_libraries(${PROJECT_NAME}
    deepin_cross::framework
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    co
)

# 平台相关的库，添加到这里
if (CMAKE_SYSTEM MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME}
        ${DtkWidget_LIBRARIES}
    )
endif()
if (CMAKE_SYSTEM MATCHES "Windows")
    #target_link_libraries(${PROJECT_NAME}
    #    Qt5::Sql
    #)
endif()
#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_COOPERATION_PLUGIN_DIR}
)
