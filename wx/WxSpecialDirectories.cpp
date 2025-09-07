// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace id::io
{

namespace
{

class WxSpecialDirectories : public SpecialDirectories
{
public:
    ~WxSpecialDirectories() override = default;

    std::filesystem::path documents_dir() const override;

    std::filesystem::path program_dir() const override;
};

std::filesystem::path WxSpecialDirectories::documents_dir() const
{
    const wxString dir{wxStandardPaths::Get().GetDocumentsDir()};
    return dir.ToStdString();
}

std::filesystem::path WxSpecialDirectories::program_dir() const
{
    const wxFileName exe_file{wxStandardPaths::Get().GetExecutablePath()};
    const wxString dir{exe_file.GetPath()};
    return dir.ToStdString();
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<WxSpecialDirectories>();
}

} // namespace id::io
