if(NOT TARGET barrier)

  # Module subdirectory
  set(BARRIER_DIR "${PROJECT_SOURCE_DIR}/3rdparty/barrier")

  message("   >> include barrier...")
  # barrier v2.4.0 release, upstream: https://github.com/debauchee/barrier
  if (UNIX)
      add_subdirectory("${BARRIER_DIR}" barrier)
  else()
      # 拷贝编译好的版本windows版本
      file(GLOB BARRIER_BINS ${BARRIER_DIR}/bins/*)
      file(COPY ${BARRIER_BINS}
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dde-cooperation)
      message("   >>> copy barrier binary and depends:  ${BARRIER_BINS}")
  endif()

endif()
