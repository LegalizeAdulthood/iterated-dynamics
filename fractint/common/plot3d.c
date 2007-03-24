/*
    This file includes miscellaneous plot functions and logic
    for 3D, used by lorenz.c and line3d.c
    By Tim Wegner and Marc Reinig.
*/

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"

/* Use these palette indices for red/blue - same on ega/vga */
#define PAL_BLUE    1
#define PAL_RED 2
#define PAL_MAGENTA 3

int g_which_image;
int xxadjust1;
int yyadjust1;
int g_eye_separation = 0;
int g_glasses_type = STEREO_NONE;
int xshift1;
int yshift1;
int xtrans = 0;
int ytrans = 0;
int red_local_left;
int red_local_right;
int blue_local_left;
int blue_local_right;
int red_crop_left   = 4;
int red_crop_right  = 0;
int blue_crop_left  = 0;
int blue_crop_right = 4;
int red_bright      = 80;
int blue_bright     = 100;

BYTE T_RED;

/* Bresenham's algorithm for drawing line */
void cdecl draw_line (int X1, int Y1, int X2, int Y2, int color)

{               /* uses Bresenham algorithm to draw a line */
    int dX, dY;                     /* vector components */
    int row, col,
        final,                      /* final row or column number */
        G,                  /* used to test for new row or column */
        inc1,           /* G increment when row or column doesn't change */
        inc2;               /* G increment when row or column changes */
    char pos_slope;

    dX = X2 - X1;                   /* find vector components */
    dY = Y2 - Y1;
    pos_slope = (char)(dX > 0);                   /* is slope positive? */
    if (dY < 0)
    pos_slope = (char)!pos_slope;
    if (abs (dX) > abs (dY))                /* shallow line case */
    {
        if (dX > 0)         /* determine start point and last column */
        {
            col = X1;
            row = Y1;
            final = X2;
        }
        else
        {
            col = X2;
            row = Y2;
            final = X1;
        }
        inc1 = 2 * abs (dY);            /* determine increments and initial G */
        G = inc1 - abs (dX);
        inc2 = 2 * (abs (dY) - abs (dX));
        if (pos_slope)
            while (col <= final)    /* step through columns checking for new row */
            {
                (*plot) (col, row, color);
                col++;
                if (G >= 0)             /* it's time to change rows */
                {
                    row++;      /* positive slope so increment through the rows */
                    G += inc2;
                }
                else                        /* stay at the same row */
                    G += inc1;
            }
        else
            while (col <= final)    /* step through columns checking for new row */
            {
                (*plot) (col, row, color);
                col++;
                if (G > 0)              /* it's time to change rows */
                {
                    row--;      /* negative slope so decrement through the rows */
                    G += inc2;
                }
                else                        /* stay at the same row */
                    G += inc1;
            }
    }   /* if |dX| > |dY| */
    else                            /* steep line case */
    {
        if (dY > 0)             /* determine start point and last row */
        {
            col = X1;
            row = Y1;
            final = Y2;
        }
        else
        {
            col = X2;
            row = Y2;
            final = Y1;
        }
        inc1 = 2 * abs (dX);            /* determine increments and initial G */
        G = inc1 - abs (dY);
        inc2 = 2 * (abs (dX) - abs (dY));
        if (pos_slope)
            while (row <= final)    /* step through rows checking for new column */
            {
                (*plot) (col, row, color);
                row++;
                if (G >= 0)                 /* it's time to change columns */
                {
                    col++;  /* positive slope so increment through the columns */
                    G += inc2;
                }
                else                    /* stay at the same column */
                    G += inc1;
            }
        else
            while (row <= final)    /* step through rows checking for new column */
            {
                (*plot) (col, row, color);
                row++;
                if (G > 0)                  /* it's time to change columns */
                {
                    col--;  /* negative slope so decrement through the columns */
                    G += inc2;
                }
                else                    /* stay at the same column */
                    G += inc1;
            }
    }
}   /* draw_line */

#if 0
/* use this for continuous colors later */
void _fastcall plot3dsuperimpose16b(int x, int y, int color)
{
    int tmp;
    if (color != 0)         /* Keeps index 0 still 0 */
    {
        color = colors - color; /*  Reverses color order */
        color = color / 4;
        if (color == 0)
            color = 1;
    }
    color = 3;
    tmp = getcolor(x, y);

    /* map to 4 colors */
    if (g_which_image == 1) /* RED */
    {
        if (red_local_left < x && x < red_local_right)
        {
            putcolor(x, y, color|tmp);
            if (Targa_Out)
                targa_color(x, y, color|tmp);
        }
    }
    else if (g_which_image == 2) /* BLUE */
        if (blue_local_left < x && x < blue_local_right)
        {
            color = color <<2;
            putcolor(x, y, color|tmp);
            if (Targa_Out)
                targa_color(x, y, color|tmp);
        }
}

