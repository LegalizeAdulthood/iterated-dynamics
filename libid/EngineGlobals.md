# Global Variables in libid/include/engine

This document identifies groups of global variables in the `libid/include/engine` directory that are typically used together. These groupings represent variables that are conceptually related and often accessed together in the fractal calculation and rendering pipeline.

## 1. Screen/Display Dimensions Group

**Files:** `VideoInfo.h`, `LogicalScreen.h`

Related global variables for screen and display configuration:

- `g_adapter` - index into g_video_table[]
- `g_init_mode` - initial video mode
- `g_colors` - maximum colors available
- `g_screen_x_dots` - number of dots on the physical screen (horizontal)
- `g_screen_y_dots` - number of dots on the physical screen (vertical)
- `g_video_entry` - current video mode information
- `g_video_table[]` - array of available video modes
- `g_video_table_len` - length of video table
- `g_logical_screen.x_dots` - logical screen horizontal dots
- `g_logical_screen.y_dots` - logical screen vertical dots
- `g_logical_screen.x_offset` - physical top left x offset
- `g_logical_screen.y_offset` - physical top left y offset
- `g_logical_screen.x_size_dots` - xdots-1
- `g_logical_screen.y_size_dots` - ydots-1

## 2. Viewport/View Window Group

**File:** `Viewport.h`

Variables controlling the viewport and windowing behavior:

- `g_viewport.enabled` - false for full screen, true for window
- `g_viewport.keep_aspect_ratio` - true to keep virtual aspect
- `g_viewport.crop` - true to crop default coordinates
- `g_viewport.z_scroll` - screen/zoom box fixed (false) or relaxed (true)
- `g_viewport.final_aspect_ratio` - for view shape and rotation
- `g_viewport.reduction` - window auto-sizing
- `g_viewport.x_dots` - explicit view sizing (horizontal)
- `g_viewport.y_dots` - explicit view sizing (vertical)

## 3. Image Coordinate/Region Group

**Files:** `ImageRegion.h`, `calc_frac_init.h`

Variables defining the mathematical coordinates of the fractal region:

- `g_image_region.m_min` - minimum corner (top left mathematically)
- `g_image_region.m_max` - maximum corner (bottom right)
- `g_image_region.m_3rd` - third corner (for rotation)
- `g_save_image_region` - saved image region for restoration
- `g_delta_x` - horizontal pixel increment
- `g_delta_y` - vertical pixel increment
- `g_delta_x2` - horizontal pixel increment (alternative)
- `g_delta_y2` - vertical pixel increment (alternative)
- `g_plot_mx1` - real->screen multiplier 1
- `g_plot_mx2` - real->screen multiplier 2
- `g_plot_my1` - imaginary->screen multiplier 1
- `g_plot_my2` - imaginary->screen multiplier 2
- `g_aspect_drift` - allowed aspect ratio drift
- `g_screen_aspect` - aspect ratio of the screen
- `g_delta_min` - minimum delta value
- `g_math_tol[2]` - math transition tolerance from double to bignum

## 4. Calculation State Group

**File:** `calcfrac.h`

Variables tracking the overall calculation state and progress:

- `g_calc_status` - calculation status (NO_FRACTAL, PARAMS_CHANGED, IN_PROGRESS, RESUMABLE, NON_RESUMABLE, COMPLETED)
- `g_calc_time` - time spent calculating (in clock ticks)
- `g_calc_type` - function pointer to calculation routine
- `g_std_calc_mode` - current calculation mode
- `g_old_std_calc_mode` - previous calculation mode
- `g_passes` - rendering pass mode (NONE, SEQUENTIAL_SCAN, SOLID_GUESS, BOUNDARY_TRACE, etc.)
- `g_current_pass` - current pass number
- `g_total_passes` - total number of passes
- `g_quick_calc` - quick calculation mode flag
- `g_three_pass` - three pass mode flag

## 5. Iteration/Bailout Group

**File:** `calcfrac.h`

Variables controlling iteration limits and bailout conditions:

- `g_max_iterations` - maximum iteration count to try
- `g_color_iter` - current iteration for coloring
- `g_old_color_iter` - previous iteration value
- `g_real_color_iter` - real iteration value
- `g_first_saved_and` - first saved AND value
- `g_magnitude` - current magnitude value
- `g_magnitude_calc` - magnitude calculation flag
- `g_magnitude_limit` - magnitude limit for bailout
- `g_magnitude_limit2` - squared magnitude limit
- `g_periodicity_check` - periodicity checking enabled
- `g_periodicity_next_saved_incr` - next periodicity save increment
- `g_use_old_periodicity` - use old periodicity algorithm
- `g_reset_periodicity` - reset periodicity flag

## 6. Bailout Formula Group

**File:** `bailout_formula.h`

