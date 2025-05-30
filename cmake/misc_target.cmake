# Create a Miscellaneous target to organize files in the IDE
function(misc_target)
    cmake_parse_arguments(ARG "" "" "FILES" ${ARGN})
    add_custom_target(Miscellaneous)
    if(ARG_FILES)
        target_sources(Miscellaneous PRIVATE ${ARG_FILES})
    endif()
endfunction()

# Add files to the Miscellaneous target and group them in the IDE
function(misc_group_sources group)
    cmake_parse_arguments(ARG "" "" "FILES" ${ARGN})
    if(ARG_FILES)
        target_sources(Miscellaneous PRIVATE ${ARG_FILES})
        source_group("${group}" FILES ${ARG_FILES})
    endif()
endfunction()
