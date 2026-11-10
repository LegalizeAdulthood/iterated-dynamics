<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# BigNum Orbit Product Storage Contract

This specifies the ownership conversion of `g_old_z_bn`,
`g_tmp_sqr_x_bn`, `g_tmp_sqr_y_bn`, and `g_bn_tmp`. It preserves their
existing storage windows and calculation statements. The production
declarations and calculations have not yet been converted.

## Extents And Existing Operations

Let `N = g_bn_length`, `I = g_int_length`, `P = g_padding`,
`R = g_r_length = N + P`, and `S = g_shift_factor = P - I`.
Offsets are bytes from the start of an allocation.

| Representation | Byte window | Interpretation |
|---|---|---|
| Canonical number | `[0, N)` | Signed fixed point, `I` integer bytes |
| Retained product | `[0, R)` | Partial product, `2 * I` integer bytes |
| Product observation | `[S, S + N)` | Existing canonical truncation |

The BigNum arena's canonical `value_size()` is `N`. Its block stride
is `N + P` rounded up to 64 bytes. Every retained product fits one
existing single allocation, including the minimum and maximum supported
precisions. At `N = 4` and `N = 8`, `R = 2 * N`; the same single block
also fits that full product. At larger precisions these callers retain
only `R` bytes, not a complete `2 * N` product.

Replace `g_old_z_bn` with `BNComplex`, and each of the three scalar
globals with `BigNum`. Each component owns one working-arena single
allocation. No paired allocation, new allocator metadata, representation
flag, handle accessor, or new arithmetic API is required for these five
slots. Keep the existing reduction callback configured on the arena.

Use existing `BigNum::data()` for borrowed representation windows.
`data()` addresses the slot base; `data() + S` addresses its existing
shifted window. The owner keeps the entire padded slot alive. Borrowed
addresses do not survive arena relocation and must not be retained.
`BigNum::arena().block_size()` supplies the existing aligned capacity
when clearing these working slots.

Keep `square_bn(destination.data(), source.data())` and
`mult_bn(destination.data(), left.data(), right.data())` at their
existing statements. They write `R` bytes and restore signed operands.
Do not replace them with `sqr(source)` or `left * right`: those owning
operations reduce a paired result to `N` canonical bytes and lose the
retained product. Do not call `Allocation::reduce()` on these singles.

Keep `sub_a_bn(cache_x.data() + S, cache_y.data() + S)` and
`double_a_bn(g_bn_tmp.data() + S)` as shifted in-place writes. Each
changes exactly `N` bytes. Bytes before and after that window remain
as produced by the original product operation.

For these mixed-role destinations, retain the existing scalar raw calls
through `data()` in the first ownership conversion. This includes
`copy_bn`, `float_to_bn`, `mult_bn_int`, `abs_bn`, additions,
subtractions, and periodicity's `abs_a_bn`. Their canonical writes touch
only `[0, N)` and leave the rest of the slot unchanged. An assignment
from an owning expression can replace the allocation through move
assignment; it is not a substitute for these partial-buffer writes.
Canonical inputs and outputs that already own values keep their existing
operations. Further kernel encapsulation is separate work.

`bn_to_int` reads the same base or shifted pointer as before. It
zero-extends one- and two-byte integer parts. Its four-byte result uses
the existing unsigned-32-bit-to-`long` conversion, signed on Windows.
Preserve that extraction and the `long` conversion of the bailout limit.
Do not replace an integer threshold test with a full-precision
comparison. `cmplx_bn_to_float` will take `const BNComplex &` and call
each component's `to_float()` at offset zero. It must not silently shift
an observation after an escaping Manhattan bailout.

## Statement-Level Width Transitions

Canonical writes below retain any previous product bytes beyond `N`.
A return on escape leaves scratch exactly as written before that return.

- `mandel_per_pixel_bn`: `g_bn_tmp` is a canonical row product. Old x/y
  hold canonical affine scratch, then the seed. Both caches receive
  retained squares.

