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

## Current Problems

- Formula `rand` ignores `rseed=` because the interpreter seeds from
  current time directly.
- Some fractal types already call `set_random_seed()`, but still consume
  the process-global C RNG.
- Some fractal paths seed directly with `std::srand()`.
- UI code also uses the process-global C RNG.  Any UI code that calls
  `std::srand()` or `std::rand()` can poison image generation if image
  generation also uses that same global stream.

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

## Current Image RNG Consumers

Already seeded through `set_random_seed()`:

- `type=ant`
- `type=cellular`
- `type=diffusion`
- `type=plasma`
- perturbation random reference selection

Known gaps:

- formula interpreter default `rand`
- lyapunov random population seed
- IFS3D hardcoded `std::srand(1)`
- 3D randomize colors in `line3d.cpp`
- Lorenz and inverse Julia random-walk paths using `std::rand()`

UI or non-fractal RNG users to keep separate:

- intro screen
- evolver UI and evolver generation state
- palette rotate randomization
- random dot stereogram UI
- GIF view dithering
- starfield UI

## Slice 1: Random Seed Tests

Goal: lock down the current seed contract before changing consumers.

Work:

- Add focused tests for `set_random_seed()`.
- Verify fixed `rseed=` does not mutate `g_random_seed`.
- Verify non-fixed seeding uses the generated image seed policy.
- Verify repeat calls with the same fixed seed produce the same first
  values.
- Verify different fixed seeds produce different first values.
- Add a test proving image RNG output is unchanged by interleaved UI calls
  to `std::srand()` and `std::rand()`.

Tests:

- New `tests/libid/engine/test_random_seed.cpp`.
- Check exact state:
  - `g_random_seed`
  - `g_random_seed_flag`
  - first `random15()` values
  - first `random_int()` values

Done when:

- Existing `std::rand()` users are still unchanged.
- New tests describe the target behavior for later slices.

## Slice 2: Add Image RNG Helpers

Goal: make image RNG independent from the process-global C RNG.

Work:

- Move image RNG state into `engine/random_seed.cpp`.
- Keep `set_random_seed()` as the only public image seed function.
- Add `random15()`.
- Add `random_int(int limit)`.
- Add `random_unit()`.
- Preserve the current sequence used by existing image tests as closely as
  practical.
- Keep `std::rand()` and `std::srand()` available only for transitional
  callers and UI code.

Tests:

- Unit test deterministic sequences for fixed seeds.
- Unit test `random_int(limit)` bounds:
  - `limit == 1`
  - small limits
  - large limits
- Unit test `random_unit()` bounds:
  - greater than or equal to `0`
  - less than or equal to `1`
- Unit test UI poisoning:
  - seed image RNG
  - read one image RNG value
  - call `std::srand()` and `std::rand()`
  - verify the next image RNG value is unchanged from the expected sequence

Done when:

- Image RNG can be used without depending on process-global C RNG state.

## Slice 3: Convert Seeded Fractal Types

Goal: convert existing `set_random_seed()` users to consume image RNG
helpers.

Work:

- Convert `Ant.cpp` from `std::rand()` to image RNG helpers.
- Convert `Cellular.cpp` from `std::rand()` to image RNG helpers.
- Convert `Diffusion.cpp` from `std::rand()` to image RNG helpers.
- Convert `Plasma.cpp` from `RAND15()` to `random15()`.
- Convert `PertEngine.cpp` from `std::rand()` to image RNG helpers.
- Keep existing type-specific seed parameter handling:
  - ant `params[5]`
  - cellular random/reuse parameters
  - plasma `params[2]`

Tests:

- Existing image tests for random fractal types still pass:
  - ant random tests
  - cellular tests using `rseed=4567`
  - diffusion
  - plasma
- Add focused unit tests only where a type has direct seed logic:
  - ant reuse-last seed path preserves `g_random_seed`
  - plasma reuse-last seed path preserves `g_random_seed`
  - cellular reuse-last seed path preserves `g_random_seed`

Done when:

- These types no longer call `std::rand()` or `RAND15()`.
- Their image tests still prove repeatability.

## Slice 4: Fix Formula Default `rand`

Goal: make formula `rand` honor `rseed=`.

Work:

- Replace interpreter default time-based `random_seed()` with image RNG
  seeding through `set_random_seed()`.
- Move formula `rand` consumption off `RAND15()`.
- Keep formula-language `srand()` as an explicit formula-local seed.
- Ensure a formula using `rand` and no explicit formula `srand()` uses the
  image seed.
