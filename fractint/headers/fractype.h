#ifndef FRACTYPE_H
#define FRACTYPE_H

// these are indices into prompts1.cpp:function_list
enum FunctionType
{
	FUNCTION_SIN	= 0,
	FUNCTION_COSXX	= 1,
	FUNCTION_SINH	= 2,
	FUNCTION_COSH	= 3,
	FUNCTION_EXP	= 4,
	FUNCTION_LOG	= 5,
	FUNCTION_SQR	= 6,
	FUNCTION_RECIP	= 7,
	FUNCTION_IDENT	= 8,
	FUNCTION_COS	= 9,
	FUNCTION_TAN	= 10,
	FUNCTION_TANH	= 11,
	FUNCTION_COTAN	= 12,
	FUNCTION_COTANH	= 13,
	FUNCTION_FLIP	= 14,
	FUNCTION_CONJ	= 15,
	FUNCTION_ZERO	= 16,
	FUNCTION_ASIN	= 17,
	FUNCTION_ASINH	= 18,
	FUNCTION_ACOS	= 19,
	FUNCTION_ACOSH	= 20,
	FUNCTION_ATAN	= 21,
	FUNCTION_ATANH	= 22,
	FUNCTION_CABS	= 23,
	FUNCTION_ABS	= 24,
	FUNCTION_SQRT	= 25,
	FUNCTION_FLOOR	= 26,
	FUNCTION_CEIL	= 27,
	FUNCTION_TRUNC	= 28,
	FUNCTION_ROUND	= 29,
	FUNCTION_ONE	= 30
};

