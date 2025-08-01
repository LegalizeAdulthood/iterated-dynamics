// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadfile.h>

#include "MockDriver.h"
#include "test_migrate_gif.h"

#include <engine/cmdfiles.h>
#include <engine/id_data.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <misc/ValueSaver.h>
#include <ui/history.h>
#include <ui/make_batch_file.h>
#include <ui/video_mode.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>

using namespace testing;
using namespace id::test::migrate_gif;

inline std::ostream &operator<<(std::ostream &str, FractalType value)
{
    return str << +value;
}

void PrintTo(const FractalType value, std::ostream *str)
{
    *str << value;
}

namespace {

struct FileFractalType
{
    const char *filename;
    FractalType type;
};

std::ostream &operator<<(std::ostream &str, const FileFractalType &value)
{
    return str << std::filesystem::path(value.filename).filename().string() << ", " << +value.type;
}

void PrintTo(const FileFractalType &value, std::ostream *str)
{
    *str << value;
}

class TestLoadFile : public TestWithParam<FileFractalType>
{
public:
    ~TestLoadFile() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<ShowFile> saved_show_file{g_show_file};
    ValueSaver<int> saved_init_mode{g_init_mode};
    ValueSaver<bool> saved_loaded_3d{g_loaded_3d};
    ValueSaver<bool> saved_fast_restore{g_fast_restore};
    ValueSaver<bool> saved_view_window{g_view_window};
    ValueSaver<FractalType> saved_fractal_type{g_fractal_type, FractalType::JULIA};
    ValueSaver<FractalSpecific *> saved_fractal_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA)};
    ValueSaver<int> saved_invert{g_invert};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file};
    ValueSaver<int> saved_colors{g_colors};
    ValueSaver<bool> saved_potential_flag{g_potential_flag};
    ValueSaver<int> saved_user_biomorph_value{g_user_biomorph_value};
    ValueSaver<long> saved_user_distance_estimator_value{g_user_distance_estimator_value};
    ValueSaver<long> saved_calc_time{g_calc_time};
    ValueSaver<CalcStatus> saved_calc_status{g_calc_status};
    ValueSaver<bool> saved_potential_16bit{g_potential_16bit};
    ValueSaver<int> saved_save_system{g_save_system};
    ValueSaver<int> saved_file_x_dots{g_file_x_dots};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio};
    ValueSaver<bool> saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded};
    ValueSaver<double> saved_math_tol0{g_math_tol[0]};
    ValueSaver<double> saved_math_tol1{g_math_tol[1]};
    ValueSaver<Version> saved_file_version{g_file_version};
    ValueSaver<bool> saved_ask_video{g_ask_video, false};
    ValueSaver<int> saved_video_table_len{g_video_table_len, 1};
    ValueSaver<int> saved_video_colors{g_video_table[0].colors, 256};
    ValueSaver<int> saved_video_width{g_video_table[0].x_dots, 800};
    ValueSaver<int> saved_video_height{g_video_table[0].y_dots, 600};
    ValueSaver<Driver *> saved_driver{g_driver, &m_driver};
};

void TestLoadFile::SetUp()
{
    EXPECT_CALL(m_driver, check_memory);
    EXPECT_CALL(m_driver, write_palette);
    EXPECT_CALL(m_driver, delay(_));
    history_init();
    save_history_info();
}

void TestLoadFile::TearDown()
{
    restore_history_info(g_history_ptr);
    Test::TearDown();
}

} // namespace

TEST_P(TestLoadFile, integerTypeMigrated)
{
    ValueSaver saved_read_filename{g_read_filename, GetParam().filename};

    const int result{read_overlay()};

    EXPECT_EQ(0, result);
    EXPECT_EQ(GetParam().type, g_fractal_type);
}

