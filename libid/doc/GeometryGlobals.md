# Global Variables in libid/include/geometry

This document identifies groups of global variables in the `libid/include/geometry` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in 3D rendering, line drawing, and perspective transformations.

## 1. Regular 3D Transformation Group

**File:** `3d.h`

Variables for standard 3D transformations:

- `g_sphere` - sphere mode flag (true = yes, false = no)
- `g_x_rot` - rotate x-axis angle (degrees)
- `g_y_rot` - rotate y-axis angle (degrees)
- `g_z_rot` - rotate z-axis angle (degrees)
- `g_x_scale` - scale x-axis (percent)
- `g_y_scale` - scale y-axis (percent)

## 2. Spherical Coordinate Group

**File:** `3d.h`

Variables for sphere 3D mode:

- `g_sphere_phi_min` - longitude start angle
- `g_sphere_phi_max` - longitude end angle
- `g_sphere_theta_min` - latitude start angle (degrees)
- `g_sphere_theta_max` - latitude stop angle (degrees)
- `g_sphere_radius` - sphere radius (user input)

## 3. Common 3D Parameters Group

**File:** `3d.h`

Variables shared across 3D rendering modes:

- `g_rough` - scale z-axis (percent) for terrain roughness
- `g_water_line` - water level for coloring
- `g_fill_type` - fill type (SURFACE_GRID, POINTS, WIRE_FRAME, SURFACE_INTERPOLATED, SURFACE_CONSTANT, SOLID_FILL, LIGHT_SOURCE_BEFORE, LIGHT_SOURCE_AFTER)
- `g_viewer_z` - perspective view point distance
- `g_shift_x` - x-axis shift
- `g_shift_y` - y-axis shift

## 4. Lighting Model Group

**File:** `3d.h`

Variables for illumination calculations:

- `g_light_x` - x component of light vector
- `g_light_y` - y component of light vector
- `g_light_z` - z component of light vector
- `g_light_avg` - number of points to average for smooth lighting

## 5. Line3D Rendering Group

**File:** `line3d.h`

Variables for 3D line and surface rendering:

- `g_ambient` - ambient light parameter value
- `g_background_color[]` - array of background RGB values
- `g_brief` - brief output mode flag
- `g_converge_x_adjust` - convergence x-axis adjustment
- `g_converge_y_adjust` - convergence y-axis adjustment
- `g_haze` - atmospheric haze parameter
- `g_light_name` - name of lighting configuration
- `g_m` - transformation matrix (4x4 double matrix)
- `g_preview` - preview mode flag
- `g_preview_factor` - preview reduction factor
- `g_randomize_3d` - randomization parameter for 3D
- `g_raytrace_filename` - filename for ray tracing output
- `g_raytrace_format` - ray tracer format (NONE, DKB_POVRAY, VIVID, RAW, MTV, RAYSHADE, ACROSPIN, DXF)
- `g_show_box` - show bounding box flag
- `g_standard_plot` - function pointer for standard plot routine
- `g_targa_out` - 3D full color (24-bit) output flag
- `g_targa_overlay` - overlay targa on existing image flag
- `g_transparent_color_3d[]` - transparency min/max RGB values
- `g_view` - view vector (3D double vector)
- `g_x_shift` - x-axis shift for line3d
- `g_xx_adjust` - x fine adjustment
- `g_y_shift` - y-axis shift for line3d
- `g_yy_adjust` - y fine adjustment

## 6. Stereo 3D Group

**File:** `plot3d.h`

Variables for stereoscopic 3D rendering:

