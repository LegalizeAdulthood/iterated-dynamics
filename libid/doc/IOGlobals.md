# Global Variables in libid/include/io

This document identifies groups of global variables in the `libid/include/io` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in I/O operations, file handling, and image encoding/decoding.

## 1. File Loading/Reading Group

**File:** `loadfile.h`

Variables for loading fractal images and managing file state:

- `g_bad_outside` - flag indicating invalid outside color mode in loaded file
- `g_fast_restore` - true = reset view windows prior to restore, skip warnings on video mode changes
- `g_file_aspect_ratio` - aspect ratio of loaded file
- `g_file_colors` - number of colors in loaded file
- `g_file_x_dots` - horizontal resolution of loaded file
- `g_file_y_dots` - vertical resolution of loaded file
- `g_loaded_3d` - flag indicating 3D image was loaded
- `g_overlay_3d` - 3D overlay flag
- `g_new_bifurcation_functions_loaded` - flag for new bifurcation function format
- `g_skip_x_dots` - number of x pixels to skip during load
- `g_skip_y_dots` - number of y pixels to skip during load
- `g_file_version` - version of loaded file format
- `g_read_filename` - path to file being read
- `g_show_file` - file display status (REQUEST_IMAGE, LOAD_IMAGE, IMAGE_LOADED)

## 2. File Saving/Writing Group

**File:** `encoder.h`

Variables for saving fractal images:

- `g_block[]` - buffer for encoder block data
- `g_save_filename` - path to file for saving

## 3. Color Map/Palette Group

**File:** `loadmap.h`

Variables for color map handling:

- `g_last_map_name` - name from last color map load/save or colors=@filename
- `g_map_clut[256][3]` - color lookup table (default colors from map=)
- `g_map_specified` - flag indicating map= was specified

## 4. GIF Decoding Group

**File:** `gifview.h`

Variables for GIF image decoding:

- `g_dither_flag` - true if dithering is enabled for GIF display
- `g_height` - height of GIF being decoded
- `g_num_colors` - number of colors in GIF
- `g_out_line` - function pointer for output line during decoding (in `decoder.h`)

## 5. Library/Search Path Group

**File:** `library.h`

Variables for file search paths and directories:

- `g_check_cur_dir` - flag to check current directory for files
- `g_fractal_search_dir1` - first additional search directory for fractal files
- `g_fractal_search_dir2` - second additional search directory for fractal files

## 6. Special Directories Group

**File:** `special_dirs.h`

Variables for system and working directories:

- `g_special_dirs` - shared pointer to platform-specific directory interface
- `g_save_dir` - directory for saving files
- `g_temp_dir` - name of temporary directory
- `g_working_dir` - name of directory for miscellaneous files

## 7. Configuration Loading Group

**File:** `load_config.h`

Variables for configuration file handling:

- `g_bad_config` - configuration load status (OK, BAD_WITH_MESSAGE, BAD_NO_MESSAGE)
- `g_cfg_line_nums[MAX_VIDEO_MODES]` - array mapping video modes to config file line numbers

## 8. Auto-Save/Timed Save Group

**File:** `save_timer.h`

Variables for automatic periodic saves:

- `g_resave_flag` - resave status (NONE, STARTED, FINAL)
- `g_save_time_interval` - interval in minutes between automatic saves
- `g_started_resaves` - flag indicating auto-save has been started
- `g_timed_save` - timed save status (NONE, STARTED, FINAL)

## Usage Patterns

These variable groups follow typical I/O operation patterns:

1. **File Loading Flow**: 
   - Check `g_read_filename` and `g_file_version`
   - Load image properties (`g_file_x_dots`, `g_file_y_dots`, `g_file_colors`, `g_file_aspect_ratio`)
   - Set state flags (`g_loaded_3d`, `g_overlay_3d`, `g_bad_outside`)
   - Update display status with `g_show_file`

2. **File Saving Flow**:
   - Set `g_save_filename`
   - Use `g_block[]` buffer for encoding
   - Respect `g_save_time_interval` for auto-saves
   - Update `g_timed_save` and `g_resave_flag` status

3. **Color Map Handling**:
   - Check `g_map_specified` flag
 - Load palette into `g_map_clut[]`
   - Remember name in `g_last_map_name`

4. **GIF Decoding**:
   - Set up `g_height`, `g_num_colors`
   - Configure `g_dither_flag`
   - Use `g_out_line` function pointer for line output

5. **File Search**:
   - Check `g_check_cur_dir` first
   - Search `g_fractal_search_dir1` and `g_fractal_search_dir2`
   - Use `g_special_dirs` for system paths
   - Use `g_working_dir`, `g_save_dir`, and `g_temp_dir` for file operations

6. **Configuration**:
   - Load settings and populate `g_cfg_line_nums[]`
   - Set `g_bad_config` status on errors

## Relationships with Other Global Groups

I/O globals work closely with other subsystems:

