# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 2.8.11)
project(iterated-dynamics CXX)

set(home_dir "${CMAKE_SOURCE_DIR}/home")

function(set_src_dir src_file)
    set_source_files_properties(${src_file}
        PROPERTIES COMPILE_DEFINITIONS SRCDIR="${home_dir}")
endfunction()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(OS_DRIVER_SOURCES
        win32/d_win32.cpp
        win32/d_win32_disk.cpp
        win32/d_win32_gdi.cpp
        win32/frame.cpp
        win32/os_win32.cpp
        win32/parsera.cpp
        win32/plot.cpp
        win32/WinText.cpp
        win32/fractint.rc)
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
        unix/x11_frame.cpp
        unix/x11_text.cpp
        unix/x11_plot.cpp
        unix/general.cpp
        unix/os_unix.cpp
        unix/unix.cpp
        unix/unixscr.cpp
        unix/video.cpp)
    set(OS_DRIVER_LIBRARIES X11 m)
    set(OS_DEFINITIONS "XFRACT" "NOBSTRING" "LINUX")
    set(HAVE_OS_DEFINITIONS true)
    set_src_dir(unix/unix.cpp)
    set(DISK_VID "")
    set(OS_HC_FILES "unix/unix.cpp")
    set(OS_ID_OPTIONS "")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|Clang)")
    set(CMAKE_CXX_FLAGS "-std=c++11 -Werror")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
endif()

add_executable(id ${OS_ID_OPTIONS}
    headers/os.h

    common/3d.cpp headers/3d.h
    common/line3d.cpp headers/line3d.h
    common/plot3d.cpp headers/plot3d.h

    common/calcfrac.cpp headers/calcfrac.h
    common/calcmand.cpp headers/calcmand.h
    common/calmanfp.cpp headers/calmanfp.h
    common/fracsuba.cpp headers/fracsuba.h
    common/fracsubr.cpp headers/fracsubr.h
    common/fractalb.cpp headers/fractalb.h
    common/fractalp.cpp headers/fractalp.h
    common/fractals.cpp headers/fractals.h
    common/frasetup.cpp headers/frasetup.h
    common/soi.cpp headers/soi.h
    common/soi1.cpp
    common/testpt.cpp headers/testpt.h

    common/ant.cpp headers/ant.h
    common/jb.cpp headers/jb.h
    common/lorenz.cpp headers/lorenz.h
    common/lsys.cpp headers/lsys_fns.h
    common/lsysf.cpp
    common/miscfrac.cpp headers/miscfrac.h

    common/cmdfiles.cpp headers/cmdfiles.h
    common/decoder.cpp headers/decoder.h
    common/diskvid.cpp headers/diskvid.h
    common/editpal.cpp headers/editpal.h
    common/encoder.cpp headers/encoder.h
    common/evolve.cpp headers/evolve.h
    common/gifview.cpp headers/gifview.h
    common/loadfdos.cpp headers/loadfdos.h
    common/loadfile.cpp headers/loadfile.h
    common/loadmap.cpp headers/loadmap.h
    common/parser.cpp headers/parser.h
    common/parserfp.cpp
    common/rotate.cpp headers/rotate.h
    common/slideshw.cpp headers/slideshw.h
    common/stereo.cpp headers/stereo.h

    common/bigflt.cpp
    common/biginit.cpp headers/biginit.h
    common/bignum.cpp
    common/bignumc.cpp
    common/fpu087.cpp headers/fpu087.h
    common/hcmplx.cpp headers/hcmplx.h
    common/mpmath_c.cpp headers/mpmath_c.h

    common/drivers.cpp
    common/memory.cpp headers/memory.h

    common/fractint.cpp
    common/framain2.cpp headers/framain2.h
    common/help.cpp
    hc/helpcom.cpp
    common/intro.cpp headers/intro.h
    common/jiim.cpp headers/jiim.h
    common/miscovl.cpp headers/miscovl.h
    common/miscres.cpp headers/miscres.h
    common/prompts1.cpp headers/prompts1.h
    common/prompts2.cpp headers/prompts2.h
    common/realdos.cpp headers/realdos.h
    common/zoom.cpp headers/zoom.h

    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/drivers.h
    headers/fractint.h
    headers/id_data.h
    headers/fractype.h
    headers/helpcom.h
    headers/lsys.h
    headers/mpmath.h
    headers/newton.h
    headers/port.h
    headers/prototyp.h
    headers/winprot.h

    headers/helpdefs.h

    ${OS_DRIVER_SOURCES}
    ${OS_DRIVER_HEADERS}
)

source_group("Header Files\\common\\3d" FILES
    headers/3d.h
    headers/line3d.h
    headers/plot3d.h
)
source_group("Source Files\\common\\3d" FILES
    common/3d.cpp
    common/line3d.cpp
    common/plot3d.cpp
)
source_group("Header Files\\common\\engine" FILES
    headers/calcfrac.h
    headers/calcmand.h
    headers/calmanfp.h
    headers/fracsuba.h
    headers/fracsubr.h
    headers/fractalb.h
    headers/fractalp.h
    headers/fractals.h
    headers/fractype.h
    headers/frasetup.h
    headers/id_data.h
    headers/soi.h
    headers/testpt.h
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
source_group("Header Files\\common\\fractal specific" FILES
    headers/ant.h
    headers/jb.h
    headers/lorenz.h
    headers/lsys.h
    headers/lsysf.h
    headers/lsys_fns.h
    headers/miscfrac.h
    headers/newton.h
)
source_group("Source Files\\common\\fractal specific" FILES
    common/ant.cpp
    common/jb.cpp
    common/lorenz.cpp
    common/lsys.cpp
    common/lsysf.cpp
    common/miscfrac.cpp
)
source_group("Header Files\\common\\i/o" FILES
    headers/cmdfiles.h
    headers/decoder.h
    headers/diskvid.h
    headers/editpal.h
    headers/encoder.h
    headers/evolve.h
    headers/gifview.h
    headers/loadfdos.h
    headers/loadfile.h
    headers/loadmap.h
    headers/parser.h
    headers/rotate.h
    headers/slideshw.h
    headers/stereo.h
)
source_group("Source Files\\common\\i/o" FILES
    common/cmdfiles.cpp
    common/decoder.cpp
    common/diskvid.cpp
    common/editpal.cpp
    common/encoder.cpp
    common/evolve.cpp
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
source_group("Header Files\\common\\math" FILES
    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/fpu087.h
    headers/hcmplx.h
    headers/mpmath.h
    headers/mpmath_c.h
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
source_group("Header Files\\common\\plumbing" FILES
    headers/drivers.h
    headers/memory.h
)
source_group("Source Files\\common\\plumbing" FILES
    common/drivers.cpp
    common/memory.cpp
)
source_group("Header Files\\common\\ui" FILES
    headers/fractint.h
    headers/framain2.h
    headers/help.h
    headers/helpcom.h
    headers/helpdefs.h
    headers/intro.h
    headers/jiim.h
    headers/miscovl.h
    headers/miscres.h
    headers/prompts1.h
    headers/prompts2.h
    headers/realdos.h
    headers/zoom.h
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

set_src_dir(common/help.cpp)
set_src_dir(common/fractint.cpp)

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
    OUTPUT ${ID_DIR}/headers/helpdefs.h ${home_dir}/fractint.hlp
    WORKING_DIRECTORY ${HELP_SRC_DIR}
    COMMAND hc /c
    COMMAND ${CMAKE_COMMAND} -E copy helpdefs.h ${ID_DIR}/headers
    COMMAND ${CMAKE_COMMAND} -E copy fractint.hlp ${home_dir}
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
