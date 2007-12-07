#pragma once

inline void strcpy(char *dest, const boost::format &source)
{
	strcpy(dest, str(source).c_str());
}
