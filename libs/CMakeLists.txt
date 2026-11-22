# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
add_subdirectory(algos)
add_subdirectory(config)
if(("id" IN_LIST VCPKG_MANIFEST_FEATURES) OR ("wx" IN_LIST VCPKG_MANIFEST_FEATURES) OR NOT VCPKG_MANIFEST_FEATURES)
    add_subdirectory(core)
    add_subdirectory(ui)
endif()