Variables defining bailout test methods:

- `g_bailout_test` - bailout test type (MOD, REAL, IMAG, OR, AND, MANH, MANR)
- `g_bailout_float` - function pointer for float bailout test
- `g_bailout_bignum` - function pointer for bignum bailout test
- `g_bailout_bigfloat` - function pointer for bigfloat bailout test

## 7. Complex Number Calculation Group

**Files:** `calcfrac.h`, `fractals.h`

Variables holding complex number values during iteration:

- `g_init` - initial complex value
- `g_old_z` - previous z value
- `g_new_z` - new z value
- `g_tmp_z` - temporary z value
- `g_param_z1` - parameter complex value 1
- `g_param_z2` - parameter complex value 2
- `g_power_z` - power for calculations
- `g_temp_sqr_x` - temporary squared x value
- `g_temp_sqr_y` - temporary squared y value
- `g_sin_x` - sine of x
- `g_cos_x` - cosine of x
- `g_float_param` - floating point parameter pointer

## 8. Orbit Display Group

**Files:** `calcfrac.h`, `orbit.h`

Variables controlling orbit visualization:

- `g_init_orbit` - initial orbit value
- `g_use_init_orbit` - init orbit mode (NORMAL, VALUE, PIXEL)
- `g_show_orbit` - flag to turn orbit display on/off
- `g_start_show_orbit` - show orbits at start of fractal
- `g_orbit_color` - XOR color for orbit display
- `g_orbit_save_index` - index into save_orbit array
- `g_orbit_delay` - microsecond orbit delay

## 9. Color/Palette Group

**Files:** `spindac.h`, `color_state.h`, `text_color.h`

Variables managing color palettes and colormap state:

- `g_dac_box[256][3]` - current color palette (RGB values)
- `g_old_dac_box[256][3]` - previous palette for restoration
- `g_got_real_dac` - flag indicating DAC was successfully loaded
- `g_dac_count` - number of DAC entries
- `g_color_state` - color map state (DEFAULT_MAP, UNKNOWN_MAP, MAP_FILE)
- `g_color_cycle_range_lo` - lower bound of cycling color range
- `g_color_cycle_range_hi` - upper bound of cycling color range
- `g_cycle_limit` - color-rotator upper limit
- `g_init_cycle_limit` - initial cycle limit
- `g_map_name` - name of loaded colormap file
- `g_map_set` - flag indicating colormap was set
- `g_text_color[31]` - UI text colors for various elements

## 10. True Color Group

**File:** `spindac.h`

Variables for true color (24-bit) rendering:

- `g_is_true_color` - true color mode is active
- `g_true_color` - escape time true color flag
- `g_true_mode` - true color coloring scheme (DEFAULT_COLOR, ITERATE)

## 11. Inside/Outside Coloring Group

**File:** `calcfrac.h`

Variables controlling coloring methods for points:

- `g_inside_color` - color method for inside points
- `g_outside_color` - color method for outside points
- `g_fill_color` - fill color (-1 = normal)
- `g_atan_colors` - atan coloring parameter
- `g_and_color` - AND mask for iteration to get color index
- `g_decomp[2]` - decomposition coloring array

## 12. Work List/Progress Group

**Files:** `work_list.h`, `calcfrac.h`

Variables managing the calculation work queue and current position:

- `g_num_work_list` - number of work list entries
- `g_work_list[MAX_CALC_WORK]` - array of work list entries
- `g_start_pt` - current work list entry start point
- `g_stop_pt` - current work list entry stop point
- `g_i_start_pt` - integer start point
- `g_i_stop_pt` - integer stop point
- `g_begin_pt` - begin point within window
- `g_work_pass` - pass number for current work item
- `g_work_symmetry` - symmetry for current work item
- `g_current_row` - current pixel row
- `g_current_column` - current pixel column
- `g_row` - row iterator
- `g_col` - column iterator
- `g_color` - current color value

## 13. Symmetry/Plotting Group

**File:** `calcfrac.h`

Variables controlling symmetry and pixel plotting:

- `g_symmetry` - current symmetry type
- `g_force_symmetry` - forced symmetry override
- `g_plot` - function pointer for plotting pixels
- `g_put_color` - function pointer for putting color
- `g_pi_in_pixels` - PI value in pixels for symmetry calculations

## 14. Fractal Parameters Group

**Files:** `calcfrac.h`, `fractals.h`

Variables holding fractal-specific parameters:

- `g_params[MAX_PARAMS]` - array of 10 fractal parameters
- `g_basin` - basin calculation parameter
- `g_degree` - polynomial degree
- `g_c_exponent` - exponent value
- `g_quaternion_c` - quaternion parameter (real)
- `g_quaternion_ci` - quaternion parameter (i)
- `g_quaternion_cj` - quaternion parameter (j)
- `g_quaternion_ck` - quaternion parameter (k)
- `g_biomorph` - biomorph flag
- `g_bof_match_book_images` - use normal BOF initialization
- `g_fudge_half` - fudge factor

