#if !defined(FILESYSTEM_H)
#define FILESYSTEM_H

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum
{
	FILEATTR = 0x37,      // File attributes; select all but volume labels
	HIDDEN = 2,
	SYSTEM = 4,
	SUBDIR = 16
};

struct DIR_SEARCH				// Allocate	DTA	and	define structure
{
	std::string path;			// DOS path	and	filespec
	char attribute;				// File	attributes wanted
	int	 ftime;					// File	creation time
	int	 fdate;					// File	creation date
	long size;					// File	size in bytes
	std::string filename;		// Filename	and	extension
};

extern DIR_SEARCH g_dta;

extern int merge_path_names(char *old_full_path, char *new_filename, int mode);
extern int merge_path_names(bool copy_directory, char *old_full_path, char *new_filename);
extern int merge_path_names(std::string &old_full_path, char *new_filename, int mode);
extern int merge_path_names(bool copy_directory, std::string &old_full_path, char *new_filename);
extern int merge_path_names(char *old_full_path, std::string &new_filename, int mode);
extern int merge_path_names(bool copy_directory, char *old_full_path, std::string &new_filename);
extern int merge_path_names(std::string &old_full_path, std::string &new_filename, int mode);
extern int merge_path_names(bool copy_directory, std::string &old_full_path, std::string &new_filename);
extern void ensure_slash_on_directory(char *dirname);
extern void ensure_slash_on_directory(std::string &dirname);
extern int fr_find_first(char *path);
extern int fr_find_next();
extern void make_path(char *file_template, const char *drive, const char *dir, const char *fname, const char *ext);
extern fs::path make_path(const char *drive, const char *dir, const char *fname, const char *ext);
extern void split_path(const std::string &file_template, char *drive, char *dir, char *filename, char *extension);
extern void split_path(const char *file_template, char *drive, char *dir, char *filename, char *extension);
extern void split_path(const char *file_template, char *drive, std::string &dir, char *filename, char *extension);
extern bool is_a_directory(const char *s);
extern int dir_remove(const std::string &dir, const std::string &filename);
extern int dir_remove(const fs::path &dir, const std::string &filename);
extern FILE *dir_fopen(const char *dir, const char *filename, const char *mode);
extern FILE *dir_fopen(const std::string &dir, const std::string &filename, const std::string &mode);
extern FILE *dir_fopen(const fs::path &dir, const std::string &filename, const std::string &mode);
extern void extract_filename(char *target, const char *source);
extern void extract_filename(std::string &target, const std::string &source);
extern const char *has_extension(const char *source);
extern const char *has_extension(const std::string &source);
extern void find_path(const char *filename, char *fullpathname);
extern void check_write_file(char *filename, const char *ext);
extern void check_write_file(std::string &name, const char *ext);
extern void update_save_name(char *filename);
extern void update_save_name(std::string &filename);
extern void ensure_extension(fs::path &path, const char *extension);
extern bool read_access(const char *path);
extern bool write_access(const char *path);
extern bool read_write_access(const char *path);
extern bool exists(const char *path);
extern bool exists(const std::string &path);

#endif
