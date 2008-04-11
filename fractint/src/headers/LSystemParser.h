#if !defined(L_SYSTEM_PARSER_H)
#define L_SYSTEM_PARSER_H

#include <string>
#include <vector>

class LSystemParserImpl;

class LSystemProduction
{
public:
	LSystemProduction(const std::string &symbol, const std::string &production)
		: m_symbol(symbol),
		m_production(production)
	{
	}
	~LSystemProduction()
	{
	}
	const std::string &Symbol() const { return m_symbol; }
	const std::string &Production() const { return m_production; }

private:
	std::string m_symbol;
	std::string m_production;
};

class LSystemEntry
{
public:
	LSystemEntry(const std::string &id, int angle, const std::string axiom,
		const std::vector<LSystemProduction> &productions)
		: m_id(id),
		m_angle(angle),
		m_axiom(axiom),
		m_productions(productions)
	{
	}
	~LSystemEntry()
	{
	}

	const std::string &Id() const { return m_id; }
	int Angle() const { return m_angle; }
	const std::string Axiom() const { return m_axiom; }
	int ProductionCount() const { return int(m_productions.size()); }
	LSystemProduction Production(int index) const { return m_productions[index]; }

private:
	std::string m_id;
	int m_angle;
	std::string m_axiom;
	std::vector<LSystemProduction> m_productions;
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
