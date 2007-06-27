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
	int setup();
	virtual int orbit();

//protected:
//	struct MP m_degree_minus_1_over_degree_mp;
//	struct MP m_root_over_degree_mp;

private:
	double m_root_over_degree;
	double m_degree_minus_1_over_degree;
};

//class NewtonMPC : public Newton
//{
//public:
//	virtual int orbit();
//};

class NewtonComplex
{
public:
	int setup();
	int orbit();
};

extern int cdecl    newton2_orbit();
extern int newton_orbit_mpc();
extern int newton_setup();
extern int complex_newton_setup();
extern int complex_newton();

#endif
