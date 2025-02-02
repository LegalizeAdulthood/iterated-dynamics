# SPDX-License-Identifier: GPL-3.0-only
#
set(image_files mandel.png)
foreach(image ${image_files})
    configure_file(${image} ${image} COPYONLY)
endforeach()
