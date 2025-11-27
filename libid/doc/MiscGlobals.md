# Global Variables in libid/include/misc

This document identifies groups of global variables in the `libid/include/misc` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in miscellaneous system operations, driver management, and debugging.

## 1. Debug Flags Group

**File:** `debug_flags.h`

Variables for development and debugging:

- `g_debug_flag` - debug flag controlling various debugging features and workarounds

The `DebugFlags` enum provides a rich set of debugging capabilities:

### Performance and Benchmarking Flags
- `BENCHMARK_TIMER` (1) - Enable timer benchmarking
- `BENCHMARK_ENCODER` (200) - Benchmark image encoding

### Formula and Parser Flags
- `WRITE_FORMULA_DEBUG_INFORMATION` (98) - Write formula debug info
- `SHOW_FORMULA_INFO_AFTER_COMPILE` (324) - Display formula information after compilation
- `FORCE_SCALED_SOUND_FORMULA` (4030) - Force scaled sound formula mode

### Calculation Engine Flags
- `FORCE_STANDARD_FRACTAL` (90) - Force standard fractal calculation
- `FORCE_REAL_POPCORN` (96) - Force real popcorn calculation
- `FORCE_BOUNDARY_TRACE_ERROR` (470) - Force boundary trace error for testing
- `FORCE_SOLID_GUESS_ERROR` (472) - Force solid guess error for testing
- `PREVENT_MIIM` (300) - Prevent "MIIM" optimization
- `MANDELBROT_MIX4_FLIP_SIGN` (1012) - Flip sign in Mandelbrot mix4

### Precision and Math Flags
- `FORCE_PRECISION_0_DIGITS` (700) - Force 0-digit precision
- `FORCE_PRECISION_20_DIGITS` (720) - Force 20-digit precision
- `FORCE_LONG_DOUBLE_PARAM_OUTPUT` (750) - Force long double parameter output
- `FORCE_ARBITRARY_PRECISION_MATH` (3200) - Force arbitrary precision math mode
- `PREVENT_ARBITRARY_PRECISION_MATH` (3400) - Prevent arbitrary precision math
- `FORCE_COMPLEX_POWER` (6000) - Force complex power calculation
- `FORCE_SMALLER_BIT_SHIFT` (1234) - Force smaller bit shift value
- `SHOW_FLOAT_FLAG` (2224) - Display float flag state

### I/O and Display Flags
- `HISTORY_DUMP_JSON` (2) - Dump history in JSON format
- `FORCE_DISK_RESTORE_NOT_SAVE` (50) - Force disk restore without save
- `FORCE_MEMORY_FROM_DISK` (420) - Force memory to come from disk
- `FORCE_MEMORY_FROM_MEMORY` (422) - Force memory to come from RAM
- `FORCE_DISK_MIN_CACHE` (4200) - Force minimum disk cache
- `DISPLAY_MEMORY_STATISTICS` (10000) - Display memory usage statistics

### 3D and Rendering Flags
- `FORCE_FLOAT_PERSPECTIVE` (22) - Force floating-point perspective
- `ALLOW_NEGATIVE_CROSS_PRODUCT` (4010) - Allow negative cross product in 3D

### Color and Palette Flags
- `ALLOW_LARGE_COLORMAP_CHANGES` (910) - Allow large colormap changes
- `FORCE_LOSSLESS_COLORMAP` (920) - Force lossless colormap mode

### Special Mode Flags
- `ALLOW_INIT_COMMANDS_ANYTIME` (110) - Allow init commands at any time
- `ALLOW_NEWTON_MP_TYPE` (1010) - Allow Newton MP type
- `PREVENT_PLASMA_RANDOM` (3600) - Prevent randomization in plasma fractals

## 2. Driver Management Group

**File:** `Driver.h`

Variables for device driver management:

- `g_driver` - pointer to current active driver instance

The driver system provides an abstract interface for:
- **Video output**: Setting video modes, reading/writing pixels, drawing lines
- **Text display**: Cursor positioning, text output, screen scrolling
- **Input handling**: Keyboard input, key polling, character reading
- **System interface**: Shell access, file dialogs, timing/delays
- **Sound output**: Buzzer, tone generation, muting
- **Screen management**: Saving/restoring graphics, stacking screens
- **Platform abstraction**: Platform-specific implementations (Windows, Unix, etc.)

