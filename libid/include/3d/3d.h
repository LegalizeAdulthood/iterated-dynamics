// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    COL_MAX = 4, // maximum column (4 x 4 matrix)
    ROW_MAX = 4  // maximum row    (4 x 4 matrix)
};
using Matrix = double[ROW_MAX][COL_MAX];  // matrix of doubles
using MatrixL = long[ROW_MAX][COL_MAX];  // matrix of longs
/* A Matrix is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */
using Vector = double[3];  // vector of doubles
using VectorL = long[3];  // vector of longs
/* A Vector is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */

inline double dot_product(Vector v1, Vector v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

enum class FillType
{
    SURFACE_GRID = -1,
    POINTS = 0,
    WIRE_FRAME = 1,
    SURFACE_INTERPOLATED = 2,
    SURFACE_CONSTANT = 3,
    SOLID_FILL = 4,
    LIGHT_SOURCE_BEFORE = 5,
    LIGHT_SOURCE_AFTER = 6
};
constexpr int operator+(FillType value)
{
    return static_cast<int>(value);
}

// regular 3D
extern bool g_sphere;  // sphere? true = yes, false = no
extern int g_x_rot;   // rotate x-axis 60 degrees
extern int g_y_rot;   // rotate y-axis 90 degrees
extern int g_z_rot;   // rotate x-axis  0 degrees
extern int g_x_scale; // scale x-axis, 90 percent
extern int g_y_scale; // scale y-axis, 90 percent
// sphere 3D
extern int g_sphere_phi_min;   // longitude start, 180
extern int g_sphere_phi_max;   // longitude end ,   0
extern int g_sphere_theta_min; // latitude start,-90 degrees
extern int g_sphere_theta_max; // latitude stop,  90 degrees
extern int g_sphere_radius;    // should be user input
// common parameters
extern int g_rough;      // scale z-axis, 30 percent
extern int g_water_line; // water level
extern FillType g_fill_type;  // fill type
extern int g_viewer_z;   // perspective view point
extern int g_shift_x;    // x shift
extern int g_shift_y;    // y shift
extern int g_light_x;    // x light vector coordinate
extern int g_light_y;    // y light vector coordinate
extern int g_light_z;    // z light vector coordinate
extern int g_light_avg;  // number of points to average

inline bool illumine()
{
    return g_fill_type > FillType::SOLID_FILL; // illumination model
}

void identity(Matrix m);
void mat_mul(Matrix lhs, Matrix rhs, Matrix result);
void scale(double sx, double sy, double sz, Matrix m);
void x_rot(double theta, Matrix m);
void y_rot(double theta, Matrix m);
void z_rot(double theta, Matrix m);
void trans(double tx, double ty, double tz, Matrix m);
int cross_product(Vector v, Vector w, Vector cross);
bool normalize_vector(Vector v);
int vec_mat_mul(Vector s, Matrix m, Vector t);
void vec_g_mat_mul(Vector s);
int perspective(Vector v);
int long_vec_mat_mul_persp(VectorL s, MatrixL m, VectorL t0, VectorL t, VectorL view, int bit_shift);
