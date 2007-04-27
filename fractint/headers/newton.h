#if !defined(NEWTON_H)
#define NEWTON_H

class Newton
{
public:
	int setup();
	virtual int orbit();
};

class NewtonMPC : public Newton
{
public:
	virtual int orbit();
};

class NewtonComplex
{
public:
	int setup();
	int orbit();
};

extern int cdecl    newton2_orbit(void);
extern int newton_orbit_mpc(void);
extern int newton_setup(void);
extern int complex_newton_setup(void);
extern int complex_newton(void);

#endif
