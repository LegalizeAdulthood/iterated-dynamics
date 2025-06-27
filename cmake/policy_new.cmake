# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
function(policy_new policy)
    if(POLICY ${policy})
        cmake_policy(SET ${policy} NEW)
    endif()
endfunction()
