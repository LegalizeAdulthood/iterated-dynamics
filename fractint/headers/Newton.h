#if !defined(NEWTON_H)
#define NEWTON_H

class Newton
{
public:
	Newton()
		: m_root_over_degree(0.0),
		m_degree_minus_1_over_degree(0.0)
	{
	}
	virtual ~Newton()
	{
	}
	bool setup();
	virtual int orbit();

private:
	double m_root_over_degree;
	double m_degree_minus_1_over_degree;
};

class NewtonComplex
{
public:
	bool setup();
	int orbit();
};

extern int newton2_orbit();
extern int newton_orbit_mpc();
extern bool newton_setup();
extern bool complex_newton_setup();
extern int complex_newton();

#endif
