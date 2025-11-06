# Global Variables in libid/include/ui

This document identifies groups of global variables in the `libid/include/ui` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in the user interface, input handling, and file management pipeline.

## 1. Text Screen/Cursor Group

**File:** `text_screen.h`

Variables controlling text mode cursor position:

- `g_text_col` - current column in text mode
- `g_text_row` - current row in text mode
- `g_text_col_base` - g_text_col is relative to this base
- `g_text_row_base` - g_text_row is relative to this base

## 2. Mouse Input Group

**File:** `mouse.h`

Variables controlling mouse behavior and tracking:

- `g_look_at_mouse` - mouse handling mode (IGNORE_MOUSE, NAVIGATE_GRAPHICS, NAVIGATE_TEXT, POSITION)
- `g_cursor_mouse_tracking` - cursor follows mouse movement

## 3. Tab Display Group

**File:** `tab_display.h`

Variables for tab display feature:

- `g_tab_enabled` - tab display feature enabled/disabled

## 4. Help System Group

**File:** `help.h` (in help namespace)

Variables for online help system:

- `g_help_mode` - current help context/label

## 5. Zoom Box Group

**File:** `zoom.h`

Variables controlling zoom box display and manipulation:

- `g_box_x[]` - array of x coordinates for box
- `g_box_y[]` - array of y coordinates for box
- `g_box_values[]` - array of values associated with box points
- `g_box_color` - color of zoom box
- `g_box_count` - number of points in box
- `g_zoom_box_x` - zoom box x position
- `g_zoom_box_y` - zoom box y position
- `g_zoom_box_width` - zoom box width
- `g_zoom_box_height` - zoom box height
- `g_zoom_box_skew` - zoom box skew angle
- `g_zoom_box_rotation` - zoom box rotation angle
- `g_zoom_enabled` - zoom box feature enabled

## 6. Video/Display Output Group

**File:** `video.h`

Variables for video output and line rendering:

- `g_row_count` - row counter for decoder and out_line
- `g_video_start_x` - video start x coordinate
- `g_video_start_y` - video start y coordinate
- `g_vesa_x_res` - VESA x resolution
- `g_vesa_y_res` - VESA y resolution

## 7. Disk Video Group

**File:** `diskvid.h`

Variables for disk-based video (virtual screen in file):

- `g_disk_flag` - disk video is active
- `g_disk_16_bit` - using 16-bit disk video mode
- `g_disk_targa` - disk video is in Targa format
- `g_good_mode` - video mode is valid

## 8. Slideshow Mode Group

**File:** `slideshw.h`

Variables controlling slideshow/demo mode:

- `g_slides` - slideshow mode (OFF, PLAY, RECORD)
- `g_auto_name` - auto-generated filename path
- `g_busy` - busy flag for slideshow operations

## 9. Evolution/Genetic Algorithm Group

**File:** `evolve.h`

Variables for evolutionary parameter exploration:

- `g_evolving` - evolution mode flags (NONE, FIELD_MAP, RAND_WALK, RAND_PARAM, NO_GROUT, PARAM_BOX)
- `g_evolve_image_grid_size` - size of evolution image grid
- `g_evolve_this_generation_random_seed` - random seed for current generation
- `g_evolve_max_random_mutation` - maximum random mutation amount
- `g_evolve_mutation_reduction_factor` - factor to reduce mutation over time
- `g_evolve_x_parameter_range` - range of x parameter variation
- `g_evolve_y_parameter_range` - range of y parameter variation
- `g_evolve_x_parameter_offset` - x parameter offset
- `g_evolve_y_parameter_offset` - y parameter offset
- `g_evolve_new_x_parameter_offset` - new x parameter offset
- `g_evolve_new_y_parameter_offset` - new y parameter offset
- `g_evolve_discrete_x_parameter_offset` - discrete x offset
- `g_evolve_discrete_y_parameter_offset` - discrete y offset
- `g_evolve_new_discrete_x_parameter_offset` - new discrete x offset
- `g_evolve_new_discrete_y_parameter_offset` - new discrete y offset
- `g_evolve_param_grid_x` - parameter grid x coordinate
- `g_evolve_param_grid_y` - parameter grid y coordinate
- `g_evolve_param_box_count` - parameter box count
- `g_evolve_param_zoom` - parameter zoom level
- `g_evolve_dist_per_x` - distance per x unit
- `g_evolve_dist_per_y` - distance per y unit
- `g_gene_bank[NUM_GENES]` - array of genetic parameters

## 10. Display Speed Group

**File:** `intro.h`

Variables controlling display timing:

- `g_slow_display` - slow down display for visual effect

## Usage Patterns

These variable groups follow typical UI interaction patterns:

1. **Input Handling**: Mouse and keyboard variables track user input state
2. **Text Display**: Text screen variables manage text mode cursor positioning
3. **Interactive Features**: Zoom box and evolution variables support interactive parameter exploration
4. **File Operations**: Disk video and slideshow variables handle file-based operations
5. **Visual Feedback**: Display speed and help system variables enhance user experience

## Relationships with Engine Globals

Many UI globals work in conjunction with engine globals:

- **Zoom box variables** (`zoom.h`) work with image region variables (`ImageRegion.h` in engine)
- **Video output variables** (`video.h`) work with screen dimension variables (`VideoInfo.h` in engine)
- **Evolution variables** (`evolve.h`) modify fractal parameter variables (`calcfrac.h` in engine)
- **Help system** references many engine and fractal-specific globals for context-sensitive help

## Refactoring Considerations

When refactoring to reduce global state in UI code, consider:

1. **Text Screen** variables could become a `TextCursor` class
2. **Mouse Input** variables could become a `MouseState` class
3. **Zoom Box** variables could become a `ZoomBox` class with methods for manipulation
4. **Evolution** variables could become an `EvolutionEngine` class
5. **Slideshow** variables could become a `SlideshowController` class
6. **Video Output** variables could be part of a `VideoContext` or `DisplayBuffer` class
7. **Disk Video** variables could become a `VirtualScreen` or `DiskVideoDevice` class

The evolution system in particular has many related variables that form a natural candidate for encapsulation into a dedicated class that manages the genetic algorithm state and operations.

## Cross-Cutting Concerns

Some UI globals are used throughout both UI and engine code:

- `g_tab_enabled` is checked in many places to enable/disable tab key functionality
- `g_look_at_mouse` is used in input loops throughout the codebase
- `g_help_mode` determines context for F1 help across all menu screens
- Zoom box variables are used in both UI interaction and fractal calculation setup

These cross-cutting variables may require careful interface design during refactoring to maintain functionality while reducing coupling.
