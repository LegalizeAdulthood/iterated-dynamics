# Plan: BF_MATH Formula Parser

## Goal

Add arbitrary precision support for `type=formula` so formulas can switch
from `double` evaluation to `BF_MATH` when image coordinates or formula
arithmetic need more precision than `double` can provide.

The first target is issue #11, `Straight_Forward`, where the formula
parser loses precision evaluating nearly equal complex powers:

```text
20*z^0.99 - 20*z^1.01
```

## Constraints

- Preserve current `double` formula behavior by default.
- Reuse existing `BFMathType`, `init_bf_dec()`, `fractal_float_to_bf()`,
  and `g_alternate_math`.
- Prefer `BIG_FLT` for formulas. `BIG_NUM` is not a good fit for
  noninteger powers, logs, exponentials, and trig functions.
- Keep parser compilation separate from runtime numeric type.
- Put arbitrary precision math in small units that can be tested without
  renderer, parser, UI, image-region, or command globals.
- Pass explicit precision/context objects into new BF formula code. Do not
  read globals unless wrapping legacy BF allocation or Fractint-compatible
  entry points.
- Do not make every formula pay BF cost.

## Current State

- `formula` has no `BF_MATH` alternate.
- The parser compiles to operation function pointers in
  `fractals/parser.cpp`.
- The interpreter evaluates through `DComplex`, which is
  `Complex<double>`.
- `^` is implemented as `log`, multiply, `exp` in
  `math/cmplx.cpp`.
- BF complex helpers exist for selected built-in fractals in
  `engine/fractalb.cpp`.

## Phase 1: Separate Runtime State

Use `BFMathType` to select the formula runtime. Do not add another math
enum. Keep `DComplex` out of shared interpreter state.

Split runtime storage into:

- `FormulaRuntimeDbl`
- `FormulaRuntimeBf`

Keep compiled formula metadata shared:

- operation ids
- load/store references by variable index
- jump metadata
- variable names and formula flags

Do not store BF pointers in the existing `ConstArg` union. Use parallel BF
storage owned by `FormulaRuntimeBf`.

## Phase 2: Replace Function Pointer Ops

Convert compiled ops from raw function pointers to compact op ids.

Today:

```cpp
std::vector<FunctionPtr> fns;
```

Target:

```cpp
std::vector<FormulaOp> ops;
```

Then dispatch through type-specific evaluators:

```cpp
eval_double(op, runtime);
eval_bigfloat(op, runtime);
```

This prevents duplicating parser compilation and makes BF support an
interpreter concern.

Keep a short compatibility layer while migrating. Existing parser tests
can then compare old and new op streams.

## Phase 3: BF Stack And Variables

Implement a BF formula stack using `BFComplex`.

Storage:

- `std::vector<BFComplex> vars`
- `std::vector<BFComplex> stack`
- `std::vector<int> load_vars`
- `std::vector<int> store_vars`

Each `BFComplex` owns two `BigFloat` values allocated with
`alloc_stack(g_r_bf_length + 2)` or equivalent persistent BF allocation.

Required mapped variables:

- `z`
- `pixel`
- `p1` through `p5`
- `lastsqr`
- `rand`
- constants such as `pi`, `e`, `pixel`, `scrnpix`

Convert PAR parameters into `g_bf_params[]` in `fractal_float_to_bf()`.
Then copy them into the BF formula runtime during per-image setup.

Keep the core runtime free of direct global reads:

- no `g_formula`
- no `g_bf_math`
- no `g_params`
- no image-region globals
- no screen-size globals

Legacy entry points may gather those values, then pass plain data into the
runtime.

## Phase 4: BF Operation Coverage

Implement BF versions of all parser ops needed by normal formulas.

Minimum for issue #11:

- load/store
- add, subtract, multiply, divide
- negation
- real, imag
- ident
- power
- modulus squared
- less-than bailout

Then add common functions:

- sqr, sqrt
- exp, log
- sin, cos, sinh, cosh
- tan, tanh
- recip, conj, flip
- abs, cabs
- floor, ceil
- comparisons and boolean ops

Implement `bf_complex_pow()` as:

```text
pow(x, y) = exp(y * log(x))
```

Use BF math for log magnitude, atan2, exp, sin, and cos. If exact BF
transcendentals are not present, add them before enabling formula BF
generally. Falling back to `double` here defeats the feature.

## Phase 5: Precision Detection

Use two triggers.

### Coordinate Trigger

