#if !defined(THREED_H)
#define THREED_H

enum
{
	CMAX = 4,				// maximum column (4 x 4 matrix)
	RMAX = 4,				// maximum row    (4 x 4 matrix)
	DIM = 3					// number of dimensions
};

// TODO: use a template type for different kinds of matrices
typedef double MATRIX[RMAX][CMAX];  // matrix of doubles
typedef long  MATRIX_L[RMAX][CMAX];  // matrix of longs

/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */

// TODO: use a template type for different kinds of vectors
typedef double VECTOR[DIM];  // vector of doubles
typedef long  VECTOR_L[DIM];  // vector of longs

/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */

inline double DOT_PRODUCT(VECTOR const v1,VECTOR const v2)
{
	return ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2]);
}

extern void identity(MATRIX);
extern void scale(double, double, double, MATRIX);
extern void xrot(double, MATRIX);
extern void yrot(double, MATRIX);
extern void zrot(double, MATRIX);
extern void trans(double, double, double, MATRIX);
extern int cross_product(VECTOR, VECTOR, VECTOR);
extern int normalize_vector(VECTOR);
extern int vmult(VECTOR, MATRIX, VECTOR);
extern void mult_vec(VECTOR, MATRIX);
extern int perspective(VECTOR);
extern int vmult_perspective_l(VECTOR_L, MATRIX_L, VECTOR_L, VECTOR_L, VECTOR_L, int);
extern int longpersp(VECTOR_L, VECTOR_L, int);
extern int longvmult(VECTOR_L, MATRIX_L, VECTOR_L, int);

#endif
