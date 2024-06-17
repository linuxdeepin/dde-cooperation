if (WIN32)
    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/deepin-data-transfer)
endif()
add_subdirectory(core)
