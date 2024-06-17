if (WIN32)
    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/dde-cooperation)
endif()
add_subdirectory(core)

if (CMAKE_SYSTEM MATCHES "Linux")
    add_subdirectory(daemon)
endif()
