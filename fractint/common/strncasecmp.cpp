#include <locale>
#include "strncasecmp.h"

#if !defined(XFRACT)
// case independent version of strncmp
int strncasecmp(const char *s, const char *t, int ct)
{
	std::locale C("C");
	for (; (tolower(*s, C) == tolower(*t, C)) && --ct; s++, t++)
	{
		if (*s == '\0')
		{
			return 0;
		}
	}
	return tolower(*s, C) - tolower(*t, C);
}
#endif

