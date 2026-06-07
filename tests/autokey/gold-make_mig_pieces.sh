#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#

id_bin='ID_EXECUTABLE'
script_dir=$(dirname "$0")
cd "${script_dir}" || exit 2

"${id_bin}" batch=yes overwrite=yes savedir=. '@make_mig_pieces.par/make_mig_00'
status=$?
if [ "${status}" -ge 2 ]; then
    exit "${status}"
fi
"${id_bin}" batch=yes overwrite=yes savedir=. '@make_mig_pieces.par/make_mig_01'
status=$?
if [ "${status}" -ge 2 ]; then
    exit "${status}"
fi
"${id_bin}" batch=yes overwrite=yes savedir=. '@make_mig_pieces.par/make_mig_10'
status=$?
if [ "${status}" -ge 2 ]; then
    exit "${status}"
fi
"${id_bin}" batch=yes overwrite=yes savedir=. '@make_mig_pieces.par/make_mig_11'
status=$?
if [ "${status}" -ge 2 ]; then
    exit "${status}"
fi
"${id_bin}" savedir=. makemig=2/2
