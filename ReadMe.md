<!--
SPDX-License-Identifier: GPL-3.0-only
-->
[![GitHub Release](https://img.shields.io/github/v/release/LegalizeAdulthood/iterated-dynamics?label=Latest+Release)](https://github.com/LegalizeAdulthood/iterated-dynamics/releases)
[![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/LegalizeAdulthood/iterated-dynamics/latest/total?label=Downloads&color=blue)](https://github.com/LegalizeAdulthood/iterated-dynamics/releases)
[![Build workflow](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/build.yml/badge.svg)](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/build.yml)
[![Analysis workflow](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/analysis.yml/badge.svg)](https://github.com/LegalizeAdulthood/iterated-dynamics/actions/workflows/analysis.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![License](https://img.shields.io/github/license/LegalizeAdulthood/iterated-dynamics?label=License)](https://github.com/LegalizeAdulthood/iterated-dynamics/blob/master/LICENSE.txt)

<!-- begin FOTD -->
<!-- https://user.xmission.com/~legalize/fractals/fotd/random.php?id=2133 -->
[![FOTD 2005.01.20](https://user.xmission.com/~legalize/fractals/fotd/2005/01/2005.01.20-Dont_Square_This.thumb.jpg)](https://user.xmission.com/~legalize/fractals/fotd/2005/01/2005.01.20-Dont_Square_This.jpg)<br/>
[Fractal of the Day](https://user.xmission.com/~legalize/fractals/fotd/index.html), 2005.01.20<br/>
Dont Square This ([parameter file](https://user.xmission.com/~legalize/fractals/fotd/2005/01/2005.01.20-Dont_Square_This.par))<br/>
<!-- end FOTD -->

# Iterated Dynamics

Iterated Dynamics is an open source fractal renderer that can generate
the following fractal types:

| Fractal Type | Fractal Type |
| ---- | ---- |
| [Mandelbrot set](https://legalizeadulthood.github.io/iterated-dynamics/#_the_mandelbrot_set) | [Lambda sets](https://legalizeadulthood.github.io/iterated-dynamics/#_lambda_sets) |
| [Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_julia_sets) | [Generalized lambda sets](https://legalizeadulthood.github.io/iterated-dynamics/#_lambdafn) |
| [Inverse Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_inverse_julias) | [Latoocarfian](https://legalizeadulthood.github.io/iterated-dynamics/#_latoocarfian) |
| [Ant automaton](https://legalizeadulthood.github.io/iterated-dynamics/#_ant_automaton) | [Lorenz attractors](https://legalizeadulthood.github.io/iterated-dynamics/#_lorenz_attractors) |
| [Barnsley IFS](https://legalizeadulthood.github.io/iterated-dynamics/#_barnsley_ifs_fractals) | [Lyapunov](https://legalizeadulthood.github.io/iterated-dynamics/#_lyapunov_fractals) |
| [Barnsley Mandelbrot/Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_barnsley_mandelbrotjulia_sets) | [Magnetic](https://legalizeadulthood.github.io/iterated-dynamics/#_magnetic_fractals) |
| [Bifurcation](https://legalizeadulthood.github.io/iterated-dynamics/#_bifurcation) | [Mandelbrot Mix4](https://legalizeadulthood.github.io/iterated-dynamics/#_mandelbrot_mix_4) |
| [Burning Ship](https://legalizeadulthood.github.io/iterated-dynamics/#_burning_ship) | [Mandelcloud](https://legalizeadulthood.github.io/iterated-dynamics/#_mandelcloud) |
| [Cellular automata](https://legalizeadulthood.github.io/iterated-dynamics/#_cellular_automata) | [Mandelbrot version of lambda sets](https://legalizeadulthood.github.io/iterated-dynamics/#_mandellambda_sets) |
| [Circle](https://legalizeadulthood.github.io/iterated-dynamics/#_circle) | [Martin attractors](https://legalizeadulthood.github.io/iterated-dynamics/#_martin_attractors) |
| [Diffusion limited aggregation](https://legalizeadulthood.github.io/iterated-dynamics/#_diffusion_limited_aggregation) | [Newton domains of attraction](https://legalizeadulthood.github.io/iterated-dynamics/#_newton_domains_of_attraction) |
| [Divide Brot5](https://legalizeadulthood.github.io/iterated-dynamics/#_dividebrot5) | [Complex Newton](https://legalizeadulthood.github.io/iterated-dynamics/#_complex_newton) |
| [Dynamic system](https://legalizeadulthood.github.io/iterated-dynamics/#_dynamic_system) | [Peterson Mandelbrot variations](https://legalizeadulthood.github.io/iterated-dynamics/#_peterson_variations) |
| [Escher-like Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_escher_like_julia_sets) | [Phoenix curve](https://legalizeadulthood.github.io/iterated-dynamics/#_phoenix) |
| [Custom escape-time formula](https://legalizeadulthood.github.io/iterated-dynamics/#_formula) | [Pickover attractors](https://legalizeadulthood.github.io/iterated-dynamics/#_pickover_attractors) |
| [Frothy basins](https://legalizeadulthood.github.io/iterated-dynamics/#_frothy_basins) | [Pickover Mandelbrot/Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_pickover_mandelbrotjulia_types) |
| [Gingerbread man](https://legalizeadulthood.github.io/iterated-dynamics/#_gingerbreadman) | [Pickover popcorn](https://legalizeadulthood.github.io/iterated-dynamics/#_pickover_popcorn) |
| [Halley map](https://legalizeadulthood.github.io/iterated-dynamics/#_halley) | [Plasma clouds](https://legalizeadulthood.github.io/iterated-dynamics/#_plasma_clouds) |
| [Henon attractors](https://legalizeadulthood.github.io/iterated-dynamics/#_henon_attractors) | [Quartic Mandelbrot/Julia sets](https://legalizeadulthood.github.io/iterated-dynamics/#_quartic_mandelbrotjulia) |
| [Hyper complex](https://legalizeadulthood.github.io/iterated-dynamics/#_hypercomplex) | [Quaternion](https://legalizeadulthood.github.io/iterated-dynamics/#_quaternion) |
| [Icon](https://legalizeadulthood.github.io/iterated-dynamics/#_icon) | [Rossler attractors](https://legalizeadulthood.github.io/iterated-dynamics/#_rossler_attractors) |
| [Julibrot](https://legalizeadulthood.github.io/iterated-dynamics/#_julibrots) | [Scott Taylor / Lee Skinner variations](https://legalizeadulthood.github.io/iterated-dynamics/#_scott_taylor_lee_skinner_variations) |
| [Kam torus](https://legalizeadulthood.github.io/iterated-dynamics/#_kam_torus) | [Sierpinski gasket](https://legalizeadulthood.github.io/iterated-dynamics/#_sierpinski_gasket) |
| [L-systems](https://legalizeadulthood.github.io/iterated-dynamics/#_l_systems) | [Unity](https://legalizeadulthood.github.io/iterated-dynamics/#_unity) |
| | [Volterra-Lotka](https://legalizeadulthood.github.io/iterated-dynamics/#_volterra_lotka_fractals) |

See the [on-line help](http://legalizeadulthood.github.io/iterated-dynamics/)
for more of what the software can do.

See the [wiki](https://github.com/LegalizeAdulthood/iterated-dynamics/wiki)
for the current release plan.

# Building

Iterated Dynamics uses [CMake](http://www.cmake.org) to generate build scripts
and [vcpkg](http://vcpkg.io) to manage third party library dependencies.
You will need a C++ compiler capable of supporting the C++17 standard.

Make a directory for the source code and build directories and then clone this
source repository, initialize and update git submodules with the commands:

```
mkdir id
cd id
git clone https://github.com/LegalizeAdulthood/iterated-dynamics.git
cd iterated-dynamics
git submodule init
git submodule update
```

CMake presets are provided to simplify building the code.  The command

```
cmake --workflow --preset default
```

will configure, build and test the code in the directory `id/build-default`,
a sibling directory to the source code in `id/iterated-dynamics`.

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
[GitHub Actions](https://docs.github.com/en/actions) on your repository to get feedback
on your changes as you push to your branch in your repository.

Once your change is ready, prepare a pull request and submit it back for
incorporation into the main repository.  We couldn't have gotten this far
without contributions from many persons!

To ensure your code will fit in wth the existing code base, check our
[Style Guide](Style.md) and adjust your code to match.

# Code Structure

Most of the source code is in a library named `libid` in a directory by the same
name.  Additional subdirectories serve to categorize the role of the source files in
that directory.  Include files are similarly grouped in subdirectories of 
`include`.  The subdirectories are:

- **3d**
      Files containing 3D drawing support.  Eventually these should be
      subsumed into the driver interface, with this sofwtare implementation
      as a fallback if the driver doesn't support 3D rendering.

- **engine**
      Files containing the implementation of the fractal rendering engines
      that are shared between multiple fractal types.  Also contains the
      big array that defines the available fractal types.

- **fractals**
      Files containing code to implement specific fractal types,
      such as the Lorenz fractal or L-system type.

- **io**
      Anything related to doing external file I/O: saving and loading GIF
      files, dealing with overlay files, saving parameter files, etc.

- **math**
      Files containing math routines for arbitrary precision arithmetic,
      complex arithmetic, etc.

- **misc**
      Miscellaneous routines that don't fit elesewhere like memory and
      driver management.

- **ui**
      Anything to do with displaying menu screens, help screens, intro
      screen, etc.

Additional files are in the following directories:

- `unix`
    Files needed for the unix platform.

- `win32`
    Files needed for the Windows platform.

- `hc`
    Help source files with a custom build step on help.src to run the help
    compiler on the source to generate new id.hlp and helpdefs.h
    files.  Automated tests for `hc` are in the subdirectory `tests`.
    Help compiler input source files are in the subdirectory `src`.

- `tests`
    Contains automated unit and image tests.  Unit tests are executable
    code that tests other bits of code.  Image tests are performed by running
    Id to generate an image and compare against a gold standard image.

- `packaging`
    Scripts and data files for building packaged releases of Id.

- `.github/workflows`
    Scripts that configure automated continuous integration jobs on github.

- `home`
    Data files, such as color maps, parameter files, formula files, and
    so-on that are installed with Id.

- `legacy`
  Miscellaneous legacy FRACTINT files used for reference.  These are not
  currently used for any of the compilation of the code and are placed
  here just for reference.  Once the corresponding features are implemented
  in Id, these files will be deleted.
