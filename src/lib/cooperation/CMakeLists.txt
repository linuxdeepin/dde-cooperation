
if (CMAKE_SYSTEM MATCHES "Linux")
    include("libvnc.cmake")
endif()

add_subdirectory(core)
if (CMAKE_SYSTEM MATCHES "Linux")
    add_subdirectory(transfer)
endif()
