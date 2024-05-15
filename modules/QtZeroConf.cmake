if(NOT TARGET QtZeroConf)

  # Module subdirectory
  set(QTZEROCONF_DIR "${PROJECT_SOURCE_DIR}/3rdparty/QtZeroConf")

  # Module subdirectory
  add_subdirectory("${QTZEROCONF_DIR}" QtZeroConf)
endif()