- `g_adjust_3d` - 3D adjustment point (x, y)
- `g_blue_bright` - blue channel brightness adjustment
- `g_blue_crop_left` - blue channel left crop
- `g_blue_crop_right` - blue channel right crop
- `g_eye_separation` - distance between stereo eyes
- `g_glasses_type` - type of 3D glasses (NONE, ALTERNATING, SUPERIMPOSE, PHOTO, STEREO_PAIR)
- `g_red_bright` - red channel brightness adjustment
- `g_red_crop_left` - red channel left crop
- `g_red_crop_right` - red channel right crop
- `g_which_image` - which stereo image (NONE, RED, BLUE)
- `g_x_shift1` - x-axis shift for stereo
- `g_xx_adjust1` - x fine adjustment for stereo
- `g_y_shift1` - y-axis shift for stereo
- `g_yy_adjust1` - y fine adjustment for stereo

## Usage Patterns

These variable groups follow typical 3D rendering patterns:

1. **3D Transformation Setup**:
   - Set rotation angles (`g_x_rot`, `g_y_rot`, `g_z_rot`)
   - Set scale factors (`g_x_scale`, `g_y_scale`, `g_rough`)
 - Choose sphere vs. regular mode (`g_sphere`)
   - Configure spherical coordinates if in sphere mode

2. **Viewing Configuration**:
   - Set viewer position (`g_viewer_z`)
   - Set shift/offset (`g_shift_x`, `g_shift_y`)
   - Configure fill type (`g_fill_type`)
   - Set water line for terrain (`g_water_line`)

3. **Lighting Setup**:
   - Configure light vector (`g_light_x`, `g_light_y`, `g_light_z`)
   - Set ambient light (`g_ambient`)
   - Configure light averaging (`g_light_avg`)
   - Set haze effects (`g_haze`)

4. **Rendering Mode**:
   - Choose preview vs. full render (`g_preview`, `g_preview_factor`)
   - Configure ray tracing if enabled (`g_raytrace_format`, `g_raytrace_filename`)
   - Set output format (`g_targa_out`, `g_targa_overlay`)
   - Configure transparency (`g_transparent_color_3d[]`)

5. **Stereo 3D Configuration**:
   - Select glasses type (`g_glasses_type`)
   - Set eye separation (`g_eye_separation`)
   - Configure red/blue cropping and brightness
   - Set which eye to render (`g_which_image`)

## Relationships with Other Global Groups

Geometry globals interact with multiple subsystems:

- **3D transformation variables** work with fractal parameters to map fractal data into 3D space
- **Lighting variables** work with color palette variables to compute shaded colors
- **Stereo variables** work with screen dimension variables (`VideoInfo.h`) to render split images
- **Ray tracing variables** work with file I/O (`io/encoder.h`) to export 3D models
- **View transformation** uses the transformation matrix `g_m` built from rotation and scale variables
- **Line3D rendering** works with calculation engine to convert fractal iterations into 3D surfaces

## Matrix and Vector Operations

The geometry subsystem uses mathematical structures for 3D transformations:

- **Matrix** (4×4 double) - represents coordinate transformations
- **MatrixL** (4×4 long) - integer version for fixed-point math
- **Vector** (3D double) - represents position or direction
- **VectorL** (3D long) - integer version for fixed-point math

These structures enable:
- Rotation around each axis (x_rot, y_rot, z_rot functions)
- Scaling in each dimension (scale function)
- Translation (trans function)
- Perspective projection (perspective function)
- Matrix concatenation (mat_mul function)
- Vector transformations (vec_mat_mul function)

## Refactoring Considerations

When refactoring to reduce global state in geometry code, consider:

1. **3D Transform Context**: Rotation, scale, and sphere variables could become a `Transform3D` class with methods to build transformation matrices.

2. **Lighting Context**: Lighting variables could become a `LightingModel` class that encapsulates illumination calculations.

3. **View Context**: Viewing variables (viewer position, shift, fill type) could become a `ViewConfiguration` class.

4. **Stereo Renderer**: Stereo 3D variables could become a `StereoRenderer` class that manages red/blue eye rendering.

5. **Ray Tracer Interface**: Ray tracing variables could become a `RayTracerExporter` class that handles different output formats.

6. **Line3D Renderer**: The line3d rendering variables could become a `SurfaceRenderer` class.

