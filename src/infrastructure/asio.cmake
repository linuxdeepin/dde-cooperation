if(NOT TARGET asio)

  # Module subdirectory
  set(ASIO_DIR "${PROJECT_SOURCE_DIR}/3rdparty/asio")

  # import cmake from asio source directory
  add_subdirectory("${ASIO_DIR}" asio)
  include_directories(${ASIO_DIR}/include)

  # # Module library
  # file(GLOB SOURCE_FILES "${ASIO_DIR}/src/*.cpp")
  # add_library(asio ${SOURCE_FILES})
  # if(MSVC)
  #   set_target_properties(asio PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}")
  # else()
  #   set_target_properties(asio PROPERTIES COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-shadow")
  # endif()
  # target_compile_definitions(asio PRIVATE ASIO_STANDALONE ASIO_SEPARATE_COMPILATION)
  # target_include_directories(asio PUBLIC "${ASIO_DIR}/include" PUBLIC ${OPENSSL_INCLUDE_DIR})
  # target_link_libraries(asio ${OPENSSL_LIBRARIES})

  # if(FPIC)
  #   set_target_properties(asio PROPERTIES POSITION_INDEPENDENT_CODE ON)
  # endif()

  # # Module folder
  # set_target_properties(asio PROPERTIES FOLDER "modules/asio")

endif()
