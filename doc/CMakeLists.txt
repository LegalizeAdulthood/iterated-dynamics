set(ID_DOC_INSTALL_DIR doc)

function(id_install_doc)
    cmake_parse_arguments(id_install "" "" "FILES" ${ARGV})
    install(FILES ${id_install_FILES} DESTINATION ${ID_DOC_INSTALL_DIR})
endfunction()

id_install_doc(FILES known-bugs.txt debug-flags.txt)
