// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/find_file.h"

#include "io/path_match.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace id::io
{

enum class DirPos
{
    NONE = 0,
    DOT = 1,
    DOT_DOT = 2,
    FILES = 3
};

struct DirSearch
{
    MatchFn path_matcher;
    DirPos pos{DirPos::NONE};
    std::filesystem::directory_iterator it;
};

FindResult g_dta;

static DirSearch s_search;

/* fill_dta
 *
 * Use data in s_find_data to fill in DTA.filename, DTA.attribute and DTA.path
 */
static void fill_dta()
{
    g_dta.path = s_search.it->path().string();
    g_dta.filename = s_search.it->path().filename().string();
    g_dta.attribute = is_directory(*s_search.it) ? SUB_DIR : 0;
}

static bool next_match()
{
    if (s_search.pos == DirPos::NONE)
    {
        g_dta.path = (s_search.it->path() / ".").string();
        g_dta.filename = ".";
        g_dta.attribute = SUB_DIR;
        s_search.pos = DirPos::DOT;
        return true;
    }
    if (s_search.pos == DirPos::DOT)
    {
        g_dta.path = (s_search.it->path() / "..").string();
        g_dta.filename = "..";
        g_dta.attribute = SUB_DIR;
        s_search.pos = DirPos::DOT_DOT;
        return true;
    }
    s_search.pos = DirPos::FILES;

    while (s_search.it != fs::directory_iterator() && !s_search.path_matcher(*s_search.it))
    {
        ++s_search.it;
    }
    if (s_search.it == fs::directory_iterator())
    {
        s_search.pos = DirPos::NONE;
        return false;
    }

    fill_dta();
    return true;
}

bool fr_find_first(const char *path)       // Find 1st file (or subdir) meeting path/filespec
{
    const fs::path search{path};
    const fs::path search_dir{is_directory(search) ? search : search.has_parent_path()             ? search.parent_path()
                                                   : "."};
    std::error_code err;
    s_search.it = fs::directory_iterator(search_dir, err);
    if (err)
    {
        return false;
    }

    s_search.path_matcher = match_fn(path);
    if (is_directory(search) || search.filename() == "*" || search.filename() == "*.*")
    {
        s_search.pos = DirPos::NONE;
    }
    else
    {
        s_search.pos = DirPos::FILES;
    }
    return next_match();
}

bool fr_find_next()
{
    if (s_search.pos == DirPos::FILES)
    {
        ++s_search.it;
    }
    return next_match();
}

} // namespace id::io
