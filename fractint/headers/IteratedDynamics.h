#if !defined(ITERATED_DYNAMICS_H)
#define ITERATED_DYNAMICS_H

class IteratedDynamics
{
public:
	virtual ~IteratedDynamics() { }

	virtual int Main() = 0;
};

#endif
