#if !defined(BUSY_H)
#define BUSY_H

extern bool g_busy;

class BusyMarker
{
public:
	BusyMarker()
	{
		g_busy = true;
	}
	~BusyMarker()
	{
		g_busy = false;
	}
};

#endif