// These MUST match the corresponding g_fractal_specific record in fractals.c
enum FractalType
{
	FRACTYPE_NO_FRACTAL = -1,
	FRACTYPE_MANDELBROT							= 0,
	FRACTYPE_JULIA								= 1,
	FRACTYPE_NEWTON_BASIN						= 2,
	FRACTYPE_LAMBDA								= 3,
	FRACTYPE_MANDELBROT_FP						= 4,
	FRACTYPE_NEWTON								= 5,
	FRACTYPE_JULIA_FP							= 6,
	FRACTYPE_PLASMA								= 7,
	FRACTYPE_MANDELBROT_FUNC_FP					= 8,
	FRACTYPE_MAN_O_WAR_FP						= 9,
	FRACTYPE_MAN_O_WAR							= 10,
	FRACTYPE_TEST								= 11,
	FRACTYPE_SIERPINSKI							= 12,
	FRACTYPE_BARNSLEY_M1						= 13,
	FRACTYPE_BARNSLEY_J1						= 14,
	FRACTYPE_BARNSLEY_M2						= 15,
	FRACTYPE_BARNSLEY_J2						= 16,
	FRACTYPE_SQR_FUNC							= 17,
	FRACTYPE_SQR_FUNC_FP						= 18,
	FRACTYPE_FUNC_PLUS_FUNC						= 19,
	FRACTYPE_MANDELBROT_LAMBDA					= 20,
	FRACTYPE_MARKS_MANDELBROT					= 21,
	FRACTYPE_MARKS_JULIA						= 22,
	FRACTYPE_UNITY								= 23,
	FRACTYPE_MANDELBROT_4						= 24,
	FRACTYPE_JULIA_4							= 25,
	FRACTYPE_IFS								= 26,
	FRACTYPE_IFS_3D								= 27,
	FRACTYPE_BARNSLEY_M3						= 28,
	FRACTYPE_BARNSLEY_J3						= 29,
	FRACTYPE_FUNC_SQR							= 30,
	FRACTYPE_FUNC_SQR_FP						= 31,
	FRACTYPE_BIFURCATION						= 32,
	FRACTYPE_FUNC_PLUS_FUNC_FP					= 33,
	FRACTYPE_FUNC_TIMES_FUNC					= 34,
	FRACTYPE_FUNC_TIMES_FUNC_FP					= 35,
	FRACTYPE_SQR_RECIPROCAL_FUNC				= 36,
	FRACTYPE_SQR_RECIPROCAL_FUNC_FP				= 37,
	FRACTYPE_FUNC_TIMES_Z_PLUS_Z				= 38,
	FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP				= 39,
	FRACTYPE_KAM_TORUS_FP						= 40,
	FRACTYPE_KAM_TORUS							= 41,
	FRACTYPE_KAM_TORUS_3D_FP					= 42,
	FRACTYPE_KAM_TORUS_3D						= 43,
	FRACTYPE_LAMBDA_FUNC						= 44,
	FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L	= 45,
	FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L		= 46,
	FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP	= 47,
	FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP		= 48,
	FRACTYPE_LAMBDA_FUNC_FP						= 49,
	FRACTYPE_MANDELBROT_FUNC					= 50,
	FRACTYPE_MANDELBROT_Z_POWER_L				= 51,
	FRACTYPE_JULIA_Z_POWER_L					= 52,
	FRACTYPE_MANDELBROT_Z_POWER_FP				= 53,
	FRACTYPE_JULIA_Z_POWER_FP					= 54,
	FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP	= 55,
	FRACTYPE_JULIA_Z_TO_Z_PLUS_Z_POWER_FP		= 56,
	FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L			= 57,
	FRACTYPE_JULIA_FUNC_PLUS_EXP_L				= 58,
	FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP		= 59,
	FRACTYPE_JULIA_FUNC_PLUS_EXP_FP				= 60,
	FRACTYPE_POPCORN_FP							= 61,
	FRACTYPE_POPCORN_L							= 62,
	FRACTYPE_LORENZ_FP							= 63,
	FRACTYPE_LORENZ_L							= 64,
	FRACTYPE_LORENZ_3D_L						= 65,
	FRACTYPE_NEWTON_MP							= 66,
	FRACTYPE_NEWTON_BASIN_MP					= 67,
	FRACTYPE_NEWTON_COMPLEX						= 68,
	FRACTYPE_NEWTON_BASIN_COMPLEX				= 69,
	FRACTYPE_MARKS_MANDELBROT_COMPLEX			= 70,
	FRACTYPE_MARKS_JULIA_COMPLEX				= 71,
	FRACTYPE_FORMULA							= 72,
	FRACTYPE_FORMULA_FP							= 73,
	FRACTYPE_SIERPINSKI_FP						= 74,
	FRACTYPE_LAMBDA_FP							= 75,
	FRACTYPE_BARNSLEY_M1_FP						= 76,
	FRACTYPE_BARNSLEY_J1_FP						= 77,
	FRACTYPE_BARNSLEY_M2_FP						= 78,
	FRACTYPE_BARNSLEY_J2_FP						= 79,
	FRACTYPE_BARNSLEY_M3_FP						= 80,
	FRACTYPE_BARNSLEY_J3_FP						= 81,
	FRACTYPE_MANDELBROT_LAMBDA_FP				= 82,
	FRACTYPE_JULIBROT							= 83,
	FRACTYPE_LORENZ_3D_FP						= 84,
	FRACTYPE_ROSSLER_L							= 85,
	FRACTYPE_ROSSLER_FP							= 86,
	FRACTYPE_HENON_L							= 87,
	FRACTYPE_HENON_FP							= 88,
	FRACTYPE_PICKOVER_FP						= 89,
	FRACTYPE_GINGERBREAD_FP						= 90,
	FRACTYPE_DIFFUSION							= 91,
	FRACTYPE_UNITY_FP							= 92,
	FRACTYPE_SPIDER_FP							= 93,
	FRACTYPE_SPIDER								= 94,
	FRACTYPE_TETRATE_FP							= 95,
	FRACTYPE_MAGNET_1M							= 96,
	FRACTYPE_MAGNET_1J							= 97,
	FRACTYPE_MAGNET_2M							= 98,
	FRACTYPE_MAGNET_2J							= 99,
	FRACTYPE_BIFURCATION_L						= 100,
	FRACTYPE_BIFURCATION_LAMBDA_L				= 101,
	FRACTYPE_BIFURCATION_LAMBDA					= 102,
	FRACTYPE_BIFURCATION_PLUS_FUNC_PI			= 103,
	FRACTYPE_BIFURCATION_EQUAL_FUNC_PI			= 104,
	FRACTYPE_POPCORN_JULIA_FP					= 105,
	FRACTYPE_POPCORN_JULIA_L					= 106,
	FRACTYPE_L_SYSTEM							= 107,
	FRACTYPE_MAN_O_WAR_JULIA_FP					= 108,
	FRACTYPE_MAN_O_WAR_JULIA					= 109,
	FRACTYPE_FUNC_PLUS_FUNC_PIXEL_FP			= 110,
	FRACTYPE_FUNC_PLUS_FUNC_PIXEL_L				= 111,
	FRACTYPE_MARKS_MANDELBROT_POWER_FP			= 112,
	FRACTYPE_MARKS_MANDELBROT_POWER				= 113,
	FRACTYPE_TIMS_ERROR_FP						= 114,
	FRACTYPE_TIMS_ERROR							= 115,
	FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L		= 116,
	FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L			= 117,
	FRACTYPE_BIFURCATION_STEWART				= 118,
	FRACTYPE_BIFURCATION_STEWART_L				= 119,
	FRACTYPE_HOPALONG_FP						= 120,
	FRACTYPE_CIRCLE_FP							= 121,
	FRACTYPE_MARTIN_FP							= 122,
	FRACTYPE_LYAPUNOV							= 123,
	FRACTYPE_LORENZ_3D_1_FP						= 124,
	FRACTYPE_LORENZ_3D_3_FP						= 125,
	FRACTYPE_LORENZ_3D_4_FP						= 126,
	FRACTYPE_LAMBDA_FUNC_OR_FUNC_L				= 127,
	FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP				= 128,
	FRACTYPE_JULIA_FUNC_OR_FUNC_L				= 129,
	FRACTYPE_JULIA_FUNC_OR_FUNC_FP				= 130,
	FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L	= 131,
	FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP	= 132,
	FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L			= 133,
	FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP			= 134,
	FRACTYPE_BIFURCATION_MAY_L					= 135,
	FRACTYPE_BIFURCATION_MAY					= 136,
	FRACTYPE_HALLEY_MP							= 137,
	FRACTYPE_HALLEY								= 138,
	FRACTYPE_DYNAMIC_FP							= 139,
	FRACTYPE_QUATERNION_FP						= 140,
	FRACTYPE_QUATERNION_JULIA_FP				= 141,
	FRACTYPE_CELLULAR							= 142,
	FRACTYPE_JULIBROT_FP						= 143,
	FRACTYPE_INVERSE_JULIA						= 144,
	FRACTYPE_INVERSE_JULIA_FP					= 145,
	FRACTYPE_MANDELBROT_CLOUD					= 146,
	FRACTYPE_PHOENIX							= 147,
	FRACTYPE_PHOENIX_FP							= 148,
	FRACTYPE_MANDELBROT_PHOENIX					= 149,
	FRACTYPE_MANDELBROT_PHOENIX_FP				= 150,
	FRACTYPE_HYPERCOMPLEX_FP					= 151,
	FRACTYPE_HYPERCOMPLEX_JULIA_FP				= 152,
	FRACTYPE_FROTHY_BASIN						= 153,
	FRACTYPE_FROTHY_BASIN_FP					= 154,
	FRACTYPE_MANDELBROT_4_FP					= 155,
	FRACTYPE_JULIA_4_FP							= 156,
	FRACTYPE_MARKS_MANDELBROT_FP				= 157,
	FRACTYPE_MARKS_JULIA_FP						= 158,
	FRACTYPE_ICON								= 159,
	FRACTYPE_ICON_3D							= 160,
	FRACTYPE_PHOENIX_COMPLEX					= 161,
	FRACTYPE_PHOENIX_COMPLEX_FP					= 162,
	FRACTYPE_MANDELBROT_PHOENIX_COMPLEX			= 163,
	FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP		= 164,
	FRACTYPE_ANT								= 165,
	FRACTYPE_CHIP								= 166,
	FRACTYPE_QUADRUP_TWO						= 167,
	FRACTYPE_THREE_PLY							= 168,
	FRACTYPE_VOLTERRA_LOTKA						= 169,
	FRACTYPE_ESCHER_JULIA						= 170,
	FRACTYPE_LATOOCARFIAN						= 171,
	FRACTYPE_MANDELBROT_MIX4					= 172,

