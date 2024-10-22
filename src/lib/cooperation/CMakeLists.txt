# build LibVNCServer
message("   >> Building LibVNCServer from android droidvnc-ng directory...")
set(WITH_EXAMPLES OFF)
set(WITH_TESTS OFF)
set(LIBVNCSERVER_INSTALL OFF)
set(libvnc_src_DIR ${CMAKE_SOURCE_DIR}/android/library/droidvnc-ng/libvncserver)
set(libvnc_build_DIR ${CMAKE_BINARY_DIR}/libvnc)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libvnc_src_DIR} ${libvnc_build_DIR})
# need to add the build dir to include dirs as well because of generated rfbconfig.h
include_directories(
        ${libvnc_src_DIR}/include
        ${libvnc_build_DIR}/include
)

add_subdirectory(core)
if (CMAKE_SYSTEM MATCHES "Linux")
    add_subdirectory(transfer)
endif()
