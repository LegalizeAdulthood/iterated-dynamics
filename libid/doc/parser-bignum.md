<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# BF Formula Parser Plan

## Goal And Scope

Add arbitrary-precision evaluation for `type=formula` when coordinates or
formula arithmetic require more precision than double provides.
Preserve ordinary double behavior and avoid BF allocation for formulas
that do not need it.

The original target is issue #11's `Straight_Forward` parameter set and
`MandAutoCritInZ` formula, particularly cancellation between powers:

```text
20*z^0.99 - 20*z^1.01
```

Those named fixtures are not currently present under `home` or `tests`.
Add a minimal, verified formula and parameter fixture before relying on
them as regressions.  A DOS reference image is supplementary evidence,
not a substitute for a checked high-precision numerical reference.

[Release 1.5](release-1.5.md) explicitly defers this work.
The implementation slices below remain future work; existing foundations
are identified separately so they are reused rather than rebuilt.

## Current Implementation

### Compilation And Runtime

- `CompiledFormula` in `fractals/parser.h` holds source text, operation
  function pointers, load/store pointer tables, variable values, jumps,
  pending compile operations, counters, and formula-use flags.
- `RuntimeState` in `fractals/interpreter.h` already groups the operand
  stack, execution/init indices, and random state.
- `g_formula` and `g_runtime` remain shared bindings.  Grouping fields
  did not make execution independent: operation bodies still use these
  bindings and `math::g_arg1`/`g_arg2`.
- `ConstArg` is a struct containing a source-text view and a double-valued
  `Arg`, not a union or a backend-neutral constant representation.
- `parse_formula(const FormulaEntry &)` already supports compiling an
  in-memory definition.  Named operation diagnostics and parser-state
  dumps already exist; the compiled runtime stream still uses pointers.
- Parser initialization populates built-ins and parameters from engine
  globals.  Compilation and runtime variable storage are not yet cleanly
  separated.
- `formula_per_image()` resets formula random state and compiles the
  selected definition.  `formula_per_pixel()` initializes pixel inputs
  and the initialization section; `formula_orbit()` executes the orbit
  section and publishes its result.

The existing parser tests cover operation order, jumps, diagnostics,
failed-program cleanup, reload, and source handling.  Extend those tests;
do not add another parser or discard their compatibility oracle.

### Dispatch And Image Lifecycle

`g_fractal_specific` and `g_alternate_math` are const tables.
`g_dispatch` holds runtime callbacks.  Formula currently has neither
`FractalFlags::BF_MATH` nor a formula alternate-math row.

Successful compilation installs the double `formula_per_pixel` and
`formula_orbit` callbacks into `g_dispatch`.  Merely adding a BF row is
therefore insufficient: compiling during BF setup would overwrite the
selected callbacks.

`StandardFractal` owns calculation and the incremental pixel lifecycle.
A formula orbit callback is one step inside that lifecycle, not a new
image renderer.  Per-image callbacks perform setup.  Keep keyboard input,
diagnostic presentation, and waits in the existing UI owners.

### Existing Precision Foundations

- `fractal_float_to_bf()` already initializes precision and converts image
  corners and applicable `g_params` values into BF storage.
- `cmd_params` retains supplied text in `g_param_text`, but current
  conversion to `g_bf_params` uses the double values.  That conversion
  cannot recover digits already rounded away.
- BF scalar arithmetic, log, exp, sin/cos, and atan/atan2 operations exist
  in the math implementation.
- `cmplx_log_bf`, `cmplx_mul_bf`, `cmplx_div_bf`, and `cmplx_pow_bf`
  exist in `engine/fractalb.cpp`.  They use shared precision/scratch
  state; division also sets `g_overflow`.
- `math/complex_fn.h` has BF integer-power support.  That does not alone
  implement formula powers with noninteger or complex exponents.
- Precision storage uses the existing static 1 MiB arena and supported
  length limits.  `BigFloat` is a raw byte pointer and `BFComplex`
  contains two such pointers; neither owns an allocation.

