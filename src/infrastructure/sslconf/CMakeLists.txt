cmake_minimum_required(VERSION 3.13)

project(sslconf C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_CXX_STANDARD 17)

if (UNIX)
    set(PLATNAME unix)
    add_definitions(-DSYSAPI_UNIX=1)
else()
    set(PLATNAME win32)
    add_definitions(-DSYSAPI_WIN32=1)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _BIND_TO_CURRENT_VCLIBS_VERSION=1")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2")
endif()

if (NOT OPENSSL_VERSION)
  find_package(OpenSSL REQUIRED)
  # OpenSSL libraries
  message(STATUS "sslconf > OpenSSL version: ${OPENSSL_VERSION} ${OPENSSL_INCLUDE_DIR} ${OPENSSL_LIBRARIES}")
endif()

# Module library
file(GLOB SOURCE_FILES
    "src/${PLATNAME}/*.h"
    "src/${PLATNAME}/*.cpp"
    "include/*.h"
    "src/*.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

if(FPIC)
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
