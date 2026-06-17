# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#

function(copy_image_failure_artifact artifact_dir artifact)
    if("${artifact_dir}" STREQUAL "" OR "${artifact}" STREQUAL "")
        return()
    endif()
    if(NOT EXISTS "${artifact}")
        return()
    endif()
    file(MAKE_DIRECTORY "${artifact_dir}")
    get_filename_component(artifact_name "${artifact}" NAME)
    file(COPY_FILE "${artifact}" "${artifact_dir}/${artifact_name}"
        ONLY_IF_DIFFERENT)
endfunction()

function(copy_image_failure_artifacts artifact_dir)
    foreach(artifact IN LISTS ARGN)
        copy_image_failure_artifact("${artifact_dir}" "${artifact}")
    endforeach()
endfunction()
