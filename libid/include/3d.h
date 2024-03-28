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

extern int                   g_init_3d[20];

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
