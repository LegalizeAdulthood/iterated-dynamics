#include <cassert>
#include <string>

#include "id.h"

#include "ant.h"
#include "calcfrac.h"
#include "Diffusion.h"
#include "fractals.h"
#include "fractype.h"
#include "frasetup.h"
#include "FrothyBasin.h"
#include "Halley.h"
#include "helpdefs.h"
#include "jb.h"
#include "lorenz.h"
#include "lsys.h"
#include "miscfrac.h"
#include "mpmath.h"
#include "Newton.h"
#include "Plasma.h"
#include "QuaternionEngine.h"
#include "Test.h"

// for Mandelbrots
static char s_real_z0[] = "Real Perturbation of Z(0)";
static char s_imag_z0[] = "Imaginary Perturbation of Z(0)";

// for Julias
static char s_parameter_real[] = "Real Part of Parameter";
static char s_parameter_imag[] = "Imaginary Part of Parameter";

// for Newtons
static char s_newton_degree[] = "+Polynomial Degree (>= 2)";

// for MarksMandel/Julia
static char s_exponent[] = "Real part of Exponent";
static char s_exponent_imag[] = "Imag part of Exponent";

// for Lorenz
static char s_time_step[] = "Time Step";

// for formula
static char s_p1_real[] = "Real portion of p1";
static char s_p2_real[] = "Real portion of p2";
static char s_p3_real[] = "Real portion of p3";
static char s_p4_real[] = "Real portion of p4";
static char s_p5_real[] = "Real portion of p5";
static char s_p1_imag[] = "Imaginary portion of p1";
static char s_p2_imag[] = "Imaginary portion of p2";
static char s_p3_imag[] = "Imaginary portion of p3";
static char s_p4_imag[] = "Imaginary portion of p4";
static char s_p5_imag[] = "Imaginary portion of p5";

// trig functions
static char s_trig1_coefficient_re[] = "Real Coefficient First Function";
static char s_trig1_coefficient_im[] = "Imag Coefficient First Function";
static char s_trig2_coefficient_re[] = "Real Coefficient Second Function";
static char s_trig2_coefficient_im[] = "Imag Coefficient Second Function";

// KAM Torus
static char s_kam_angle[] = "Angle (radians)";
static char s_kam_step[] =  "Step size";
static char s_kam_stop[] =  "Stop value";
static char s_points_per_orbit[] = "+Points per orbit";

// popcorn and julia popcorn generalized
static char s_step_x[] = "Step size (real)";
static char s_step_y[] = "Step size (imaginary)";
static char s_constant_x[] = "Constant C (real)";
static char s_constant_y[] = "Constant C (imaginary)";

// bifurcations
static char s_filter_cycles[] = "+Filter Cycles";
static char s_seed_population[] = "Seed Population";

// frothy basins
static char s_frothy_mapping[] = "+Apply mapping once (1) or twice (2)";
static char s_frothy_shade[] =  "+Enter non-zero value for alternate color shading";
static char s_frothy_a_value[] =  "A (imaginary part of C)";

// plasma and ant

static char s_random_seed[] = "+Random Seed Value (0 = Random, 1 = Reuse Last)";

// ifs
static char s_color_method[] = "+Coloring method (0,1)";

// phoenix fractals
static char s_degree_z[] = "Degree = 0 | >= 2 | <= -3";

// julia inverse
static char s_max_hits_per_pixel[] = "Max Hits per Pixel";

// halley
static char s_order[] = {"+Order (integer > 1)"};
static char s_real_relaxation_coefficient[] = {"Real Relaxation coefficient"};
static char s_epsilon[] = {"Epsilon"};
static char s_imag_relaxation_coefficient[] = {"Imag Relaxation coefficient"};

static char s_barnsleyj1_name[] = "*barnsleyj1";
static char s_barnsleyj2_name[] = "*barnsleyj2";
static char s_barnsleyj3_name[] = "*barnsleyj3";
static char s_barnsleym1_name[] = "*barnsleym1";
static char s_barnsleym2_name[] = "*barnsleym2";
static char s_barnsleym3_name[] = "*barnsleym3";
static char s_bifplussinpi_name[] = "*bif+sinpi";
static char s_bifeqsinpi_name[] = "*bif=sinpi";
static char s_biflambda_name[] = "*biflambda";
static char s_bifmay_name[] = "*bifmay";
static char s_bifstewart_name[] = "*bifstewart";
static char s_bifurcation_name[] = "*bifurcation";
static char s_fn_z_plusfn_pix__name[] = "*fn(z)+fn(pix)";
static char s_fn_zz__name[] = "*fn(z*z)";
static char s_fnfn_name[] = "*fn*fn";
static char s_fnzplusz_name[] = "*fn*z+z";
static char s_fnplusfn_name[] = "*fn+fn";
static char s_formula_name[] = "*formula";
static char s_henon_name[] = "*henon";
static char s_ifs3d_name[] = "*ifs3d";
static char s_julfnplusexp_name[] = "*julfn+exp";
static char s_julfnpluszsqrd_name[] = "*julfn+zsqrd";
static char s_julia_name[] = "*julia";
static char s_julia_fnorfn__name[] = "*julia(fn||fn)";
static char s_julia4_name[] = "*julia4";
static char s_julia_inverse_name[] = "*julia_inverse";
static char s_julibrot_name[] = "*julibrot";
static char s_julzpower_name[] = "*julzpower";
static char s_kamtorus_name[] = "*kamtorus";
static char s_kamtorus3d_name[] = "*kamtorus3d";
static char s_lambda_name[] = "*lambda";
static char s_lambda_fnorfn__name[] = "*lambda(fn||fn)";
static char s_lambdafn_name[] = "*lambdafn";
static char s_lorenz_name[] = "*lorenz";
static char s_lorenz3d_name[] = "*lorenz3d";
static char s_mandel_name[] = "*mandel";
static char s_mandel_fnorfn__name[] = "*mandel(fn||fn)";
static char s_mandel4_name[] = "*mandel4";
static char s_mandelfn_name[] = "*mandelfn";
static char s_mandellambda_name[] = "*mandellambda";
static char s_mandphoenix_name[] = "*mandphoenix";
static char s_mandphoenixcplx_name[] = "*mandphoenixclx";
static char s_manfnplusexp_name[] = "*manfn+exp";
static char s_manfnpluszsqrd_name[] = "*manfn+zsqrd";
static char s_manlam_fnorfn__name[] = "*manlam(fn||fn)";
static char s_manowar_name[] = "*manowar";
static char s_manowarj_name[] = "*manowarj";
static char s_manzpower_name[] = "*manzpower";
static char s_marksjulia_name[] = "*marksjulia";
static char s_marksmandel_name[] = "*marksmandel";
static char s_marksmandelpwr_name[] = "*marksmandelpwr";
static char s_newtbasin_name[] = "*newtbasin";
static char s_newton_name[] = "*newton";
static char s_phoenix_name[] = "*phoenix";
static char s_phoenixcplx_name[] = "*phoenixcplx";
static char s_popcorn_name[] = "*popcorn";
static char s_popcornjul_name[] = "*popcornjul";
static char s_rossler3d_name[] = "*rossler3d";
static char s_sierpinski_name[] = "*sierpinski";
static char s_spider_name[] = "*spider";
static char s_sqr_1divfn__name[] = "*sqr(1/fn)";
static char s_sqr_fn__name[] = "*sqr(fn)";
static char s_tims_error_name[] = "*tim's_error";
static char s_unity_name[] = "*unity";
static char s_frothybasin_name[] = "*frothybasin";
static char s_halley_name[] = "*halley";

// bailout defines
enum
{
	ORBIT_BAILOUT_TRIG_L		= 64,
	ORBIT_BAILOUT_FROTHY_BASIN	= 7,
	ORBIT_BAILOUT_STANDARD		= 4,
	ORBIT_BAILOUT_NONE			= 0
};

class IFractalCalculator
{
public:
	virtual ~IFractalCalculator() { }

	virtual int Orbit() = 0;
	virtual int PerPixel() = 0;
	virtual bool PerImage() = 0;
	virtual int CalculateType() = 0;
};

class NamedParameterList
{
public:
	NamedParameterList() : _names(), _values() { }
	NamedParameterList &Add(const char *name, double value = 0.0)
	{
		_names.push_back(name);
		_values.push_back(value);
		return *this;
	}
	void Swap(NamedParameterList &rhs)
	{
		_names.swap(rhs._names);
		_values.swap(rhs._values);
	}
public:
	std::vector<std::string> _names;
	std::vector<double> _values;
};

;

