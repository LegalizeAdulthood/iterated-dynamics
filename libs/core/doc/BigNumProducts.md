<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# BigNum Orbit Product Storage Contract

This specifies the owning storage of `g_old_z_bn`, `g_tmp_sqr_x_bn`,
`g_tmp_sqr_y_bn`, and `g_bn_tmp`. The square caches now contain
canonical values, as do the orbit locals and bailout metrics. Retained
products use private paired temporaries in the value implementation.

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
is `N` rounded up to 64 bytes. Each BigNum owns one slot.
A BigNumProduct owns two adjacent slots and retains `R` bytes. Full
`2 * N` products remain at the byte-kernel boundary.

Both square caches are BigNum values in the working arena, with one
slot each. `square_bn(BigNum &, BigNum &)` allocates a paired
BigNumProduct in the temporary arena, computes the retained square,
restores the source sign, copies exactly `N` bytes from `msb_bn` into
the existing destination, and releases the pair on return. The cache
address and bytes after `N` remain unchanged.

`square_bn(BigNumProduct &, BigNum &)` still writes `R` bytes into
an existing pair without allocation or reduction. The MANH and MANR
bailout metrics use canonical BigNum destinations and the same private
paired scratch/copy/release as the square caches.

`bn_bailout_reached` borrows a canonical value through `const BigNum &`.
It uses the existing owning sign and comparison functions without a
product conversion or numeric copy. The unsigned one/two-byte and signed
four-byte interpretations, fractional threshold adjustment, range
checks, and temporary limit construction stay in place.

Julia's local cross-product is a single-slot BigNum in the temporary
arena. `mult_bn(BigNum &, BigNum &, BigNum &)` computes in a private
temporary-arena pair, restores the operands, copies exactly `N` bytes
from `msb_bn` into the existing destination, and releases that pair on
return. No product-read conversion or operand-copy allocation is added.

`bn_to_int(const BigNum &)` observes a canonical value at offset zero.
It borrows bytes for `bn_to_int(const Byte *)` in big.cpp and returns
long without allocating or changing the source. Extraction still
zero-extends one/two integer bytes and uses the native U32-to-long
conversion for four bytes. It does not normalize signedness or convert
through machine floating point. Encoded product-window fixtures retain
their byte-level observations.

## Statement-Level Width Transitions

- `mandel_per_pixel_bn` and `julia_per_pixel_bn`: Keep the canonical
  seed and affine calculations in place. Each cache receives its
  canonical square through private paired scratch.

- `julia_orbit_bn`: Subtract canonical cache y from cache x in place,
  writing exactly `N` bytes. Add canonical cache x to the real
  parameter. Multiply old x/y into the local canonical product, double
  that `N`-byte value, and add it to the imaginary parameter. These use
  owning free functions in the same statement order. Call the selected
  bailout at its existing site.

- MOD bailout: Write both canonical square caches, then add them into
  canonical `g_bn_tmp`. Test the existing threshold and copy the new
  orbit into old x/y only on continuation.

- REAL, IMAG, OR, AND: Write both canonical square caches and test the
  same selected values at offset zero. Leave `g_bn_tmp` untouched.
  Copy the canonical orbit only on continuation.

- MANH and MANR: Write both canonical square caches. Keep the existing
  separate absolute-value/addition statements and canonical metric
  square. Test that canonical metric directly. Old x/y remain orbit
  values and are copied from the new orbit only on continuation.

- Periodicity and orbit observations: Keep their existing canonical
  `g_bn_tmp` and old x/y reads and writes.

No calculation order, fixed-width wrapping, threshold, source
restoration, or escape/continuation timing changes with cache storage.

## Construction Relocation And Release

The MathArenas constructor binds both caches with `init_big_num` in
`big_num.working`. Each reserves one slot, rather than a product pair.
The old x/y components and `g_bn_tmp` also remain single-slot values.

`MathArenas::bind` reconstructs each null cache in that working arena
and clears its entire aligned slot. Rebinding an existing cache clears
the same slot without allocating again.

`MathArenas::change_precision` retains the existing guarded cache
address comparisons in its workspace classification. It clears and
discards cache contents during relocation, keeping each index and
single-slot width.

`MathArenas::unbind` resets both caches with `BigNum{}` before arena
destruction. Temporary stack rewinds leave these working slots intact.
Borrowed addresses from `data()` expire on arena relocation.

Julia reserves its single-slot local before subtraction. The multiply's
private pair is released before doubling and bailout. The local stays
alive through bailout, then releases its slot before the surrounding
BigStackSaver rewinds borrowed temporaries.

MANH reserves its two single-slot absolute components and one single-slot
metric at the existing sites. MANR reserves one single-slot metric.
Each metric square releases its private pair before threshold comparison.
The helper constructs only its existing canonical limit; it does not
allocate or copy a metric at entry. Both bailout outcomes release the
locals before their surrounding BigStackSaver rewinds.

## Characterization

- `TestBigNumSqrStorage` checks independently encoded canonical cache
  bytes, source restoration, untouched slot tails and the other cache,
  fixed-width subtraction, scratch reuse, binding, relocation, cleanup,
  and temporary rewinds.

- `TestBigNumProductWindows` retains encoded byte fixtures for paired
  products, shifted subtraction and doubling, and untouched product
  bytes outside the selected window.

- `TestBigNumRetainedSqr` retains full product-byte checks, consuming
  reduction, companion-slot release, source restoration, and padding.

- `TestBigNumJuliaProduct` checks the canonical local, untouched slot
  tails and neighbors, private multiply scratch release before bailout,
  local release before rewind, source restoration, and both outcomes.
  `TestBigNumJuliaCanonicalProduct` checks discarded low bits before
  doubling, fixed-width overflow, and destination identity and tails.

- `TestBigNumBailoutProductStorage` pins a neighboring slot and checks
  that canonical metrics and private sqr scratch reuse the available
  single/paired reservations on escape and continuation. Existing
  independent bailout fixtures retain their numerical expectations;
  high-bit metric tests also cover signed four-byte wrap and fractional
  thresholds on both sides of the metric.

- Existing setup, Julia, bailout, and continuation fixtures check the
  same orbit results and fixed-width overflow with canonical caches.
  Wide fractional cases compare the selected bytes of encoded kernel
  products against each cache. Exact byte expectations avoid numerical
  tolerances.

- `TestBigNumIntegerObservation` checks independent integer results,
  negative fractions, boundary bits, const inputs, unchanged canonical
  bytes and padding, and temporary slot release at all integer widths.
  Canonical cache fixtures observe subtraction and doubling through the
  owning interface.
