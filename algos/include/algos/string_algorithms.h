//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace id::algos
{

std::string ascii_to_lower_copy(std::string_view text);
std::string ascii_to_upper_copy(std::string_view text);
void replace_all(std::string &text, std::string_view old_text, std::string_view new_text);

// Split helpers preserve empty fields from empty input, adjacent
// delimiters, leading delimiters, and trailing delimiters.
std::vector<std::string> split(std::string_view text, char delimiter);
std::vector<std::string> split_any(std::string_view text, std::string_view delimiters);

} // namespace id::algos
