#include "history.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "fractalp.h"
#include "fractype.h"
#include "id_data.h"
#include "jb.h"
#include "line3d.h"
#include "lorenz.h"
#include "merge_path_names.h"
#include "parser.h"
#include "plot3d.h"
#include "resume.h"
#include "rotate.h"
#include "spindac.h"
#include "sticky_orbits.h"
#include "trig_fns.h"
#include "version.h"

#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace
{

struct HISTORY
{
    fractal_type image_fractal_type;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double creal;
    double cimag;
    double potential_params[3];
    int random_seed;
    bool random_seed_flag;
    int biomorph;
    int inside_color;
    long log_map_flag;
    double inversion[3];
    int decomp;
    symmetry_type force_symmetry;
    int init_3d[16];
    int preview_factor;
    int adjust_3d_x;
    int adjust_3d_y;
    int red_crop_left;
    int red_crop_right;
    int blue_crop_left;
    int blue_crop_right;
    int red_bright;
    int blue_bright;
    int converge_x_adjust;
    int eye_separation;
    int glasses_type;
    int outside_color;
    double x_3rd;
    double y_3rd;
    long distest;
    trig_fn trig_index[4];
    bool finite_attractor;
    double initorbit[2];
    int periodicity_check;
    bool disk_16_bit;
    int release;
    int save_release;
    display_3d_modes display_3d;
    int transparent_color_3d[2];
    int ambient;
    int haze;
    int randomize_3d;
    int color_cycle_range_lo;
    int color_cycle_range_hi;
    int distance_estimator_width_factor;
    double dparm3;
    double dparm4;
    int fill_color;
    double julibrot_x_max;
    double julibrot_x_min;
    double julibrot_y_max;
    double julibrot_y_min;
    int julibrot_z_dots;
    float julibrot_origin_fp;
    float julibrot_depth_fp;
    float julibrot_height_fp;
    float julibrot_width_fp;
    float julibrot_dist_fp;
    float eyes_fp;
    fractal_type new_orbit_type;
    julibrot_3d_mode julibrot_mode;
    Major major_method;
    Minor inverse_julia_minor_method;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
    long bailout;
    bailouts bail_out_test;
    long iterations;
    int converge_y_adjust;
    bool old_demm_colors;
    std::string filename;
    std::string file_item_name;
    unsigned char dac_box[256][3];
    char  max_function;
    char user_std_calc_mode;
    bool three_pass;
    init_orbit_mode use_init_orbit;
    int log_map_fly_calculate;
    int stop_pass;
    bool is_mandelbrot;
    double close_proximity;
    bool bof_match_book_images;
    int orbit_delay;
    long orbit_interval;
    double orbit_corner_min_x;
    double orbit_corner_max_x;
    double orbit_corner_min_y;
    double orbit_corner_max_y;
    double orbit_corner_3_x;
    double orbit_corner_3_y;
    bool keep_screen_coords;
    char draw_mode;
};

} // namespace

int historyptr = -1;      // user pointer into history tbl
int saveptr = 0;          // save ptr into history tbl
bool historyflag = false; // are we backing off in history?

static std::vector<HISTORY> s_history;
int g_max_image_history = 10;

void history_init()
{
    s_history.resize(g_max_image_history);
}