#endif

void _fastcall plot3dsuperimpose16(int x, int y, int color)
{
    int tmp;

    tmp = getcolor(x, y);

    if (g_which_image == 1) /* RED */
    {
        color = PAL_RED;
        if (tmp > 0 && tmp != color)
            color = PAL_MAGENTA;
        if (red_local_left < x && x < red_local_right)
        {
            putcolor(x, y, color);
            if (Targa_Out)
                targa_color(x, y, color);
        }
    }
    else if (g_which_image == 2) /* BLUE */
        if (blue_local_left < x && x < blue_local_right)
        {
            color = PAL_BLUE;
            if (tmp > 0 && tmp != color)
                color = PAL_MAGENTA;
            putcolor(x, y, color);
            if (Targa_Out)
                targa_color(x, y, color);
        }
}


void _fastcall plot3dsuperimpose256(int x, int y, int color)
{
    int tmp;
    BYTE t_c;

    t_c = (BYTE)(255-color);

    if (color != 0)         /* Keeps index 0 still 0 */
    {
        color = colors - color; /*  Reverses color order */
        if (max_colors == 236)
        color = 1 + color / 21; /*  Maps colors 1-255 to 13 even ranges */
        else
        color = 1 + color / 18; /*  Maps colors 1-255 to 15 even ranges */
    }

    tmp = getcolor(x, y);
    /* map to 16 colors */
    if (g_which_image == 1) /* RED */
    {
        if (red_local_left < x && x < red_local_right)
        {
            /* Overwrite prev Red don't mess w/blue */
            putcolor(x, y, color|(tmp&240));
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, color|(tmp&240));
                else
                    targa_writedisk (x+sxoffs, y+syoffs, t_c, 0, 0);
            }
        }
    }
    else if (g_which_image == 2) /* BLUE */
        if (blue_local_left < x && x < blue_local_right)
        {
            /* Overwrite previous blue, don't mess with existing red */
            color = color <<4;
            putcolor(x, y, color|(tmp&15));
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, color|(tmp&15));
                else
                {
                    targa_readdisk (x+sxoffs, y+syoffs, &T_RED, (BYTE *)&tmp, (BYTE *)&tmp);
                    targa_writedisk (x+sxoffs, y+syoffs, T_RED, 0, t_c);
                }
            }
        }
}

void _fastcall plotIFS3dsuperimpose256(int x, int y, int color)
{
    int tmp;
    BYTE t_c;

    t_c = (BYTE)(255-color);

    if (color != 0)         /* Keeps index 0 still 0 */
    {
        /* my mind is fried - lower indices = darker colors is EASIER! */
        color = colors - color; /*  Reverses color order */
        if (max_colors == 236)
        color = 1 + color / 21; /*  Maps colors 1-255 to 13 even ranges */
        else
        color = 1 + color / 18; /*  Looks weird but maps colors 1-255 to 15
                    relatively even ranges */
    }

    tmp = getcolor(x, y);
    /* map to 16 colors */
    if (g_which_image == 1) /* RED */
    {
        if (red_local_left < x && x < red_local_right)
        {
            putcolor(x, y, color|tmp);
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, color|tmp);
                else
                    targa_writedisk (x+sxoffs, y+syoffs, t_c, 0, 0);
             }
        }
    }
    else if (g_which_image == 2) /* BLUE */
        if (blue_local_left < x && x < blue_local_right)
        {
            color = color <<4;
            putcolor(x, y, color|tmp);
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, color|tmp);
                else
                {
                    targa_readdisk (x+sxoffs, y+syoffs, &T_RED, (BYTE *)&tmp, (BYTE *)&tmp);
                    targa_writedisk (x+sxoffs, y+syoffs, T_RED, 0, t_c);
                }
            }
        }
}

void _fastcall plot3dalternate(int x, int y, int color)
{
    BYTE t_c;

    t_c = (BYTE)(255-color);
    /* lorez high color red/blue 3D plot function */
    /* if which image = 1, compresses color to lower 128 colors */

    /* my mind is STILL fried - lower indices = darker colors is EASIER! */
    color = colors - color;
    if ((g_which_image == 1) && !((x+y)&1)) /* - lower half palette */
    {
        if (red_local_left < x && x < red_local_right)
        {
            putcolor(x, y, color >> 1);
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, color >> 1);
                else
                    targa_writedisk (x+sxoffs, y+syoffs, t_c, 0, 0);
            }
        }
    }
    else if ((g_which_image == 2) && ((x+y)&1)) /* - upper half palette */
    {
        if (blue_local_left < x && x < blue_local_right)
        {
            putcolor(x, y, (color >> 1)+(colors >> 1));
            if (Targa_Out) {
                if (!ILLUMINE)
                    targa_color(x, y, (color >> 1)+(colors >> 1));
                else
                    targa_writedisk (x+sxoffs, y+syoffs, T_RED, 0, t_c);
            }
        }
    }
}

