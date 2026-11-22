<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Math Global Variables

Inventory of public `g_*` declarations in `libs/core/include/math`. Lengths
and offsets below describe the current allocator in
`libs/core/math/big_init.cpp`; some older comments in `big.h` differ from
the actual reservations.

## Numeric Representation And Precision

Headers: `big.h`, `big_init.h`, and `BigFloat.h`.

- `g_bf_math` - active representation: `NONE` or `BIG_FLT`.
- `g_bn_step` - byte-length alignment, set to four by `calc_lengths()`.
- `g_int_length` - integer-byte count for internal fixed-point kernels.
- `g_bn_length` - internal fixed-point working length in bytes.
- `g_padding` - extra multiplication working bytes.
- `g_r_length` - `g_bn_length + g_padding`.
- `g_decimals` - requested decimal precision.
- `g_shift_factor` - offset used by partial fixed-point multiplication.
- `g_bf_length` - floating-point mantissa length.
- `g_r_bf_length` - mantissa working length including padding.
- `g_bf_decimals` - decimal precision derived from mantissa length.

Mandelbrot and Julia use the BigFloat orbit, pixel, and image functions.
The owning BigNum type, its rendering state, and its arenas are removed.
BigFloat still uses fixed-point byte primitives for mantissa arithmetic and
native conversion. Remaining `g_bn_*` lengths serve those primitives.

BigFloat owns an indexed arena allocation containing a signed base-256
mantissa and a two-byte exponent. Default construction is null. Numeric
construction and eager results use the working arena unless a compatible
arena is selected explicitly. Copies retain the source allocator;
assignments retain the destination allocator. Moves within an arena
transfer a block; moves across arenas copy the value. Moved-from objects
retain their allocator binding.

The allocator separates canonical size, aligned stride, and allocation
width. Live owning handles survive `free_bf_vars()` at the same layout.
Precision changes replace the numeric context and preserve current and
saved corners. Arithmetic, parsing, native conversion, and formatting
require a compatible active context and share scratch values.

Arithmetic preserves its operands, including self-assignment.
Multiplication and squaring normalize a paired wide product and reduce the
canonical mantissa and exponent into one block. Unsigned scalar operations
retain legacy rounding, normalization, and zero-divisor behavior. Newton
iterations can retain guard-digit differences from reused destination
bytes; value operations start with fresh results.

BFComplex owns two BigFloat components. Complex multiplication and division
materialize both results before assignment. Logarithms of zero and powers
with a zero base return zero. Integer powers with nonpositive degrees
return one. Division by zero sets `g_overflow` and returns zero.
Trigonometric operations require `init_big_pi()` in the active context.

MPConversions bridges BigFloat and MPReal through binary significands and
exponents. Imports use MPPrecision. Exports use a selected legacy arena.
Lossy conversions round to nearest with ties to even. Exports reject non-
finite inputs and overflow and canonicalize signed zero. Conversions do not
use a native floating-point intermediate.

Precision selection depends on coordinate/resolution error, overrides, and
alternate math. The supported `g_bn_length` limit is 708 bytes for every
arbitrary-precision fractal, independently of arena capacity.

### Value Storage Measurements

`check-big-value-storage` measures warmed scalar expressions, integer
complex powers, and the actual Julia orbit callbacks. Its allocation hooks
are confined to the measurement executable. The following historical
results predate the BigFloat-only cutover and compare vector storage with
indexed arena handles on Windows/MSVC, Release, at 40 decimal digits. Times
are medians of three runs in milliseconds; heap counts are calls to the
replacement `operator new`.

| Case | Reps | Heap before | Heap after | ms before | ms after |
|---|---|---|---|---|---|
| BN scalar | 10000 | 50000 | 0 | 2.688 | 1.819 |
| BN complex power | 10000 | 450000 | 0 | 23.866 | 17.019 |
| BN Julia orbit | 10000 | 0 | 0 | 2.373 | 2.473 |
| BF scalar | 10000 | 30000 | 0 | 4.062 | 3.654 |
| BF complex power | 10000 | 310000 | 0 | 38.118 | 33.030 |
| BF Julia orbit | 10000 | 0 | 0 | 5.102 | 5.262 |
| BF Julia power orbit | 250 | 6250 | 0 | 35.503 | 36.203 |

