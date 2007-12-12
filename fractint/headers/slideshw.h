#pragma once

class SlideShowImpl;

enum SlideType
{
	SLIDES_OFF		= 0,
	SLIDES_PLAY		= 1,
	SLIDES_RECORD	= 2
};


class SlideShow
{
public:
	SlideShow();
	~SlideShow();

	int GetKeyStroke();
	SlideType Start();
	void Stop();
	void Record(int keyStroke);
	SlideType Mode() const;
	void Mode(SlideType value);

	const std::string &AutoKeyFile() const;
	void SetAutoKeyFile(const std::string &value);

private:
	SlideShowImpl *_impl;
};

extern SlideShow g_slideShow;
