// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/history.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/Inversion.h"
#include "engine/log_map.h"
#include "engine/orbit.h"
#include "engine/Potential.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "engine/spindac.h"
#include "engine/sticky_orbits.h"
#include "engine/trig_fns.h"
#include "engine/UserData.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "geometry/3d.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"
#include "io/library.h"
#include "math/Point.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/version.h"
#include "ui/diskvid.h"

#include <config/port.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

namespace
{

struct ImageHistory
{
    FractalType image_fractal_type;
    ImageRegion image_region;
    std::array<double, MAX_PARAMS> params;
    std::array<double, 3> potential_params;
    int random_seed;
    bool random_seed_flag;
    int biomorph;
    int inside_color;
    long log_map_flag;
    InversionParams inversion;
    std::array<int, 2> decomp;
    SymmetryType force_symmetry;
    std::array<int, 16> init_3d;
    int preview_factor;
    Point2i adjust_3d;
    int red_crop_left;
    int red_crop_right;
    int blue_crop_left;
    int blue_crop_right;
    int red_bright;
    int blue_bright;
    int converge_x_adjust;
    int eye_separation;
    GlassesType glasses_type;
    int outside_color;
    long dist_est;
    std::array<TrigFn, 4> trig_index;
    bool finite_attractor;
    DComplex init_orbit;
    int periodicity_check;
    bool disk_16_bit;
    int release;
    int save_release;
    Display3DMode display_3d;
    std::array<int, 2> transparent_color_3d;
    int ambient;
    int haze;
    int randomize_3d;
    int color_cycle_range_lo;
    int color_cycle_range_hi;
    int distance_estimator_width_factor;
    int fill_color;
    double julibrot_x_max;
    double julibrot_x_min;
    double julibrot_y_max;
    double julibrot_y_min;
    int julibrot_z_dots;
    float julibrot_origin;
    float julibrot_depth;
    float julibrot_height;
    float julibrot_width;
    float julibrot_dist;
    float eyes;
    FractalType new_orbit_type;
    Julibrot3DMode julibrot_mode;
    Major major_method;
    Minor inverse_julia_minor_method;
    long bailout;
    Bailout bailout_test;
    long iterations;
    int converge_y_adjust;
    bool old_demm_colors;
    std::string filename;
    std::string file_item_name;
    unsigned char dac_box[256][3];
    char max_function;
    char user_std_calc_mode;
    bool three_pass;
    InitOrbitMode use_init_orbit;
    int log_map_fly_calculate;
    int stop_pass;
    bool is_mandelbrot;
    double close_proximity;
    bool bof_match_book_images;
    int orbit_delay;
    long orbit_interval;
    ImageRegion orbit_corner;
    bool keep_screen_coords;
    OrbitDrawMode draw_mode;
};

bool dac_box_equal(const Byte lhs[256][3], const Byte rhs[256][3])
{
    for (int i = 0; i < 256; ++i)
    {
        if (lhs[i][0] != rhs[i][0] || lhs[i][1] != rhs[i][1] || lhs[i][2] != rhs[i][2])
        {
            return false;
        }
    }
    return true;
}

bool operator==(const ImageHistory &lhs, const ImageHistory &rhs)
{
    return lhs.image_fractal_type == rhs.image_fractal_type                                             //
        && lhs.image_region == rhs.image_region                                                         //
        && lhs.params == rhs.params                                                                     //
        && lhs.potential_params == rhs.potential_params                                                 //
        && lhs.random_seed == rhs.random_seed                                                           //
        && lhs.random_seed_flag == rhs.random_seed_flag                                                 //
        && lhs.biomorph == rhs.biomorph                                                                 //
        && lhs.inside_color == rhs.inside_color                                                         //
        && lhs.log_map_flag == rhs.log_map_flag                                                         //
        && std::equal(std::begin(lhs.inversion), std::end(lhs.inversion), std::begin(rhs.inversion))    //
        && std::equal(std::begin(lhs.decomp), std::end(lhs.decomp), std::begin(rhs.decomp))             //
        && lhs.force_symmetry == rhs.force_symmetry                                                     //
        && std::equal(std::begin(lhs.init_3d), std::end(lhs.init_3d), std::begin(rhs.init_3d))          //
        && lhs.preview_factor == rhs.preview_factor                                                     //
        && lhs.adjust_3d == rhs.adjust_3d                                                               //
        && lhs.red_crop_left == rhs.red_crop_left                                                       //
        && lhs.red_crop_right == rhs.red_crop_right                                                     //
        && lhs.blue_crop_left == rhs.blue_crop_left                                                     //
        && lhs.blue_crop_right == rhs.blue_crop_right                                                   //
        && lhs.red_bright == rhs.red_bright                                                             //
        && lhs.blue_bright == rhs.blue_bright                                                           //
        && lhs.converge_x_adjust == rhs.converge_x_adjust                                               //
        && lhs.eye_separation == rhs.eye_separation                                                     //
        && lhs.glasses_type == rhs.glasses_type                                                         //
        && lhs.outside_color == rhs.outside_color                                                       //
        && lhs.dist_est == rhs.dist_est                                                                 //
        && std::equal(std::begin(lhs.trig_index), std::end(lhs.trig_index), std::begin(rhs.trig_index)) //
        && lhs.finite_attractor == rhs.finite_attractor                                                 //
        && lhs.init_orbit == rhs.init_orbit                                                             //
        && lhs.periodicity_check == rhs.periodicity_check                                               //
        && lhs.disk_16_bit == rhs.disk_16_bit                                                           //
        && lhs.release == rhs.release                                                                   //
        && lhs.save_release == rhs.save_release                                                         //
        && lhs.display_3d == rhs.display_3d                                                             //
        && std::equal(std::begin(lhs.transparent_color_3d), std::end(lhs.transparent_color_3d),
               std::begin(rhs.transparent_color_3d))                                                    //
        && lhs.ambient == rhs.ambient                                                                   //
        && lhs.haze == rhs.haze                                                                         //
        && lhs.randomize_3d == rhs.randomize_3d                                                         //
        && lhs.color_cycle_range_lo == rhs.color_cycle_range_lo                                         //
        && lhs.color_cycle_range_hi == rhs.color_cycle_range_hi                                         //
        && lhs.distance_estimator_width_factor == rhs.distance_estimator_width_factor                   //
        && lhs.fill_color == rhs.fill_color                                                             //
        && lhs.julibrot_x_max == rhs.julibrot_x_max                                                     //
        && lhs.julibrot_x_min == rhs.julibrot_x_min                                                     //
        && lhs.julibrot_y_max == rhs.julibrot_y_max                                                     //
        && lhs.julibrot_y_min == rhs.julibrot_y_min                                                     //
        && lhs.julibrot_z_dots == rhs.julibrot_z_dots                                                   //
        && lhs.julibrot_origin == rhs.julibrot_origin                                                   //
        && lhs.julibrot_depth == rhs.julibrot_depth                                                     //
        && lhs.julibrot_height == rhs.julibrot_height                                                   //
        && lhs.julibrot_width == rhs.julibrot_width                                                     //
        && lhs.julibrot_dist == rhs.julibrot_dist                                                       //
        && lhs.eyes == rhs.eyes                                                                         //
        && lhs.new_orbit_type == rhs.new_orbit_type                                                     //
        && lhs.julibrot_mode == rhs.julibrot_mode                                                       //
        && lhs.major_method == rhs.major_method                                                         //
        && lhs.inverse_julia_minor_method == rhs.inverse_julia_minor_method                             //
        && lhs.bailout == rhs.bailout                                                                   //
        && lhs.bailout_test == rhs.bailout_test                                                         //
        && lhs.iterations == rhs.iterations                                                             //
        && lhs.converge_y_adjust == rhs.converge_y_adjust                                               //
        && lhs.old_demm_colors == rhs.old_demm_colors                                                   //
        && lhs.filename == rhs.filename                                                                 //
        && lhs.file_item_name == rhs.file_item_name                                                     //
        && dac_box_equal(lhs.dac_box, rhs.dac_box)                                                      //
        && lhs.max_function == rhs.max_function                                                         //
        && lhs.user_std_calc_mode == rhs.user_std_calc_mode                                             //
        && lhs.three_pass == rhs.three_pass                                                             //
        && lhs.use_init_orbit == rhs.use_init_orbit                                                     //
        && lhs.log_map_fly_calculate == rhs.log_map_fly_calculate                                       //
        && lhs.stop_pass == rhs.stop_pass                                                               //
        && lhs.is_mandelbrot == rhs.is_mandelbrot                                                       //
        && lhs.close_proximity == rhs.close_proximity                                                   //
        && lhs.bof_match_book_images == rhs.bof_match_book_images                                       //
        && lhs.orbit_delay == rhs.orbit_delay                                                           //
        && lhs.orbit_interval == rhs.orbit_interval                                                     //
        && lhs.orbit_corner == rhs.orbit_corner                                                         //
        && lhs.keep_screen_coords == rhs.keep_screen_coords                                             //
        && lhs.draw_mode == rhs.draw_mode;                                                              //
}

std::ostream &operator<<(std::ostream &str, const FractalType value)
{
    str << +value;
    return str;
}

std::ostream &operator<<(std::ostream &str, SymmetryType value)
{
    str << static_cast<int>(value);
    return str;
}

std::ostream &operator<<(std::ostream &str, const TrigFn value)
{
    str << +value;
    return str;
}

std::ostream &operator<<(std::ostream &str, Display3DMode value)
{
    str << static_cast<int>(value);
    return str;
}

std::ostream &operator<<(std::ostream &str, const Julibrot3DMode value)
{
    str << '"' << to_string(value) << '"';
    return str;
}

std::ostream &operator<<(std::ostream &str, const Major value)
{
    str << +value;
    return str;
}

std::ostream &operator<<(std::ostream &str, const Minor value)
{
    str << +value;
    return str;
}

std::ostream &operator<<(std::ostream &str, Bailout value)
{
    str << static_cast<int>(value);
    return str;
}

std::ostream &operator<<(std::ostream &str, InitOrbitMode value)
{
    str << static_cast<int>(value);
    return str;
}

std::ostream &operator<<(std::ostream &str, GlassesType value)
{
    str << static_cast<int>(value);
    return str;
}

template <typename T, size_t N>
struct JsonArray
{
    explicit JsonArray(const T (&value)[N])
    {
        for (size_t i = 0; i < N; ++i)
        {
            array[i] = value[i];
        }
    }

