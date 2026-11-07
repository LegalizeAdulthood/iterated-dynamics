# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
add_executable(test-ui
    include/ui/NonStandardFractalTest.h
    include/ui/TestRender.h TestRender.cpp
    init_failure.cpp
    test_AntController.cpp
    test_BifurcationController.cpp
    test_ChoiceBuilder.cpp
    test_cellular.cpp
    test_command_diagnostics.cpp
    test_command_effects.cpp
    test_CommandRouter.cpp
    test_comments.cpp
    test_diskvid.cpp
    test_DiffusionController.cpp
    test_double_to_string.cpp
    test_DriverSoundOutput.cpp
    test_DynamicSound.cpp
    test_file_item.cpp
    test_full_screen_prompt.cpp
    test_history.cpp
    test_id_keys.cpp
    test_input_field.cpp
    test_inverse_julia.cpp
    test_IFS2DController.cpp
    test_KeyboardHandler.cpp
    test_lsystem.cpp
    test_load_config.cpp
    test_config_keys.h.in
    test_main_menu_switch.cpp
    test_make_batch_file.cpp
    test_Orbit2DSound.cpp
    test_Orbit3DSound.cpp
    test_OverlaySound.cpp
    test_RestoredImageSound.cpp
    test_reset_command.cpp
    test_passes_options.cpp
    test_plasma.cpp
    test_PopcornController.cpp
    test_PotentialRestore.cpp
    test_precision_diagnostics.cpp
    test_restore_image.cpp
    test_rotate.cpp
    test_save_image.cpp
    test_save_timer.cpp
    test_ShowDotController.cpp
    test_slideshw.cpp
    test_SoundSession.cpp
    test_StickyOrbitSound.cpp
    test_stereo.cpp
    test_Standard4DController.cpp
    test_StereoRestore.cpp
    test_tab_display.cpp
    test_thinking.cpp
    test_TestPointController.cpp
    test_video_mode.cpp
    test_zoom.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/include/ui/test_config_keys.h"
)
get_property(ID_ORBIT_DELAY_TIMER_AVAILABLE GLOBAL PROPERTY ID_ORBIT_DELAY_TIMER_AVAILABLE)
if(ID_ORBIT_DELAY_TIMER_AVAILABLE)
    target_sources(test-ui PRIVATE test_OrbitDelayTimer.cpp)
endif()
source_group("Headers" REGULAR_EXPRESSION ".*/include/ui/.*")
source_group("CMake Templates" REGULAR_EXPRESSION ".*\\.in$")
target_include_directories(test-ui PRIVATE
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(test-ui PRIVATE id::ui id-test-support GTest::gmock_main)
target_folder(test-ui "Tests")
gtest_discover_tests(test-ui PROPERTIES LABELS id)
