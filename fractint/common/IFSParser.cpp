#include <algorithm>
#include <cassert>
#include <iomanip>
#include <string>
#include <vector>

#include <boost/mem_fn.hpp>
#include <boost/spirit.hpp>

#include "IFSParser.h"

using namespace std;
using namespace boost::spirit;

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

class IFSEntry
{
public:
	virtual ~IFSEntry() {}

	virtual int GetCoefficientCount() const = 0;
	virtual const double *GetCoefficients() const = 0;
	virtual double GetCoefficient(int index) const = 0;
};

template <int Count>
class IFSEntryN : public IFSEntry
{
public:
	IFSEntryN()
	{
	}
	IFSEntryN(const double *coefficients)
	{
		std::copy(coefficients, coefficients + Count, &m_coefficients[0]);
	}
	virtual ~IFSEntryN()
	{
	}

	virtual int GetCoefficientCount() const
	{
		return Count;
	}
	virtual const double *GetCoefficients() const
	{
		return m_coefficients;
	}
	virtual double GetCoefficient(int index) const
	{
		return m_coefficients[index];
	}

private:
	double m_coefficients[Count];
};

class IFSEntry2D : public IFSEntryN<7>
{
public:
	IFSEntry2D(const double *coefficients) : IFSEntryN(coefficients)
	{
	}
	virtual ~IFSEntry2D()
	{
	}
};

class IFSEntry3D : public IFSEntryN<13>
{
public:
	IFSEntry3D(const double *coefficients) : IFSEntryN(coefficients)
	{
	}
	virtual ~IFSEntry3D()
	{
	}
};

class IFSParserImpl
{
public:
	static void AssignId(string::const_iterator first, string::const_iterator last);
	static void Assign2D(double value);
	static void Output2D(string::const_iterator first, string::const_iterator last);
	static void Assign3D(double value);
	static void Output3D(string::const_iterator first, string::const_iterator last);
	static int EntriesParsed()
	{
		return int(s_entries.size());
	}

private:
	static double s_coefficients[13];
	static int s_coefficientCount;
	static vector<IFSEntry *> s_entries;
};

double IFSParserImpl::s_coefficients[13] = { 0 };
int IFSParserImpl::s_coefficientCount = 0;
vector<IFSEntry *> IFSParserImpl::s_entries;

void IFSParserImpl::AssignId(string::const_iterator first, string::const_iterator last)
{
	string id;
	id.assign(first, last);
}

void IFSParserImpl::Assign2D(double value)
{
	assert(s_coefficientCount < 7);
	s_coefficients[s_coefficientCount++] = value;
}

void IFSParserImpl::Output2D(string::const_iterator first, string::const_iterator last)
{
	string params;
	params.assign(first, last);
	s_entries.push_back(new IFSEntry2D(&s_coefficients[0]));
	s_coefficientCount = 0;
}

void IFSParserImpl::Assign3D(double value)
{
	assert(s_coefficientCount < 13);
	s_coefficients[s_coefficientCount++] = value;
}

void IFSParserImpl::Output3D(string::const_iterator first, string::const_iterator last)
{
	s_entries.push_back(new IFSEntry3D(&s_coefficients[0]));
	s_coefficientCount = 0;
}

struct IFSGrammar : public grammar<IFSGrammar>
{
    template <typename ScannerT>
    struct definition
    {
		typedef rule<ScannerT> rule_t;
		rule_t ifs_entry;
		rule_t ifs_2d_body;
		rule_t ifs_3d_body;
		rule_t ifs_file;
		rule_t ifs_id;
		rule_t ifs_body;

        definition(IFSGrammar const &self)
		{
			ifs_file = *ifs_entry >> end_p;

			ifs_entry = ifs_id >> ifs_body;
			
			ifs_id = lexeme_d[+(print_p - blank_p - eol_p - '(' - '{')][&IFSParserImpl::AssignId];

			ifs_body = 
					(lexeme_d[as_lower_d["(3d)"]] >> '{' >> ifs_3d_body >> '}')
				|	(!lexeme_d[as_lower_d["(2d)"]] >> '{' >> ifs_2d_body >> '}')
				;

			ifs_2d_body = +((repeat_p(7)[real_p[&IFSParserImpl::Assign2D]])[&IFSParserImpl::Output2D]);

			ifs_3d_body = +((repeat_p(13)[real_p[&IFSParserImpl::Assign3D]])[&IFSParserImpl::Output3D]);

			BOOST_SPIRIT_DEBUG_NODE(ifs_file);
			BOOST_SPIRIT_DEBUG_NODE(ifs_entry);
			BOOST_SPIRIT_DEBUG_NODE(ifs_id);
			BOOST_SPIRIT_DEBUG_NODE(ifs_body);
			BOOST_SPIRIT_DEBUG_NODE(ifs_2d_body);
			BOOST_SPIRIT_DEBUG_NODE(ifs_3d_body);
		}
		
        rule_t const &start() const
		{
			return ifs_file;
		}
    };
};

bool IFSParser::Parse(const string &text)
{
	IFSGrammar g;
	parse_info<string::const_iterator> results =
		parse(text.begin(), text.end(), g, space_p | comment_p(";"));
	
	return results.full;
}

int IFSParser::Count() const
{
	return m_count;
}
