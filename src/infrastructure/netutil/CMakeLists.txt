cmake_minimum_required(VERSION 3.13)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project name
project(netutil)

if (ANDROID)
    message("   >>> ANDROID_HOME  $ENV{ANDROID_HOME}")
    
    if (DEFINED ENV{ANDROID_HOME})
      set(ssl_root_path $ENV{ANDROID_HOME}/android_openssl)
      list(APPEND android_extra_libs
         ${ssl_root_path}/ssl_1.1/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_1_1.so
         ${ssl_root_path}/ssl_1.1/${CMAKE_ANDROID_ARCH_ABI}/libssl_1_1.so)

      set(OPENSSL_ROOT_DIR ${ssl_root_path}/ssl_1.1/${CMAKE_ANDROID_ARCH_ABI})
      set(OPENSSL_INCLUDE_DIR ${ssl_root_path}/ssl_1.1/include)
      set(OPENSSL_LIBRARIES ${android_extra_libs})
      set(OPENSSL_VERSION 1.1)
    else()
        message(FATAL_ERROR "ANDROID_HOME环境变量未设置!")
    endif()
else()
    find_package(OpenSSL REQUIRED)
endif()

list(APPEND LINKLIBS ${OPENSSL_LIBRARIES})

if(WIN32)
  find_package(Crypt)
  find_package(WinSock)
endif()

# Link libraries
if(WIN32)
  list(APPEND LINKLIBS ${CRYPT_LIBRARIES})
  list(APPEND LINKLIBS ${WINSOCK_LIBRARIES})
endif()

if (NOT ANDROID)
  list(APPEND LINKLIBS logging)
endif()

list(APPEND LINKLIBS basekit)

# OpenSSL libraries
message(STATUS "OpenSSL version: ${OPENSSL_VERSION} ${OPENSSL_INCLUDE_DIR} ${OPENSSL_LIBRARIES}")

# Library
file(GLOB_RECURSE LIB_HEADER_FILES "include/*.h" "src/*.h")
file(GLOB_RECURSE LIB_INLINE_FILES "include/*.inl" "src/*.inl")
file(GLOB_RECURSE LIB_SOURCE_FILES "include/*.cpp" "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${LIB_HEADER_FILES} ${LIB_INLINE_FILES} ${LIB_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}" FOLDER "libraries")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} ${LINKLIBS} asio)

if(FPIC)
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
endif()

# 添加测试支持
option(BUILD_NETUTIL_TESTS "Build netutil tests" OFF)
if(BUILD_NETUTIL_TESTS)
    # 启用测试
    enable_testing()
    # 添加tests子目录
    add_subdirectory(tests)
endif()
