# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

# MPC supplies neither a CMake config nor pkg-config metadata.
find_path(MPC_INCLUDE_DIR NAMES mpc.h)
find_library(MPC_LIBRARY_RELEASE NAMES mpc libmpc
    HINTS "${MPC_INCLUDE_DIR}/../lib"
    NO_DEFAULT_PATH)
if(NOT MPC_LIBRARY_RELEASE)
    find_library(MPC_LIBRARY_RELEASE NAMES mpc libmpc)
endif()
find_library(MPC_LIBRARY_DEBUG NAMES mpc libmpc
    HINTS "${MPC_INCLUDE_DIR}/../debug/lib"
    NO_DEFAULT_PATH)
select_library_configurations(MPC)
mark_as_advanced(MPC_INCLUDE_DIR MPC_LIBRARY_RELEASE MPC_LIBRARY_DEBUG)

find_package_handle_standard_args(MPC REQUIRED_VARS MPC_INCLUDE_DIR MPC_LIBRARY)

if(MPC_FOUND AND NOT TARGET MPC::MPC)
    add_library(MPC::MPC UNKNOWN IMPORTED)
    set_target_properties(MPC::MPC PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MPC_INCLUDE_DIR}")
    foreach(config IN ITEMS RELEASE DEBUG)
        if(MPC_LIBRARY_${config})
            set_property(TARGET MPC::MPC APPEND PROPERTY IMPORTED_CONFIGURATIONS ${config})
            set_target_properties(MPC::MPC PROPERTIES
                IMPORTED_LOCATION_${config} "${MPC_LIBRARY_${config}}")
        endif()
    endforeach()
endif()
