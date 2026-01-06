if(NOT TARGET barrier)

  # Module subdirectory
  set(BARRIER_DIR "${PROJECT_SOURCE_DIR}/3rdparty/barrier")

  message("   >> include barrier...")

  add_subdirectory("${BARRIER_DIR}" barrier)

endif()
