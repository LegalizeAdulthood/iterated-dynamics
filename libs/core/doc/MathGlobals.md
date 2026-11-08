<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Math Global Variables

Inventory of public `g_*` declarations in `libs/core/include/math`. Lengths
and offsets below describe the current allocator in
`libs/core/math/biginit.cpp`; some older comments in `big.h` differ from
the actual reservations.

## Numeric Representation And Precision

Header: `big.h`.

- `g_bf_math` - active representation: `NONE`, `BIG_NUM`, or `BIG_FLT`.
- `g_bn_step` - byte-length alignment, set to four by `calc_lengths()`;
  not an iteration step.
- `g_int_length` - fixed-point integer bytes, selected as one, two, or four
  from fractal and bailout requirements.
- `g_bn_length` - fixed-point number length in bytes.
- `g_padding` - extra multiplication working bytes.
- `g_r_length` - `g_bn_length + g_padding`.
- `g_decimals` - requested decimal precision.
- `g_shift_factor` - offset used by partial fixed-point multiplication.
- `g_bf_length` - floating-point mantissa length.
- `g_r_bf_length` - mantissa working length including padding.
- `g_bf_decimals` - decimal precision derived from mantissa length.

`BigNumPtr` points to binary fixed-point storage, not exact decimal
arithmetic. `BigFloatPtr` points to a base-256 mantissa and a two-byte
exponent. `Big`, `BigNumPtr`, and `BigFloatPtr` are raw byte-pointer
aliases; `BNPtrComplex` and `BFPtrComplex` contain two such pointers and
do not own their storage.

`BigFloat` in `math/BigFloat.h` owns canonical value bytes independently
of the arena. Copies, moves, destruction, and legacy-buffer export use
the vector's own storage size. Constructors use the active legacy
lengths. Arithmetic, scalar conversion, parsing, and formatting use the
existing global precision and shared scratch. Callers establish a
compatible context, as with the raw routines; there is no custom precision
object or per-operation initialization or precision validation.

Arithmetic operators preserve their operands. Compound assignments update
only their destination, including self-assignment. Multiplication owns its
wider workspace and returns canonical bytes. Numerical comparisons read
values directly. Operators and temporary raw adapters share private
arithmetic kernels in `math/BigFloat.cpp`.

`BigNum` in `math/BigNum.h` owns canonical fixed-point bytes with the same
lifetime and active-context contract. Its little-endian representation
uses `g_int_length` high bytes for the signed integer part and
`g_bn_length - g_int_length` low bytes for the fractional part. Parsing
uses the legacy fixed-decimal syntax; formatting retains legacy output.
`BigNumConstPtr` denotes read-only legacy bytes.

`BigNum` member operators and `sqr()` share private arithmetic kernels
with legacy adapters in `math/BigNum.cpp`. Multiplication and squaring
use separate work buffers and copy the `g_bn_length` bytes at
`g_shift_factor` into the owning result. Addition, subtraction, and
comparisons read operands directly; destructive kernels receive scratch
copies. Truncation, fixed-width overflow, division saturation, and zero
handling retain legacy behavior.

Legacy Newton iterations can retain guard-digit differences from reused
destination bytes. Value division, logarithms, and arctangents start with
fresh results. Compound division agrees with the corresponding value
expression. Comparisons with legacy calculations retain the requested
decimal precision rather than depending on old destination contents.

`BNComplex` and `BFComplex` in `math/BigComplex.h` own two scalar values
through `Complex<T>`. Member multiplication and division compute both
components before assignment. The legacy complex signatures import their
inputs before exporting results, including when outputs alias inputs.
`BFConstPtrComplex` provides a read-only view for these imports.

Complex `log` and `pow` use named BigFloat value functions backed by the
private legacy kernels. Trigonometric calculations still require
`init_big_pi()` in the active context. Complex division by zero sets
`g_overflow` and returns zero. Complex logarithms of zero and complex
powers with a zero base return zero. Integer `power` retains the separate
legacy rule that nonpositive degrees return one. Existing branch and
sine/cosine axis behavior is retained.

