#if !defined(HALLEY_H)
#define HALLEY_H

class Halley
{
public:
	Halley()
		: m_a_plus_1_degree(0),
		m_a_plus_1(0)
	{
	}
	~Halley()
	{
	}

	virtual int setup();
	virtual int orbit();
	virtual int per_pixel();

protected:
	int					m_a_plus_1_degree;
	int					m_a_plus_1;

private:
	int bail_out();
};

extern int halley_setup();
extern int halley_orbit_fp();
extern int halley_per_pixel();
extern int halley_orbit_mpc();
extern int halley_per_pixel_mpc();

#endif
