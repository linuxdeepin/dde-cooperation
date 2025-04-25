cmake_minimum_required(VERSION 3.13)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project name
project(logging)

# Link libraries
list(APPEND LINKLIBS basekit)

# Support zlib/contrib/minizip
file(GLOB_RECURSE MINIZIP_FILES "src/logging/appenders/minizip/*.c")
if(NOT WIN32)
  list(REMOVE_ITEM MINIZIP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/logging/appenders/minizip/iowin32.c")
endif()

# Library
file(GLOB_RECURSE LIB_HEADER_FILES "include/*.h" "src/*.h")
file(GLOB_RECURSE LIB_INLINE_FILES "include/*.inl" "src/*.inl")
file(GLOB_RECURSE LIB_SOURCE_FILES "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${LIB_HEADER_FILES} ${LIB_INLINE_FILES} ${LIB_SOURCE_FILES} ${MINIZIP_FILES})
if(MSVC)
  # zlib
  target_include_directories(${PROJECT_NAME} PRIVATE "zlib/include")
  set(ZLIB_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zlib.lib")

  # C4067: unexpected tokens following preprocessor directive - expected a newline
  # C4131: 'function' : uses old-style declarator
  # C4189: 'identifier' : local variable is initialized but not referenced
  # C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
  # C4456: declaration of 'identifier' hides previous local declaration
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} /wd4067 /wd4131 /wd4189 /wd4244 /wd4456" FOLDER "libraries")
else()
  find_package(ZLIB REQUIRED)

  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow -Wno-unused-variable" FOLDER "libraries")
endif()
if(CYGWIN OR MINGW)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FILE32API=1)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} ${LINKLIBS} ${ZLIB_LIBRARIES})

if(FPIC)
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# 添加测试支持
option(BUILD_LOGGING_TESTS "Build logging tests" OFF)
if(BUILD_LOGGING_TESTS)
    # 启用测试
    enable_testing()
    # 添加tests子目录
    add_subdirectory(tests)
endif()