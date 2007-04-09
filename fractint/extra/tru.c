/*
  Fractint writes the escape iteration of fractal to a 24 bit Targa file
  called ITERATES.TGA. This program reads that file and recovers the
  iteration value. You can write your own truecolor mapping algorithm. It
  should go in the routine rgbmap().
*/

#include <stdio.h>

static long maxi = -1;

void rgbmap(long maxiter, long iter,
	unsigned char *r, unsigned char *g, unsigned char *b)
{
	if (iter > maxi)
	{
		maxi = iter;
	}
	*b = iter & 0xff;
	*g = (iter >> 8) & 0xff;
	*r = (iter >> 16) & 0xff;
}

main()
{
	int g_x_dots, g_y_dots, i, j, err;
	long iter, maxiter;
	char red, green, blue;
	FILE *fpin, *fpout;
	char buf1[12];
	char buf2[2];

	fpin = fopen("iterates.tga", "rb");
	if (fpin == NULL)
	{
		fprintf(stderr, "Can't open flat.out\n");
		exit(1);
	}
	fpout = fopen("new.tga", "wb");
	if (fpout == NULL)
	{
		fprintf(stderr, "Can't open new.tga\n");
		exit(1);
	}

	fread(buf1,    12, 1, fpin);    /* read 12 bytes */
	fread(&g_x_dots,   2, 1, fpin);    /* read g_x_dots */
	fread(&g_y_dots,   2, 1, fpin);    /* read g_y_dots */
	fread(buf2,     2, 1, fpin);    /* read 2 bytes */
	fread(&maxiter, 4, 1, fpin);    /* read 4 bytes */
	buf1[0] = 0; /* were not writing maxiter */
	fwrite(buf1,   12, 1, fpout);   /* write 12 bytes */
	fwrite(&g_x_dots,  2, 1, fpout);   /* write g_x_dots */
	fwrite(&g_y_dots,  2, 1, fpout);   /* write g_y_dots */
	fwrite(buf2,    2, 1, fpout);   /* write 2 bytes */

	printf("g_x_dots %d g_y_dots %d maxiter %ld\n", g_x_dots, g_y_dots, maxiter);

	for (j = 0; j < g_y_dots; j++)
	{
//      printf("row %2d maxi %6ld   \r", j, maxi);
		for (i = 0; i < g_x_dots; i++)
		{
			iter = 0;
			err = fread(&iter, 3, 1, fpin);
			if (err == 0)
			{
				printf("err at row %d col %d\n", j, i);
				exit(1);
			}
			printf("row %2d col %2d iter %8ld          \n", j, i, iter);
			/* map iterations to g_colors */
			rgbmap(maxiter, iter, &red, &green, &blue);
			fwrite(&blue, 1, 1, fpout);
			fwrite(&green, 1, 1, fpout);
			fwrite(&red,  1, 1, fpout);
		}
	}
}

