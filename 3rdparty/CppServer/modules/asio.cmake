if(NOT TARGET asio)

  if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
  endif()

  # Module library
  file(GLOB SOURCE_FILES "asio/asio/src/*.cpp")
  add_library(asio STATIC ${SOURCE_FILES})
  if(MSVC)
    set_target_properties(asio PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}")
  else()
    set_target_properties(asio PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
  endif()
  target_compile_definitions(asio PRIVATE ASIO_STANDALONE ASIO_SEPARATE_COMPILATION)
  target_include_directories(asio PUBLIC "asio/asio/include" PUBLIC ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(asio ${OPENSSL_LIBRARIES})

  if(FPIC)
    set_target_properties(asio PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  # Module folder
  set_target_properties(asio PROPERTIES FOLDER "modules/asio")

endif()
