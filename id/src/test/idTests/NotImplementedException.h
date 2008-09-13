#if !defined(NOT_IMPLEMENTED_EXCEPTION_H)
#define NOT_IMPLEMENTED_EXCEPTION_H

#include <stdexcept>

class not_implemented : public std::exception
{
public:
	explicit not_implemented(char const *what) : std::exception(what)
	{
	}
	explicit not_implemented() : std::exception()
	{
	}
	virtual ~not_implemented()
	{
	}
};

#endif
