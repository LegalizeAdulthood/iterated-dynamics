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

#define SPHERE    g_init_3d[0]     // sphere? 1 = yes, 0 = no
#define ILLUMINE (FILLTYPE > +fill_type::SOLID_FILL) // illumination model
// regular 3D
#define XROT      g_init_3d[1]     // rotate x-axis 60 degrees
#define YROT      g_init_3d[2]     // rotate y-axis 90 degrees
#define ZROT      g_init_3d[3]     // rotate x-axis  0 degrees
#define XSCALE    g_init_3d[4]     // scale x-axis, 90 percent
#define YSCALE    g_init_3d[5]     // scale y-axis, 90 percent
// sphere 3D
#define PHI1      g_init_3d[1]     // longitude start, 180
#define PHI2      g_init_3d[2]     // longitude end ,   0
#define THETA1    g_init_3d[3]     // latitude start,-90 degrees
#define THETA2    g_init_3d[4]     // latitude stop,  90 degrees
#define RADIUS    g_init_3d[5]     // should be user input
// common parameters
#define ROUGH     g_init_3d[6]     // scale z-axis, 30 percent
#define WATERLINE g_init_3d[7]     // water level
#define FILLTYPE  g_init_3d[8]     // fill type
#define ZVIEWER   g_init_3d[9]     // perspective view point
#define XSHIFT    g_init_3d[10]    // x shift
#define YSHIFT    g_init_3d[11]    // y shift
#define XLIGHT    g_init_3d[12]    // x light vector coordinate
#define YLIGHT    g_init_3d[13]    // y light vector coordinate
#define ZLIGHT    g_init_3d[14]    // z light vector coordinate
#define LIGHTAVG  g_init_3d[15]    // number of points to average

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
