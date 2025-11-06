# Global Variables Documentation Index

This directory contains comprehensive documentation of global variables in Iterated Dynamics, organized by subsystem. Each document identifies groups of related global variables, their usage patterns, relationships with other subsystems, and refactoring considerations.

## Documentation Files

### 1. [EngineGlobals.md](EngineGlobals.md)
Documents global variables in `libid/include/engine/` related to:
- Screen dimensions and viewport
- Image region coordinates
- Calculation state and control
- Color and palette management
- Iteration and bailout parameters
- Complex number calculations
- Symmetry detection
- Work lists and optimization
- Video mode configuration
- Function tables and specifications
- User settings and parameters

**Key Files**: `VideoInfo.h`, `ImageRegion.h`, `calcfrac.h`, `solid_guess.h`, `bailout_formula.h`, `random_seed.h`, `show_dot.h`

### 2. [FractalGlobals.md](FractalGlobals.md)
Documents global variables in `libid/include/fractals/` related to:
- Fractal type and specification
- Formula parser state
- IFS (Iterated Function Systems)
- L-Systems
- 3D display and orbit calculations
- Julibrot (4D fractals)
- Fractal-specific algorithm parameters

**Key Files**: `fractype.h`, `parser.h`, `ifs.h`, `lsystem.h`, `lorenz.h`, `julibrot.h`, `fractals.h`

### 3. [IOGlobals.md](IOGlobals.md)
Documents global variables in `libid/include/io/` related to:
- File loading and reading
- File saving and writing
- Color map/palette handling
- GIF encoding and decoding
- Library search paths
- Special directories
- Configuration loading
- Auto-save and timed save

**Key Files**: `loadfile.h`, `encoder.h`, `loadmap.h`, `gifview.h`, `decoder.h`, `library.h`, `special_dirs.h`, `load_config.h`, `save_timer.h`

### 4. [GeometryGlobals.md](GeometryGlobals.md)
Documents global variables in `libid/include/geometry/` related to:
- 3D transformations (rotation, scale, translation)
- Spherical coordinate systems
- Viewing and projection parameters
- Lighting models
- Line3D rendering
- Stereoscopic 3D display
- Ray tracing output
- Matrix and vector operations

**Key Files**: `3d.h`, `line3d.h`, `plot3d.h`

### 5. [MathGlobals.md](MathGlobals.md)
Documents global variables in `libid/include/math/` related to:
- Arbitrary precision math configuration
- Big number (BigNum) variables
- Big float (BigFloat) variables
- Fractal coordinates in arbitrary precision
- Fractal iteration state
- Temporary calculation buffers
- Mathematical constants (?)
- Precision mode selection

**Key Files**: `big.h`, `cmplx.h`

### 6. [MiscGlobals.md](MiscGlobals.md)
Documents global variables in `libid/include/misc/` related to:
- Debug flags and diagnostics
- Driver management (video, input, sound, system)
- Platform abstraction
- System services

**Key Files**: `debug_flags.h`, `Driver.h`

### 7. [UIGlobals.md](UIGlobals.md)
Documents global variables in `libid/include/ui/` related to:
- Help system state
- Zoom box manipulation
- Slideshow control
- Text screen cursor
- Video output mode
- Disk video system
- Mouse input state
- Evolution/genetic algorithms
- User interaction control

**Key Files**: `help.h`, `zoom.h`, `slideshow.h`, `cursor.h`, `video.h`, `diskvid.h`, `mouse.h`, `evolve.h`

## Summary Statistics

Based on the documentation:

- **Total documented subsystems**: 7 (fractals, engine, ui, io, geometry, math, misc)
- **Total major variable groups**: ~60 across all subsystems
- **Total documented global variables**: 250+

## Common Patterns

Several patterns emerge across all subsystems:

### 1. Configuration Groups
Many subsystems have "configuration" variables that control behavior:
- Fractal type selection and parameters
- Video mode and screen dimensions
- 3D transformation parameters
- Precision and math mode selection
- Debug flags and options

### 2. State Machine Variables
Several subsystems maintain state machines:
- Calculation state (CALC_NOT_STARTED ? CALCULATING ? COMPLETED)
- File display state (REQUEST_IMAGE ? LOAD_IMAGE ? IMAGE_LOADED)
- Auto-save state (NONE ? STARTED ? FINAL)

### 3. Temporary/Working Variables
Most subsystems have temporary variables for calculations:
- Complex number calculation buffers
- Big number/big float temporaries
- Matrix and vector working space
- Color palette buffers

### 4. Coordinate/Region Variables
Multiple subsystems track spatial regions:
- Fractal coordinate bounds (x_min, x_max, y_min, y_max)
- Screen viewport dimensions
- 3D transformation ranges
- Zoom box coordinates

