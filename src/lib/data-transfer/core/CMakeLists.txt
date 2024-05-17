cmake_minimum_required(VERSION 3.10)

# enable qt build support
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# set build as share library
set(BUILD_SHARED_LIBS ON)

project(data-transfer-core VERSION 1.0.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 插件相关的固定内容，或者公共头文件源文件，添加到这里。
FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/log.h"
    "${CMAKE_SOURCE_DIR}/src/common/logger.h"
    "${CMAKE_SOURCE_DIR}/src/common/logger.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.h"
    "${CMAKE_SOURCE_DIR}/src/common/commonutils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/datatransfercoreplugin.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/datatransfercoreplugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/type_defines.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/connect/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/connect/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/transfer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/transfer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/transferhepler.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/transferhepler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/transferworker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/transferworker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/optionsmanager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/optionsmanager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
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
    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/data-transfer)
elseif (CMAKE_SYSTEM MATCHES "Linux")
    FILE(GLOB BUSS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/linux/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/backupload/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/backupload/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/settinghepler.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/settinghepler.cpp"
    )
    find_package(Dtk COMPONENTS Widget REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libzip REQUIRED IMPORTED_TARGET libzip)
else()
    message (FATAL_ERROR "not support on ${CMAKE_SYSTEM} yet.")
endif()

if (WIN32)
    set(qt_required_components Core Gui Network Widgets  Sql WinExtras)
else()
    set(qt_required_components Core Gui Network Widgets DBus)
endif()
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${qt_required_components})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${qt_required_components})

qt5_add_resources(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources.qrc)

list(APPEND LINKLIBS cpplogging)
list(APPEND LINKLIBS session)
list(APPEND LINKLIBS httpweb)

add_library(${PROJECT_NAME}
    ${PLUGIN_FILES}
    ${BUSS_FILES}
    ${RESOURCE_FILES}
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_SHARED)
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
)

# 公共依赖库，添加到这里
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${LINKLIBS}
)

# 平台相关的库，添加到这里
if (CMAKE_SYSTEM MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${DtkWidget_LIBRARIES}
         PkgConfig::libzip
    )
endif()
if (CMAKE_SYSTEM MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${QUAZIP_LIB_TARGET_NAME}
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::WinExtras
    )

    target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/3rdparty/quazip/quazip
    )
endif()


add_library(quazipd SHARED IMPORTED)
set_target_properties(quazipd PROPERTIES

   IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/quazipd.lib
)

#install library file
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if (WIN32)
    deployqt_qt_runtime(${PROJECT_NAME} ${qt_required_components})
endif()
