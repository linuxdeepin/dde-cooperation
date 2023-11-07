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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network Widgets)

qt5_add_resources(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJ_NAME}
        MANUAL_FINALIZATION
        ${CPP_SRC}
        ${RESOURCE_FILES}
    )
else()
    add_executable(${PROJ_NAME}
        ${CPP_SRC}
        ${RESOURCE_FILES}
    )
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

if (WIN32)
    # icon, start and inno setup script file
    file(GLOB SETUPS ${CMAKE_CURRENT_SOURCE_DIR}/setup/win/*)
    file(COPY ${SETUPS}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
else()
    # icons
    FILE(GLOB ICON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/setup/linux/*.svg)
    install(FILES ${ICON_FILES} DESTINATION share/icons/hicolor/scalable/apps)

    # desktop
    install(FILES setup/linux/deepin-data-transfer.desktop DESTINATION share/applications)
endif()

