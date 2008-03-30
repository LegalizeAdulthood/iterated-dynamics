#pragma once

#include "Externals.h"

/*
 * Generate a gaussian distributed number.
 * The right half of the distribution is folded onto the lower half.
 * That is, the curve slopes up to the peak and then drops to 0.
 * The larger slope is, the smaller the standard deviation.
 * The values vary from 0 + offset to range + offset, with the peak
 * at range + offset.
 * To make this more complicated, you only have a
 * 1 in Distribution*(1-Probability/Range*con) + 1 chance of getting a
 * Gaussian; otherwise you just get offset.
 */
class GaussianDistribution
{
public:
	GaussianDistribution(int distribution, int offset, int slope, long constant) : _distribution(distribution),
		_offset(offset),
		_slope(slope),
		_constant(constant)
	{
	}

	int operator()(int probability, int range);

private:
	int _distribution;
	int _offset;
	int _slope;
	long _constant;
};

extern int gaussian_number(int probability, int range, Externals &externs = g_externs);