Explicit constructors convert between `BigNum` and `BigFloat`. Their
conversion export adapters write directly into destination storage with
`bn_to_bf` and `bf_to_bn`, preserving the existing truncation, saturation,
and underflow behavior without a machine floating-point intermediate.

Precision selection depends on coordinate/resolution error, overrides,
and the fractal's supported alternate math.  There is no fixed zoom ladder
from double to BigNum to BigFloat.  Current supported `g_bn_length`
limits are 672 bytes for `BIG_NUM` and 708 for `BIG_FLT`; arena capacity
does not increase those representation limits.

## Arena And Scratch Stack

Header: `big_init.h`.

- `g_bf_digits` - explicit decimal-precision override.
- `bn_arena()` and `bf_arena()` select persistent, working, or temporary
  storage for their representation through `BigLifetime`.
- `big_memory_stats()` reports global and temporary high-water payload
  bytes and total payload capacity across the six arenas.

Each arena starts with about 1 MiB of payload capacity. Blocks and backing
storage are aligned to 64 bytes. Precision changes replace backing storage
at the new stride while preserving live allocation indices and widths.
All six replacements are prepared before any new layout is published.
The six corners, ten parameters, and six saved corners use persistent
BigFloat blocks. Image boundaries explicitly pack these 22 values in
their historical order, excluding block padding and allocator metadata.

`init_bf_dec()` and `init_bf_length()` establish lengths and bindings.
`bn_stack()` and `bf_stack()` retain handles for legacy raw workspaces;
their `allocate_single()` and `allocate_double()` requests take no byte
count. `save_stack()`, `restore_stack()`, and `BigStackSaver` mark and
rewind both adapters together. Rejected precision and stack requests
preserve the active allocation state. Exhaustion raises `PrecisionError`;
uninitialized stack use raises `std::logic_error`, without UI calls.

`free_bf_vars()` clears active raw bindings and releases stack-adapter
reservations. The arenas remain alive for attached owning handles;
subsequent initialization migrates these values and creates fresh active
corner bindings. `calc_lengths()` remains allocation-free for temporary
Newton working-length changes within already reserved storage.

## BigNum Temporaries

Header: `big.h`.

- `g_bn_tmp1`, `g_bn_tmp2`, `g_bn_tmp3`, `g_bn_tmp4`, `g_bn_tmp5`,
  `g_bn_tmp6` - general temporaries, each `g_r_length` bytes.
- `g_bn_tmp_copy1`, `g_bn_tmp_copy2` - operand-preservation buffers, each
  `2 * g_r_length` bytes.
- `g_bn_tmp` - additional fixed-point temporary, `g_r_length` bytes.
- `g_bn_pi` - fixed-point view into shared Pi storage, not another buffer.

## BigFloat Temporaries

Header: `big.h`.

- `g_bf_tmp1`, `g_bf_tmp2`, `g_bf_tmp3`, `g_bf_tmp4`, `g_bf_tmp5`,
  `g_bf_tmp6` - general temporaries, each `g_r_bf_length + 2` bytes.
- `g_bf_tmp_copy1`, `g_bf_tmp_copy2` - preservation buffers, each
  `2 * (g_r_bf_length + 2)` bytes.
- `g_bf_tmp` - additional temporary, `g_r_bf_length + 2` bytes.
- `g_big_pi` - shared Pi storage.
- `g_bf_pi` - floating-point alias of `g_big_pi`.

`init_big_pi()` copies the appropriate precision from a constant table.
`g_bf_pi` aliases its start and `g_bn_pi` points into the same storage;
the three names are not independently allocated constants.

## BigNum Coordinates And Iteration State

Header: `big.h`.

Each coordinate or increment below reserves `g_bn_length` bytes:

