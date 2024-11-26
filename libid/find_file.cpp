// SPDX-License-Identifier: GPL-3.0-only
//
#include "find_file.h"

#include "path_match.h"

enum class dir_pos
{
    NONE = 0,
    DOT = 1,
    DOT_DOT = 2,
    FILES = 3
};

DirSearch g_dta;

namespace fs = std::filesystem;

static MatchFn s_path_matcher;
static dir_pos s_dir_pos{dir_pos::NONE};
static fs::directory_iterator s_dir_it;

/* fill_dta
 *
 * Use data in s_find_data to fill in DTA.filename, DTA.attribute and DTA.path
 */
static void fill_dta()
{
    g_dta.path = s_dir_it->path().string();
    g_dta.filename = s_dir_it->path().filename().string();
    g_dta.attribute = is_directory(*s_dir_it) ? SUBDIR : 0;
}

static bool next_match()
{
    if (s_dir_pos == dir_pos::NONE)
    {
        g_dta.path = (s_dir_it->path() / ".").string();
        g_dta.filename = ".";
        g_dta.attribute = SUBDIR;
        s_dir_pos = dir_pos::DOT;
        return true;
    }
    if (s_dir_pos == dir_pos::DOT)
    {
        g_dta.path = (s_dir_it->path() / "..").string();
        g_dta.filename = "..";
        g_dta.attribute = SUBDIR;
        s_dir_pos = dir_pos::DOT_DOT;
        return true;
    }
    s_dir_pos = dir_pos::FILES;

    while (s_dir_it != fs::directory_iterator() && !s_path_matcher(*s_dir_it))
    {
        ++s_dir_it;
    }
    if (s_dir_it == fs::directory_iterator())
    {
        s_dir_pos = dir_pos::NONE;
        return false;
    }

    fill_dta();
    return true;
}

/* fr_findfirst
 *
 * Fill in DTA.filename, DTA.path and DTA.attribute for the first file
 * matching the wildcard specification in path.  Return zero if a file
 * is found, or non-zero if a file was not found or an error occurred.
 */
int fr_findfirst(char const *path)       // Find 1st file (or subdir) meeting path/filespec
{
    const fs::path search{path};
    const fs::path search_dir{is_directory(search) ? search : (search.has_parent_path() ? search.parent_path() : ".")};
    std::error_code err;
    s_dir_it = fs::directory_iterator(search_dir, err);
    if (err)
    {
        return 1;
    }

    s_path_matcher = match_fn(path);
    if (is_directory(search) || search.filename() == "*" || search.filename() == "*.*")
    {
        s_dir_pos = dir_pos::NONE;
    }
    else
    {
        s_dir_pos = dir_pos::FILES;
    }
    return next_match() ? 0 : 1;
}

/* fr_findnext
 *
 * Find the next file matching the wildcard search begun by fr_findfirst.
 * Fill in DTA.filename, DTA.path, and DTA.attribute
 */
int fr_findnext()
{
    if (s_dir_pos == dir_pos::FILES)
    {
        ++s_dir_it;
    }
    return next_match() ? 0 : -1;
}

