set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(target_folder target folder)
    set_target_properties("${target}" PROPERTIES FOLDER "${folder}")
endfunction()
