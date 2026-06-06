# SPDX-License-Identifier: GPL-3.0-only
#

set(ID_EXTRA_ARGS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ID_EXTRA_ARGS "-geometry" "+100+100")
endif()