static FileFractalType s_int_types[]{
    {ID_TEST_GIF_MANDEL_INT, FractalType::MANDEL},                      //
    {ID_TEST_GIF_JULIA_INT, FractalType::JULIA},                        //
    {ID_TEST_GIF_LAMBDA_INT, FractalType::LAMBDA},                      //
    {ID_TEST_GIF_MANOWAR_INT, FractalType::MAN_O_WAR},                  //
    {ID_TEST_GIF_SIERPINSKI_INT, FractalType::SIERPINSKI},              //
    {ID_TEST_GIF_BARNSLEYM1_INT, FractalType::BARNSLEY_M1},             //
    {ID_TEST_GIF_BARNSLEYJ1_INT, FractalType::BARNSLEY_J1},             //
    {ID_TEST_GIF_BARNSLEYM2_INT, FractalType::BARNSLEY_M2},             //
    {ID_TEST_GIF_BARNSLEYJ2_INT, FractalType::BARNSLEY_J2},             //
    {ID_TEST_GIF_SQRTRIG_INT, FractalType::SQR_FN},                     //
    {ID_TEST_GIF_FNPLUSFN_INT, FractalType::FN_PLUS_FN},                //
    {ID_TEST_GIF_MANDELLAMBDA_INT, FractalType::MANDEL_LAMBDA},         //
    {ID_TEST_GIF_MARKSMANDEL_INT, FractalType::MARKS_MANDEL},           //
    {ID_TEST_GIF_MARKSJULIA_INT, FractalType::MARKS_JULIA},             //
    {ID_TEST_GIF_UNITY_INT, FractalType::UNITY},                        //
    {ID_TEST_GIF_MANDEL4_INT, FractalType::MANDEL4},                    //
    {ID_TEST_GIF_JULIA4_INT, FractalType::JULIA4},                      //
    {ID_TEST_GIF_BARNSLEYM3_INT, FractalType::BARNSLEY_M3},             //
    {ID_TEST_GIF_BARNSLEYJ3_INT, FractalType::BARNSLEY_J3},             //
    {ID_TEST_GIF_TRIGSQR_INT, FractalType::FN_Z_SQR},                   //
    {ID_TEST_GIF_FNTIMESFN_INT, FractalType::FN_TIMES_FN},              //
    {ID_TEST_GIF_SQR1FN_INT, FractalType::SQR_1_OVER_FN},               //
    {ID_TEST_GIF_FNMULZZ_INT, FractalType::FN_MUL_Z_PLUS_Z},            //
    {ID_TEST_GIF_KAM_INT, FractalType::KAM},                            //
    {ID_TEST_GIF_KAM3D_INT, FractalType::KAM_3D},                       //
    {ID_TEST_GIF_LAMBDAFN_INT, FractalType::LAMBDA_FN},                 //
    {ID_TEST_GIF_MANFNZSQRD_INT, FractalType::MANDEL_FN_PLUS_Z_SQRD},   //
    {ID_TEST_GIF_JULFNZSQRD_INT, FractalType::JULIA_FN_PLUS_Z_SQRD},    //
    {ID_TEST_GIF_MANDELFN_INT, FractalType::MANDEL_FN},                 //
    {ID_TEST_GIF_MANZPOWER_INT, FractalType::MANDEL_Z_POWER},           //
    {ID_TEST_GIF_JULZPOWER_INT, FractalType::JULIA_Z_POWER},            //
    {ID_TEST_GIF_MANFNPLUSEXP_INT, FractalType::MANDEL_FN_PLUS_EXP},    //
    {ID_TEST_GIF_JULFNPLUSEXP_INT, FractalType::JULIA_FN_PLUS_EXP},     //
    {ID_TEST_GIF_POPCORN_INT, FractalType::POPCORN},                    //
    {ID_TEST_GIF_LORENZ_INT, FractalType::LORENZ},                      //
    {ID_TEST_GIF_NEWTON_INT, FractalType::NEWTON},                      //
    {ID_TEST_GIF_NEWTBASIN_INT, FractalType::NEWT_BASIN},               //
    {ID_TEST_GIF_LORENZ3D_INT, FractalType::LORENZ_3D},                 //
    {ID_TEST_GIF_FORMULA_INT, FractalType::FORMULA},                    //
    {ID_TEST_GIF_JULIBROT_INT, FractalType::JULIBROT},                  //
    {ID_TEST_GIF_ROSSLER_INT, FractalType::ROSSLER},                    //
    {ID_TEST_GIF_HENON_INT, FractalType::HENON},                        //
    {ID_TEST_GIF_SPIDER_INT, FractalType::SPIDER},                      //
    {ID_TEST_GIF_BIFURCATION_INT, FractalType::BIFURCATION},            //
    {ID_TEST_GIF_BIFLAMBDA_INT, FractalType::BIF_LAMBDA},               //
    {ID_TEST_GIF_POPCORNJULIA_INT, FractalType::POPCORN_JUL},           //
    {ID_TEST_GIF_MANOWARJULIA_INT, FractalType::MAN_O_WAR_J},           //
    {ID_TEST_GIF_FNZFNPIX_INT, FractalType::FN_PLUS_FN_PIX},            //
    {ID_TEST_GIF_MARKSMANDELPOWER_INT, FractalType::MARKS_MANDEL_PWR},  //
    {ID_TEST_GIF_BIFEQSINPI_INT, FractalType::BIF_EQ_SIN_PI},           //
    {ID_TEST_GIF_BIFPLUSSINPI_INT, FractalType::BIF_PLUS_SIN_PI},       //
    {ID_TEST_GIF_BIFSTEWART_INT, FractalType::BIF_STEWART},             //
    {ID_TEST_GIF_LAMBDAFNFN_INT, FractalType::LAMBDA_FN_FN},            //
    {ID_TEST_GIF_JULIAFNFN_INT, FractalType::JUL_FN_FN},                //
    {ID_TEST_GIF_MANLAMFNFN_INT, FractalType::MAN_LAM_FN_FN},           //
    {ID_TEST_GIF_MANDELFNFN_INT, FractalType::MAN_FN_FN},               //
    {ID_TEST_GIF_BIFMAY_INT, FractalType::BIF_MAY},                     //
    {ID_TEST_GIF_HALLEYMP_INT, FractalType::HALLEY},                    //
    {ID_TEST_GIF_JULIAINVERSE_INT, FractalType::INVERSE_JULIA},         //
    {ID_TEST_GIF_PHOENIX_INT, FractalType::PHOENIX},                    //
    {ID_TEST_GIF_MANDPHOENIX_INT, FractalType::MAND_PHOENIX},           //
    {ID_TEST_GIF_FROTHYBASIN_INT, FractalType::FROTHY_BASIN},           //
    {ID_TEST_GIF_PHOENIXCMPLX_INT, FractalType::PHOENIX_CPLX},          //
    {ID_TEST_GIF_MANDPHOENIXCMPLX_INT, FractalType::MAND_PHOENIX_CPLX}, //
};

INSTANTIATE_TEST_SUITE_P(MigrateTypes, TestLoadFile, ValuesIn(s_int_types));
