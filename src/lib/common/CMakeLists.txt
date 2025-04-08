include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/infrastructure/basekit/include")
include_directories("${CMAKE_SOURCE_DIR}/src/infrastructure/logging/include")
include_directories("${CMAKE_SOURCE_DIR}/src/infrastructure/netutil/include")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/proto")

# protocol library
add_subdirectory("proto")
 
# network communication session
add_subdirectory("session")

# the file sync server and client
add_subdirectory("httpweb")

add_subdirectory("manager")