Every case had zero arena high-water growth after warmup, before and after
the change. Arena free lists reuse released value blocks without repeated
backing allocations. Scalar and complex timings improved in these runs;
fractal timings were slightly slower. These short timings depend on the
machine and load and are not a rendering-speed guarantee.

Build the `check-big-value-storage` target and run
`tests/tools/Release/check-big-value-storage.exe` from the build directory
to repeat the measurement.

### BigFloat Cutover Measurements

The BigFloat-only cutover baseline uses Windows/MSVC, Release, at 40
decimal digits. The table records medians of three warmed runs of
`check-big-value-storage`; timings depend on the machine and its load.
Repeat this configuration when comparing the Boost implementation.

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

Header: `big_init.h`.

- `g_bf_digits` - explicit decimal-precision override.
- `bf_arena()` selects persistent, working, or temporary BigFloat
  storage through BigLifetime.
- `big_memory_stats()` reports high-water payload bytes and capacity
  across the three arenas.

Each arena starts with about 1 MiB of payload capacity. Blocks and backing
storage are aligned to 64 bytes. Precision changes rebuild the context,
preserving six current and six saved corners. The ten parameter bindings
are initialized from render parameters. These 22 persistent values retain
their historical serialized order, excluding allocator metadata.

`init_bf_dec()` and `init_bf_length()` establish lengths and bindings.
`bf_stack()` retains handles for borrowed raw workspaces. Allocation
requests take no byte count. BigStackMark contains one BigFloat stack mark;
`save_stack()`, `restore_stack()`, and BigStackSaver manage it. Exhaustion
raises PrecisionError. Uninitialized stack use raises std::logic_error.
Rejected requests preserve active allocation state.

`free_bf_vars()` unbinds active globals and releases stack reservations.
Other attached handles survive until the context is replaced.
`calc_lengths()` remains allocation-free for temporary Newton lengths.

## BigNum Temporaries

Removed with the BigNum rendering implementation. BigFloat owns the
remaining scratch values and paired operand-preservation buffers.

## BigFloat Temporaries

Header: `big.h`.

- `g_bf_tmp1`, `g_bf_tmp2`, `g_bf_tmp3`, `g_bf_tmp4`, `g_bf_tmp5`,
  `g_bf_tmp6` - owning canonical `BigFloat` kernel workspaces, each in one
  working-arena slot. They hold `g_bf_length + 2` bytes or shortened
  scratch values at the existing calculation sites.
- `g_bf_tmp_copy1`, `g_bf_tmp_copy2` - owning `BigArena::Allocation`
  handles, each reserving two working-arena blocks for operand copies
  and destructive formatter scratch.
- `g_bf_tmp` - owning `BigFloat` temporary, `g_bf_length + 2` encoded
  bytes in the working arena.
- `g_bf_pi` - owning `BigFloat` Pi storage in one working-arena
  slot, with `g_bf_length + 2` canonical bytes.

Single slots are sized from `g_bf_length + 2`, rounded up to 64 bytes.
Multiply and sqr use separate `BigFloatProduct` temporaries, each owning
two adjacent slots for `g_r_bf_length + 2` wide bytes. The existing
reduction keeps canonical bytes in the first slot and releases the second.
Square caches and bailout metrics remain canonical `BigFloat`.

`MathArenas` constructs these six kernel values and two paired buffers in
their existing allocation order. Binding clears each full reservation and
reuses its allocation; unbinding releases it and leaves a null owner.
Precision relocation discards kernel scratch, copy-buffer, and pi bytes.
Live canonical values and products retain their existing conversions.
Temporary stack rewinds do not release working-arena owners.

