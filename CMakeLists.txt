# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 2.8.11)
project(iterated-dynamics CXX C)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(OS_DRIVER_SOURCES
        win32/calcmand.c
        win32/calmanp5.c
        win32/d_win32.c
        win32/d_win32.h
        win32/d_win32_disk.c
        win32/d_win32_gdi.c
        win32/fpu087.c
        win32/fracsuba.c
        win32/frame.c
        win32/frame.h
        win32/ods.h
        win32/os_win32.c
        win32/parsera.c
        win32/plot.c
        win32/plot.h
        win32/resource.h
        win32/WinText.c)
    set(OS_DRIVER_LIBRARIES
        gdi32.lib user32.lib shlwapi.lib)
    set(HAVE_OS_DEFINITIONS false)
    set(OS_HC_FILES "")
else()
    set(OS_DRIVER_SOURCES
        unix/calcmand.c
        unix/d_x11.c
        unix/fpu087.c
        unix/fracsuba.c
        unix/general.c
        unix/os_unix.c
        unix/unix.c
        unix/unixscr.c
        unix/video.c)
    set(OS_DRIVER_LIBRARIES ncurses X11 m)
    set(OS_DEFINITIONS "XFRACT" "NOBSTRING" "BIG_ANSI_C" "LINUX")
    set(HAVE_OS_DEFINITIONS true)
    set_source_files_properties(unix/unix.c
        PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")
    set(DISK_VID "")
    set(OS_HC_FILES "unix/unix.c")
endif()

add_executable(id
    common/3d.c
    common/line3d.c
    common/plot3d.c

    common/calcfrac.c
    common/calmanfp.c
    common/fracsubr.c
    common/fractalb.c
    common/fractalp.c
    common/fractals.c
    common/frasetup.c
    common/soi.c
    common/soi1.c
    common/testpt.c

    common/ant.c
    common/jb.c
    common/lorenz.c
    common/lsys.c
    common/lsysf.c
    common/miscfrac.c

    common/cmdfiles.c
    common/decoder.c
    common/diskvid.c
    common/editpal.c
    common/encoder.c
    common/evolve.c
    common/f16.c
    common/gifview.c
    common/loadfdos.c
    common/loadfile.c
    common/loadmap.c
    common/parser.c
    common/parserfp.c
    common/rotate.c
    common/slideshw.c
    common/stereo.c
    common/tgaview.c

    common/bigflt.c
    common/biginit.c
    common/bignum.c
    common/bignumc.c
    common/hcmplx.c
    common/mpmath_c.c

    common/drivers.c
    common/memory.c

    common/fractint.c
    common/framain2.c
    common/help.c
    common/intro.c
    common/jiim.c
    common/miscovl.c
    common/miscres.c
    common/prompts1.c
    common/prompts2.c
    common/realdos.c
    common/zoom.c

    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/drivers.h
    headers/externs.h
    headers/fmath.h
    headers/fractint.h
    headers/fractype.h
    headers/helpcom.h
    headers/lsys.h
    headers/mpmath.h
    headers/port.h
    headers/prototyp.h
    headers/winprot.h

    headers/helpdefs.h

    ${OS_DRIVER_SOURCES}
)

source_group("Source Files\\common\\3d" FILES
    common/3d.c
    common/line3d.c
    common/plot3d.c
)
source_group("Source Files\\common\\engine" FILES
    common/calcfrac.c
    common/fracsubr.c
    common/fractalb.c
    common/fractalp.c
    common/fractals.c
    common/frasetup.c
    common/soi.c
    common/soi1.c
    common/testpt.c
)
source_group("Source Files\\common\\fractal specific" FILES
    common/ant.c
    common/jb.c
    common/lorenz.c
    common/lsys.c
    common/lsysf.c
    common/miscfrac.c
)
source_group("Source Files\\common\\i/o" FILES
    common/cmdfiles.c
    common/decoder.c
    common/diskvid.c
    common/editpal.c
    common/encoder.c
    common/evolve.c
    common/f16.c
    common/gifview.c
    common/loadfdos.c
    common/loadfile.c
    common/loadmap.c
    common/parser.c
    common/parserfp.c
    common/rotate.c
    common/slideshw.c
    common/stereo.c
    common/tgaview.c
)
source_group("Source Files\\common\\math" FILES
    common/bigflt.c
    common/biginit.c
    common/bignum.c
    common/bignumc.c
    common/hcmplx.c
    common/mpmath_c.c
)
source_group("Source Files\\common\\plumbing" FILES
    common/drivers.c
    common/memory.c
)
source_group("Source Files\\common\\ui" FILES
    common/fractint.c
    common/framain2.c
    common/help.c
    common/intro.c
    common/jiim.c
    common/miscovl.c
    common/miscres.c
    common/prompts1.c
    common/prompts2.c
    common/realdos.c
    common/zoom.c
)
source_group("Source Files\\Win32" FILES ${OS_DRIVER_SOURCES})

set_source_files_properties(common/help.c
    PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")

add_executable(hc hc/hc.c ${OS_HC_FILES})
target_include_directories(hc PRIVATE headers)
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(hc PRIVATE ${OS_DEFINITIONS})
endif()

target_include_directories(id PRIVATE headers)
target_link_libraries(id ${OS_DRIVER_LIBRARIES})
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(id PRIVATE ${OS_DEFINITIONS})
endif()
add_dependencies(id hc)

set(ID_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HELP_SRC_DIR ${ID_DIR}/hc)
add_custom_command(
    OUTPUT ${ID_DIR}/headers/helpdefs.h ${ID_DIR}/fractint.hlp
    WORKING_DIRECTORY ${HELP_SRC_DIR}
    COMMAND hc /c
    COMMAND ${CMAKE_COMMAND} -E copy helpdefs.h ${ID_DIR}/headers
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${ID_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS hc
        ${HELP_SRC_DIR}/help.src ${HELP_SRC_DIR}/help2.src
        ${HELP_SRC_DIR}/help3.src ${HELP_SRC_DIR}/help4.src
        ${HELP_SRC_DIR}/help5.src)
