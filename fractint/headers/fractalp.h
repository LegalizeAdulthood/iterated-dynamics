#if !defined(FRACTAL_P_H)
#define FRACTAL_P_H

struct alternate_math
{
	int type;                    /* index in fractalname of the fractal */
	int math;                    /* kind of math used */
	int (*orbitcalc)();      /* function that calculates one orbit */
	int (*per_pixel)();      /* once-per-pixel init */
	int (*per_image)();      /* once-per-image setup */
};

extern bool type_has_parameter(int type, int parameter);
extern const char *parameter_prompt(int type, int parameter);
extern bool parameter_not_used(int);
extern alternate_math *find_alternate_math(int math_type);

#endif
