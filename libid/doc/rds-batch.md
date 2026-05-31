# Batch RDS With Separate Filename Parameter

## Summary

Add two command keywords:

```text
rds=
rds-texture=
```

Reuse existing controls:

```text
stereowidth=<float>
usegrayscale=yes|no
savename=<filename>
savedir=<directory>
```

`rds=` enables batch RDS and carries non-filename RDS settings.
`rds-texture=` sets the texture GIF filename.

## Public Interface

Syntax:

```text
rds=no
rds=yes[/<depth>[/<bars>]]
rds=random[/<depth>[/<bars>]]
rds=texture[/<depth>[/<bars>]]
rds-texture=<filename>
```

Field meanings:

```text
depth  integer for g_auto_stereo_depth
bars   none|middle|top or 0|1|2
```

Defaults:

```text
rds=yes       same as rds=random
depth         existing g_auto_stereo_depth default, currently 100
bars          existing g_calibrate default, currently middle
rds-texture   required when rds=texture and no prior texture exists
```

Examples:

```text
rds=yes
rds=yes/-120/top
rds=random/80/none
rds=texture/100/middle rds-texture=textures\grain.gif
```

## Parameter Sets

Use these as image-test inputs after copying the named depth and texture
GIF fixtures into the test working directory.

```text
rds-random-computed {
  batch=yes video=F6 overwrite=yes savedir=.
  type=mandel corners=-2.0/1.0/-1.2/1.2 maxiter=150
  rseed=12345 rds=random/100/none
  savename=rds-random-computed.gif
}

rds-random-loaded {
  batch=yes video=F6 overwrite=yes savedir=.
  filename=rds-depth.gif
  rseed=12345 rds=yes/100/middle
  savename=rds-random-loaded.gif
}

rds-random-gray {
  batch=yes video=F6 overwrite=yes savedir=.
  type=mandel corners=-2.0/1.0/-1.2/1.2 maxiter=150
  rseed=12345 rds=random/-120/top
  usegrayscale=yes stereowidth=4.5
  savename=rds-random-gray.gif
}

rds-texture-computed {
  batch=yes video=F6 overwrite=yes savedir=.
  type=mandel corners=-2.0/1.0/-1.2/1.2 maxiter=150
  rds=texture/100/none rds-texture=rds-texture.gif
  savename=rds-texture-computed.gif
}

rds-texture-loaded {
  batch=yes video=F6 overwrite=yes savedir=.
  filename=rds-depth.gif
  rds=texture/-120/top rds-texture=rds-texture.gif
  usegrayscale=yes stereowidth=4.5
  savename=rds-texture-loaded.gif
}

rds-disabled-control {
  batch=yes video=F6 overwrite=yes savedir=.
  type=mandel corners=-2.0/1.0/-1.2/1.2 maxiter=150
  rds=no
  savename=rds-disabled-control.gif
}
```

## Docs And Tests

- Document `rds=` and `rds-texture=` in parameter help and RDS help.
- Parser tests: `rds=no`, `rds=yes`, depth, bars names, numeric bars,
  empty default fields, invalid mode, invalid depth, invalid bars,
  `rds-texture=`.
- Lowercase test: `RDS-TEXTURE=C:\Tmp\Texture.GIF` preserves filename case.
- Batch image tests covering all parameter sets above.
- Run `cmake --workflow rt-default`.

## Assumptions

- Two new keywords are the minimum once filenames must be their own
  parameter.
- `rds-texture=` selects texture input; `rds=texture` is still required to
  request batch RDS generation.
- `stereowidth=` and `usegrayscale=` remain the existing width and
  depth-source controls.
