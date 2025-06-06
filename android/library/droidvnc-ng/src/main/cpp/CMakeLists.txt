# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.4.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("droidvnc-ng" C)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared Libs" FORCE)

# Requred to enable SIMD support
if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=aarch64-linux-android${ANDROID_VERSION}")
elseif (CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi.*")  # armeabi-v7a || armeabi-v6 || armeabi
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=arm-linux-androideabi${ANDROID_VERSION}")
endif ()

# # build libJPEG
# message("------libjpeg-turbo-----")
# set(libjpeg_src_DIR ${CMAKE_SOURCE_DIR}/../../../libjpeg-turbo)
# set(libjpeg_build_DIR ${CMAKE_BINARY_DIR}/libjpeg)
# # adds a source subdir with specifying a build output dir
# add_subdirectory(${libjpeg_src_DIR} ${libjpeg_build_DIR})
# # set these variables so FindJPEG can find the library
# set(JPEG_LIBRARY ${libjpeg_build_DIR}/libturbojpeg.a CACHE FILEPATH "")
# set(JPEG_INCLUDE_DIR ${libjpeg_src_DIR} CACHE PATH "")
# # set include directories so dependent code can find the headers
# include_directories(
#         ${libjpeg_src_DIR}
#         ${libjpeg_build_DIR}
# )

# # build LibVNCServer
# message("------LibVNCServer-----")
# set(libvnc_src_DIR ${CMAKE_SOURCE_DIR}/../../../libvncserver)
# set(libvnc_build_DIR ${CMAKE_BINARY_DIR}/libvnc)
# # adds a source subdir with specifying a build output dir
# add_subdirectory(${libvnc_src_DIR} ${libvnc_build_DIR})
# # need to add the build dir to include dirs as well because of generated rfbconfig.h
# include_directories(
#         ${libvnc_src_DIR}/include
#         ${libvnc_build_DIR}/include
# )

# message("libvnc_build_DIR= ${libvnc_build_DIR}")

# # build libdroidvnc-ng
# add_library(${CMAKE_PROJECT_NAME} SHARED droidvnc-ng.c)
# target_link_libraries(${CMAKE_PROJECT_NAME}
#         log
#         vncserver
# )

# link LibVNCServer
message("------LibVNCServer-----")
set(LIBVNCSERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libvncserver)
set(LIBVNCSERVER_LIBS
    ${LIBVNCSERVER_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/libvncserver.so
)

include_directories(
    ${LIBVNCSERVER_DIR}/include
)

# build libdroidvnc-ng
add_library(${CMAKE_PROJECT_NAME} SHARED droidvnc-ng.c)
target_link_libraries(${CMAKE_PROJECT_NAME}
    log
    ${LIBVNCSERVER_LIBS}
)