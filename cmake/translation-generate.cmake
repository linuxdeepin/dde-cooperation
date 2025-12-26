# 查找Qt Linguist工具 (lupdate, lrelease)
if (NOT QT_LRELEASE OR NOT QT_LUPDATE)
    find_package(Qt${QT_VERSION_MAJOR}LinguistTools QUIET)

    if (NOT Qt${QT_VERSION_MAJOR}_LRELEASE_EXECUTABLE)
        if (UNIX)
            # Allow callers to override the search prefix for Qt translation tools.
            # This improves portability across distros with different Qt layouts
            # (e.g. /usr/lib/qtX/bin vs /lib/qtX/bin, custom prefixes, etc.).
            set(QT_TRANSLATION_TOOLS_PREFIX "" CACHE PATH
                "Prefix where Qt translation tools (lupdate, lrelease) are installed")
            set(_qt_translation_tool_paths)
            # If the user specified a prefix, search common layouts under it first.
            if (QT_TRANSLATION_TOOLS_PREFIX)
                list(APPEND _qt_translation_tool_paths
                    "${QT_TRANSLATION_TOOLS_PREFIX}/lib/qt${QT_VERSION_MAJOR}/bin"
                    "${QT_TRANSLATION_TOOLS_PREFIX}/lib/qt/bin"
                    "${QT_TRANSLATION_TOOLS_PREFIX}/bin")
            endif()
            # Common system locations on typical UNIX distros.
            list(APPEND _qt_translation_tool_paths
                "/usr/lib/qt${QT_VERSION_MAJOR}/bin"
                "/usr/lib/qt/bin"
                "/lib/qt${QT_VERSION_MAJOR}/bin"
                "/lib/qt/bin")
            find_program(QT_LUPDATE NAMES lupdate
                PATHS ${_qt_translation_tool_paths} NO_DEFAULT_PATH)
            find_program(QT_LRELEASE NAMES lrelease
                PATHS ${_qt_translation_tool_paths} NO_DEFAULT_PATH)
        endif()
        if (NOT QT_LUPDATE)
            find_program(QT_LUPDATE NAMES lupdate lupdate-qt${QT_VERSION_MAJOR})
            if (QT_LUPDATE)
                message(STATUS "Found lupdate in sys path = ${QT_LUPDATE}")
            else()
                message(STATUS "NOT found lupdate in sys path")
            endif()
        endif()
        if (NOT QT_LRELEASE)
            find_program(QT_LRELEASE NAMES lrelease lrelease-qt${QT_VERSION_MAJOR})
            if (QT_LRELEASE)
                message(STATUS "Found lrelease in sys path = ${QT_LRELEASE}")
            else()
                message(STATUS "NOT found lrelease in sys path")
            endif()
        endif()
    else()
        set(QT_LUPDATE "${Qt${QT_VERSION_MAJOR}_LUPDATE_EXECUTABLE}")
        set(QT_LRELEASE "${Qt${QT_VERSION_MAJOR}_LRELEASE_EXECUTABLE}")
    endif()

    # Fail fast when neither lrelease nor lupdate can be resolved to avoid silent misconfiguration
    if (NOT QT_LUPDATE)
        message(FATAL_ERROR "Could not find Qt lupdate tool. Please install Qt translation tools or set QT_TRANSLATION_TOOLS_PREFIX.")
    endif()
    if (NOT QT_LRELEASE)
        message(FATAL_ERROR "Could not find Qt lrelease tool. Please install Qt translation tools or set QT_TRANSLATION_TOOLS_PREFIX.")
    endif()

    message(STATUS "Find and set QT_LUPDATE = ${QT_LUPDATE}, QT_LRELEASE = ${QT_LRELEASE}")
else()
    # 如果找到， 直接使用
    message(STATUS "Use found QT_LUPDATE = ${QT_LUPDATE}, QT_LRELEASE = ${QT_LRELEASE}")
endif()

function(TRANSLATION_GENERATE QMS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: TRANSLATION_GENERATE() called without any .ts path")
    return()
  endif()

  # Explicitly validate or set QT_LUPDATE/QT_LRELEASE at the start of the function
  # to avoid surprising behavior when the function is reused in different contexts
  if (NOT QT_LUPDATE OR NOT QT_LRELEASE)
    message(FATAL_ERROR "TRANSLATION_GENERATE: Qt translation tools not found. QT_LUPDATE=${QT_LUPDATE}, QT_LRELEASE=${QT_LRELEASE}")
  endif()

  message(STATUS "Found lrelease: ${QT_LRELEASE}")
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
          # COMMAND ${QT_LUPDATE} ${CMAKE_SOURCE_DIR} -ts ${TSFIL}
          COMMAND ${QT_LRELEASE} ${TSFIL} -qm ${QMFIL}
          DEPENDS ${TSFIL}
          COMMENT "Running ${QT_LRELEASE} on ${TSFIL}"
          VERBATIM
      )
  endforeach()

  set_source_files_properties(${${QMS}} PROPERTIES GENERATED TRUE)
  set(${QMS} ${${QMS}} PARENT_SCOPE)
endfunction()
