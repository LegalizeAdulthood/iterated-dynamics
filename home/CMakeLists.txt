set(ID_HOME_INSTALL_DIR .)

function(id_install_home)
    cmake_parse_arguments(id_install "" "" "FILES" ${ARGV})
    install(FILES ${id_install_FILES} DESTINATION ${ID_HOME_INSTALL_DIR})
endfunction()

function(id_install_shortcut file shortcut)
    set_property(INSTALL ${file} PROPERTY CPACK_START_MENU_SHORTCUTS ${shortcut})
endfunction()

function(id_install_subdirectory dir)
    set(ID_HOME_INSTALL_DIR "${ID_HOME_INSTALL_DIR}/${dir}")
    add_subdirectory(${dir})
endfunction()

if(WIN32)
    id_install_home(FILES demo.bat)
    id_install_shortcut(demo.bat "Demo Script")
endif()

id_install_home(FILES id.cfg sstools.ini)

id_install_subdirectory(extra)
id_install_subdirectory(formulas)
id_install_subdirectory(ifs)
id_install_subdirectory(key)
id_install_subdirectory(lsystem)
id_install_subdirectory(maps)
id_install_subdirectory(pars)