class FractalTypeSpecificData2
{
public:
	FractalTypeSpecificData2(int type_, char const *name_,
		NamedParameterList &parameters_,
		int helpText_, int helpFormula_, int flags_,
		float xMin_, float xMax_, float yMin_, float yMax_,
		int isInteger_, int toJulia_, int toMandel_, int toFloat_,
		SymmetryType symmetry_, IFractalCalculator &calculator_,
		int orbitBailOut_)
		: fractal_type(type_),
		name(name_),
		parameters(),
		helptext(helpText_),
		helpformula(helpFormula_),
		flags(flags_),
		x_min(xMin_), x_max(xMax_), y_min(yMin_), y_max(yMax_),
		isinteger(isInteger_), tojulia(toJulia_), tomandel(toMandel_), tofloat(toFloat_),
		symmetry(symmetry_),
		calculator(calculator_),
		orbit_bailout(orbitBailOut_)
	{
		parameters.Swap(parameters_);
	}
	FractalTypeSpecificData2(int type_, char const *name_,
		NamedParameterList const &parameters_,
		int helpText_, int helpFormula_, int flags_,
		float xMin_, float xMax_, float yMin_, float yMax_,
		int isInteger_, int toJulia_, int toMandel_, int toFloat_,
		SymmetryType symmetry_, IFractalCalculator &calculator_,
		int orbitBailOut_)
		: fractal_type(type_),
		name(name_),
		parameters(parameters_),
		helptext(helpText_),
		helpformula(helpFormula_),
		flags(flags_),
		x_min(xMin_), x_max(xMax_), y_min(yMin_), y_max(yMax_),
		isinteger(isInteger_), tojulia(toJulia_), tomandel(toMandel_), tofloat(toFloat_),
		symmetry(symmetry_),
		calculator(calculator_),
		orbit_bailout(orbitBailOut_)
	{
	}
	FractalTypeSpecificData2(int type_, char const *name_,
		NamedParameterList &parameters_,
		int helpText_, int helpFormula_, int flags_,
		int isInteger_, int toJulia_, int toMandel_, int toFloat_,
		SymmetryType symmetry_, IFractalCalculator &calculator_,
		int orbitBailOut_)
		: fractal_type(type_),
		name(name_),
		parameters(),
		helptext(helpText_),
		helpformula(helpFormula_),
		flags(flags_),
		x_min(-2.0f), x_max(2.0f), y_min(-1.5f), y_max(1.5f),
		isinteger(isInteger_), tojulia(toJulia_), tomandel(toMandel_), tofloat(toFloat_),
		symmetry(symmetry_),
		calculator(calculator_),
		orbit_bailout(orbitBailOut_)
	{
		parameters.Swap(parameters_);
	}
	FractalTypeSpecificData2(int type_, char const *name_,
		NamedParameterList const &parameters_,
		int helpText_, int helpFormula_, int flags_,
		int isInteger_, int toJulia_, int toMandel_, int toFloat_,
		SymmetryType symmetry_, IFractalCalculator &calculator_,
		int orbitBailOut_)
		: fractal_type(type_),
		name(name_),
		parameters(parameters_),
		helptext(helpText_),
		helpformula(helpFormula_),
		flags(flags_),
		x_min(-2.0f), x_max(2.0f), y_min(-1.5f), y_max(1.5f),
		isinteger(isInteger_), tojulia(toJulia_), tomandel(toMandel_), tofloat(toFloat_),
		symmetry(symmetry_),
		calculator(calculator_),
		orbit_bailout(orbitBailOut_)
	{
	}

private:
	int fractal_type;
	const std::string name;				// name of the fractal
										// (leading "*" supresses name display)
	NamedParameterList parameters;
	int   helptext;						// helpdefs.h HT_xxxx, -1 for none
	int   helpformula;					// helpdefs.h HF_xxxx, -1 for none
	int flags;							// constraints, bits defined below
	float x_min;						// default XMIN corner
	float x_max;						// default XMAX corner
	float y_min;						// default YMIN corner
	float y_max;						// default YMAX corner
	int   isinteger;					// >= 1 if integer fractal, 0 otherwise
	int   tojulia;						// index of corresponding julia type
	int   tomandel;						// index of corresponding mandelbrot type
	int   tofloat;						// index of corresponding floating-point type
	SymmetryType symmetry;				/* applicable symmetry logic
										   0 = no symmetry
										  -1 = y-axis symmetry (If No Params)
										   1 = y-axis symmetry
										  -2 = x-axis symmetry (No Parms)
										   2 = x-axis symmetry
										  -3 = y-axis AND x-axis (No Parms)
										   3 = y-axis AND x-axis symmetry
										  -4 = polar symmetry (No Parms)
										   4 = polar symmetry
										   5 = PI (sin/cos) symmetry
										   6 = NEWTON (power) symmetry
																*/
	IFractalCalculator &calculator;
	int orbit_bailout;					// usual bailout value for orbit calc

	int num_functions() const				{ return (flags >> FRACTALFLAG_FUNCTION_SHIFT) & FRACTALFLAG_FUNCTION_MASK; }
	bool flag(FractalFlags flag) const		{ return (flags & flag) != 0; }
	bool no_boundary_tracing() const		{ return flag(FRACTALFLAG_NO_BOUNDARY_TRACING); }
	bool no_solid_guessing() const			{ return flag(FRACTALFLAG_NO_SOLID_GUESSING); }
	bool arbitrary_precision() const		{ return flag(FRACTALFLAG_ARBITRARY_PRECISION); }
	bool no_zoom_box_rotate() const			{ return flag(FRACTALFLAG_NO_ZOOM_BOX_ROTATE); }
	const char *get_type() const			{ return &name[is_hidden() ? 1 : 0]; }
	bool is_hidden() const					{ return (name[0] == '*'); }
};

class FractalCalculator : public IFractalCalculator
{
public:
	typedef int OrbitProc();
	typedef int PerPixelProc();
	typedef bool PerImageProc();
	typedef int CalculateTypeProc();
	FractalCalculator(OrbitProc *orbit, PerPixelProc *perPixel, PerImageProc *perImage, CalculateTypeProc *calculateType)
		: _orbit(orbit),
		_perPixel(perPixel),
		_perImage(perImage),
		_calculateType(calculateType)
	{ }
	virtual ~FractalCalculator()		{ }
	virtual int Orbit()					{ assert(_orbit);			return _orbit(); }
	virtual int PerPixel()				{ assert(_perPixel);		return _perPixel(); }
	virtual bool PerImage()				{ assert(_perImage);		return _perImage(); }
	virtual int CalculateType()			{ assert(_calculateType);	return _calculateType(); }

private:
	OrbitProc *_orbit;
	PerPixelProc *_perPixel;
	PerImageProc *_perImage;
	CalculateTypeProc *_calculateType;
};
class StandardCalculator : public FractalCalculator
{
public:
	StandardCalculator(OrbitProc *orbit, PerPixelProc *perPixel, PerImageProc *perImage)
		: FractalCalculator(orbit, perPixel, perImage, standard_fractal)
	{ }
	virtual ~StandardCalculator()		{ }
};
class PerImageCalculator : public FractalCalculator
{
public:
	PerImageCalculator(PerImageProc *perImage, CalculateTypeProc *calculateType)
		: FractalCalculator(0, 0, perImage, calculateType)
	{ }
	virtual ~PerImageCalculator()		{ }
};
class StandaloneCalculator : public PerImageCalculator
{
public:
	StandaloneCalculator(CalculateTypeProc *calculateType)
		: PerImageCalculator(stand_alone_setup, calculateType)
	{ }
	virtual ~StandaloneCalculator()		{ }
};
class BifurcationCalculator : public FractalCalculator
{
public:
	BifurcationCalculator(OrbitProc *orbit, PerImageProc *perImage = stand_alone_setup)
		: FractalCalculator(orbit, 0, perImage, bifurcation)
	{ }
	virtual ~BifurcationCalculator()	{ }
};
class Orbit3DFpCalculator : public FractalCalculator
{
public:
	Orbit3DFpCalculator(OrbitProc *orbit)
		: FractalCalculator(orbit, 0, orbit_3d_setup_fp, orbit_3d_fp)
	{ }
	virtual ~Orbit3DFpCalculator()		{ }
};
class Orbit2DFpCalculator : public FractalCalculator
{
public:
	Orbit2DFpCalculator(OrbitProc *orbit)
		: FractalCalculator(orbit, 0, orbit_3d_setup_fp, orbit_2d_fp)
	{ }
	virtual ~Orbit2DFpCalculator()		{ }
};
class Orbit3DCalculator : public FractalCalculator
{
public:
	Orbit3DCalculator(OrbitProc *orbit)
		: FractalCalculator(orbit, 0, orbit_3d_setup, orbit_3d)
	{ }
	virtual ~Orbit3DCalculator()		{ }
};
class Orbit2DCalculator : public FractalCalculator
{
public:
	Orbit2DCalculator(OrbitProc *orbit)
		: FractalCalculator(orbit, 0, orbit_3d_setup, orbit_2d)
	{ }
	virtual ~Orbit2DCalculator()		{ }
};

typedef int (*VF)();

