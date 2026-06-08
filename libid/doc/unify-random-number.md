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

Fractal code must use these helpers instead of `std::rand()` or UI-local
random helpers.

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
- Generated parameter files emit `rseed=` when the seed was fixed and
  omit generated image seeds.
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
- The legacy 15-bit C RNG macro was removed.  Remaining C RNG audit
  matches are tests, UI-only callers, formula-runtime seeding, or GIF view
  dithering.
- `engine/random_seed` documents allowed image, formula, and UI random
  sources.

Remaining gaps:

- None.

## UI Or Non-Fractal RNG Users

These may keep a UI-local random source, but must not affect image RNG:

- intro screen
- evolver UI and evolver generation state
- palette rotate randomization
- random dot stereogram UI
- GIF view dithering
- starfield UI

## Remaining Work

None.
