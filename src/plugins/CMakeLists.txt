add_subdirectory(data-transfer)
add_subdirectory(daemon)

if (CMAKE_SYSTEM MATCHES "Linux")
add_subdirectory(cooperation)
endif()
