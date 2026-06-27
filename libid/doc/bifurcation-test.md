<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Bifurcation Test Plan

Goal: test bifurcation fractal behavior without requiring exact rendered
pixels on every platform.

Bifurcation images are sensitive to small floating-point differences.  The
renderer iterates a one-dimensional population sequence, then maps each
population value to a screen row.  Small differences in `sin`, `pow`, or
rounding can shift later values and move many pixels while leaving the
fractal behavior valid.

The tests should ensure the sequence of iterated values has not regressed
dramatically.  Small platform differences are acceptable.

## Scope

Cover these fractal types:

- `bifurcation`
- `biflambda`
- `bifmay`
- `bifstewart`
- `bif+sinpi`
- `bif=sinpi`

Leave `Killer_Mosquito` alone for now.

## Test Strategy

Prefer direct sequence and statistical tests over exact image comparison.

For stable regions:

- Run a fixed number of recurrence steps.
- Compare early population values to a reference sequence.
- Use absolute and relative tolerances.
- Verify bailout or no-bailout behavior.

For chaotic regions:

- Burn in the sequence.
- Compare aggregate behavior instead of every later value.
- Check minimum and maximum population.
- Check mean and variance.
- Bucket population values into a fixed histogram.
- Compare histogram buckets with tolerance.

This catches wrong formulas, wrong parameters, wrong trig setup, bad May
beta handling, broken bailout behavior, and major sequence drift.

## Implementation Slices

### 1. Add Chaotic Summary Cases

Add chaotic-region cases for sensitive types such as `bifmay`,
`bif+sinpi`, and `bif=sinpi`.

Each case should:

- run enough iterations to exercise chaotic behavior
- discard an initial burn-in window
- compute min, max, mean, variance, and histogram buckets
- compare those values with platform-tolerant thresholds

Do not compare every value after burn-in.

### 2. Add Column-Level Coverage

After the renderer state is easier to call directly, expose or test the
single-column calculation shape.

For selected rates, compare:

- total visible hits
- occupied row count
- first and last occupied row
- centroid row
- coarse row bucket occupancy

This verifies the population-to-row mapping without comparing a full GIF.

### 3. Reduce Full-Image Exactness

Replace platform-sensitive full-image exact tests for bifurcation types
with the sequence and column tests above.

Exact image golds may remain for one reference platform if useful, but do
not add more platform-specific gold files just to handle ordinary math
library differences.

Keep a lightweight smoke test if needed:

- run the image command
- verify the image file is written
- verify it is non-empty and has the expected dimensions
- avoid exact pixel comparison for sensitive default views

