cmake_minimum_required(VERSION 3.10)

project(data-transfer-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB PLUGIN_FILES
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.h"
    "${CMAKE_SOURCE_DIR}/src/base/baseutils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    )

set(QRC_RES "${CMAKE_CURRENT_SOURCE_DIR}/data-transfer.qrc")

#find_package(PkgConfig REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick Gui Widgets)
find_package(Dtk COMPONENTS Widget REQUIRED)


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
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    ${DtkWidget_LIBRARIES}
)

#install library file
install(TARGETS
    ${PROJECT_NAME}
    LIBRARY
    DESTINATION
    ${DEEPIN_DATA_TRANS_PLUGIN_DIR}
)

