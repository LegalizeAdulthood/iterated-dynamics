#include <string>
#include "port.h"
#include "prototyp.h"
#include "GaussianDistribution.h"

int GaussianDistribution::s_distribution = 30;
int GaussianDistribution::s_offset = 0;
int GaussianDistribution::s_slope = 25;
long GaussianDistribution::s_constant = 0;

int GaussianDistribution::Evaluate(int probability, int range)
{
	long p = divide(long(probability) << 16, long(range) << 16, 16);
	p = multiply(p, s_constant, 16);
	p = multiply(long(s_distribution) << 16, p, 16);
	if (!(rand15() % (s_distribution - int(p >> 16) + 1)))
	{
		long accum = 0;
		for (int n = 0; n < s_slope; n++)
		{
			accum += rand15();
		}
		accum /= s_slope;
		int r = int(multiply(long(range) << 15, accum, 15) >> 14);
		r -= range;
		if (r < 0)
		{
			r = -r;
		}
		return range - r + s_offset;
	}
	return s_offset;
}
