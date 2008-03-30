#include "EnsureExtension.h"

void ensure_extension(char *filename, const char *extension)
{
	boost::filesystem::path path = filename;
	ensure_extension(path, extension);
	strcpy(filename, path.string().c_str());
}

void ensure_extension(boost::filesystem::path &path, const char *extension)
{
	if (boost::filesystem::extension(path).length() == 0)
	{
		path.remove_leaf() /= (path.leaf() + extension);
	}
}

