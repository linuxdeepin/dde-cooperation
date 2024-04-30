# protocol library
add_subdirectory("proto")
 
# network communication session
add_subdirectory("session")

# the file sync server and client
add_subdirectory("filesync")

if (WIN32)
    set(QUAZIP_DIR "${PROJECT_SOURCE_DIR}/3rdparty/quazip")
    include_directories(${QUAZIP_DIR}/quazip)
endif()

# launcher
# add_subdirectory("launcher")

# enable qt build support
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(apps)
add_subdirectory(framework)
add_subdirectory(plugins)
