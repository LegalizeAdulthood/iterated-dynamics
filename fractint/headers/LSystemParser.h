#if !defined(L_SYSTEM_PARSER_H)
#define L_SYSTEM_PARSER_H

#include <string>
#include <vector>

class LSystemParserImpl;

class LSystemEntry
{
public:
	LSystemEntry(const std::string &id)
		: m_id(id)
	{
	}
	~LSystemEntry()
	{
	}

	const std::string &Id() const { return m_id; }
	int Angle() const { return 6; }
	const std::string Axiom() const { return "F--F--F"; }
	int ProductionCount() const { return 1; }
	std::string Production(int index) const { return "F=F+F--F+F"; }

private:
	std::string m_id;
};

class LSystemParser
{
public:
	static LSystemParser StackInstance()
	{
		return LSystemParser();
	}
	~LSystemParser();

	bool Parse(const std::string &text);
	int Count() const;
	const LSystemEntry *Entry(int index) const;

private:
	LSystemParser();
	LSystemParserImpl *m_impl;
};

#endif
