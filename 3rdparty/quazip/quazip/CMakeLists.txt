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
    set(LIB_VERSION_SUFFIX ${QT_VERSION_MAJOR})
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
endif()

# Generate moc files for headers
file(GLOB PUBLIC_HEADERS "*.h")
qt_wrap_cpp(MOC_SRCS ${PUBLIC_HEADERS})

# Add generated moc files to sources
file(GLOB SRCS "*.c" "*.cpp")
set(SRCS ${SRCS} ${MOC_SRCS})

# Create libraries with position independent code
add_library(quazip SHARED ${SRCS})
set_property(TARGET quazip PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(quazip_static STATIC ${SRCS})
set_property(TARGET quazip_static PROPERTY POSITION_INDEPENDENT_CODE ON)

# Handle library naming
if(NOT WIN32)
    set_target_properties(quazip_static PROPERTIES OUTPUT_NAME quazip${LIB_VERSION_SUFFIX})
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

# Include directories
target_include_directories(quazip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(quazip PRIVATE BUILD)

# Install targets
install(FILES ${PUBLIC_HEADERS} DESTINATION include/quazip${LIB_VERSION_SUFFIX})
install(TARGETS
    quazip
    quazip_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION lib
)
