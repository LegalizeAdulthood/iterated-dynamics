# Global Variables in libid/include/fractals

This document identifies groups of global variables in the `libid/include/fractals` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in fractal-specific calculations and algorithms.

## 1. Fractal Type/Specification Group

**Files:** `fractype.h`, `fractalp.h`

Variables defining the current fractal type and its specifications:

- `g_fractal_type` - current fractal type enum value
- `g_cur_fractal_specific` - pointer to current fractal's specification structure
- `g_fractal_specific[]` - array of all fractal specifications
- `g_num_fractal_types` - total number of fractal types
- `g_alternate_math[]` - alternate math function pointers for fractals
- `g_more_fractal_params[]` - extended parameter specifications

## 2. Formula Parser Group

**File:** `parser.h`

Variables for user-defined fractal formulas:

- `g_formula_filename` - path to formula definition file
- `g_formula_name` - name of current formula
- `g_is_mandelbrot` - formula uses Mandelbrot-style calculations
- `g_frm_uses_ismand` - formula uses ismand variable
- `g_frm_uses_p1` through `g_frm_uses_p5` - formula uses parameters 1-5
- `g_max_function` - maximum function ID used in formula
- `g_max_function_args` - maximum function arguments
- `g_max_function_ops` - maximum function operations
- `g_operation_index` - current operation index
- `g_variable_index` - current variable index
- `g_last_init_op` - last initialization operation
- `g_load_index` - load stack index
- `g_store_index` - store stack index

## 3. IFS (Iterated Function System) Group

**File:** `ifs.h`

Variables for IFS fractals:

- `g_ifs_definition` - vector of IFS transformation parameters
- `g_ifs_filename` - path to IFS definition file
- `g_ifs_name` - name of current IFS
- `g_ifs_dim` - IFS dimension (2D or 3D)
- `g_num_affine_transforms` - number of affine transformations

## 4. L-System Group

**File:** `lsystem.h`

Variables for L-system fractals:

- `g_l_system_filename` - path to L-system definition file
- `g_l_system_name` - name of current L-system
- `g_max_angle` - maximum angle for L-system drawing

## 5. 3D Display/Orbit Group

**File:** `lorenz.h`

Variables for 3D fractal display and orbit calculations:

- `g_display_3d` - 3D display mode (NONE, YES, B_COMMAND)
- `g_orbit_corner` - corner coordinates for orbit calculations
- `g_orbit_interval` - interval between saved orbit points
- `g_orbit_save_flags` - flags for orbit data output (RAW, MIDI)
- `g_orbit_save_name` - filename for saved orbit data
- `g_set_orbit_corners` - flag to set orbit corners
- `g_keep_screen_coords` - keep screen coordinates flag
- `g_max_count` - maximum iteration count for orbits
- `g_major_method` - orbit traversal method (BREADTH_FIRST, DEPTH_FIRST, RANDOM_WALK, RANDOM_RUN)
- `g_inverse_julia_minor_method` - inverse Julia minor method (LEFT_FIRST, RIGHT_FIRST)

## 6. Julibrot Group

**File:** `julibrot.h`

Variables for Julibrot (4D) fractal calculations:

- `g_julibrot` - Julibrot mode enabled flag
- `g_julibrot_3d_mode` - 3D viewing mode (MONOCULAR, LEFT_EYE, RIGHT_EYE, RED_BLUE)
- `g_julibrot_x_min`, `g_julibrot_x_max` - x-axis range
- `g_julibrot_y_min`, `g_julibrot_y_max` - y-axis range
- `g_julibrot_width` - width of Julibrot slice
- `g_julibrot_height` - height of Julibrot slice
- `g_julibrot_depth` - depth of Julibrot slice
- `g_julibrot_dist` - distance for 3D viewing
- `g_julibrot_origin` - origin point for Julibrot
- `g_julibrot_z_dots` - number of z-axis dots
- `g_eyes` - eye separation for stereo viewing
- `g_new_orbit_type` - fractal type for orbit calculation
- `g_save_dac` - save DAC (color palette) flag
- `g_julibrot_3d_options[]` - array of 3D option strings

