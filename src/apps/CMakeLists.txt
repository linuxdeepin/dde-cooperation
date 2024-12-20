add_subdirectory(dde-cooperation)

if (CMAKE_SYSTEM MATCHES "Linux")
    add_subdirectory(dde-cooperation-transfer)
endif()

add_subdirectory(data-transfer)

# launcher
# add_subdirectory("launcher")
