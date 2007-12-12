#pragma once

class SlideShowImpl;

class SlideShow
{
public:
	SlideShow();
	~SlideShow();

	int GetKeyStroke();
	int Start();
	void Stop();
	void Record(int keyStroke);

	const std::string &AutoKeyFile() const;
	void SetAutoKeyFile(const std::string &value);

private:
	SlideShowImpl *_impl;
};

extern SlideShow g_slideShow;

inline int slide_show()
{
	return g_slideShow.GetKeyStroke();
}
inline int start_slide_show()
{
	return g_slideShow.Start();
}
inline void stop_slide_show()
{
	g_slideShow.Stop();
}
inline void record_show(int keyStroke)
{
	g_slideShow.Record(keyStroke);
}
