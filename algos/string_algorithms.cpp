//
// Copyright 2026 Richard Thomson
//
#include "algos/string_algorithms.h"

namespace id::algos
{

namespace
{

char ascii_to_lower(const char ch)
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

char ascii_to_upper(const char ch)
{
    return ch >= 'a' && ch <= 'z' ? static_cast<char>(ch - 'a' + 'A') : ch;
}

template <typename DelimiterFinder>
std::vector<std::string> split_using(std::string_view text, DelimiterFinder find_delimiter)
{
    std::vector<std::string> result;
    std::size_t start{};
    for (;;)
    {
        const std::size_t end{find_delimiter(text, start)};
        if (end == std::string_view::npos)
        {
            result.push_back(std::string{text.substr(start)});
            return result;
        }
        result.push_back(std::string{text.substr(start, end - start)});
        start = end + 1;
    }
}

} // namespace

std::string ascii_to_lower_copy(const std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const char ch : text)
    {
        result.push_back(ascii_to_lower(ch));
    }
    return result;
}

std::string ascii_to_upper_copy(const std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const char ch : text)
    {
        result.push_back(ascii_to_upper(ch));
    }
    return result;
}

void replace_all(std::string &text, const std::string_view old_text, const std::string_view new_text)
{
    if (old_text.empty())
    {
        return;
    }

    const std::string old_string{old_text};
    const std::string new_string{new_text};
    for (std::size_t pos{}; (pos = text.find(old_string, pos)) != std::string::npos; pos += new_string.size())
    {
        text.replace(pos, old_string.size(), new_string);
    }
}

std::vector<std::string> split(const std::string_view text, const char delimiter)
{
    return split_using(text,
        [delimiter](const std::string_view value, const std::size_t start) { return value.find(delimiter, start); });
}

std::vector<std::string> split_any(const std::string_view text, const std::string_view delimiters)
{
    return split_using(text, [delimiters](const std::string_view value, const std::size_t start)
        { return value.find_first_of(delimiters, start); });
}

} // namespace id::algos