See [MathGlobals.md](MathGlobals.md) for current lengths, allocations,
aliases, and stack lifetime.  Preserve the tested low-level arithmetic
where possible; isolate dependencies instead of rewriting it from scratch.

## Design Direction

### Shared Program, Separate Numeric Storage

Compile one numeric-neutral program and evaluate it with double or
`BIG_FLT` storage, selected through existing `BFMathType`.
Do not introduce another representation-selection enum.

The program should describe operation identities, variable-slot indices,
constant spellings, initialization boundaries, jumps, selected function
identities, and formula metadata.  It must not contain pointers into a
particular runtime's values.

Runtime-owned storage includes variables as well as the operand stack,
execution/init indices, overflow/domain state, and RNG state.
Two runtime objects sharing a program must not share writable variables.
Reuse/refactor the existing `RuntimeState` rather than claiming its
already-grouped fields still need initial extraction.

`FormulaRuntimeDbl`, `FormulaRuntimeBf`, and `FormulaInstruction` are
proposed names, not existing types.  Keep their implementations simple;
no generic VM framework or template hierarchy is required.

Preserve source lifetime for literal/name references, using owned text or
stable offsets as appropriate.  Construct BF literals directly from their
original spelling, not from `ConstArg::a.d`.

### Inputs And Published Results

Keep numerical interpretation independent of engine configuration.
Legacy formula entry points gather per-image/per-pixel inputs and publish
results through the existing calculation contract.

Inputs include the pixel, screen coordinates, dimensions, parameters,
iteration limit, Mandelbrot intent, selected functions, and the existing
coordinate-derived built-ins.  Preserve bindings for `pixel`, `z`,
`LastSqr`, `p1` through `p5`, `pi`, `e`, `rand`, `whitesq`,
`scrnpix`, `scrnmax`, `maxit`, `ismand`, `center`, `magxmag`, and
`rotskew`.

BF inputs must come from precise coordinates/parameters where available.
Reuse BF coordinate and inversion operations at the engine boundary;
do not convert a double pixel grid into BF and claim deep-zoom accuracy.
Use retained parameter text when it represents the accepted value.
Constants such as Pi and e also need BF sources, not rounded doubles.

The numerical runtime returns values, continuation/bailout decisions, and
errors.  Entry points retain existing per-pixel initialization and orbit
return meanings; do not reinterpret every integer result as a pixel color
or an image-level interrupt.

Publishing double values for legacy display/coloring is a distinct
conversion boundary.  Never narrow a BF orbit to double and feed that
rounded value back into the next BF iteration.  Additional coloring
methods needing precise orbit state require explicit integration tests.

### Precision Context And Storage Lifetime

New formula math takes explicit operands and a small precision/allocation
context.  The context initially adapts existing BF arithmetic; it does not
make the underlying global lengths and scratch buffers reentrant.

Borrow an initialized `BIG_FLT` arena at fixed precision.  Do not call
`init_bf_dec()` inside every operation or create nested contexts that
silently overwrite a live arena.  Current initialization selects numeric
mode from the fractal's alternate-math support; passing a digit count
alone does not independently select `BIG_FLT`.

The runtime must own stable backing storage or have an explicitly
enclosing allocation lifetime.  A vector of `BFComplex` pointers does
not establish that ownership.  Keep runtime values alive across pixel
increments and reserve temporary scratch only within operation scopes.
Rebuild bindings when precision changes.

Tests should own precision setup/teardown and borrow that arena for
operations.  Saving scalar length globals with `ValueSaver` is not a
snapshot of the arena contents.  Do not promise restoration of a prior
calculation after destructive reinitialization.

Preserve current representation limits and report `PrecisionError`
without UI.  Separate contexts do not authorize concurrent BF execution
until the underlying shared arithmetic state is isolated.

### Reuse Arithmetic And Define Its Contracts

Reuse and, where appropriate, move general complex BF helpers from engine
into math.  Do not introduce a math-to-engine dependency merely to call
them from a new formula math module.

