#pragma once

#include "calcfrac.h"

class DiffusionScan : WorkListScanner
{
public:
	virtual void Scan() = 0;
	virtual std::string CalculationTime() const = 0;
	virtual std::string Status() const = 0;

protected:
	virtual ~DiffusionScan() { }
};

extern DiffusionScan &g_diffusionScan;
