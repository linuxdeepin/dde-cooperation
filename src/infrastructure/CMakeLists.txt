# import non-boost asio
include("asio.cmake")

# build the base kit first
add_subdirectory(basekit)

if (NOT ANDROID)
add_subdirectory(logging)
else()
include("android-uuid.cmake")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# network utils
add_subdirectory(netutil)

if(ENABLE_SLOTIPC)
    include("slotipc.cmake")
endif()

# rpc module for compat old protocal
# add_subdirectory(zrpc)