void save_history_info()
{
    if (g_max_image_history <= 0 || g_bf_math != bf_math_type::NONE)
    {
        return;
    }
    HISTORY last = s_history[saveptr];

    HISTORY current{};
    current.image_fractal_type = g_fractal_type;
    current.x_min = g_x_min;
    current.x_max = g_x_max;
    current.y_min = g_y_min;
    current.y_max = g_y_max;
    current.creal = g_params[0];
    current.cimag = g_params[1];
    current.dparm3 = g_params[2];
    current.dparm4 = g_params[3];
    current.dparm5 = g_params[4];
    current.dparm6 = g_params[5];
    current.dparm7 = g_params[6];
    current.dparm8 = g_params[7];
    current.dparm9 = g_params[8];
    current.dparm10 = g_params[9];
    current.fill_color = g_fill_color;
    current.potential_params[0] = g_potential_params[0];
    current.potential_params[1] = g_potential_params[1];
    current.potential_params[2] = g_potential_params[2];
    current.random_seed_flag = g_random_seed_flag;
    current.random_seed = g_random_seed;
    current.inside_color = g_inside_color;
    current.log_map_flag = g_log_map_flag;
    current.inversion[0] = g_inversion[0];
    current.inversion[1] = g_inversion[1];
    current.inversion[2] = g_inversion[2];
    current.decomp = g_decomp[0];
    current.biomorph = g_biomorph;
    current.force_symmetry = g_force_symmetry;
    current.init_3d[0] = g_sphere;           // sphere? 1 = yes, 0 = no
    current.init_3d[1] = g_x_rot;            // rotate x-axis 60 degrees
    current.init_3d[2] = g_y_rot;            // rotate y-axis 90 degrees
    current.init_3d[3] = g_z_rot;            // rotate x-axis  0 degrees
    current.init_3d[4] = g_x_scale;          // scale x-axis, 90 percent
    current.init_3d[5] = g_y_scale;          // scale y-axis, 90 percent
    current.init_3d[1] = g_sphere_phi_min;   // longitude start, 180
    current.init_3d[2] = g_sphere_phi_max;   // longitude end ,   0
    current.init_3d[3] = g_sphere_theta_min; // latitude start,-90 degrees
    current.init_3d[4] = g_sphere_theta_max; // latitude stop,  90 degrees
    current.init_3d[5] = g_sphere_radius;    // should be user input
    current.init_3d[6] = g_rough;            // scale z-axis, 30 percent
    current.init_3d[7] = g_water_line;       // water level
    current.init_3d[8] = g_fill_type;        // fill type
    current.init_3d[9] = g_viewer_z;         // perspective view point
    current.init_3d[10] = g_shift_x;         // x shift
    current.init_3d[11] = g_shift_y;         // y shift
    current.init_3d[12] = g_light_x;         // x light vector coordinate
    current.init_3d[13] = g_light_y;         // y light vector coordinate
    current.init_3d[14] = g_light_z;         // z light vector coordinate
    current.init_3d[15] = g_light_avg;       // number of points to average
    current.preview_factor = g_preview_factor;
    current.adjust_3d_x = g_adjust_3d_x;
    current.adjust_3d_y = g_adjust_3d_y;
    current.red_crop_left = g_red_crop_left;
    current.red_crop_right = g_red_crop_right;
    current.blue_crop_left = g_blue_crop_left;
    current.blue_crop_right = g_blue_crop_right;
    current.red_bright = g_red_bright;
    current.blue_bright = g_blue_bright;
    current.converge_x_adjust = g_converge_x_adjust;
    current.converge_y_adjust = g_converge_y_adjust;
    current.eye_separation = g_eye_separation;
    current.glasses_type = g_glasses_type;
    current.outside_color = g_outside_color;
    current.x_3rd = g_x_3rd;
    current.y_3rd = g_y_3rd;
    current.user_std_calc_mode = g_user_std_calc_mode;
    current.three_pass = g_three_pass;
    current.stop_pass = g_stop_pass;
    current.distest = g_distance_estimator;
    current.trig_index[0] = g_trig_index[0];
    current.trig_index[1] = g_trig_index[1];
    current.trig_index[2] = g_trig_index[2];
    current.trig_index[3] = g_trig_index[3];
    current.finite_attractor = g_finite_attractor;
    current.initorbit[0] = g_init_orbit.x;
    current.initorbit[1] = g_init_orbit.y;
    current.use_init_orbit = g_use_init_orbit;
    current.periodicity_check = g_periodicity_check;
    current.disk_16_bit = g_disk_16_bit;
    current.release = g_release;
    current.save_release = g_release;
    current.display_3d = g_display_3d;
    current.ambient = g_ambient;
    current.randomize_3d = g_randomize_3d;
    current.haze = g_haze;
    current.transparent_color_3d[0] = g_transparent_color_3d[0];
    current.transparent_color_3d[1] = g_transparent_color_3d[1];
    current.color_cycle_range_lo = g_color_cycle_range_lo;
    current.color_cycle_range_hi = g_color_cycle_range_hi;
    current.distance_estimator_width_factor = g_distance_estimator_width_factor;
    current.julibrot_x_max = g_julibrot_x_max;
    current.julibrot_x_min = g_julibrot_x_min;
    current.julibrot_y_max = g_julibrot_y_max;
    current.julibrot_y_min = g_julibrot_y_min;
    current.julibrot_z_dots = g_julibrot_z_dots;
    current.julibrot_origin_fp = g_julibrot_origin_fp;
    current.julibrot_depth_fp = g_julibrot_depth_fp;
    current.julibrot_height_fp = g_julibrot_height_fp;
    current.julibrot_width_fp = g_julibrot_width_fp;
    current.julibrot_dist_fp = g_julibrot_dist_fp;
    current.eyes_fp = g_eyes_fp;
    current.new_orbit_type = g_new_orbit_type;
    current.julibrot_mode = g_julibrot_3d_mode;
    current.max_function = g_max_function;
    current.major_method = g_major_method;
    current.inverse_julia_minor_method = g_inverse_julia_minor_method;
    current.bailout = g_bail_out;
    current.bail_out_test = g_bail_out_test;
    current.iterations = g_max_iterations;
    current.old_demm_colors = g_old_demm_colors;
    current.log_map_fly_calculate = g_log_map_fly_calculate;
    current.is_mandelbrot = g_is_mandelbrot;
    current.close_proximity = g_close_proximity;
    current.bof_match_book_images = g_bof_match_book_images;
    current.orbit_delay = g_orbit_delay;
    current.orbit_interval = g_orbit_interval;
    current.orbit_corner_min_x = g_orbit_corner_min_x;
    current.orbit_corner_max_x = g_orbit_corner_max_x;
    current.orbit_corner_min_y = g_orbit_corner_min_y;
    current.orbit_corner_max_y = g_orbit_corner_max_y;
    current.orbit_corner_3_x = g_orbit_corner_3_x;
    current.orbit_corner_3_y = g_orbit_corner_3_y;
    current.keep_screen_coords = g_keep_screen_coords;
    current.draw_mode = g_draw_mode;
    std::memcpy(current.dac_box, g_dac_box, 256*3);
    switch (g_fractal_type)
    {
    case fractal_type::FORMULA:
    case fractal_type::FFORMULA:
        current.filename = g_formula_filename;
        current.file_item_name = g_formula_name;
        break;
    case fractal_type::IFS:
    case fractal_type::IFS3D:
        current.filename = g_ifs_filename;
        current.file_item_name = g_ifs_name;
        break;
    case fractal_type::LSYSTEM:
        current.filename = g_l_system_filename;
        current.file_item_name = g_l_system_name;
        break;
    default:
        current.filename.clear();
        current.file_item_name.clear();
        break;
    }
    if (historyptr == -1)        // initialize the history file
    {
        for (int i = 0; i < g_max_image_history; i++)
        {
            s_history[i] = current;
        }
        historyflag = false;
        historyptr = 0;
        saveptr = 0;   // initialize history ptr
    }
    else if (historyflag)
    {
        historyflag = false;            // coming from user history command, don't save
    }
    else if (std::memcmp(&current, &last, sizeof(HISTORY)))
    {
        if (++saveptr >= g_max_image_history)    // back to beginning of circular buffer
        {
            saveptr = 0;
        }
        if (++historyptr >= g_max_image_history)    // move user pointer in parallel
        {
            historyptr = 0;
        }
        s_history[saveptr] = current;
    }
}

