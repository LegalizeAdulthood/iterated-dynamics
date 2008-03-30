#if !defined(SYCHRONOUS_ORBIT_SCANNER_IMPL_H)
#define SYCHRONOUS_ORBIT_SCANNER_IMPL_H

#include "calcfrac.h"

class SynchronousOrbitScannerImpl : public WorkListScanner
{
public:
	SynchronousOrbitScannerImpl() : WorkListScanner()
	{
	}
	virtual ~SynchronousOrbitScannerImpl()
	{
	}

	virtual void Scan();

private:
	void soi_double();
	void soi_long_double();
};

#endif
