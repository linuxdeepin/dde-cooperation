if(NOT TARGET QtZeroConf)

  # Module subdirectory
  set(QTZEROCONF_DIR "${PROJECT_SOURCE_DIR}/3rdparty/QtZeroConf")

  # set build as share library
  set(BUILD_SHARED_LIBS ON)

  # Module subdirectory
  add_subdirectory("${QTZEROCONF_DIR}" QtZeroConf)
endif()
