[![CMake workflow](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/cmake.yml/badge.svg)](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/cmake.yml)

# Iterated Dynamics

Iterated Dynamics is an open source fractal renderer that can generate
the following fractal types:

- Mandelbrot set
- Julia sets
- Inverse Julia sets
- Newton domains of attraction
- Lambda sets
- Mandelbrot version of lambda sets
- Plasma clouds
- Generalized lambda sets
- Halley map
- Phoenix curve
- Barnsley Mandelbrot/Julia sets
- Barnsley IFS
- Sierpinski gasket
- Quartic Mandelbrot/Julia sets
- Pickover Mandelbrot/Julia sets
- Pickover popcorn
- Dynamic system
- Mandelcloud
- Peterson Mandelbrot variations
- Unity
- Kam torus
- Bifurcation
- Lorenz attractors
- Rossler attractors
- Henon attractors
- Pickover attractors
- Gingerbread man
- Martin attractors
- Icon
- Latoocarfian
- Quaternion
- Hyper complex
- Cellular automata
- Ant automaton
- Custom escape-time formula
- Frothy basins
- Julibrot
- Diffusion limited aggregation
- Lyapunov
- Magnetic
- Volterra-Lotka
- Escher-like Julia sets
- L-systems

See the [on-line help](http://legalizeadulthood.github.io/iterated-dynamics/)
for more of what the software can do.

See the [wiki](https://github.com/LegalizeAdulthood/iterated-dynamics/wiki)
for the current release plan.

# Building

Iterated Dynamics uses [CMake](http://www.cmake.org) to generate build scripts
and [vcpkg](http://vcpkg.io) to manage third party library dependencies.
You will need a C++ compiler capable of supporting the C++17 standard.

After cloning this source repository, initialize and update git submodules
with the commands:
```
git submodule init
git submodule update
```

CMake presets are provided to simplify building the code.  The command

```
cmake --workflow --preset default
```

will configure, build and test the code.

The build will first compile `hc`, the help compiler.  This generates
the run-time help file from the help source files and an include file
used by the iterated dynamics compile.

# Contributing

Iterated Dynamics welcomes contributions from everyone!  The code has a
long history of contributions from many people over the years.  There are
many small ways in which the code can be improved as well as large changes
that are on the [release plan](https://github.com/LegalizeAdulthood/iterated-dynamics/wiki).

To get started, fork the repository into your github account.  We recommend
using the [github workflow](https://guides.github.com/introduction/flow/index.html)
to make contributions to Iterated Dynamics.  We recommend you enable
[Travis CI builds](https://travis-ci.org) on your repository to get feedback
from static analysis tools on your changes as you push to your branch in
your repository.

Once your change is ready, prepare a pull request and submit it back for
incorporation into the main repository.  We couldn't have gotten this far
without contributions from many persons!

# Code Structure

- - `common`
    Files common to all implementations
 - **3d**
        Files containing 3D drawing support.  Eventually these should be
        subsumed into the driver interface, with this sofwtare implementation
        as a fallback if the driver doesn't support 3D rendering.

 - **engine**
        Files containing the implementation of the fractal rendering engines
        that are shared between multiple fractal types.  Also contains the
        big array that defines the available fractal types.

 - **fractal specific**
        Files containing rendering code or user interaction code that is
        specific to a particular fractal, such as the Lorenz fractal or
        L-system type.

 - **IO**
        Anything related to doing external file I/O: saving and loading GIF
        files, dealing with overlay files, saving parameter files, etc.

 - **math**
        Files containing math routines for arbitrary precision arithmetic,
        complex arithmetic, etc.

 - **plumbing**
        Miscellaneous routines that don't fit elesewhere like memory and
        driver management.

 - **ui**
        Anything to do with displaying menu screens, help screens, intro
        screen, etc.

 - **main**
    Miscellaneous files in the main fractint source folder.  These are not
    currently used for any of the compilation of the code and are placed
    here just for reference.

- `headers`
    Header files

- `hc`
    Help source files with a custom build step on help.src to run the help
    compiler on the source to generate new fractint.hlp and helpdefs.h
    files.

- `unix`
    Files needed for the unix platform.

- `win32`
    Files needed for the Win32 platform.
