// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadfile.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/id_data.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <misc/ValueSaver.h>
#include <ui/cmdfiles.h>
#include <ui/history.h>
#include <ui/make_batch_file.h>
#include <ui/video_mode.h>

#include <gtest/gtest.h>

using namespace testing;

TEST(TestLoadFile, migrateIntegerType)
{
    ValueSaver saved_show_file{g_show_file};
    ValueSaver saved_init_mode{g_init_mode};
    ValueSaver saved_loaded_3d{g_loaded_3d};
    ValueSaver saved_fast_restore{g_fast_restore};
    ValueSaver saved_view_window{g_view_window};
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::JULIA};
    ValueSaver saved_fractal_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA)};
    ValueSaver saved_invert{g_invert};
    ValueSaver saved_make_parameter_file{g_make_parameter_file};
    ValueSaver saved_colors{g_colors};
    ValueSaver saved_potential_flag{g_potential_flag};
    ValueSaver saved_user_biomorph_value{g_user_biomorph_value};
    ValueSaver saved_user_distance_estimator_value{g_user_distance_estimator_value};
    ValueSaver saved_calc_time{g_calc_time};
    ValueSaver saved_calc_status{g_calc_status};
    ValueSaver saved_potential_16bit{g_potential_16bit};
    ValueSaver saved_save_system{g_save_system};
    ValueSaver saved_file_x_dots{g_file_x_dots};
    ValueSaver saved_file_y_dots{g_file_y_dots};
    ValueSaver saved_file_aspect_ratio{g_file_aspect_ratio};
    ValueSaver saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded};
    ValueSaver saved_math_tol0{g_math_tol[0]};
    ValueSaver saved_math_tol1{g_math_tol[1]};
    ValueSaver saved_file_version{g_file_version};
    ValueSaver saved_read_filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver saved_ask_video{g_ask_video, false};
    ValueSaver saved_video_table_len{g_video_table_len, 1};
    ValueSaver saved_video_colors{g_video_table[0].colors, 256};
    ValueSaver saved_video_width{g_video_table[0].x_dots, 800};
    ValueSaver saved_video_height{g_video_table[0].y_dots, 600};
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    EXPECT_CALL(driver, check_memory);
    EXPECT_CALL(driver, write_palette);
    EXPECT_CALL(driver, delay(_));
    history_init();
    save_history_info();

    const int result{read_overlay()};

    EXPECT_EQ(0, result);
    EXPECT_EQ(FractalType::MANDEL_FP, g_fractal_type);

    restore_history_info(g_history_ptr);
}
