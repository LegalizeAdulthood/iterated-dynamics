// SPDX-License-Identifier: GPL-3.0-only
//
#include "wx_special_dirs.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace id
{

std::string WxSpecialDirectories::documents_dir() const
{
    const wxString dir{wxStandardPaths::Get().GetDocumentsDir()};
    return dir.ToStdString();
}

std::string WxSpecialDirectories::exeuctable_dir() const
{
    const wxFileName exe_file{wxStandardPaths::Get().GetExecutablePath()};
    const wxString dir{exe_file.GetPath()};
    return dir.ToStdString();
}

} // namespace id