### 5. File Path Variables
File-related operations track paths and filenames:
- Read/save filenames
- Formula/IFS/L-system file paths
- Special directory paths
- Color map file names

## Refactoring Opportunities

All documents identify similar refactoring opportunities:

### Object-Oriented Encapsulation
Many variable groups naturally map to classes:
- `FractalContext` - fractal type and parameters
- `CalculationEngine` - calculation state and control
- `DisplayContext` - screen and viewport
- `FileReader`/`FileSaver` - I/O operations
- `Transform3D` - 3D transformations
- `PrecisionContext` - arbitrary precision math
- `DriverRegistry` - device driver management

### Strategy Pattern
Some areas could use strategy patterns:
- Fractal calculation algorithms
- File format encoders/decoders
- 3D rendering techniques
- Precision selection

### State Pattern
State machines could use formal state pattern:
- Calculation lifecycle
- File loading sequence
- Auto-save progression

### Service Locator / Dependency Injection
Cross-cutting services could use:
- Path resolution service
- Driver service registry
- Debug configuration service

## Cross-Cutting Concerns

Several globals are used across multiple subsystems:

- **g_fractal_type** - checked everywhere to determine fractal behavior
- **g_driver** - provides services to all subsystems
- **g_screen_x_dots**, **g_screen_y_dots** - screen dimensions used widely
- **g_save_filename**, **g_read_filename** - file operations
- **g_bf_math** - switches calculation mode throughout engine
- **g_debug_flag** - affects behavior in all subsystems
- **g_special_dirs** - provides paths throughout the codebase

These variables represent the most challenging refactoring targets due to their pervasive use.

## Performance-Critical Variables

Several globals are in performance-critical paths:

- Complex calculation variables (g_old_z, g_new_z, g_tmp_sqr_x, g_tmp_sqr_y)
- Big number/float temporaries (g_bn_tmp1-6, g_bf_tmp1-6)
- Transformation matrix (g_m)
- Screen coordinate deltas (g_delta_x, g_delta_y)
- Driver function pointer (g_driver)
- Pixel plotting functions

Any refactoring must preserve or improve performance in these areas.

## Memory Footprint

Approximate memory usage of global variables:

- **Engine state**: ~1-2 KB (calculation variables, coordinates)
- **UI state**: ~10-20 KB (video buffers, zoom box, evolution state)
- **Fractal definitions**: ~50-100 KB (fractal_specific array, function tables)
- **Arbitrary precision**: ~10-100+ KB per calculation (depends on precision)
- **Color palettes**: ~3-4 KB (g_map_clut, g_dac_box)
- **File I/O buffers**: ~10-50 KB (encoder blocks, gif buffers)

Total global variable memory: **~100-300 KB** (excluding arbitrary precision)

## Historical Context

Many of these global variables date back to the original FRACTINT:

- Written in C (no classes/objects available)
- Tight memory constraints (640KB DOS memory limit)
- Performance critical (slow CPUs, no FPU)
- Single-threaded (no concurrency concerns)

Modern refactoring goals:

- Enable multi-threading
- Improve testability
- Reduce coupling
- Support better encapsulation

## Thread Safety Considerations

Currently, nearly all globals are NOT thread-safe:

- No synchronization mechanisms
- Shared mutable state
- Global function pointers
- Static buffers

Future multi-threading would require:

- Thread-local storage for calculation state
- Immutable configuration objects
- Concurrent data structures
- Reader-writer locks for shared data

## Testing Implications

Global variables complicate testing:

- Tests interfere with each other
- Hard to isolate units
- Setup/teardown complexity
- Difficult to mock dependencies

Refactoring benefits for testing:

- Dependency injection enables mocking
- Object encapsulation enables unit testing
- Immutable configuration simplifies tests
- Less global state reduces test interference

## Migration Strategy

Suggested migration approach:

1. **Document** - Complete (this documentation set)
2. **Isolate** - Group related variables into structs
3. **Encapsulate** - Convert structs to classes with methods
4. **Inject** - Pass objects instead of using globals
5. **Eliminate** - Remove global variables entirely

The migration can proceed subsystem-by-subsystem:
- Start with least coupled subsystems (math, geometry)
- Move to moderately coupled (I/O, fractals)
- Finish with highly coupled (engine, driver)

## Conclusion

This documentation provides a comprehensive map of global variables in Iterated Dynamics, establishing a foundation for future refactoring efforts. The identified patterns, relationships, and refactoring opportunities guide the evolution toward a more maintainable, testable, and thread-safe architecture while preserving the computational performance that is essential for fractal generation.

## Further Reading

For more information on the codebase structure:
- [Style.md](../Style.md) - Coding conventions
- [ReadMe.md](../ReadMe.md) - Project overview and build instructions
- Individual subsystem documentation in each subdirectory

## Updates

- November, 2025 - Initial comprehensive documentation created
