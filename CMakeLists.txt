cmake_minimum_required(VERSION 3.13)

project(dde-cooperation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-need")

    #安全加固
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")

    #安全加固
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -z relro")
        add_definitions(-D ARCH_MIPSEL)
    endif()
endif()

if(WIN32)
    # windows7 support
    add_definitions(-D_WIN32_WINNT=0x0601)

    # windeployqt 依赖
    include (cmake/qt-helper.cmake)
endif()

include(GNUInstallDirs)

option(FPIC "build with -fPIC" FALSE)

if(MSVC)
    add_compile_options(/fp:fast /EHsc)
    add_link_options(/SAFESEH:NO)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
else()
    add_compile_options(-Wall -O2 -g -Wno-sign-compare -Wno-strict-aliasing)
    set(FPIC TRUE)
endif()

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
endif()

include (cmake/version.cmake)

message("   >>> LIB_INSTALL_DIR  ${LIB_INSTALL_DIR}")
message("   >>> COMPILE ON SYSTEM: ${CMAKE_SYSTEM}")

include_directories(include)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

if (WIN32)
    set(DEPLOY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    message("   >>> DEPLOY_OUTPUT_DIRECTORY  ${DEPLOY_OUTPUT_DIRECTORY}")

    # windows runtime output defined
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})
#    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})
#    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY})

    file(GLOB INNO_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/dist/inno/scripts)
    file(COPY ${INNO_SCRIPTS} DESTINATION ${CMAKE_BINARY_DIR})
endif()

## 如果未设置CMAKE_BUILD_TYPE，则将其设置为Release
#if(NOT CMAKE_BUILD_TYPE)
#    message("Cmake build type is not defined!")
#    # set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
#else()

#if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
#    # 设置环境变量
#    set(ENV{QT_SELECT} "5")
#    # generate qm
#    execute_process(COMMAND bash "translate_generation.sh"
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
#endif()

#endif()
# 引入翻译生成
include(translation-generate)

# header-only libraries: jwt, picojson
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include)

# Modules
add_subdirectory("modules")

if(MSVC)
    add_compile_options(/utf-8)
endif()
add_subdirectory(src)
