cmake_minimum_required(VERSION 3.13)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(proto)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Proto library
file(GLOB_RECURSE PROTO_HEADER_FILES "*.h")
file(GLOB_RECURSE PROTO_INLINE_FILES "*.inl")
file(GLOB_RECURSE PROTO_SOURCE_FILES "*.cpp")
add_library(proto ${PROTO_HEADER_FILES} ${PROTO_INLINE_FILES} ${PROTO_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}" FOLDER "proto")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND LINKLIBS fmt)
list(APPEND LINKLIBS cppcommon)
message(STATUS "Proto library added ${LINKLIBS}")

message(STATUS "bin output dir: ${CMAKE_BINARY_DIR}")
link_directories("${CMAKE_BINARY_DIR}/output")
target_link_libraries(${PROJECT_NAME} PRIVATE cppserver)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)


#install library file
#install(TARGETS
#    ${PROJECT_NAME}
 #   LIBRARY
 #   DESTINATION
 #   ${LIB_INSTALL_DIR}
#)
