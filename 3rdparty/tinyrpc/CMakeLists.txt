cmake_minimum_required(VERSION 3.1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(tinyrpc)

find_package(Protobuf REQUIRED)

add_subdirectory(baseproto)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/baseproto)
link_libraries(baseproto)