## 7. Fractal Algorithm Parameters Group

**File:** `fractals.h` (in engine)

Variables for fractal-specific algorithm parameters:

- `g_basin` - basin calculation parameter
- `g_degree` - polynomial degree
- `g_c_exponent` - exponent value for calculations
- `g_max_color` - maximum color value for current fractal
- `g_bof_match_book_images` - use normal BOF initialization
- `g_fudge_half` - fudge factor for calculations
- `g_power_z` - complex power value

## Usage Patterns

These variable groups follow typical fractal calculation patterns:

1. **Type Selection**: Fractal type variables determine which algorithm to use
2. **Algorithm Configuration**: Parser, IFS, and L-system variables configure specific fractal algorithms
3. **3D Visualization**: 3D display and orbit variables control advanced visualization
4. **4D Extensions**: Julibrot variables extend fractals into four dimensions
5. **Parameter Control**: Algorithm parameter variables fine-tune fractal behavior

## Relationships with Engine and UI Globals

Fractal globals work closely with other global variable groups:

- **Fractal type variables** work with calculation state variables (`calcfrac.h` in engine)
- **3D display variables** work with screen dimension variables (`VideoInfo.h` in engine)
- **Orbit variables** work with plotting and symmetry variables (`calcfrac.h` in engine)
- **Julibrot variables** work with bailout and iteration variables (`calcfrac.h` in engine)
- **Formula parser variables** may modify any engine calculation variables during formula evaluation

## Fractal-Specific Patterns

### Formula-Based Fractals
User-defined formula fractals use the parser group variables extensively. The formula parser reads formula files and sets up the calculation based on the parsed formula.

### Attractor Fractals  
3D attractors (Lorenz, Rossler, Henon) use the orbit group variables to define the viewing transformation and save orbit data.

### IFS Fractals
IFS fractals use transformation matrices stored in `g_ifs_definition` and apply them probabilistically based on `g_num_affine_transforms`.

### L-System Fractals
L-system fractals generate their structure through string rewriting rules loaded from files referenced by `g_l_system_filename`.

### Julibrot Fractals
Julibrot fractals extend 2D Mandelbrot/Julia sets into 4D space, with the extra dimensions controlled by the julibrot group variables.

## Refactoring Considerations

When refactoring to reduce global state in fractal code, consider:

1. **Fractal Type** variables could become part of a `FractalContext` class
2. **Formula Parser** variables could become a `FormulaEngine` class
3. **IFS** variables could become an `IFSGenerator` class
4. **L-System** variables could become an `LSystemGenerator` class
5. **3D Display** variables could become a `Display3DContext` or `OrbitRenderer` class
6. **Julibrot** variables could become a `JulibrotEngine` class with 4D parameter management

The fractal-specific variables are good candidates for encapsulation because:
- They form natural groupings by fractal type
- Each fractal type has its own set of parameters and state
- Moving to objects would enable better testing and reusability
- Polymorphic fractal objects could replace the function pointer arrays

## Cross-Cutting Concerns

Some fractal globals are used throughout the codebase:

- `g_fractal_type` is checked everywhere to determine current fractal behavior
- `g_cur_fractal_specific` provides function pointers used by the calculation engine
- `g_display_3d` affects UI, engine, and I/O code paths
- Formula parser variables can access and modify nearly any other global during evaluation
- Julibrot mode affects parameter handling throughout the UI

These cross-cutting variables are among the most challenging to refactor due to their wide usage across multiple subsystems.

## Data File Dependencies

Several fractal types depend on external data files:

- **Formula fractals** require `.frm` formula files
- **IFS fractals** require `.ifs` definition files  
- **L-systems** require `.l` system definition files

The filename and name variables track which files are loaded, while the definition variables store the parsed content. This pattern of "filename + name + data" appears for each file-based fractal type and could be abstracted into a common file-loading mechanism.
