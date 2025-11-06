# Global Variables in libid/include/math

This document identifies groups of global variables in the `libid/include/math` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in mathematical calculations, arbitrary precision arithmetic, and complex number operations.

## 1. Big Number Math Configuration Group

**File:** `big.h`

Variables controlling arbitrary precision math mode and configuration:

- `g_bf_math` - big float math mode (NONE, BIG_NUM, BIG_FLT)
- `g_bn_step` - step size for big number iterations
- `g_int_length` - number of bytes for integer part
- `g_bn_length` - total big number length in bytes
- `g_r_length` - reserved length for big number
- `g_padding` - padding bytes
- `g_decimals` - decimal precision for big numbers
- `g_shift_factor` - shift factor for fixed-point operations
- `g_bf_length` - big float length in bytes
- `g_r_bf_length` - reserved length for big floats
- `g_bf_decimals` - decimal precision for big floats

## 2. Big Number Temporary Variables Group

**File:** `big.h`

Temporary big number working variables (g_r_length bytes each):

- `g_bn_tmp1` through `g_bn_tmp6` - six general-purpose BigNum temporaries
- `g_bn_tmp_copy1` - BigNum copy buffer (g_bn_length bytes)
- `g_bn_tmp_copy2` - BigNum copy buffer (g_bn_length bytes)
- `g_bn_pi` - BigNum constant for ?
- `g_bn_tmp` - additional BigNum temporary

## 3. Big Float Temporary Variables Group

**File:** `big.h`

Temporary big float working variables (g_r_bf_length+2 bytes each):

- `g_bf_tmp1` through `g_bf_tmp6` - six general-purpose BigFloat temporaries
- `g_bf_tmp_copy1` - BigFloat copy buffer
- `g_bf_tmp_copy2` - BigFloat copy buffer
- `g_bf_pi` - BigFloat constant for ?
- `g_bf_tmp` - additional BigFloat temporary (g_r_bf_length bytes)
- `g_bf10_tmp` - BigFloat base-10 temporary (dec+4 bytes)
- `g_big_pi` - Generic Big constant for ?

## 4. Big Number Fractal Coordinate Group

**File:** `big.h`

Big number variables for fractal region coordinates (g_bn_length bytes each):

- `g_x_min_bn` - minimum x coordinate
- `g_x_max_bn` - maximum x coordinate
- `g_y_min_bn` - minimum y coordinate
- `g_y_max_bn` - maximum y coordinate
- `g_x_3rd_bn` - third corner x coordinate (for rotated views)
- `g_y_3rd_bn` - third corner y coordinate
- `g_delta_x_bn` - x increment per pixel
- `g_delta_y_bn` - y increment per pixel
- `g_delta2_x_bn` - secondary x delta
- `g_delta2_y_bn` - secondary y delta
- `g_close_enough_bn` - convergence threshold

## 5. Big Number Fractal State Group

**File:** `big.h`

Big number complex variables for fractal iteration (various lengths):

- `g_tmp_sqr_x_bn` - squared x value (g_r_length)
- `g_tmp_sqr_y_bn` - squared y value (g_r_length)
- `g_old_z_bn` - previous z value (g_bn_length, BNComplex)
- `g_param_z_bn` - parameter z value (g_bn_length, BNComplex)
- `g_saved_z_bn` - saved z value (g_bn_length, BNComplex)
- `g_new_z_bn` - new z value (g_r_length, BNComplex)

## 6. Big Float Fractal Coordinate Group

**File:** `big.h`

Big float variables for fractal region coordinates (g_r_bf_length+2 bytes each):

- `g_delta_x_bf` - x increment per pixel
- `g_delta_y_bf` - y increment per pixel
- `g_delta2_x_bf` - secondary x delta
- `g_delta2_y_bf` - secondary y delta
- `g_close_enough_bf` - convergence threshold
- `g_tmp_sqr_x_bf` - squared x value
- `g_tmp_sqr_y_bf` - squared y value

## 7. Big Float Fractal State Group

**File:** `big.h`

Big float complex variables for fractal iteration (g_r_bf_length+2 bytes each, BFComplex):

