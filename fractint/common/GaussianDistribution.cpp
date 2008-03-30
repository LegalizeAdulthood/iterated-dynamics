#include <string>
#include "port.h"
#include "prototyp.h"
#include "GaussianDistribution.h"

int GaussianDistribution::operator()(int probability, int range)
{
	long p = divide(long(probability) << 16, long(range) << 16, 16);
	p = multiply(p, _constant, 16);
	p = multiply(long(_distribution) << 16, p, 16);
	if (!(rand15() % (_distribution - int(p >> 16) + 1)))
	{
		long accum = 0;
		for (int n = 0; n < _slope; n++)
		{
			accum += rand15();
		}
		accum /= _slope;
		int r = int(multiply(long(range) << 15, accum, 15) >> 14);
		r -= range;
		if (r < 0)
		{
			r = -r;
		}
		return range - r + _offset;
	}
	return _offset;
}

int gaussian_number(int probability, int range, Externals &externs)
{
	GaussianDistribution d(externs.GaussianDistribution(),
		externs.GaussianOffset(), externs.GaussianSlope(),
		externs.GaussianConstant());
	return d(probability, range);
}
