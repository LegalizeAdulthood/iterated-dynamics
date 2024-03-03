#pragma once

void make_path(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext);

inline void make_fname_ext(char *template_str, char const *fname, char const *ext)
{
    make_path(template_str, nullptr, nullptr, fname, ext);
}