Tests must establish operand preservation, allowed aliasing, branch
conventions, zero behavior, and divide/domain failures before helpers are
used by the VM.  Existing helper names do not prove those contracts.
Separate numerical failure state from the global `g_overflow`; the legacy
entry point publishes the corresponding engine state.

For noninteger powers, retain the BF log/multiply/exp path and its required
BF trigonometry.  Preserve intended legacy zero/branch semantics.
Internal low-precision estimates used by an iterative BF algorithm are
not the same as falling back to a double result; verify final precision
against independent references.

Classify supported operations before BF execution.  Do not silently use a
double operator for an unsupported BF instruction.  Ordinary double-only
formulas remain supported; requests requiring unavailable BF behavior
must receive a clear diagnostic.

## Precision Selection And Restart

### Existing Coordinate Selection

Reuse the coordinate/resolution-error machinery in
`engine/calc_frac_init.cpp` and existing `bfdigits` handling.
`get_prec_dbl()` is currently private to that file, not a public context
API that a new VM can call.

The relevant transition setting is `g_math_tol[1]`.
The current parameter handler uses the second slash-separated field:
`mathtolerance=/1` sets that field to one.  A single
`mathtolerance=1` does not; the old integer-to-float field is ignored.

Current `debugflag=3200` setup has explicit built-in-fractal branches.
It is not an existing forced-BF formula path.  Add and test formula
integration rather than assuming the flag already works for formulas.

### Formula Arithmetic Detection

Cancellation monitoring remains proposed work.  A candidate indicator is:

```text
loss estimate = log2((abs(lhs) + abs(rhs)) / abs(result))
```

It is a diagnostic heuristic, not a proof that precision is inadequate.
Handle zero, nonfinite values, legitimate exact cancellation, and scale.
Do not equate magnification alone with the required coordinate precision
or adopt an unvalidated fixed lost-bit threshold.

Use checked formula fixtures to establish when higher precision improves
the result, how much precision is needed, and when ordinary double should
remain unchanged.  Instrumentation must not change the numerical result
merely by observing an operation.

### Engine-Owned Transition

Do not switch representations halfway through an orbit while retaining
already-rounded state.  Prefer deciding before plotting; if a precision
request arises during calculation, the engine must own a defined restart
at the appropriate calculation boundary from original inputs.

Define whether that restart affects the pixel or the image before enabling
automatic switching.  Preserve committed output consistency, work-list
state, random replay, timing, output side effects, and resume behavior.
A numerical evaluator reports the need; it does not enter a UI loop,
reinitialize the arena beneath live pointers, or hide a recursive render.

Honor `bfdigits`, transition-disable settings, and debug overrides with
explicit precedence tests.  Precision allocation failure or an unsupported
BF instruction must not silently publish a partially converted image.

## Implementation Slices

Retain the existing slice numbers.  Sub-slices keep the required work
small enough to review independently.  Completed foundations described
above are not instructions to rebuild them.

Use focused tests through `cmake --workflow temp`, followed by integrated
validation through `cmake --workflow rt-default` as appropriate.
Documentation-only changes require neither workflow.
Remove completed slices without renumbering the remaining work.

Any temporary bridge must name its removing sub-slice and carry
`// TODO: removed later`.  Do not add permanent forwarding APIs, new
assertions, hidden mutable function statics, or a second parser.

### Slice 1: Operation Identities

- Add `FormulaOp` identities covering emitted operations.
- Reuse the existing operation-name mapping and diagnostics instead of
  adding a duplicate debug naming system.
- Keep the current double function stream as the compatibility oracle.
- Extend named parser fixtures such as `s_moe` to verify operation
  identities/order and unknown-operation handling.
- Preserve current failed-compilation and reset behavior.

Numeric-neutral execution is not enabled in this slice.

### Slice 2: Numeric-Neutral Instructions

#### 2.1. Instruction Stream And Variable References

- Emit `FormulaInstruction` records alongside the existing pointer
  stream during transition.
- Represent load/store operands by variable indices, not `Arg *`.
- Preserve raw literal spellings and source lifetime.
- Verify operation order, variable mapping, and initialization boundaries.

