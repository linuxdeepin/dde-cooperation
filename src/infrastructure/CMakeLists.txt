if(ENABLE_SLOTIPC)
    include("slotipc.cmake")
endif()

# the ssl configuration module
add_subdirectory(sslconf)

# Compiler features
include(SetCompilerFeatures)
if (NOT ANDROID)
include(SetCompilerWarnings)
endif()
include(SetPlatformFeatures)
include(SystemInformation)

# import non-boost asio
include("asio.cmake")

if (ANDROID)
include("android-uuid.cmake")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# build the base kit first
add_subdirectory(basekit)

if (NOT ANDROID)
add_subdirectory(logging)
endif()

# network utils
add_subdirectory(netutil)

# rpc module for compat old protocal
# add_subdirectory(zrpc)
