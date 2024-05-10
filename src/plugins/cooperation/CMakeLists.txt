if (WIN32)
    # 从定位输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEPLOY_OUTPUT_DIRECTORY}/dde-cooperation)
endif()

# 公共定义
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/share")

add_subdirectory(core)

if (CMAKE_SYSTEM MATCHES "Linux")
    add_subdirectory(daemon)
endif()
