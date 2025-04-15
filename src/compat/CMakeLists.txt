if(NOT TARGET COMPAT_DEAMON)
    include("${CMAKE_SOURCE_DIR}/modules/compat.cmake")

    set(COMPAT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    include_directories(${COMPAT_ROOT_DIR})
    include_directories(${COMPAT_ROOT_DIR}/include)

    # zrpc module base on coost library
    set(ZRPC_DIR "${COMPAT_ROOT_DIR}/zrpc")
    include_directories(${ZRPC_DIR}/include)
    add_subdirectory("${ZRPC_DIR}" zrpc)    

    add_subdirectory(protocol)

    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_subdirectory(framework)
    add_subdirectory(plugins)

    add_subdirectory(apps)

endif()