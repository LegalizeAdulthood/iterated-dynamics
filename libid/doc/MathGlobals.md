<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Math Global Variables

Inventory of public `g_*` declarations in `libid/include/math`.
Lengths and offsets below describe the current allocator in
`libid/math/biginit.cpp`; some older comments in `big.h` differ from the
actual reservations.

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

`BigNum` is binary fixed-point, not exact decimal arithmetic.
`BigFloat` has a base-256 mantissa and a two-byte exponent.  `Big`,
`BigNum`, and `BigFloat` are raw byte-pointer aliases; `BNComplex` and
`BFComplex` contain two such pointers and do not own their storage.

Precision selection depends on coordinate/resolution error, overrides,
and the fractal's supported alternate math.  There is no fixed zoom ladder
from double to BigNum to BigFloat.  Current supported `g_bn_length`
limits are 672 bytes for `BIG_NUM` and 708 for `BIG_FLT`; arena capacity
does not increase those representation limits.

## Arena And Scratch Stack

Header: `biginit.h`.

- `g_bf_digits` - explicit decimal-precision override.
- `g_bf_save_len` - precision length of the retained corner/parameter area.
- `g_start_stack` - scratch-stack starting byte offset.
- `g_max_stack` - scratch-stack upper bound, before persistent values.
- `g_bignum_max_stack_addr` - high-water byte offset.

The allocator partitions one static 1 MiB arena into working values,
scratch stack, and a persistent area containing six corners, six saved
corners, and ten parameters.  The pointers listed below designate slices
of that arena, not separate heap allocations.

`init_bf_dec()` and `init_bf_length()` establish lengths and bindings.
`alloc_stack()`, `save_stack()`, `restore_stack()`, and `BigStackSaver`
manage temporary storage.  Rejected precision and stack requests preserve
the active allocation state.  Exhaustion raises `PrecisionError`;
uninitialized stack use raises `std::logic_error`, without UI calls.

`free_bf_vars()` resets allocator and active-math state.  It does not free
individual buffers or release the static arena; retained pointer values
must not be used as evidence that an arena is active.

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
- `g_bf10_tmp` - decimal-conversion temporary, `g_bf_decimals + 4` bytes.
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

Shared arena, temporaries, lengths, and interpreter bindings are not
thread-isolated.  Future ownership work belongs in
[hexagonal.md](hexagonal.md), not an assumed performance or zoom model.
