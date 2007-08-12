#if !defined(IFS_PARSER_H)
#define IFS_PARSER_H

#include <string>
#include <vector>

class IFSParserImpl;

class IFSTransformation
{
public:
	virtual ~IFSTransformation() {}

	virtual int GetCoefficientCount() const = 0;
	virtual const double *GetCoefficients() const = 0;
	virtual double GetCoefficient(int index) const = 0;
};

class IFSEntry
{
public:
	IFSEntry(std::string id, const std::vector<IFSTransformation *> &transforms)
		: m_id(id),
		m_transforms(transforms)
	{
	}
	~IFSEntry()
	{
		for (size_t i = 0; i < m_transforms.size(); i++)
		{
			delete m_transforms[i];
		}
	}

	const std::string &Id() const
	{ return m_id; }
	const std::vector<IFSTransformation *> &Transforms() const
	{ return m_transforms; }

private:
	std::string m_id;
	std::vector<IFSTransformation *> m_transforms;
};


class IFSParser
{
public:
	static IFSParser StackInstance()
	{
		return IFSParser();
	}
	~IFSParser();

	bool Parse(const std::string &text);
	int Count() const;
	const IFSEntry *Entry(int index) const;

private:
	IFSParser();
	IFSParserImpl *m_impl;
};

#endif
