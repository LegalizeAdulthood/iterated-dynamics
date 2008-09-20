#if !defined(ODS_H)
#define ODS_H

#if defined(RT_VERBOSE)
#define ODS(text_)						ods(__FILE__, __LINE__, text_)
#define ODS1(fmt_, arg_)				ods(__FILE__, __LINE__, fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)			ods(__FILE__, __LINE__, fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)		ods(__FILE__, __LINE__, fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)		ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)	ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4, _5)
extern void ods(const char *file, unsigned int line, const char *format, ...);
#else
#define ODS(text_)
#define ODS1(fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)
#endif

#endif
