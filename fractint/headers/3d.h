#if !defined(THREED_H)
#define THREED_H

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
extern int longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
extern int longpersp(LVECTOR, LVECTOR, int);
extern int longvmult(LVECTOR, LMATRIX, LVECTOR, int);

#endif