Keep the existing `mathtolerance` path in `calc_frac_init.cpp`:

```cpp
if (ratio_bad(test_x_try, test_x_exact) ||
    ratio_bad(test_y_try, test_y_exact))
```

When the current fractal has `BF_MATH`, call `fractal_float_to_bf()` and
restart.

### Formula Arithmetic Trigger

During `double` formula evaluation, estimate cancellation loss for `+` and
`-`:

```text
lost_bits = log2((abs(lhs) + abs(rhs)) / abs(result))
```

Switch when:

```text
lost_bits + coord_bits + safety_bits > DBL_MANT_DIG
```

Use:

```text
coord_bits = ceil(log2(magnification))
safety_bits = 8
```

Start with a conservative fallback:

```text
lost_bits > 20
```

Record the trigger in debug output so regressions are explainable.

## Phase 6: Wire Formula Into BF_MATH

Add `FractalFlags::BF_MATH` to the formula fractal entry.

Add an alternate math row:

```cpp
{FractalType::FORMULA,
 BFMathType::BIG_FLT,
 formula_orbit_bf,
 formula_per_pixel_bf,
 formula_per_image_bf},
```

Implement:

- `formula_per_image_bf()`
- `formula_per_pixel_bf()`
- `formula_orbit_bf()`

These should mirror the `double` formula entry points and differ only in
runtime storage and operation dispatch.

## Phase 7: Precision Sizing

Initial BF digits should be:

```text
digits = max(get_prec_dbl(CURRENT), requested_digits)
```

For formula-triggered precision, add:

```text
extra_digits = ceil(lost_bits / log2(10)) + 4
```

Then:

```text
digits = max(digits, DBL_DIG + extra_digits)
```

Honor `bfdigits` as an override.

## Phase 8: Tests

Use strict unit tests before image tests. New BF code should take explicit
inputs and need no renderer state.

Add low-level BF number tests:

- decimal string round-trip
- add/sub with cancellation
- multiply/divide with guard digits
- comparison and sign
- allocation lifetime through a local precision context

Add BF complex op tests:

- add/sub cancellation
- multiply/divide
- `pow(real, real)`
- `pow(complex, real)`
- `pow(complex, complex)`

Add BF formula VM tests:

- load/store with explicit variables
- stack depth and underflow failures
- jump state reset
- bailout comparison
- no reads from parser or renderer globals

Add formula parser tests:

- compile once, run double
- compile once, run BF
- compare simple formulas against double at low precision
- force BF and verify `MandAutoCritInZ` does not use double `pow`

Add image tests:

- `Straight_Forward` renders close to DOS FRACTINT reference
- a normal formula stays in double
- `mathtolerance=1` prevents BF switch
- `debugflag=3200` forces BF switch

Unit tests must assert exact strings or bounded error against checked
high-precision references. Do not compare only to `double` except for
low-precision compatibility tests.

## Phase 9: Diagnostics

Add optional formula precision tracing:

```text
formula_precision_trace=on
```

Trace:

- pixel
- iteration
- op id
- lost bits
- switch reason
- selected BF digits

Keep trace output off by default.

## Risks

- BF transcendental functions are the hard part.
- Parser op ids may expose hidden function pointer ordering dependencies.
- Some formulas depend on old overflow or domain behavior.
- BF formula evaluation will be much slower.
- Resume files must save enough state to preserve BF formula settings.

## Implementation Slices

Each code slice adds tests with exact state assertions. Unit tests should
use explicit inputs and `ValueSaver` for any global touched by legacy
wrappers.
Code implementation slices should run targeted tests first, then the
normal workflow. Documentation-only edits do not need a workflow run.

### Slice 1: Name Parser Ops

Goal: make parser output inspectable without changing behavior.

Work:

- Add `FormulaOp`.
- Map each current parser function pointer to one op id.
- Add `formula_op_name(FormulaOp)`.
- Keep the existing `double` interpreter and `g_formula.fns`.
- Add debug helpers that print op ids by name.

Tests:

- Parse the existing `Moe` fixture.
- Verify `g_formula.op_count == 40`.
- Verify selected emitted ops: first load, one power op, store-z, bailout.
- Verify no emitted op maps to `FormulaOp::UNKNOWN`.
- Verify unknown op id formats as a stable failure string.
- After `parser_reset()`, verify:
  - `g_formula.formula.empty()`
  - `g_formula.fns.empty()`
  - `g_formula.op_count == 0`

Done when:

