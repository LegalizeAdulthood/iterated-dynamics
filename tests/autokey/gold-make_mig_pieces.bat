start/wait id batch=yes overwrite=yes @make_mig_pieces.par/make_mig_00
if errorlevel 2 goto oops
start/wait id batch=yes overwrite=yes @make_mig_pieces.par/make_mig_01
if errorlevel 2 goto oops
start/wait id batch=yes overwrite=yes @make_mig_pieces.par/make_mig_10
if errorlevel 2 goto oops
start/wait id batch=yes overwrite=yes @make_mig_pieces.par/make_mig_11
if errorlevel 2 goto oops
start/wait id makemig=2/2
:oops
