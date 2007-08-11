#if !defined(IFS_PARSER_H)
#define IFS_PARSER_H

#include <string>

class IFSParserImpl;

class IFSParser
{
public:
	static IFSParser StackInstance()
	{
		return IFSParser();
	}
	~IFSParser();

	bool Parse(const std::string &text);
	int Count() const;

private:
	IFSParser();
	IFSParserImpl *m_impl;
};

#endif
