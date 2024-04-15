# Proto library
file(GLOB_RECURSE PROTO_HEADER_FILES "proto/*.h")
file(GLOB_RECURSE PROTO_INLINE_FILES "proto/*.inl")
file(GLOB_RECURSE PROTO_SOURCE_FILES "proto/*.cpp")
add_library(proto ${PROTO_HEADER_FILES} ${PROTO_INLINE_FILES} ${PROTO_SOURCE_FILES})
set_target_properties(proto PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}" FOLDER "proto")
target_include_directories(proto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/proto")
target_link_libraries(proto ${LINKLIBS})

# network communication session
add_subdirectory("session")

# the file sync server and client
add_subdirectory("filesync")

# launcher
add_subdirectory("launcher")

# enable qt build support
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(apps)
add_subdirectory(framework)
add_subdirectory(plugins)

add_subdirectory(protocol)
