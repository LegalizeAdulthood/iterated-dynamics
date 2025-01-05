// SPDX-License-Identifier: GPL-3.0-only
//
// A word about the 3D library. Even though this library supports
// three dimensions, the matrices are 4x4 for the following reason.
// With normal 3 dimensional vectors, translation is an ADDITION,
// and rotation is a MULTIPLICATION. A vector {x,y,z} is represented
// as a 4-tuple {x,y,z,1}. It is then possible to define a 4x4
// matrix such that multiplying the vector by the matrix translates
// the vector. This allows combinations of translation and rotation
// to be obtained in a single matrix by multiplying a translation
// matrix and a rotation matrix together. Note that in the code,
// vectors have three components; since the fourth component is
// always 1, that value is not included in the vector variable to
// save space, but the routines make use of the fourth component
// (see vec_mult()). Similarly, the fourth column of EVERY matrix is
// always
//          0
//          0
//          0
//          1
// but currently the C version of a matrix includes this even though
// it could be left out of the data structure and assumed in the
// routines. Vectors are ROW vectors, and are always multiplied with
// matrices FROM THE LEFT (e.g. vector*matrix). Also note the order
// of indices of a matrix is matrix[row][column], and in usual C
// fashion, numbering starts with 0.
//
// TRANSLATION MATRIX =  1     0     0     0
//                       0     1     0     0
//                       0     0     1     0
//                       Tx    Ty    Tz    1
//
// SCALE MATRIX =        Sx    0     0     0
//                       0     Sy    0     0
//                       0     0     Sz    0
//                       0     0     0     1
//
// Rotation about x axis i degrees:
// ROTX(i) =             1     0     0     0
//                       0   cosi  sini    0
//                       0  -sini  cosi    0
//                       0     0     0     1
//
// Rotation about y axis i degrees:
// ROTY(i) =           cosi    0  -sini    0
//                       0     1     0     0
//                     sini    0   cosi    0
//                       0     0     0     1
//
// Rotation about z axis i degrees:
// ROTZ(i) =           cosi  sini    0     0
//                    -sini  cosi    0     0
//                       0     0     1     0
//                       0     0     0     1
//
#include "3d.h"

#include "fixed_pt.h"
#include "fractals.h"
#include "line3d.h"

#include <cfloat>
#include <cmath>
#include <cstring>

// '3d=nn/nn/nn/...' values
bool g_sphere{}; // sphere? 1 = yes, 0 = no
int g_x_rot{};   // rotate x-axis 60 degrees
int g_y_rot{};   // rotate y-axis 90 degrees
int g_z_rot{};   // rotate x-axis  0 degrees
int g_x_scale{}; // scale x-axis, 90 percent
int g_y_scale{}; // scale y-axis, 90 percent
// sphere 3D
int g_sphere_phi_min{};   // longitude start, 180
int g_sphere_phi_max{};   // longitude end ,   0
int g_sphere_theta_min{}; // latitude start,-90 degrees
int g_sphere_theta_max{}; // latitude stop,  90 degrees
int g_sphere_radius{};    // should be user input
// common parameters
int g_rough{};           // scale z-axis, 30 percent
int g_water_line{};      // water level
FillType g_fill_type{};  // fill type
int g_viewer_z{};        // perspective view point
int g_shift_x{};         // x shift
int g_shift_y{};         // y shift
int g_light_x{};         // x light vector coordinate
int g_light_y{};         // y light vector coordinate
int g_light_z{};         // z light vector coordinate
int g_light_avg{};       // number of points to average

// initialize a matrix and set to identity matrix (all 0's, 1's on diagonal)
void identity(MATRIX m)
{
    for (int i = 0 ; i < CMAX; i++)
    {
        for (int j = 0; j < RMAX; j++)
        {
            if (i == j)
            {
                m[j][i] = 1.0;
            }
            else
            {
                m[j][i] = 0.0;
            }
        }
    }
}

// Multiply two matrices
void mat_mul(MATRIX lhs, MATRIX rhs, MATRIX result)
{
    // result stored in product to avoid problems
    //  in case parameter mat3 == mat2 or mat 1
    MATRIX product;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            product[j][i] =             //
                lhs[j][0] * rhs[0][i] + //
                lhs[j][1] * rhs[1][i] + //
                lhs[j][2] * rhs[2][i] + //
                lhs[j][3] * rhs[3][i];
        }
    }
    std::memcpy(result, product, sizeof(product));
}