## Usage Patterns

These variable groups follow miscellaneous operational patterns:

1. **Debug Flag Usage**:
   - Check `g_debug_flag` at decision points
   - Use bitwise operators to test specific flags
   - Enable special modes or workarounds based on flag values
   - Output diagnostic information when flags are set
- Force specific code paths for testing

2. **Driver Usage**:
   - Initialize driver at startup with `load_driver()`
   - Access driver functions through `g_driver` pointer
   - Use inline helper functions (driver_*) for common operations
   - Switch video modes through driver interface
   - Handle platform differences transparently

## Relationships with Other Global Groups

Misc globals interact with all other subsystems:

- **Debug flags** can affect behavior in engine, fractals, I/O, UI, math, and geometry subsystems
- **Driver** provides services to all subsystems that need video, input, or system services
- Debug flags are checked throughout the codebase to enable special behaviors
- Driver functions are called from engine (for display), UI (for menus), I/O (for file dialogs), etc.

## Debug Flag Architecture

The `g_debug_flag` uses bitwise combinations to enable multiple debugging features:

```cpp
// Example usage:
if (g_debug_flag == DebugFlags::BENCHMARK_TIMER)
{
    // enable timer benchmarking
}

if ((g_debug_flag & DebugFlags::FORCE_ARBITRARY_PRECISION_MATH) != DebugFlags::NONE)
{
    // force arbitrary precision mode
}
```

The enum values are carefully chosen to be recognizable:
- Round numbers (100, 200, 300) for major categories
- Specific values for targeted features
- Some values form mnemonic patterns (e.g., 420-422 for memory options)

## Driver Architecture

The `Driver` abstract class defines a comprehensive interface:

### Initialization and Lifecycle
- `init()` - Initialize driver with command-line args
- `terminate()` - Shutdown driver
- `pause()` / `resume()` - Pause/resume driver operation
- `validate_mode()` - Check if video mode is supported
- `get_max_screen()` - Query maximum screen dimensions

### Window and Display Management
- `create_window()` - Create application window
- `resize()` - Handle window resize events
- `set_video_mode()` - Change video mode
- `is_text()` - Check if in text mode
- `set_for_text()` / `set_for_graphics()` - Switch between text/graphics
- `set_clear()` - Clear screen

### Graphics Operations
- `read_pixel()` / `write_pixel()` - Individual pixel access
- `draw_line()` - Line drawing primitive
- `display_string()` - Draw text in graphics mode
- `save_graphics()` / `restore_graphics()` - Save/restore graphics state

### Palette Management
- `read_palette()` - Read current palette into g_dac_box
- `write_palette()` - Write g_dac_box to hardware palette

### Text Mode Operations
- `put_string()` - Display text at row/column
- `move_cursor()` - Position text cursor
- `hide_text_cursor()` - Hide text cursor
- `set_attr()` - Set text attributes
- `scroll_up()` - Scroll text region
- `get_char_attr()` / `put_char_attr()` - Character/attribute access

### Screen Stack Operations
- `stack_screen()` - Push screen onto stack
- `unstack_screen()` - Pop screen from stack
- `discard_screen()` - Discard top of stack

### Input Handling
- `get_key()` - Read keyboard input
- `key_pressed()` - Check if key is available
- `wait_key_pressed()` - Wait for keypress with optional timeout
- `unget_key()` - Push key back into input buffer
- `key_cursor()` - Get key with cursor display
- `set_keyboard_timeout()` - Set keyboard polling timeout

### Sound Operations
- `init_fm()` - Initialize FM synthesis
- `buzzer()` - Play buzzer tone (COMPLETE, INTERRUPT, PROBLEM)
- `sound_on()` / `sound_off()` - Tone generation
- `mute()` - Silence all sound

### System Operations
- `shell()` - Invoke command shell
- `schedule_alarm()` - Schedule periodic alarm
- `delay()` - Sleep for specified milliseconds
- `get_filename()` - Show file selection dialog
- `flush()` - Flush pending operations

### Debugging Support
- `debug_text()` - Emit debug text
- `check_memory()` - Check heap for corruption
- `get_cursor_pos()` - Query cursor position