Logarithm uses a borrowed, shifted window into `g_bf_tmp5`; arctangent does
the same with `g_bf_tmp3`. Their `orig_*` pointers retain the base
addresses while the existing loops change the active lengths. Exponent
cursors and operand-copy pointers are also borrows, not new values.

`init_big_pi()` copies the appropriate precision from a constant table. It
writes the existing `g_bf_pi` slot. Pointer callers borrow `data()`.
Arctangent preserves its existing precision offsets in a local borrowed
pointer and passes that shortened view to the private sine/cosine helper.
The owner keeps its base allocation throughout.

Pi is allocated after the kernel copy buffers. Binding clears the full slot
before the other BigFloat workspaces; repeated binding reuses it. Precision
relocation discards its bytes, as the previous raw binding did. Callers
repopulate the table with `init_big_pi()`; `data()` resolves the relocated
slot. Temporary rewinds leave the working slot intact. Unbinding releases
the owner and leaves `g_bf_pi` null.

The 22 parameter bindings already refer to owning BigFloat values in the
persistent arena. Their construction, precision conversion, and payload
copy sites are unchanged. Pi was the last raw working binding, so the
`WorkingBinding` table and its single/wide binding methods are retired.

## BigNum Coordinates And Iteration State

Removed with the fixed-point Mandelbrot and Julia implementations.
Coordinates, square caches, periodicity state, sticky orbits, and
perturbation continuation use the BigFloat values below.

## BigFloat Coordinates And Iteration State

Header: `big.h`.

- `g_delta_x_bf`, `g_delta_y_bf`, `g_delta2_x_bf`, `g_delta2_y_bf` -
  owning `BigFloat` increments.
- `g_close_enough_bf` - owning `BigFloat` threshold.
- `g_tmp_sqr_x_bf`, `g_tmp_sqr_y_bf` - owning `BigFloat` square caches.
- `g_old_z_bf`, `g_new_z_bf`, `g_saved_z_bf`, `g_param_z_bf` - owning
  `BFComplex` iteration values and parameter.

The owning globals have canonical `g_bf_length + 2` bytes per scalar. Their
handles start null. `MathArenas::bind` initializes them in the BigFloat
working arena and clears existing values on later binds.
`MathArenas::unbind` releases their blocks before arena destruction.
Remaining legacy scalar callers borrow buffers with `BigFloat::data()`.
Each bailout's threshold is an owning temporary-arena value, assigned at
the existing scalar conversion site.

## Persistent BigFloat Bounds And Parameters

Header: `big.h`.

Each scalar below reserves `g_bf_length + 2` bytes in the persistent area:

- `g_bf_x_min`, `g_bf_x_max`, `g_bf_y_min`, `g_bf_y_max` - bounds.
- `g_bf_x_3rd`, `g_bf_y_3rd` - third corner.
- `g_bf_save_x_min`, `g_bf_save_x_max`, `g_bf_save_y_min`,
  `g_bf_save_y_max`, `g_bf_save_x_3rd`, `g_bf_save_y_3rd` - saved bounds.
- `g_bf_params` - ten scalar parameter bindings.

The complete serialized area occupies `22 * (g_bf_length + 2)` bytes. It is
retained across scratch-stack use and converted when precision changes.

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

Arithmetic uses shared temporaries.  The distinction between ordinary and
`unsafe_*` operations primarily concerns destructive operand use:
preserving wrappers copy operands before calling destructive arithmetic. It
is not a general validated-versus-unchecked input contract.

The remaining fixed-point primitives support BigFloat mantissa arithmetic.
Native conversion uses float_to_bf and bf_to_float; convert_bf handles
legacy precision changes. The float spelling does not necessarily mean C++
float; native values use LDouble.

BigFloat formatting keeps its decimal representation and conversion helpers
private to the formatter. Each call owns its decimal scratch buffer; this
storage is outside the shared arena.

Shared arena, temporaries, lengths, and interpreter bindings are not
thread-isolated.  Future ownership work belongs in
[hexagonal.md](hexagonal.md), not an assumed performance or zoom model.
