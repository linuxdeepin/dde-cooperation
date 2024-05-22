if(NOT TARGET QtZeroConf)

  # Module subdirectory
  set(QTZEROCONF_DIR "${PROJECT_SOURCE_DIR}/3rdparty/QtZeroConf")

  # set build as share library
  set(BUILD_SHARED_LIBS ON)

  # Module subdirectory
  add_subdirectory("${QTZEROCONF_DIR}" QtZeroConf)

if(MSVC)
message("   >>> QtZeroConf build type: ${CMAKE_BUILD_TYPE}")
message("   >>> QtZeroConf runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  # 拷贝编译好的版本windows版本
  file(GLOB OUTPUTS ${CMAKE_INSTALL_LIBDIR}/QtZeroConf.*)
  file(COPY ${OUTPUTS}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dde-cooperation/${CMAKE_BUILD_TYPE})
  message("   >>> copy QtZeroConf output libraries:  ${OUTPUTS}")
endif()

endif()
