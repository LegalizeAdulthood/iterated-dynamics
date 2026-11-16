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

`BigFloat` in `math/BigFloat.h` owns an indexed arena allocation. Default
construction leaves a null handle; eager results use the working arena.
The first assignment to an unbound value uses that arena. Constructors can
select another compatible allocator explicitly. Copy construction keeps
the source allocator; assignment keeps the destination allocator. Moves
within one arena transfer a block. Moves across arenas copy the encoded
value into destination storage. Moved-from objects keep their allocator
binding for later assignment.

The allocator records canonical byte counts separately from aligned block
stride and allocation width. Copies, moves, destruction, and legacy-buffer
export remain valid after `free_bf_vars()`. Precision changes migrate all
attached values and update their allocator's canonical size. There is no
per-value precision object. Arithmetic, scalar conversion, parsing, and
formatting retain the compatible active-context precondition and shared
scratch, without per-operation initialization or precision validation.

Arithmetic operators preserve their operands. Compound assignments update
only their destination, including self-assignment. Multiplication owns its
wider paired allocation and returns canonical bytes. Numerical comparisons
read values directly. Operators and temporary raw adapters share private
arithmetic kernels in `math/BigFloat.cpp`.

`BigNum` in `math/BigNum.h` owns canonical fixed-point bytes with the same
lifetime and active-context contract. Its little-endian representation
uses `g_int_length` high bytes for the signed integer part and
`g_bn_length - g_int_length` low bytes for the fractional part. Parsing
uses the legacy fixed-decimal syntax; formatting retains legacy output.
`BigNumConstPtr` denotes read-only legacy bytes.

`BigNum` member operators and `sqr()` share private arithmetic kernels
with legacy adapters in `math/BigNum.cpp`. Products of two `BigNum`
values and `sqr()` use paired result storage. The reducer moves
`g_bn_length` bytes at `g_shift_factor` into the first block, then
releases the second block without changing the handle. BigFloat's wide
kernels normalize once before its reducer moves the canonical mantissa
and exponent at `g_padding` into the first block. Addition, subtraction,
and comparisons read operands directly; destructive kernels receive
scratch copies. Truncation, fixed-width overflow, division saturation,
and zero handling retain legacy behavior.

The owning `square_bn(BigNum &, BigNum &)` free function writes all
`g_r_length` retained product bytes into the existing destination slot
and restores its mutable source's sign. It returns `void` without
allocating, copying its source, or reducing the product. Ordinary `sqr()`
remains canonical.

The owning `mult_bn` free function likewise writes the retained product
into its existing slot. It preserves the raw adapter's mutable input
roles and sign-restoration sequence, including shared-input behavior,
without copies, allocation, or reduction. It returns `void`.
Ordinary `operator*` keeps its existing operand copies and returns
canonical bytes.

`operator*(U16)` and `operator/=(U16)` use the existing integer kernels.
They preserve all unsigned operand bits, fixed-width overflow, division
rounding, and zero-divisor saturation without a fixed-point conversion
of the integer operand.

Owning `add_bn`, `sub_bn`, `mult_bn_int`, and `abs_bn` free functions
return independent canonical values in one working-arena single slot.
Owning `copy_bn`, `add_a_bn`, `sub_a_bn`, and `abs_a_bn` procedures return
`void`; they keep the destination allocation and write only N bytes.
Nested and retained-tail-sensitive raw calls remain at their existing
sites until their separate operator or representation conversion.

BigFloat's `U16` multiplication and division-assignment overloads also
use the existing integer kernels, retaining their mantissa normalization,
rounding, and zero-divisor saturation. Reference callbacks keep their
pointer signatures and use temporary-arena values for local storage.
Their doubling operations remain at their original calculation sites.

Precision migration sign-extends new BigNum integer bytes for live signed
values. The legacy `convert_bn()` scratch-conversion behavior remains
unchanged. Canonical import/export excludes alignment padding and free
list links.

Legacy Newton iterations can retain guard-digit differences from reused
destination bytes. Value division, logarithms, and arctangents start with
fresh results. Compound division agrees with the corresponding value
expression. Comparisons with legacy calculations retain the requested
decimal precision rather than depending on old destination contents.

`BNComplex` and `BFComplex`, available through `math/BigComplex.h`, own
two scalar values through `Complex<T>`. Multiplication and division
compute both components before assignment, including when the result
replaces an input. Complex callers use owning values directly; the raw
BigFloat complex arithmetic adapters have been removed.

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

### Value Storage Measurements

`check-big-value-storage` measures warmed scalar expressions, integer
complex powers, and the actual Julia orbit callbacks. Its allocation
hooks are confined to the measurement executable. The following results
compare vector storage with indexed arena handles on Windows/MSVC,
Release, at 40 decimal digits. Times are medians of three runs in
milliseconds; heap counts are calls to the replacement `operator new`.

| Case | Reps | Heap before | Heap after | ms before | ms after |
|---|---|---|---|---|---|
| BN scalar | 10000 | 50000 | 0 | 2.688 | 1.819 |
| BN complex power | 10000 | 450000 | 0 | 23.866 | 17.019 |
| BN Julia orbit | 10000 | 0 | 0 | 2.373 | 2.473 |
| BF scalar | 10000 | 30000 | 0 | 4.062 | 3.654 |
| BF complex power | 10000 | 310000 | 0 | 38.118 | 33.030 |
| BF Julia orbit | 10000 | 0 | 0 | 5.102 | 5.262 |
| BF Julia power orbit | 250 | 6250 | 0 | 35.503 | 36.203 |

Every case had zero arena high-water growth after warmup, before and
after the change. Arena free lists reuse released value blocks without
repeated backing allocations. Scalar and complex timings improved in
these runs; fractal timings were slightly slower. These short timings
depend on the machine and load and are not a rendering-speed guarantee.