The geometry variables are good candidates for encapsulation because:
- They form natural groupings by rendering stage (transform, view, light, render)
- 3D operations have clear input/output boundaries
- Matrix operations are already encapsulated in functions
- Moving to objects would enable better testing of 3D transformations
- Stereo rendering could be implemented as a rendering strategy pattern

## Cross-Cutting Concerns

Several geometry globals are used throughout the codebase:

- `g_fill_type` determines rendering approach in multiple rendering loops
- `g_preview` and `g_preview_factor` affect calculation resolution throughout the engine
- `g_glasses_type` affects screen output in the video driver
- `g_targa_out` changes color handling throughout the rendering pipeline
- `g_ambient` and haze parameters affect final color calculations
- Transformation matrix `g_m` is used in multiple coordinate conversion routines

These cross-cutting variables would require careful interface design during refactoring. The transformation matrix in particular is a performance-critical data structure used in tight rendering loops.

## Rendering Pipeline

The 3D rendering pipeline follows these stages:

1. **Fractal Calculation** ? 2D array of iteration values or z-heights
2. **3D Transformation** ? Apply rotation, scale, and perspective using `g_m` matrix
3. **View Projection** ? Project 3D coordinates to 2D screen using `g_viewer_z`
4. **Lighting** ? Calculate shading using light vector and surface normals
5. **Stereo Split** ? Render separate images for left/right eyes if stereo enabled
6. **Rasterization** ? Draw lines, polygons, or pixels using fill type
7. **Output** ? Display on screen or export to ray tracer format

Global variables control each stage of this pipeline, making the geometry subsystem one of the most stateful parts of the codebase.

## Fill Type Hierarchy

The `FillType` enum defines a hierarchy of rendering techniques:

- **SURFACE_GRID** (-1) - Grid overlay on surface
- **POINTS** (0) - Render as discrete points
- **WIRE_FRAME** (1) - Render edges only
- **SURFACE_INTERPOLATED** (2) - Smooth interpolated surface
- **SURFACE_CONSTANT** (3) - Flat shaded surface
- **SOLID_FILL** (4) - Solid filled surface
- **LIGHT_SOURCE_BEFORE** (5) - Apply lighting before rendering
- **LIGHT_SOURCE_AFTER** (6) - Apply lighting after rendering

Values > SOLID_FILL enable illumination via the `illumine()` helper function, which triggers lighting calculations using the light vector variables.

## Ray Tracing Format Support

The system can export 3D scenes to multiple ray tracer formats:

- **DKB_POVRAY** - POV-Ray format (popular ray tracer)
- **VIVID** - Vivid ray tracer format
- **RAW** - Raw triangle data
- **MTV** - MTV ray tracer format
- **RAYSHADE** - Rayshade format
- **ACROSPIN** - Acrospin format
- **DXF** - AutoCAD DXF format

This multi-format support allows fractal surfaces to be imported into professional 3D modeling and rendering tools for high-quality rendering with advanced lighting, materials, and effects.

## Stereo Rendering Modes

The `GlassesType` enum supports multiple stereoscopic viewing methods:

- **ALTERNATING** - Alternate lines for interlaced displays
- **SUPERIMPOSE** - Red/blue anaglyph for colored glasses
- **PHOTO** - Side-by-side for stereoscopic viewers
- **STEREO_PAIR** - Separate images for manual viewing

Each mode requires different rendering strategies and uses the eye separation, crop, and brightness variables differently to optimize the 3D effect.

## Performance Considerations

Geometry calculations are performance-critical:

- Matrix operations are called for every pixel in 3D mode
- The transformation matrix `g_m` is accessed in tight loops
- Vector operations should avoid unnecessary allocations
- Fixed-point math (MatrixL, VectorL) provides faster integer-only calculations
- `g_standard_plot` function pointer enables fast pixel plotting without virtual dispatch

When refactoring, these hot paths must remain efficient, avoiding vtable lookups or excessive parameter passing in innermost loops.
