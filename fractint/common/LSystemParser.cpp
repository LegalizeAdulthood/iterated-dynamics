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
	}
	void AssignEntry(string::const_iterator first, string::const_iterator last)
	{
		m_entries.push_back(new LSystemEntry(m_id));
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
	string m_id;
	int m_angle;
	string m_axiom;
	string m_production;
	vector<string> m_productions;
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
static void AssignProduction(string::const_iterator first, string::const_iterator last)
{ s_impl->AssignProduction(first, last); }



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

			lsystem_entry = (lsystem_id >> '{' >> lsystem_body >> '}')[&AssignEntry];
			
			lsystem_id = lexeme_d[+(print_p - blank_p - eol_p - '(' - '{')][&AssignId];

			lsystem_body = *(lsystem_statement >> eol_p);

			lsystem_statement = lsystem_angle | lsystem_axiom | lsystem_production;

			lsystem_angle = as_lower_d["angle"] >> int_p[&AssignAngle];

			lsystem_symbols = (*(anychar_p - eol_p));

			lsystem_axiom = as_lower_d["axiom"] >> lsystem_symbols[&AssignAxiom];

			lsystem_production = as_lower_d[range_p('a', 'z')] >> '=' >> lsystem_symbols[&AssignProduction];

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
		parse(text.begin(), text.end(), g, space_p | comment_p(";"));
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
