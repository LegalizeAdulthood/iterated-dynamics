#if !defined(FAKE_BROWSE_STATE_H)
#define FAKE_BROWSE_STATE_H

#include "Browse.h"
#include "NotImplementedException.h"

class FakeBrowseState : public BrowseState
{
public:
	FakeBrowseState() : BrowseState(),
		_setBrowsingCalled(false),
		_setBrowsingLastValue(false),
		_setNameCalled(false),
		_setNameLastValue()
	{
	}
	virtual ~FakeBrowseState() { }

	virtual bool AutoBrowse() const { throw not_implemented("AutoBrowse"); }
	virtual bool Browsing() const { throw not_implemented("Browsing"); }
	virtual bool CheckParameters() const { throw not_implemented("CheckParameters"); }
	virtual bool CheckType() const { throw not_implemented("CheckType"); }
	virtual const std::string &Mask() const { throw not_implemented("Mask"); }
	virtual const std::string &Name() const { throw not_implemented("Name"); }
	virtual bool SubImages() const { throw not_implemented("SubImages"); }
	virtual int CrossHairBoxSize() const { throw not_implemented("CrossHairBoxSize"); }
	virtual bool DoubleCaution() const { throw not_implemented("DoubleCaution"); }
	virtual float TooSmall() const { throw not_implemented("TooSmall"); }

	virtual void SetAutoBrowse(bool value) { throw not_implemented("SetAutoBrowse"); }
	virtual void SetBrowsing(bool value) { _setBrowsingCalled = true; _setBrowsingLastValue = value; }
	bool SetBrowsingCalled() const					{ return _setBrowsingCalled; }
	bool SetBrowsingLastValue() const				{ return _setBrowsingLastValue; }
	virtual void SetCheckParameters(bool value) { throw not_implemented("SetCheckParameters"); }
	virtual void SetCheckType(bool value) { throw not_implemented("SetCheckType"); }
	virtual void SetName(const std::string &value)	{ _setNameCalled = true; _setNameLastValue = value; }
	bool SetNameCalled() const						{ return _setNameCalled; }
	std::string const &SetNameLastValue() const		{ return _setNameLastValue; }
	virtual void SetSubImages(bool value) { throw not_implemented("SetSubImages"); }
	virtual void SetCrossHairBoxSize(int value) { throw not_implemented("SetCrossHairBoxSize"); }
	virtual void SetDoubleCaution(bool value) { throw not_implemented("SetDoubleCaution"); }
	virtual void SetTooSmall(float value) { throw not_implemented("SetTooSmall"); }

	virtual void ExtractReadName() { throw not_implemented("ExtractReadName"); }
	virtual int GetParameters() { throw not_implemented("GetParameters"); }
	virtual void MakePath(const char *fname, const char *ext) { throw not_implemented("MakePath"); }
	virtual void MergePathNames(char *read_name) { throw not_implemented("MergePathNames"); }
	virtual void MergePathNames(std::string &read_name) { throw not_implemented("MergePathNames"); }
	virtual void Restart() { throw not_implemented("Restart"); }

private:
	bool _setBrowsingCalled;
	bool _setBrowsingLastValue;
	bool _setNameCalled;
	std::string _setNameLastValue;
};

#endif