#### 2.2. Jump Metadata And Compatibility

- Preserve jump destinations and initialization/orbit cursor behavior in
  the new stream, including compiler optimizations.
- Compare emitted programs and double results against current fixtures.
- Keep compatibility execution until the explicit runtime is complete;
  mark the transitional stream/adapter for removal in slice 3.5.

No BF values or general automatic selection belong here.

### Slice 3: Explicit Double Runtime

#### 3.1. Runtime-Owned Values

- Reuse the existing runtime grouping for stack, indices, and RNG fields.
- Separate mutable variable values from compiled metadata.
- Preserve per-image constants and per-pixel initialization behavior.
- Prove that two runtimes can share a program without sharing values.

#### 3.2. Explicit Operand Evaluation

- Convert ordinary arithmetic, loads/stores, and stack operations to
  explicit runtime operands.
- Preserve existing double overflow/domain and expression semantics.
- Test results without rebinding `g_arg1`/`g_arg2` to fake independence.

#### 3.3. Initialization And Control Flow

- Execute initialization, orbit bodies, conditionals, and jumps from the
  numeric-neutral stream.
- Retain and restore execution cursors through bounded engine work.
- Test nested branches, empty initialization, and repeated pixels.

#### 3.4. Selected Functions And Random State

- Capture `fn1` through `fn4` choices for explicit evaluation.
- Preserve formula RNG, `srand()`, and per-image/per-pixel sequencing.
- Compare seeded behavior with the current interpreter.
- Do not use process-wide trig operand pointers from the new evaluator.

#### 3.5. Activate And Remove Compatibility Execution

- Route existing double formula entry points through the explicit runtime.
- Preserve publication of `g_old_z`, `g_new_z`, and engine overflow.
- Remove the transitional executable pointer stream/adapters once no
  callers need them; do not remove unrelated built-in trig bindings.
- Retain parser diagnostics and double image/semantic regressions.

This slice does not claim that all engine state is thread-isolated.

### Slice 4: Explicit Formula Inputs

- Extract current implicit engine-input population from compilation into
  per-image/per-pixel setup.
- Capture parameters, built-ins, function selection, and coordinate
  inputs in explicit data.
- Keep double behavior and diagnostic output unchanged.
- Verify execution uses captured values after unrelated globals change.
- Document the BF source for each precision-sensitive input.

Compilation should describe the program, not bind it to one image.

### Slice 5: BF Context And Storage

#### 5.1. Borrowed Precision Context

- Adapt an initialized `BIG_FLT` precision/scratch environment.
- Make representation, precision, lifetime, and non-reentrancy explicit.
- Reuse current precision tests and setup rather than assuming
  `init_bf_dec()` selects BF independently of fractal support.
- Verify that borrowing a context preserves existing values and restores
  temporary stack offsets on success and failure.

#### 5.2. Runtime Value Storage

- Define ownership of scalar/complex buffers and their raw-pointer views.
- Bind stable variable and stack storage for a fixed precision.
- Parse/format values through existing BF conversion operations.
- Test pointer lifetime, isolation of stored values, and invalidated
  bindings after precision changes.
- Keep tests independent of parser and render setup except for the
  explicitly acknowledged legacy math initialization fixture.

Do not advertise multiple simultaneous independent arenas.

### Slice 6: Core BF Operations

Reuse/extract existing arithmetic with explicit operands and context.

#### 6.1. Basic Arithmetic

- Add, subtract, multiply, negate, and compare.
- Test exact representable results, cancellation, and operand aliasing.
- Keep formula and renderer globals outside the operation interface.

#### 6.2. Division And Complex Projections

- Divide, reciprocal, real/imaginary parts, conjugation, flip, and
  identity.
- Define zero/domain behavior and operation-local failure state.
- Test preservation of operands and bounded scratch lifetime.

#### 6.3. Modulus And Boolean Results

- Modulus squared, comparisons, and required boolean operations.
- Preserve the formula language's complex/real truth conventions.
- Test equal/below/above thresholds and zero results.

