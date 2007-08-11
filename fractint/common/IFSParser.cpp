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

class IFSTransformation
{
public:
	virtual ~IFSTransformation() {}

	virtual int GetCoefficientCount() const = 0;
	virtual const double *GetCoefficients() const = 0;
	virtual double GetCoefficient(int index) const = 0;
};

class IFSEntry
{
public:
	IFSEntry(string id, const vector<IFSTransformation *> &transforms)
		: m_id(id),
		m_transforms(transforms)
	{
	}
	~IFSEntry()
	{
		for (size_t i = 0; i < m_transforms.size(); i++)
		{
			delete m_transforms[i];
		}
	}

	const string &Id() const { return m_id; }
	const vector<IFSTransformation *> Transforms() const { return m_transforms; }

private:
	string m_id;
	vector<IFSTransformation *> m_transforms;
};


template <int Count>
class IFSTransformationN : public IFSTransformation
{
public:
	IFSTransformationN()
	{
	}
	IFSTransformationN(const double *coefficients)
	{
		std::copy(coefficients, coefficients + Count, &m_coefficients[0]);
	}
	virtual ~IFSTransformationN()
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

class IFSTransformation2D : public IFSTransformationN<7>
{
public:
	IFSTransformation2D(const double *coefficients) : IFSTransformationN(coefficients)
	{
	}
	virtual ~IFSTransformation2D()
	{
	}
};

class IFSTransformation3D : public IFSTransformationN<13>
{
public:
	IFSTransformation3D(const double *coefficients) : IFSTransformationN(coefficients)
	{
	}
	virtual ~IFSTransformation3D()
	{
	}
};

class IFSParserImpl
{
public:
	IFSParserImpl()
		: m_id(),
		m_coefficientCount(0),
		m_transforms(),
		m_entries()
	{
	}
	~IFSParserImpl()
	{
	}

	void AssignId(string::const_iterator first, string::const_iterator last);
	void Assign2D(double value);
	void Output2D(string::const_iterator first, string::const_iterator last);
	void Assign3D(double value);
	void Output3D(string::const_iterator first, string::const_iterator last);
	void AssignEntry(string::const_iterator first, string::const_iterator last);

	int EntriesParsed() const
	{
		return int(m_entries.size());
	}

private:
	string m_id;
	double m_coefficients[13];
	int m_coefficientCount;
	vector<IFSTransformation *> m_transforms;
	vector<IFSEntry *> m_entries;
};

static IFSParserImpl *s_impl = NULL;

void IFSParserImpl::AssignId(string::const_iterator first, string::const_iterator last)
{
	m_id.assign(first, last);
}

void IFSParserImpl::Assign2D(double value)
{
	assert(m_coefficientCount < 7);
	m_coefficients[m_coefficientCount++] = value;
}

void IFSParserImpl::Output2D(string::const_iterator first, string::const_iterator last)
{
	m_transforms.push_back(new IFSTransformation2D(&m_coefficients[0]));
	m_coefficientCount = 0;
}

void IFSParserImpl::Assign3D(double value)
{
	assert(m_coefficientCount < 13);
	m_coefficients[m_coefficientCount++] = value;
}

void IFSParserImpl::Output3D(string::const_iterator first, string::const_iterator last)
{
	m_transforms.push_back(new IFSTransformation3D(&m_coefficients[0]));
	m_coefficientCount = 0;
}

void IFSParserImpl::AssignEntry(string::const_iterator first, string::const_iterator last)
{
	m_entries.push_back(new IFSEntry(m_id, m_transforms));
	m_transforms.clear();
}

void AssignId(string::const_iterator first, string::const_iterator last)
{
	s_impl->AssignId(first, last);
}
void Assign2D(double value)
{
	s_impl->Assign2D(value);
}
void Assign3D(double value)
{
	s_impl->Assign3D(value);
}
void Output2D(string::const_iterator first, string::const_iterator last)
{
	s_impl->Output2D(first, last);
}
void Output3D(string::const_iterator first, string::const_iterator last)
{
	s_impl->Output3D(first, last);
}
void AssignEntry(string::const_iterator first, string::const_iterator last)
{
	s_impl->AssignEntry(first, last);
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

			ifs_entry = (ifs_id >> ifs_body)[&AssignEntry];
			
			ifs_id = lexeme_d[+(print_p - blank_p - eol_p - '(' - '{')][&AssignId];

			ifs_body = 
					(lexeme_d[as_lower_d["(3d)"]] >> '{' >> ifs_3d_body >> '}')
				|	(!lexeme_d[as_lower_d["(2d)"]] >> '{' >> ifs_2d_body >> '}')
				;

			ifs_2d_body = +((repeat_p(7)[real_p[&Assign2D]])[&Output2D]);

			ifs_3d_body = +((repeat_p(13)[real_p[&Assign3D]])[&Output3D]);

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

IFSParser::IFSParser()
	: m_impl(new IFSParserImpl())
{
}

IFSParser::~IFSParser()
{
	delete m_impl;
}

bool IFSParser::Parse(const string &text)
{
	IFSGrammar g;
	s_impl = m_impl;
	parse_info<string::const_iterator> results =
		parse(text.begin(), text.end(), g, space_p | comment_p(";"));
	s_impl = NULL;
	
	return results.full;
}

int IFSParser::Count() const
{
	return m_impl->EntriesParsed();
}
