cmake_minimum_required (VERSION 3.4)

set (APP_VERSION_MAJOR 0)
set (APP_VERSION_MINOR 6)
set (APP_VERSION_PATCH 0)
set (APP_VERSION_STAGE "release")

#
# Version
#
if (NOT DEFINED APP_VERSION_MAJOR)
    if (DEFINED ENV{APP_VERSION_MAJOR})
        set (APP_VERSION_MAJOR $ENV{APP_VERSION_MAJOR})
    else()
        set (APP_VERSION_MAJOR 1)
    endif()
endif()

if (NOT DEFINED APP_VERSION_MINOR)
    if (DEFINED ENV{APP_VERSION_MINOR})
        set (APP_VERSION_MINOR $ENV{APP_VERSION_MINOR})
    else()
        set (APP_VERSION_MINOR 0)
    endif()
endif()

if (NOT DEFINED APP_VERSION_PATCH)
    if (DEFINED ENV{APP_VERSION_PATCH})
        set (APP_VERSION_PATCH $ENV{APP_VERSION_PATCH})
    else()
        set (APP_VERSION_PATCH 0)
        message (WARNING "version wasn't set. Set to ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
    endif()
endif()

if (DEFINED ENV{BUILD_NUMBER})
    set (APP_BUILD_NUMBER $ENV{BUILD_NUMBER})
else()
    set (APP_BUILD_NUMBER 1)
endif()

if(NOT DEFINED APP_VERSION)
    set (APP_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}-${APP_VERSION_STAGE}")
endif()
message (STATUS "Full version string is '" ${APP_VERSION} "'")

add_definitions (-DAPP_VERSION="${APP_VERSION}")
add_definitions (-DAPP_VERSION_MAJOR=${APP_VERSION_MAJOR})
add_definitions (-DAPP_VERSION_MINOR=${APP_VERSION_MINOR})
add_definitions (-DAPP_VERSION_PATCH=${APP_VERSION_PATCH})
add_definitions (-DAPP_BUILD_NUMBER=${APP_BUILD_NUMBER})
