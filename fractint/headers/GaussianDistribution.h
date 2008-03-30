#if !defined(GAUSSIAN_DISTRIBUTION_H)
#define GAUSSIAN_DISTRIBUTION_H

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
	static int Evaluate(int probability, int range);
	static void SetDistribution(int value)	{ s_distribution = value; }
	static void SetOffset(int value)		{ s_offset = value; }
	static void SetSlope(int value)			{ s_slope = value; }
	static void SetConstant(long value)		{ s_constant = value; }

private:
	static int s_distribution;
	static int s_offset;
	static int s_slope;
	static long s_constant;
};

#endif
