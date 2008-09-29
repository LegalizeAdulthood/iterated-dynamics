#if !defined(FILE_SYSTEM_H)
#define FILE_SYSTEM_H

#include <exception>
#include <string>

class CStandardLibraryException : public std::exception
{
public:
	CStandardLibraryException(int error) : _error(error)
	{ }
	int Error() const { return _error; }

private:
	int _error;
};

class FileSystem
{
public:
	static void DirectorySetCurrent(std::string const &path);
	static std::string DirectoryGetCurrent();
	static int ExecuteCommand(std::string const &command);
	static bool Exists(std::string const &path);
};

class CurrentDirectoryPusher
{
public:
	CurrentDirectoryPusher(std::string const &path)
	{
		_currentDirectory = FileSystem::DirectoryGetCurrent();
		FileSystem::DirectorySetCurrent(path);
	}
	~CurrentDirectoryPusher()
	{
		FileSystem::DirectorySetCurrent(_currentDirectory);
	}
private:
	std::string _currentDirectory;
};

#endif
