if(NOT TARGET cppserver)

  # Module flag, disable build test and samples
  set(CPPSERVER_MODULE Y)

  # Module subdirectory
  add_subdirectory("${PROJECT_SOURCE_DIR}/3rdparty/CppServer" cppserver)

endif()
