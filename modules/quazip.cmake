if(NOT TARGET quazip)

  # Module subdirectory
  set(QUAZIP_DIR "${PROJECT_SOURCE_DIR}/3rdparty/quazip")

  message("   >> include quazip module...")

  # Add quazip library
  if (UNIX)
    message("   >> Linux platform, exclude quazip module!")
  else()
      add_subdirectory("${QUAZIP_DIR}/quazip" quazip)
      include_directories(${QUAZIP_DIR}/quazip)
  endif()

endif()