- Ensure a formula using explicit `srand()` remains repeatable independent
  of the global `rseed=` value after the formula seed is set.

Tests:

- Add a small formula using `rand`.
- Render or unit-test with `rseed=1234` twice and verify identical output.
- Render or unit-test with `rseed=1234` and `rseed=5678` and verify
  different random values.
- Verify formula explicit `srand()` produces the same sequence regardless
  of `g_random_seed`.
- Verify formula default `rand` mutates only formula runtime random state:
  - `g_runtime.rand_num`
  - `g_runtime.randomized`
  - `g_runtime.set_random`

Done when:

- Issue 363 is reproducible by a test before the change and fixed after the
  change.

## Slice 5: Convert Lyapunov

Goal: make lyapunov random population selection honor `rseed=`.

Work:

- Seed in `lyapunov_per_image()` when population seed mode needs random
  values.
- Replace `std::rand()` in `lyapunov_type()` with image RNG helpers.
- Keep fixed population seed behavior unchanged when `params[1]` is a
  normal explicit value.

Tests:

- Add a focused lyapunov test with `params[1] == 0` or `params[1] == 1`.
- Verify identical `rseed=` gives identical initial `g_population`.
- Verify different `rseed=` gives different `g_population`.
- Verify explicit `params[1]` does not consume the image RNG.

Done when:

- Lyapunov random population is repeatable with `rseed=`.

## Slice 6: Convert IFS3D And Lorenz Random Walk

Goal: remove hardcoded and unseeded random use from 3D IFS and Lorenz
paths.

Work:

- Replace `std::srand(1)` in IFS3D setup with `set_random_seed()`.
- Replace IFS3D `std::rand()` and `RAND15()` calls with image RNG helpers.
- Audit Lorenz random-walk and random-run paths.
- Seed Lorenz image-generation random paths through `set_random_seed()`.
- Do not change UI-only random selection behavior.

Tests:

- Add or update image tests for IFS3D with explicit `rseed=`.
- Verify repeated IFS3D render with the same `rseed=` is identical.
- Verify different `rseed=` changes the random transform sequence.
- Add a focused Lorenz random-walk test if the path can be isolated without
  broad UI setup.

Done when:

- No fractal-generation code in `lorenz.cpp` calls `std::srand()`,
  `std::rand()`, or `RAND15()`.

## Slice 7: Convert 3D Randomized Coloring

Goal: make 3D randomized color perturbation use the image RNG stream.

Work:

- Identify the 3D render setup point that corresponds to one image.
- Call `set_random_seed()` there when `randomize=` requires random colors.
- Replace `std::rand()` and `RAND15()` in `line3d.cpp` with image RNG
  helpers.
- Preserve `randomize=0` behavior.

Tests:

- Add or update a 3D image test using `randomize=` and explicit `rseed=`.
- Verify same `rseed=` gives the same randomized colors.
- Verify `randomize=0` does not consume image RNG state.
- Verify `randomize=` with no `rseed=` still varies between generated
  images.

Done when:

- 3D random coloring is not affected by UI RNG calls.

## Slice 8: Audit Remaining Fractal RNG Calls

Goal: make remaining image-generation random calls explicit.

Work:

- Audit all `std::rand()`, `std::srand()`, and `RAND15()` callers.
- Move any remaining fractal-generation caller to image RNG helpers.
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

## Slice 9: Save, Load, And Batch Round Trip

Goal: preserve repeatability through saved images and generated parameter
sets.

Work:

- Verify GIF save/load preserves `g_random_seed` and
  `g_random_seed_flag`.
- Verify generated parameter sets write `rseed=` when the seed was fixed.
- Decide whether generated parameter sets should write the generated seed
  when no fixed `rseed=` was supplied.  If yes, document and test it.

Tests:

- Load a GIF with stored seed data and verify:
  - `g_random_seed`
  - `g_random_seed_flag`
- Generate a PAR with fixed `rseed=` and verify `rseed=` is emitted.
- Generate a PAR without fixed `rseed=` and verify documented behavior.

Done when:

- Repeatability survives save/load and generated PAR workflows.

## Slice 10: Final Regression Pass

Goal: prove image RNG behavior is unified.

Work:

- Add a short developer note to the RNG code naming allowed RNG sources.
- Remove obsolete random helper comments that refer to C RNG.
- Update any stale documentation.

Tests:

- Run `cmake --workflow rt-default`.
- Run targeted image tests for:
  - formula `rand`
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
