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

Remaining gaps:

- 3D randomized coloring in `line3d.cpp` still uses `RAND15()` and
  `std::rand()`.
- JIIM random inverse Julia paths still use `std::rand()`.
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

### Slice 1: Convert 3D Randomized Coloring

Goal: make 3D randomized color perturbation use the image RNG stream.

Work:

- Identify the 3D render setup point that corresponds to one image.
- Call `set_random_seed()` there when `randomize=` requires random colors.
- Replace `std::rand()` and `RAND15()` in `line3d.cpp` with image RNG
  helpers.
- Preserve `randomize=0` behavior.

Tests:

- Add or update a 3D image test using `randomize=` and explicit `rseed=`.
- Verify the same `rseed=` gives the same randomized colors.
- Verify `randomize=0` does not consume image RNG state.
- Verify `randomize=` with no `rseed=` still varies between generated
  images.

Done when:

- 3D random coloring is not affected by UI RNG calls.

### Slice 2: Audit Remaining Image RNG Calls

Goal: make all remaining image-generation random calls explicit.

Work:

- Audit all `std::rand()`, `std::srand()`, and `RAND15()` callers.
- Convert JIIM random inverse Julia callers to image RNG helpers, or prove
  they are not image-generation callers and document that classification.
- Move any other fractal-generation caller to image RNG helpers.
- Mark non-image UI callers with short comments only where the
  classification is not obvious.
- Keep UI RNG separate from image RNG.

Tests:

- `rg` audit must show no image-generation use of:
  - `std::rand()`
  - `std::srand()`
  - `RAND15()`
- Remaining matches must be UI, tests, or explicit compatibility code.
- Run existing image tests.

Done when:

- Every fractal type uses `set_random_seed()` before consuming image RNG.
- UI RNG cannot poison image-generation RNG.

### Slice 3: Save, Load, And Batch Round Trip

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

### Slice 4: Final Regression Pass

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
