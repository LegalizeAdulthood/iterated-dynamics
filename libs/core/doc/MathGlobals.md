<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Math Global Variables

This document describes the current owning values, numeric setup, and
saved-file compatibility in the core math layer.

## Numeric Representation And Precision

Headers: `big_init.h`, `BigFloat.h`, and `MPConversions.h`.

- `g_bf_math` selects native or arbitrary-precision calculations.
- `g_decimals` records requested decimal precision; `g_bf_digits` is the
  explicit user override.
- `g_bn_length` and `g_int_length` retain the existing file-precision
  policy and metadata interpretation.
- `g_bf_length` and `g_bf_decimals` describe legacy mantissa layouts and
  saved-file precision metadata.

BigFloat owns an optional Boost.Multiprecision MPReal. Default
construction is null; numeric construction selects working precision
through MPPrecision. Copies and assignments retain the source value's
precision. Moves transfer ownership and leave the source null. Values
remain valid after numeric-context cleanup or precision changes.

Ordinary arithmetic and math functions use Boost. The combined
sine/cosine operation uses mpfr_sin_cos to share argument reduction.
BFComplex owns two BigFloat components. Complex logarithm and power use
MPC temporaries inside the math layer. Logarithms of zero and powers with
a zero base return zero; integer powers with nonpositive degrees return
one. Complex division by zero sets `g_overflow` and returns zero.

MPConversions encodes and decodes legacy binary significands and
exponents without native floating-point intermediates. Lossy encoding
rounds to nearest with ties to even, rejects non-finite values and
overflow, and canonicalizes signed zero. Legacy layouts are a codec
concern rather than owning-value storage.

The supported `g_bn_length` limit remains 708 bytes for every
arbitrary-precision fractal. Automatic owner precision retains native
scalar bits; explicit `bfdigits` overrides remain authoritative.

### Value Storage Measurements

The arena measurement tool is retired with arena-backed owning values.
These historical Windows/MSVC Release measurements at 40 decimal digits
predate the BigFloat-only cutover and compare vector storage with indexed
arena handles. They do not describe current Boost allocation behavior.
Times are medians of three runs in milliseconds; heap counts are calls
to the replacement `operator new`.

| Case | Reps | Heap before | Heap after | ms before | ms after |
|---|---|---|---|---|---|
| BN scalar | 10000 | 50000 | 0 | 2.688 | 1.819 |
| BN complex power | 10000 | 450000 | 0 | 23.866 | 17.019 |
| BN Julia orbit | 10000 | 0 | 0 | 2.373 | 2.473 |
| BF scalar | 10000 | 30000 | 0 | 4.062 | 3.654 |
| BF complex power | 10000 | 310000 | 0 | 38.118 | 33.030 |
| BF Julia orbit | 10000 | 0 | 0 | 5.102 | 5.262 |
| BF Julia power orbit | 250 | 6250 | 0 | 35.503 | 36.203 |

### BigFloat Cutover Measurements

The retired measurement tool recorded this BigFloat-only baseline on
Windows/MSVC, Release, at 40 decimal digits. Times are medians of three
warmed runs and depend on the machine and its load.

| Case | Reps | Median ms | Heap allocations | Arena growth |
|---|---|---|---|---|
| BF scalar | 10000 | 5.056 | 0 | 0 |
| BF complex power | 10000 | 43.930 | 0 | 0 |
| BF Julia orbit | 10000 | 8.237 | 0 | 0 |
| BF Julia power orbit | 250 | 50.621 | 0 | 0 |

The quadratic-orbit accuracy check compares twenty steps at 32- and
64-byte legacy lengths with a 400-digit MPFR reference. Its seeds and
parameters include differences below native floating-point precision.

All three deep migration views compare equal to the original golds in
indexed pixels and palette after the BigFloat cutover.

## Arena And Scratch Stack

The arena classes, byte stacks, lifetime selection, product reservations,
and their allocation tests are removed. Numeric setup initializes
owning globals directly. Precision changes resize preserved current and
saved corners; changes to the legacy byte layout repopulate the ten
parameter values from native render parameters. Repeated setup at the
same precision preserves parameter values and clears working state.
`free_bf_vars()` resets globals to null.

## BigNum Temporaries

Removed with the BigNum rendering implementation.

## BigFloat Temporaries

Header: `BigFloat.h`.

`g_bf_tmp` is an owning scalar used by existing orbit and bailout code.
The unused kernel owners, paired copy buffers, global Pi value, and Pi
setup calls are removed. BigFloat operations retain their own Boost
values; trigonometric functions require no global Pi initialization.

## BigNum Coordinates And Iteration State

Removed with the fixed-point Mandelbrot and Julia implementations.
Coordinates, square caches, periodicity state, sticky orbits, and
perturbation continuation use the BigFloat values below.

## BigFloat Coordinates And Iteration State

Header: `BigFloat.h`.

- `g_delta_x_bf`, `g_delta_y_bf`, `g_delta2_x_bf`, `g_delta2_y_bf` -
  owning `BigFloat` increments.
- `g_close_enough_bf` - owning `BigFloat` threshold.
- `g_tmp_sqr_x_bf`, `g_tmp_sqr_y_bf` - owning `BigFloat` square caches.
- `g_old_z_bf`, `g_new_z_bf`, `g_saved_z_bf`, `g_param_z_bf` - owning
  `BFComplex` iteration values and parameter.

These globals start null. Numeric setup initializes and clears working
values directly; cleanup releases their owned values. Bailout thresholds
are ordinary owning temporaries. Render-state ownership and continuation
interfaces retain their existing organization.

## Persistent BigFloat Bounds And Parameters

Header: `BigFloat.h`.

Each scalar below owns an MPReal:

- `g_bf_x_min`, `g_bf_x_max`, `g_bf_y_min`, `g_bf_y_max` - bounds.
- `g_bf_x_3rd`, `g_bf_y_3rd` - third corner.
- `g_bf_save_x_min`, `g_bf_save_x_max`, `g_bf_save_y_min`,
  `g_bf_save_y_max`, `g_bf_save_x_3rd`, `g_bf_save_y_3rd` - saved bounds.
- `g_bf_params` - ten scalar parameter bindings.

The serialized payload occupies `22 * (g_bf_length + 2)` bytes in its
historical field order. Packing and unpacking retain this binary format;
live values have no arena identity or encoded-size field.

## Double Interpreter And Trigonometric Bindings

Header: `arg.h`.

- `g_arg1`, `g_arg2` - active `Arg` operand pointers.
- `g_d_trig0`, `g_d_trig1`, `g_d_trig2`, `g_d_trig3` - selected
  double-valued trigonometric operation pointers.

Formula runtime stack ownership belongs to `fractals::g_runtime`. These
arithmetic bindings still share state with formula/trig execution; they are
not isolated per-render contexts.

## Fixed-Point Overflow

Header: `fixed_pt.h`.

- `g_overflow` - overflow indication for legacy fixed-point operations.

## Arithmetic And Conversion Contracts

BigFloat arithmetic preserves operands and materializes owning results.
Precision is selected at construction or explicit changes; ordinary
operations use Boost precision propagation and default rounding. Complex
library functions preserve the documented branch and zero conventions.
Formatting retains the existing notation and decimal carry conventions.

The old byte arithmetic, normalization, Pi tables, and native conversion
routines are removed. MPConversions exposes only binary file-record
encoding and decoding. BigFloat packing and unpacking use that codec;
there are no live-value conversion bridges or alternate byte backend.

Numeric setup, render state, file-layout metadata, and interpreter bindings
remain global. Future render-ownership work belongs in
[hexagonal.md](hexagonal.md).
