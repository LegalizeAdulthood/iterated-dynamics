<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Big Number Migration Images

These PAR files and GIF gold images establish the legacy BigFloat
baseline for the Boost.Multiprecision migration. They were regenerated
from the BigFloat Mandelbrot and Julia implementation at source revision
`449e02c5e1e6419f893ef9aa2b62c2164cf5f4eb`, built with Windows/MSVC,
the `x64-windows` vcpkg triplet, and the `rt-default` Release preset.

All views use the default palette, 40 decimal digits, 300 iterations,
one-pass rendering, modulus bailout at 4, and disabled periodicity. F6 in
the repository's `home/id.cfg` selects 800 by 600 pixels.

| PAR entry | View |
| --- | --- |
| `mandel-direct` | Mandelbrot centered at i, direct arithmetic |
| `mandel-perturb` | Same Mandelbrot view with perturbation |
| `julia-direct` | Julia c=-1 at the positive real fixed point |

## Baseline History

The original BigNum-era images were rendered from source revision
`360b5474dab4e68321194682767436c8d6da105c`. Those gold files remain
available in Git at revision
`449e02c5e1e6419f893ef9aa2b62c2164cf5f4eb`, before this refresh.

All three regenerated images have exactly the same indexed pixels,
palette, dimensions, and 572-byte high-precision parameter payload as
their original golds. Their GIF fractal metadata now records BigFloat
arithmetic (`bf_math=2`) instead of BigNum (`bf_math=1`), with the same
stored precision length of 20.

Two independent renders of each view produced identical indexed pixels,
palettes, and stable metadata. Whole GIF files can differ because the
calculation time and unforced random seed are recorded in their metadata.
These runtime fields do not change the image comparison result.

## Reproduction

The following registered image tests render the PAR inputs and compare
their indexed pixels and palette with the corresponding gold images:

- `ImageTest.bmp-mandel-direct`
- `ImageTest.bmp-mandel-perturb`
- `ImageTest.bmp-julia-direct`

They use `add_image_test` with separate test homes and the `image`, `bmp`,
and `precision` labels. Failures retain the actual and gold images and a
difference image under `build-rt-default/test-artifacts`.

Run the complete workflow, including these tests, from the source root:

```powershell
cmake --workflow rt-default
```

For a focused run, set the `temp` test preset's include-name filter to
`^ImageTest[.]bmp-(mandel-direct|mandel-perturb|julia-direct)$`, set its
build targets to `id` and `image-compare`, and run:

```powershell
cmake --workflow temp
```

## Gold Refresh

Use the focused preset configuration above. From the source root, render
once and retain the output before rendering again:

```powershell
$build = (Resolve-Path ../build-rt-default).Path
$repeat = Join-Path $build bigfloat-gold-repeat
$views = 'mandel-direct', 'mandel-perturb', 'julia-direct'
New-Item -ItemType Directory -Path $repeat -Force | Out-Null
cmake --workflow temp
if ($LASTEXITCODE -ne 0) { throw 'First render failed' }
foreach ($view in $views) {
    Copy-Item "$build/tests/bmp/test-bmp-$view.gif" `
        "$repeat/first-$view.gif" -Force
}
cmake --workflow temp
if ($LASTEXITCODE -ne 0) { throw 'Repeat render failed' }
```

Compare the two renders with the repository's indexed-pixel and palette
comparison tool. Review differences from the old golds before replacing
them; investigate blank output, incorrect framing, or large escape-pattern
changes. Compare each view with its own baseline.

```powershell
foreach ($view in $views) {
    & "$build/tests/tools/Release/image-compare.exe" `
        "$repeat/first-$view.gif" `
        "$build/tests/bmp/test-bmp-$view.gif"
    if ($LASTEXITCODE -ne 0) { throw "Repeat mismatch: $view" }
}
```

After review, copy the regenerated images and validate the refreshed
golds through both workflows:

```powershell
foreach ($view in $views) {
    Copy-Item "$build/tests/bmp/test-bmp-$view.gif" `
        "tests/bmp/gold-$view.gif" -Force
}
cmake --workflow temp
if ($LASTEXITCODE -ne 0) { throw 'Focused image tests failed' }
cmake --workflow rt-default
if ($LASTEXITCODE -ne 0) { throw 'Full workflow failed' }
```

Use these BigFloat golds for subsequent Boost comparisons. Numerical
differences require review before any further refresh. GIF preserves
indexed pixels and palette without loss; its saved fractal metadata also
records the view, arithmetic mode, and precision.
