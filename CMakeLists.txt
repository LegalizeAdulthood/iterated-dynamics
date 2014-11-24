# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 2.8.11)
project(iterated-dynamics CXX C)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(OS_DRIVER_SOURCES
        fractint/win32/calcmand.c
        fractint/win32/calmanp5.c
        fractint/win32/d_win32.c
        fractint/win32/d_win32.h
        fractint/win32/d_win32_disk.c
        fractint/win32/d_win32_gdi.c
        fractint/win32/fpu087.c
        fractint/win32/fracsuba.c
        fractint/win32/frame.c
        fractint/win32/frame.h
        fractint/win32/ods.h
        fractint/win32/os_win32.c
        fractint/win32/parsera.c
        fractint/win32/plot.c
        fractint/win32/plot.h
        fractint/win32/resource.h
        fractint/win32/WinText.c)
    set(OS_DRIVER_LIBRARIES
        gdi32.lib user32.lib shlwapi.lib)
    set(HAVE_OS_DEFINITIONS false)
    set(OS_HC_FILES "")
else()
    set(OS_DRIVER_SOURCES
        fractint/unix/calcmand.c
        fractint/unix/d_x11.c
        fractint/unix/fpu087.c
        fractint/unix/fracsuba.c
        fractint/unix/general.c
        fractint/unix/os_unix.c
        fractint/unix/unix.c
        fractint/unix/unixscr.c
        fractint/unix/video.c)
    set(OS_DRIVER_LIBRARIES ncurses X11 m)
    set(OS_DEFINITIONS "XFRACT" "NOBSTRING" "BIG_ANSI_C" "LINUX")
    set(HAVE_OS_DEFINITIONS true)
    set_source_files_properties(fractint/unix/unix.c
        PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")
    set(DISK_VID "")
    set(OS_HC_FILES "fractint/unix/unix.c")
endif()

add_executable(id
    fractint/common/3d.c
    fractint/common/line3d.c
    fractint/common/plot3d.c

    fractint/common/calcfrac.c
	fractint/common/calmanfp.c
    fractint/common/fracsubr.c
    fractint/common/fractalb.c
    fractint/common/fractalp.c
    fractint/common/fractals.c
    fractint/common/frasetup.c
    fractint/common/soi.c
    fractint/common/soi1.c
    fractint/common/testpt.c

    fractint/common/ant.c
    fractint/common/jb.c
    fractint/common/lorenz.c
    fractint/common/lsys.c
    fractint/common/lsysf.c
    fractint/common/miscfrac.c

    fractint/common/cmdfiles.c
    fractint/common/decoder.c
    fractint/common/diskvid.c
    fractint/common/editpal.c
    fractint/common/encoder.c
    fractint/common/evolve.c
    fractint/common/f16.c
    fractint/common/gifview.c
    fractint/common/loadfdos.c
    fractint/common/loadfile.c
    fractint/common/loadmap.c
    fractint/common/parser.c
    fractint/common/parserfp.c
    fractint/common/rotate.c
    fractint/common/slideshw.c
    fractint/common/stereo.c
    fractint/common/tgaview.c

    fractint/common/bigflt.c
    fractint/common/biginit.c
    fractint/common/bignum.c
    fractint/common/bignumc.c
    fractint/common/hcmplx.c
    fractint/common/mpmath_c.c

    fractint/common/drivers.c
    fractint/common/memory.c

    fractint/common/fractint.c
    fractint/common/framain2.c
    fractint/common/help.c
    fractint/common/intro.c
    fractint/common/jiim.c
    fractint/common/miscovl.c
    fractint/common/miscres.c
    fractint/common/prompts1.c
    fractint/common/prompts2.c
    fractint/common/realdos.c
    fractint/common/zoom.c

    fractint/headers/big.h
    fractint/headers/biginit.h
    fractint/headers/cmplx.h
    fractint/headers/drivers.h
    fractint/headers/externs.h
    fractint/headers/fmath.h
    fractint/headers/fractint.h
    fractint/headers/fractype.h
    fractint/headers/helpcom.h
    fractint/headers/lsys.h
    fractint/headers/mpmath.h
    fractint/headers/port.h
    fractint/headers/prototyp.h
    fractint/headers/winprot.h

    fractint/headers/helpdefs.h

    ${OS_DRIVER_SOURCES}
)

source_group("Source Files\\common\\3d" FILES
    fractint/common/3d.c
    fractint/common/line3d.c
    fractint/common/plot3d.c
)
source_group("Source Files\\common\\engine" FILES
    fractint/common/calcfrac.c
    fractint/common/fracsubr.c
    fractint/common/fractalb.c
    fractint/common/fractalp.c
    fractint/common/fractals.c
    fractint/common/frasetup.c
    fractint/common/soi.c
    fractint/common/soi1.c
    fractint/common/testpt.c
)
source_group("Source Files\\common\\fractal specific" FILES
    fractint/common/ant.c
    fractint/common/jb.c
    fractint/common/lorenz.c
    fractint/common/lsys.c
    fractint/common/lsysf.c
    fractint/common/miscfrac.c
)
source_group("Source Files\\common\\i/o" FILES
    fractint/common/cmdfiles.c
    fractint/common/decoder.c
    fractint/common/diskvid.c
    fractint/common/editpal.c
    fractint/common/encoder.c
    fractint/common/evolve.c
    fractint/common/f16.c
    fractint/common/gifview.c
    fractint/common/loadfdos.c
    fractint/common/loadfile.c
    fractint/common/loadmap.c
    fractint/common/parser.c
    fractint/common/parserfp.c
    fractint/common/rotate.c
    fractint/common/slideshw.c
    fractint/common/stereo.c
    fractint/common/tgaview.c
)
source_group("Source Files\\common\\math" FILES
    fractint/common/bigflt.c
    fractint/common/biginit.c
    fractint/common/bignum.c
    fractint/common/bignumc.c
    fractint/common/hcmplx.c
    fractint/common/mpmath_c.c
)
source_group("Source Files\\common\\plumbing" FILES
    fractint/common/drivers.c
    fractint/common/memory.c
)
source_group("Source Files\\common\\ui" FILES
    fractint/common/fractint.c
    fractint/common/framain2.c
    fractint/common/help.c
    fractint/common/intro.c
    fractint/common/jiim.c
    fractint/common/miscovl.c
    fractint/common/miscres.c
    fractint/common/prompts1.c
    fractint/common/prompts2.c
    fractint/common/realdos.c
    fractint/common/zoom.c
)
source_group("Source Files\\Win32" FILES ${OS_DRIVER_SOURCES})

set_source_files_properties(fractint/common/help.c
    PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")

add_executable(hc fractint/dos_help/hc.c ${OS_HC_FILES})
target_include_directories(hc PRIVATE fractint/headers)
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(hc PRIVATE ${OS_DEFINITIONS})
endif()

target_include_directories(id PRIVATE fractint/headers)
target_link_libraries(id ${OS_DRIVER_LIBRARIES})
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(id PRIVATE ${OS_DEFINITIONS})
endif()
add_dependencies(id hc)

set(FRACTINT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fractint)
set(HELP_SRC_DIR ${FRACTINT_DIR}/dos_help)
add_custom_command(
    OUTPUT ${FRACTINT_DIR}/headers/helpdefs.h ${FRACTINT_DIR}/fractint.hlp
    WORKING_DIRECTORY ${HELP_SRC_DIR}
    COMMAND hc /c
    COMMAND ${CMAKE_COMMAND} -E copy helpdefs.h ${FRACTINT_DIR}/headers
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${FRACTINT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS hc
        ${HELP_SRC_DIR}/help.src ${HELP_SRC_DIR}/help2.src
        ${HELP_SRC_DIR}/help3.src ${HELP_SRC_DIR}/help4.src
        ${HELP_SRC_DIR}/help5.src)
