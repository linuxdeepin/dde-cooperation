cmake_minimum_required(VERSION 3.10)

project(data-transfer-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 插件相关的固定内容，或者公共头文件源文件，添加到这里。
FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.h"
    "${CMAKE_SOURCE_DIR}/src/ipc/frontendservice.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/datatransfercoreplugin.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/datatransfercoreplugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/type_defines.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/connect/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/connect/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/transfer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/transfer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/*.json"
)

# 在这里添加业务代码
if (CMAKE_SYSTEM MATCHES "Windows")
    FILE(GLOB BUSS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/win/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/win/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/select/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/select/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/getbackup/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/getbackup/*.cpp"
    )
   find_package(Qt5 COMPONENTS Sql REQUIRED)
elseif (CMAKE_SYSTEM MATCHES "Linux")
    FILE(GLOB BUSS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/backupload/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/backupload/*.cpp"
    )
    find_package(Dtk COMPONENTS Widget REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)
else()
    message (FATAL_ERROR "not support on ${CMAKE_SYSTEM} yet.")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

qt5_add_resources(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources.qrc)

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
    Qt${QT_VERSION_MAJOR}::Network
    co
)

# 平台相关的库，添加到这里
if (CMAKE_SYSTEM MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME}
        ${DtkWidget_LIBRARIES}
         PkgConfig::libzip
    )
endif()
if (CMAKE_SYSTEM MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
        Qt5::Sql
    )
endif()
#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_DATA_TRANS_PLUGIN_DIR}
)