Do not replace existing low-level BF arithmetic with another implementation
solely to change its calling shape.

### Slice 7: BF Noninteger Power

#### 7.1. Reusable Log And Power Foundations

- Isolate the existing complex BF log/power helpers and their dependencies
  at the proper math boundary.
- Reuse scalar BF log, exp, sin/cos, and atan2 support.
- Add only missing composition/contract handling, including complex exp
  where the formula language needs it.
- Preserve existing built-in callers while validating branch, zero,
  aliasing, and overflow behavior.

#### 7.2. Precision Regressions

- Check real and complex powers against independent reference values.
- Cover the nearby exponents in the target expression.
- Compare stable digits at two supported precisions.
- Prove no unsupported-operation fallback supplies a double result.
- Use exact strings only for representable/defined formatting cases;
  otherwise compare against an explicit numerical error bound.

A diagnostic fallback counter is not a substitute for numerical proof.

### Slice 8: BF Formula Evaluation

#### 8.1. Variables, Stack, And Core Instructions

- Add `FormulaRuntimeBf` over the shared program and owned value storage.
- Execute the supported operations from slices 6 and 7.
- Test hand-built programs before adding compilation/setup dependencies.
- Reject invalid operands, stack underflow, and unsupported instructions
  with clear diagnostics, not assertions.

#### 8.2. Program Binding And Capability Checks

- Bind parsed literal spellings and explicit BF built-ins/parameters.
- Classify the program and selected functions before BF evaluation.
- Test shared-program, separate-runtime values and initialization/orbit
  sections, including unsupported branch bodies.
- Do not claim language coverage from testing only `z*z+c`.

The numeric evaluator still has no keyboard, UI, or render-loop callbacks.

### Slice 9: BF Entry Points And Dispatch Ownership

#### 9.1. Compilation Versus Backend Binding

- Separate program compilation from installation of double callbacks.
- Preserve failure-safe dispatch and existing parser/selection callers.
- Add an explicit setup step that binds the selected numeric callbacks.
- Test that compiling/recompiling cannot overwrite a BF selection.

#### 9.2. BF Formula Entry Points

- Add BF per-image, per-pixel, and orbit entry points using the tested
  runtime and input boundary.
- Supply precise pixel/inversion values and accepted parameter text.
- Keep BF state across iterations; publish legacy views only outward.
- Test direct forced-BF setup/evaluation without advertising general
  formula `BF_MATH` support yet.
- Retain current return meanings and cleanup on failure.

Normal engine selection is connected in slice 12.  Do not add a
production preprocessor feature guard or a new debug flag just for tests.

### Slice 10: Target Formula Regression

- Add verified `MandAutoCritInZ` and `Straight_Forward` fixtures, or a
  minimal equivalent where provenance/content is not yet available.
- Exercise the target expression through forced BF runtime/entry tests.
- Check orbit values against a high-precision reference and stability
  across two precisions.
- Keep the current double result as a documented comparison, not the BF
  correctness oracle.
- Reserve full render/save comparisons for normal integration in slice 12.

No test should depend on an untracked local formula or DOS installation.

### Slice 11: Automatic Precision Requests

#### 11.1. Detection Without Switching

- Instrument candidate cancellation/precision indicators.
- Test finite, zero, nonfinite, exact-cancellation, and scale cases.
- Establish a validated policy from target and ordinary formulas.
- Keep observation independent of the decision to change representation.

#### 11.2. Restart Policy And Lifecycle

- Specify pixel-versus-image restart and its consequences before coding.
- Let the calculation owner apply precision requests from original inputs
  at a bounded lifecycle boundary.
- Preserve work, counters, image consistency, and seeded random behavior.
- Test cancellation, errors, output side effects, and arena lifetime.
- Do not recursively render or initialize BF beneath an active runtime.

#### 11.3. Controls And Capability Gates

- Honor accepted precision overrides and disable settings.
- Test the second `mathtolerance` field and debug-override precedence.
- Diagnose an unsupported required-BF program before mixed output is
  accepted.
