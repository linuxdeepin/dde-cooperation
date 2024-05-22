set(PROJ_NAME "deepin-data-transfer")

# enable qt build support
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

#加快qrc编译
qt_add_big_resources(QRC_RESOURCES resources.qrc)
list(APPEND QRC_RESOURCES resources.qrc)
set_property(SOURCE resources.qrc PROPERTY SKIP_AUTORCC ON)
list(APPEND RESOURCE_FILES ${QRC_RESOURCES})

#如果是Windows平台，则生成rc文件，还有inno setup脚本文件
set(VERSION_RC_PATH "")
if (WIN32)
    # 文件描述信息
    set (PRODUCT_INFO "Deepin data transfer")
    set (PRODUCT_NAME "deepin-datatransfer")
    set (PRODUCT_ID "636B356F-47E1-491D-B66E-B254233FFCB1")

    set(VERSION_RC_PATH ${CMAKE_CURRENT_BINARY_DIR}/version_${PROJECT_NAME}.rc)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../version.rc.in ${VERSION_RC_PATH})

    # 安装包制作inno脚本
    configure_file(${CMAKE_SOURCE_DIR}/dist/inno/setup.iss.in ${CMAKE_BINARY_DIR}/${PROJ_NAME}-setup.iss)

    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/${PROJ_NAME})
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJ_NAME}
        MANUAL_FINALIZATION
        ${CPP_SRC}
        ${RESOURCE_FILES}
        ${VERSION_RC_PATH}
    )
else()
    add_executable(${PROJ_NAME}
        ${CPP_SRC}
        ${RESOURCE_FILES}
        ${VERSION_RC_PATH}
    )
endif()

target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/lib/data-transfer)

if (WIN32)
    # add_link_options(${PROJ_NAME} PRIVATE -mwindows)
    #设置属性
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER www.deepin.org
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(${PROJ_NAME}
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Widgets
  data-transfer-core
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
    # 添加一个自定义构建步骤，在构建后立即拷贝翻译文件到目标目录
    add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/translations
        COMMAND ${CMAKE_COMMAND} -E copy ${QM_FILES} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/translations
        COMMENT "Copying translation files to output directory"
    )

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

    # manual
    install(DIRECTORY assets/deepin-data-transfer DESTINATION share/deepin-manual/manual-assets/application)

    # 日志收集配置
    install(FILES res/linux/deepin-data-transfer.json DESTINATION share/deepin-log-viewer/deepin-log.conf.d)
endif()

