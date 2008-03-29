#pragma once

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
