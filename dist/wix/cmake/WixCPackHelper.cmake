# WixCPackHelper.cmake
# WiX Toolset CPack integration helper functions

include(CMakeParseArguments)

# Find WiX tools
function(find_wix_tools)
    # Try standard installation paths
    set(WIX_SEARCH_PATHS
        "$ENV{WIX}/bin"
        "C:/Program Files/WiX Toolset v6.0/bin"
        "C:/Program Files (x86)/WiX Toolset v6.0/bin"
        "C:/WixTools/bin"
    )

    find_program(WIX_CANDLE_EXE
        NAMES candle candle.exe
        PATHS ${WIX_SEARCH_PATHS}
        DOC "WiX Candle compiler"
    )

    find_program(WIX_LIGHT_EXE
        NAMES light light.exe
        PATHS ${WIX_SEARCH_PATHS}
        DOC "WiX Light linker"
    )

    if(NOT WIX_CANDLE_EXE OR NOT WIX_LIGHT_EXE)
        message(WARNING "WiX Toolset not found. Please install WiX from https://wixtoolset.org/")
    else()
        message(STATUS "WiX Candle: ${WIX_CANDLE_EXE}")
        message(STATUS "WiX Light: ${WIX_LIGHT_EXE}")
    endif()
endfunction()

# Add WiX MSI build target
function(add_wix_msi_target target_name)
    cmake_parse_arguments(WIX_ARGS
        ""
        "PROJ_NAME;PRODUCT_NAME;PRODUCT_ID;UPGRADE_CODE;OUTPUT_DIR"
        "SOURCES;FRAGMENTS;LANG_FILES"
        ${ARGN})

    # Validate required parameters
    if(NOT WIX_ARGS_PROJ_NAME OR NOT WIX_ARGS_PRODUCT_NAME OR NOT WIX_ARGS_PRODUCT_ID)
        message(FATAL_ERROR "add_wix_msi_target requires PROJ_NAME, PRODUCT_NAME, and PRODUCT_ID")
    endif()

    # Set default output directory
    if(NOT WIX_ARGS_OUTPUT_DIR)
        set(WIX_ARGS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/installer-wix")
    endif()

    # Create WiX output directory
    set(WIX_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wix")
    file(MAKE_DIRECTORY "${WIX_OUTPUT_DIR}")

    # Configure WiX source file templates
    set(CONFIGURED_SOURCES "")
    foreach(source IN LISTS WIX_ARGS_SOURCES)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
            "${WIX_OUTPUT_DIR}/${source}.wxs"
            @ONLY)
        list(APPEND CONFIGURED_SOURCES "${WIX_OUTPUT_DIR}/${source}.wxs")
    endforeach()

    # Configure fragment files
    foreach(frag IN LISTS WIX_ARGS_FRAGMENTS)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${frag}"
            "${WIX_OUTPUT_DIR}/${frag}.wxs"
            @ONLY)
        list(APPEND CONFIGURED_SOURCES "${WIX_OUTPUT_DIR}/${frag}.wxs")
    endforeach()

    # Copy language files
    set(CONFIGURED_LANGS "")
    foreach(lang IN LISTS WIX_ARGS_LANG_FILES)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${lang}"
            "${WIX_OUTPUT_DIR}/${lang}"
            COPYONLY)
        list(APPEND CONFIGURED_LANGS "${WIX_OUTPUT_DIR}/${lang}")
    endforeach()

    # Create targets for each architecture
    foreach(arch x64 x86)
        set(wix_platform ${arch})
        set(arch_suffix ${arch})

        # Determine OpenSSL architecture suffix
        if(arch STREQUAL "x64")
            set(openssl_arch "x64")
        else()
            set(openssl_arch "x86")
        endif()

        # Candle compile target
        set(WIXOBJ_FILE "${WIX_OUTPUT_DIR}/${target_name}_${arch}.wixobj")
        set(MSI_OUTPUT "${WIX_ARGS_OUTPUT_DIR}/${WIX_ARGS_PRODUCT_NAME}-${APP_VERSION}-${arch}.msi")

        add_custom_command(
            OUTPUT "${WIXOBJ_FILE}"
            COMMAND ${WIX_CANDLE_EXE}
                -arch ${wix_platform}
                -dAppVersion="${APP_VERSION}"
                -dPlatform="${wix_platform}"
                -dOpenSSLArch="${openssl_arch}"
                -dProjName="${WIX_ARGS_PROJ_NAME}"
                -dProductName="${WIX_ARGS_PRODUCT_NAME}"
                -dProductId="${WIX_ARGS_PRODUCT_ID}"
                -dUpgradeCode="${WIX_ARGS_UPGRADE_CODE}"
                -ext WixUtilExtension
                -ext WixUIExtension
                -out "${WIX_OUTPUT_DIR}/"
                ${CONFIGURED_SOURCES}
            DEPENDS ${CONFIGURED_SOURCES} ${CONFIGURED_LANGS}
            COMMENT "Compiling WiX sources for ${arch}"
        )

        # Light link target
        add_custom_target(
            "${target_name}_wix_${arch}"
            ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${WIX_ARGS_OUTPUT_DIR}"
            COMMAND ${WIX_LIGHT_EXE}
                -ext WixUtilExtension
                -ext WixUIExtension
                -loc "${WIX_OUTPUT_DIR}/en-us.wxl"
                -loc "${WIX_OUTPUT_DIR}/zh-cn.wxl"
                -out "${MSI_OUTPUT}"
                "${WIXOBJ_FILE}"
            DEPENDS "${WIXOBJ_FILE}"
            COMMENT "Linking WiX MSI for ${arch}"
        )

        # Add dependency: main executable must be built first
        add_dependencies("${target_name}_wix_${arch}" ${target_name})

        message(STATUS "Added WiX target: ${target_name}_wix_${arch} -> ${MSI_OUTPUT}")
    endforeach()
endfunction()
