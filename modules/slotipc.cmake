if(NOT TARGET SlotIPC)

  # Module subdirectory
  set(SLOTIPC_DIR "${PROJECT_SOURCE_DIR}/3rdparty/slotipc")

  # set build as share library
  if(MSVC)
    set(BUILD_SHARED_LIBS OFF)
  else()
    set(BUILD_SHARED_LIBS ON)
  endif()

  SET(QT_DESIRED_VERSION ${QT_VERSION_MAJOR})

  # Module subdirectory
  add_subdirectory("${SLOTIPC_DIR}" SlotIPC)
  include_directories(${SLOTIPC_DIR}/include)

if(MSVC)
  # 拷贝输出文件到应用
  file(GLOB OUTPUTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/SlotIPC.*)
  file(COPY ${OUTPUTS}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dde-cooperation/${CMAKE_BUILD_TYPE})
  message("   >>> copy SlotIPC output libraries:  ${OUTPUTS}")
endif()

endif()
