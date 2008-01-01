#include "port.h"
#include "id.h"
#include "cmplx.h"
#include "externs.h"

#include "ColormapTable.h"

/*
	ColormapTable::FindSpecialColors()

	Find the darkest and brightest colors in palette, and a medium
	color which is reasonably bright and reasonably grey.
*/
void ColormapTable::FindSpecialColors()
{
	int maxb = 0;
	int minb = 9999;
	int med = 0;
	int maxgun, mingun;
	int brt;
	int i;

	_darkIndex = 0;
	_mediumIndex = 7;
	_brightIndex = 15;

	if (!g_.RealDAC())
	{
		return;
	}

	for (i = 0; i < g_colors; i++)
	{
		brt = int(Red(i)) + int(Green(i)) + int(Blue(i));
		if (brt > maxb)
		{
			maxb = brt;
			_brightIndex = i;
		}
		if (brt < minb)
		{
			minb = brt;
			_darkIndex = i;
		}
		if (brt < 150 && brt > 80)
		{
			maxgun = int(Red(i));
			mingun = int(Red(i));
			if (int(Green(i)) > int(Red(i)))
			{
				maxgun = int(Green(i));
			}
			else
			{
				mingun = int(Green(i));
			}
			if (int(Blue(i)) > maxgun)
			{
				maxgun = int(Blue(i));
			}
			if (int(Blue(i)) < mingun)
			{
				mingun = int(Blue(i));
			}
			if (brt - (maxgun - mingun)/2 > med)
			{
				_mediumIndex = i;
				med = brt - (maxgun - mingun)/2;
			}
		}
	}
}
