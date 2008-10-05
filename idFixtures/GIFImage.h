#if !defined(GIF_IMAGE_H)
#define GIF_IMAGE_H

#include <string>
#include "gif_lib.h"

class GIFImage
{
public:
	GIFImage(std::string const &path);
	~GIFImage();

	bool operator==(GIFImage const &right);
	bool operator!=(GIFImage const &right);
	bool SameSize(GIFImage const &right);
	bool SameColors(GIFImage const &right);
	bool SameColors(GIFImage const &right, int &lastMatchingColor);
	bool SameColors(GIFImage const &right, int &lastMatchingColor, std::string &mismatchedColor);

	bool SamePixels(GIFImage const &right);

private:
	bool SameMaskedColors(int &lastMatchingColor, std::string &mismatchedColor,
		ColorMapObject const *self, int mask, ColorMapObject const *other);
	void DecodeGifFile();

	std::string _path;
	GifFileType *_file;
};

#endif
