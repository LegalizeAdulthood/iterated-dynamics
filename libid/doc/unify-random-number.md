# Unify Random Number Use

## Goal

All fractal image generation that consumes random numbers must seed from
`set_random_seed()`.

`set_random_seed()` is the single image RNG seed point:

- if `rseed=` was specified, seed from `g_random_seed`
- otherwise seed from current time or the existing generated image seed
- preserve existing "reuse last random" behavior where a fractal type
  documents it

UI random behavior is separate.  UI code may still use its own random
source, but UI random calls must not change the image-generation random
stream while an image is being calculated.

## Desired RNG Ownership

The engine owns an image RNG stream in `engine/random_seed`.

Public image RNG API:

- `set_random_seed()`
- `random_int(int limit)`
- `random15()`
- `random_unit()`

Fractal code must use these helpers instead of `std::rand()` or `RAND15()`.

Formula `srand()` remains formula-language behavior.  It should seed the
formula runtime random state, not the UI RNG and not unrelated image RNG
users.

## Audit Status

Implemented:

- `engine/random_seed` owns private image RNG state and exposes the image
  RNG helper API.
- `type=ant`, `type=cellular`, `type=diffusion`, `type=plasma`, and
  perturbation random reference selection consume image RNG helpers.
- Formula default `rand` seeds through `set_random_seed()`.
- Formula-language `srand()` seeds formula runtime random state only.
- Lyapunov random population modes seed through `set_random_seed()` and
  consume `random15()`.
- GIF save/load stores and restores `g_random_seed` and
  `g_random_seed_flag`.
- Generated parameter files emit `rseed=` when the seed was fixed.
- Random seed tests cover fixed and non-fixed seed behavior and prove C RNG
  calls do not affect image RNG output.
- Ant, cellular, and plasma reuse-last seed paths are covered by focused
  tests.
- Lyapunov random population seed behavior is covered by focused tests.
- IFS2D and IFS3D seed through `set_random_seed()` and honor `rseed=`.
- Lorenz and inverse Julia random-walk paths consume image RNG helpers.
- 3D randomized coloring seeds through `set_random_seed()` and consumes
  image RNG helpers.
- JIIM random inverse Julia fallback paths seed through `set_random_seed()`
  and consume image RNG helpers.
- The remaining C RNG audit matches are tests, UI-only callers, GIF view
  dithering, or the legacy `RAND15()` macro.

Remaining gaps:

- `RAND15()` remains a process-global C RNG macro.
- Generated parameter file behavior for non-fixed generated image seeds is
  undecided and untested.
- The final RNG source audit and developer note are not done.

## UI Or Non-Fractal RNG Users

These may keep a UI-local random source, but must not affect image RNG:

- intro screen
- evolver UI and evolver generation state
- palette rotate randomization
- random dot stereogram UI
- GIF view dithering
- starfield UI

## Remaining Work

### Slice 1: Save, Load, And Batch Round Trip

Goal: preserve repeatability through saved images and generated parameter
sets.

Work:

- Verify GIF save/load tests cover `g_random_seed` and
  `g_random_seed_flag`.
- Generate a PAR with fixed `rseed=` and verify `rseed=` is emitted.
- Decide whether generated parameter sets should write the generated seed
  when no fixed `rseed=` was supplied.
- If generated seeds are emitted, document and test that behavior.
- If generated seeds are not emitted, document and test that behavior.

Tests:

- Load a GIF with stored seed data and verify:
  - `g_random_seed`
  - `g_random_seed_flag`
- Generate a PAR with fixed `rseed=` and verify `rseed=` is emitted.
- Generate a PAR without fixed `rseed=` and verify documented behavior.

Done when:

- Repeatability survives save/load and generated PAR workflows.

### Slice 2: Final Regression Pass

Goal: prove image RNG behavior is unified.

Work:

- Add a short developer note to the RNG code naming allowed RNG sources.
- Remove obsolete random helper comments that refer to C RNG.
- Remove `RAND15()` or confine it to explicit UI or compatibility callers.
- Update stale documentation.

Tests:

- Run `cmake --workflow rt-default`.
- Run targeted image tests for:
  - ant
  - cellular
  - diffusion
  - plasma
  - lyapunov
  - IFS3D
  - 3D randomized coloring
- Run `rg` audits for random calls.

Done when:

- Fixed `rseed=` makes all fractal random behavior repeatable.
- Missing `rseed=` uses a generated seed.
- UI random calls do not affect image random calls.
- Issue 363 is fixed.