- `g_param_z_bf` - parameter z value
- `g_saved_z_bf` - saved z value
- `g_old_z_bf` - previous z value
- `g_new_z_bf` - new z value

## 8. Big Float Fractal Bounds Group

**File:** `big.h`

Big float variables for fractal region bounds (g_bf_length+2 bytes each):

- `g_bf_x_min` - minimum x coordinate
- `g_bf_x_max` - maximum x coordinate
- `g_bf_y_min` - minimum y coordinate
- `g_bf_y_max` - maximum y coordinate
- `g_bf_x_3rd` - third corner x coordinate
- `g_bf_y_3rd` - third corner y coordinate
- `g_bf_save_x_min` - saved minimum x coordinate
- `g_bf_save_x_max` - saved maximum x coordinate
- `g_bf_save_y_min` - saved minimum y coordinate
- `g_bf_save_y_max` - saved maximum y coordinate
- `g_bf_save_x_3rd` - saved third corner x
- `g_bf_save_y_3rd` - saved third corner y
- `g_bf_params[10]` - array of 10 big float parameters ((g_bf_length+2)*10 bytes total)

## Usage Patterns

These variable groups follow typical arbitrary precision math patterns:

1. **Precision Initialization**:
   - Set `g_bf_math` mode (BIG_NUM or BIG_FLT)
   - Calculate lengths with `calc_lengths()`
 - Allocate temporary variables based on calculated lengths
   - Initialize constants like `g_bn_pi` or `g_bf_pi`

2. **Fractal Coordinate Setup**:
   - Set region bounds (x_min, x_max, y_min, y_max, x_3rd, y_3rd)
   - Calculate per-pixel deltas (delta_x, delta_y)
   - Set convergence threshold (close_enough)

3. **Fractal Iteration**:
   - Initialize iteration variables (old_z, new_z, param_z)
   - Use temporary variables for intermediate calculations
   - Store squared values in tmp_sqr_x and tmp_sqr_y
   - Save state in saved_z when needed

4. **Arithmetic Operations**:
   - Use tmp1-tmp6 for intermediate results
   - Use tmp_copy1 and tmp_copy2 for value preservation
   - Perform operations with bn/bf function library
   - Convert between BigNum and BigFloat as needed

## Relationships with Other Global Groups

Math globals work with other subsystems:

- **Arbitrary precision variables** provide high-precision alternatives to standard double-precision fractal coordinates (`ImageRegion.h`)
- **Big number state variables** replace standard complex variables (`calcfrac.h`) when precision requires it
- **Configuration variables** control when arbitrary precision is used vs. standard floating point
- **Temporary variables** are used throughout fractal calculation engines
- **Parameter arrays** extend standard fractal parameters with arbitrary precision

## Arbitrary Precision Architecture

The arbitrary precision system supports two number types:

### BigNum (Fixed-Point Decimal)
- Integer part: `BN_INT_LENGTH` bytes (typically 4)
- Fractional part: remaining bytes
- Total length: `g_bn_length` bytes
- Operations: exact decimal arithmetic
- Use case: when exact decimal values are required

### BigFloat (Floating-Point)
- Mantissa: most of allocated bytes
- Exponent: separate field
- Total length: `g_bf_length` bytes + 2 overhead
- Operations: normalized floating-point arithmetic
- Use case: when very large or very small numbers are required

The choice between BigNum and BigFloat depends on the fractal type and zoom level.

## Memory Management

Arbitrary precision variables use dynamic memory:

- Lengths are calculated based on required precision
- Variables are allocated once at the start of calculation
- Memory is reused across iterations for efficiency
- Temporary variables avoid repeated allocation/deallocation
- All variables must be freed when switching precision modes

The system allocates approximately:
- BigNum: 6 temps + 2 copies + Pi + calculation state ? 10+ allocations
- BigFloat: 6 temps + 2 copies + Pi + calculation state ? 10+ allocations
- Fractal coordinates: 11 values for BigNum + 7 for BigFloat
- Total: ~30 large allocations for full arbitrary precision mode

## Refactoring Considerations

When refactoring to reduce global state in math code, consider:

