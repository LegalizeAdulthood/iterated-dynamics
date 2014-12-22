# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 2.8.11)
project(iterated-dynamics CXX C)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(OS_DRIVER_SOURCES
        win32/d_win32.cpp
        win32/d_win32_disk.cpp
        win32/d_win32_gdi.cpp
        win32/frame.cpp
        win32/os_win32.cpp
        win32/parsera.cpp
        win32/plot.cpp
        win32/WinText.cpp)
    set(OS_DRIVER_HEADERS
        win32/d_win32.h
        win32/frame.h
        win32/ods.h
        win32/plot.h
        win32/resource.h
        win32/WinText.h)
    set(OS_DRIVER_LIBRARIES
        gdi32.lib user32.lib shlwapi.lib)
    set(HAVE_OS_DEFINITIONS false)
    set(OS_HC_FILES "")
    set(OS_ID_OPTIONS "WIN32")
else()
    set(OS_DRIVER_SOURCES
        unix/d_x11.cpp
        unix/general.cpp
        unix/os_unix.cpp
        unix/unix.cpp
        unix/unixscr.cpp
        unix/video.cpp)
    set(OS_DRIVER_LIBRARIES ncurses X11 m)
    set(OS_DEFINITIONS "XFRACT" "NOBSTRING" "LINUX")
    set(HAVE_OS_DEFINITIONS true)
    set_source_files_properties(unix/unix.cpp
        PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")
    set(DISK_VID "")
    set(OS_HC_FILES "unix/unix.cpp")
    set(OS_ID_OPTIONS "")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|Clang)")
    set(CMAKE_CXX_FLAGS "-std=c++11 -Werror")
endif()

add_executable(id ${OS_ID_OPTIONS}
    common/3d.cpp
    common/line3d.cpp
    common/plot3d.cpp

    common/calcfrac.cpp
    common/calcmand.cpp
    common/calmanfp.cpp
    common/fracsuba.cpp
    common/fracsubr.cpp
    common/fractalb.cpp
    common/fractalp.cpp
    common/fractals.cpp
    common/frasetup.cpp
    common/soi.cpp
    common/soi1.cpp
    common/testpt.cpp

    common/ant.cpp
    common/jb.cpp
    common/lorenz.cpp
    common/lsys.cpp
    common/lsysf.cpp
    common/miscfrac.cpp

    common/cmdfiles.cpp
    common/decoder.cpp
    common/diskvid.cpp
    common/editpal.cpp
    common/encoder.cpp
    common/evolve.cpp
    common/f16.cpp
    common/gifview.cpp
    common/loadfdos.cpp
    common/loadfile.cpp
    common/loadmap.cpp
    common/parser.cpp
    common/parserfp.cpp
    common/rotate.cpp
    common/slideshw.cpp
    common/stereo.cpp

    common/bigflt.cpp
    common/biginit.cpp
    common/bignum.cpp
    common/bignumc.cpp
    common/fpu087.cpp
    common/hcmplx.cpp
    common/mpmath_c.cpp

    common/drivers.cpp
    common/memory.cpp

    common/fractint.cpp
    common/framain2.cpp
    common/help.cpp
    hc/helpcom.cpp
    common/intro.cpp
    common/jiim.cpp
    common/miscovl.cpp
    common/miscres.cpp
    common/prompts1.cpp
    common/prompts2.cpp
    common/realdos.cpp
    common/zoom.cpp

    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/drivers.h
    headers/externs.h
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
    ${OS_DRIVER_HEADERS}
)

source_group("Source Files\\common\\3d" FILES
    common/3d.cpp
    common/line3d.cpp
    common/plot3d.cpp
)
source_group("Source Files\\common\\engine" FILES
    common/calcfrac.cpp
    common/calcmand.cpp
    common/calmanfp.cpp
    common/fracsuba.cpp
    common/fracsubr.cpp
    common/fractalb.cpp
    common/fractalp.cpp
    common/fractals.cpp
    common/frasetup.cpp
    common/soi.cpp
    common/soi1.cpp
    common/testpt.cpp
)
source_group("Source Files\\common\\fractal specific" FILES
    common/ant.cpp
    common/jb.cpp
    common/lorenz.cpp
    common/lsys.cpp
    common/lsysf.cpp
    common/miscfrac.cpp
)
source_group("Source Files\\common\\i/o" FILES
    common/cmdfiles.cpp
    common/decoder.cpp
    common/diskvid.cpp
    common/editpal.cpp
    common/encoder.cpp
    common/evolve.cpp
    common/f16.cpp
    common/gifview.cpp
    common/loadfdos.cpp
    common/loadfile.cpp
    common/loadmap.cpp
    common/parser.cpp
    common/parserfp.cpp
    common/rotate.cpp
    common/slideshw.cpp
    common/stereo.cpp
)
source_group("Source Files\\common\\math" FILES
    common/bigflt.cpp
    common/biginit.cpp
    common/bignum.cpp
    common/bignumc.cpp
    common/fpu087.cpp
    common/hcmplx.cpp
    common/mpmath_c.cpp
)
source_group("Source Files\\common\\plumbing" FILES
    common/drivers.cpp
    common/memory.cpp
)
source_group("Source Files\\common\\ui" FILES
    common/fractint.cpp
    common/framain2.cpp
    common/help.cpp
    hc/helpcom.cpp
    common/intro.cpp
    common/jiim.cpp
    common/miscovl.cpp
    common/miscres.cpp
    common/prompts1.cpp
    common/prompts2.cpp
    common/realdos.cpp
    common/zoom.cpp
)
source_group("Header Files\\Win32" FILES ${OS_DRIVER_HEADERS})
source_group("Source Files\\Win32" FILES ${OS_DRIVER_SOURCES})

set_source_files_properties(common/help.cpp
    PROPERTIES COMPILE_DEFINITIONS SRCDIR=".")

add_executable(hc hc/hc.cpp hc/helpcom.cpp ${OS_HC_FILES})
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

# In debug builds, tell MSVC to:
#   - not warn us about unchecked iterators
#   - not warn us about deprecated CRT functions
#   - not warn us about possibly unsafe CRT functions
#   - store source code location information for memory allocations
if(${MSVC})
    set(MSVC_DEFINITIONS
        "_SCL_SECURE_NO_WARNINGS"
        "_CRT_SECURE_NO_DEPRECATE"
        "_CRT_SECURE_NO_WARNINGS"
        "_CRTDBG_MAP_ALLOC")
    set_target_properties(id PROPERTIES
        COMPILE_DEFINITIONS_DEBUG "${MSVC_DEFINITIONS}")
endif()
