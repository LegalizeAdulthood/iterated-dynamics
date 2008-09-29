#include "stdafx.h"
#include <cassert>
#include <stdexcept>

#include "FileSystem.h"
#include "GIFImage.h"
#include "gif_lib.h"

class FileNotFoundException : public std::exception
{
public:
	FileNotFoundException(std::string const &path)
		: exception(),
		_path(path)
	{ }

	std::string const &Path()
	{ return _path; }

private:
	std::string _path;
};

GIFImage::GIFImage(const std::string &path)
	: _path(path),
	_file(0)
{
	if (!FileSystem::Exists(path))
	{
		throw FileNotFoundException(path);
	}

	DecodeGifFile();
}

GIFImage::~GIFImage()
{
	int result = DGifCloseFile(_file);
	assert(GIF_OK == result);
}

void GIFImage::DecodeGifFile()
{
	_file = DGifOpenFileName(_path.c_str());
	if (!_file)
	{
		throw std::invalid_argument("'" + _path + "' is not a GIF file.");
	}

	if (GIF_OK != DGifSlurp(_file))
	{
		throw std::invalid_argument("'" + _path + "' is not a GIF file.");
	}
}

bool GIFImage::operator ==(const GIFImage &right)
{
	return SameSize(right) && SameColors(right) && SamePixels(right);
}

bool GIFImage::operator !=(const GIFImage &right)
{
	return !operator==(right);
}

bool GIFImage::SameSize(const GIFImage &right)
{
	return (_file->SHeight == right._file->SHeight)
		&& (_file->SWidth == right._file->SWidth)
		&& (_file->ImageCount == right._file->ImageCount);
}

bool GIFImage::SameColors(const GIFImage &right)
{
	if ((_file->SColorMap && !right._file->SColorMap)
		|| (!_file->SColorResolution && right._file->SColorMap))
	{
		return false;
	}

	ColorMapObject *self = 0;
	ColorMapObject *other = 0;
	if (_file->SColorMap)
	{
		self = _file->SColorMap;
		other = right._file->SColorMap;
	}
	else
	{
		self = _file->Image.ColorMap;
		other = _file->Image.ColorMap;
	}
	assert(self && other);

	if ((self->BitsPerPixel != other->BitsPerPixel)
		|| (self->ColorCount != other->ColorCount))
	{
		return false;
	}

	for (int i = 0; i < self->ColorCount; i++)
	{
		if ((self->Colors[i].Red != other->Colors[i].Red)
			|| (self->Colors[i].Green != other->Colors[i].Green)
			|| (self->Colors[i].Blue != other->Colors[i].Blue))
		{
			return false;
		}
	}

	return true;
}

bool GIFImage::SamePixels(const GIFImage &right)
{
	for (int i = 0; i < _file->ImageCount; i++)
	{
		int offset = 0;
		for (int y = 0; y < _file->Image.Height; y++)
		{
			for (int x = 0; x < _file->Image.Width; x++)
			{
				if (      _file->SavedImages[i].RasterBits[offset] !=
					right._file->SavedImages[i].RasterBits[offset])
				{
					return false;
				}

				++offset;
			}
			if (offset & 3)
			{
				offset += 4 - (offset & 3);
			}
		}
	}

	return true;
}
