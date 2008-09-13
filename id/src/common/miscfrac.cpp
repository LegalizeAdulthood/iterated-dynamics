//
//	Miscellaneous fractal-specific code
//
#include <boost/format.hpp>

std::string precision_format(const char *specifier, int precision)
{
	return str(boost::format("%%.%d%s") % precision % specifier);
}

