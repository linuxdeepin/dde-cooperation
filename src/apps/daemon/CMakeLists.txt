set(PROJ_NAME "cooperation-daemon")

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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if (WIN32)
    # 管理员权限
    set (RC_FILES res/win/daemon.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJ_NAME}
        MANUAL_FINALIZATION
        ${CPP_SRC}
        ${RC_FILES}
    )
else()
    add_executable(${PROJ_NAME}
        ${CPP_SRC}
        ${RC_FILES}
    )
endif()

target_link_libraries(${PROJ_NAME}
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  deepin_cross::framework
)

install(TARGETS ${PROJ_NAME} DESTINATION bin)

# systemd service
install(FILES service/${PROJ_NAME}.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user)

# xdg-autostart
install(FILES service/${PROJ_NAME}.desktop
DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)
