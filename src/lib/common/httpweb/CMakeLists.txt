project(httpweb)

FILE(GLOB CPP_SRC
    "${CMAKE_SOURCE_DIR}/src/common/filesystem.h"
    "${CMAKE_SOURCE_DIR}/src/common/filesystem.cpp"
    *.h
    *.cpp
)

list(APPEND LINKLIBS netutil)

# zlib is the permanent protocol baseline (deflate); every build must have it.
find_package(ZLIB REQUIRED)
list(APPEND LINKLIBS ZLIB::ZLIB)

# zstd is the preferred codec for new peers. Builds without it still work and
# negotiate down to deflate at runtime.
find_package(zstd CONFIG QUIET)
set(HTTPWEB_ZSTD_TARGET "")
if(WIN32 OR ANDROID)
    # Windows/Android: static so the installer/APK needs no extra zstd runtime.
    if(TARGET zstd::libzstd_static)
        set(HTTPWEB_ZSTD_TARGET zstd::libzstd_static)
    elseif(TARGET zstd::libzstd_shared)
        set(HTTPWEB_ZSTD_TARGET zstd::libzstd_shared)
    endif()
else()
    # Linux: prefer the shared library. httpweb is linked into shared libraries
    # (cooperation-core); a static libzstd.a would export ZSTD_*/HUF_* from
    # those .so files, and a second copy (libzstd.so.1 loaded by another
    # dependency) then interposes them. Mixing contexts from two copies makes
    # ZSTD_decompressStream crash inside HUF_readDTableX2_wksp.
    if(TARGET zstd::libzstd_shared)
        set(HTTPWEB_ZSTD_TARGET zstd::libzstd_shared)
    elseif(TARGET zstd::libzstd)
        set(HTTPWEB_ZSTD_TARGET zstd::libzstd)
    elseif(TARGET zstd::libzstd_static)
        set(HTTPWEB_ZSTD_TARGET zstd::libzstd_static)
    endif()
endif()

# Never let host pkg-config leak a host libzstd into a Windows/Android
# cross-build; those platforms must provide an imported zstd target.
if(NOT HTTPWEB_ZSTD_TARGET AND NOT WIN32 AND NOT ANDROID)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ZSTD QUIET IMPORTED_TARGET libzstd)
        if(ZSTD_FOUND)
            set(HTTPWEB_ZSTD_TARGET PkgConfig::ZSTD)
        endif()
    endif()
endif()

# httpweb library
add_library(${PROJECT_NAME} ${CPP_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}" FOLDER "httpweb")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# PUBLIC: httpweb is a static library, so consumers must resolve zlib/zstd too.
target_link_libraries(${PROJECT_NAME} PUBLIC ${LINKLIBS})

if(HTTPWEB_ZSTD_TARGET)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${HTTPWEB_ZSTD_TARGET})
    target_compile_definitions(${PROJECT_NAME} PUBLIC HTTPWEB_HAVE_ZSTD=1)
    message(STATUS "httpweb: zstd enabled (${HTTPWEB_ZSTD_TARGET})")
else()
    message(STATUS "httpweb: zstd not found, deflate/none only")
    if(ANDROID)
        message(STATUS "httpweb: Android needs a prebuilt zstd (expects zstd::libzstd_static); "
                       "pass its prefix via CMAKE_PREFIX_PATH to enable zstd")
    endif()
endif()

if(FPIC)
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