// multiply a matrix by a scalar
void scale(double sx, double sy, double sz, MATRIX m)
{
    MATRIX scale;
    identity(scale);
    scale[0][0] = sx;
    scale[1][1] = sy;
    scale[2][2] = sz;
    mat_mul(m, scale, m);
}

// rotate about X axis
void x_rot(double theta, MATRIX m)
{
    MATRIX rot;
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    identity(rot);
    rot[1][1] = cos_theta;
    rot[1][2] = -sin_theta;
    rot[2][1] = sin_theta;
    rot[2][2] = cos_theta;
    mat_mul(m, rot, m);
}

// rotate about Y axis
void y_rot(double theta, MATRIX m)
{
    MATRIX rot;
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    identity(rot);
    rot[0][0] = cos_theta;
    rot[0][2] = sin_theta;
    rot[2][0] = -sin_theta;
    rot[2][2] = cos_theta;
    mat_mul(m, rot, m);
}

// rotate about Z axis
void z_rot(double theta, MATRIX m)
{
    MATRIX rot;
    double sin_theta = std::sin(theta);
    double cos_theta = std::cos(theta);
    identity(rot);
    rot[0][0] = cos_theta;
    rot[0][1] = -sin_theta;
    rot[1][0] = sin_theta;
    rot[1][1] = cos_theta;
    mat_mul(m, rot, m);
}

// translate
void trans(double tx, double ty, double tz, MATRIX m)
{
    MATRIX trans;
    identity(trans);
    trans[3][0] = tx;
    trans[3][1] = ty;
    trans[3][2] = tz;
    mat_mul(m, trans, m);
}

// cross product  - useful because cross is perpendicular to v and w
int cross_product(VECTOR v, VECTOR w, VECTOR cross)
{
    VECTOR tmp;
    tmp[0] =  v[1]*w[2] - w[1]*v[2];
    tmp[1] =  w[0]*v[2] - v[0]*w[2];
    tmp[2] =  v[0]*w[1] - w[0]*v[1];
    cross[0] = tmp[0];
    cross[1] = tmp[1];
    cross[2] = tmp[2];
    return 0;
}

// normalize a vector to length 1
bool normalize_vector(VECTOR v)
{
    double length = dot_product(v, v);

    // bailout if zero length
    if (length < FLT_MIN || length > FLT_MAX)
    {
        return true;
    }
    length = std::sqrt(length);
    if (length < FLT_MIN)
    {
        return true;
    }

    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
    return false;
}

// multiply source vector s by matrix m, result in target t
// used to apply transformations to a vector
int vec_mat_mul(VECTOR s, MATRIX m, VECTOR t)
{
    VECTOR tmp;
    for (int j = 0; j < CMAX-1; j++)
    {
        tmp[j] = 0.0;
        for (int i = 0; i < RMAX-1; i++)
        {
            tmp[j] += s[i]*m[i][j];
        }
        // vector is really four dimensional with last component always 1
        tmp[j] += m[3][j];
    }
    // set target = tmp. Necessary to use tmp in case source = target
    std::memcpy(t, tmp, sizeof(tmp));
    return 0;
}

// multiply vector s by matrix m, result in s
// use with a function pointer in line3d.c
// must coordinate calling conventions with
// mult_vec in general.asm
void vec_g_mat_mul(VECTOR s)
{
    VECTOR tmp;
    for (int j = 0; j < CMAX-1; j++)
    {
        tmp[j] = 0.0;
        for (int i = 0; i < RMAX-1; i++)
        {
            tmp[j] += s[i]*g_m[i][j];
        }
        // vector is really four dimensional with last component always 1
        tmp[j] += g_m[3][j];
    }
    // set target = tmp. Necessary to use tmp in case source = target
    std::memcpy(s, tmp, sizeof(tmp));
}

// perspective projection of vector v with respect to viewpoint vector view
int perspective(VECTOR v)
{
    double denom = g_view[2] - v[2];

    if (denom >= 0.0)
    {
        v[0] = g_bad_value;   // clipping will catch these values
        v[1] = g_bad_value;   // so they won't plot values BEHIND viewer
        v[2] = g_bad_value;
        return -1;
    }
    v[0] = (v[0]*g_view[2] - g_view[0]*v[2])/denom;
    v[1] = (v[1]*g_view[2] - g_view[1]*v[2])/denom;

    // calculation of z if needed later
    // v[2] =  v[2]/denom;
    return 0;
}