	FRACTYPE_OBSOLETE_LAMBDA_SINE				= 8,
	FRACTYPE_OBSOLETE_LAMBDA_COS				= 9,
	FRACTYPE_OBSOLETE_LAMBDA_EXP				= 10,
	FRACTYPE_OBSOLETE_MANDELBROT_SINE			= 17,
	FRACTYPE_OBSOLETE_MANDELBROT_COS			= 18,
	FRACTYPE_OBSOLETE_MANDELBROT_EXP			= 19,
	FRACTYPE_OBSOLETE_DEM_MANDELBROT			= 30,
	FRACTYPE_OBSOLETE_DEM_JULIA					= 31,
	FRACTYPE_OBSOLETE_MANDELBROT_SINH			= 33,
	FRACTYPE_OBSOLETE_LAMBDA_SINH				= 34,
	FRACTYPE_OBSOLETE_MANDELBROT_COSH			= 35,
	FRACTYPE_OBSOLETE_LAMBDA_COSH				= 36,
	FRACTYPE_OBSOLETE_MANDELBROT_SINE_L			= 37,
	FRACTYPE_OBSOLETE_LAMBDA_SINE_L				= 38,
	FRACTYPE_OBSOLETE_MANDELBROT_COS_L			= 39,
	FRACTYPE_OBSOLETE_LAMBDA_COS_L				= 40,
	FRACTYPE_OBSOLETE_MANDELBROT_SINH_L			= 41,
	FRACTYPE_OBSOLETE_LAMBDA_SINH_L				= 42,
	FRACTYPE_OBSOLETE_MANDELBROT_COSH_L			= 43,
	FRACTYPE_OBSOLETE_LAMBDA_COSH_L				= 44,
	FRACTYPE_OBSOLETE_MANDELBROT_EXP_L			= 49,
	FRACTYPE_OBSOLETE_LAMBDA_EXP_L				= 50
};

extern bool fractal_type_formula(int fractal_type);
extern bool fractal_type_julia(int fractal_type);
extern bool fractal_type_inverse_julia(int fractal_type);
extern bool fractal_type_julia_or_inverse(int fractal_type);
extern bool fractal_type_mandelbrot(int fractal_type);
extern bool fractal_type_ifs(int fractal_type);
extern bool fractal_type_none(int fractal_type);
extern bool fractal_type_ant_or_cellular(int fractal_type);
extern bool fractal_type_julibrot(int fractal_type);
extern bool inside_coloring_beauty_of_fractals();
extern bool inside_coloring_beauty_of_fractals_allowed();

#endif
