if(NOT TARGET quazip)

  # Module subdirectory
  set(QUAZIP_DIR "${PROJECT_SOURCE_DIR}/3rdparty/quazip")

  message("   >> include quazip module...")

  # Add quazip library
  if (UNIX)
    message("   >> Linux platform, exclude quazip module!")
  else()
    file(GLOB ZLIB_HEADERS ${PROJECT_SOURCE_DIR}/3rdparty/zlib-header/*)
    file(COPY ${ZLIB_HEADERS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/quazip/quazip/)
    add_subdirectory("${QUAZIP_DIR}" quazip)
    include_directories(${QUAZIP_DIR}/quazip)

    # ��������ļ���Ӧ��
    file(GLOB OUTPUTS ${CMAKE_INSTALL_LIBDIR}/quazip5d.*)
    file(COPY ${OUTPUTS}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data-transfer/${CMAKE_BUILD_TYPE})
    message("   >>> copy QtZeroConf output libraries:  ${OUTPUTS}")
  endif()

endif()