static FractalTypeSpecificData2 s_fractal_specific2[] =
{
	//
	//{ [OLDTYPEINDEX, ]NEWTYPEINDEX
	//	fractal type,
	//	fractal name,
	//	{parameter text strings},
	//	{parameter values},
	//	helptext, helpformula, flags,
	//	x_min, x_max, y_min, y_max,
	//	integer, tojulia, tomandel,
	//	tofloat, symmetry,
	//	orbit fnct, per_pixel fnct,
	//	per_image fnct, calctype fcnt,
	//	bailout
	//}
	//
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT, s_mandel_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT, IDHELP_MANDELBROT_TYPE,
		FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(julia_orbit, mandelbrot_per_pixel, mandelbrot_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA, s_julia_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.3)
			.Add(s_parameter_imag, 0.6),
		IDHELP_JULIA, IDHELP_JULIA_TYPE,
		FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_JULIA_FP, SYMMETRY_ORIGIN,
		StandardCalculator(julia_orbit, julia_per_pixel, julia_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_NEWTON_BASIN, s_newtbasin_name,
		NamedParameterList()
			.Add(s_newton_degree, 3.0)
			.Add("Enter non-zero value for stripes"),
		IDHELP_NEWTON_BASIN, IDHELP_NEWTON_BASIN_TYPE,
		0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON_BASIN_MP, SYMMETRY_NONE,
		StandardCalculator(newton2_orbit, other_julia_per_pixel_fp, newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LAMBDA, s_lambda_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.85)
			.Add(s_parameter_imag, 0.6),
		IDHELP_LAMBDA, IDHELP_LAMBDA_TYPE,
		FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-1.5f, 2.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA,
		FRACTYPE_LAMBDA_FP, SYMMETRY_NONE,
		StandardCalculator(lambda_orbit, julia_per_pixel, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FP, s_mandel_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT, IDHELP_MANDELBROT_TYPE, FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(julia_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_NEWTON, s_newton_name,
		NamedParameterList()
			.Add(s_newton_degree, 3.0),
		IDHELP_NEWTON, IDHELP_NEWTON_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON_MP, SYMMETRY_X_AXIS,
		StandardCalculator(newton2_orbit, other_julia_per_pixel_fp, newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FP, s_julia_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.3)
			.Add(s_parameter_imag, 0.6),
		IDHELP_JULIA, IDHELP_JULIA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FP,
		FRACTYPE_JULIA, SYMMETRY_ORIGIN,
		StandardCalculator(julia_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_PLASMA, "plasma",
		NamedParameterList()
			.Add("Graininess Factor (0 or 0.125 to 100, default is 2)", 2.0)
			.Add("+Algorithm (0 = original, 1 = new)")
			.Add("+Random Seed Value (0 = Random, 1 = Reuse Last)")
			.Add("+Save as Pot File? (0 = No,     1 = Yes)"),
		IDHELP_PLASMA, IDHELP_PLASMA_TYPE, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(plasma),
		ORBIT_BAILOUT_NONE),
	// FRACTYPE_OBSOLETE_LAMBDA_SINE, FRACTYPE_MANDELBROT_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_FP, s_mandelfn_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT_FUNC, IDHELP_MANDELBROT_FUNC_TYPE, FRACTALFLAG_1_FUNCTION,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_LAMBDA_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC, SYMMETRY_XY_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_trig_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_LAMBDA_COS, FRACTYPE_MAN_O_WAR_FP
	FractalTypeSpecificData2(FRACTYPE_MAN_O_WAR_FP, s_manowar_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_MAN_O_WAR_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_MAN_O_WAR_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MAN_O_WAR, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(man_o_war_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	// FRACTYPE_OBSOLETE_LAMBDA_EXP, FRACTYPE_MAN_O_WAR
	FractalTypeSpecificData2(FRACTYPE_MAN_O_WAR, s_manowar_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_MAN_O_WAR_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f,  1.5f, -1.5f, 1.5f,
		1, FRACTYPE_MAN_O_WAR_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MAN_O_WAR_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(man_o_war_orbit, mandelbrot_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_TEST, "test",
		NamedParameterList().Add("(testpt Param #1)")
			.Add("(testpt param #2)")
			.Add("(testpt param #3)")
			.Add("(testpt param #4)"),
		IDHELP_TEST, IDHELP_TEST_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(test),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_SIERPINSKI, s_sierpinski_name + 1,
		NamedParameterList(),
		IDHELP_SIERPINSKI, IDHELP_SIERPINSKI_TYPE, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SIERPINSKI_FP, SYMMETRY_NONE,
		StandardCalculator(sierpinski_orbit, julia_per_pixel_l, sierpinski_setup),
		127),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M1, s_barnsleym1_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M1_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_BARNSLEY_J1, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M1_FP, SYMMETRY_XY_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley1_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J1, s_barnsleyj1_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 1.1),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J1_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M1,
		FRACTYPE_BARNSLEY_J1_FP, SYMMETRY_ORIGIN,
		StandardCalculator(barnsley1_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M2, s_barnsleym2_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M2_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_BARNSLEY_J2, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M2_FP, SYMMETRY_Y_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley2_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J2, s_barnsleyj2_name + 1,
		NamedParameterList().Add(s_parameter_real, 0.6).Add(s_parameter_imag, 1.1),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J2_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M2,
		FRACTYPE_BARNSLEY_J2_FP, SYMMETRY_ORIGIN,
		StandardCalculator(barnsley2_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	// FRACTYPE_OBSOLETE_MANDELBROT_SINE, FRACTYPE_SQR_FUNC
	FractalTypeSpecificData2(FRACTYPE_SQR_FUNC, s_sqr_fn__name + 1,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SQR_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_FUNC_FP, SYMMETRY_X_AXIS,
		StandardCalculator(sqr_trig_orbit, julia_per_pixel_l, sqr_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_MANDELBROT_COS, FRACTYPE_SQR_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_SQR_FUNC_FP, s_sqr_fn__name,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SQR_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_FUNC, SYMMETRY_X_AXIS,
		StandardCalculator(sqr_trig_orbit_fp, other_julia_per_pixel_fp, sqr_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_MANDELBROT_EXP, FRACTYPE_FUNC_PLUS_FUNC
	FractalTypeSpecificData2(FRACTYPE_FUNC_PLUS_FUNC, s_fnplusfn_name + 1,
		NamedParameterList()
			.Add(s_trig1_coefficient_re, 1.0)
			.Add(s_trig1_coefficient_im, 0.0)
			.Add(s_trig2_coefficient_re, 1.0)
			.Add(s_trig2_coefficient_im, 0.0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_PLUS_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_FP, SYMMETRY_X_AXIS,
		StandardCalculator(trig_plus_trig_orbit, julia_per_pixel_l, trig_plus_trig_setup_l),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_LAMBDA, s_mandellambda_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT_LAMBDA, IDHELP_MANDELBROT_LAMBDA_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-3.0f, 5.0f, -3.0f, 3.0f,
		1, FRACTYPE_LAMBDA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_orbit, mandelbrot_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_MANDELBROT, s_marksmandel_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0).Add(s_exponent, 1.0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_MANDELBROT_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_MARKS_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_FP, SYMMETRY_NONE,
		StandardCalculator(marks_lambda_orbit, marks_mandelbrot_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_JULIA, s_marksjulia_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.1)
			.Add(s_parameter_imag, 0.9)
			.Add(s_exponent, 1.0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_JULIA_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT,
		FRACTYPE_MARKS_JULIA_FP, SYMMETRY_ORIGIN,
		StandardCalculator(marks_lambda_orbit, julia_per_pixel, marks_julia_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_UNITY, s_unity_name + 1,
		NamedParameterList(),
		IDHELP_UNITY, IDHELP_UNITY_TYPE, 0,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_UNITY_FP, SYMMETRY_XY_AXIS,
		StandardCalculator(unity_orbit, julia_per_pixel_l, unity_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_4, s_mandel4_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_QUARTIC_MANDELBROT_JULIA, IDHELP_MANDELBROT_4_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_JULIA_4, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_4_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(mandel4_orbit, mandelbrot_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_4, s_julia4_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 0.55),
		IDHELP_QUARTIC_MANDELBROT_JULIA, IDHELP_JULIA_4_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_4,
		FRACTYPE_JULIA_4_FP, SYMMETRY_ORIGIN,
		StandardCalculator(mandel4_orbit, julia_per_pixel, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_IFS, "ifs",
		NamedParameterList().Add(s_color_method),
		IDHELP_IFS, SPECIALHF_IFS, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_INFINITE_CALCULATION,
		-8.0f, 8.0f, -1.0f, 11.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(ifs),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_IFS_3D, s_ifs3d_name,
		NamedParameterList().Add(s_color_method),
		IDHELP_IFS, SPECIALHF_IFS, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-11.0f, 11.0f, -11.0f, 11.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(ifs),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M3, s_barnsleym3_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M3_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_BARNSLEY_J3, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M3_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley3_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J3, s_barnsleyj3_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.1)
			.Add(s_parameter_imag, 0.36),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J3_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M3,
		FRACTYPE_BARNSLEY_J3_FP, SYMMETRY_NONE,
		StandardCalculator(barnsley3_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	// FRACTYPE_OBSOLETE_DEM_MANDELBROT, FRACTYPE_FUNC_SQR
	FractalTypeSpecificData2(FRACTYPE_FUNC_SQR, s_fn_zz__name + 1,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_SQR_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_SQR_FP, SYMMETRY_XY_AXIS,
		StandardCalculator(trig_z_squared_orbit, julia_per_pixel, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	// FRACTYPE_OBSOLETE_DEM_JULIA, FRACTYPE_FUNC_SQR_FP
	FractalTypeSpecificData2(FRACTYPE_FUNC_SQR_FP, s_fn_zz__name,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_SQR_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_SQR, SYMMETRY_XY_AXIS,
		StandardCalculator(trig_z_squared_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION, s_bifurcation_name,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_verhulst_trig_fp),
		ORBIT_BAILOUT_NONE),
	// FRACTYPE_OBSOLETE_MANDELBROT_SINH, FRACTYPE_FUNC_PLUS_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_FUNC_PLUS_FUNC_FP, s_fnplusfn_name,
		NamedParameterList()
			.Add(s_trig1_coefficient_re, 1.0)
			.Add(s_trig1_coefficient_im)
			.Add(s_trig2_coefficient_re, 1.0)
			.Add(s_trig2_coefficient_im),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_PLUS_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC, SYMMETRY_X_AXIS,
		StandardCalculator(trig_plus_trig_orbit_fp, other_julia_per_pixel_fp, trig_plus_trig_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_LAMBDA_SINH, FRACTYPE_FUNC_TIMES_FUNC
	FractalTypeSpecificData2(FRACTYPE_FUNC_TIMES_FUNC, s_fnfn_name + 1,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_TIMES_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_FUNC_FP, SYMMETRY_X_AXIS,
		StandardCalculator(trig_trig_orbit, julia_per_pixel_l, fn_fn_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_MANDELBROT_COSH, FRACTYPE_FUNC_TIMES_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_FUNC_TIMES_FUNC_FP, s_fnfn_name,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_TIMES_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_FUNC, SYMMETRY_X_AXIS,
		StandardCalculator(trig_trig_orbit_fp, other_julia_per_pixel_fp, fn_fn_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_LAMBDA_COSH, FRACTYPE_SQR_RECIPROCAL_FUNC
	FractalTypeSpecificData2(FRACTYPE_SQR_RECIPROCAL_FUNC, s_sqr_1divfn__name + 1,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SQR_RECIPROCAL_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_RECIPROCAL_FUNC_FP, SYMMETRY_NONE,
		StandardCalculator(sqr_1_over_trig_z_orbit, julia_per_pixel_l, sqr_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	// FRACTYPE_OBSOLETE_MANDELBROT_SINE_L, FRACTYPE_SQR_RECIPROCAL_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_SQR_RECIPROCAL_FUNC_FP,
	s_sqr_1divfn__name,
		NamedParameterList(),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SQR_RECIPROCAL_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_RECIPROCAL_FUNC, SYMMETRY_NONE,
		StandardCalculator(sqr_1_over_trig_z_orbit_fp, other_julia_per_pixel_fp, sqr_trig_setup),
		ORBIT_BAILOUT_TRIG_L),

	// FRACTYPE_OBSOLETE_LAMBDA_SINE_L, FRACTYPE_FUNC_TIMES_Z_PLUS_Z
	FractalTypeSpecificData2(FRACTYPE_FUNC_TIMES_Z_PLUS_Z,
	s_fnzplusz_name + 1,
		NamedParameterList()
			.Add(s_trig1_coefficient_re, 1.0)
			.Add(s_trig1_coefficient_im)
			.Add("Real Coefficient Second Term", 1.0)
			.Add("Imag Coefficient Second Term"),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_TIMES_Z_PLUS_Z_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP, SYMMETRY_X_AXIS,
		StandardCalculator(z_trig_z_plus_z_orbit, julia_per_pixel, z_trig_plus_z_setup),
		ORBIT_BAILOUT_TRIG_L),

	// FRACTYPE_OBSOLETE_MANDELBROT_COS_L, FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP
	FractalTypeSpecificData2(FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP,
	s_fnzplusz_name,
		NamedParameterList()
			.Add(s_trig1_coefficient_re, 1.0)
			.Add(s_trig1_coefficient_im)
			.Add("Real Coefficient Second Term", 1.0)
			.Add("Imag Coefficient Second Term"),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_TIMES_Z_PLUS_Z_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_Z_PLUS_Z, SYMMETRY_X_AXIS,
		StandardCalculator(z_trig_z_plus_z_orbit_fp, julia_per_pixel_fp, z_trig_plus_z_setup),
		ORBIT_BAILOUT_TRIG_L),

	// FRACTYPE_OBSOLETE_LAMBDA_COS_L, FRACTYPE_KAM_TORUS_FP
	FractalTypeSpecificData2(FRACTYPE_KAM_TORUS_FP,
	s_kamtorus_name,
		NamedParameterList()
			.Add(s_kam_angle, 1.3)
			.Add(s_kam_step, 0.05)
			.Add(s_kam_stop, 1.5)
			.Add(s_points_per_orbit, 150),
		IDHELP_KAM_TORUS, IDHELP_KAM_TORUS_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -.75f, .75f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(kam_torus_orbit_fp)),
		ORBIT_BAILOUT_NONE),

	// FRACTYPE_OBSOLETE_MANDELBROT_SINH_L, FRACTYPE_KAM_TORUS
	FractalTypeSpecificData2(FRACTYPE_KAM_TORUS,
	s_kamtorus_name + 1,
		NamedParameterList()
			.Add(s_kam_angle, 1.3)
			.Add(s_kam_step, 0.05)
			.Add(s_kam_stop, 1.5)
			.Add(s_points_per_orbit, 150),
		IDHELP_KAM_TORUS, IDHELP_KAM_TORUS_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -.75f, .75f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_FP, SYMMETRY_NONE,
		Orbit2DCalculator(VF(kam_torus_orbit)),
		ORBIT_BAILOUT_NONE),

	// FRACTYPE_OBSOLETE_LAMBDA_SINH_L, FRACTYPE_KAM_TORUS_3D_FP
	FractalTypeSpecificData2(FRACTYPE_KAM_TORUS_3D_FP,
	s_kamtorus3d_name,
		NamedParameterList()
			.Add(s_kam_angle, 1.3)
			.Add(s_kam_step, 0.05)
			.Add(s_kam_stop, 1.5)
			.Add(s_points_per_orbit, 150),
		IDHELP_KAM_TORUS, IDHELP_KAM_TORUS_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-3.0f, 3.0f, -1.0f, 3.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_3D, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(kam_torus_orbit_fp)),
		ORBIT_BAILOUT_NONE),

	// FRACTYPE_OBSOLETE_MANDELBROT_COSH_L, FRACTYPE_KAM_TORUS_3D
	FractalTypeSpecificData2(FRACTYPE_KAM_TORUS_3D, s_kamtorus3d_name + 1,
		NamedParameterList()
			.Add(s_kam_angle, 1.3)
			.Add(s_kam_step, 0.05)
			.Add(s_kam_stop, 1.5)
			.Add(s_points_per_orbit, 150),
		IDHELP_KAM_TORUS, IDHELP_KAM_TORUS_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-3.0f, 3.0f, -1.0f, 3.5f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_3D_FP, SYMMETRY_NONE,
		Orbit3DCalculator(VF(kam_torus_orbit)),
		ORBIT_BAILOUT_NONE),

	// FRACTYPE_OBSOLETE_LAMBDA_COSH_L, FRACTYPE_LAMBDA_FUNC
	FractalTypeSpecificData2(FRACTYPE_LAMBDA_FUNC, s_lambdafn_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 1.0)
			.Add(s_parameter_imag, 0.4),
		IDHELP_LAMBDA_FUNC, IDHELP_LAMBDA_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC,
		FRACTYPE_LAMBDA_FUNC_FP, SYMMETRY_PI,
		StandardCalculator(VF(lambda_trig_orbit), julia_per_pixel_l, lambda_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L, s_manfnpluszsqrd_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_FUNC_PLUS_Z_SQUARED_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		16, FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(trig_plus_z_squared_orbit, mandelbrot_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L, s_julfnpluszsqrd_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, -0.5)
			.Add(s_parameter_imag, 0.5),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_FUNC_PLUS_Z_SQUARED_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L,
		FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP, SYMMETRY_NONE,
		StandardCalculator(trig_plus_z_squared_orbit, julia_per_pixel, julia_fn_plus_z_squared_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP, s_manfnpluszsqrd_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_FUNC_PLUS_Z_SQUARED_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(trig_plus_z_squared_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP, s_julfnpluszsqrd_name,
		NamedParameterList()
			.Add(s_parameter_real, -0.5)
			.Add(s_parameter_imag, 0.5),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_FUNC_PLUS_Z_SQUARED_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP,
		FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L, SYMMETRY_NONE,
		StandardCalculator(trig_plus_z_squared_orbit_fp, julia_per_pixel_fp, julia_fn_plus_z_squared_setup),
		ORBIT_BAILOUT_STANDARD),
	// FRACTYPE_OBSOLETE_MANDELBROT_EXP_L, FRACTYPE_LAMBDA_FUNC_FP
	FractalTypeSpecificData2(FRACTYPE_LAMBDA_FUNC_FP, s_lambdafn_name,
		NamedParameterList()
			.Add(s_parameter_real, 1.0)
			.Add(s_parameter_imag, 0.4),
		IDHELP_LAMBDA_FUNC, IDHELP_LAMBDA_FUNC_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_FP, FRACTYPE_LAMBDA_FUNC, SYMMETRY_PI,
		StandardCalculator(lambda_trig_orbit_fp, other_julia_per_pixel_fp, lambda_trig_setup),
		ORBIT_BAILOUT_TRIG_L),

	// FRACTYPE_OBSOLETE_LAMBDA_EXP_L, FRACTYPE_MANDELBROT_FUNC
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC,
	s_mandelfn_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT_FUNC, IDHELP_MANDELBROT_FUNC_TYPE, FRACTALFLAG_1_FUNCTION,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, FRACTYPE_LAMBDA_FUNC, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_FP, SYMMETRY_XY_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_trig_orbit, mandelbrot_per_pixel_l, mandelbrot_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_Z_POWER_L, s_manzpower_name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_exponent, 2.0)
			.Add(s_exponent_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_Z_POWER_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_JULIA_Z_POWER_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_Z_POWER_FP, SYMMETRY_X_AXIS_NO_IMAGINARY,
		StandardCalculator(z_power_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_Z_POWER_L, s_julzpower_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 0.3)
			.Add(s_parameter_imag, 0.6)
			.Add(s_exponent, 2.0)
			.Add(s_exponent_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_Z_POWER_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_POWER_L,
		FRACTYPE_JULIA_Z_POWER_FP, SYMMETRY_ORIGIN,
		StandardCalculator(z_power_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_Z_POWER_FP, s_manzpower_name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_exponent, 2.0)
			.Add(s_exponent_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_Z_POWER_TYPE, FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_Z_POWER_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_Z_POWER_L, SYMMETRY_X_AXIS_NO_IMAGINARY,
		StandardCalculator(z_power_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_Z_POWER_FP, s_julzpower_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.3)
			.Add(s_parameter_imag, 0.6)
			.Add(s_exponent, 2.0)
			.Add(s_exponent_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_Z_POWER_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_POWER_FP,
		FRACTYPE_JULIA_Z_POWER_L, SYMMETRY_ORIGIN,
		StandardCalculator(z_power_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP, "manzzpwr",
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_exponent, 2.0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_Z_TO_Z_PLUS_Z_POWER_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(z_to_z_plus_z_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_Z_TO_Z_PLUS_Z_POWER_FP, "julzzpwr",
		NamedParameterList()
			.Add(s_parameter_real, -0.3)
			.Add(s_parameter_imag, 0.3)
			.Add(s_exponent, 2.0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_Z_TO_Z_PLUS_POWER_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(z_to_z_plus_z_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L, s_manfnplusexp_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_FUNC_PLUS_EXP_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, FRACTYPE_JULIA_FUNC_PLUS_EXP_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(trig_plus_exponent_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_PLUS_EXP_L, s_julfnplusexp_name + 1,
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_FUNC_PLUS_EXP_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L,
		FRACTYPE_JULIA_FUNC_PLUS_EXP_FP, SYMMETRY_NONE,
		StandardCalculator(trig_plus_exponent_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP, s_manfnplusexp_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_MANDELBROT_FUNC_PLUS_EXP_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_JULIA_FUNC_PLUS_EXP_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(trig_plus_exponent_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_PLUS_EXP_FP, s_julfnplusexp_name,
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_PICKOVER_MANDELBROT_JULIA, IDHELP_JULIA_FUNC_PLUS_EXP_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP,
		FRACTYPE_JULIA_FUNC_PLUS_EXP_L, SYMMETRY_NONE,
		StandardCalculator(trig_plus_exponent_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_POPCORN_FP, s_popcorn_name,
		NamedParameterList()
			.Add(s_step_x, 0.05)
			.Add(s_step_y)
			.Add(s_constant_x, 3.00)
			.Add(s_constant_y),
		IDHELP_POPCORN, IDHELP_POPCORN_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_L, SYMMETRY_NO_PLOT,
		FractalCalculator(popcorn_fn_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp, popcorn),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_POPCORN_L, s_popcorn_name + 1,
		NamedParameterList()
			.Add(s_step_x, 0.05)
			.Add(s_step_y)
			.Add(s_constant_x, 3.00)
			.Add(s_constant_y),
		IDHELP_POPCORN, IDHELP_POPCORN_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_FP, SYMMETRY_NO_PLOT,
		FractalCalculator(popcorn_fn_orbit, julia_per_pixel_l, julia_setup_l, popcorn),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_FP, s_lorenz_name,
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 5.0)
			.Add("b", 15.0)
			.Add("c", 1.0),
		IDHELP_LORENZ, IDHELP_LORENZ_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-15.0f, 15.0f, 0.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_L, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(lorenz_3d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_L, s_lorenz_name + 1,
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 5.0)
			.Add("b", 15.0)
			.Add("c", 1.0),
		IDHELP_LORENZ, IDHELP_LORENZ_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-15.0f, 15.0f, 0.0f, 30.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_FP, SYMMETRY_NONE,
		Orbit2DCalculator(VF(lorenz_3d_orbit)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_3D_L, s_lorenz3d_name + 1,
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 5.0)
			.Add("b", 15.0)
			.Add("c", 1.0),
		IDHELP_LORENZ, IDHELP_LORENZ_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_3D_FP, SYMMETRY_NONE,
		Orbit3DCalculator(VF(lorenz_3d_orbit)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_NEWTON_MP, s_newton_name + 1,
		NamedParameterList().Add(s_newton_degree, 3.0),
		IDHELP_NEWTON, IDHELP_NEWTON_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON, SYMMETRY_X_AXIS,
		StandardCalculator(newton_orbit_mpc, julia_per_pixel_mpc, newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_NEWTON_BASIN_MP, s_newtbasin_name + 1,
		NamedParameterList()
			.Add(s_newton_degree, 3.0)
			.Add("Enter non-zero value for stripes"),
		IDHELP_NEWTON_BASIN, IDHELP_NEWTON_BASIN_TYPE,
		0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON_BASIN, SYMMETRY_NONE,
		StandardCalculator(newton_orbit_mpc, julia_per_pixel_mpc, newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_NEWTON_COMPLEX, "complexnewton",
		NamedParameterList()
			.Add("Real part of Degree", 3.0)
			.Add("Imag part of Degree")
			.Add("Real part of Root", 1.0)
			.Add("Imag part of Root"),
		IDHELP_NEWTON_COMPLEX, IDHELP_NEWTON_COMPLEX_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(complex_newton, other_julia_per_pixel_fp, complex_newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_NEWTON_BASIN_COMPLEX, "complexbasin",
		NamedParameterList()
			.Add("Real part of Degree", 3.0)
			.Add("Imag part of Degree")
			.Add("Real part of Root", 1.0)
			.Add("Imag part of Root"),
		IDHELP_NEWTON_COMPLEX, IDHELP_NEWTON_COMPLEX_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(complex_basin, other_julia_per_pixel_fp, complex_newton_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_MARKS_MANDELBROT_COMPLEX, "cmplxmarksmand",
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_exponent, 1.0)
			.Add(s_exponent_imag),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_MANDELBROT_COMPLEX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_MARKS_JULIA_COMPLEX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(marks_complex_mandelbrot_orbit, marks_complex_mandelbrot_per_pixel, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_JULIA_COMPLEX, "cmplxmarksjul",
		NamedParameterList()
			.Add(s_parameter_real, 0.3)
			.Add(s_parameter_imag, 0.6)
			.Add(s_exponent, 1.0)
			.Add(s_exponent_imag),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_JULIA_COMPLEX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT_COMPLEX,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(marks_complex_mandelbrot_orbit, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_FORMULA, s_formula_name + 1,
		NamedParameterList()
			.Add(s_p1_real).Add(s_p1_imag).Add(s_p2_real).Add(s_p2_imag),
		IDHELP_FORMULA, SPECIALHF_FORMULA, FRACTALFLAG_MORE_PARAMETERS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FORMULA_FP, SYMMETRY_SETUP,
		StandardCalculator(formula_orbit, form_per_pixel, formula_setup_int),
		0),
	FractalTypeSpecificData2(FRACTYPE_FORMULA_FP, s_formula_name,
		NamedParameterList()
			.Add(s_p1_real).Add(s_p1_imag).Add(s_p2_real).Add(s_p2_imag),
		IDHELP_FORMULA, SPECIALHF_FORMULA, FRACTALFLAG_MORE_PARAMETERS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FORMULA, SYMMETRY_SETUP,
		StandardCalculator(formula_orbit, form_per_pixel, formula_setup_fp),
		0),
	FractalTypeSpecificData2(FRACTYPE_SIERPINSKI_FP, s_sierpinski_name,
		NamedParameterList(),
		IDHELP_SIERPINSKI, IDHELP_SIERPINSKI_TYPE, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SIERPINSKI, SYMMETRY_NONE,
		StandardCalculator(sierpinski_orbit_fp, other_julia_per_pixel_fp, sierpinski_setup_fp),
		127),
	FractalTypeSpecificData2(FRACTYPE_LAMBDA_FP, s_lambda_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.85)
			.Add(s_parameter_imag, 0.6),
		IDHELP_LAMBDA, IDHELP_LAMBDA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FP,
		FRACTYPE_LAMBDA, SYMMETRY_NONE,
		StandardCalculator(lambda_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M1_FP, s_barnsleym1_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M1_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_BARNSLEY_J1_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M1, SYMMETRY_XY_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley1_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J1_FP, s_barnsleyj1_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 1.1),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J1_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M1_FP,
		FRACTYPE_BARNSLEY_J1, SYMMETRY_ORIGIN,
		StandardCalculator(barnsley1_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M2_FP, s_barnsleym2_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M2_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_BARNSLEY_J2_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M2, SYMMETRY_Y_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley2_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J2_FP, s_barnsleyj2_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 1.1),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J2_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M2_FP,
		FRACTYPE_BARNSLEY_J2, SYMMETRY_ORIGIN,
		StandardCalculator(barnsley2_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_M3_FP, s_barnsleym3_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_M3_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_BARNSLEY_J3_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M3, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(barnsley3_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BARNSLEY_J3_FP, s_barnsleyj3_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 1.1),
		IDHELP_BARNSLEY, IDHELP_BARNSLEY_J3_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M3_FP,
		FRACTYPE_BARNSLEY_J3, SYMMETRY_NONE,
		StandardCalculator(barnsley3_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_LAMBDA_FP, s_mandellambda_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MANDELBROT_LAMBDA, IDHELP_MANDELBROT_LAMBDA_TYPE,
		FRACTALFLAG_BAIL_OUT_TESTS,
		-3.0f, 5.0f, -3.0f, 3.0f,
		0, FRACTYPE_LAMBDA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIBROT, s_julibrot_name + 1,
		NamedParameterList(),
		IDHELP_JULIBROT, SPECIALHF_JULIBROT, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE | FRACTALFLAG_NOT_RESUMABLE,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_JULIBROT_FP, SYMMETRY_NONE,
		FractalCalculator(julia_orbit, julibrot_per_pixel, julibrot_setup, standard_4d_fractal),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_3D_FP, s_lorenz3d_name,
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 5.0)
			.Add("b", 15.0)
			.Add("c", 1.0),
		IDHELP_LORENZ, IDHELP_LORENZ_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_3D_L, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(lorenz_3d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_ROSSLER_L, s_rossler3d_name + 1,
		NamedParameterList()
			.Add(s_time_step, 0.04)
			.Add("a", 0.2)
			.Add("b", 0.2)
			.Add("c", 5.7),
		IDHELP_ROSSLER, IDHELP_ROSSLER_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -20.0f, 40.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_ROSSLER_FP, SYMMETRY_NONE,
		Orbit3DCalculator(VF(rossler_orbit)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_ROSSLER_FP, s_rossler3d_name,
		NamedParameterList()
			.Add(s_time_step, 0.04)
			.Add("a", 0.2)
			.Add("b", 0.2)
			.Add("c", 5.7),
		IDHELP_ROSSLER, IDHELP_ROSSLER_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -20.0f, 40.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_ROSSLER_L, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(rossler_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_HENON_L, s_henon_name + 1,
		NamedParameterList()
			.Add("a", 1.4)
			.Add("b", 0.3),
		IDHELP_HENON, IDHELP_HENON_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-1.4f, 1.4f, -.5f, .5f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HENON_FP, SYMMETRY_NONE,
		Orbit2DCalculator(VF(henon_orbit)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_HENON_FP, s_henon_name,
		NamedParameterList()
			.Add("a", 1.4)
			.Add("b", 0.3),
		IDHELP_HENON, IDHELP_HENON_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-1.4f, 1.4f, -.5f, .5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HENON_L, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(henon_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_PICKOVER_FP, "pickover",
		NamedParameterList()
			.Add("a", 2.24)
			.Add("b", 0.43)
			.Add("c", -0.65)
			.Add("d", -2.43),
		IDHELP_PICKOVER, IDHELP_PICKOVER_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-8.0f/3.0f, 8.0f/3.0f, -2.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(pickover_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_GINGERBREAD_FP, "gingerbreadman",
		NamedParameterList()
			.Add("Initial x", -0.1)
			.Add("Initial y"),
		IDHELP_GINGERBREAD, IDHELP_GINGERBREAD_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-4.5f, 8.5f, -4.5f, 8.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(gingerbread_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_DIFFUSION, "diffusion",
		NamedParameterList()
			.Add("+Border size", 10.0)
			.Add("+Type (0=Central, 1=Falling, 2=Square Cavity)")
			.Add("+Color change rate (0=Random)"),
		IDHELP_DIFFUSION, IDHELP_DIFFUSION_TYPE, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(diffusion),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_UNITY_FP, s_unity_name,
		NamedParameterList(),
		IDHELP_UNITY, IDHELP_UNITY_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_UNITY, SYMMETRY_XY_AXIS,
		StandardCalculator(unity_orbit_fp, other_julia_per_pixel_fp, standard_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_SPIDER_FP, s_spider_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SPIDER_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SPIDER, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(spider_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_SPIDER, s_spider_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_SPIDER_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SPIDER_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(spider_orbit, mandelbrot_per_pixel, spider_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_TETRATE_FP, "tetrate",
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_TETRATE_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		StandardCalculator(tetrate_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MAGNET_1M, "magnet1m",
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MAGNETIC, IDHELP_MAGNET_1M_TYPE, 0,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_MAGNET_1J, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(magnet1_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		100),
	FractalTypeSpecificData2(FRACTYPE_MAGNET_1J, "magnet1j",
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_MAGNETIC, IDHELP_MAGNET_1J_TYPE, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAGNET_1M,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		StandardCalculator(magnet1_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		100),
	FractalTypeSpecificData2(FRACTYPE_MAGNET_2M, "magnet2m",
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MAGNETIC, IDHELP_MAGNET_2M_TYPE, 0,
		-1.5f, 3.7f, -1.95f, 1.95f,
		0, FRACTYPE_MAGNET_2J, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(magnet2_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		100),
	FractalTypeSpecificData2(FRACTYPE_MAGNET_2J, "magnet2j",
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_MAGNETIC, IDHELP_MAGNET_2J_TYPE, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAGNET_2M,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		StandardCalculator(magnet2_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		100),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_L, s_bifurcation_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_verhulst_trig),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_LAMBDA_L, s_biflambda_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_LAMBDA_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_LAMBDA, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_lambda_trig),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_LAMBDA, s_biflambda_name,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_LAMBDA_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_LAMBDA_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_lambda_trig_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_PLUS_FUNC_PI, s_bifplussinpi_name,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_PLUS_SIN_PI_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_add_trig_pi_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_EQUAL_FUNC_PI, s_bifeqsinpi_name,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_EQUAL_SIN_PI_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_set_trig_pi_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_POPCORN_JULIA_FP, s_popcornjul_name,
		NamedParameterList()
			.Add(s_step_x, 0.05)
			.Add(s_step_y)
			.Add(s_constant_x, 3.00)
			.Add(s_constant_y),
		IDHELP_POPCORN, IDHELP_POPCORN_JULIA_TYPE, FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_JULIA_L, SYMMETRY_NONE,
		StandardCalculator(popcorn_fn_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_POPCORN_JULIA_L, s_popcornjul_name + 1,
		NamedParameterList()
			.Add(s_step_x, 0.05)
			.Add(s_step_y)
			.Add(s_constant_x, 3.0)
			.Add(s_constant_y),
		IDHELP_POPCORN, IDHELP_POPCORN_JULIA_TYPE, FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_JULIA_FP, SYMMETRY_NONE,
		StandardCalculator(popcorn_fn_orbit, julia_per_pixel_l, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_L_SYSTEM, "lsystem",
		NamedParameterList().Add(" + Order", 2.0),
		IDHELP_L_SYSTEMS, SPECIALHF_L_SYSTEM, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -1.0f, 1.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(l_system),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_MAN_O_WAR_JULIA_FP, s_manowarj_name,
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_MAN_O_WAR_JULIA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAN_O_WAR_FP,
		FRACTYPE_MAN_O_WAR_JULIA, SYMMETRY_NONE,
		StandardCalculator(man_o_war_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MAN_O_WAR_JULIA, s_manowarj_name + 1,
		NamedParameterList().Add(s_parameter_real).Add(s_parameter_imag),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_MAN_O_WAR_JULIA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MAN_O_WAR,
		FRACTYPE_MAN_O_WAR_JULIA_FP, SYMMETRY_NONE,
		StandardCalculator(man_o_war_orbit, julia_per_pixel, julia_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_FUNC_PLUS_FUNC_PIXEL_FP, s_fn_z_plusfn_pix__name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_trig2_coefficient_re, 1.0)
			.Add(s_trig2_coefficient_im),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_PLUS_FUNC_PIXEL_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-3.6f, 3.6f, -2.7f, 2.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_PIXEL_L, SYMMETRY_NONE,
		StandardCalculator(richard8_orbit_fp, other_richard8_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_FUNC_PLUS_FUNC_PIXEL_L, s_fn_z_plusfn_pix__name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_trig2_coefficient_re, 1.0)
			.Add(s_trig2_coefficient_im),
		IDHELP_TAYLOR_SKINNER_VARIATIONS, IDHELP_FUNC_PLUS_FUNC_PIXEL_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-3.6f, 3.6f, -2.7f, 2.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_PIXEL_FP, SYMMETRY_NONE,
		StandardCalculator(richard8_orbit, richard8_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MARKS_MANDELBROT_POWER_FP, s_marksmandelpwr_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_MANDELBROT_POWER_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_POWER, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(marks_mandel_power_orbit_fp, marks_mandelbrot_power_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_MANDELBROT_POWER, s_marksmandelpwr_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_MANDELBROT_POWER_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_POWER_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(marks_mandel_power_orbit, marks_mandelbrot_power_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_TIMS_ERROR_FP, s_tims_error_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MARK_PETERSON, IDHELP_TIMS_ERROR_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.9f, 4.3f, -2.7f, 2.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_TIMS_ERROR, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(tims_error_orbit_fp, marks_mandelbrot_power_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_TIMS_ERROR, s_tims_error_name + 1,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_MARK_PETERSON, IDHELP_TIMS_ERROR_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.9f, 4.3f, -2.7f, 2.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_TIMS_ERROR_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(tims_error_orbit, marks_mandelbrot_power_per_pixel, mandelbrot_setup_l),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L, s_bifeqsinpi_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_EQUAL_SIN_PI_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_EQUAL_FUNC_PI, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_set_trig_pi),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L, s_bifplussinpi_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_PLUS_SIN_PI_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_PLUS_FUNC_PI, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_add_trig_pi),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_STEWART, s_bifstewart_name,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_STEWART_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_STEWART_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_stewart_trig_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_STEWART_L, s_bifstewart_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 1000.0)
			.Add(s_seed_population, 0.66),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_STEWART_TYPE, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_STEWART, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_stewart_trig),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_HOPALONG_FP, "hopalong",
		NamedParameterList()
			.Add("a", 0.4)
			.Add("b", 1.0)
			.Add("c"),
		IDHELP_MARTIN, IDHELP_HOPALONG_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-2.0f, 3.0f, -1.625f, 2.625f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(hopalong_2d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_CIRCLE_FP, "circle",
		NamedParameterList().Add("magnification", 200000.0),
		IDHELP_CIRCLE, IDHELP_CIRCLE_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_XY_AXIS,
		StandardCalculator(circle_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_MARTIN_FP, "martin",
		NamedParameterList().Add("a", 3.14),
		IDHELP_MARTIN, IDHELP_MARTIN_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-32.0f, 32.0f, -24.0f, 24.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(martin_2d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LYAPUNOV, "lyapunov",
		NamedParameterList()
			.Add("+Order (integer)")
			.Add("Population Seed", 0.5)
			.Add("+Filter Cycles"),
		IDHELP_LYAPUNOV, IDHELP_LYAPUNOV, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		FractalCalculator(bifurcation_lambda, 0, lyapunov_setup, lyapunov),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_3D_1_FP, "lorenz3d1",
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 5.0)
			.Add("b", 15.0)
			.Add("c", 1.0),
		IDHELP_LORENZ, IDHELP_LORENZ_3D_1_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(lorenz_3d1_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_3D_3_FP, "lorenz3d3",
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 10.0)
			.Add("b", 28.0)
			.Add("c", 2.66),
		IDHELP_LORENZ, IDHELP_LORENZ_3D_3_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(lorenz_3d3_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LORENZ_3D_4_FP, "lorenz3d4",
		NamedParameterList()
			.Add(s_time_step, 0.02)
			.Add("a", 10.0)
			.Add("b", 28.0)
			.Add("c", 2.66),
		IDHELP_LORENZ, IDHELP_LORENZ_3D_4_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(lorenz_3d4_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_LAMBDA_FUNC_OR_FUNC_L, s_lambda_fnorfn__name + 1,
		NamedParameterList()
			.Add(s_parameter_real, 1.0)
			.Add(s_parameter_imag, 0.1)
			.Add("Function Shift Value", 1.0),
		IDHELP_FUNC_OR_FUNC, IDHELP_LAMBDA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L,
		FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP, SYMMETRY_ORIGIN,
		StandardCalculator(lambda_trig_or_trig_orbit, julia_per_pixel_l, lambda_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP, s_lambda_fnorfn__name,
		NamedParameterList()
			.Add(s_parameter_real, 1.0)
			.Add(s_parameter_imag, 0.1)
			.Add("Function Shift Value", 1.0),
		IDHELP_FUNC_OR_FUNC, IDHELP_LAMBDA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP,
		FRACTYPE_LAMBDA_FUNC_OR_FUNC_L, SYMMETRY_ORIGIN,
		StandardCalculator(lambda_trig_or_trig_orbit_fp, other_julia_per_pixel_fp, lambda_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_OR_FUNC_L, s_julia_fnorfn__name + 1,
		NamedParameterList()
			.Add(s_parameter_real)
			.Add(s_parameter_imag)
			.Add("Function Shift Value", 8.0),
		IDHELP_FUNC_OR_FUNC, IDHELP_JULIA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L,
		FRACTYPE_JULIA_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS,
		StandardCalculator(julia_trig_or_trig_orbit, julia_per_pixel_l, julia_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_JULIA_FUNC_OR_FUNC_FP, s_julia_fnorfn__name,
		NamedParameterList()
			.Add(s_parameter_real)
			.Add(s_parameter_imag)
			.Add("Function Shift Value", 8),
		IDHELP_FUNC_OR_FUNC, IDHELP_JULIA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP,
		FRACTYPE_JULIA_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS,
		StandardCalculator(julia_trig_or_trig_orbit_fp, other_julia_per_pixel_fp, julia_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L, s_manlam_fnorfn__name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add("Function Shift Value", 10),
		IDHELP_FUNC_OR_FUNC, IDHELP_MANDELBROT_LAMBDA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_LAMBDA_FUNC_OR_FUNC_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_trig_or_trig_orbit, mandelbrot_per_pixel_l, mandelbrot_lambda_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP, s_manlam_fnorfn__name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add("Function Shift Value", 10),
		IDHELP_FUNC_OR_FUNC, IDHELP_MANDELBROT_LAMBDA_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(lambda_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_lambda_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L, s_mandel_fnorfn__name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add("Function Shift Value", 0.5),
		IDHELP_FUNC_OR_FUNC, IDHELP_MANDELBROT_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_JULIA_FUNC_OR_FUNC_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(julia_trig_or_trig_orbit, mandelbrot_per_pixel_l, mandelbrot_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP, s_mandel_fnorfn__name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add("Function Shift Value", 0.5),
		IDHELP_FUNC_OR_FUNC, IDHELP_MANDELBROT_FUNC_OR_FUNC_TYPE, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_JULIA_FUNC_OR_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(julia_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_trig_or_trig_setup),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_MAY_L, s_bifmay_name + 1,
		NamedParameterList()
			.Add(s_filter_cycles, 300.0)
			.Add(s_seed_population, 0.9)
			.Add("Beta >= 2", 5.0),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_MAY_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_MAY, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_may, bifurcation_may_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_BIFURCATION_MAY, s_bifmay_name,
		NamedParameterList()
			.Add(s_filter_cycles, 300.0)
			.Add(s_seed_population, 0.9)
			.Add("Beta >= 2", 5.0),
		IDHELP_BIFURCATION, IDHELP_BIFURCATION_MAY_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_MAY_L, SYMMETRY_NONE,
		BifurcationCalculator(bifurcation_may_fp, bifurcation_may_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_HALLEY_MP, s_halley_name + 1,
		NamedParameterList()
			.Add(s_order, 6.0)
			.Add(s_real_relaxation_coefficient, 1.0)
			.Add(s_epsilon, 0.0001)
			.Add(s_imag_relaxation_coefficient),
		IDHELP_HALLEY, IDHELP_HALLEY_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HALLEY, SYMMETRY_XY_AXIS,
		StandardCalculator(halley_orbit_mpc, halley_per_pixel_mpc, halley_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_HALLEY, s_halley_name,
		NamedParameterList()
			.Add(s_order, 6.0)
			.Add(s_real_relaxation_coefficient, 1.0)
			.Add(s_epsilon, 0.0001)
			.Add(s_imag_relaxation_coefficient),
		IDHELP_HALLEY, IDHELP_HALLEY_TYPE, 0,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HALLEY_MP, SYMMETRY_XY_AXIS,
		StandardCalculator(halley_orbit_fp, halley_per_pixel, halley_setup),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_DYNAMIC_FP, "dynamic",
		NamedParameterList()
			.Add("+# of intervals (<0 = connect)", 50.0)
			.Add("time step (<0 = Euler)", 0.1)
			.Add("a", 1.0)
			.Add("b", 3.0),
		IDHELP_DYNAMIC_SYSTEM, IDHELP_DYNAMIC_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_1_FUNCTION,
		-20.0f, 20.0f, -20.0f, 20.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		FractalCalculator(VF(dynamic_orbit_fp), 0, dynamic_2d_setup_fp, dynamic_2d_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_QUATERNION_FP, "quat",
		NamedParameterList()
			.Add("notused").Add("notused").Add("cj").Add("ck"),
		IDHELP_QUATERNION, IDHELP_QUATERNION_TYPE, FRACTALFLAG_JULIBROT,
		0, FRACTYPE_QUATERNION_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS,
		StandardCalculator(quaternion_orbit_fp, quaternion_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_QUATERNION_JULIA_FP, "quatjul",
		NamedParameterList()
			.Add("c1", -0.745)
			.Add("ci")
			.Add("cj", 0.113)
			.Add("ck", 0.05),
		IDHELP_QUATERNION, IDHELP_QUATERNION_JULIA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_MORE_PARAMETERS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_QUATERNION_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		StandardCalculator(quaternion_orbit_fp, quaternion_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_CELLULAR, "cellular",
		NamedParameterList()
			.Add("#Initial String | 0 = Random | -1 = Reuse Last Random", 11.0)
			.Add("#Rule = # of digits (see below) | 0 = Random", 3311100320.0)
			.Add("+Type (see below)", 41.0)
			.Add("#Starting Row Number"),
		IDHELP_CELLULAR, IDHELP_CELLULAR_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		PerImageCalculator(cellular_setup, cellular),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_JULIBROT_FP, s_julibrot_name,
		NamedParameterList(),
		IDHELP_JULIBROT, SPECIALHF_JULIBROT, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE | FRACTALFLAG_NOT_RESUMABLE,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_JULIBROT, SYMMETRY_NONE,
		FractalCalculator(julia_orbit_fp, julibrot_per_pixel_fp, julibrot_setup, standard_4d_fractal_fp),
		ORBIT_BAILOUT_STANDARD),

#ifdef RANDOM_RUN
	FractalTypeSpecificData2(FRACTYPE_INVERSE_JULIA, s_julia_inverse_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, -0.11)
			.Add(s_parameter_imag, 0.6557)
			.Add(s_max_hits_per_pixel, 4.0)
			.Add("Random Run Interval", 1024),
		IDHELP_INVERSE_JULIA, IDHELP_INVERSE_JULIA_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		24, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA_FP, SYMMETRY_NONE,
		FractalCalculator(Linverse_julia_orbit, 0, orbit_3d_setup, inverse_julia_per_image),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_INVERSE_JULIA_FP, s_julia_inverse_name,
		NamedParameterList()
			.Add(s_parameter_real, -0.11)
			.Add(s_parameter_imag, 0.6557)
			.Add(s_max_hits_per_pixel, 4.0)
			.Add("Random Run Interval", 1024.0),
		IDHELP_INVERSE_JULIA, IDHELP_INVERSE_JULIA_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA, SYMMETRY_NONE,
		FractalCalculator(Minverse_julia_orbit, 0, orbit_3d_setup_fp, inverse_julia_per_image),
		ORBIT_BAILOUT_NONE),
#else
	FractalTypeSpecificData2(FRACTYPE_INVERSE_JULIA, s_julia_inverse_name + 1,
		NamedParameterList()
			.Add(s_parameter_real, -0.11)
			.Add(s_parameter_imag, 0.6557)
			.Add(s_max_hits_per_pixel, 4)
			.Add("", 1024),
		IDHELP_INVERSE_JULIA, IDHELP_INVERSE_JULIA_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		24, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA_FP, SYMMETRY_NONE,
		FractalCalculator(Linverse_julia_orbit, 0, orbit_3d_setup, inverse_julia_per_image),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_INVERSE_JULIA_FP, s_julia_inverse_name,
		NamedParameterList()
			.Add(s_parameter_real, -0.11)
			.Add(s_parameter_imag, 0.6557)
			.Add(s_max_hits_per_pixel, 4)
			.Add("", 1024),
		IDHELP_INVERSE_JULIA, IDHELP_INVERSE_JULIA_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA, SYMMETRY_NONE,
		FractalCalculator(Minverse_julia_orbit, 0, orbit_3d_setup_fp, inverse_julia_per_image),
		ORBIT_BAILOUT_NONE),
#endif
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_CLOUD, "mandelcloud",
		NamedParameterList()
			.Add("+# of intervals (<0 = connect)", 50),
		IDHELP_MANDELBROT_CLOUD, IDHELP_MANDELBROT_CLOUD_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		FractalCalculator(VF(mandel_cloud_orbit_fp), 0, dynamic_2d_setup_fp, dynamic_2d_fp),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_PHOENIX, s_phoenix_name + 1,
		NamedParameterList()
			.Add(s_p1_real, 0.56667)
			.Add(s_p2_real, -0.5)
			.Add(s_degree_z, 0),
		IDHELP_PHOENIX, IDHELP_PHOENIX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX,
		FRACTYPE_PHOENIX_FP, SYMMETRY_X_AXIS,
		StandardCalculator(phoenix_orbit, phoenix_per_pixel, phoenix_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_PHOENIX_FP, s_phoenix_name,
		NamedParameterList()
			.Add(s_p1_real, 0.56667)
			.Add(s_p2_real, -0.5)
			.Add(s_degree_z, 0),
		IDHELP_PHOENIX, IDHELP_PHOENIX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_FP,
		FRACTYPE_PHOENIX, SYMMETRY_X_AXIS,
		StandardCalculator(phoenix_orbit_fp, phoenix_per_pixel_fp, phoenix_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_PHOENIX, s_mandphoenix_name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_degree_z),
		IDHELP_PHOENIX, IDHELP_MANDELBROT_PHOENIX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_PHOENIX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_FP, SYMMETRY_NONE,
		StandardCalculator(phoenix_orbit, mandelbrot_phoenix_per_pixel, mandelbrot_phoenix_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_PHOENIX_FP, s_mandphoenix_name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_degree_z),
		IDHELP_PHOENIX, IDHELP_MANDELBROT_PHOENIX_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_PHOENIX_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX, SYMMETRY_NONE,
		StandardCalculator(phoenix_orbit_fp, mandelbrot_phoenix_per_pixel_fp, mandelbrot_phoenix_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_HYPERCOMPLEX_FP, "hypercomplex",
		NamedParameterList().Add("notused").Add("notused").Add("cj").Add("ck"),
		IDHELP_HYPERCOMPLEX, IDHELP_HYPERCOMPLEX_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_1_FUNCTION,
		0, FRACTYPE_HYPERCOMPLEX_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS,
		StandardCalculator(hyper_complex_orbit_fp, quaternion_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_HYPERCOMPLEX_JULIA_FP, "hypercomplexj",
		NamedParameterList()
			.Add("c1", -0.745)
			.Add("ci")
			.Add("cj", 0.113)
			.Add("ck", 0.05),
		IDHELP_HYPERCOMPLEX, IDHELP_HYPERCOMPLEX_JULIA_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_1_FUNCTION | FRACTALFLAG_MORE_PARAMETERS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_HYPERCOMPLEX_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		StandardCalculator(hyper_complex_orbit_fp, quaternion_julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_TRIG_L),
	FractalTypeSpecificData2(FRACTYPE_FROTHY_BASIN, s_frothybasin_name + 1,
		NamedParameterList()
			.Add(s_frothy_mapping, 1.0)
			.Add(s_frothy_shade)
			.Add(s_frothy_a_value, 1.028713768218725),
		IDHELP_FROTHY_BASINS, IDHELP_FROTHY_BASIN_TYPE, FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.8f, 2.8f, -2.355f, 1.845f,
		28, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FROTHY_BASIN_FP, SYMMETRY_NONE,
		FractalCalculator(froth_per_orbit, froth_per_pixel, froth_setup, froth_calc),
		ORBIT_BAILOUT_FROTHY_BASIN),
	FractalTypeSpecificData2(FRACTYPE_FROTHY_BASIN_FP, s_frothybasin_name,
		NamedParameterList()
			.Add(s_frothy_mapping, 1.0)
			.Add(s_frothy_shade)
			.Add(s_frothy_a_value, 1.028713768218725),
		IDHELP_FROTHY_BASINS, IDHELP_FROTHY_BASIN_TYPE, FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.8f, 2.8f, -2.355f, 1.845f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FROTHY_BASIN, SYMMETRY_NONE,
		FractalCalculator(froth_per_orbit, froth_per_pixel, froth_setup, froth_calc),
		ORBIT_BAILOUT_FROTHY_BASIN),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_4_FP, s_mandel4_name,
		NamedParameterList().Add(s_real_z0).Add(s_imag_z0),
		IDHELP_QUARTIC_MANDELBROT_JULIA, IDHELP_MANDELBROT_4_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_JULIA_4_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_4, SYMMETRY_X_AXIS_NO_PARAMETER,
		StandardCalculator(mandel4_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_JULIA_4_FP, s_julia4_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.6)
			.Add(s_parameter_imag, 0.55),
		IDHELP_QUARTIC_MANDELBROT_JULIA, IDHELP_JULIA_4_TYPE, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_4_FP,
		FRACTYPE_JULIA_4, SYMMETRY_ORIGIN,
		StandardCalculator(mandel4_orbit_fp, julia_per_pixel_fp, julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_MANDELBROT_FP, s_marksmandel_name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_exponent, 1.0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_MANDELBROT_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_MARKS_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT, SYMMETRY_NONE,
		StandardCalculator(marks_lambda_orbit_fp, marks_mandelbrot_per_pixel_fp, mandelbrot_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MARKS_JULIA_FP, s_marksjulia_name,
		NamedParameterList()
			.Add(s_parameter_real, 0.1)
			.Add(s_parameter_imag, 0.9)
			.Add(s_exponent, 1.0),
		IDHELP_MARK_PETERSON, IDHELP_MARKS_JULIA_TYPE, FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT_FP,
		FRACTYPE_MARKS_JULIA, SYMMETRY_ORIGIN,
		StandardCalculator(marks_lambda_orbit_fp, julia_per_pixel_fp, marks_julia_setup_fp),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_ICON, "icons",
		NamedParameterList()
			.Add("Lambda", -2.34)
			.Add("Alpha", 2.0)
			.Add("Beta", 0.2)
			.Add("Gamma", 0.1),
		IDHELP_ICON, IDHELP_ICON_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_MORE_PARAMETERS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(icon_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_ICON_3D, "icons3d",
		NamedParameterList()
			.Add("Lambda", -2.34)
			.Add("Alpha", 2.0)
			.Add("Beta", 0.2)
			.Add("Gamma", 0.1),
		IDHELP_ICON, IDHELP_ICON_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_MORE_PARAMETERS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit3DFpCalculator(VF(icon_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_PHOENIX_COMPLEX, s_phoenixcplx_name + 1,
		NamedParameterList()
			.Add(s_p1_real, 0.2)
			.Add(s_p1_imag)
			.Add(s_p2_real, 0.3)
			.Add(s_p2_imag),
		IDHELP_PHOENIX, IDHELP_PHOENIX_COMPLEX_TYPE, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_COMPLEX,
		FRACTYPE_PHOENIX_COMPLEX_FP, SYMMETRY_ORIGIN,
		StandardCalculator(phoenix_complex_orbit, phoenix_per_pixel, phoenix_complex_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_PHOENIX_COMPLEX_FP, s_phoenixcplx_name,
		NamedParameterList()
			.Add(s_p1_real, 0.2)
			.Add(s_p1_imag)
			.Add(s_p2_real, 0.3)
			.Add(s_p2_imag),
		IDHELP_PHOENIX, IDHELP_PHOENIX_COMPLEX_TYPE, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP,
		FRACTYPE_PHOENIX_COMPLEX, SYMMETRY_ORIGIN,
		StandardCalculator(phoenix_complex_orbit_fp, phoenix_per_pixel_fp, phoenix_complex_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_PHOENIX_COMPLEX, s_mandphoenixcplx_name + 1,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_p2_real, 0.5)
			.Add(s_p2_imag),
		IDHELP_PHOENIX, IDHELP_MANDELBROT_PHOENIX_COMPLEX_TYPE, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_PHOENIX_COMPLEX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP, SYMMETRY_X_AXIS,
		StandardCalculator(phoenix_complex_orbit, mandelbrot_phoenix_per_pixel, mandelbrot_phoenix_complex_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP, s_mandphoenixcplx_name,
		NamedParameterList()
			.Add(s_real_z0)
			.Add(s_imag_z0)
			.Add(s_p2_real, 0.5)
			.Add(s_p2_imag),
		IDHELP_PHOENIX, IDHELP_MANDELBROT_PHOENIX_COMPLEX_TYPE, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_PHOENIX_COMPLEX_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_COMPLEX, SYMMETRY_X_AXIS,
		StandardCalculator(phoenix_complex_orbit_fp, mandelbrot_phoenix_per_pixel_fp, mandelbrot_phoenix_complex_setup),
		ORBIT_BAILOUT_STANDARD),
	FractalTypeSpecificData2(FRACTYPE_ANT, "ant",
		NamedParameterList()
			.Add("#Rule String (1's and non-1's, 0 rand)", 1100.0)
			.Add("#Maxpts", 1.0E9)
			.Add("+Numants (max 256)", 1.0)
			.Add("+Ant type (1 or 2)", 1.0),
		IDHELP_ANT, IDHELP_ANT_TYPE, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_MORE_PARAMETERS,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandaloneCalculator(ant),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_CHIP, "chip",
		NamedParameterList()
			.Add("a", -15.0)
			.Add("b", -19.0)
			.Add("c", 1.0),
		IDHELP_MARTIN, IDHELP_CHIP_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-760.0f, 760.0f, -570.0f, 570.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(chip_2d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_QUADRUP_TWO, "quadruptwo",
		NamedParameterList()
			.Add("a", 34.0)
			.Add("b", 1.0)
			.Add("c", 5.0),
		IDHELP_MARTIN, IDHELP_QUADRUP_TWO_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-82.93367f, 112.2749f, -55.76383f, 90.64257f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(quadrup_two_2d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_THREE_PLY, "threeply",
		NamedParameterList()
			.Add("a", -55.0)
			.Add("b", -1.0)
			.Add("c", -42.0),
		IDHELP_MARTIN, IDHELP_THREE_PLY_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-8000.0f, 8000.0f, -6000.0f, 6000.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(three_ply_2d_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_VOLTERRA_LOTKA, "volterra-lotka",
		NamedParameterList()
			.Add("h", 0.739)
			.Add("p", 0.739),
		IDHELP_VOLTERRA_LOTKA, IDHELP_VOLTERRA_LOTKA_TYPE, 0,
		0.0f, 6.0f, 0.0f, 4.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(volterra_lotka_orbit_fp, other_julia_per_pixel_fp, volterra_lotka_setup),
		256),
	FractalTypeSpecificData2(FRACTYPE_ESCHER_JULIA, "escher_julia",
		NamedParameterList()
			.Add(s_parameter_real, 0.32)
			.Add(s_parameter_imag, 0.043),
		IDHELP_ESCHER, IDHELP_ESCHER_JULIA_TYPE, 0,
		-1.6f, 1.6f, -1.2f, 1.2f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		StandardCalculator(escher_orbit_fp, julia_per_pixel_fp, standard_setup),
		ORBIT_BAILOUT_STANDARD),

	// From Pickovers' "Chaos in Wonderland"
	// included by Humberto R. Baptista
	// code adapted from king.cpp bt James Rankin
	FractalTypeSpecificData2(FRACTYPE_LATOOCARFIAN, "latoocarfian",
		NamedParameterList()
			.Add("a", -0.966918)
			.Add("b", 2.879879)
			.Add("c", 0.765145)
			.Add("d", 0.744728),
		IDHELP_LATOOCARFIAN, IDHELP_LATOOCARFIAN_TYPE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_4_FUNCTIONS,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		Orbit2DFpCalculator(VF(latoo_orbit_fp)),
		ORBIT_BAILOUT_NONE),
	FractalTypeSpecificData2(FRACTYPE_MANDELBROT_MIX4, "mandelbrotmix4",
		NamedParameterList()
			.Add(s_p1_real, 0.05)
			.Add(s_p1_imag, 3.0)
			.Add(s_p2_real, -1.5)
			.Add(s_p2_imag, -2),
		IDHELP_MANDELBROT_MIX_4, IDHELP_MANDELBROT_MIX4_TYPE,
		FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_1_FUNCTION | FRACTALFLAG_MORE_PARAMETERS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		StandardCalculator(mandelbrot_mix4_orbit_fp, mandelbrot_mix4_per_pixel_fp, mandelbrot_mix4_setup),
		ORBIT_BAILOUT_STANDARD)
};

