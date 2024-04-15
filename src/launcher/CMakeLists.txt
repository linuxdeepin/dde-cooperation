set(PROJ_NAME "cooperation-launcher")

FILE(GLOB CPP_SRC
    *.h
    *.cpp
)

list(APPEND LINKLIBS cppserver)
list(APPEND LINKLIBS proto)

add_executable(${PROJ_NAME} ${CPP_SRC})

target_link_libraries(${PROJ_NAME} ${LINKLIBS})
list(APPEND INSTALL_TARGETS ${PROJ_NAME})
list(APPEND INSTALL_TARGETS_PDB ${PROJ_NAME})

#install(TARGETS ${PROJ_NAME} DESTINATION bin)
# Install
install(TARGETS ${INSTALL_TARGETS}
        RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin"
        LIBRARY DESTINATION "${PROJECT_SOURCE_DIR}/bin"
        ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/bin")

# Install *.pdb files
if(MSVC)
  foreach(INSTALL_TARGET_PDB ${INSTALL_TARGETS_PDB})
    install(FILES $<TARGET_PDB_FILE:${INSTALL_TARGET_PDB}> DESTINATION "${PROJECT_SOURCE_DIR}/bin")
  endforeach()
endif()
