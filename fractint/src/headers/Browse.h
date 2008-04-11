#if !defined(BROWSE_H)
#define BROWSE_H

#include "id.h"

class BrowseState
{
public:
	virtual ~BrowseState()
	{
	}

	virtual bool AutoBrowse() const = 0;
	virtual bool Browsing() const = 0;
	virtual bool CheckParameters() const = 0;
	virtual bool CheckType() const = 0;
	virtual const std::string &Mask() const = 0;
	virtual const std::string &Name() const = 0;
	virtual bool SubImages() const = 0;
	virtual int CrossHairBoxSize() const = 0;
	virtual bool DoubleCaution() const = 0;
	virtual float TooSmall() const = 0;

	virtual void SetAutoBrowse(bool value) = 0;
	virtual void SetBrowsing(bool value) = 0;
	virtual void SetCheckParameters(bool value) = 0;
	virtual void SetCheckType(bool value) = 0;
	virtual void SetName(const std::string &value) = 0;
	virtual void SetSubImages(bool value) = 0;
	virtual void SetCrossHairBoxSize(int value) = 0;
	virtual void SetDoubleCaution(bool value) = 0;
	virtual void SetTooSmall(float value) = 0;

	virtual void ExtractReadName() = 0;
	virtual int GetParameters() = 0;
	virtual void MakePath(const char *fname, const char *ext) = 0;
	virtual void MergePathNames(std::string &read_name) = 0;
	virtual void Restart() = 0;
};

extern BrowseState &g_browse_state;
extern std::string g_file_name_stack[16];

extern ApplicationStateType handle_look_for_files(bool &stacked);

#endif
