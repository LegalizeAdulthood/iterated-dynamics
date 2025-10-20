# To Do List

This file contains a list of things that are on the "To Do" list of
the Id development team, practiced in the true Stone Soup
tradition.  Any item on this list is up for grabs and you are
encouraged to use this as a starting point for your Id
contributions!

This document is arranged by the functional area within Id.  The
functional areas are listed in alphabetical order with each idea
that's been suggested for improving the various sections.

This file last updated May 24th, 2024.

##  3D Support
- Provide a way to plug-in a 3D driver by name; platform support
  determines what drivers are available.  Id "native" 3D support
  available on all platforms.
- Add arcball for iteractive manipulation of 3D viewing parameters
  (interactively manipulate viewed object by its bounding box)

## Arbitrary Precision
- Extend arbitrary precision arithmetic to other fractal types, most
  notably formula types
- Allow arbitrary precision values to be entered into text field boxes
  and PAR files

## Deep Color Support
- 24-bit color modes
- 32-bit color modes (RGB plus alpha)
- PNG 24/32-bit output/input
- Coloring pixels by formulas
- Texture mapping (probably best integrated into formula coloring)
- HDR (high dynamic range) color output

## Formula Parser
- Add type information for expressions and variables
- Add remainder (modulus) operator/function
- Make C versions of corresponding assembly functions more efficient
  (reduce function call overhead, apply optimizations)
- Provide a way to perform user-defined computations once per-image
- Provide a way to define and call named user functions like regular
  functions
- Generalize the functions (user-defined) defaults and incorporate
  this in the formula parser and related areas.
- Extend the parser support to deal w/ orbits (orbit-like fractals)

## Fractal Types
- Add 2D cellular automata
- Add continuously valued automata, a la CAPOW
- Various 3D fractal types that could be added
- Volume rendered fractal types (3D projection of quaternion julia set)
- dL-systems (need more research first)
- HIFS (Hierachical IFS) seems easy.

## Fractal Types: Cellular
- Extend 1D cellular automata types beyond existing totalistic automata

## Help Files
- Add formula tutorial
- Add formula coloring methods tutorial
- Add color editor tutorial
- Add support to the help compiler for generating postscript / PDF /
  HTML output.
- Add support for inlined images in help browser (initially present
  only in PS/PDF/HTML versions)

## Image Computation
- Provide anti-aliasing support directly (requires deep color)
- Synchronous Orbits Iteration
- Gamma correction

## Image I/O
- Provide PNG support for both 8-bit and deeper video modes; handle
  gamma correction properly on output

## Map Files
- Gamma correction

## Miscellaneous
- Space mappings (maybe using the parser also) like generalizations to
  inversion but allowing any distortion/mapping to be applied.
- Support to plot grids, axes, scales, color scales, legends etc. to
  illustrate the images onscreen
- Support fot HSB/HLS color models.
- Distributed/collaborative computing of fractals over the network.
  (See [zeromq-fractal-renderers.md](zeromq-fractal-renderers.md) for
  information about ZeroMQ as a potential technology choice for this feature)

## Platform Support: unix/X
- Visual selection assumed root is 8-bit psuedocolor; improve to
  select appropriate visual for requested video mode (could be 24-bit
  with deep color support)
- Eliminate use of curses and xterm in favor of native X-based text
  windows
- Fix function key support (probably a free side-effect of previous item)
- Use Xt for interface components of "Id GUI API"
- 3D drivers: OpenGL, native X
- Generate /bin/sh scripts instead of MS-DOS bat files for "b" command
- long filename problems?

## Zoom Box
- Use XaoS like techniques to speedup the zoom box and/or initialize
  the screen from the zoomed section.
