# Copyright 2014 Richard Thomson
cmake_minimum_required(VERSION 3.23)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# <filesystem> is C++17 or later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(iterated-dynamics CXX)

set(home_dir "${CMAKE_SOURCE_DIR}/home")
set(ID_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(set_src_dir src_file)
    set_source_files_properties(${src_file}
        PROPERTIES COMPILE_DEFINITIONS SRCDIR="${home_dir}")
endfunction()

set(ID_TARGET_DEFINITIONS "")
set(ID_TARGET_DEFINITIONS_DEBUG "")
set(ID_EXECUTABLE_TYPE "")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_EXECUTABLE_TYPE "WIN32")
else()
    list(APPEND ID_TARGET_DEFINITIONS XFRACT LINUX)
endif()

set(ID_TARGET_OPTIONS "")
if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    list(APPEND ID_TARGET_OPTIONS "-Werror")
endif()


# In debug builds, tell MSVC to:
#   - not warn us about unchecked iterators
#   - not warn us about deprecated CRT functions
#   - not warn us about possibly unsafe CRT functions
#   - store source code location information for memory allocations
if(MSVC)
    list(APPEND ID_TARGET_DEFINITIONS_DEBUG
        "_SCL_SECURE_NO_WARNINGS"
        "_CRT_SECURE_NO_DEPRECATE"
        "_CRT_SECURE_NO_WARNINGS"
        "_CRTDBG_MAP_ALLOC")
endif()

add_subdirectory(unix)
add_subdirectory(win32)
add_subdirectory(hc)

add_executable(id ${ID_EXECUTABLE_TYPE}
    common/main.cpp
)
target_link_libraries(id PUBLIC libid)

add_library(libid
    headers/id_io.h
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
    common/intro.cpp headers/intro.h
    common/jiim.cpp headers/jiim.h
    common/miscovl.cpp headers/miscovl.h
    common/miscres.cpp headers/miscres.h
    common/prompts1.cpp headers/prompts1.h
    common/prompts2.cpp headers/prompts2.h
    common/realdos.cpp headers/realdos.h
    common/update_save_name.cpp headers/update_save_name.h
    common/zoom.cpp headers/zoom.h

    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/drivers.h
    headers/fractint.h
    headers/id_data.h
    headers/fractype.h
    headers/lsys.h
    headers/mpmath.h
    headers/newton.h
    headers/port.h
    headers/prototyp.h
    headers/winprot.h
)

source_group("Header Files/common/3d" FILES
    headers/3d.h
    headers/line3d.h
    headers/plot3d.h
)
source_group("Source Files/common/3d" FILES
    common/3d.cpp
    common/line3d.cpp
    common/plot3d.cpp
)
source_group("Header Files/common/engine" FILES
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
source_group("Source Files/common/engine" FILES
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
source_group("Header Files/common/fractal specific" FILES
    headers/ant.h
    headers/jb.h
    headers/lorenz.h
    headers/lsys.h
    headers/lsysf.h
    headers/lsys_fns.h
    headers/miscfrac.h
    headers/newton.h
)
source_group("Source Files/common/fractal specific" FILES
    common/ant.cpp
    common/jb.cpp
    common/lorenz.cpp
    common/lsys.cpp
    common/lsysf.cpp
    common/miscfrac.cpp
)
source_group("Header Files/common/IO" FILES
    headers/cmdfiles.h
    headers/decoder.h
    headers/diskvid.h
    headers/editpal.h
    headers/encoder.h
    headers/evolve.h
    headers/gifview.h
    headers/id_io.h
    headers/loadfdos.h
    headers/loadfile.h
    headers/loadmap.h
    headers/parser.h
    headers/rotate.h
    headers/slideshw.h
    headers/stereo.h
)
source_group("Source Files/common/IO" FILES
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
source_group("Header Files/common/math" FILES
    headers/big.h
    headers/biginit.h
    headers/cmplx.h
    headers/fpu087.h
    headers/hcmplx.h
    headers/mpmath.h
    headers/mpmath_c.h
)
source_group("Source Files/common/math" FILES
    common/bigflt.cpp
    common/biginit.cpp
    common/bignum.cpp
    common/bignumc.cpp
    common/fpu087.cpp
    common/hcmplx.cpp
    common/mpmath_c.cpp
)
source_group("Header Files/common/plumbing" FILES
    headers/drivers.h
    headers/memory.h
)
source_group("Source Files/common/plumbing" FILES
    common/drivers.cpp
    common/memory.cpp
)
source_group("Header Files/common/ui" FILES
    headers/fractint.h
    headers/framain2.h
    headers/help.h
    headers/helpdefs.h
    headers/intro.h
    headers/jiim.h
    headers/miscovl.h
    headers/miscres.h
    headers/prompts1.h
    headers/prompts2.h
    headers/realdos.h
    headers/update_save_name.h
    headers/zoom.h
)
source_group("Source Files/common/ui" FILES
    common/fractint.cpp
    common/framain2.cpp
    common/help.cpp
    common/intro.cpp
    common/jiim.cpp
    common/miscovl.cpp
    common/miscres.cpp
    common/prompts1.cpp
    common/prompts2.cpp
    common/realdos.cpp
    common/update_save_name.cpp
    common/zoom.cpp
)

set_src_dir(common/help.cpp)
set_src_dir(common/fractint.cpp)

target_compile_definitions(libid PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(libid PUBLIC ${ID_TARGET_OPTIONS})
target_include_directories(libid PRIVATE headers)
target_link_libraries(libid PRIVATE helpcom os)
add_dependencies(libid native_help)