    explicit JsonArray(const std::array<T, N> &value) :
        array(value)
    {
    }

    std::array<T, N> array{};
};

template <typename T, size_t N>
std::ostream &operator<<(std::ostream &str, const JsonArray<T, N> &value)
{
    str << '[';
    bool first{true};
    for (T elem : value.array)
    {
        if (!first)
        {
            str << ',';
        }
        str << elem;
        first = false;
    }
    str << ']';
    return str;
}

struct JsonValue
{
    explicit JsonValue(const DComplex &value) :
        m_value(value)
    {
    }

    friend std::ostream &operator<<(std::ostream &str, const JsonValue &value)
    {
        str << '[' << value.m_value.x << ',' << value.m_value.y << ']';
        return str;
    }

private:
    const DComplex &m_value;
};

struct DacBox
{
    explicit DacBox(const unsigned char dac[256][3])
    {
        for (int i = 0; i < 256; ++i)
        {
            value[i][0] = dac[i][0];
            value[i][1] = dac[i][1];
            value[i][2] = dac[i][2];
        }
    }

    unsigned char value[256][3];
};

std::ostream &operator<<(std::ostream &str, const DacBox &value)
{
    str << '[';
    for (int i = 0; i < 256; ++i)
    {
        if (i > 0)
        {
            str << ',';
        }
        str << '[' << static_cast<int>(value.value[i][0]) << ',' << static_cast<int>(value.value[i][1]) << ','
            << static_cast<int>(value.value[i][2]) << ']';
    }
    str << ']';
    return str;
}

std::ostream &operator<<(std::ostream &str, const ImageHistory &value)
{
    str << '{';
    str << R"json("image_fractal_type":)json" << value.image_fractal_type << ',';
    str << R"json("x_min":)json" << value.image_region.m_min.x << ',';
    str << R"json("y_min":)json" << value.image_region.m_min.y << ',';
    str << R"json("x_max":)json" << value.image_region.m_max.x << ',';
    str << R"json("y_max":)json" << value.image_region.m_max.y << ',';
    str << R"json("x_3rd":)json" << value.image_region.m_3rd.x << ',';
    str << R"json("y_3rd":)json" << value.image_region.m_3rd.y << ',';
    str << R"json("params":)json" << JsonArray(value.params) << ',';
    str << R"json("potential_params":)json" << JsonArray(value.potential_params) << ',';
    str << R"json("random_seed":)json" << value.random_seed << ',';
    str << R"json("random_seed_flag":)json" << value.random_seed_flag << ',';
    str << R"json("biomorph":)json" << value.biomorph << ',';
    str << R"json("inside_color":)json" << value.inside_color << ',';
    str << R"json("log_map_flag":)json" << value.log_map_flag << ',';
    str << R"json("inversion":)json" << JsonArray(value.inversion) << ',';
    str << R"json("decomp":)json" << JsonArray(value.decomp) << ',';
    str << R"json("force_symmetry":)json" << value.force_symmetry << ',';
    str << R"json("init_3d":)json" << JsonArray(value.init_3d) << ',';
    str << R"json("preview_factor":)json" << value.preview_factor << ',';
    str << R"json("adjust_3d_x":)json" << value.adjust_3d.x << ',';
    str << R"json("adjust_3d_y":)json" << value.adjust_3d.y << ',';
    str << R"json("red_crop_left":)json" << value.red_crop_left << ',';
    str << R"json("red_crop_right":)json" << value.red_crop_right << ',';
    str << R"json("blue_crop_left":)json" << value.blue_crop_left << ',';
    str << R"json("blue_crop_right":)json" << value.blue_crop_right << ',';
    str << R"json("red_bright":)json" << value.red_bright << ',';
    str << R"json("blue_bright":)json" << value.blue_bright << ',';
    str << R"json("converge_x_adjust":)json" << value.converge_x_adjust << ',';
    str << R"json("eye_separation":)json" << value.eye_separation << ',';
    str << R"json("glasses_type":)json" << value.glasses_type << ',';
    str << R"json("outside_color":)json" << value.outside_color << ',';
    str << R"json("dist_est":)json" << value.dist_est << ',';
    str << R"json("trig_index":)json" << JsonArray(value.trig_index) << ',';
    str << R"json("finite_attractor":)json" << value.finite_attractor << ',';
    str << R"json("init_orbit":)json" << JsonValue(value.init_orbit) << ',';
    str << R"json("periodicity_check":)json" << value.periodicity_check << ',';
    str << R"json("disk_16_bit":)json" << value.disk_16_bit << ',';
    str << R"json("release":)json" << value.release << ',';
    str << R"json("save_release":)json" << value.save_release << ',';
    str << R"json("display_3d":)json" << value.display_3d << ',';
    str << R"json("transparent_color_3d":)json" << JsonArray(value.transparent_color_3d) << ',';
    str << R"json("ambient":)json" << value.ambient << ',';
    str << R"json("haze":)json" << value.haze << ',';
    str << R"json("randomize_3d":)json" << value.randomize_3d << ',';
    str << R"json("color_cycle_range_lo":)json" << value.color_cycle_range_lo << ',';
    str << R"json("color_cycle_range_hi":)json" << value.color_cycle_range_hi << ',';
    str << R"json("distance_estimator_width_factor":)json" << value.distance_estimator_width_factor << ',';
    str << R"json("fill_color":)json" << value.fill_color << ',';
    str << R"json("julibrot_x_max":)json" << value.julibrot_x_max << ',';
    str << R"json("julibrot_x_min":)json" << value.julibrot_x_min << ',';
    str << R"json("julibrot_y_max":)json" << value.julibrot_y_max << ',';
    str << R"json("julibrot_y_min":)json" << value.julibrot_y_min << ',';
    str << R"json("julibrot_z_dots":)json" << value.julibrot_z_dots << ',';
    str << R"json("julibrot_origin":)json" << value.julibrot_origin << ',';
    str << R"json("julibrot_depth":)json" << value.julibrot_depth << ',';
    str << R"json("julibrot_height":)json" << value.julibrot_height << ',';
    str << R"json("julibrot_width":)json" << value.julibrot_width << ',';
    str << R"json("julibrot_dist":)json" << value.julibrot_dist << ',';
    str << R"json("eyes":)json" << value.eyes << ',';
    str << R"json("new_orbit_type":)json" << value.new_orbit_type << ',';
    str << R"json("julibrot_mode":)json" << value.julibrot_mode << ',';
    str << R"json("major_method":)json" << value.major_method << ',';
    str << R"json("inverse_julia_minor_method":)json" << value.inverse_julia_minor_method << ',';
    str << R"json("bailout":)json" << value.bailout << ',';
    str << R"json("bailout_test":)json" << value.bailout_test << ',';
    str << R"json("iterations":)json" << value.iterations << ',';
    str << R"json("converge_y_adjust":)json" << value.converge_y_adjust << ',';
    str << R"json("old_demm_colors":)json" << value.old_demm_colors << ',';
    str << R"json("filename":)json" << value.filename << ',';
    str << R"json("file_item_name":)json" << value.file_item_name << ',';
    str << R"json("dac_box":)json" << DacBox(value.dac_box) << ',';
    str << R"json("max_function":)json" << static_cast<int>(value.max_function) << ',';
    str << R"json("user_std_calc_mode":)json" << '"' << value.user_std_calc_mode << '"' << ',';
    str << R"json("three_pass":)json" << value.three_pass << ',';
    str << R"json("use_init_orbit":)json" << value.use_init_orbit << ',';
    str << R"json("log_map_fly_calculate":)json" << value.log_map_fly_calculate << ',';
    str << R"json("stop_pass":)json" << value.stop_pass << ',';
    str << R"json("is_mandelbrot":)json" << value.is_mandelbrot << ',';
    str << R"json("close_proximity":)json" << value.close_proximity << ',';
    str << R"json("bof_match_book_images":)json" << value.bof_match_book_images << ',';
    str << R"json("orbit_delay":)json" << value.orbit_delay << ',';
    str << R"json("orbit_interval":)json" << value.orbit_interval << ',';
    str << R"json("orbit_corner_min_x":)json" << value.orbit_corner.m_min.x << ',';
    str << R"json("orbit_corner_max_x":)json" << value.orbit_corner.m_max.x << ',';
    str << R"json("orbit_corner_min_y":)json" << value.orbit_corner.m_min.y << ',';
    str << R"json("orbit_corner_max_y":)json" << value.orbit_corner.m_max.y << ',';
    str << R"json("orbit_corner_3rd_x":)json" << value.orbit_corner.m_3rd.x << ',';
    str << R"json("orbit_corner_3rd_y":)json" << value.orbit_corner.m_3rd.y << ',';
    str << R"json("keep_screen_coords":)json" << value.keep_screen_coords << ',';
    str << R"json("draw_mode":)json" << '"' << static_cast<char>(value.draw_mode) << '"';
    str << '}';
    return str;
}

} // namespace

