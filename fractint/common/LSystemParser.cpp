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

static LSystemParserImpl *s_impl = 0;

static void AssignId(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignId(first, last); }
static void AssignEntry(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignEntry(first, last); }
static void AssignAngle(int value)
{ s_impl->AssignAngle(value); }
static void AssignAxiom(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignAxiom(first, last); }
static void AssignProductionSymbol(string::const_iterator first, string::const_iterator last)
{
	std::string symbol;
	symbol.assign(first, last);
	s_impl->AssignProductionSymbol(symbol);
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
		rule_t file;
		rule_t entry;
		rule_t id;
		rule_t body;
		rule_t statement;
		rule_t angle;
		rule_t symbols;
		rule_t axiom;
		rule_t production;
		rule_t letter;
		rule_t symbol_increase_angle;
		rule_t symbol_decrease_angle;
		rule_t symbol_select_color;
		rule_t symbol_increment_color;
		rule_t symbol_decrement_color;
		rule_t symbol_segment_size;

        definition(LSystemGrammar const &self)
		{
			file = *entry >> end_p;

			entry = (id >> body)[&AssignEntry];
			
			id = lexeme_d[+(print_p - blank_p - eol_p - '{')][&AssignId];

			body = '{' >> eol_p >> *(statement >> eol_p) >> '}' >> eol_p;

			statement = angle | axiom | production;

			angle = lexeme_d[as_lower_d["angle"]] >> int_p[&AssignAngle];

			letter = range_p('A', 'Z');

			symbol_increase_angle = lexeme_d[ch_p('\\') >> int_p];
			symbol_decrease_angle = lexeme_d[ch_p('/') >> int_p];
			symbol_select_color = lexeme_d[ch_p('C') >> int_p];
			symbol_increment_color = lexeme_d[ch_p('>') >> int_p];
			symbol_decrement_color = lexeme_d[ch_p('<') >> int_p];
			symbol_segment_size = lexeme_d[(ch_p('@') >> int_p)
				|	('I' >> int_p) | ('Q' >> int_p)
				|	("IQ" >> int_p) | ("QI" >> int_p)];

			symbols = *(letter | '-' | '+' | '|' | '!' | '[' | ']'
				| symbol_increase_angle | symbol_decrease_angle
				| symbol_select_color | symbol_increment_color | symbol_decrement_color
				| symbol_segment_size);

			axiom = lexeme_d[as_lower_d["axiom"]] >> symbols[&AssignAxiom];

			production =
				letter[&AssignProductionSymbol] >> '=' >> symbols[&AssignProduction];

			BOOST_SPIRIT_DEBUG_NODE(file);
			BOOST_SPIRIT_DEBUG_NODE(entry);
			BOOST_SPIRIT_DEBUG_NODE(id);
			BOOST_SPIRIT_DEBUG_NODE(body);
			BOOST_SPIRIT_DEBUG_NODE(statement);
			BOOST_SPIRIT_DEBUG_NODE(angle);
			BOOST_SPIRIT_DEBUG_NODE(symbols);
			BOOST_SPIRIT_DEBUG_NODE(axiom);
			BOOST_SPIRIT_DEBUG_NODE(production);
			BOOST_SPIRIT_DEBUG_NODE(letter);
			BOOST_SPIRIT_DEBUG_NODE(symbol_increase_angle);
			BOOST_SPIRIT_DEBUG_NODE(symbol_decrease_angle);
			BOOST_SPIRIT_DEBUG_NODE(symbol_select_color);
			BOOST_SPIRIT_DEBUG_NODE(symbol_increment_color);
			BOOST_SPIRIT_DEBUG_NODE(symbol_decrement_color);
			BOOST_SPIRIT_DEBUG_NODE(symbol_segment_size);
		}
		
        rule_t const &start() const
		{
			return file;
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

struct SpaceGrammar : public grammar<SpaceGrammar>
{
    template <typename ScannerT>
    struct definition
    {
		typedef rule<ScannerT> rule_t;
		rule_t space;

        definition(SpaceGrammar const &self)
		{
			space = (space_p | comment_p(";")) - eol_p;

			BOOST_SPIRIT_DEBUG_NODE(space);
		}
		
        rule_t const &start() const
		{
			return space;
		}
    };
};

bool LSystemParser::Parse(const string &text)
{
	LSystemGrammar systems;
	SpaceGrammar spaces;

	s_impl = m_impl;
	parse_info<string::const_iterator> results =
		parse(text.begin(), text.end(), systems, blank_p | (";" >> *(anychar_p - eol_p)));
	s_impl = 0;
	
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
