#if !defined(MATH_UTIL_H)
#define MATH_UTIL_H

class MathUtil
{
public:
	static const double Pi;

	static double RadiansToDegrees(double radians)
	{
		return radians*180.0/Pi;
	}
	static double DegreesToRadians(double degrees)
	{
		return degrees*Pi/180.0;
	}

	template <typename T>
	static T Clamp(T value, T minimum, T maximum)
	{
		if (value < minimum)
		{
			return minimum;
		}
		if (value > maximum)
		{
			return maximum;
		}
		return value;
	}
};

#endif