int g_history_ptr{-1};       // user pointer into history tbl
bool g_history_flag{};       // are we backing off in history?
int g_max_image_history{10}; //

static int s_save_ptr{}; // save ptr into history tbl
static std::vector<ImageHistory> s_history;

void history_init()
{
    s_history.resize(g_max_image_history);
}

void save_history_info()
{
    if (g_max_image_history <= 0 || g_bf_math != BFMathType::NONE)
    {
        return;
    }
    ImageHistory last = s_history[s_save_ptr];

    ImageHistory current{};
    current.image_fractal_type = g_fractal_type;
    current.image_region = g_image_region;
    std::copy_n(g_params, MAX_PARAMS, current.params.data());
    current.fill_color = g_fill_color;
    current.potential_params = g_potential.params;
    current.random_seed_flag = g_random_seed_flag;
    current.random_seed = g_random_seed;
    current.inside_color = g_inside_color;
    current.log_map_flag = g_log_map_flag;
    current.inversion = g_inversion.params;
    std::copy_n(g_decomp, 2, current.decomp.data());
    current.biomorph = g_biomorph;
    current.force_symmetry = g_force_symmetry;
    current.init_3d[0] = g_sphere ? 1 : 0;   // sphere? 1 = yes, 0 = no
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
    current.init_3d[8] = +g_fill_type;       // fill type
    current.init_3d[9] = g_viewer_z;         // perspective view point
    current.init_3d[10] = g_shift_x;         // x shift
    current.init_3d[11] = g_shift_y;         // y shift
    current.init_3d[12] = g_light_x;         // x light vector coordinate
    current.init_3d[13] = g_light_y;         // y light vector coordinate
    current.init_3d[14] = g_light_z;         // z light vector coordinate
    current.init_3d[15] = g_light_avg;       // number of points to average
    current.preview_factor = g_preview_factor;
    current.adjust_3d = g_adjust_3d;
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
    current.user_std_calc_mode = static_cast<char>(g_user.std_calc_mode);
    current.three_pass = g_three_pass;
    current.stop_pass = g_stop_pass;
    current.dist_est = g_distance_estimator;
    std::copy_n(g_trig_index, 4, current.trig_index.data());
    current.finite_attractor = g_finite_attractor;
    current.init_orbit = g_init_orbit;
    current.use_init_orbit = g_use_init_orbit;
    current.periodicity_check = g_periodicity_check;
    current.disk_16_bit = g_disk_16_bit;
    current.release = g_release;
    current.save_release = g_release;
    current.display_3d = g_display_3d;
    current.ambient = g_ambient;
    current.randomize_3d = g_randomize_3d;
    current.haze = g_haze;
    std::copy_n(g_transparent_color_3d, 2, current.transparent_color_3d.data());
    current.color_cycle_range_lo = g_color_cycle_range_lo;
    current.color_cycle_range_hi = g_color_cycle_range_hi;
    current.distance_estimator_width_factor = g_distance_estimator_width_factor;
    current.julibrot_x_max = g_julibrot_x_max;
    current.julibrot_x_min = g_julibrot_x_min;
    current.julibrot_y_max = g_julibrot_y_max;
    current.julibrot_y_min = g_julibrot_y_min;
    current.julibrot_z_dots = g_julibrot_z_dots;
    current.julibrot_origin = g_julibrot_origin;
    current.julibrot_depth = g_julibrot_depth;
    current.julibrot_height = g_julibrot_height;
    current.julibrot_width = g_julibrot_width;
    current.julibrot_dist = g_julibrot_dist;
    current.eyes = g_eyes;
    current.new_orbit_type = g_new_orbit_type;
    current.julibrot_mode = g_julibrot_3d_mode;
    current.max_function = g_max_function;
    current.major_method = g_major_method;
    current.inverse_julia_minor_method = g_inverse_julia_minor_method;
    current.bailout = g_user.bailout_value;
    current.bailout_test = g_bailout_test;
    current.iterations = g_max_iterations;
    current.old_demm_colors = g_old_demm_colors;
    current.log_map_fly_calculate = static_cast<int>(g_log_map_fly_calculate);
    current.is_mandelbrot = g_is_mandelbrot;
    current.close_proximity = g_close_proximity;
    current.bof_match_book_images = g_bof_match_book_images;
    current.orbit_delay = g_orbit_delay;
    current.orbit_interval = g_orbit_interval;
    current.orbit_corner = g_orbit_corner;
    current.keep_screen_coords = g_keep_screen_coords;
    current.draw_mode = g_draw_mode;
    std::memcpy(current.dac_box, g_dac_box, 256*3);
    switch (g_fractal_type)
    {
    case FractalType::FORMULA:
        current.filename = g_formula_filename.string();
        current.file_item_name = g_formula_name;
        break;
    case FractalType::IFS:
    case FractalType::IFS_3D:
        current.filename = g_ifs_filename.string();
        current.file_item_name = g_ifs_name;
        break;
    case FractalType::L_SYSTEM:
        current.filename = g_l_system_filename.string();
        current.file_item_name = g_l_system_name;
        break;
    default:
        current.filename.clear();
        current.file_item_name.clear();
        break;
    }
    if (g_history_ptr == -1)        // initialize the history file
    {
        for (int i = 0; i < g_max_image_history; i++)
        {
            s_history[i] = current;
        }
        g_history_flag = false;
        g_history_ptr = 0;
        s_save_ptr = 0;   // initialize history ptr
    }
    else if (g_history_flag)
    {
        g_history_flag = false;            // coming from user history command, don't save
    }
    else if (current == last)
    {
        if (++s_save_ptr >= g_max_image_history)    // back to beginning of circular buffer
        {
            s_save_ptr = 0;
        }
        if (++g_history_ptr >= g_max_image_history)    // move user pointer in parallel
        {
            g_history_ptr = 0;
        }
        s_history[s_save_ptr] = current;
    }
    if (g_debug_flag == DebugFlags::HISTORY_DUMP_JSON)
    {
        std::filesystem::path path{get_save_path(WriteFile::ROOT, "history.json")};
        assert(!path.empty());
        std::ofstream str(path, std::ios_base::app);
        str << current;
    }
}

