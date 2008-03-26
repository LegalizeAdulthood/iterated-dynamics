#if !defined(SOLID_GUESS_H)
#define SOLID_GUESS_H

class SolidGuessScanner
{
public:
	virtual void Execute() = 0;

protected:
	virtual ~SolidGuessScanner() { }
};

extern SolidGuessScanner &g_solidGuessScanner;

#endif
