#pragma once

class SynchronousOrbitScannerImpl : public SynchronousOrbitScanner
{
public:
	SynchronousOrbitScannerImpl() : SynchronousOrbitScanner()
	{
	}
	virtual ~SynchronousOrbitScannerImpl()
	{
	}

	virtual void Execute();

private:
	void soi_double();
	void soi_long_double();
};