Build the `check-big-value-storage` target and run
`tests/tools/Release/check-big-value-storage.exe` from the build directory
to repeat the measurement.

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

`free_bf_vars()` clears active raw bindings, resets the owning iteration
globals, and releases stack-adapter reservations. The arenas remain
alive for other attached owning handles;
subsequent initialization migrates these values and creates fresh active
corner bindings. `calc_lengths()` remains allocation-free for temporary
Newton working-length changes within already reserved storage.

## BigNum Temporaries

Header: `big.h`.

- `g_bn_tmp1`, `g_bn_tmp2`, `g_bn_tmp3` - owning `BigNum` kernel
  workspaces, each in one padded working-arena slot. Reciprocal precision
  ramps retain their existing base addresses and active byte windows.
- `g_bn_tmp_copy1`, `g_bn_tmp_copy2` - owning `BigArena::Allocation`
  handles, each reserving two working-arena blocks for operand copies
  and destructive formatter scratch. Their paired widths are retained.
- `g_bn_tmp` - owning `BigNum` temporary retaining `g_r_length` bytes in
  its padded working-arena slot.

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
reduction keeps canonical bytes in the first slot and releases the
second. Square caches and bailout metrics remain canonical `BigFloat`.

`MathArenas` constructs these six kernel values and two paired buffers
in their existing allocation order. Binding clears each full reservation
and reuses its allocation; unbinding releases it and leaves a null owner.
Precision relocation discards kernel scratch, copy-buffer, and pi bytes.
Live canonical values and products retain their existing conversions.
Temporary stack rewinds do not release working-arena owners.

Logarithm uses a borrowed, shifted window into `g_bf_tmp5`; arctangent
does the same with `g_bf_tmp3`. Their `orig_*` pointers retain the base
addresses while the existing loops change the active lengths. Exponent
cursors and operand-copy pointers are also borrows, not new values.

`init_big_pi()` copies the appropriate precision from a constant table.
It writes the existing `g_bf_pi` slot. Pointer callers borrow `data()`.
Arctangent preserves its existing precision offsets in a local borrowed
pointer and passes that shortened view to the private sine/cosine
helper. The owner keeps its base allocation throughout.

Pi is allocated after the kernel copy buffers. Binding clears the full
slot before the other BigFloat workspaces; repeated binding reuses it.
Precision relocation discards its bytes, as the previous raw binding
did. Callers repopulate the table with `init_big_pi()`; `data()` resolves
the relocated slot. Temporary rewinds leave the working slot intact.
Unbinding releases the owner and leaves `g_bf_pi` null.

The 22 parameter bindings already refer to owning BigFloat values in the
persistent arena. Their construction, precision conversion, and payload
copy sites are unchanged. Pi was the last raw working binding, so the
`WorkingBinding` table and its single/wide binding methods are retired.

## BigNum Coordinates And Iteration State

The [kernel ownership contract](BigNumKernels.md) records the three
reciprocal workspace owners, two paired copy buffers, borrowed precision
windows, and proposed member interfaces for the remaining conversions.

Header: `big.h`.

Each coordinate, increment, or threshold below owns a `BigNum` with
`g_bn_length` canonical bytes:

- `g_x_min_bn`, `g_x_max_bn`, `g_y_min_bn`, `g_y_max_bn` - image bounds.
- `g_x_3rd_bn`, `g_y_3rd_bn` - third corner.
- `g_delta_x_bn`, `g_delta_y_bn`, `g_delta2_x_bn`, `g_delta2_y_bn` -
  pixel-coordinate increments.
- `g_close_enough_bn` - comparison threshold.

`g_param_z_bn`, `g_new_z_bn`, and `g_saved_z_bn` own `BNComplex`
components with canonical `g_bn_length` bytes. These globals and the
scalar globals above start with null handles. `MathArenas::bind`
initializes them in the BigNum working arena and clears existing values
on later binds.
`MathArenas::unbind` releases their blocks. The arena owner also unbinds
them before destruction. Remaining legacy calls borrow their buffers
explicitly with `BigNum::data()`; borrowed addresses expire on relocation.

Iteration state uses these reservations:

- `g_tmp_sqr_x_bn`, `g_tmp_sqr_y_bn` - owning `BigNum` values retaining
  `g_r_length` product bytes in each padded working-arena slot.
- `g_old_z_bn` - owning `BNComplex` iteration values. Each component's
  padded slot retains Manhattan bailout's unreduced scratch product.

These five mixed-role slots, including `g_bn_tmp`, use existing single
allocations. The MathArenas constructor installs their owners at the
former raw reservation sites. Binding reconstructs released owners and
clears the entire slots; unbinding releases them. Precision migration
discards their scratch contents. Product writes and shifted reads borrow
their existing byte windows through `data()` without reducing products.

The [orbit product storage contract](BigNumProducts.md) records these
mixed canonical/product roles, exact byte windows, and construction and
cleanup requirements.

## BigFloat Coordinates And Iteration State

Header: `big.h`.

- `g_delta_x_bf`, `g_delta_y_bf`, `g_delta2_x_bf`, `g_delta2_y_bf` -
  owning `BigFloat` increments.
- `g_close_enough_bf` - owning `BigFloat` threshold.
- `g_tmp_sqr_x_bf`, `g_tmp_sqr_y_bf` - owning `BigFloat` square caches.
- `g_old_z_bf`, `g_new_z_bf`, `g_saved_z_bf`, `g_param_z_bf` - owning
  `BFComplex` iteration values and parameter.

The owning globals have canonical `g_bf_length + 2` bytes per scalar.
Their handles start null. `MathArenas::bind` initializes them in the
BigFloat working arena and clears existing values on later binds.
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