void restore_history_info(const int i)
{
    if (g_max_image_history <= 0 || g_bf_math != BFMathType::NONE)
    {
        return;
    }
    ImageHistory last = s_history[i];
    g_inversion.invert = 0;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_resuming = false;
    set_fractal_type(last.image_fractal_type);
    g_image_region = last.image_region;
    std::copy_n(last.params.data(), MAX_PARAMS, g_params);
    g_fill_color = last.fill_color;
    g_potential.params = last.potential_params;
    g_random_seed_flag = last.random_seed_flag;
    g_random_seed = last.random_seed;
    g_inside_color = last.inside_color;
    g_log_map_flag = last.log_map_flag;
    g_inversion.params = last.inversion;
    g_decomp[0] = last.decomp[0];
    g_decomp[1] = last.decomp[1];
    g_user.biomorph_value = last.biomorph;
    g_biomorph = last.biomorph;
    g_force_symmetry = last.force_symmetry;
    g_sphere = last.init_3d[0] != 0;                       // sphere? 1 = yes, 0 = no
    g_x_rot = last.init_3d[1];                             // rotate x-axis 60 degrees
    g_y_rot = last.init_3d[2];                             // rotate y-axis 90 degrees
    g_z_rot = last.init_3d[3];                             // rotate x-axis  0 degrees
    g_x_scale = last.init_3d[4];                           // scale x-axis, 90 percent
    g_y_scale = last.init_3d[5];                           // scale y-axis, 90 percent
    g_sphere_phi_min = last.init_3d[1];                    // longitude start, 180
    g_sphere_phi_max = last.init_3d[2];                    // longitude end ,   0
    g_sphere_theta_min = last.init_3d[3];                  // latitude start,-90 degrees
    g_sphere_theta_max = last.init_3d[4];                  // latitude stop,  90 degrees
    g_sphere_radius = last.init_3d[5];                     // should be user input
    g_rough = last.init_3d[6];                             // scale z-axis, 30 percent
    g_water_line = last.init_3d[7];                        // water level
    g_fill_type = static_cast<FillType>(last.init_3d[8]); // fill type
    g_viewer_z = last.init_3d[9];                          // perspective view point
    g_shift_x = last.init_3d[10];                          // x shift
    g_shift_y = last.init_3d[11];                          // y shift
    g_light_x = last.init_3d[12];                          // x light vector coordinate
    g_light_y = last.init_3d[13];                          // y light vector coordinate
    g_light_z = last.init_3d[14];                          // z light vector coordinate
    g_light_avg = last.init_3d[15];                        // number of points to average
    g_preview_factor = last.preview_factor;
    g_adjust_3d = last.adjust_3d;
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
    g_user.std_calc_mode = static_cast<CalcMode>(last.user_std_calc_mode);
    g_std_calc_mode = static_cast<CalcMode>(last.user_std_calc_mode);
    g_three_pass = last.three_pass != 0;
    g_stop_pass = last.stop_pass;
    g_distance_estimator = last.dist_est;
    g_user.distance_estimator_value = last.dist_est;
    g_trig_index[0] = last.trig_index[0];
    g_trig_index[1] = last.trig_index[1];
    g_trig_index[2] = last.trig_index[2];
    g_trig_index[3] = last.trig_index[3];
    g_finite_attractor = last.finite_attractor;
    g_init_orbit = last.init_orbit;
    g_use_init_orbit = last.use_init_orbit;
    g_periodicity_check = last.periodicity_check;
    g_user.periodicity_value = last.periodicity_check;
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
    g_julibrot_origin = last.julibrot_origin;
    g_julibrot_depth = last.julibrot_depth;
    g_julibrot_height = last.julibrot_height;
    g_julibrot_width = last.julibrot_width;
    g_julibrot_dist = last.julibrot_dist;
    g_eyes = last.eyes;
    g_new_orbit_type = last.new_orbit_type;
    g_julibrot_3d_mode = last.julibrot_mode;
    g_max_function = last.max_function;
    g_major_method = last.major_method;
    g_inverse_julia_minor_method = last.inverse_julia_minor_method;
    g_user.bailout_value = last.bailout;
    g_bailout_test = last.bailout_test;
    g_max_iterations = last.iterations;
    g_old_demm_colors = last.old_demm_colors;
    g_potential.flag = g_potential.params[0] != 0.0;
    if (g_inversion.params[0] != 0.0)
    {
        g_inversion.invert = 3;
    }
    g_log_map_fly_calculate = static_cast<LogMapCalculate>(last.log_map_fly_calculate);
    g_is_mandelbrot = last.is_mandelbrot;
    g_close_proximity = last.close_proximity;
    g_bof_match_book_images = last.bof_match_book_images;
    g_orbit_delay = last.orbit_delay;
    g_orbit_interval = last.orbit_interval;
    g_orbit_corner = last.orbit_corner;
    g_keep_screen_coords = last.keep_screen_coords;
    if (g_keep_screen_coords)
    {
        g_set_orbit_corners = true;
    }
    g_draw_mode = last.draw_mode;
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
    refresh_dac();
    g_save_dac = g_fractal_type == FractalType::JULIBROT ? SaveDAC::NO : SaveDAC::YES;
    switch (g_fractal_type)
    {
    case FractalType::FORMULA:
        g_formula_filename = last.filename;
        g_formula_name = last.file_item_name;
        if (g_formula_name.length() > ITEM_NAME_LEN)
        {
            g_formula_name.resize(ITEM_NAME_LEN);
        }
        break;
    case FractalType::IFS:
    case FractalType::IFS_3D:
        g_ifs_filename = last.filename;
        g_ifs_name = last.file_item_name;
        if (g_ifs_name.length() > ITEM_NAME_LEN)
        {
            g_ifs_name.resize(ITEM_NAME_LEN);
        }
        break;
    case FractalType::L_SYSTEM:
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

} // namespace id::ui