- Existing formula tests pass.
- Formula debug output names ops without using function pointer addresses.
- No BF behavior exists yet.

### Slice 2: Emit Formula Instructions

Goal: make compiled formula bytecode numeric-type neutral.

Work:

- Add `FormulaInstruction { FormulaOp op; int arg; }`.
- Add `CompiledFormula::program`.
- Populate `program` in parallel with `g_formula.fns`.
- Rename pending compile ops so `program` is the only runtime op stream.
- Dispatch `FormulaOp` to existing `double` op bodies.
- Keep load/store/jump behavior identical.

Tests:

- Parse `Moe`.
- Verify `program.size() == g_formula.op_count`.
- Verify each load/store instruction arg matches the old index stream.
- Verify jump instruction args match `g_formula.jump_control`.
- Evaluate a simple formula and compare exact `double` output.
- Save before parse, then verify unchanged globals:
  - `g_runtime.op_index`
  - `g_runtime.load_index`
  - `g_runtime.store_index`
  - `g_arg1`
  - `g_arg2`

Done when:

- Formula output matches before/after for a small PAR set.
- Parser trace shows the same op order as before.
- No BF storage is introduced.

### Slice 3: Add Runtime Split

Goal: isolate mutable runtime state.

Work:

- Add `FormulaRuntimeDbl`.
- Move stack, load index, store index, jump index, op index, init indexes,
  and random state into the runtime.
- Keep `g_runtime`, `g_arg1`, and `g_arg2` as legacy wrappers only.
- Run the existing `double` interpreter through `FormulaRuntimeDbl`.

Tests:

- Reset runtime and verify:
  - `op_index == 0`
  - `load_index == 0`
  - `store_index == 0`
  - `jump_index == 0`
  - `set_random == false`
  - `randomized == false`
- Run two runtimes from one compiled formula without state leakage.
- Verify both runtimes produce the same `z`, stack top, and op indexes.
- Verify explicit-runtime execution leaves unchanged:
  - `g_runtime`
  - `g_arg1`
  - `g_arg2`
- Verify per-pixel reset preserves per-image constants.
- Verify legacy wrapper still updates:
  - `g_old_z`
  - `g_new_z`
  - `g_runtime.op_index`
  - `g_runtime.load_index`
  - `g_runtime.store_index`

Done when:

- No formula behavior changes.
- Runtime state can be reset per image and per pixel.
- Global formula compile state no longer owns runtime stack cursors.

### Slice 4: Isolate Formula Inputs

Goal: make core formula execution independent of renderer globals.

Work:

- Add an immutable formula input object.
- Include pixel, screen pixel, params, row, col, maxit, and inversion
  data.
- Gather globals once in legacy entry points.
- Pass plain input data into double and BF runtimes.

Tests:

- Construct input without reading renderer globals.
- Run one orbit from explicit input.
- Mutate `g_params`, `g_row`, and `g_col` after input creation.
- Verify result uses captured input, not changed globals.
- Verify legacy wrapper still writes:
  - `g_formula.vars[0]` for `pixel`
  - `g_formula.vars[9]` for white square
  - `g_formula.vars[10]` for `scrnpix`

### Slice 5: Add BF Test Context

Goal: make BF code unit-testable before parser integration.

Work:

- Add a small BF precision/context wrapper.
- Allocate BF values through the context, not direct globals.
- Add helpers to create BF scalars and BF complex values from strings.
- Add helpers to convert BF values to normalized strings.

Tests:

- Save BF globals before creating the context.
- Create context at 50 decimals.
- Verify during context:
  - `g_bf_math == BFMathType::BIG_FLT`
  - `g_decimals == 50`
  - `g_bf_length > 0`
  - `g_r_bf_length > 0`
- Destroy context.
- Verify exact restoration of:
  - `g_bf_math`
  - `g_bn_length`
  - `g_bf_length`
  - `g_r_bf_length`
  - `g_decimals`
  - `g_bf_decimals`
  - `g_bf_digits`
- Round-trip strings: `0`, `0.1`, `-2.5`, `1e-30`.

Done when:

- BF tests can run without renderer, parser, UI, or image globals.
- Context teardown leaves no leaked BF stack allocations.
- No formula entry point uses the context yet.

### Slice 6: Implement Core BF Ops In Isolation

Goal: test arbitrary precision arithmetic before VM integration.

Work:

- Implement add, subtract, multiply, divide, and negation.
- Implement real, imag, flip, ident, conj, and recip.
- Implement modulus squared and bailout comparison.
- Put these behind functions that take explicit operands and context.
- Add focused unit tests for each op.