void restore_history_info(int i)
{
    if (g_max_image_history <= 0 || g_bf_math != bf_math_type::NONE)
    {
        return;
    }
    HISTORY last = s_history[i];
    g_invert = 0;
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    g_resuming = false;
    g_fractal_type = last.image_fractal_type;
    g_x_min = last.x_min;
    g_x_max = last.x_max;
    g_y_min = last.y_min;
    g_y_max = last.y_max;
    g_params[0] = last.creal;
    g_params[1] = last.cimag;
    g_params[2] = last.dparm3;
    g_params[3] = last.dparm4;
    g_params[4] = last.dparm5;
    g_params[5] = last.dparm6;
    g_params[6] = last.dparm7;
    g_params[7] = last.dparm8;
    g_params[8] = last.dparm9;
    g_params[9] = last.dparm10;
    g_fill_color = last.fill_color;
    g_potential_params[0] = last.potential_params[0];
    g_potential_params[1] = last.potential_params[1];
    g_potential_params[2] = last.potential_params[2];
    g_random_seed_flag = last.random_seed_flag;
    g_random_seed = last.random_seed;
    g_inside_color = last.inside_color;
    g_log_map_flag = last.log_map_flag;
    g_inversion[0] = last.inversion[0];
    g_inversion[1] = last.inversion[1];
    g_inversion[2] = last.inversion[2];
    g_decomp[0] = last.decomp;
    g_user_biomorph_value = last.biomorph;
    g_biomorph = last.biomorph;
    g_force_symmetry = last.force_symmetry;
    g_sphere = last.init_3d[0];           // sphere? 1 = yes, 0 = no
    g_x_rot = last.init_3d[1];            // rotate x-axis 60 degrees
    g_y_rot = last.init_3d[2];            // rotate y-axis 90 degrees
    g_z_rot = last.init_3d[3];            // rotate x-axis  0 degrees
    g_x_scale = last.init_3d[4];          // scale x-axis, 90 percent
    g_y_scale = last.init_3d[5];          // scale y-axis, 90 percent
    g_sphere_phi_min = last.init_3d[1];   // longitude start, 180
    g_sphere_phi_max = last.init_3d[2];   // longitude end ,   0
    g_sphere_theta_min = last.init_3d[3]; // latitude start,-90 degrees
    g_sphere_theta_max = last.init_3d[4]; // latitude stop,  90 degrees
    g_sphere_radius = last.init_3d[5];    // should be user input
    g_rough = last.init_3d[6];            // scale z-axis, 30 percent
    g_water_line = last.init_3d[7];       // water level
    g_fill_type = last.init_3d[8];        // fill type
    g_viewer_z = last.init_3d[9];         // perspective view point
    g_shift_x = last.init_3d[10];         // x shift
    g_shift_y = last.init_3d[11];         // y shift
    g_light_x = last.init_3d[12];         // x light vector coordinate
    g_light_y = last.init_3d[13];         // y light vector coordinate
    g_light_z = last.init_3d[14];         // z light vector coordinate
    g_light_avg = last.init_3d[15];       // number of points to average
    g_preview_factor = last.preview_factor;
    g_adjust_3d_x = last.adjust_3d_x;
    g_adjust_3d_y = last.adjust_3d_y;
    g_red_crop_left = last.red_crop_left;
    g_red_crop_right = last.red_crop_right;
    g_blue_crop_left = last.blue_crop_left;
    g_blue_crop_right = last.blue_crop_right;
    g_red_bright = last.red_bright;
    g_blue_bright = last.blue_bright;
    g_converge_x_adjust = last.converge_x_adjust;
    g_converge_y_adjust = last.converge_y_adjust;
    g_eye_separation = last.eye_separation;
    g_glasses_type = last.glasses_type;
    g_outside_color = last.outside_color;
    g_x_3rd = last.x_3rd;
    g_y_3rd = last.y_3rd;
    g_user_std_calc_mode = last.user_std_calc_mode;
    g_std_calc_mode = last.user_std_calc_mode;
    g_three_pass = last.three_pass != 0;
    g_stop_pass = last.stop_pass;
    g_distance_estimator = last.distest;
    g_user_distance_estimator_value = last.distest;
    g_trig_index[0] = last.trig_index[0];
    g_trig_index[1] = last.trig_index[1];
    g_trig_index[2] = last.trig_index[2];
    g_trig_index[3] = last.trig_index[3];
    g_finite_attractor = last.finite_attractor;
    g_init_orbit.x = last.initorbit[0];
    g_init_orbit.y = last.initorbit[1];
    g_use_init_orbit = last.use_init_orbit;
    g_periodicity_check = last.periodicity_check;
    g_user_periodicity_value = last.periodicity_check;
    g_disk_16_bit = last.disk_16_bit;
    g_release = last.release;
    g_display_3d = last.display_3d;
    g_ambient = last.ambient;
    g_randomize_3d = last.randomize_3d;
    g_haze = last.haze;
    g_transparent_color_3d[0] = last.transparent_color_3d[0];
    g_transparent_color_3d[1] = last.transparent_color_3d[1];
    g_color_cycle_range_lo = last.color_cycle_range_lo;
    g_color_cycle_range_hi = last.color_cycle_range_hi;
    g_distance_estimator_width_factor = last.distance_estimator_width_factor;
    g_julibrot_x_max = last.julibrot_x_max;
    g_julibrot_x_min = last.julibrot_x_min;
    g_julibrot_y_max = last.julibrot_y_max;
    g_julibrot_y_min = last.julibrot_y_min;
    g_julibrot_z_dots = last.julibrot_z_dots;
    g_julibrot_origin_fp = last.julibrot_origin_fp;
    g_julibrot_depth_fp = last.julibrot_depth_fp;
    g_julibrot_height_fp = last.julibrot_height_fp;
    g_julibrot_width_fp = last.julibrot_width_fp;
    g_julibrot_dist_fp = last.julibrot_dist_fp;
    g_eyes_fp = last.eyes_fp;
    g_new_orbit_type = last.new_orbit_type;
    g_julibrot_3d_mode = last.julibrot_mode;
    g_max_function = last.max_function;
    g_major_method = last.major_method;
    g_inverse_julia_minor_method = last.inverse_julia_minor_method;
    g_bail_out = last.bailout;
    g_bail_out_test = last.bail_out_test;
    g_max_iterations = last.iterations;
    g_old_demm_colors = last.old_demm_colors;
    g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
    g_potential_flag = (g_potential_params[0] != 0.0);
    if (g_inversion[0] != 0.0)
    {
        g_invert = 3;
    }
    g_log_map_fly_calculate = last.log_map_fly_calculate;
    g_is_mandelbrot = last.is_mandelbrot;
    g_close_proximity = last.close_proximity;
    g_bof_match_book_images = last.bof_match_book_images;
    g_orbit_delay = last.orbit_delay;
    g_orbit_interval = last.orbit_interval;
    g_orbit_corner_min_x = last.orbit_corner_min_x;
    g_orbit_corner_max_x = last.orbit_corner_max_x;
    g_orbit_corner_min_y = last.orbit_corner_min_y;
    g_orbit_corner_max_y = last.orbit_corner_max_y;
    g_orbit_corner_3_x = last.orbit_corner_3_x;
    g_orbit_corner_3_y = last.orbit_corner_3_y;
    g_keep_screen_coords = last.keep_screen_coords;
    if (g_keep_screen_coords)
    {
        g_set_orbit_corners = true;
    }
    g_draw_mode = last.draw_mode;
    g_user_float_flag = g_cur_fractal_specific->isinteger == 0;
    std::memcpy(g_dac_box, last.dac_box, 256*3);
    std::memcpy(g_old_dac_box, last.dac_box, 256*3);
    if (g_map_specified)
    {
        for (int j = 0; j < 256; ++j)
        {
            g_map_clut[j][0] = last.dac_box[j][0];
            g_map_clut[j][1] = last.dac_box[j][1];
            g_map_clut[j][2] = last.dac_box[j][2];
        }
    }
    spindac(0, 1);
    if (g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
    {
        g_save_dac = 0;
    }
    else
    {
        g_save_dac = 1;
    }
    switch (g_fractal_type)
    {
    case fractal_type::FORMULA:
    case fractal_type::FFORMULA:
        g_formula_filename = last.filename;
        g_formula_name = last.file_item_name;
        if (g_formula_name.length() > ITEM_NAME_LEN)
        {
            g_formula_name.resize(ITEM_NAME_LEN);
        }
        break;
    case fractal_type::IFS:
    case fractal_type::IFS3D:
        g_ifs_filename = last.filename;
        g_ifs_name = last.file_item_name;
        if (g_ifs_name.length() > ITEM_NAME_LEN)
        {
            g_ifs_name.resize(ITEM_NAME_LEN);
        }
        break;
    case fractal_type::LSYSTEM:
        g_l_system_filename = last.filename;
        g_l_system_name = last.file_item_name;
        if (g_l_system_name.length() > ITEM_NAME_LEN)
        {
            g_l_system_name.resize(ITEM_NAME_LEN);
        }
        break;
    default:
        break;
    }
}
