cmake_minimum_required(VERSION 3.11)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project name
project(asio)

if (NOT OPENSSL_VERSION)
  find_package(OpenSSL REQUIRED)
  # OpenSSL libraries
  message(STATUS "asio > OpenSSL version: ${OPENSSL_VERSION} ${OPENSSL_INCLUDE_DIR} ${OPENSSL_LIBRARIES}")
endif()

# Module library
file(GLOB SOURCE_FILES "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}")
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE ASIO_STANDALONE ASIO_SEPARATE_COMPILATION)
target_include_directories(${PROJECT_NAME} PUBLIC "include" PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

if(FPIC)
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
endif()

