# Plan: Convert make_mig to giflib

## Goal

Replace the byte-oriented GIF stitching in `libid/io/make_mig.cpp` with the
giflib API supplied by vcpkg. Keep the existing behavior first: read
`frmig_xy.gif` tile files, write `fractmig.gif`, preserve the first tile's
global screen/color-map data, offset each tile image into the output canvas,
copy GIF extensions only from the last tile, and delete input tiles only after
a successful write.

`libid` already links `GIF::GIF`, so the implementation should only need
`#include <gif_lib.h>` plus local wrappers.

## Current Behavior To Preserve

- Input tile names are `frmig_00.gif`, `frmig_10.gif`, ..., using
  `par_key()` for x/y indices.
- The output logical screen is the first tile's logical screen dimensions
  multiplied by `x_mult` and `y_mult`.
- Every image descriptor from every input GIF is copied to the output, with
  left/top adjusted by `x_step * tile_width` and `y_step * tile_height`.
- The first tile's global color table is written to the output.
- Later tiles must match the first tile's logical screen dimensions and global
  color-table size. A later cleanup can compare the actual color values too.
- Extension blocks are read from every tile but written only from the last
  tile, matching the current raw-copy logic.
- Fractint extension blocks are not decoded, migrated, recomputed, or updated
  for the stitched dimensions. The last tile's extension bytes survive
  verbatim; earlier tiles' extension bytes are dropped.
- Input tiles are removed only if the output is created successfully.

## giflib API Shape

Use the high-level in-core API:

- `DGifOpenFileName(path, &error)` to open each tile.
- `DGifSlurp(gif)` to parse screen state, saved images, raster data, local
  color maps, image extension blocks, and trailing extension blocks.
- `EGifOpenFileName(path, false, &error)` to create the output.
- `GifMakeMapObject(...)` to copy the first tile's global color map.
- `GifMakeSavedImage(out, &image)` to append a deep copy of each saved image.
- `EGifSpew(out)` to write the output GIF and close it.
- `DGifCloseFile(...)` to close each input.

This avoids hand-parsing GIF headers, image descriptors, LZW minimum code
sizes, sub-blocks, local color tables, extension chains, and the trailer.

## Implementation Slices

Each slice should compile on its own. Add or adjust tests with the slice that
changes behavior.

1. Linkage and include slice

   Add `#include <gif_lib.h>` to `make_mig.cpp` and prove the existing target
   still builds against the already-linked `GIF::GIF` dependency.

2. Input wrapper slice

   Add a local `InputGif` RAII wrapper for `DGifOpenFileName`,
   `DGifSlurp`, and `DGifCloseFile`. Use it in a narrow helper that reads one
   named tile and returns an owned slurped GIF. Keep the existing byte-copy
   implementation in place while this helper is unused or covered by a small
   test.

3. Output wrapper slice

   Add a local `OutputGif` RAII wrapper for `EGifOpenFileName` and
   `EGifSpew`. Make the wrapper explicit about ownership transfer because
   `EGifSpew` writes the trailer and closes the file.

4. Metadata validation slice

   Replace the raw header reads with giflib metadata reads:
   `SWidth`, `SHeight`, `SColorMap->ColorCount`, `SColorResolution`,
   `SBackGroundColor`, and `AspectByte`. Preserve the current checks for tile
   dimensions and color-table size. Add a 16-bit canvas-size check before
   creating the output.

5. Output screen slice

   Create the output `GifFileType` through giflib and assign the logical
   screen fields from the first tile. Copy the first tile's global color map
   with `GifMakeMapObject`. Do not write images yet.

6. Single-image copy slice

   Copy the first tile's first `SavedImage` with `GifMakeSavedImage`, write it
   with `EGifSpew`, and verify the output slurps successfully with the same
   screen, color map, image descriptor, and raster pixels. Compare decoded
   pixels, not file bytes.

7. Tile-offset slice

   Extend image copying to all tiles. After each `GifMakeSavedImage`, adjust
   `ImageDesc.Left` and `ImageDesc.Top` by `x_step * tile_width` and
   `y_step * tile_height`. Verify a 2x2 stitch has the expected output screen
   size, image count, and image offsets.

8. Multi-image tile slice

   Preserve the legacy loop over every image descriptor by copying every
   `SavedImage` from each tile, not just image zero. Verify image count equals
   the sum of all tile image counts.

9. Extension slice

   Match the current extension policy: discard copied extension blocks for
   non-last tiles, preserve image-local and trailing extension blocks only from
   the last tile, and verify only the last tile's extension survives. Do not
   migrate Fractint extension contents; preserve the last tile's bytes
   verbatim.

10. Cleanup and errors slice

    Replace the old `error_flag` and `input_error_flag` paths with giflib
    error handling. Keep input tiles until all operations succeed. Delete input
    tiles only after `EGifSpew` succeeds and all GIF handles are closed.

11. Remove raw I/O slice

    Delete the remaining `std::FILE`, `std::fread`, `std::fwrite`,
    byte-buffer, `std::memcpy`, and `std::memset` logic from `make_mig.cpp`.
    Keep `par_key()` and the user-visible messages unless a test requires a
    wording update.

12. Full verification slice

    Run the focused make_mig tests, then run `cmake --workflow rt-default` in
    the top-level source directory.

## Extension Handling Detail

`DGifSlurp` stores extension blocks in two places:

- `SavedImage::ExtensionBlocks` for extensions immediately before an image.
- `GifFileType::ExtensionBlocks` for extensions after the last image.

The existing byte copier writes extensions only while processing the final
tile. The giflib implementation should match that first, even if the
extensions are attached to a saved image. This is preservation, not migration:
Fractint extension blocks from the last tile are emitted unchanged, and
Fractint extension blocks from earlier tiles are discarded. A future
improvement can decode and rewrite Fractint extension data through
`io/gif_file.cpp` if the stitched GIF should advertise the combined screen
dimensions in metadata.

## Tests

Add focused tests before replacing the implementation:

- Build four small GIF tiles in a temporary directory with giflib, run
  `make_mig(2, 2)`, then slurp `fractmig.gif` and verify:
  - output screen size is doubled in both axes,
  - image count matches the sum of tile images,
  - image left/top offsets match tile positions,
  - global color map matches the first tile.
- Add a tile-size mismatch test and verify the output fails and inputs remain.
- Add a color-table-size mismatch test matching current behavior.
- Add an extension test showing only the last tile's extension survives.
- Add a Fractint extension test showing that the last tile's extension bytes
  are unchanged rather than migrated.
- Compare raster pixels after slurp, not compressed bytes; giflib may emit
  different LZW bytes for identical raster data.

Run `cmake --workflow rt-default` after the implementation.

## Non-Goals

- Do not manually parse GIF byte fields after the conversion.
- Do not require bitwise-identical GIF output.
- Do not delete input tile files after partial output failure.
- Do not change Fractint metadata contents in the first migration pass.
