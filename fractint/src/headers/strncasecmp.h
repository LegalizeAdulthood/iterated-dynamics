#if !defined(STRNCASECMP_H)
#define STRNCASECMP_H

#if !defined(XFRACT) // Unix should have this in string.h
extern int strncasecmp(const char *, const char *, int);
#endif

#endif
