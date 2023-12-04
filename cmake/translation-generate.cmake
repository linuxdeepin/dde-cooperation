function(TRANSLATION_GENERATE QMS)
  find_package(Qt5LinguistTools QUIET)

  if (NOT Qt5_LRELEASE_EXECUTABLE)
    message(STATUS "set Qt5_LRELEASE_EXECUTABLE = lrelease")
    set(Qt5_LRELEASE_EXECUTABLE "lrelease")
  endif()

  if(NOT ARGN)
    message(SEND_ERROR "Error: TRANSLATION_GENERATE() called without any .ts path")
    return()
  endif()

  # 获取 translations 目录下的所有 .ts 文件
  file(GLOB_RECURSE TS_FILES "${ARGN}/*.ts")

  set(${QMS})
  foreach(TSFIL ${TS_FILES})
      get_filename_component(FIL_WE ${TSFIL} NAME_WE)
#      get_filename_component(TS_DIR ${TSFIL} DIRECTORY)
      set(QMFIL ${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.qm)
      list(APPEND ${QMS} ${QMFIL})
      add_custom_command(
          OUTPUT ${QMFIL}
          # COMMAND ${Qt5_LUPDATE_EXECUTABLE} ${CMAKE_SOURCE_DIR} -ts ${TSFIL}
          COMMAND ${Qt5_LRELEASE_EXECUTABLE} ${TSFIL} -qm ${QMFIL}
          DEPENDS ${TSFIL}
          COMMENT "Running ${Qt5_LRELEASE_EXECUTABLE} on ${TSFIL}"
          VERBATIM
      )
  endforeach()

  set_source_files_properties(${${QMS}} PROPERTIES GENERATED TRUE)
  set(${QMS} ${${QMS}} PARENT_SCOPE)
endfunction()
