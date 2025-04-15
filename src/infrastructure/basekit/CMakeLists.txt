cmake_minimum_required(VERSION 3.13)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# fmt library > 10.1
include("fmt.cmake")

# Project name
project(basekit)

# CMake module path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

# External packages
find_package(Threads REQUIRED)
if(UNIX AND NOT APPLE AND NOT MSYS)
  find_package(LibBFD)
  find_package(LibDL)
  find_package(LibRT)
  find_package(LibUUID)
endif()
if(WIN32 OR MSYS)
  find_package(DbgHelp)
  find_package(RPC)
  find_package(Userenv)
endif()

# Link libraries
list(APPEND LINKLIBS Threads::Threads)
if(UNIX AND NOT APPLE AND NOT MSYS)
  list(APPEND LINKLIBS ${LIBBFD_LIBRARIES})
  list(APPEND LINKLIBS ${LIBDL_LIBRARIES})
  list(APPEND LINKLIBS ${LIBRT_LIBRARIES})
  list(APPEND LINKLIBS ${LIBUUID_LIBRARIES})
endif()
if(WIN32 OR MSYS)
  list(APPEND LINKLIBS ${DBGHELP_LIBRARIES})
  list(APPEND LINKLIBS ${RPC_LIBRARIES})
  list(APPEND LINKLIBS ${USERENV_LIBRARIES})
  list(APPEND LINKLIBS ${VLD_LIBRARIES})
endif()

# Library
file(GLOB_RECURSE LIB_HEADER_FILES "include/*.h" "src/*.h")
file(GLOB_RECURSE LIB_INLINE_FILES "include/*.inl" "src/*.inl")
file(GLOB_RECURSE LIB_SOURCE_FILES "include/*.cpp" "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${LIB_HEADER_FILES} ${LIB_INLINE_FILES} ${LIB_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}" FOLDER "libraries")
if(FPIC)
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PUBLIC ${vld})
target_link_libraries(${PROJECT_NAME} ${LINKLIBS} fmt)

# # Enable testing
# enable_testing()

# # Add tests
# add_subdirectory(tests)
