# SPDX-License-Identifier: GPL-3.0-only
#
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

if(WIN32)
    include(find_wix)
    set(CPACK_GENERATOR ZIP WIX)

    install(FILES "github.ico" DESTINATION .)

    set(CPACK_PACKAGE_ICON              "${CMAKE_SOURCE_DIR}/win32/Id.ico")
    set(CPACK_WIX_UPGRADE_GUID          "0E9EE50A-71DB-4723-BC37-CC831B7A7EF7")
    # Change the product GUID every time the version is changed
    set(CPACK_WIX_PRODUCT_GUID          "7413BC9A-B404-4061-A528-A77E0ADDFAE7")
    set(CPACK_WIX_PRODUCT_ICON          "${CMAKE_SOURCE_DIR}/win32/Id.ico")
    set(CPACK_WIX_UI_BANNER             "${CMAKE_CURRENT_SOURCE_DIR}/banner.png")
    set(CPACK_WIX_UI_DIALOG             "${CMAKE_CURRENT_SOURCE_DIR}/dialog.png")
    set(CPACK_WIX_LICENSE_RTF           "${CMAKE_CURRENT_SOURCE_DIR}/license.rtf")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER   "${ID_PROGRAM_NAME}")
    set(CPACK_WIX_EXTENSIONS            "WiXUtilExtension")
    set(CPACK_WIX_EXTRA_SOURCES         "${CMAKE_CURRENT_SOURCE_DIR}/shortcuts.xml")
    set(CPACK_WIX_PATCH_FILE            "${CMAKE_CURRENT_SOURCE_DIR}/patch.xml")
    set(CPACK_WIX_INSTALL_SCOPE         "perMachine")
    
    # Define the version for WiX variables in the Bundle.wxs
    set(VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    
    # Add a custom target to build the Burn bootstrapper after the MSI is built
    add_custom_target(setup
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bundle/downloads
        COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_SOURCE_DIR}/license.rtf 
            ${CMAKE_CURRENT_BINARY_DIR}/bundle/
        COMMAND ${CPACK_WIX_CANDLE_EXECUTABLE}
            "-dProjectDir=$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>"
            "-dID_PROGRAM_NAME=${ID_PROGRAM_NAME}"
            "-dID_PACKAGE_FILE=$<SHELL_PATH:${CMAKE_BINARY_DIR}/${ID_PROGRAM_NAME}-${VERSION}-win64.msi>"
            "-dID_VENDOR_NAME=${ID_VENDOR_NAME}"
            "-dVERSION=${VERSION}"
            -ext WixBalExtension
            -ext WixUtilExtension
            -out ${CMAKE_CURRENT_BINARY_DIR}/bundle/Bundle.wixobj
            ${CMAKE_CURRENT_SOURCE_DIR}/Bundle.wxs
        COMMAND ${CPACK_WIX_LIGHT_EXECUTABLE}
            -ext WixBalExtension 
            -ext WixUtilExtension
            -out ${CMAKE_BINARY_DIR}/${ID_PROGRAM_NAME}-${VERSION}-Setup.exe
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
