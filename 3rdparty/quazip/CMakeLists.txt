cmake_minimum_required(VERSION 3.13)

project(QuaZip)

set(CMAKE_CXX_STANDARD 17)

# CMP0042: Explicitly acknowledge MACOSX_RPATH
cmake_policy(SET CMP0042 NEW)

option(BUILD_WITH_QT4 "Build QuaZip with Qt4 no matter if Qt5/Qt6 was found" OFF)

if(NOT BUILD_WITH_QT4)
    # Find Qt version
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
    set(QTCORE_LIBRARIES Qt${QT_VERSION_MAJOR}::Core)
    set(QUAZIP_LIB_VERSION_SUFFIX ${QT_VERSION_MAJOR})
    include_directories(${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS})
    macro(qt_wrap_cpp)
        if(QT_VERSION_MAJOR EQUAL 6)
            qt6_wrap_cpp(${ARGN})
        else()
            qt5_wrap_cpp(${ARGN})
        endif()
    endmacro()
else()
    set(qt_min_version "4.5.0")
    find_package(Qt4 REQUIRED)
    set(QT_USE_QTGUI false)
    include(${QT_USE_FILE})
    include_directories(${QT_INCLUDES})
    set(QTCORE_LIBRARIES ${QT_QTCORE_LIBRARY})

    macro(qt_wrap_cpp)
        qt4_wrap_cpp(${ARGN})
    endmacro()
endif()

# Use system zlib on unix and Qt ZLIB on Windows
if(UNIX OR MINGW)
    find_package(ZLIB REQUIRED)
else(UNIX OR MINGW)
    # set(ZLIB_INCLUDE_DIRS "${QT_ROOT}/src/3rdparty/zlib" CACHE STRING "Path to ZLIB headers of Qt")
    # set(ZLIB_LIBRARIES "")
    # if(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
    #     message("Please specify a valid zlib include dir")
    # endif(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
endif(UNIX OR MINGW)

# All build libraries are moved to this directory
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(LIB_DESTINATION "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Library directory name" FORCE)
set(QUAZIP_LIB_TARGET_NAME quazip CACHE
    INTERNAL "Target name of libquazip" FORCE)

# Define sources and headers
file(GLOB_RECURSE QUAZIP_SRCS "quazip/*.c" "quazip/*.cpp")
file(GLOB_RECURSE QUAZIP_HEADERS "quazip/*.h")

# Create libraries with position independent code
add_library(quazip SHARED ${QUAZIP_SRCS})
set_property(TARGET quazip PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(quazip_static STATIC ${QUAZIP_SRCS})
set_property(TARGET quazip_static PROPERTY POSITION_INDEPENDENT_CODE ON)

# Handle library naming
if(NOT WIN32)
    set_target_properties(quazip_static PROPERTIES OUTPUT_NAME quazip${QUAZIP_LIB_VERSION_SUFFIX})
endif()

set_target_properties(quazip quazip_static PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    DEBUG_POSTFIX d
)

# Link libraries
target_link_libraries(quazip PRIVATE
    quazip_static
    ${QTCORE_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

# Install targets
install(FILES ${QUAZIP_HEADERS} DESTINATION include/quazip${QUAZIP_LIB_VERSION_SUFFIX})
install(TARGETS
    quazip
    quazip_static
    LIBRARY DESTINATION ${LIB_DESTINATION}
    ARCHIVE DESTINATION ${LIB_DESTINATION}
    RUNTIME DESTINATION ${LIB_DESTINATION}
)

# install(FILES FindQuaZip.cmake RENAME FindQuaZip${QUAZIP_LIB_VERSION_SUFFIX}.cmake DESTINATION ${CMAKE_ROOT}/Modules)
