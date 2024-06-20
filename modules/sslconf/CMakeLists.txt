cmake_minimum_required(VERSION 3.13)

project(sslconf)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Module subdirectory
set(BARRIER_LIB_DIR "${CMAKE_SOURCE_DIR}/3rdparty/barrier/src/lib")

if (UNIX)
    set(PLATNAME unix)
else()
    set(PLATNAME win32)
endif()

# Module library
file(GLOB SOURCE_FILES
    "${BARRIER_LIB_DIR}/base/finally.h"
    "${BARRIER_LIB_DIR}/io/filesystem.h"
    "${BARRIER_LIB_DIR}/io/filesystem.cpp"
    "${BARRIER_LIB_DIR}/common/DataDirectories.h"
    "${BARRIER_LIB_DIR}/common/${PLATNAME}/DataDirectories.cpp"
    "${BARRIER_LIB_DIR}/common/DataDirectories_static.cpp"
    "${BARRIER_LIB_DIR}/net/FingerprintData.h"
    "${BARRIER_LIB_DIR}/net/FingerprintData.cpp"
    "${BARRIER_LIB_DIR}/net/FingerprintDatabase.h"
    "${BARRIER_LIB_DIR}/net/FingerprintDatabase.cpp"
    "${BARRIER_LIB_DIR}/net/SecureUtils.h"
    "${BARRIER_LIB_DIR}/net/SecureUtils.cpp"
    "*.h"
    "*.cpp"
)

if (APPLE)
    add_definitions (-DWINAPI_CARBON=1 -D_THREAD_SAFE)
else()
    add_definitions (-DWINAPI_XWINDOWS=1)
endif()

add_library(${PROJECT_NAME} ${SOURCE_FILES})
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
endif()

target_compile_definitions (${PROJECT_NAME} PRIVATE -DBARRIER_VERSION="2.4.0")
target_include_directories(${PROJECT_NAME} PUBLIC "${BARRIER_LIB_DIR}" PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

if(FPIC)
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/sslcertconf.h)
# file(COPY ${HEADERS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
