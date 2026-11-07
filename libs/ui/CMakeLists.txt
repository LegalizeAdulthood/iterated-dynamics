# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
find_package(fmt CONFIG REQUIRED)

set(ID_MAKE_MIG_SCRIPT_SOURCE make_mig_script_unix.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_MAKE_MIG_SCRIPT_SOURCE make_mig_script_windows.cpp)
endif()

set(ID_ORBIT_DELAY_TIMER_SOURCE)
set_property(GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_ORBIT_DELAY_TIMER_SOURCE OrbitDelayTimerWin32.cpp)
    set_property(GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ID_ORBIT_DELAY_TIMER_SOURCE OrbitDelayTimerLinux.cpp)
    set_property(GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(ID_ORBIT_DELAY_TIMER_SOURCE OrbitDelayTimerMac.cpp)
    set_property(GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE TRUE)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(FreeBSD|OpenBSD|NetBSD)$")
    set(ID_ORBIT_DELAY_TIMER_SOURCE OrbitDelayTimerBSD.cpp)
    set_property(GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE TRUE)
endif()

add_library(id-ui
    include/ui/AntController.h AntController.cpp
    include/ui/BifurcationController.h BifurcationController.cpp
    include/ui/big_while_loop.h big_while_loop.cpp
    include/ui/calculate_image.h calculate_image.cpp
    include/ui/CellularController.h CellularController.cpp
    include/ui/ChoiceBuilder.h
    include/ui/command_result.h
    include/ui/command_diagnostics.h command_diagnostics.cpp
    include/ui/command_effects.h command_effects.cpp
    include/ui/CommandRouter.h CommandRouter.cpp
    include/ui/comments.h comments.cpp
    include/ui/DiffusionController.h DiffusionController.cpp
    include/ui/diskvid.h diskvid.cpp
    include/ui/double_to_string.h double_to_string.cpp
    include/ui/DriverSoundOutput.h DriverSoundOutput.cpp
    include/ui/editpal.h editpal.cpp
    include/ui/evolve.h evolve.cpp
    include/ui/evolver_menu_switch.h evolver_menu_switch.cpp
    include/ui/field_prompt.h field_prompt.cpp
    include/ui/file_get_window.h file_get_window.cpp
    include/ui/find_special_colors.h find_special_colors.cpp
    include/ui/flip_image.h flip_image.cpp
    include/ui/FrothyBasinController.h FrothyBasinController.cpp
    include/ui/full_screen_choice.h full_screen_choice.cpp
    include/ui/full_screen_prompt.h full_screen_prompt.cpp
    include/ui/get_3d_params.h get_3d_params.cpp
    include/ui/ThreeDSettings.h ThreeDSettings.cpp
    include/ui/get_a_number.h get_a_number.cpp
    include/ui/get_browse_params.h get_browse_params.cpp
    include/ui/get_calculation_time.h get_calculation_time.cpp
    include/ui/get_cmd_string.h get_cmd_string.cpp
    include/ui/get_commands.h get_commands.cpp
    include/ui/get_corners.h get_corners.cpp
    include/ui/get_file_entry.h get_file_entry.cpp
    include/ui/get_fract_type.h get_fract_type.cpp
    include/ui/get_key_no_help.h get_key_no_help.cpp
    include/ui/get_rds_params.h get_rds_params.cpp
    include/ui/get_sound_params.h get_sound_params.cpp
    include/ui/get_toggles.h get_toggles.cpp
    include/ui/get_toggles2.h get_toggles2.cpp
    include/ui/get_video_mode.h get_video_mode.cpp
    include/ui/get_view_params.h get_view_params.cpp
    include/ui/goodbye.h goodbye.cpp
    include/ui/help.h help.cpp
    include/ui/help_title.h help_title.cpp
    include/ui/history.h history.cpp
    include/ui/id_keys.h
    include/ui/id_main.h id_main.cpp
    include/ui/IFS2DController.h IFS2DController.cpp
    include/ui/ImageRenderContext.h ImageRenderContext.cpp
    include/ui/init_failure.h
    include/ui/intro.h intro.cpp
    include/ui/input_field.h input_field.cpp
    include/ui/KeyboardHandler.h KeyboardHandler.cpp
    include/ui/inverse_julia.h inverse_julia.cpp
    include/ui/InverseJuliaFractalController.h InverseJuliaFractalController.cpp
    include/ui/LSystemController.h LSystemController.cpp
    include/ui/load_config.h load_config.cpp
    include/ui/LyapunovController.h LyapunovController.cpp
    include/ui/make_batch_file.h make_batch_file.cpp
    include/ui/main_menu.h main_menu.cpp
    include/ui/main_menu_switch.h main_menu_switch.cpp
    include/ui/make_mig_script.h ${ID_MAKE_MIG_SCRIPT_SOURCE}
    include/ui/main_state.h
    include/ui/menu_handler.h menu_handler.cpp
    include/ui/mouse.h mouse.cpp
    include/ui/not_disk_msg.h not_disk_msg.cpp
    include/ui/OrbitController.h OrbitController.cpp
    include/ui/OrbitDelayTimer.h ${ID_ORBIT_DELAY_TIMER_SOURCE}
    include/ui/passes_options.h passes_options.cpp
    include/ui/PlasmaController.h PlasmaController.cpp
    include/ui/PopcornController.h PopcornController.cpp
    include/ui/precision_diagnostics.h precision_diagnostics.cpp
    include/ui/put_string_center.h put_string_center.cpp
    include/ui/read_ticker.h read_ticker.cpp
    include/ui/restore_image.h restore_image.cpp
    include/ui/rotate.h rotate.cpp
    include/ui/SaveImageProgress.h SaveImageProgress.cpp
    include/ui/save_image.h save_image.cpp
    include/ui/save_timer.h save_timer.cpp
    include/ui/select_video_mode.h select_video_mode.cpp
    include/ui/set_default_params.h set_default_params.cpp
    include/ui/shell_sort.h shell_sort.cpp
    include/ui/ShowDotController.h ShowDotController.cpp
    include/ui/slideshw.h slideshw.cpp
    include/ui/SoundSession.h SoundSession.cpp
    include/ui/StereoController.h StereoController.cpp
    include/ui/StandardFractalController.h StandardFractalController.cpp
    include/ui/standard_orbit_plot.h standard_orbit_plot.cpp
    include/ui/Standard4DController.h Standard4DController.cpp
    include/ui/starfield.h starfield.cpp
    include/ui/stereo.h stereo.cpp
    include/ui/stop_msg.h stop_msg.cpp
    include/ui/tab_display.h tab_display.cpp
    include/ui/temp_msg.h temp_msg.cpp
    include/ui/TestPointController.h TestPointController.cpp
    include/ui/text_screen.h text_screen.cpp
    include/ui/thinking.h thinking.cpp
    include/ui/video.h video.cpp
    include/ui/video_mode.h video_mode.cpp
    include/ui/zoom.h zoom.cpp
)
add_library(id::ui ALIAS id-ui)
source_group("Headers" REGULAR_EXPRESSION ".*/include/ui/.*")
target_include_directories(id-ui PUBLIC include)
target_link_libraries(id-ui PUBLIC id::core PRIVATE helpcom fmt::fmt-header-only)
target_folder(id-ui "Libraries")
add_dependencies(id-ui native-help)
