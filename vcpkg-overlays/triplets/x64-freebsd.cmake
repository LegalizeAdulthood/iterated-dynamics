# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME FreeBSD)

# FreeBSD provides iconv in libc; GNU libiconv breaks libxml2 tools.
set(X_VCPKG_BUILD_GNU_LIBICONV 0)