- `g_x_min_bn`, `g_x_max_bn`, `g_y_min_bn`, `g_y_max_bn` - image bounds.
- `g_x_3rd_bn`, `g_y_3rd_bn` - third corner.
- `g_delta_x_bn`, `g_delta_y_bn`, `g_delta2_x_bn`, `g_delta2_y_bn` -
  pixel-coordinate increments.
- `g_close_enough_bn` - comparison threshold.

Iteration state uses these reservations:

- `g_tmp_sqr_x_bn`, `g_tmp_sqr_y_bn` - squared values,
  `g_r_length` bytes each.
- `g_old_z_bn`, `g_new_z_bn` - complex iteration values,
  `g_r_length` bytes per component.
- `g_param_z_bn`, `g_saved_z_bn` - complex parameter and saved value,
  `g_bn_length` bytes per component.

## BigFloat Coordinates And Iteration State

Header: `big.h`.

- `g_delta_x_bf`, `g_delta_y_bf`, `g_delta2_x_bf`, `g_delta2_y_bf` -
  increments, each `g_bf_length + 2` bytes.
- `g_close_enough_bf` - threshold, `g_bf_length + 2` bytes.
- `g_tmp_sqr_x_bf`, `g_tmp_sqr_y_bf` - squared values,
  each `g_r_bf_length + 2` bytes.
- `g_old_z_bf`, `g_new_z_bf` - complex iteration values,
  `g_r_bf_length + 2` bytes per component.
- `g_param_z_bf`, `g_saved_z_bf` - complex parameter and saved value,
  `g_bf_length + 2` bytes per component.

## Persistent BigFloat Bounds And Parameters

Header: `big.h`.

Each scalar below reserves `g_bf_length + 2` bytes in the persistent area:

- `g_bf_x_min`, `g_bf_x_max`, `g_bf_y_min`, `g_bf_y_max` - bounds.
- `g_bf_x_3rd`, `g_bf_y_3rd` - third corner.
- `g_bf_save_x_min`, `g_bf_save_x_max`, `g_bf_save_y_min`,
  `g_bf_save_y_max`, `g_bf_save_x_3rd`, `g_bf_save_y_3rd` - saved bounds.
- `g_bf_params` - ten scalar parameter bindings.

The complete serialized area occupies `22 * (g_bf_length + 2)` bytes.
It is retained across scratch-stack use and converted when precision
changes.

## Double Interpreter And Trigonometric Bindings

Header: `arg.h`.

- `g_arg1`, `g_arg2` - active `Arg` operand pointers.
- `g_d_trig0`, `g_d_trig1`, `g_d_trig2`, `g_d_trig3` - selected
  double-valued trigonometric operation pointers.

Formula runtime stack ownership belongs to `fractals::g_runtime`.
These arithmetic bindings still share state with formula/trig execution;
they are not isolated per-render contexts.

## Fixed-Point Overflow

Header: `fixed_pt.h`.

- `g_overflow` - overflow indication for legacy fixed-point operations.

## Arithmetic And Conversion Contracts

Arithmetic uses shared temporaries.  The distinction between ordinary and
`unsafe_*` operations primarily concerns destructive operand use:
preserving wrappers copy operands before calling destructive arithmetic.
It is not a general validated-versus-unchecked input contract.

Conversions include `float_to_bn()`, `bn_to_float()`, `float_to_bf()`,
`bf_to_float()`, `bn_to_bf()`, `bf_to_bn()`, `convert_bn()`, and
`convert_bf()`.  The name `float` in these APIs does not necessarily mean
C++ `float`; consult their declarations for `LDouble` arguments/results.

`bn_to_string()`, `bf_to_string()`, `bf_to_string_e()`, and
`bf_to_string_f()` return `std::string`.  There is no current
`bn_to_str()` caller-buffer API.

BigFloat formatting keeps its decimal representation and conversion
helpers private to the formatter. Each call owns its decimal scratch
buffer; this storage is outside the shared arena.

Shared arena, temporaries, lengths, and interpreter bindings are not
thread-isolated.  Future ownership work belongs in
[hexagonal.md](hexagonal.md), not an assumed performance or zoom model.