- `julia_per_pixel_bn`: Old x/y hold the canonical seed. `g_bn_tmp` is
  canonical affine scratch. Both caches receive retained squares.

- `julia_orbit_bn`: Subtract shifted caches into cache x in place; read
  that window for new x. Multiply canonical old x/y into the full retained
  `g_bn_tmp` window, double only its shifted window, and read that window
  for new y. Then call the selected bailout at its current location.

- MOD bailout: Write both retained squares, then add their shifted windows
  into canonical `g_bn_tmp`. Read its integer part. Copy the canonical new
  orbit into old x/y only on continuation.

- REAL, IMAG, OR, AND: Write both retained squares and read the same
  selected shifted integer windows. Leave `g_bn_tmp` untouched. Copy the
  canonical orbit only on continuation.

- MANH: Write both retained squares; write canonical absolute components
  into old x/y; add them into canonical `g_bn_tmp`; overwrite all `R` old-x
  bytes with its square. Read shifted old x. On continuation, copy `N`
  canonical bytes into old x/y.

- MANR: Write both retained squares; add new x/y into canonical `g_bn_tmp`;
  overwrite all `R` old-x bytes with its square. Old y stays untouched
  until continuation copies the orbit. Read shifted old x.

- Periodicity in `calcfrac.cpp`: Subtract and take absolute value in
  canonical `g_bn_tmp`, at the existing nested comparison statements.

- Orbit observations: Observe the existing offset-zero old x/y bytes.
  `StickyOrbits::start_orbit` passes owning `g_old_z_bn` directly to
  `begin_orbit_bn`, removing only its canonical import copies.

## Construction Relocation And Release

At each of the five current `big_num.single(...)` sites in the
`MathArenas` constructor, construct a `BigNum` in `big_num.working`.
Stage that value before destroying the default-null global component,
then placement-move it into that component. This uses the same allocation
order without depending on the not-yet-installed global arena owner.
Remove only those five raw `WorkingBinding` reservations.

In `MathArenas::bind`, construct any released component in the same
working arena before clearing it. Use the same staged construction,
only when the component is null. Clear each complete block with
`memset(data(), 0, big_num.working.block_size())`, alongside the existing
working-buffer initialization, before the coordinate assignments. A
canonical assignment of zero alone does not clear the retained tail.
Do not allocate a duplicate reservation or clear only canonical bytes.

Before changing these declarations, extend the BigNum migration lambda
in `MathArenas::change_precision`. Its workspace test must include all
five owning components in addition to `big_num.is_workspace(source)`.
For each component, guard its `data()` comparison with its existing
allocated-value test. Do not obtain an address from a null handle.
This preserves the current zero-fill-and-discard treatment for working
scratch; it must not apply signed canonical conversion to a product.
Keep each single allocation's index and width through relocation.

In `MathArenas::unbind`, clear `g_old_z_bn` with `{}` and the three
scalar owners with `BigNum{}`, before their working arena can be
destroyed. Do not keep raw bindings to released owners. A later `bind`
reconstructs the same five slots in their existing roles. Other raw
kernel bindings, temporary rewinds, parameter preservation, and precision
limits retain their existing behavior.

## Characterization

`test_BigNumBailout.cpp` covers the current production implementation:

- `TestBigNumProductWindows` checks that one single allocation fits the
  complete product; shifted subtraction and doubling preserve the other
  product bytes; and precision changes and cleanup discard scratch.
- `TestBigNumProductBailout` checks all seven bailout modes with both
  escape and continuation, signed fixed-width wrapping, unchanged input
  bytes and overflow flags, and every byte of all five scratch slots.
  Canary tails distinguish canonical copies from full-product writes.
- Layouts use `N = 4, 8, 12, 56, 60, 64, 672` and integer widths one,
  two, and four. These include both full-product minimum layouts and
  64-byte stride boundaries.
- Exact integer byte expectations avoid floating-point tolerances.
  `TestBigNumWideBailout` separately retains the sub-machine-precision
  product comparisons against the original kernels.

The contract adds no production arithmetic or storage implementation.