Tests:

- Exact string-checked add, sub, mul, div, and neg cases.
- Cancellation case: `10000000000000000 - 9999999999999999`.
- Verify result string is exactly `1`.
- Divide-by-zero and recip-zero set the chosen overflow/error state.
- Modulus and bailout comparisons at below, equal, and above threshold.
- Save and verify unchanged:
  - `g_formula.op_count`
  - `g_runtime.op_index`
  - `g_arg1`
  - `g_arg2`

Done when:

- Core BF op tests pass without formula parser setup.
- Tests include cancellation cases that exceed `double` precision.
- No op reads `g_formula`, `g_params`, or image-region globals.

### Slice 7: Implement BF Power Path In Isolation

Goal: support the operation needed by `MandAutoCritInZ`.

Work:

- Implement BF complex log.
- Implement BF complex exp.
- Implement BF complex power.
- Implement BF sin, cos, and atan2 support needed by log/exp.
- Add tests for real and complex powers.
- Add `double_fallback_count` to BF formula diagnostics.

Tests:

- `pow(real, real)` checked against embedded reference strings.
- `pow(complex, real)` checked against embedded reference strings.
- `pow(complex, complex)` checked against embedded reference strings.
- `log(exp(z))` and `exp(log(z))` round-trip where branch-safe.
- Verify `z^0.99` and `z^1.01` set:
  - `double_fallback_count == 0`
- Verify 50-digit and 70-digit runs have the same checked prefix.

Done when:

- `z^0.99` and `z^1.01` execute without double fallback.
- BF power tests pass at low and high precision.
- Any temporary double fallback is deleted before slice completion.
- Tests compare against checked reference values, not only `double`.

### Slice 8: Add BF Formula Runtime Skeleton

Goal: connect tested BF ops to a formula VM.

Work:

- Add `FormulaRuntimeBf`.
- Allocate BF variable and stack storage through the BF context.
- Implement load and store.
- Implement stack execution for the ops from slices 5 and 6.
- Add VM tests using hand-built op streams.

Tests:

- Hand-built op stream computes `z*z+c`.
- Hand-built op stream computes `20*z^0.99-20*z^1.01`.
- Verify BF runtime state after `z*z+c`:
  - stored `z`
  - stack depth
  - `op_index == program.size()`
  - expected `load_index`
  - expected `store_index`
  - expected `jump_index`
- Stack underflow, unsupported op, and bad store fail cleanly.
- Two BF runtimes execute independently from one op stream.
- Verify unchanged globals:
  - `g_formula`
  - `g_runtime`
  - `g_arg1`
  - `g_arg2`

Done when:

- Hand-built BF op streams execute without parser globals.
- Unsupported ops fail cleanly with a fixed message.
- Stack underflow and type errors are unit tested.

### Slice 9: Add Forced BF Formula Entry Points

Goal: create BF entry points that compile but are not generally enabled.

Work:

- Add `formula_per_image_bf()`.
- Add `formula_per_pixel_bf()`.
- Add `formula_orbit_bf()`.
- Add an alternate math row for formula behind a local feature guard.
- Make entry points gather globals once, then pass plain data inward.

Tests:

- Forced BF entry points build a runtime from explicit formula data.
- Per-image setup copies params without later reading `g_params`.
- Per-pixel setup maps pixel coordinates without renderer state in VM.
- Force BF mode for a simple formula.
- Verify during BF call:
  - `g_bf_math == BFMathType::BIG_FLT`
  - `g_old_z` and `g_new_z` update as expected
  - `g_overflow` matches the double path for the same formula
- Verify BF context cleanup restores BF globals listed in slice 5.

Done when:

- Forced BF formula setup reaches the BF entry points.
- Normal formulas still use `double`.
- Unit-tested runtime remains independent of renderer globals.

### Slice 10: Run `MandAutoCritInZ` Under Forced BF

Goal: prove the target formula can execute.

Work:

- Force BF for `Straight_Forward`.
- Verify the formula uses BF ops for both powers.
- Compare the rendered image to the FRACTINT reference.
- Tune initial BF digit count if needed.

Tests:

- Unit fixture for `MandAutoCritInZ` first iteration at center pixel.
- Assert both power ops use BF dispatch.
- Assert the same fixture is stable at two BF precisions.
- Verify diagnostics:
  - power op count is `2`
  - `double_fallback_count == 0`
