# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
function(dump_var name)
    message(STATUS "${name}=${${name}}")
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/ImageTestArtifacts.cmake")

set(DEBUG ON)
if(DEBUG)
    set(COMMAND_ECHO "STDOUT")
else()
    set(COMMAND_ECHO "NONE")
endif()

if(DEBUG)
    dump_var(ID)
    dump_var(IMAGE_COMPARE)
    dump_var(GOLD_IMAGE)
    dump_var(DIFF_IMAGE)
    dump_var(TEST_ARTIFACT_DIR)
    dump_var(ID_EXTRA_ARGS)
endif()

set(STOP_MESSAGE_FILE "debug/stopmsg.txt")
get_filename_component(TEST_HOME "." ABSOLUTE)
set(POT_FILE "image/potntial.pot")
set(TGA_FILE "image/3dlook-targa.tga")

file(MAKE_DIRECTORY "image")
file(REMOVE "${TGA_FILE}")
file(REMOVE "${DIFF_IMAGE}")
if(NOT EXISTS "${POT_FILE}")
    message(FATAL_ERROR "Potential file '${POT_FILE}' does not exist.")
endif()

execute_process(
    COMMAND "${ID}" ${ID_EXTRA_ARGS} "video=F6" "askvideo=no" "parmfile=radar.par"
        "sound=off" "librarydirs=." "savedir=." "overwrite=yes" "exitnoask=yes"
        "lightname=3dlook-targa" "autokeyname=3dlook_targa.key" "autokey=play"
        "@radar/3dlook"
    RESULT_VARIABLE ID_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(ID_RESULT)
    if(EXISTS "${STOP_MESSAGE_FILE}")
        file(READ "${STOP_MESSAGE_FILE}" STOP_MESSAGE)
        message(STATUS "${STOP_MESSAGE_FILE}:\n${STOP_MESSAGE}")
    endif()
    message(FATAL_ERROR "3dlook execution failed with result ${ID_RESULT}.")
endif()
if(NOT EXISTS "${TGA_FILE}")
    message(FATAL_ERROR "Targa file '${TGA_FILE}' does not exist.")
endif()

execute_process(
    COMMAND "${IMAGE_COMPARE}" "--diff-image" "${DIFF_IMAGE}"
        "${GOLD_IMAGE}" "${TGA_FILE}"
    RESULT_VARIABLE IMAGE_COMPARE_RESULT
    COMMAND_ECHO ${COMMAND_ECHO})
if(IMAGE_COMPARE_RESULT)
    copy_image_failure_artifacts("${TEST_ARTIFACT_DIR}"
        "${GOLD_IMAGE}"
        "${TGA_FILE}"
        "${DIFF_IMAGE}")
    message(FATAL_ERROR "Image comparison failed with result ${IMAGE_COMPARE_RESULT}.")
endif()

file(MAKE_DIRECTORY image/blocked.tga)
file(WRITE image/invalid.tga "not a supported Targa file")
foreach(output IN ITEMS blocked.tga invalid.tga)
    file(REMOVE debug/stopmsg.txt image/should-not-save.gif)
    set(OVERLAY no)
    if(output STREQUAL "invalid.tga")
        set(OVERLAY yes)
    endif()
    execute_process(
        COMMAND "${ID}" ${ID_EXTRA_ARGS} "video=F6" "askvideo=no"
            "sound=off" "librarydirs=." "savedir=." "overwrite=yes"
            "@radar/3dlook" "lightname=${output}" "targa_overlay=${OVERLAY}"
            "batch=yes" "savename=should-not-save.gif"
        RESULT_VARIABLE RESULT TIMEOUT 30 COMMAND_ECHO STDOUT)
    if(NOT RESULT STREQUAL "2" OR EXISTS image/should-not-save.gif)
        message(FATAL_ERROR "Rejected 3D output '${output}' did not fail without saving: ${RESULT}")
    endif()
    file(READ debug/stopmsg.txt DIAGNOSTIC)
    if(output STREQUAL "blocked.tga")
        set(EXPECTED "OOPS, couldn't open")
    else()
        set(EXPECTED "OOPS, can't handle this type of file.")
        file(READ image/invalid.tga ORIGINAL)
        if(NOT ORIGINAL STREQUAL "not a supported Targa file")
            message(FATAL_ERROR "Failed overlay changed the original image")
        endif()
    endif()
    string(FIND "${DIAGNOSTIC}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "Missing 3D output diagnostic: ${DIAGNOSTIC}")
    endif()
endforeach()
