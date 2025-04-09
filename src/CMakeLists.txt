# some base common libraries
add_subdirectory(infrastructure)

# share libraries
add_subdirectory(lib)

add_subdirectory(apps)

if(ENABLE_SLOTIPC)
    # compat v20 daemon app
    add_subdirectory(compat)
endif()
