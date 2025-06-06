if(NOT TARGET fmt)

  # Module subdirectory
  set(FMT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/fmt")

  # Module library
  file(GLOB SOURCE_FILES "${FMT_DIR}/src/*.cc")
  list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/fmt.cc")
  add_library(fmt STATIC ${SOURCE_FILES})
  if(MSVC)
    # C4702: unreachable code
    set_target_properties(fmt PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} /wd4702")
  else()
    set_target_properties(fmt PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
  endif()
  target_include_directories(fmt PUBLIC "${FMT_DIR}/include")
  target_link_libraries(fmt)

  if(FPIC)
    set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  # Module folder
  set_target_properties(fmt PROPERTIES FOLDER "modules/fmt")

endif()
