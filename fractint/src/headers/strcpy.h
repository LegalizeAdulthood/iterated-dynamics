#if !defined(STRCPY_H)
#define STRCPY_H

inline void strcpy(char *dest, const boost::format &source)
{
	strcpy(dest, str(source).c_str());
}

#endif
