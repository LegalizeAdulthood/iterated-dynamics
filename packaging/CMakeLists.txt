# SPDX-License-Identifier: GPL-3.0-only
#
if(WIN32)
    include(find_wix)
    if(NOT (CPACK_WIX_CANDLE_EXECUTABLE AND CPACK_WIX_LIGHT_EXECUTABLE))
        message(WARNING "Could not find the WiX executables needed to build an installer; skipping packaging.")
        return()
    endif()
endif()

set(CPACK_PACKAGE_NAME                  "${ID_PROGRAM_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "${ID_PROGRAM_NAME} fractal renderer")
set(CPACK_PACKAGE_VENDOR                "${ID_VENDOR_NAME}")
set(CPACK_PACKAGE_VERSION_MAJOR         ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR         ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH         ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY     ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES            TRUE)
set(CPACK_PACKAGE_DESCRIPTION_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/package_description.txt")
set(CPACK_RESOURCE_FILE_WELCOME         "${CMAKE_CURRENT_SOURCE_DIR}/welcome.txt")
set(CPACK_RESOURCE_FILE_LICENSE         "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README          "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
set(CPACK_PACKAGE_EXECUTABLES           "id" "${ID_PROGRAM_NAME}")

set(ID_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(ID_TRANSLATE_SYSTEM_NAME_Darwin "mac")
set(ID_TRANSLATE_SYSTEM_NAME_Linux "linux")
set(ID_TRANSLATE_SYSTEM_NAME_Windows "win")
set(ID_TRANSLATE_SYSTEM_NAME "${ID_TRANSLATE_SYSTEM_NAME_${CMAKE_SYSTEM_NAME}}")
set(ID_TRANSLATE_PROCESSOR_AMD64 "x64")
set(ID_TRANSLATE_PROCESSOR_ARM64 "arm64")
set(ID_TRANSLATE_PROCESSOR_aarch64 "arm64")
set(ID_TRANSLATE_PROCESSOR_arm64 "arm64")
set(ID_TRANSLATE_PROCESSOR_x86_64 "x64")
set(ID_TRANSLATE_PROCESSOR "${ID_TRANSLATE_PROCESSOR_${CMAKE_SYSTEM_PROCESSOR}}")
if(NOT ID_TRANSLATE_SYSTEM_NAME)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" ID_TRANSLATE_SYSTEM_NAME)
endif()
if(NOT ID_TRANSLATE_PROCESSOR)
    set(ID_TRANSLATE_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(ID_PACKAGE_BASE_NAME
    "${ID_PROGRAM_NAME}-${ID_PACKAGE_VERSION}-${ID_TRANSLATE_SYSTEM_NAME}-${ID_TRANSLATE_PROCESSOR}")
set(CPACK_PACKAGE_FILE_NAME "${ID_PACKAGE_BASE_NAME}")

if(WIN32)
    set(CPACK_GENERATOR "ZIP" "WIX")

    install(FILES "github.ico" DESTINATION .)

    set(CPACK_PACKAGE_ICON              "${CMAKE_SOURCE_DIR}/win32/Id.ico")
    set(CPACK_WIX_UPGRADE_GUID          "0E9EE50A-71DB-4723-BC37-CC831B7A7EF7")
    # Change the product GUID every time the version is changed
    set(CPACK_WIX_PRODUCT_GUID          "035D206B-A3CD-521D-A1C9-1A8868A89CE5")
    set(CPACK_WIX_PRODUCT_ICON          "${CMAKE_SOURCE_DIR}/win32/Id.ico")
    set(CPACK_WIX_UI_BANNER             "${CMAKE_CURRENT_SOURCE_DIR}/banner.png")
    set(CPACK_WIX_UI_DIALOG             "${CMAKE_CURRENT_SOURCE_DIR}/dialog.png")
    set(CPACK_WIX_LICENSE_RTF           "${CMAKE_CURRENT_SOURCE_DIR}/license.rtf")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER   "${ID_PROGRAM_NAME}")
    set(CPACK_WIX_EXTENSIONS            "WiXUtilExtension")
    set(CPACK_WIX_EXTRA_SOURCES         "${CMAKE_CURRENT_SOURCE_DIR}/shortcuts.xml")
    set(CPACK_WIX_PATCH_FILE            "${CMAKE_CURRENT_SOURCE_DIR}/patch.xml")
    set(CPACK_WIX_INSTALL_SCOPE         "perMachine")
    
    # Add a custom target to build the Burn bootstrapper after the MSI is built
    add_custom_target(setup
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bundle/downloads
        COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_SOURCE_DIR}/license.rtf 
            ${CMAKE_CURRENT_BINARY_DIR}/bundle/
        COMMAND ${CPACK_WIX_CANDLE_EXECUTABLE}
            "-dProjectDir=$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>"
            "-dID_PROGRAM_NAME=${ID_PROGRAM_NAME}"
            "-dID_PACKAGE_FILE=$<SHELL_PATH:${CMAKE_BINARY_DIR}/${ID_PACKAGE_BASE_NAME}.msi>"
            "-dID_VENDOR_NAME=${ID_VENDOR_NAME}"
            "-dVERSION=${ID_PACKAGE_VERSION}"
            -ext WixBalExtension
            -ext WixUtilExtension
            -out ${CMAKE_CURRENT_BINARY_DIR}/bundle/Bundle.wixobj
            ${CMAKE_CURRENT_SOURCE_DIR}/Bundle.wxs
        COMMAND ${CPACK_WIX_LIGHT_EXECUTABLE}
            -ext WixBalExtension 
            -ext WixUtilExtension
            -out ${CMAKE_BINARY_DIR}/${ID_PACKAGE_BASE_NAME}-Setup.exe
            -b downloads=${CMAKE_CURRENT_BINARY_DIR}/bundle/downloads
            ${CMAKE_CURRENT_BINARY_DIR}/bundle/Bundle.wixobj
        DEPENDS package
        COMMENT "Building Burn bootstrapper with VC++ runtime installer"
        VERBATIM
        SOURCES Bundle.wxs
    )

    # Download the VC runtime for inclusion in the bundle
    add_custom_command(
        TARGET setup PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bundle/downloads
        COMMAND curl -L
            -o "${CMAKE_CURRENT_BINARY_DIR}/bundle/downloads/vc_redist.x64.exe"
            "https://aka.ms/vs/17/release/vc_redist.x64.exe"
        COMMENT "Checking for VC++ Runtime redistributable"
    )
else()
    set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
