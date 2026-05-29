# SPDX-License-Identifier: GPL-3.0-only
#
function(id_make_test_home name out_var)
    cmake_parse_arguments(ID_TEST_HOME "" "" "PARS" ${ARGN})
    if(ID_TEST_HOME_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${ID_TEST_HOME_UNPARSED_ARGUMENTS}")
    endif()

    set(ID_HOME "${CMAKE_SOURCE_DIR}/home")
    set(home "${CMAKE_CURRENT_BINARY_DIR}/${name}/home")
    file(MAKE_DIRECTORY "${home}")
    file(COPY_FILE "${ID_HOME}/sstools.ini" "${home}/sstools.ini")
    file(COPY_FILE "${ID_HOME}/id.cfg" "${home}/id.cfg")

    if(ID_TEST_HOME_PARS)
        file(MAKE_DIRECTORY "${home}/par")
    endif()
    foreach(par IN LISTS ID_TEST_HOME_PARS)
        get_filename_component(par_name "${par}" NAME)
        if(NOT par STREQUAL par_name)
            message(FATAL_ERROR "PARS entries must be filenames from one parameter library: ${par}")
        endif()
        file(COPY_FILE "${ID_HOME}/par/${par}" "${home}/par/${par}")
    endforeach()

    set(${out_var} "${home}" PARENT_SCOPE)
endfunction()
