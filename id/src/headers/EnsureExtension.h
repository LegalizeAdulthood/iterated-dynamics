#if !defined(ENSURE_EXTENSION_H)
#define ENSURE_EXTENSION_H

#include <boost/filesystem.hpp>

extern void ensure_extension(char *filename, const char *extension);
extern void ensure_extension(boost::filesystem::path &path, const char *extension);
extern std::string ensure_extension(std::string const &path, const char *extension);

#endif