## 15. Attractor Group

**File:** `calcfrac.h`

Variables for finite attractor logic:

- `g_attractor.enabled` - finite attractor logic enabled
- `g_attractor.count` - number of finite attractors
- `g_attractor.radius` - finite attractor radius
- `g_attractor.z[MAX_NUM_ATTRACTORS]` - finite attractor values
- `g_attractor.period[MAX_NUM_ATTRACTORS]` - period of each attractor

## 16. Distance Estimator Group

**File:** `calc_frac_init.h`

Variables for distance estimator calculations:

- `g_distance_estimator` - distance estimator value
- `g_distance_estimator_width_factor` - width factor for DE
- `g_distance_estimator_x_dots` - x dots for video independence
- `g_distance_estimator_y_dots` - y dots for video independence
- `g_use_old_distance_estimator` - use old DE algorithm

## 17. Potential Group

**File:** `Potential.h`

Variables for continuous potential calculations:

- `g_potential.flag` - continuous potential enabled
- `g_potential.params[3]` - three potential parameters
- `g_potential.store_16bit` - store 16-bit continuous potential values

## 18. Inversion Group

**File:** `Inversion.h`

Variables for circle inversion:

- `g_inversion.invert` - inversion flag/type
- `g_inversion.radius` - inversion circle radius
- `g_inversion.center` - inversion center point (complex)
- `g_inversion.params[3]` - inversion parameters (radius, x center, y center)

## 19. Resume/Save State Group

**File:** `resume.h`

Variables for saving and resuming calculations:

- `g_resume_data` - vector of resume data bytes
- `g_resuming` - flag indicating if resuming a calculation
- `g_resume_len` - length of resume data

## 20. Log Map Group

**File:** `log_map.h`

Variables for logarithmic color mapping:

- `g_log_map_auto_calculate` - automatically calculate log table
- `g_log_map_flag` - log map enabled
- `g_log_map_fly_calculate` - calculate on-the-fly vs use table
- `g_log_map_calculate` - calculate log map flag
- `g_log_map_table` - vector of log map table values
- `g_log_map_table_max_size` - maximum size of log table

## 21. Sound Group

**File:** `sound.h`

Variables for sound output during calculation:

- `g_sound_flag` - sound control flags (OFF, BEEP, X, Y, Z, ORBIT, SPEAKER, OPL3_FM, MIDI, QUANTIZED)
- `g_base_hertz` - base frequency in Hertz
- `g_fm_attack` - FM attack time
- `g_fm_decay` - FM decay time
- `g_fm_sustain` - FM sustain level
- `g_fm_release` - FM release time
- `g_fm_wave_type` - FM waveform type
- `g_fm_volume` - volume of OPL-3 sound output
- `g_polyphony` - polyphony setting
- `g_scale_map[12]` - musical scale mapping array
- `g_hi_attenuation` - high frequency attenuation

## 22. Command File/Batch Group

**File:** `cmdfiles.h`

Variables for command-line and parameter file processing:

- `g_parameter_file` - path to parameter file
- `g_parameter_set_name` - name of parameter set
- `g_escape_exit` - escape key exits program
- `g_first_init` - first initialization flag
- `g_image_filename_mask` - filename mask for images
- `g_init_batch` - batch mode initialization
- `g_read_color` - read color from parameter file
- `g_record_colors` - record colors mode

## 23. User Settings Group

**File:** `UserData.h`

Variables storing user preferences vs actual values:

- `g_user.biomorph_value` - user's biomorph setting
- `g_user.distance_estimator_value` - user's distance estimator setting
- `g_user.periodicity_value` - user's periodicity setting
- `g_user.std_calc_mode` - user's calculation mode preference
- `g_user.bailout_value` - user's bailout value

## 24. BigNum/BigFloat Math Group

**File:** `big.h` (in math directory, referenced by engine)

Variables for arbitrary precision mathematics:

- `g_bf_math` - bignum/bigfloat mode (NONE, BIG_NUM, BIG_FLT)
- `g_bn_step` - bignum step size
- `g_int_length` - bytes for integer part
- `g_bn_length` - bignum length in bytes
- `g_r_length` - working bignum length
- `g_padding` - padding bytes
- `g_decimals` - decimal places
- `g_shift_factor` - shift factor for fixed point
- `g_bf_length` - bigfloat length
- `g_r_bf_length` - working bigfloat length
- `g_bf_decimals` - bigfloat decimal places
- `g_bn_tmp1` through `g_bn_tmp6` - temporary bignum variables
- `g_bf_tmp1` through `g_bf_tmp5` - temporary bigfloat variables
- `g_bn_tmp_copy1`, `g_bn_tmp_copy2` - bignum copy buffers
- `g_bn_pi` - bignum value of PI
- `g_bn_tmp` - general temporary bignum

