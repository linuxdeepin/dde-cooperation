# SPDX-FileCopyrightText: 2024 Uniontech
# SPDX-License-Identifier: MIT

# dde-cooperation Windows WiX packaging using CPack
# This file should be included from the application's CMakeLists.txt

if(WIN32 AND ENABLE_WIX)
    # Architecture detection
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BUILD_ARCHITECTURE "x64")
    else()
        set(BUILD_ARCHITECTURE "x86")
    endif()

    # Set CPack generators - use 7Z as default fallback
    list(APPEND CPACK_GENERATOR "7Z")

    # Check for WiX 4+ (wix command)
    find_program(WIX_APP wix)
    if(NOT "${WIX_APP}" STREQUAL "")
        set(CPACK_WIX_VERSION 4)
        set(CPACK_WIX_ARCHITECTURE ${BUILD_ARCHITECTURE})
        list(APPEND CPACK_GENERATOR "WIX")
        message(STATUS "WiX 4+ found for dde-cooperation, enabling MSI packaging")
    else()
        message(WARNING "WiX Toolset 4+ not found. Only 7Z package will be generated. Install from https://wixtoolset.org/")
    endif()

    # CPack package configuration
    set(CPACK_PACKAGE_NAME "deepin-cooperation")
    set(CPACK_PACKAGE_VENDOR "Uniontech, Inc.")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Deepin Cooperation - Cross-device collaboration tool")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../../README.md")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.deepin.org/")
    set(CPACK_PACKAGE_CONTACT "support@deepin.org")

    # Architecture string for package naming
    set(OS_STRING "win-${BUILD_ARCHITECTURE}")
    set(CPACK_PACKAGE_FILE_NAME "deepin-cooperation-${APP_VERSION}-${OS_STRING}")

    # Wix UI configuration
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Deepin Cooperation")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "DeepinCooperation")
    set(CPACK_PACKAGE_EXECUTABLES "dde-cooperation" "Deepin Cooperation")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Deepin Cooperation")

    # Wix upgrade GUID - must be unique per application
    set(CPACK_WIX_UPGRADE_GUID "A3AE65FC-2431-49AE-9A9C-87D3DBC2B7A4")

    # Wix UI images (optional - will use defaults if not found)
    if(EXISTS "${CMAKE_SOURCE_DIR}/dist/wix/wix-banner.png")
        set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/dist/wix/wix-banner.png")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/dist/inno/res/banner.jpg")
        set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/dist/inno/res/banner.jpg")
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/dist/wix/wix-dialog.png")
        set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/dist/wix/wix-dialog.png")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/dist/inno/res/dialog.jpg")
        set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/dist/inno/res/dialog.jpg")
    endif()

    # Icon
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../res/win/dde-cooperation.ico")
        set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../../res/win/dde-cooperation.ico")
    endif()

    # License file
    if(EXISTS "${CMAKE_SOURCE_DIR}/dist/License.rtf")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/dist/License.rtf")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    endif()

    # Wix extensions for additional features
    list(APPEND CPACK_WIX_EXTENSIONS "WixToolset.Util.wixext")
    list(APPEND CPACK_WIX_EXTENSIONS "WixToolset.UI.wixext")

    # Wix custom XML namespaces
    list(APPEND CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util")

    # ARP (Add/Remove Programs) properties
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://www.deepin.org/")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://www.deepin.org/cooperation")
    set(CPACK_WIX_PROPERTY_ARPSIZE_approximate "30000")
    set(CPACK_WIX_PROPERTY_ARPNOREPAIR "1")  # Disable repair option
    set(CPACK_WIX_PROPERTY_ARPNOMODIFY "1")  # Disable modify option

    # Desktop shortcut creation
    set(CPACK_WIX_CMAKE_PROJECT_PATCH_FILE "${CMAKE_CURRENT_LIST_DIR}/fragments/shortcuts.xml")

    message(STATUS "Configured CPack WiX for dde-cooperation (${BUILD_ARCHITECTURE})")
endif()
