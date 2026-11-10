<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# BigNum Kernel Ownership Contract

This records owning BigNum kernel storage and the remaining member
interface contracts. Calculation bodies, statement order, precision
ramps, operand copies, and scratch reuse are preserved. The member
proposals below are superseded by the
[value-type plan](../../../../plans/bignum-value.md). Implemented retained
products use the owning free functions described here.

## Storage And Lifetime

Use `N = g_bn_length`, `I = g_int_length`, `P = g_padding`,
`R = g_r_length = N + P`, and `S = g_shift_factor = P - I` for the
active kernel lengths. `N0` is the entry length of a precision ramp.
Arena canonical sizes and aligned strides remain at their outer widths;
`calc_lengths()` changes the active globals without reallocating arenas.

| Global | Owner | Reservation |
|---|---|---|
| `g_bn_tmp1` | `BigNum` | Working `allocate_single()` |
| `g_bn_tmp2` | `BigNum` | Working `allocate_single()` |
| `g_bn_tmp3` | `BigNum` | Working `allocate_single()` |
| `g_bn_tmp_copy1` | `BigArena::Allocation` | Working `allocate_double()` |
| `g_bn_tmp_copy2` | `BigArena::Allocation` | Working `allocate_double()` |

`g_bn_tmp1` retains `[0, R)` products. `g_bn_tmp2` holds canonical one
and two at `[0, N)`. `g_bn_tmp3` holds the canonical correction at
`[0, N)`. All three stay at their allocation bases as `N` changes.
Do not shift those globals to follow the reciprocal's result window.

The copy buffers receive `[0, N)` operand copies in `binary_operation`,
`BigNum::sqr`, and `BigNum::to_float`; the second also holds destructive
formatter scratch. Keep both existing paired reservations. They are
encoded workspaces, not independent canonical values. Do not reduce
their handles or replace their reservations with singles.

`MathArenas` constructs these five owners at their former reservation
sites, in the same order. Each single-slot BigNum is staged in
`big_num.working`, then placement-moved into its global. Each paired
handle is assigned directly from `big_num.working.allocate_double()`.
These slots have no raw `WorkingBinding` entries.

`bind` reconstructs null owners at the existing working setup site,
then clears a single's full block or a pair's two full blocks. The
guarded workspace test in `change_precision` includes all five live
base addresses; relocation discards their bytes and preserves width
metadata and indices. `unbind` releases the three BigNum values with
`BigNum{}` and resets both allocation handles before arena destruction.
Rebinding live owners does not allocate additional slots. Working
scratch is discarded rather than converted as a live numeric value.

## Canonical Arithmetic And Retained Products

Ordinary canonical values use the existing operators and `sqr()`. Their
current operand-copy policy and allocation/reduction sites stay intact:

- `operator*` allocates a paired working result, copies both operands
  through the two copy buffers, calls `multiply`, then reduces once.
- `sqr()` allocates a paired working result, copies its operand into
  copy1, calls `square`, then reduces once.
- `operator/` allocates a single result, copies both operands before the
  destructive callback, and returns canonical bytes.
- Addition, subtraction, negation, and U16 multiplication allocate one
  result; their compound forms retain their existing implementations.

The [orbit product contract](BigNumProducts.md) requires unreduced
results and partial writes into existing slots. The implemented owning
free-function overloads are:

```cpp
void square_bn(BigNum &result, BigNum &source);
void mult_bn(BigNum &result, BigNum &left, BigNum &right);
```

These return `void`. They neither allocate nor replace the destination's
handle. Owning `square_bn` writes `[0, R)` with the existing
`square_bn` sign-save, kernel call, and sign-restore sequence.
Owning `mult_bn` retains the existing `mult_bn` sequence. Keep their
mutable input roles; do not add operand copies. The result must be
separate from the product inputs, as required by the existing clearing
kernels. No new alias validation or initialization checks are added.

### Canonical Owning Free Functions

```cpp
BigNum add_bn(const BigNum &left, const BigNum &right);
BigNum sub_bn(const BigNum &left, const BigNum &right);
BigNum mult_bn_int(const BigNum &source, misc::U16 factor);
BigNum abs_bn(const BigNum &source);
void abs_a_bn(BigNum &result);
void copy_bn(BigNum &result, const BigNum &source);
void add_a_bn(BigNum &result, const BigNum &source);
void sub_a_bn(BigNum &result, const BigNum &source);
```

The four value-producing functions construct one working-arena single
result and call the existing N-byte kernels. Only absolute value copies
its operand, preserving the existing copy/negate sequence. Results are
independent owning values; inputs are const. Current move/copy assignment
policy is unchanged. The in-place procedures return `void`, allocate
nothing, and preserve the destination handle and bytes beyond N.

