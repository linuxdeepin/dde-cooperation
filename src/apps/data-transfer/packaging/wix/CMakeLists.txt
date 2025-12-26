# data-transfer WiX packaging configuration

set(PROJ_NAME "deepin-data-transfer")
set(PKG_NAME "data-transfer")

if(WIN32)
    # Product information
    set(PRODUCT_INFO "Deepin data transfer")
    set(PRODUCT_NAME "deepin-datatransfer")
    set(PRODUCT_ID "636B356F-47E1-491D-B66E-B254233FFCB1")
    set(UPGRADE_CODE "636B356F-47E1-491D-B66E-B254233FFCB1")

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
