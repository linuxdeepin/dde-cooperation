
# it causes barrier build failed on MSVC, so only enable it for src
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# some base common libraries
add_subdirectory(infrastructure)

# share libraries
add_subdirectory(lib)

add_subdirectory(apps)

if(ENABLE_SLOTIPC)
    # compat v20 daemon app
    add_subdirectory(compat)
endif()