Image setup assigns `g_close_enough_bn = abs_bn(g_delta_x_bn)`;
Mandelbrot pixel setup assigns `g_param_z_bn.y = sub_bn(...)`. These are
canonical destinations. Copies and in-place additions use the owning
procedures in setup and bailouts. Nested expressions stay raw until
their operator stage; no member chaining is introduced.

Mixed-role scalar multiplies into `g_old_z_bn` and `g_bn_tmp`, Julia's
subtraction into old y, and MANH/MANR's absolute-value/addition writes
into old x/y and `g_bn_tmp` retain their raw destinations. Assigning a
fresh value would replace those scratch allocations and retained tails.
Do not force these conversions by changing assignment or allocator policy.

`TestBigNumCanonicalValues` compares all canonical bytes with independent
raw calls and checks result storage, source preservation, and independent
lifetime. `TestBigNumCanonicalProcedures` checks complete padded slots,
destination identity, aliases, unchanged sources, and high-water marks.
Both use lengths 4, 12, 64, and 672 with all three integer widths.
Compile-time checks require value results and `void` procedure returns.

### Superseded Shifted-Window Member Proposals

The shifted-window member proposals below are historical, not
implementation instructions. Follow the value-type plan's free-function
stage and later operator stage; do not add named member APIs.

Keep each unreduced cache in its existing variable. For shifted calls,
add these exact member interfaces:

```cpp
BigNum &add_products(const BigNum &left, const BigNum &right);
BigNum &add_product(const BigNum &left, const BigNum &right);
BigNum &sub_product(const BigNum &right);
BigNum &double_product();
long to_int() const;
long product_to_int() const;
```

`add_products` writes at the destination base from both inputs at `S`.
`add_product` reads only the left input at `S`; the right stays at zero.
`sub_product` subtracts the right window at `S` from the destination
window at `S`. `double_product` calls the existing doubling kernel only
on `[S, S + N)`. Keep both surrounding product regions unchanged.

The two integer observations read offsets zero and `S`, respectively.
Keep `bn_to_int`'s one-/two-byte zero extension and native U32-to-long
conversion for four bytes. Make only its pointer and `ones_byte` cursor
const; its extraction statements stay unchanged. No reduction or
full-precision comparison replaces an integer bailout test.

## Reciprocal Division And Borrowed Windows

`unsafe_inv_bn` borrows its destination and mutable input from their
owning caller. It saves `orig_r` and `orig_n`, starts at the existing
LDouble estimate, and doubles the active precision for at most 25
iterations. At each active `N`, its result and input windows start at
`orig_r + N0 - N` and `orig_n + N0 - N`. The initial approximation and
the timing of the first input adjustment remain exactly where they are.
Products in tmp1 are read at its active `S`; tmp2 and tmp3 remain at
their allocation bases. Restore the saved lengths and result pointer
before applying the result sign and returning that original pointer.

`divide` still checks machine estimates, makes operands positive, scans
and shifts their existing byte buffers, calls the reciprocal, multiplies
into tmp1, copies its shifted result, rescales, and applies the sign.
Do not change saturation, zero handling, iteration count, or those steps.

Keep the private callback's representation signature:

```cpp
Byte *binary_operation(Byte *result, const Byte *left,
    const Byte *right, Byte *(*operation)(Byte *, Byte *, Byte *));
```

The result borrows its owning BigNum's storage. The callback's inputs
borrow the two paired copy buffers after the existing two `[0, N)`
copies. Its return aliases `result`; it does not transfer an allocation.
`orig_r`, `orig_n`, n1_p/n2_p, and rp1/rp2/rp3 are interior cursors, not
additional numeric values or owners. Keep their offsets and loops.
`native_storage` continues returning an owning allocation to the value
constructor; it is not a borrowed numeric pointer.

There is no arena relocation or stack rewind while these windows are
borrowed. Keep the existing mark/rewind boundaries outside the kernels;
do not add a mark, rewind, temporary arena, or precision class inside.

## Shared BigFloat Mantissa Boundary

BigFloat's `multiply_wide` and `square_wide` temporarily set BigNum
`N` to `g_bf_length` and `R` to `g_r_bf_length`. Their inputs borrow
BigFloat mantissas, and their results borrow its wider output storage.
Exponents remain at the existing BigFloat offsets. The BigNum kernel
does not own those buffers or interpret an exponent. Restore BigNum
lengths before the existing exponent adjustment and normalization.

BigFloat add/subtract, negate, halve, U16 scale/divide, formatting, and
representation conversions also borrow BigNum byte primitives. Their
windows are selected by those existing functions, including temporary
length changes. Do not construct BigNum values from those mantissas or
copy them into the BigNum arena to call an owning operation.

