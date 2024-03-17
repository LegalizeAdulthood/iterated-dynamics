#pragma once

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
