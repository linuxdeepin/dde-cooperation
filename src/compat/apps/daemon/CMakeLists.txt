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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if (WIN32)
    # 文件描述信息
    set (PRODUCT_INFO "Deepin cooperation server")
    set (PRODUCT_NAME "cooperation")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    set (RC_FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJ_NAME} WIN32
        MANUAL_FINALIZATION
        ${CPP_SRC}
        ${RC_FILES}
    )
else()
    add_executable(${PROJ_NAME} WIN32
        ${CPP_SRC}
        ${RC_FILES}
    )
endif()

if (WIN32)
    add_link_options(${PROJ_NAME} PRIVATE -mwindows)
endif()

target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJ_NAME} PUBLIC ${COMPAT_ROOT_DIR}/apps)

target_link_libraries(${PROJ_NAME}
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  deepin_cross::framework
)

install(TARGETS ${PROJ_NAME} DESTINATION bin)

