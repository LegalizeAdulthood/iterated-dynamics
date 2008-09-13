#if !defined(RECTANGLE_SAVER_H)
#define RECTANGLE_SAVER_H

#include "port.h"
#include <vector>

class RectangleSaver
{
public:
	enum
	{
		MAXRECT = 1024      // largest width of SaveRect/RestoreRect
	};

	void Save(int x, int y, int width, int height);
	void Restore(int x, int y, int width, int height) const;
	void Clear()
	{
		std::vector<BYTE> empty;
		_buffer.swap(empty);
	}

private:
	std::vector<BYTE> _buffer;
};

#endif
