# Minimal CMake configuration for quazip subdirectory
# All main build configuration is in parent CMakeLists.txt

# Generate moc files for headers
file(GLOB PUBLIC_HEADERS "*.h")
qt_wrap_cpp(MOC_SRCS ${PUBLIC_HEADERS})

# Add generated moc files to sources
file(GLOB SRCS "*.c" "*.cpp")
set(SRCS ${SRCS} ${MOC_SRCS})

# Set include directories for this subdirectory only
target_include_directories(quazip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(quazip PRIVATE QUAZIP_BUILD)