void _fastcall plot3dcrosseyedA(int x, int y, int color)
{
   x /= 2;
   y /= 2;
   if (g_which_image == 2)
      x += xdots/2;
   if (g_row_count >= ydots/2)
      /* hidden surface kludge */
      if (getcolor(x, y) != 0)
         return;
   putcolor(x, y, color);
}

void _fastcall plot3dcrosseyedB(int x, int y, int color)
{
   x /= 2;
   y /= 2;
   if (g_which_image == 2)
      x += xdots/2;
   putcolor(x, y, color);
}

void _fastcall plot3dcrosseyedC(int x, int y, int color)
{
   if (g_row_count >= ydots/2)
      /* hidden surface kludge */
      if (getcolor(x, y) != 0)
         return;
   putcolor(x, y, color);
}

void plot_setup()
{
    double d_red_bright  = 0;
    double d_blue_bright = 0;
    int i;

    /* set funny glasses plot function */
    switch (g_glasses_type)
    {
    case STEREO_ALTERNATE:
        standardplot = plot3dalternate;
        break;

    case STEREO_SUPERIMPOSE:
        if (colors == 256)
            if (fractype != IFS3D)
                standardplot = plot3dsuperimpose256;
            else
                standardplot = plotIFS3dsuperimpose256;
        else
            standardplot = plot3dsuperimpose16;
        break;

    case 4: /* crosseyed mode */
        if (sxdots < 2*xdots)
        {
           if (XROT == 0 && YROT == 0)
              standardplot = plot3dcrosseyedA; /* use hidden surface kludge */
           else
              standardplot = plot3dcrosseyedB;
        }
        else if (XROT == 0 && YROT == 0)
           standardplot = plot3dcrosseyedC; /* use hidden surface kludge */
        else
           standardplot = putcolor;
        break;

    default:
        standardplot = putcolor;
        break;
    }

    xshift1 = xshift = (int)((XSHIFT * (double)xdots)/100);
    yshift1 = yshift = (int)((YSHIFT * (double)ydots)/100);

    if (g_glasses_type)
    {
        red_local_left  =   (int)((red_crop_left      * (double)xdots)/100.0);
        red_local_right =   (int)(((100 - red_crop_right) * (double)xdots)/100.0);
        blue_local_left =   (int)((blue_crop_left     * (double)xdots)/100.0);
        blue_local_right =  (int)(((100 - blue_crop_right) * (double)xdots)/100.0);
        d_red_bright    =   (double)red_bright/100.0;
        d_blue_bright   =   (double)blue_bright/100.0;

        switch (g_which_image)
        {
        case 1:
            xshift  += (int)((g_eye_separation* (double)xdots)/200);
            xxadjust = (int)(((xtrans+xadjust)* (double)xdots)/100);
            xshift1 -= (int)((g_eye_separation* (double)xdots)/200);
            xxadjust1 = (int)(((xtrans-xadjust)* (double)xdots)/100);
            if (g_glasses_type == STEREO_PAIR && sxdots >= 2*xdots)
               sxoffs = sxdots / 2 - xdots;
            break;

        case 2:
            xshift  -= (int)((g_eye_separation* (double)xdots)/200);
            xxadjust = (int)(((xtrans-xadjust)* (double)xdots)/100);
            if (g_glasses_type == STEREO_PAIR && sxdots >= 2*xdots)
               sxoffs = sxdots / 2;
            break;
        }
    }
    else
        xxadjust = (int)((xtrans* (double)xdots)/100);
    yyadjust = (int)(-(ytrans* (double)ydots)/100);

    if (mapset)
    {
        ValidateLuts(MAP_name); /* read the palette file */
        if (g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE)
        {
            if (g_glasses_type == STEREO_SUPERIMPOSE && colors < 256)
            {
                g_dac_box[PAL_RED  ][0] = 63;
                g_dac_box[PAL_RED  ][1] =  0;
                g_dac_box[PAL_RED  ][2] =  0;

                g_dac_box[PAL_BLUE ][0] =  0;
                g_dac_box[PAL_BLUE ][1] =  0;
                g_dac_box[PAL_BLUE ][2] = 63;

                g_dac_box[PAL_MAGENTA][0] = 63;
                g_dac_box[PAL_MAGENTA][1] =    0;
                g_dac_box[PAL_MAGENTA][2] = 63;
            }
            for (i = 0; i < 256; i++)
            {
                g_dac_box[i][0] = (BYTE)(g_dac_box[i][0] * d_red_bright);
                g_dac_box[i][2] = (BYTE)(g_dac_box[i][2] * d_blue_bright);
            }
        }
        spindac(0, 1); /* load it, but don't spin */
    }
}

