#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "EnsureExtension.h"

bool g_fractal_overwrite;

int fr_find_first(char *path)
{
	return 0;
}

int expand_dirname(char *, char *)
{
	return 0;
}

BOOST_AUTO_TEST_CASE(EnsureExtension_AddedPath)
{
	boost::filesystem::path p = "foo";
	ensure_extension(p, ".gif");
	BOOST_CHECK_EQUAL("foo.gif", p);
}

BOOST_AUTO_TEST_CASE(EnsureExtension_NotAddedSamePath)
{
	boost::filesystem::path p = "foo.gif";
	ensure_extension(p, ".gif");
	BOOST_CHECK_EQUAL("foo.gif", p);
}

BOOST_AUTO_TEST_CASE(EnsureExtension_NotAddedDifferentPath)
{
	boost::filesystem::path p = "foo.gif";
	ensure_extension(p, ".map");
	BOOST_CHECK_EQUAL("foo.gif", p);
}

BOOST_AUTO_TEST_CASE(EnsureExtension_AddedCString)
{
	char filename[80] = "foo";
	ensure_extension(filename, ".gif");
	BOOST_CHECK_EQUAL(std::string("foo.gif"), std::string(filename));
}

BOOST_AUTO_TEST_CASE(EnsureExtension_NotAddedSameCString)
{
	char filename[80] = "foo.gif";
	ensure_extension(filename, ".gif");
	BOOST_CHECK_EQUAL(std::string("foo.gif"), std::string(filename));
}

BOOST_AUTO_TEST_CASE(EnsureExtension_NotAddedDifferentCString)
{
	char filename[80] = "foo.gif";
	ensure_extension(filename, ".map");
	BOOST_CHECK_EQUAL(std::string("foo.gif"), std::string(filename));
}
