#pragma once

enum
{
    CMAX = 4, // maximum column (4 x 4 matrix)
    RMAX = 4   // maximum row    (4 x 4 matrix)
};
typedef double MATRIX [RMAX] [CMAX];  // matrix of doubles
typedef int   IMATRIX [RMAX] [CMAX];  // matrix of ints
typedef long  LMATRIX [RMAX] [CMAX];  // matrix of longs
/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */
typedef double VECTOR [3];  // vector of doubles
typedef int   IVECTOR [3];  // vector of ints
typedef long  LVECTOR [3];  // vector of longs
/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */

inline double dot_product(VECTOR v1, VECTOR v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

enum class fill_type
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
constexpr int operator+(fill_type value)
{
    return static_cast<int>(value);
}

extern int g_init_3d[20];

// regular 3D
extern int &g_sphere; // sphere? 1 = yes, 0 = no
extern int &g_x_rot;  // rotate x-axis 60 degrees
extern int &g_y_rot;  // rotate y-axis 90 degrees
extern int &g_z_rot;  // rotate x-axis  0 degrees
extern int &XSCALE;   // scale x-axis, 90 percent
extern int &YSCALE;   // scale y-axis, 90 percent
// sphere 3D
extern int &PHI1;   // longitude start, 180
extern int &PHI2;   // longitude end ,   0
extern int &THETA1; // latitude start,-90 degrees
extern int &THETA2; // latitude stop,  90 degrees
extern int &RADIUS; // should be user input
// common parameters
extern int &ROUGH;     // scale z-axis, 30 percent
extern int &WATERLINE; // water level
extern int &FILLTYPE;  // fill type
extern int &ZVIEWER;   // perspective view point
extern int &XSHIFT;    // x shift
extern int &YSHIFT;    // y shift
extern int &XLIGHT;    // x light vector coordinate
extern int &YLIGHT;    // y light vector coordinate
extern int &ZLIGHT;    // z light vector coordinate
extern int &LIGHTAVG;  // number of points to average

#define ILLUMINE (FILLTYPE > +fill_type::SOLID_FILL) // illumination model

void identity(MATRIX);
void mat_mul(MATRIX, MATRIX, MATRIX);
void scale(double, double, double, MATRIX);
void xrot(double, MATRIX);
void yrot(double, MATRIX);
void zrot(double, MATRIX);
void trans(double, double, double, MATRIX);
int cross_product(VECTOR, VECTOR, VECTOR);
bool normalize_vector(VECTOR);
int vmult(VECTOR, MATRIX, VECTOR);
void mult_vec(VECTOR);
int perspective(VECTOR);
int longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
int longpersp(LVECTOR, LVECTOR, int);
int longvmult(LVECTOR, LMATRIX, LVECTOR, int);
