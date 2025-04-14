if(ENABLE_SLOTIPC)
    include("slotipc.cmake")
endif()

# the ssl configuration module, must before Compiler features
add_subdirectory(sslconf)

# Compiler features
include(SetCompilerFeatures)
include(SetPlatformFeatures)
include(SystemInformation)
if (NOT ANDROID)
include(SetCompilerWarnings)
endif()

# uuid library first for android
if (ANDROID)
include("android-uuid.cmake")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# import non-boost asio
include("asio.cmake")

# build the base kit first
add_subdirectory(basekit)

if (NOT ANDROID)
add_subdirectory(logging)
endif()

# network utils
add_subdirectory(netutil)

# rpc module for compat old protocal
# add_subdirectory(zrpc)
