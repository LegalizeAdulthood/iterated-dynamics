// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::fractals
{
enum class FractalType;
}

enum class Julibrot3DMode
{
    MONOCULAR = 0,
    LEFT_EYE = 1,
    RIGHT_EYE = 2,
    RED_BLUE = 3
};

constexpr const char *to_string(Julibrot3DMode value)
{
    switch (value)
    {
    default:
    case Julibrot3DMode::MONOCULAR:
        return "monocular";
    case Julibrot3DMode::LEFT_EYE:
        return "lefteye";
    case Julibrot3DMode::RIGHT_EYE:
        return "righteye";
    case Julibrot3DMode::RED_BLUE:
        return "red-blue";
    }
}

extern bool                  g_julibrot;
extern float                 g_eyes;
extern Julibrot3DMode        g_julibrot_3d_mode;
extern float                 g_julibrot_depth;
extern float                 g_julibrot_dist;
extern float                 g_julibrot_height;
extern float                 g_julibrot_origin;
extern float                 g_julibrot_width;
extern double                g_julibrot_x_max;
extern double                g_julibrot_x_min;
extern double                g_julibrot_y_max;
extern double                g_julibrot_y_min;
extern int                   g_julibrot_z_dots;
extern id::fractals::FractalType           g_new_orbit_type;
extern const char *          g_julibrot_3d_options[];

bool julibrot_per_image();
int julibrot_per_pixel();

namespace id::fractals
{

class Standard4D
{
public:
    Standard4D();
    bool iterate();

private:
    double m_y{};
    double m_x{};
    int m_y_dot{};
    int m_x_dot{};
};

} // namespace id::fractals
