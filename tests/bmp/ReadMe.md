<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Big Number Migration Images

These PAR files and pre-migration GIF gold images preserve three deep
views for comparison after the arbitrary-precision backend migration.
They use the legacy implementation at source revision
`360b5474dab4e68321194682767436c8d6da105c`, built with the `rt-default`
Release preset on Windows.

All views use the default palette, 40 decimal digits, 300 iterations,
one-pass rendering, modulus bailout at 4, and disabled periodicity. F6 in
the repository's `home/id.cfg` selects 800 by 600 pixels.

| PAR entry | View |
| --- | --- |
| `mandel-direct` | Mandelbrot centered at i, direct arithmetic |
| `mandel-perturb` | Same Mandelbrot view with perturbation |
| `julia-direct` | Julia c=-1 at the positive real fixed point |

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

To render additional images for visual review, use PowerShell from the
source root:

```powershell
$source = (Get-Location).Path
$build = [IO.Path]::GetFullPath("$source/../build-rt-default")
$exe = "$build/win32/Release/id.exe"
$views = 'mandel-direct', 'mandel-perturb', 'julia-direct'
foreach ($view in $views) {
    $par = Join-Path $source "tests\bmp\$view.par"
    $arguments = @(
        'batch=yes', 'overwrite=yes', 'savedir=.'
        "@$par/$view"
        "savename=check-$view.gif"
    )
    $render = Start-Process -FilePath $exe -ArgumentList $arguments `
        -WorkingDirectory "$source/home" -WindowStyle Hidden `
        -Wait -PassThru
    if ($render.ExitCode -ne 0) { throw "Render failed: $view" }
}
```

The generated images are in `home/image/check-*.gif`. Compare each with
its corresponding `gold-*.gif` here using the same palette and settings.
Look for blank output, incorrect framing, and large changes in the escape
pattern. Compare direct and perturbation images to their own baselines;
legacy rounding can produce differences between the two modes.

Keep these pre-migration gold images when the backend changes. Numerical
differences require review rather than automatic replacement of the gold
images. GIF preserves the indexed pixels and palette without loss; its
saved fractal metadata also records the view and precision.
