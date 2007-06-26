#if !defined(FINITE_ATTRACTOR_H)
#define FINITE_ATTRACTOR_H

enum FiniteAttractorType
{
	FINITE_ATTRACTOR_NO = 0,
	FINITE_ATTRACTOR_YES = 1,
	FINITE_ATTRACTOR_PHASE = -1
};

extern int g_num_attractors;
extern int g_attractor_period[];
extern int g_finite_attractor;

extern ComplexD g_attractors[];
extern double g_attractor_radius_fp;

extern ComplexL g_attractors_l[];
extern long g_attractor_radius_l;

#endif
