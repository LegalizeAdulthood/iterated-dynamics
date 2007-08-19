#include <algorithm>
#include <cassert>
#include <iomanip>
#include <string>
#include <vector>

#include <boost/spirit.hpp>

#include "LSystemParser.h"

using namespace std;
using namespace boost::spirit;

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

class LSystemParserImpl
{
public:
	LSystemParserImpl()
		: m_id(),
		m_entries()
	{
	}
	~LSystemParserImpl()
	{
	}

	void AssignId(string::const_iterator first, string::const_iterator last)
	{
		m_id.assign(first, last);
	}
	void AssignAngle(int angle)
	{
		m_angle = angle;
	}
	void AssignAxiom(string::const_iterator first, string::const_iterator last)
	{
		m_axiom.assign(first, last);
	}
	void AssignProduction(string::const_iterator first, string::const_iterator last)
	{
		m_production.assign(first, last);
		PushProduction();
	}
	void AssignEntry(string::const_iterator first, string::const_iterator last)
	{
		m_entries.push_back(new LSystemEntry(m_id, m_angle, m_axiom, m_productions));
		m_id = "";
		m_angle = 0;
		m_axiom = "";
		m_productions.clear();
	}
	void AssignProductionSymbol(const std::string &symbol)
	{
		m_productionSymbol = symbol;
	}
	void AssignRule(const std::string &production)
	{
		m_production = production;
		PushProduction();
	}

	int EntriesParsed() const
	{
		return int(m_entries.size());
	}
	const LSystemEntry *Entry(int index) const
	{
		return m_entries[index];
	}

private:
	void PushProduction()
	{
		m_productions.push_back(LSystemProduction(m_productionSymbol, m_production));
		m_productionSymbol = "";
		m_production = "";
	}

	string m_id;
	int m_angle;
	string m_axiom;
	string m_production;
	string m_productionSymbol;
	vector<LSystemProduction> m_productions;
	vector<LSystemEntry *> m_entries;
};

static LSystemParserImpl *s_impl = NULL;

static void AssignId(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignId(first, last); }
static void AssignEntry(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignEntry(first, last); }
static void AssignAngle(int value)
{ s_impl->AssignAngle(value); }
static void AssignAxiom(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignAxiom(first, last); }
static void AssignProductionSymbol(char symbol) //string::const_iterator first, string::const_iterator last)
{
	//std::string symbol;
	//symbol.assign(first, last);
	//if (symbol.length() > 1)
	//{
	//	return;
	//}
	char text[2] = { symbol, 0 };
	s_impl->AssignProductionSymbol(text);
}
static void AssignProduction(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignProduction(first, last); }
static void AssignRule(string::const_iterator first, string::const_iterator last)
{
	std::string production;
	production.assign(first, last);
	s_impl->AssignRule(production);
}



struct LSystemGrammar : public grammar<LSystemGrammar>
{
    template <typename ScannerT>
    struct definition
    {
		typedef rule<ScannerT> rule_t;
		rule_t lsystem_file;
		rule_t lsystem_entry;
		rule_t lsystem_id;
		rule_t lsystem_body;
		rule_t lsystem_statement;
		rule_t lsystem_angle;
		rule_t lsystem_symbols;
		rule_t lsystem_axiom;
		rule_t lsystem_production;

        definition(LSystemGrammar const &self)
		{
			lsystem_file = *lsystem_entry >> end_p;

			lsystem_entry = (lsystem_id >> lsystem_body)[&AssignEntry];
			
			lsystem_id = lexeme_d[+(print_p - blank_p - eol_p - '{')][&AssignId];

			lsystem_body = '{' >> eol_p >> *(lsystem_statement >> eol_p) >> '}' >> eol_p;

			lsystem_statement = lsystem_angle | lsystem_axiom | lsystem_production;

			lsystem_angle = lexeme_d[as_lower_d["angle"]] >> int_p[&AssignAngle];

			lsystem_symbols = lexeme_d[*(range_p('A', 'Z') | '-' | '+')];

			lsystem_axiom = lexeme_d[as_lower_d["axiom"]] >> lsystem_symbols[&AssignAxiom];

			lsystem_production =
				range_p('A', 'Z')[&AssignProductionSymbol] >> '=' >> lsystem_symbols[&AssignProduction];

			BOOST_SPIRIT_DEBUG_NODE(lsystem_file);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_entry);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_id);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_body);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_statement);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_angle);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_symbols);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_axiom);
			BOOST_SPIRIT_DEBUG_NODE(lsystem_production);
		}
		
        rule_t const &start() const
		{
			return lsystem_file;
		}
    };
};

LSystemParser::LSystemParser()
	: m_impl(new LSystemParserImpl())
{
}

LSystemParser::~LSystemParser()
{
	delete m_impl;
}

bool LSystemParser::Parse(const string &text)
{
	LSystemGrammar g;
	s_impl = m_impl;
	parse_info<string::const_iterator> results =
		parse(text.begin(), text.end(), g, (space_p | comment_p(";")) - eol_p);
	s_impl = NULL;
	
	return results.full;
}

int LSystemParser::Count() const
{
	return m_impl->EntriesParsed();
}

const LSystemEntry *LSystemParser::Entry(int index) const
{
	return m_impl->Entry(index);
}
