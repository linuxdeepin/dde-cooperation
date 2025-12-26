# dde-cooperation WiX packaging configuration

set(PROJ_NAME "dde-cooperation")
set(PKG_NAME "dde-cooperation")

if(WIN32)
    # Product information
    set(PRODUCT_INFO "Deepin cooperation")
    set(PRODUCT_NAME "deepin-cooperation")
    set(PRODUCT_ID "A3AE65FC-2431-49AE-9A9C-87D3DBC2B7A4")
    set(UPGRADE_CODE "A3AE65FC-2431-49AE-9A9C-87D3DBC2B7A4")

    # WiX source files
    set(WIX_SOURCES
        "msi.wxs.in"
    )

    # WiX fragment files
    set(WIX_FRAGMENTS
        "fragments/app_files.wxs.in"
        "fragments/shortcuts.wxs.in"
    )

    # Language files
    set(WIX_LANG_FILES
        "lang/en-us.wxl"
        "lang/zh-cn.wxl"
    )

    # Include WiX helper functions
    include(${CMAKE_SOURCE_DIR}/dist/wix/cmake/WixCPackHelper.cmake)

    # Find WiX tools
    find_wix_tools()

    # Add WiX MSI target if tools are found
    if(WIX_CANDLE_EXE AND WIX_LIGHT_EXE)
        add_wix_msi_target(${PROJ_NAME}
            PROJ_NAME ${PROJ_NAME}
            PRODUCT_NAME ${PRODUCT_NAME}
            PRODUCT_ID ${PRODUCT_ID}
            UPGRADE_CODE ${UPGRADE_CODE}
            OUTPUT_DIR "${CMAKE_BINARY_DIR}/installer-wix/${PKG_NAME}"
            SOURCES ${WIX_SOURCES}
            FRAGMENTS ${WIX_FRAGMENTS}
            LANG_FILES ${WIX_LANG_FILES}
        )

        message(STATUS "WiX packaging enabled for ${PROJ_NAME}")
    else()
        message(WARNING "WiX tools not found, MSI packaging disabled")
    endif()
endif()
