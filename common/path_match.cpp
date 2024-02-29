#include <path_match.h>

#include <regex>

namespace fs = std::filesystem;

static bool match_wild_string(const std::string &pattern, const std::string &text)
{
    if (text.length() != pattern.length())
    {
        return false;
    }
    for (std::size_t i = 0; i < text.length(); ++i)
    {
        if (pattern[i] != '?' && pattern[i] != text[i])
        {
            return false;
        }
    }
    return true;
}

static std::regex wild_regex(const std::string &wildcard)
{
    std::string pat;
    for (const char c : wildcard)
    {
        if (c == '.')
        {
            pat += "\\.";
        }
        else if (c == '?')
        {
            pat += '.';
        }
        else if (c == '*')
        {
            pat += ".*";
        }
        else
        {
            pat += c;
        }
    }
    return std::regex(pat);
};

MatchFn match_fn(const fs::path &pattern)
{
    MatchFn always = [](const fs::path &) { return true; };
    
    MatchFn match_stem;
    const std::string pat_stem{pattern.stem().string()};
    // match any filename
    if (pat_stem == "*")
    {
        match_stem = always;
    }
    // match wildcard filename
    else if (pat_stem.find('?') != std::string::npos)
    {
        if (pat_stem.find('*') == std::string::npos)
        {
            match_stem = [=](const fs::path &path) { return match_wild_string(pat_stem, path.stem().string()); };
        }
        else
        {
            const std::regex pat_regex = wild_regex(pat_stem);
            match_stem = [=](const fs::path &path) { return std::regex_match(path.stem().string(), pat_regex); };
        }
    }
    // match regex filename
    else if (pat_stem.find('*') != std::string::npos)
    {
        const std::regex pat_regex = wild_regex(pat_stem);
        match_stem = [=](const fs::path &path) { return std::regex_match(path.stem().string(), pat_regex); };
    }
    // match exact filename
    else
    {
        match_stem = [=](const fs::path &path) { return pat_stem == path.stem().string(); };
    }

    MatchFn match_ext;
    const std::string pat_ext{pattern.extension().string()};
    // match any extension
    if (pat_ext == ".*" || pat_ext.empty())
    {
        match_ext = always;
    }
    // match extension with ? wildcards
    else if (pat_ext.find('?') != std::string::npos)
    {
        if (pat_ext.find('*') == std::string::npos)
        {
            match_ext = [=](const fs::path &path) { return match_wild_string(pat_ext, path.extension().string()); };
        }
        else
        {
            const std::regex pat_regex = wild_regex(pat_ext);
            match_ext = [=](const fs::path &path) { return std::regex_match(path.extension().string(), pat_regex); };
        }
    }
    else if (pat_ext.find('*') != std::string::npos)
    {
        const std::regex pat_regex = wild_regex(pat_ext);
        match_ext = [=](const fs::path &path) { return std::regex_match(path.extension().string(), pat_regex); };
    }
    else
    // match specific extension
    {
        match_ext = [=](const fs::path &path) { return pat_ext == path.extension(); };
    }

    return [=](const fs::path &path) { return match_stem(path) && match_ext(path); };
}
