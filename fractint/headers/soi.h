#if !defined(SOI_H)
#define SOI_H

class SynchronousOrbitScanner
{
public:
	virtual void Execute() = 0;

protected:
	virtual ~SynchronousOrbitScanner() { }
};

extern SynchronousOrbitScanner &g_synchronousOrbitScanner;

#endif
