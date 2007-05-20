#if !defined(COMMAND_PARSER_H)
#define COMMAND_PARSER_H

#include "cmdfiles.h"

#define NON_NUMERIC -32767

struct cmd_context
{
	const char *curarg;
	int     yesnoval[16];                /* 0 if 'n', 1 if 'y', -1 if not */
	int     numval;                      /* numeric value of arg      */
	char    *value;                      /* pointer to variable value */
	char    charval[16];                 /* first character of arg    */
	int     totparms;                    /* # of / delimited parms    */
	int     valuelen;                    /* length of value           */
	int mode;
	const char *variable;
	int     intval[64];                  /* pre-parsed integer parms  */
	double  floatval[16];                /* pre-parsed floating parms */
	char    *floatvalstr[16];            /* pointers to float vals */
	int     intparms;                    /* # of / delimited ints     */
	int     floatparms;                  /* # of / delimited floats   */
};

/* process_command(), AbstractCommandParser::parse() return values */
class Command
{
public:
	enum
	{
		Error = -1,
		OK = 0,
		FractalParameter = 1,
		ThreeDParameter = 2,
		ThreeDYes = 4,
		Reset = 8
	};
};

class AbstractCommandParser
{
public:
	virtual ~AbstractCommandParser() {}
	virtual int parse(const cmd_context &context) = 0;
};

template<typename T>
class FlagParser : public AbstractCommandParser
{
public:
	FlagParser<T>(T &flag, int result) : m_flag(flag), m_result(result)
	{}
	virtual ~FlagParser<T>() {}
	virtual int parse(const cmd_context &context)
	{
		if (context.yesnoval[0] < 0)
		{
			return bad_arg(context.curarg);
		}
		m_flag = (context.yesnoval[0] != 0);
		return m_result;
	}

private:
	T &m_flag;
	int m_result;
};

#endif
