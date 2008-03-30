#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include <boost/spirit.hpp>

using namespace boost::spirit;

struct TestGrammar : public grammar<TestGrammar>
{
	static int line;
	static std::string statement;
	static void DumpText(std::string::const_iterator first, std::string::const_iterator last)
	{
		std::string text;
		text.assign(first, last);
		statement += text;
		std::cout << "Token: '" << text << "'\n";
	}
	static void NextLine(std::string::const_iterator first, std::string::const_iterator last)
	{
		std::cout << "Line: " << line << " Text: '" << statement << "'\n";
		statement = "";
		line++;
	}

	template <typename ScannerT>
	struct definition
	{
		typedef rule<ScannerT> rule_t;
		rule_t file;
		rule_t token;

		definition(const TestGrammar &self)
		{
			file = *token >> end_p;

			token = lexeme_d[+(graph_p - ';')][&DumpText] | eol_p[&NextLine];

			BOOST_SPIRIT_DEBUG_NODE(file);
			BOOST_SPIRIT_DEBUG_NODE(token);
		}

		const rule_t &start() const
		{
			return file;
		}
	};
};
int TestGrammar::line = 1;
std::string TestGrammar::statement = "";

BOOST_AUTO_TEST_CASE(Spirit_Comment)
{
	TestGrammar grammar;
	std::string text = "Entry { ; 1 This is a comment\n"
		"     \n"
		"         ; 2 This is a comment on a line by itself.\n"
		"x-y; 3 This is a comment on a line with code.\n"
		"    1+2 + z - 4\n"
		" 3+4\n"
		"    } ; 4 this is a comment on the tail\n";
	parse_info<std::string::iterator> results =
		parse(text.begin(), text.end(), grammar,
			blank_p | (";" >> *(anychar_p - eol_p))
			);
	BOOST_CHECK
		(results.full);
}
