#include "editpal.h"
#include "RectangleSaver.h"

void RectangleSaver::Save(int x, int y, int width, int height)
{
	if (!g_has_inverse)
	{
		return;
	}
	_buffer.resize(width*height);
	BYTE *buff = &_buffer[0];

	CursorHider hider;
	for (int yoff = 0; yoff < height; yoff++)
	{
		get_row(x, y + yoff, width, buff);
		put_row(x, y + yoff, width, g_stack);
		buff += width;
	}
}

void RectangleSaver::Restore(int x, int y, int width, int height) const
{
	BYTE const *buff = &_buffer[0];

	if (!g_has_inverse)
	{
		return;
	}

	CursorHider hider;
	for (int yoff = 0; yoff < height; yoff++)
	{
		put_row(x, y + yoff, width, buff);
		buff += width;
	}
}