- Verify wrapper state after the run:
  - `g_old_z`
  - `g_new_z`
  - `g_overflow`
  - `g_bf_math`
  - BF length globals restored

Done when:

- `Straight_Forward` renders the correct structure.
- Trace confirms no double power/log/exp path was used.
- The result is stable across two BF digit counts.

### Slice 11: Add Precision Detection

Goal: switch automatically only when needed.

Work:

- Add cancellation tracking for `+` and `-`.
- Compute `lost_bits`.
- Add formula precision state to the runtime.
- Trigger BF when:

```text
lost_bits + coord_bits + safety_bits > DBL_MANT_DIG
```

- Start with `safety_bits = 8`.

Tests:

- Cancellation detector reports expected lost-bit ranges.
- Detector ignores noncancelling add/sub cases.
- Detector case:
  - `abs(lhs) == 20`
  - `abs(rhs) == 20`
  - `abs(result) == 1e-12`
  - expected `lost_bits` is between `45` and `46`
- `Straight_Forward` fixture triggers BF.
- Ordinary formula fixtures do not trigger BF.
- Pure detector leaves unchanged:
  - `g_bf_math`
  - `g_runtime`
  - `g_overflow`
  - `g_old_z`
  - `g_new_z`

Done when:

- `Straight_Forward` switches to BF automatically.
- Ordinary formulas stay in `double`.
- `mathtolerance=1` still prevents BF switching.

### Slice 12: Enable Formula BF_MATH

Goal: make BF formula support normal behavior.

Work:

- Add `FractalFlags::BF_MATH` to `formula`.
- Remove the local feature guard from the formula alternate math row.
- Save and restore BF formula state where needed.
- Update tab/status output to show BF formula math.

Tests:

- Formula alternate math lookup returns `BIG_FLT`.
- Verify alternate row state:
  - fractal type is `FractalType::FORMULA`
  - math type is `BFMathType::BIG_FLT`
  - per-image pointer is non-null
  - per-pixel pointer is non-null
  - orbit pointer is non-null
- Save/load preserves BF math and precision fields.
- Loading a normal formula keeps `g_bf_math == NONE`.
- Loading a BF-triggering formula selects BF.
- Verify save/load restores or preserves:
  - `g_bf_math`
  - `g_bf_digits`
  - `g_decimals`
  - formula name
  - formula file name

Done when:

- Formula BF works through normal PAR loading.
- Resume/save/load preserves selected math.
- Double formulas are not slowed by BF allocation.

### Slice 13: Expand Function Coverage

Goal: cover the full formula language.

Work:

- Add BF trig and hyperbolic functions.
- Add BF inverse trig functions.
- Add BF floor, ceil, abs, cabs, and boolean comparisons.
- Add BF random behavior that matches double semantics.
- Add tests for each documented formula function.

Tests:

- One unit test per documented formula function.
- Branch-sensitive functions include quadrant and sign cases.
- Boolean comparisons test equal, below, above, and zero cases.
- Random op test uses a fixed seed and expected sequence.
- Verify random BF op mutates only explicit BF runtime random state.
- Verify random BF op leaves unchanged:
  - `g_runtime.rand_num`
  - `g_runtime.rand_x`
  - `g_runtime.rand_y`

Done when:

- Unsupported-op failures are rare and documented.
- Formula coverage is tracked by tests.
- Known formula packs run without parser-level BF gaps.

### Slice 14: Diagnostics And Cleanup

Goal: make precision decisions explainable.

Work:

- Add optional formula precision trace output.
- Report switch reason and selected BF digits.
- Remove temporary guards and compatibility shims.
- Document limitations.

Tests:

- Trace formatter prints stable switch reason text.
- Trace output is off by default.
- Enabling trace does not change computed orbit values.
- No compatibility shim remains referenced by tests.
- Verify trace-on changes only diagnostics output, not:
  - `z`
  - stack contents
  - bailout result
  - `g_overflow`
  - `g_old_z`
  - `g_new_z`

Done when:

- A debug trace explains why issue #11 switches to BF.
- The implementation has no dead double/BF bridging code.
- The plan document matches the final architecture.

## Test Placement

- Add parser and VM tests under `tests/libid/fractals`.
- Add BF context and math tests under `tests/libid/math`.
- Register new files in `tests/libid/CMakeLists.txt`.
- Use explicit fixtures and `ValueSaver` for globals.
- No test may depend on parser or runtime state from a prior test.