- Keep automatic switching disabled until the normal engine path in
  slice 12 implements those guarantees.

The lost-bit expression is proposed instrumentation, not a fixed threshold
already proved by the current implementation.

### Slice 12: Normal Formula BF Integration

#### 12.1. Alternate Math Registration And Setup

- Register formula `BIG_FLT` callbacks in the const alternate table and
  add the specification's `BF_MATH` capability.
- Integrate explicit backend binding with current engine image setup.
- Reuse coordinate precision initialization and existing parameter
  conversion, adding precise source handling where it is missing.
- Implement/test formula forcing through applicable existing controls;
  do not assume the built-in-only `debugflag=3200` branch covers it.
- Preserve ordinary double selection and supported-program diagnostics.

#### 12.2. Restart, Resume, And Persistence

- Activate the tested automatic restart policy.
- Verify suspend/resume and setup/failure cleanup through
  `StandardFractal`, without making formula another image renderer.
- Check existing GIF precision/corner/parameter extensions, PAR output,
  history, and formula identity before proposing additional metadata.
- Verify saved/reloaded BF math and precision rather than serializing
  arena pointers, bytecode addresses, or a live operand stack.
- Add stable target-render comparisons where appropriate; do not demand
  exact DOS raster identity for improved transcendental arithmetic.

#### 12.3. UI Status And Documentation

- Extend existing precision status/diagnostic presentation.
- Document supported functions, selection controls, and limitations.
- Keep setup warnings/errors on the established UI reporting path.
- Automate interactions where existing controller/autokey coverage can
  observe them; require manual testing only for an identified native gap.

### Slice 13: Remaining Language Coverage

Add unsupported function families in separate commits, each with
capability updates and focused tests:

- Remaining trig, hyperbolic, and inverse functions.
- Rounding, absolute value, magnitude, and any remaining boolean rules.
- Selected-function bindings not covered by the initial subset.
- BF random/`srand()` behavior matching the defined double sequence policy.

Use one scenario per test, parameterized inputs where appropriate, and
branch/quadrant/zero cases for branch-sensitive functions.
Do not call support complete while unimplemented operations silently
fall back to double.

### Slice 14: Diagnostics And Cleanup

- Reuse existing parser/interpreter tracing for operation identities and
  add optional precision-request, restart, and selected-digit diagnostics.
- Do not invent a new public parameter merely to duplicate existing debug
  output; decide an additional control only if the current facility is
  insufficient.
- Remove remaining temporary bridges and document genuine limitations.
- Verify tracing does not alter computed values or lifecycle.
- Keep explicit failures for unsupported math and invalid runtime use.

## Validation And Test Placement

- Reuse `tests/libid/fractals/test_parser.cpp` and interpreter/formula
  coverage for compiler and double compatibility.
- Put BF arithmetic/context tests under `tests/libid/math`; use existing
  string conversion and allocation coverage as foundations.
- Put compiled-program, runtime, and entry-point tests under
  `tests/libid/fractals`.
- Shared test-helper implementations and their tests belong under
  `tests/libid/test`, with headers under `tests/libid/include`.
- Register new source files in `tests/libid/CMakeLists.txt`.
- Keep fixtures responsible for setup/teardown; each test checks one
  scenario.  Parameterize cases differing only in inputs.
- Use exact state/order assertions and checked numerical error bounds,
  not source-grep tests or wall-clock precision expectations.
- Verify low-level arithmetic first, then VM/setup, and finally image
  lifecycle and persistence.  Tests must not rely on state from a prior
  test or claim reentrancy while borrowing shared math storage.

## Risks And Completion

The difficult work is no longer discovering whether BF arithmetic exists.
It is separating program/value ownership, preserving literal precision,
binding dispatch safely, defining reliable precision requests/restarts,
and covering the formula language without hidden narrowing.

The feature is complete only when supported formulas run through the
normal engine path with correct precision, lifecycle, diagnostics, and
persistence; ordinary formulas retain their double behavior.
Concurrent independent render contexts and broader
[ports-and-adapters work](hexagonal.md) remain separate concerns.