Use the existing math-private `BigStorage.h` for the necessary byte
declarations once public numeric/test callers have migrated. Bodies stay
in their existing files and namespaces. Representative signatures are:

```cpp
Byte *clear_bn(Byte *r);
Byte *max_bn(Byte *r);
Byte *copy_bn(Byte *r, const Byte *n);
int cmp_bn(const Byte *left, const Byte *right);
bool is_bn_neg(const Byte *n);
bool is_bn_not_zero(Byte *n);
bool is_bn_zero(Byte *n);
Byte *add_bn(Byte *r, const Byte *left, const Byte *right);
Byte *add_a_bn(Byte *r, const Byte *n);
Byte *sub_bn(Byte *r, const Byte *left, const Byte *right);
Byte *sub_a_bn(Byte *r, const Byte *n);
Byte *neg_bn(Byte *r, const Byte *n);
Byte *neg_a_bn(Byte *r);
Byte *double_bn(Byte *r, Byte *n);
Byte *double_a_bn(Byte *r);
Byte *half_bn(Byte *r, Byte *n);
Byte *half_a_bn(Byte *r);
Byte *mult_bn_int(Byte *r, const Byte *n, misc::U16 factor);
Byte *mult_a_bn_int(Byte *r, misc::U16 factor);
Byte *unsafe_div_bn_int(Byte *r, Byte *n, misc::U16 divisor);
Byte *div_a_bn_int(Byte *r, misc::U16 divisor);
Byte *unsafe_mult_bn(Byte *r, Byte *left, Byte *right);
Byte *unsafe_square_bn(Byte *r, Byte *source);
Byte *full_mult_bn(Byte *r, Byte *left, Byte *right);
Byte *mult_bn(Byte *r, Byte *left, Byte *right);
Byte *full_square_bn(Byte *r, Byte *source);
Byte *square_bn(Byte *r, Byte *source);
Byte *unsafe_inv_bn(Byte *r, Byte *source);
Byte *div_bn(Byte *r, const Byte *left, const Byte *right);
long bn_to_int(const Byte *n);
```

All signatures keep their current pointer constness, scalar parameters,
return alias, and active byte extent. Only the integer observation gains
constness as specified above. The anonymous `compare`, `add`,
`subtract`, `negate`, `multiply`, `full_multiply`, `square`,
`full_square`, and `divide` implementations stay in BigNum.cpp with
their existing argument roles. Their byte cursors continue borrowing
the owning arguments or copy allocations; no additional values are
constructed inside their loops.
Public operators continue taking BigNum references. Representation
tests may include this private boundary through a test-core-only include
directory for `libs/core/math`. Do not add a new public byte wrapper.

Spelling a pointer `Byte *` is not an ownership conversion. Keep each
necessary byte declaration and cursor in the final audit, with its
actual owner, active extent, mutation, and call lifetime. Do not remove
debugging aids or untested arithmetic bodies merely to erase a name.

## Characterization And Pre-Existing Defect

`TestBigNumKernelStorage` covers all five slots at lengths 4, 12, 64,
and 672. It verifies working-arena ownership, both pairs' width metadata,
full-reservation clearing, stable addresses and high-water marks on
same-precision binds, precision-change scratch discard, repeated cleanup,
and reinitialization.

`TestBigNumKernelProducts` records complete partial/full product bytes,
destination return aliases, operand mutation, and guard bytes at lengths
4, 8, 12, and 672. `TestBigNumKernelWindows` covers reciprocal precision
ramps, both division output aliases, and BigFloat mantissa products at
lengths 12, 32, and 672 with one, two, and four integer bytes. Expected
values are exact dyadic samples; no floating-point tolerance is needed.

`TestBigNumRetainedSqr` compares every retained byte with the raw adapter,
checks source restoration and unchanged padding, and preserves the
destination address, arena, and high-water mark. Its parameterized cases
cover both signs, integer widths one, two, and four, and lengths 4, 8,
12, 56, 60, 64, and 672. Existing bailout tests retain their exact scratch
byte expectations after conversion to owning `square_bn`.

`TestBigNumRetainedMultiply` checks every retained byte, destination
identity, unchanged padding and lengths, and the arena high-water mark.
It covers all four distinct-input sign combinations and both shared-input
signs at the same precisions and integer widths as the retained-square
tests. Operand expectations preserve the raw adapter's existing behavior.

Both `mult_bn` and `full_mult_bn` restore a shared negative input twice.
The shared-source kernel makes it positive once; the two adapter restores
therefore leave it positive. Owning `mult_bn` preserves this sequence and
behavior. Distinct negative inputs are restored.
The existing value `operator*` copies inputs into separate scratch
buffers and preserves the original value even for self-multiplication.
The tests record this pre-existing defect separately. The ownership
conversion does not authorize changing it or the kernel's alias rules.