- **File loading variables** work with fractal specification variables (`fractype.h`) to restore fractal state
- **Color map variables** work with palette variables in the engine (`VideoInfo.h`)
- **Save filename** works with image region and calculation state variables for complete state saves
- **Special directories** are used throughout the codebase for locating formula files, IFS files, L-system files, etc.
- **Configuration variables** affect nearly all subsystems by loading user preferences
- **Auto-save variables** work with calculation engine to periodically save work in progress

## File Format Dependencies

The I/O subsystem handles multiple file formats:

- **GIF files**: Primary image format for saving/loading fractals with embedded metadata
  - Uses extension blocks (`FractalInfo`, `FormulaInfo`, `OrbitsInfo`, etc. in `gif_extensions.h`)
  - Stores complete fractal state for resume capability
  
- **MAP files**: Color palette files
  - 256 RGB triplets stored in `g_map_clut[]`
  
- **FRM files**: Formula definitions (referenced by parser in `fractals/parser.h`)

- **IFS files**: Iterated Function System definitions (referenced in `fractals/ifs.h`)

- **L files**: L-system definitions (referenced in `fractals/lsystem.h`)

- **PAR files**: Parameter sets for fractals

- **Configuration files**: Video mode and program settings

## Refactoring Considerations

When refactoring to reduce global state in I/O code, consider:

1. **File Reading Context**: Variables in the file loading group could become a `FileReader` or `ImageLoader` class that encapsulates file metadata and loading state.

2. **File Writing Context**: Save-related variables could become a `FileSaver` or `ImageEncoder` class with auto-save timer management.

3. **Color Map Manager**: The color map group could become a `PaletteManager` class that handles loading, saving, and applying color maps.

4. **Path Manager**: Library and directory variables could become a `PathResolver` or `FileLocator` class that encapsulates search logic.

5. **Configuration Manager**: Configuration variables could become a `ConfigLoader` class that validates and applies settings.

6. **GIF Codec**: GIF-specific variables could be encapsulated in `GifDecoder` and `GifEncoder` classes.

The I/O variables are good candidates for encapsulation because:
- They form natural groupings by operation (load, save, search, config)
- File operations have clear begin/end boundaries suitable for RAII patterns
- Moving to objects would enable better error handling and resource management
- Encapsulation would make testing I/O operations much easier

## Cross-Cutting Concerns

Several I/O globals are used throughout the codebase:

- `g_save_filename` is checked in many places to determine the current output file
- `g_read_filename` is used for display in UI and for resume operations
- `g_file_x_dots` and `g_file_y_dots` affect viewport and zoom calculations
- `g_file_aspect_ratio` affects 3D transformations and display
- `g_special_dirs` provides system paths used by all file-dependent features
- `g_working_dir`, `g_save_dir`, and `g_temp_dir` are referenced widely
- `g_map_clut[]` is the active palette used by the rendering engine

These cross-cutting variables would require careful interface design during refactoring. A service locator or dependency injection pattern might be appropriate for path management, while file context variables might be passed explicitly to functions that need them.

## State Machine Patterns

Some I/O operations follow state machine patterns:

### File Display State
- `g_show_file` transitions: REQUEST_IMAGE ? LOAD_IMAGE ? IMAGE_LOADED

### Save Timer State
- `g_timed_save` and `g_resave_flag` transition: NONE ? STARTED ? FINAL

### Configuration Loading State
- `g_bad_config` indicates: OK, BAD_WITH_MESSAGE, or BAD_NO_MESSAGE

These state variables control flow through complex I/O operations and would benefit from explicit state machine implementations or state pattern refactoring.

## Performance Considerations

Some I/O globals are performance-sensitive:

- `g_block[]` is a buffer used during encoding; its size affects encoding performance
- `g_out_line` function pointer enables fast line-by-line GIF decoding without virtual dispatch
- `g_skip_x_dots` and `g_skip_y_dots` enable fast preview loading by skipping pixels
- `g_dither_flag` trades quality for speed in GIF display

When refactoring, these performance-critical variables should remain easily accessible and avoid adding indirection overhead.

## Extension Block Architecture

The GIF file format support includes a sophisticated extension block system defined in `gif_extensions.h`:

- **ExtBlock2**: Resume data (arbitrary length byte vector)
- **ExtBlock3**: Formula information (40-char name + flags)
- **ExtBlock4**: Range data (integer vector)
- **ExtBlock5**: Arbitrary precision math data (char vector)
- **ExtBlock6**: Evolution parameters (genetic algorithm state)
- **ExtBlock7**: Orbit drawing parameters (coordinate ranges + flags)

These structures allow Iterated Dynamics to save complete fractal state in GIF files, enabling:
- Exact resume from any point in calculation
- Sharing of fractal parameters between users
- Evolution of fractal parameters across generations
- Preservation of orbit visualization settings

The extension block system is a key architectural feature that distinguishes Iterated Dynamics files from standard GIF images.