## 25. Julia/JIIM Group

**File:** `jiim.h`

Variables for Julia set and inverse iteration:

- `g_has_inverse` - inverse iteration is available
- `g_julia_c` - Julia set constant (complex)
- `g_save_c` - saved Julia constant

## 26. Diffusion Scan Group

**File:** `diffusion_scan.h`

Variables for diffusion-based scanning algorithm:

- `g_diffusion_bits` - number of bits for diffusion
- `g_diffusion_counter` - current diffusion counter
- `g_diffusion_limit` - diffusion scan limit

## 27. SOI (Simultaneous Orbit Iteration) Group

**File:** `soi.h`

Variables for SOI rendering algorithm:

- `g_max_rhombus_depth` - maximum rhombus subdivision depth
- `g_rhombus_stack[]` - rhombus subdivision stack
- `g_soi_min_stack` - minimum stack value
- `g_soi_min_stack_available` - minimum stack space available

## 28. Browse Mode Group

**File:** `Browse.h`

Variables for image browsing mode:

- `g_browse.auto_browse` - automatic browsing enabled
- `g_browse.browsing` - currently in browse mode
- `g_browse.check_fractal_params` - check parameters when browsing
- `g_browse.check_fractal_type` - check fractal type when browsing
- `g_browse.sub_images` - browse sub-images
- `g_browse.confirm_delete` - confirm before deleting
- `g_browse.smallest_box` - smallest selection box size
- `g_browse.smallest_window` - smallest window size
- `g_browse.mask` - filename mask for browsing
- `g_browse.name` - current browse file name
- `g_browse.stack` - stack of filenames while browsing

## 29. Timer Group

**File:** `engine_timer.h`

Variables for timing calculations:

- `g_timer_flag` - timer is enabled
- `g_timer_interval` - timer interval in milliseconds
- `g_engine_timer_start` - engine timer start time

## 30. Random Seed Group

**File:** `random_seed.h`

Variables for random number generation:

- `g_random_seed` - random number seed value
- `g_random_seed_flag` - random seed has been set

## 31. Show Dot Group

**File:** `show_dot.h`

Variables for showing calculation progress with dots:

- `g_auto_show_dot` - auto show dot mode (NONE, AUTOMATIC, DARK, MEDIUM, BRIGHT)
- `g_show_dot` - show dot counter/flag
- `g_size_dot` - size of dot to show

## 32. Drawing Mode Group

**File:** `sticky_orbits.h`

Variables for orbit drawing methods:

- `g_draw_mode` - orbit drawing mode (NONE, RECTANGLE, LINE, FUNCTION)

## 33. Solid Guessing Group

**File:** `solid_guess.h`

Variables for solid guessing optimization:

- `g_stop_pass` - stop at this guessing pass (early termination)

## 34. Keyboard Check Group

**File:** `calcfrac.h`

Variables controlling keyboard polling frequency:

- `g_keyboard_check_interval` - current keyboard check interval
- `g_max_keyboard_check_interval` - maximum keyboard check interval

## 35. Proximity/Convergence Group

**File:** `calcfrac.h`

Variables for convergence testing:

- `g_close_enough` - closeness threshold for convergence
- `g_close_proximity` - proximity threshold

## 36. Max Color Group

**File:** `fractals.h`

Variables related to color limits:

- `g_max_color` - maximum color value
- `g_iteration_ranges` - vector of iteration->color range mappings

## Usage Patterns

These variable groups often follow these usage patterns:

1. **Initialization Phase**: Screen dimensions, viewport, and image region variables are set up first.

2. **Calculation Phase**: Iteration/bailout, complex number calculation, and work list variables are actively used.

3. **Rendering Phase**: Plotting, symmetry, color/palette, and inside/outside coloring variables control output.

4. **State Management**: Resume, user settings, and calculation state variables maintain program state.

5. **Special Effects**: Sound, orbit display, and show dot variables add user feedback.

## Refactoring Considerations

When refactoring to reduce global state, consider:

1. **Screen/Display** variables could become a `DisplayContext` class
2. **Image Region/Coordinates** variables could become a `FractalRegion` class
3. **Calculation State** variables could become a `CalculationEngine` class
4. **Color/Palette** variables could become a `ColorManager` class
5. **Work List** variables could become a `WorkQueue` class
6. **Fractal Parameters** could be grouped into a `FractalParameters` structure
7. **User Settings** are already partially grouped in `UserData` struct

Many of these groupings suggest natural object-oriented encapsulation boundaries that would improve code organization and reduce coupling.
