#if !defined(COMMAND_PARSER_H)
#define COMMAND_PARSER_H

#define NON_NUMERIC -32767

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
