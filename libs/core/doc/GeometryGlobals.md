<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Geometry Global Variables

Inventory of public `g_*` declarations in `libs/core/include/geometry`.
These are current declaration locations, not a claim that geometry is
already independent of engine, I/O, or UI.

## Transform And Projection Settings

Header: `3d.h`.

- `g_sphere` - spherical rather than regular 3D mapping.
- `g_x_rot`, `g_y_rot`, `g_z_rot` - rotation settings in degrees.
- `g_x_scale`, `g_y_scale` - horizontal/vertical scale percentages.
- `g_sphere_phi_min`, `g_sphere_phi_max` - longitude limits.
- `g_sphere_theta_min`, `g_sphere_theta_max` - latitude limits.
- `g_sphere_radius` - spherical radius setting.
- `g_rough` - height scale percentage.
- `g_water_line` - water-level setting.
- `g_fill_type` - surface rendering algorithm.
- `g_viewer_z` - perspective viewing distance.
- `g_shift_x`, `g_shift_y` - image-position settings.

`Matrix` and `MatrixL` are 4-by-4 arrays of `double` and `long`.
`Vector` and `VectorL` contain three components.  Their declarations and
the matrix/vector operations are not additional globals.

`FillType` includes surface grid, points, wire frame, interpolated and
constant surfaces, solid fill, and lighting before/after transformation.
`illumine()` selects the two lighting modes by testing values above
`SOLID_FILL`.

## Lighting

Headers: `3d.h`, `line3d.h`.

- `g_light_x`, `g_light_y`, `g_light_z` - light-vector components.
- `g_light_avg` - normal/light averaging setting.
- `g_ambient` - ambient illumination.
- `g_haze` - atmospheric haze.
- `g_background_color` - background RGB components.

`g_light_name` is a Targa output/overlay filename, not a lighting
configuration name; it belongs with the output settings below.

## Surface Rendering State

Header: `line3d.h`.

- `g_m` - active transformation matrix.
- `g_view` - viewing vector.
- `g_standard_plot` - selected plot function.
- `g_x_shift`, `g_y_shift` - calculated image shifts.
- `g_xx_adjust`, `g_yy_adjust` - fine adjustments.
- `g_converge_x_adjust`, `g_converge_y_adjust` - convergence adjustments.
- `g_preview`, `g_preview_factor` - subsampling of the 3D surface
  processing path; these do not change all engine calculation resolution.
- `g_show_box` - bounding-box display setting.
- `g_randomize_3d` - surface color randomization setting.
- `g_transparent_color_3d` - minimum/maximum transparent palette indices,
  not RGB component bounds.

Scanline buffers and additional working state remain private to
`geometry/line3d.cpp`.  This public inventory does not imply that all
surface state is encapsulated in a render object.

## Targa And Ray Export

Header: `line3d.h`.

- `g_light_name` - Targa output or overlay filename.
- `g_targa_out` - true-color surface output setting.
- `g_targa_overlay` - overlay on an existing Targa image.
- `g_raytrace_filename` - ray-export filename.
- `g_raytrace_format` - `NONE`, `DKB_POVRAY`, `VIVID`, `RAW`, `MTV`,
  `RAYSHADE`, `ACROSPIN`, or `DXF`.
- `g_brief` - abbreviated ray-export setting.

Ray export is implemented by `geometry/line3d.cpp`, with its own output
handle and library path resolution; it is not GIF encoding in
`io/encoder.cpp`.  Targa preparation returns an I/O storage request and
diagnostics to the UI owner.

## Stereo Projection And Plotting

Header: `plot3d.h`.

- `g_adjust_3d` - current 2D projection adjustment.
- `g_eye_separation` - eye separation setting.
- `g_glasses_type` - selected `GlassesType`.
- `g_which_image` - `StereoImage::NONE`, `RED`, or `BLUE`.
- `g_red_bright`, `g_blue_bright` - channel brightness settings.
- `g_red_crop_left`, `g_red_crop_right`, `g_blue_crop_left`,
  `g_blue_crop_right` - eye/channel cropping settings.
- `g_x_shift1`, `g_y_shift1`, `g_xx_adjust1`, `g_yy_adjust1` -
  alternate-eye shifts and adjustments.

The stereo modes are:

- `NONE` - ordinary projection.
- `ALTERNATING` - checkerboard pixel parity, not alternating scanlines.
- `SUPERIMPOSE` - overlaid red/blue output.
- `PHOTO` - separately displayed eyes with a UI photographer prompt
  between views.
- `STEREO_PAIR` - side-by-side cross-eyed viewing arrangement.

`StereoView` owns projection/view transitions and screen restoration.
Calculation objects retain their eye-specific work.  UI owns prompts,
input, palette preparation, and sound servicing.

## Random-Dot Stereogram Inputs

Header: `stereo_params.h`.

- `g_auto_stereo_depth` - depth setting.
- `g_auto_stereo_width` - physical image width setting.
- `g_calibrate` - calibration bars: none, middle, or top.
- `g_gray_flag` - derive depth from gray value rather than color index.
- `g_use_stereo_texture` - use an image texture instead of random input.
- `g_stereo_texture_filename` - selected texture filename.

Automatic conversion and texture reuse are separate UI policy in
`ui/stereo.h`.  The UI owns the complete `rds` and `rds-texture`
parameter handlers and writes these geometry inputs.

## Remaining Ownership Coupling

Geometry does not poll keyboard or mouse input or open modal diagnostics.
It still uses engine/display settings and I/O backends.
`line3d.cpp` exposes `finish_line3d()` for output cleanup.  UI registers
and invokes that operation through its own `g_out_line_cleanup` binding;
geometry does not access UI state.

Engine, geometry, and I/O still share the core library.  Their finer
subsystem dependency direction remains separate from the established
core/UI library boundary.

See [hexagonal.md](hexagonal.md) for target dependencies and
[UIGlobals.md](UIGlobals.md) for the UI-owned coordination state.