### Utility
- `get_name()` - Get driver name
- `get_description()` - Get driver description
- `is_disk()` - Check if disk-based driver

## Refactoring Considerations

When refactoring to reduce global state in misc code, consider:

1. **Debug Configuration**: `g_debug_flag` could become part of a `DebugConfiguration` class that encapsulates all debug settings and provides query methods.

2. **Driver Registry**: Instead of a single `g_driver` pointer, could use a driver registry or service locator pattern that manages multiple drivers.

3. **Driver Context**: Driver functionality could be split into multiple focused interfaces:
   - `IVideoDriver` for graphics operations
   - `IInputDriver` for keyboard/mouse
   - `ISoundDriver` for audio
   - `ISystemDriver` for OS integration

4. **Dependency Injection**: Instead of accessing `g_driver` globally, could pass driver references to subsystems that need them.

The misc variables are challenging to refactor because:
- Debug flags are checked in many conditional compilation scenarios
- The driver pointer is accessed throughout the codebase
- Driver functions are called from nearly every subsystem
- Platform abstraction requires a stable interface
- Performance-critical paths use driver functions in tight loops

## Cross-Cutting Concerns

The misc globals are among the most cross-cutting in the codebase:

- `g_debug_flag` affects behavior in all subsystems
- `g_driver` provides services to all subsystems
- Every rendering operation goes through the driver
- Every user input comes through the driver
- Debug flags can override normal behavior anywhere in the code

These variables represent fundamental system services that are difficult to eliminate without major architectural changes.

## Platform Abstraction

The driver system abstracts platform differences:

- Windows driver implementation in `win32/` directory
- Unix driver implementation in `unix/` directory  
- Common driver interface in `misc/Driver.h`
- Platform-specific behavior hidden behind driver interface
- Cross-platform code calls driver functions without knowing platform

This abstraction is one of the most important architectural elements enabling cross-platform builds.

## Inline Helper Functions

The driver interface provides inline helpers for common operations:

```cpp
// Instead of:
g_driver->write_pixel(x, y, color);

// Can use:
driver_write_pixel(x, y, color);
```

These helpers:
- Reduce verbosity
- Maintain consistent calling convention
- Enable easier refactoring of driver access
- Provide overloads for convenience (e.g., string vs. const char*)

The helpers make the global `g_driver` slightly less coupled to calling code.

## Debug Flag Usage Examples

Common debug flag usage patterns:

```cpp
// Test for specific flag
if (g_debug_flag == DebugFlags::BENCHMARK_TIMER)
{
    start_timer();
}

// Test for flag category
if ((+g_debug_flag >= +DebugFlags::FORCE_PRECISION_0_DIGITS) &&
    (+g_debug_flag <= +DebugFlags::FORCE_PRECISION_20_DIGITS))
{
    // precision debugging
}

// Bitwise test (if implemented)
if ((g_debug_flag & DebugFlags::DISPLAY_MEMORY_STATISTICS) != DebugFlags::NONE)
{
    show_memory_stats();
}
```

The `operator+` converts enum to int for comparisons and range checks.

## Driver Lifecycle

Typical driver lifecycle:

1. **Initialization** (`main()` startup):
   - `init_drivers(&argc, &argv)` - Discover available drivers
   - `load_driver(driver, &argc, &argv)` - Load specific driver
 - `driver->init(&argc, &argv)` - Initialize driver

2. **Operation** (main loop):
   - Call driver functions as needed
   - Handle driver events (resize, etc.)
   - Switch video modes via driver

3. **Shutdown** (program exit):
   - `driver_terminate()` - Cleanup driver
   - `close_drivers()` - Cleanup driver system

The driver remains active for the entire program lifetime.

## Memory Management Note

While this document covers the `misc/memory.h` header, that header declares a memory allocation interface (`MemoryHandle`, `memory_alloc()`, etc.) but does not declare global variables. The memory management system uses internal state but does not expose global variables in its public interface.

The memory system provides:
- Virtual memory management (RAM or disk-backed)
- Memory handles for large allocations
- Transparent swapping between memory and disk
- Used primarily for disk video mode

This is an example of good encapsulation where internal state is hidden behind a functional interface.