1. **Precision Context**: Configuration variables (bf_math, lengths, decimals) could become a `PrecisionContext` class that manages precision settings.

2. **Big Number Pool**: Temporary variables could become a `BigNumPool` class that manages allocation and reuse of temporaries.

3. **Big Float Pool**: Similarly for BigFloat temporaries with a `BigFloatPool` class.

4. **Fractal Coordinates**: Coordinate variables could become a `BigCoordinateSystem` class with methods for conversion and delta calculation.

5. **Iteration State**: State variables could become an `IterationState<T>` template class parameterized by number type.

6. **Arithmetic Engine**: All arithmetic operations could be encapsulated in a `BigMath` singleton or namespace.

The math variables are challenging to refactor because:
- They're used in the tightest performance-critical loops
- Arbitrary precision operations are already expensive
- Adding indirection could significantly impact performance
- The global temporaries avoid repeated allocation overhead
- Many fractal calculation functions directly access these globals

## Cross-Cutting Concerns

Several math globals affect the entire calculation engine:

- `g_bf_math` determines which calculation paths are used throughout the codebase
- Coordinate variables replace standard double coordinates in high-precision mode
- Temporary variables are accessed by all arithmetic operations
- The ? constants are used in trigonometric calculations
- Delta values affect per-pixel coordinate calculations in the main rendering loop

These variables are deeply embedded in the calculation engine and would require a comprehensive refactoring effort to eliminate.

## Performance Considerations

Arbitrary precision math is expensive:

- BigNum operations are 10-100× slower than double precision
- BigFloat operations are 100-1000× slower than double precision
- Memory allocation overhead is significant
- Cache locality matters for temporary variables
- Reusing temporaries is crucial for performance

The global temporary variables exist specifically to avoid allocation overhead in inner loops. Any refactoring must preserve this performance characteristic.

## Precision Selection

The system chooses precision based on zoom level:

1. **Standard Double** (no big math): Zoom levels up to ~10^15
2. **BigNum**: Zoom levels from 10^15 to 10^60 (limited by decimal precision)
3. **BigFloat**: Zoom levels beyond 10^60 (limited only by memory)

The `g_bf_math` variable switches between these modes, and all fractal calculation code checks this variable to select the appropriate code path.

## Complex Number Support

The system provides complex number types for arbitrary precision:

- **BNComplex** - Complex<BigNum> for fixed-point complex arithmetic
- **BFComplex** - Complex<BigFloat> for floating-point complex arithmetic

These types enable complex fractal calculations (Mandelbrot, Julia, etc.) at arbitrary precision using the same iteration formulas as standard precision.

## Constants and Initialization

Mathematical constants are pre-calculated:

- `g_bn_pi` - ? in BigNum format
- `g_bf_pi` - ? in BigFloat format  
- `g_big_pi` - ? in generic Big format

These constants are initialized once and reused throughout calculations, avoiding repeated expensive computation of ?.

## Conversion and Interchange

The system provides conversion between precision modes:

- `float_to_bn()` / `bn_to_float()` - BigNum ? double
- `float_to_bf()` / `bf_to_float()` - BigFloat ? double
- `bf_to_bn()` / `bn_to_bf()` - BigFloat ? BigNum
- `convert_bn()` / `convert_bf()` - resize allocated precision

These conversions enable switching precision modes or exporting results to standard precision for display.

## Safety and Validation

Many operations have "unsafe" and "safe" variants:

- **Unsafe**: Assumes inputs are valid, no error checking, faster
- **Safe**: Validates inputs, checks for errors, safer but slower

For example:
- `unsafe_mult_bn()` vs. `mult_bn()`
- `unsafe_div_bf()` vs. `div_bf()`

The unsafe variants are used in inner loops where inputs are guaranteed valid, while safe variants are used in user-facing code.

## String Formatting

The system supports multiple output formats:

- `bn_to_str()` - standard decimal notation
- `bf_to_str()` - standard decimal notation
- `bf_to_str_e()` - scientific (exponential) notation
- `bf_to_str_f()` - fixed-point notation

These functions require temporary buffers and return formatted strings for display or file output. The required buffer length is provided by `strlen_needed_bn()` and `strlen_needed_bf()`.