// long version of vmult and perspective combined for speed
int long_vec_mat_mul_persp(LVECTOR s, LMATRIX m, LVECTOR t0, LVECTOR t, LVECTOR view, int bit_shift)
{
    // s: source vector
    // m: transformation matrix
    // t0: after transformation, before persp
    // t: target vector
    // lview: perspective viewer coordinates
    // bitshift: fixed point conversion bitshift
    LVECTOR tmp;
    g_overflow = false;
    int k = CMAX - 1;                  // shorten the math if non-perspective and non-illum
    if (view[2] == 0 && t0[0] == 0)
    {
        k--;
    }

    for (int j = 0; j < k; j++)
    {
        tmp[j] = 0;
        for (int i = 0; i < RMAX-1; i++)
        {
            tmp[j] += multiply(s[i], m[i][j], bit_shift);
        }
        // vector is really four dimensional with last component always 1
        tmp[j] += m[3][j];
    }
    if (t0[0]) // first component of  t0 used as flag
    {
        // faster than for loop, if less general
        t0[0] = tmp[0];
        t0[1] = tmp[1];
        t0[2] = tmp[2];
    }
    if (view[2] != 0)           // perspective 3D
    {
        long denom = view[2] - tmp[2];
        if (denom >= 0)           // bail out if point is "behind" us
        {
            t[0] = g_bad_value;
            t[0] = t[0] << bit_shift;
            t[1] = t[0];
            t[2] = t[0];
            return -1;
        }

        // doing math in this order helps prevent overflow
        LVECTOR tmp_view;
        tmp_view[0] = divide(view[0], denom, bit_shift);
        tmp_view[1] = divide(view[1], denom, bit_shift);
        tmp_view[2] = divide(view[2], denom, bit_shift);

        tmp[0] = multiply(tmp[0], tmp_view[2], bit_shift) -
                 multiply(tmp_view[0], tmp[2], bit_shift);

        tmp[1] = multiply(tmp[1], tmp_view[2], bit_shift) -
                 multiply(tmp_view[1], tmp[2], bit_shift);
    }

    // set target = tmp. Necessary to use tmp in case source = target
    // faster than for loop, if less general
    t[0] = tmp[0];
    t[1] = tmp[1];
    t[2] = tmp[2];
    return g_overflow ? 1 : 0;
}

// Long version of perspective. Because of use of fixed point math, there
// is danger of overflow and underflow
int long_persp(LVECTOR lv, LVECTOR lview, int bit_shift)
{
    g_overflow = false;
    long denom = lview[2] - lv[2];
    if (denom >= 0)              // bail out if point is "behind" us
    {
        lv[0] = g_bad_value;
        lv[0] = lv[0] << bit_shift;
        lv[1] = lv[0];
        lv[2] = lv[0];
        return -1;
    }

    // doing math in this order helps prevent overflow
    LVECTOR tmp_view;
    tmp_view[0] = divide(lview[0], denom, bit_shift);
    tmp_view[1] = divide(lview[1], denom, bit_shift);
    tmp_view[2] = divide(lview[2], denom, bit_shift);

    lv[0] = multiply(lv[0], tmp_view[2], bit_shift) -
            multiply(tmp_view[0], lv[2], bit_shift);

    lv[1] = multiply(lv[1], tmp_view[2], bit_shift) -
            multiply(tmp_view[1], lv[2], bit_shift);

    return g_overflow ? 1 : 0;
}

int long_vec_mat_mul(LVECTOR s, LMATRIX m, LVECTOR t, int bit_shift)
{
    LVECTOR tmp;
    g_overflow = false;
    int k = CMAX - 1;

    for (int j = 0; j < k; j++)
    {
        tmp[j] = 0;
        for (int i = 0; i < RMAX-1; i++)
        {
            tmp[j] += multiply(s[i], m[i][j], bit_shift);
        }
        // vector is really four dimensional with last component always 1
        tmp[j] += m[3][j];
    }

    // set target = tmp. Necessary to use tmp in case source = target
    // faster than for loop, if less general
    t[0] = tmp[0];
    t[1] = tmp[1];
    t[2] = tmp[2];
    return g_overflow ? 1 : 0;
}
