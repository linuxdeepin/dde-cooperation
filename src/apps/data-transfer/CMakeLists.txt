set(PROJ_NAME "deepin-data-transfer")

configure_file(
    "../config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

FILE(GLOB CPP_SRC
    *.h
    *.cpp
    ${CMAKE_SOURCE_DIR}/src/base/*.h
    ${CMAKE_SOURCE_DIR}/src/base/*.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/src)

if (CMAKE_SYSTEM MATCHES "Linux")
    find_package(Dtk COMPONENTS Widget REQUIRED)
endif()

if (WIN32)
    set(qt_required_components Core Gui Network Widgets Sql WinExtras)
else()
    set(qt_required_components Core Gui Network Widgets)
endif()
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${qt_required_components})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${qt_required_components})

qt5_add_resources(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

if (WIN32)
    # 程序图标
    set (SET_ICON "IDI_ICON1 ICON DISCARDABLE ${CMAKE_CURRENT_SOURCE_DIR}/res/win/${PROJ_NAME}.ico")
    # 文件描述信息
    set (PRODUCT_INFO "Deepin data transfer client")
    set (PRODUCT_NAME "deepin-datatransfer")
    set (PRODUCT_ID "636B356F-47E1-491D-B66E-B254233FFCB1")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    set (RC_FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

    # 安装包制作inno脚本
    configure_file(${CMAKE_SOURCE_DIR}/dist/inno/setup.iss.in ${CMAKE_BINARY_DIR}/${PROJ_NAME}-setup.iss)

    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/${PROJ_NAME})
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJ_NAME} WIN32
        MANUAL_FINALIZATION
        ${CPP_SRC}
        ${RESOURCE_FILES}
        ${RC_FILES}
    )
else()
    add_executable(${PROJ_NAME} WIN32
        ${CPP_SRC}
        ${RESOURCE_FILES}
        ${RC_FILES}
    )
endif()

if (WIN32)
    add_link_options(${PROJ_NAME} PRIVATE -mwindows)
endif()

target_link_libraries(${PROJ_NAME}
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Widgets
  deepin_cross::framework
)

if (CMAKE_SYSTEM MATCHES "Linux")
    target_link_libraries(${PROJ_NAME}
          PRIVATE
        ${DtkWidget_LIBRARIES}
    )
endif()

install(TARGETS ${PROJ_NAME} DESTINATION bin)

# translation files
TRANSLATION_GENERATE(QM_FILES ${CMAKE_SOURCE_DIR}/translations/${PROJ_NAME})
add_custom_target(${PROJ_NAME}_qm_files DEPENDS ${QM_FILES})
add_dependencies(${PROJ_NAME} ${PROJ_NAME}_qm_files)

if (WIN32)
    # main entry vbs
    file(GLOB SETUPS ${CMAKE_CURRENT_SOURCE_DIR}/res/win/main.vbs)
    file(COPY ${SETUPS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)

    #install(FILES... does not work
    FILE(GLOB_RECURSE TRAN_FILES ${QM_FILES})
    file(COPY ${TRAN_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)

    # copy common share libs
    file(GLOB DLLS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../*.dll)
    file(COPY ${DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)

    deployqt_qt_runtime(${PROJ_NAME} ${qt_required_components})
else()
    # translation
    install(FILES ${QM_FILES} DESTINATION share/${PROJ_NAME}/translations)

    # icons
    FILE(GLOB ICON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/linux/*.svg)
    install(FILES ${ICON_FILES} DESTINATION share/icons/hicolor/scalable/apps)

    # desktop
    install(FILES res/linux/deepin-data-transfer.desktop DESTINATION share/applications)

    # 日志收集配置
    install(FILES res/linux/deepin-data-transfer.json DESTINATION share/deepin-log-viewer/deepin-log.conf.d)
endif()

