// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

enum
{
    SUB_DIR = 1
};

struct FindResult
{
    std::string path;     // path and filespec
    char attribute;       // File attributes wanted
    std::string filename; // Filename and extension
};

extern FindResult g_dta;

/* fr_find_first
 *
 * Fill in DTA.filename, DTA.path and DTA.attribute for the first file
 * matching the wildcard specification in path.  Return false if a file
 * is found, or true if a file was not found or an error occurred.
 */
bool fr_find_first(const char *path);

/* fr_find_next
 *
 * Find the next file matching the wildcard search begun by fr_find_first.
 * Fill in g_dta.filename, g_dta.path, and g_dta.attribute
 */
bool fr_find_next();
