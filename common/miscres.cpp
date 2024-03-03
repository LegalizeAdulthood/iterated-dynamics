/*
        Resident odds and ends that don't fit anywhere else.
*/
#include "port.h"
#include "prototyp.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "find_file.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractype.h"
#include "get_ifs_token.h"
#include "get_key_no_help.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_io.h"
#include "jb.h"
#include "line3d.h"
#include "loadfile.h"
#include "lorenz.h"
#include "make_path.h"
#include "miscres.h"
#include "parser.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "soi.h"
#include "split_path.h"
#include "update_save_name.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>

static void trigdetails(char *buf);
static void area();

void notdiskmsg()
{
    stopmsg(STOPMSG_NONE,
            "This type may be slow using a real-disk based 'video' mode, but may not \n"
            "be too bad if you have enough expanded or extended memory. Press <Esc> to \n"
            "abort if it appears that your disk drive is working too hard.");
}

// Wrapping version of putstring for long numbers
// row     -- pointer to row variable, internally incremented if needed
// col1    -- starting column
// col2    -- last column
// color   -- attribute (same as for putstring)
// maxrow -- max number of rows to write
// returns false if success, true if hit maxrow before done
static bool putstringwrap(int *row, int col1, int col2, int color, char *str, int maxrow)
{
    char save1, save2;
    int length, decpt, padding, startrow;
    bool done = false;
    startrow = *row;
    length = (int) std::strlen(str);
    padding = 3; // space between col1 and decimal.
    // find decimal point
    for (decpt = 0; decpt < length; decpt++)
    {
        if (str[decpt] == '.')
        {
            break;
        }
    }
    if (decpt >= length)
    {
        decpt = 0;
    }
    if (decpt < padding)
    {
        padding -= decpt;
    }
    else
    {
        padding = 0;
    }
    col1 += padding;
    decpt += col1+1; // column just past where decimal is
    while (length > 0)
    {
        if (col2-col1 < length)
        {
            done = (*row - startrow + 1) >= maxrow;
            save1 = str[col2-col1+1];
            save2 = str[col2-col1+2];
            if (done)
            {
                str[col2-col1+1]   = '+';
            }
            else
            {
                str[col2-col1+1]   = '\\';

            }
            str[col2-col1+2] = 0;
            driver_put_string(*row, col1, color, str);
            if (done)
            {
                break;
            }
            str[col2-col1+1] = save1;
            str[col2-col1+2] = save2;
            str += col2-col1;
            (*row)++;
        }
        else
        {
            driver_put_string(*row, col1, color, str);
        }
        length -= col2-col1;
        col1 = decpt; // align with decimal
    }
    return done;
}

#define rad_to_deg(x) ((x)*(180.0/PI)) // most people "think" in degrees
#define deg_to_rad(x) ((x)*(PI/180.0))
/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the Skew angles
*/

void cvtcentermag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
    double Height;

    // simple normal case first
    if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    {
        // no rotation or skewing, but stretching is allowed
        double Width = g_x_max - g_x_min;
        Height = g_y_max - g_y_min;
        *Xctr = (g_x_min + g_x_max)/2.0;
        *Yctr = (g_y_min + g_y_max)/2.0;
        *Magnification  = 2.0/Height;
        *Xmagfactor =  Height / (DEFAULT_ASPECT * Width);
        *Rotation = 0.0;
        *Skew = 0.0;
    }
    else
    {
        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        double tmpx1 = g_x_max - g_x_min;
        double tmpy1 = g_y_max - g_y_min;
        double c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        tmpx1 = g_x_max - g_x_3rd;
        tmpy1 = g_y_min - g_y_3rd;
        double a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        double a = std::sqrt(a2);
        *Rotation = -rad_to_deg(std::atan2(tmpy1, tmpx1));   // negative for image rotation

        double tmpx2 = g_x_min - g_x_3rd;
        double tmpy2 = g_y_max - g_y_3rd;
        double b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        double b = std::sqrt(b2);

        double tmpa = std::acos((a2+b2-c2)/(2*a*b)); // save tmpa for later use
        *Skew = 90.0 - rad_to_deg(tmpa);

        *Xctr = (g_x_min + g_x_max)*0.5;
        *Yctr = (g_y_min + g_y_max)*0.5;

        Height = b * std::sin(tmpa);

        *Magnification  = 2.0/Height; // 1/(h/2)
        *Xmagfactor = Height / (DEFAULT_ASPECT * a);

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
        {
            *Skew = -*Skew;
            *Xmagfactor = -*Xmagfactor;
            *Magnification = -*Magnification;
        }
    }
    // just to make par file look nicer
    if (*Magnification < 0)
    {
        *Magnification = -*Magnification;
        *Rotation += 180;
    }
#ifdef DEBUG
    {
        double txmin, txmax, tx3rd, tymin, tymax, ty3rd;
        double error;
        txmin = xxmin;
        txmax = xxmax;
        tx3rd = xx3rd;
        tymin = yymin;
        tymax = yymax;
        ty3rd = yy3rd;
        cvtcorners(*Xctr, *Yctr, *Magnification, *Xmagfactor, *Rotation, *Skew);
        error = sqr(txmin - xxmin) +
                sqr(txmax - xxmax) +
                sqr(tx3rd - xx3rd) +
                sqr(tymin - yymin) +
                sqr(tymax - yymax) +
                sqr(ty3rd - yy3rd);
        if (error > .001)
        {
            showcornersdbl("cvtcentermag problem");
        }
        xxmin = txmin;
        xxmax = txmax;
        xx3rd = tx3rd;
        yymin = tymin;
        yymax = tymax;
        yy3rd = ty3rd;
    }
#endif
    return;
}


// convert center/mag to corners
void cvtcorners(double Xctr, double Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
    double x, y;
    double h, w; // half height, width
    double tanskew, sinrot, cosrot;

    if (Xmagfactor == 0.0)
    {
        Xmagfactor = 1.0;
    }

    h = (double)(1/Magnification);
    w = h / (DEFAULT_ASPECT * Xmagfactor);

    if (Rotation == 0.0 && Skew == 0.0)
    {
        // simple, faster case
        g_x_min = Xctr - w;
        g_x_3rd = g_x_min;
        g_x_max = Xctr + w;
        g_y_min = Yctr - h;
        g_y_3rd = g_y_min;
        g_y_max = Yctr + h;
        return;
    }

    // in unrotated, untranslated coordinate system
    tanskew = std::tan(deg_to_rad(Skew));
    g_x_min = -w + h*tanskew;
    g_x_max =  w - h*tanskew;
    g_x_3rd = -w - h*tanskew;
    g_y_max = h;
    g_y_min = -h;
    g_y_3rd = g_y_min;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    sinrot = std::sin(Rotation);
    cosrot = std::cos(Rotation);

    // top left
    x = g_x_min * cosrot + g_y_max *  sinrot;
    y = -g_x_min * sinrot + g_y_max *  cosrot;
    g_x_min = x + Xctr;
    g_y_max = y + Yctr;

    // bottom right
    x = g_x_max * cosrot + g_y_min *  sinrot;
    y = -g_x_max * sinrot + g_y_min *  cosrot;
    g_x_max = x + Xctr;
    g_y_min = y + Yctr;

    // bottom left
    x = g_x_3rd * cosrot + g_y_3rd *  sinrot;
    y = -g_x_3rd * sinrot + g_y_3rd *  cosrot;
    g_x_3rd = x + Xctr;
    g_y_3rd = y + Yctr;

    return;
}

// convert corners to center/mag using bf
void cvtcentermagbf(bf_t Xctr, bf_t Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
    // needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    LDBL Height;
    bf_t bfWidth;
    bf_t bftmpx;
    int saved;

    saved = save_stack();

    // simple normal case first
    // if (xx3rd == xxmin && yy3rd == yymin)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
    {
        // no rotation or skewing, but stretching is allowed
        bfWidth  = alloc_stack(bflength+2);
        bf_t bfHeight = alloc_stack(bflength+2);
        // Width  = xxmax - xxmin;
        sub_bf(bfWidth, g_bf_x_max, g_bf_x_min);
        LDBL Width = bftofloat(bfWidth);
        // Height = yymax - yymin;
        sub_bf(bfHeight, g_bf_y_max, g_bf_y_min);
        Height = bftofloat(bfHeight);
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, g_bf_x_min, g_bf_x_max);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, g_bf_y_min, g_bf_y_max);
        half_a_bf(Yctr);
        *Magnification  = 2/Height;
        *Xmagfactor = (double)(Height / (DEFAULT_ASPECT * Width));
        *Rotation = 0.0;
        *Skew = 0.0;
    }
    else
    {
        bftmpx = alloc_stack(bflength+2);
        bf_t bftmpy = alloc_stack(bflength+2);

        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        // IMPORTANT: convert from bf AFTER subtracting

        // tmpx = xxmax - xxmin;
        sub_bf(bftmpx, g_bf_x_max, g_bf_x_min);
        LDBL tmpx1 = bftofloat(bftmpx);
        // tmpy = yymax - yymin;
        sub_bf(bftmpy, g_bf_y_max, g_bf_y_min);
        LDBL tmpy1 = bftofloat(bftmpy);
        LDBL c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        // tmpx = xxmax - xx3rd;
        sub_bf(bftmpx, g_bf_x_max, g_bf_x_3rd);
        tmpx1 = bftofloat(bftmpx);

        // tmpy = yymin - yy3rd;
        sub_bf(bftmpy, g_bf_y_min, g_bf_y_3rd);
        tmpy1 = bftofloat(bftmpy);
        LDBL a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        LDBL a = sqrtl(a2);

        // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
        // atan2() only depends on the ratio, this puts it in double's range
        int signx = sign(tmpx1);
        LDBL tmpy = 0.0;
        if (signx)
        {
            tmpy = tmpy1/tmpx1 * signx;    // tmpy = tmpy / |tmpx|
        }
        *Rotation = (double)(-rad_to_deg(std::atan2((double)tmpy, signx)));   // negative for image rotation

        // tmpx = xxmin - xx3rd;
        sub_bf(bftmpx, g_bf_x_min, g_bf_x_3rd);
        LDBL tmpx2 = bftofloat(bftmpx);
        // tmpy = yymax - yy3rd;
        sub_bf(bftmpy, g_bf_y_max, g_bf_y_3rd);
        LDBL tmpy2 = bftofloat(bftmpy);
        LDBL b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        LDBL b = sqrtl(b2);

        double tmpa = std::acos((double)((a2+b2-c2)/(2*a*b))); // save tmpa for later use
        *Skew = 90 - rad_to_deg(tmpa);

        // these are the only two variables that must use big precision
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, g_bf_x_min, g_bf_x_max);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, g_bf_y_min, g_bf_y_max);
        half_a_bf(Yctr);

        Height = b * std::sin(tmpa);
        *Magnification  = 2/Height; // 1/(h/2)
        *Xmagfactor = (double)(Height / (DEFAULT_ASPECT * a));

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
        {
            *Skew = -*Skew;
            *Xmagfactor = -*Xmagfactor;
            *Magnification = -*Magnification;
        }
    }
    if (*Magnification < 0)
    {
        *Magnification = -*Magnification;
        *Rotation += 180;
    }
    restore_stack(saved);
    return;
}


// convert center/mag to corners using bf
void cvtcornersbf(bf_t Xctr, bf_t Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
    LDBL x, y;
    LDBL h, w; // half height, width
    LDBL xmin, ymin, xmax, ymax, x3rd, y3rd;
    double tanskew, sinrot, cosrot;
    bf_t bfh, bfw;
    bf_t bftmp;
    int saved;

    saved = save_stack();
    bfh = alloc_stack(bflength+2);
    bfw = alloc_stack(bflength+2);

    if (Xmagfactor == 0.0)
    {
        Xmagfactor = 1.0;
    }

    h = 1/Magnification;
    floattobf(bfh, h);
    w = h / (DEFAULT_ASPECT * Xmagfactor);
    floattobf(bfw, w);

    if (Rotation == 0.0 && Skew == 0.0)
    {
        // simple, faster case
        // xx3rd = xxmin = Xctr - w;
        sub_bf(g_bf_x_min, Xctr, bfw);
        copy_bf(g_bf_x_3rd, g_bf_x_min);
        // xxmax = Xctr + w;
        add_bf(g_bf_x_max, Xctr, bfw);
        // yy3rd = yymin = Yctr - h;
        sub_bf(g_bf_y_min, Yctr, bfh);
        copy_bf(g_bf_y_3rd, g_bf_y_min);
        // yymax = Yctr + h;
        add_bf(g_bf_y_max, Yctr, bfh);
        restore_stack(saved);
        return;
    }

    bftmp = alloc_stack(bflength+2);
    // in unrotated, untranslated coordinate system
    tanskew = std::tan(deg_to_rad(Skew));
    xmin = -w + h*tanskew;
    xmax =  w - h*tanskew;
    x3rd = -w - h*tanskew;
    ymax = h;
    ymin = -h;
    y3rd = ymin;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    sinrot = std::sin(Rotation);
    cosrot = std::cos(Rotation);

    // top left
    x =  xmin * cosrot + ymax *  sinrot;
    y = -xmin * sinrot + ymax *  cosrot;
    // xxmin = x + Xctr;
    floattobf(bftmp, x);
    add_bf(g_bf_x_min, bftmp, Xctr);
    // yymax = y + Yctr;
    floattobf(bftmp, y);
    add_bf(g_bf_y_max, bftmp, Yctr);

    // bottom right
    x =  xmax * cosrot + ymin *  sinrot;
    y = -xmax * sinrot + ymin *  cosrot;
    // xxmax = x + Xctr;
    floattobf(bftmp, x);
    add_bf(g_bf_x_max, bftmp, Xctr);
    // yymin = y + Yctr;
    floattobf(bftmp, y);
    add_bf(g_bf_y_min, bftmp, Yctr);

    // bottom left
    x =  x3rd * cosrot + y3rd *  sinrot;
    y = -x3rd * sinrot + y3rd *  cosrot;
    // xx3rd = x + Xctr;
    floattobf(bftmp, x);
    add_bf(g_bf_x_3rd, bftmp, Xctr);
    // yy3rd = y + Yctr;
    floattobf(bftmp, y);
    add_bf(g_bf_y_3rd, bftmp, Yctr);

    restore_stack(saved);
    return;
}

static void check_writefile(char *name, char const *ext)
{
    // after v16 release, change encoder.c to also use this routine
    char openfile[FILE_MAX_DIR];
    char opentype[20];
    char const *period;
nextname:
    std::strcpy(openfile, name);
    std::strcpy(opentype, ext);
    period = has_ext(openfile);
    if (period != nullptr)
    {
        std::strcpy(opentype, period);
        openfile[period - openfile] = 0;
    }
    std::strcat(openfile, opentype);
    if (access(openfile, 0) != 0) // file doesn't exist
    {
        std::strcpy(name, openfile);
        return;
    }
    // file already exists
    if (!g_overwrite_file)
    {
        update_save_name(name);
        goto nextname;
    }
}

void check_writefile(std::string &name, char const *ext)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, name.c_str());
    check_writefile(buff, ext);
    name = buff;
}

trig_fn g_trig_index[] =
{
    trig_fn::SIN, trig_fn::SQR, trig_fn::SINH, trig_fn::COSH
};
#if !defined(XFRACT)
void (*ltrig0)() = lStkSin;
void (*ltrig1)() = lStkSqr;
void (*ltrig2)() = lStkSinh;
void (*ltrig3)() = lStkCosh;
void (*mtrig0)() = mStkSin;
void (*mtrig1)() = mStkSqr;
void (*mtrig2)() = mStkSinh;
void (*mtrig3)() = mStkCosh;
#endif
void (*dtrig0)() = dStkSin;
void (*dtrig1)() = dStkSqr;
void (*dtrig2)() = dStkSinh;
void (*dtrig3)() = dStkCosh;

// return display form of active trig functions
std::string showtrig()
{
    char tmpbuf[30];
    trigdetails(tmpbuf);
    if (tmpbuf[0])
    {
        return std::string{" function="} + tmpbuf;
    }
    return {};
}

static void trigdetails(char *buf)
{
    int numfn;
    char tmpbuf[20];
    if (g_fractal_type == fractal_type::JULIBROT || g_fractal_type == fractal_type::JULIBROTFP)
    {
        numfn = (g_fractal_specific[static_cast<int>(g_new_orbit_type)].flags >> 6) & 7;
    }
    else
    {
        numfn = (g_cur_fractal_specific->flags >> 6) & 7;
    }
    if (g_cur_fractal_specific == &g_fractal_specific[static_cast<int>(fractal_type::FORMULA)]
        || g_cur_fractal_specific == &g_fractal_specific[static_cast<int>(fractal_type::FFORMULA)])
    {
        numfn = g_max_function;
    }
    *buf = 0; // null string if none
    if (numfn > 0)
    {
        std::strcpy(buf, g_trig_fn[static_cast<int>(g_trig_index[0])].name);
        int i = 0;
        while (++i < numfn)
        {
            std::snprintf(tmpbuf, NUM_OF(tmpbuf), "/%s", g_trig_fn[static_cast<int>(g_trig_index[i])].name);
            std::strcat(buf, tmpbuf);
        }
    }
}

// set array of trig function indices according to "function=" command
int set_trig_array(int k, char const *name)
{
    char trigname[10];
    char *slash;
    std::strncpy(trigname, name, 6);
    trigname[6] = 0; // safety first

    slash = std::strchr(trigname, '/');
    if (slash != nullptr)
    {
        *slash = 0;
    }

    strlwr(trigname);

    for (int i = 0; i < g_num_trig_functions; i++)
    {
        if (std::strcmp(trigname, g_trig_fn[i].name) == 0)
        {
            g_trig_index[k] = static_cast<trig_fn>(i);
            set_trig_pointers(k);
            break;
        }
    }
    return 0;
}
void set_trig_pointers(int which)
{
    // set trig variable functions to avoid array lookup time
    switch (which)
    {
    case 0:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].lfunct;
        mtrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].mfunct;
#endif
        dtrig0 = g_trig_fn[static_cast<int>(g_trig_index[0])].dfunct;
        break;
    case 1:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].lfunct;
        mtrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].mfunct;
#endif
        dtrig1 = g_trig_fn[static_cast<int>(g_trig_index[1])].dfunct;
        break;
    case 2:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].lfunct;
        mtrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].mfunct;
#endif
        dtrig2 = g_trig_fn[static_cast<int>(g_trig_index[2])].dfunct;
        break;
    case 3:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].lfunct;
        mtrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].mfunct;
#endif
        dtrig3 = g_trig_fn[static_cast<int>(g_trig_index[3])].dfunct;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
        {
            set_trig_pointers(i);
        }
        break;
    }
}

#ifdef XFRACT
static char spressanykey[] = {"Press any key to continue, F6 for area, F7 for next page"};
#else
static char spressanykey[] = {"Press any key to continue, F6 for area, CTRL-TAB for next page"};
#endif

std::string get_calculation_time(long ctime)
{
    char msg[80];
    if (ctime >= 0)
    {
        std::snprintf(msg, NUM_OF(msg), "%3ld:%02ld:%02ld.%02ld", ctime/360000L,
                (ctime%360000L)/6000, (ctime%6000)/100, ctime%100);
    }
    else
    {
        std::strcpy(msg, "A long time! (> 24.855 days)");
    }
    return msg;
}

static void show_str_var(char const *name, char const *var, int *row, char *msg)
{
    if (var == nullptr)
    {
        return;
    }
    if (*var != 0)
    {
        std::sprintf(msg, "%s=%s", name, var);
        driver_put_string((*row)++, 2, C_GENERAL_HI, msg);
    }
}

static void write_row(int row, char const *format, ...)
{
    char text[78] = { 0 };
    std::va_list args;

    va_start(args, format);
    std::vsnprintf(text, NUM_OF(text), format, args);
    va_end(args);

    driver_put_string(row, 2, C_GENERAL_HI, text);
}

extern long maxstack;
extern long startstack;

bool tab_display_2(char *msg)
{
    int row, key = 0;

    helptitle();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background

    row = 1;
    putstringcenter(row++, 0, 80, C_PROMPT_HI, "Top Secret Developer's Screen");

    write_row(++row, "Version %d patch %d", g_release, g_patch_level);
    write_row(++row, "%ld of %ld bignum memory used", g_bignum_max_stack_addr, maxstack);
    write_row(++row, "   %ld used for bignum globals", startstack);
    write_row(++row, "   %ld stack used == %ld variables of length %d",
              g_bignum_max_stack_addr-startstack, (long)((g_bignum_max_stack_addr-startstack)/(rbflength+2)), rbflength+2);
    if (bf_math != bf_math_type::NONE)
    {
        write_row(++row, "intlength %-d bflength %-d ", intlength, bflength);
    }
    row++;
    show_str_var("tempdir",     g_temp_dir.c_str(),      &row, msg);
    show_str_var("workdir",     g_working_dir.c_str(),      &row, msg);
    show_str_var("filename",    g_read_filename.c_str(),     &row, msg);
    show_str_var("formulafile", g_formula_filename.c_str(), &row, msg);
    show_str_var("savename",    g_save_filename.c_str(),     &row, msg);
    show_str_var("parmfile",    g_command_file.c_str(),  &row, msg);
    show_str_var("ifsfile",     g_ifs_filename.c_str(),  &row, msg);
    show_str_var("autokeyname", g_auto_name.c_str(), &row, msg);
    show_str_var("lightname",   g_light_name.c_str(),   &row, msg);
    show_str_var("map",         g_map_name.c_str(),     &row, msg);
    write_row(row++, "Sizeof fractalspecific array %d",
              g_num_fractal_types*(int)sizeof(fractalspecificstuff));
    write_row(row, "calc_status %d pixel [%d, %d]", g_calc_status, g_col, row);
    ++row;
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        write_row(row++, "Max_Ops (posp) %u Max_Args (vsp) %u",
                  g_operation_index, g_variable_index);
        write_row(row++, "   Store ptr %d Loadptr %d Max_Ops var %u Max_Args var %u LastInitOp %d",
                  g_store_index, g_load_index, g_max_function_ops, g_max_function_args, g_last_init_op);
    }
    else if (g_rhombus_stack[0])
    {
        write_row(row++, "SOI Recursion %d stack free %d %d %d %d %d %d %d %d %d %d",
                  g_max_rhombus_depth+1,
                  g_rhombus_stack[0], g_rhombus_stack[1], g_rhombus_stack[2],
                  g_rhombus_stack[3], g_rhombus_stack[4], g_rhombus_stack[5],
                  g_rhombus_stack[6], g_rhombus_stack[7], g_rhombus_stack[8],
                  g_rhombus_stack[9]);
    }

    /*
        write_row(row++, "xdots %d ydots %d sxdots %d sydots %d", xdots, ydots, sxdots, sydots);
    */
    write_row(row++, "%dx%d dm=%d %s (%s)", g_logical_screen_x_dots, g_logical_screen_y_dots, g_dot_mode,
              g_driver->name, g_driver->description);
    write_row(row++, "xxstart %d xxstop %d yystart %d yystop %d %s uses_ismand %d",
              g_xx_start, g_xx_stop, g_yy_start, g_yy_stop,
#if !defined(XFRACT) && !defined(_WIN32)
              g_cur_fractal_specific->orbitcalc == fFormula ? "fast parser" :
#endif
              g_cur_fractal_specific->orbitcalc ==  Formula ? "slow parser" :
              g_cur_fractal_specific->orbitcalc ==  BadFormula ? "bad formula" :
              "", g_frm_uses_ismand ? 1 : 0);
    /*
        write_row(row++, "ixstart %d ixstop %d iystart %d iystop %d bitshift %d",
            ixstart, ixstop, iystart, iystop, bitshift);
    */
    write_row(row++, "minstackavail %d llimit2 %ld use_grid %d",
              g_soi_min_stack_available, g_l_magnitude_limit2, g_use_grid ? 1 : 0);
    putstringcenter(24, 0, 80, C_GENERAL_LO, "Press Esc to continue, Backspace for first screen");
    *msg = 0;

    // display keycodes while waiting for ESC, BACKSPACE or TAB
    while ((key != FIK_ESC) && (key != FIK_BACKSPACE) && (key != FIK_TAB))
    {
        driver_put_string(row, 2, C_GENERAL_HI, msg);
        key = getakeynohelp();
        std::sprintf(msg, "%d (0x%04x)      ", key, key);
    }
    return key != FIK_ESC;
}

int tab_display()       // display the status of the current image
{
    int addrow = 0;
    double Xctr;
    double Yctr;
    LDBL Magnification;
    double Xmagfactor;
    double Rotation;
    double Skew;
    bf_t bfXctr = nullptr;
    bf_t bfYctr = nullptr;
    char msg[350];
    char const *msgptr;
    int key;
    int saved = 0;
    int k;
    int hasformparam = 0;

    if (g_calc_status < calc_status_value::PARAMS_CHANGED)        // no active fractal image
    {
        return 0;                // (no TAB on the credits screen)
    }
    if (g_calc_status == calc_status_value::IN_PROGRESS)        // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    {
        g_calc_time += (std::clock() - g_timer_start) / (CLOCKS_PER_SEC/100);
    }
    driver_stack_screen();
    if (bf_math != bf_math_type::NONE)
    {
        saved = save_stack();
        bfXctr = alloc_stack(bflength+2);
        bfYctr = alloc_stack(bflength+2);
    }
    if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
    {
        for (int i = 0; i < MAX_PARAMS; i += 2)
        {
            if (!paramnotused(i))
            {
                hasformparam++;
            }
        }
    }

top:
    k = 0; /* initialize here so parameter line displays correctly on return
                from control-tab */
    helptitle();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background
    int s_row = 2;
    driver_put_string(s_row, 2, C_GENERAL_MED, "Fractal type:");
    if (g_display_3d > display_3d_modes::NONE)
    {
        driver_put_string(s_row, 16, C_GENERAL_HI, "3D Transform");
    }
    else
    {
        driver_put_string(s_row, 16, C_GENERAL_HI,
                          g_cur_fractal_specific->name[0] == '*' ?
                          &g_cur_fractal_specific->name[1] : g_cur_fractal_specific->name);
        int i = 0;
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_formula_name.c_str());
            i = static_cast<int>(g_formula_name.length() + 1);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if (g_formula_filename.length() >= 29)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, g_formula_filename.c_str());
        }
        trigdetails(msg);
        driver_put_string(s_row+1, 16+i, C_GENERAL_HI, msg);
        if (g_fractal_type == fractal_type::LSYSTEM)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_l_system_name.c_str());
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_l_system_filename.length() >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, g_l_system_filename.c_str());
        }
        if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, g_ifs_name.c_str());
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) g_ifs_filename.length() >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, g_ifs_filename.c_str());
        }
    }

    switch (g_calc_status)
    {
    case calc_status_value::PARAMS_CHANGED:
        msgptr = "Parms chgd since generated";
        break;
    case calc_status_value::IN_PROGRESS:
        msgptr = "Still being generated";
        break;
    case calc_status_value::RESUMABLE:
        msgptr = "Interrupted, resumable";
        break;
    case calc_status_value::NON_RESUMABLE:
        msgptr = "Interrupted, non-resumable";
        break;
    case calc_status_value::COMPLETED:
        msgptr = "Image completed";
        break;
    default:
        msgptr = "";
    }
    driver_put_string(s_row, 45, C_GENERAL_HI, msgptr);
    if (g_init_batch != batch_modes::NONE && g_calc_status != calc_status_value::PARAMS_CHANGED)
    {
        driver_put_string(-1, -1, C_GENERAL_HI, " (Batch mode)");
    }

    if (g_help_mode == help_labels::HELPCYCLING)
    {
        driver_put_string(s_row+1, 45, C_GENERAL_HI, "You are in color-cycling mode");
    }
    ++s_row;
    // if (bf_math == bf_math_type::NONE)
    ++s_row;

    int j = 0;
    if (g_display_3d > display_3d_modes::NONE)
    {
        if (g_user_float_flag)
        {
            j = 1;
        }
    }
    else if (g_float_flag)
    {
        j = g_user_float_flag ? 1 : 2;
    }

    if (bf_math == bf_math_type::NONE)
    {
        if (j)
        {
            driver_put_string(s_row, 45, C_GENERAL_HI, "Floating-point");
            driver_put_string(-1, -1, C_GENERAL_HI,
                              (j == 1) ? " flag is activated" : " in use (required)");
        }
        else
        {
            driver_put_string(s_row, 45, C_GENERAL_HI, "Integer math is in use");
        }
    }
    else
    {
        std::sprintf(msg, "(%-d decimals)", g_decimals /*getprecbf(CURRENTREZ)*/);
        driver_put_string(s_row, 45, C_GENERAL_HI, "Arbitrary precision ");
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }
    s_row += 1;

    if (g_calc_status == calc_status_value::IN_PROGRESS || g_calc_status == calc_status_value::RESUMABLE)
    {
        if (g_cur_fractal_specific->flags&NORESUME)
        {
            driver_put_string(s_row++, 2, C_GENERAL_HI,
                              "Note: can't resume this type after interrupts other than <tab> and <F1>");
        }
    }
    s_row += addrow;
    driver_put_string(s_row, 2, C_GENERAL_MED, "Savename: ");
    driver_put_string(s_row, -1, C_GENERAL_HI, g_save_filename.c_str());

    ++s_row;

    if (g_got_status >= 0 && (g_calc_status == calc_status_value::IN_PROGRESS || g_calc_status == calc_status_value::RESUMABLE))
    {
        switch (g_got_status)
        {
        case 0:
            std::sprintf(msg, "%d Pass Mode", g_total_passes);
            driver_put_string(s_row, 2, C_GENERAL_HI, msg);
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case 1:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Solid Guessing");
            if (g_user_std_calc_mode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case 2:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Boundary Tracing");
            break;
        case 3:
            std::sprintf(msg, "Processing row %d (of %d) of input image", g_current_row, g_file_y_dots);
            driver_put_string(s_row, 2, C_GENERAL_HI, msg);
            break;
        case 4:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Tesseral");
            break;
        case 5:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Diffusion");
            break;
        case 6:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Orbits");
            break;
        }
        ++s_row;
        if (g_got_status == 5)
        {
            std::sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
                    (100.0 * g_diffusion_counter)/g_diffusion_limit,
                    g_diffusion_counter, g_diffusion_limit, g_diffusion_bits);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            ++s_row;
        }
        else if (g_got_status != 3)
        {
            std::sprintf(msg, "Working on block (y, x) [%d, %d]...[%d, %d], ",
                    g_yy_start, g_xx_start, g_yy_stop, g_xx_stop);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            if (g_got_status == 2 || g_got_status == 4)  // btm or tesseral
            {
                driver_put_string(-1, -1, C_GENERAL_MED, "at ");
                std::sprintf(msg, "[%d, %d]", g_current_row, g_current_column);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            else
            {
                if (g_total_passes > 1)
                {
                    driver_put_string(-1, -1, C_GENERAL_MED, "pass ");
                    std::sprintf(msg, "%d", g_current_pass);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, " of ");
                    std::sprintf(msg, "%d", g_total_passes);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, ", ");
                }
                driver_put_string(-1, -1, C_GENERAL_MED, "at row ");
                std::sprintf(msg, "%d", g_current_row);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                driver_put_string(-1, -1, C_GENERAL_MED, " col ");
                std::sprintf(msg, "%d", g_col);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            ++s_row;
        }
    }
    driver_put_string(s_row, 2, C_GENERAL_MED, "Calculation time:");
    strncpy(msg, get_calculation_time(g_calc_time).c_str(), NUM_OF(msg));
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    if ((g_got_status == 5) && (g_calc_status == calc_status_value::IN_PROGRESS))  // estimate total time
    {
        driver_put_string(-1, -1, C_GENERAL_MED, " estimated total time: ");
        get_calculation_time((long) (g_calc_time * ((g_diffusion_limit * 1.0) / g_diffusion_counter)));
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if ((g_cur_fractal_specific->flags&INFCALC) && (g_color_iter != 0))
    {
        driver_put_string(s_row, -1, C_GENERAL_MED, " 1000's of points:");
        std::sprintf(msg, " %ld of %ld", g_color_iter-2, g_max_count);
        driver_put_string(s_row, -1, C_GENERAL_HI, msg);
    }

    ++s_row;
    if (bf_math == bf_math_type::NONE)
    {
        ++s_row;
    }
    std::snprintf(msg, NUM_OF(msg), "Driver: %s, %s", g_driver->name, g_driver->description);
    driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    if (g_video_entry.xdots && bf_math == bf_math_type::NONE)
    {
        std::sprintf(msg, "Video: %dx%dx%d %s %s",
                g_video_entry.xdots, g_video_entry.ydots, g_video_entry.colors,
                g_video_entry.name, g_video_entry.comment);
        driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    }
    if (!(g_cur_fractal_specific->flags&NOZOOM))
    {
        adjust_corner(); // make bottom left exact if very near exact
        if (bf_math != bf_math_type::NONE)
        {
            int truncaterow;
            int dec = std::min(320, g_decimals);
            adjust_cornerbf(); // make bottom left exact if very near exact
            cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            // find alignment information
            msg[0] = 0;
            bool truncate = false;
            if (dec < g_decimals)
            {
                truncate = true;
            }
            truncaterow = g_row;
            driver_put_string(++s_row, 2, C_GENERAL_MED, "Ctr");
            driver_put_string(s_row, 8, C_GENERAL_MED, "x");
            bftostr(msg, dec, bfXctr);
            if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5))
            {
                truncate = true;
            }
            driver_put_string(++s_row, 8, C_GENERAL_MED, "y");
            bftostr(msg, dec, bfYctr);
            if (putstringwrap(&s_row, 10, 78, C_GENERAL_HI, msg, 5) || truncate)
            {
                driver_put_string(truncaterow, 2, C_GENERAL_MED, "(Center values shown truncated to 320 decimals)");
            }
            driver_put_string(++s_row, 2, C_GENERAL_MED, "Mag");
            std::sprintf(msg, "%10.8Le", Magnification);
            driver_put_string(-1, 11, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
        else // bf != 1
        {
            driver_put_string(s_row, 2, C_GENERAL_MED, "Corners:                X                     Y");
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Top-l");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_min, g_y_max);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-r");
            std::sprintf(msg, "%20.16f  %20.16f", g_x_max, g_y_min);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);

            if (g_x_min != g_x_3rd || g_y_min != g_y_3rd)
            {
                driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-l");
                std::sprintf(msg, "%20.16f  %20.16f", g_x_3rd, g_y_3rd);
                driver_put_string(-1, 17, C_GENERAL_HI, msg);
            }
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Ctr");
            std::sprintf(msg, "%20.16f %20.16f  ", Xctr, Yctr);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Mag");
            std::sprintf(msg, " %10.8Le", Magnification);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            std::sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            std::sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            std::sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
    }

    if (typehasparm(g_fractal_type, 0, msg) || hasformparam)
    {
        for (int i = 0; i < MAX_PARAMS; i++)
        {
            char p[50];
            if (typehasparm(g_fractal_type, i, p))
            {
                int col;
                if (k%4 == 0)
                {
                    s_row++;
                    col = 9;
                }
                else
                {
                    col = -1;
                }
                if (k == 0)   // only true with first displayed parameter
                {
                    driver_put_string(++s_row, 2, C_GENERAL_MED, "Params ");
                }
                std::sprintf(msg, "%3d: ", i+1);
                driver_put_string(s_row, col, C_GENERAL_MED, msg);
                if (*p == '+')
                {
                    std::sprintf(msg, "%-12d", (int)g_params[i]);
                }
                else if (*p == '#')
                {
                    std::sprintf(msg, "%-12u", (U32)g_params[i]);
                }
                else
                {
                    std::sprintf(msg, "%-12.9f", g_params[i]);
                }
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                k++;
            }
        }
    }
    driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Current (Max) Iteration: ");
    std::sprintf(msg, "%ld (%ld)", g_color_iter, g_max_iterations);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    driver_put_string(-1, -1, C_GENERAL_MED, "     Effective bailout: ");
    std::sprintf(msg, "%f", g_magnitude_limit);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);

    if (g_fractal_type == fractal_type::PLASMA || g_fractal_type == fractal_type::ANT || g_fractal_type == fractal_type::CELLULAR)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Current 'rseed': ");
        std::sprintf(msg, "%d", g_random_seed);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (g_invert != 0)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Inversion radius: ");
        std::sprintf(msg, "%12.9f", g_f_radius);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  xcenter: ");
        std::sprintf(msg, "%12.9f", g_f_x_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  ycenter: ");
        std::sprintf(msg, "%12.9f", g_f_y_center);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if ((s_row += 2) < 23)
    {
        ++s_row;
    }
    //waitforkey:
    putstringcenter(/*s_row*/24, 0, 80, C_GENERAL_LO, spressanykey);
    driver_hide_text_cursor();
#ifdef XFRACT
    while (driver_key_pressed())
    {
        driver_get_key();
    }
#endif
    key = getakeynohelp();
    if (key == FIK_F6)
    {
        driver_stack_screen();
        area();
        driver_unstack_screen();
        goto top;
    }
    else if (key == FIK_CTL_TAB || key == FIK_SHF_TAB || key == FIK_F7)
    {
        if (tab_display_2(msg))
        {
            goto top;
        }
    }
    driver_unstack_screen();
    g_timer_start = std::clock(); // tab display was "time out"
    if (bf_math != bf_math_type::NONE)
    {
        restore_stack(saved);
    }
    return 0;
}

static void area()
{
    char const *msg;
    char buf[160];
    long cnt = 0;
    if (g_inside_color < COLOR_BLACK)
    {
        stopmsg(STOPMSG_NONE, "Need solid inside to compute area");
        return;
    }
    for (int y = 0; y < g_logical_screen_y_dots; y++)
    {
        for (int x = 0; x < g_logical_screen_x_dots; x++)
        {
            if (getcolor(x, y) == g_inside_color)
            {
                cnt++;
            }
        }
    }
    if (g_inside_color > COLOR_BLACK && g_outside_color < COLOR_BLACK && g_max_iterations > g_inside_color)
    {
        msg = "Warning: inside may not be unique\n";
    }
    else
    {
        msg = "";
    }
    std::sprintf(buf, "%s%ld inside pixels of %ld%s%f",
            msg, cnt, (long)g_logical_screen_x_dots*(long)g_logical_screen_y_dots, ".  Total area ",
            cnt/((float)g_logical_screen_x_dots*(float)g_logical_screen_y_dots)*(g_x_max-g_x_min)*(g_y_max-g_y_min));
    stopmsg(STOPMSG_NO_BUZZER, buf);
}

int endswithslash(char const *fl)
{
    int len;
    len = (int) std::strlen(fl);
    if (len)
    {
        if (fl[--len] == SLASHC)
        {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------
static char seps[] = {' ', '\t', '\n', '\r'};

char *get_ifs_token(char *buf, std::FILE *ifsfile)
{
    char *bufptr;
    while (true)
    {
        if (file_gets(buf, 200, ifsfile) < 0)
        {
            return nullptr;
        }
        else
        {
            bufptr = std::strchr(buf, ';');
            if (bufptr != nullptr)   // use ';' as comment to eol
            {
                *bufptr = 0;
            }
            bufptr = std::strtok(buf, seps);
            if (bufptr != nullptr)
            {
                return bufptr;
            }
        }
    }
}

char *get_next_ifs_token(char *buf, std::FILE *ifsfile)
{
    char *bufptr = std::strtok(nullptr, seps);
    if (bufptr == nullptr)
    {
        bufptr = get_ifs_token(buf, ifsfile);
    }
    return bufptr;
}

int g_num_affine_transforms;
int ifsload()                   // read in IFS parameters
{
    // release prior params
    g_ifs_definition.clear();
    g_ifs_type = false;
    std::FILE *ifsfile;
    if (find_file_item(g_ifs_filename, g_ifs_name.c_str(), &ifsfile, 3))
    {
        return -1;
    }

    char  buf[201];
    file_gets(buf, 200, ifsfile);
    char *bufptr = std::strchr(buf, ';');
    if (bufptr != nullptr)   // use ';' as comment to eol
    {
        *bufptr = 0;
    }

    strlwr(buf);
    bufptr = &buf[0];
    int rowsize = NUM_IFS_PARAMS;
    while (*bufptr)
    {
        if (std::strncmp(bufptr, "(3d)", 4) == 0)
        {
            g_ifs_type = true;
            rowsize = NUM_IFS_3D_PARAMS;
        }
        ++bufptr;
    }

    int ret = 0;
    int i = ret;
    bufptr = get_ifs_token(buf, ifsfile);
    while (bufptr != nullptr)
    {
        float value = 0.0f;
        if (std::sscanf(bufptr, " %f ", &value) != 1)
        {
            break;
        }
        g_ifs_definition.push_back(value);
        if (++i >= NUM_IFS_FUNCTIONS*rowsize)
        {
            stopmsg(STOPMSG_NONE, "IFS definition has too many lines");
            ret = -1;
            break;
        }
        bufptr = get_next_ifs_token(buf, ifsfile);
        if (bufptr == nullptr)
        {
            ret = -1;
            break;
        }
        if (*bufptr == '}')
        {
            break;
        }
    }

    if ((i % rowsize) != 0 || (bufptr && *bufptr != '}'))
    {
        stopmsg(STOPMSG_NONE, "invalid IFS definition");
        ret = -1;
    }
    if (i == 0 && ret == 0)
    {
        stopmsg(STOPMSG_NONE, "Empty IFS definition");
        ret = -1;
    }
    std::fclose(ifsfile);

    if (ret == 0)
    {
        g_num_affine_transforms = i/rowsize;
    }
    return ret;
}

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, int itemtype)
{
    std::FILE *infile = nullptr;
    bool found = false;
    char parsearchname[ITEM_NAME_LEN + 6];
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char fullpath[FILE_MAX_PATH];
    char defaultextension[5];


    splitpath(filename, drive, dir, fname, ext);
    make_fname_ext(fullpath, fname, ext);
    if (stricmp(filename, g_command_file.c_str()))
    {
        infile = std::fopen(filename, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }

        if (!found && g_check_cur_dir)
        {
            make_path(fullpath, "", DOTSLASH, fname, ext);
            infile = std::fopen(fullpath, "rb");
            if (infile != nullptr)
            {
                if (scan_entries(infile, nullptr, itemname) == -1)
                {
                    std::strcpy(filename, fullpath);
                    found = true;
                }
                else
                {
                    std::fclose(infile);
                    infile = nullptr;
                }
            }
        }
    }

    switch (itemtype)
    {
    case 1:
        std::strcpy(parsearchname, "frm:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".frm");
        splitpath(g_search_for.frm, drive, dir, nullptr, nullptr);
        break;
    case 2:
        std::strcpy(parsearchname, "lsys:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".l");
        splitpath(g_search_for.lsys, drive, dir, nullptr, nullptr);
        break;
    case 3:
        std::strcpy(parsearchname, "ifs:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".ifs");
        splitpath(g_search_for.ifs, drive, dir, nullptr, nullptr);
        break;
    default:
        std::strcpy(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".par");
        splitpath(g_search_for.par, drive, dir, nullptr, nullptr);
        break;
    }

    if (!found)
    {
        infile = std::fopen(g_command_file.c_str(), "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, parsearchname) == -1)
            {
                std::strcpy(filename, g_command_file.c_str());
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        make_path(fullpath, drive, dir, fname, ext);
        infile = std::fopen(fullpath, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                std::strcpy(filename, fullpath);
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        // search for file
        int out;
        make_path(fullpath, drive, dir, "*", defaultextension);
        out = fr_findfirst(fullpath);
        while (out == 0)
        {
            char msg[200];
            std::sprintf(msg, "Searching %13s for %s      ", DTA.filename.c_str(), itemname);
            showtempmsg(msg);
            if (!(DTA.attribute & SUBDIR)
                && DTA.filename != "."
                && DTA.filename != "..")
            {
                splitpath(DTA.filename, nullptr, nullptr, fname, ext);
                make_path(fullpath, drive, dir, fname, ext);
                infile = std::fopen(fullpath, "rb");
                if (infile != nullptr)
                {
                    if (scan_entries(infile, nullptr, itemname) == -1)
                    {
                        std::strcpy(filename, fullpath);
                        found = true;
                        break;
                    }
                    else
                    {
                        std::fclose(infile);
                        infile = nullptr;
                    }
                }
            }
            out = fr_findnext();
        }
        cleartempmsg();
    }

    if (!found && g_organize_formulas_search && itemtype == 1)
    {
        splitpath(g_organize_formulas_dir, drive, dir, nullptr, nullptr);
        fname[0] = '_';
        fname[1] = (char) 0;
        if (std::isalpha(itemname[0]))
        {
            if (strnicmp(itemname, "carr", 4))
            {
                fname[1] = itemname[0];
                fname[2] = (char) 0;
            }
            else if (std::isdigit(itemname[4]))
            {
                std::strcat(fname, "rc");
                fname[3] = itemname[4];
                fname[4] = (char) 0;
            }
            else
            {
                std::strcat(fname, "rc");
            }
        }
        else if (std::isdigit(itemname[0]))
        {
            std::strcat(fname, "num");
        }
        else
        {
            std::strcat(fname, "chr");
        }
        make_path(fullpath, drive, dir, fname, defaultextension);
        infile = std::fopen(fullpath, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                std::strcpy(filename, fullpath);
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        std::sprintf(fullpath, "'%s' file entry item not found", itemname);
        stopmsg(STOPMSG_NONE, fullpath);
        return true;
    }
    // found file
    if (fileptr != nullptr)
    {
        *fileptr = infile;
    }
    else if (infile != nullptr)
    {
        std::fclose(infile);
    }
    return false;
}

bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, int itemtype)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    bool const result = find_file_item(buf, itemname, fileptr, itemtype);
    filename = buf;
    return result;
}

int file_gets(char *buf, int maxlen, std::FILE *infile)
{
    int len, c;
    // similar to 'fgets', but file may be in either text or binary mode
    // returns -1 at eof, length of string otherwise
    if (std::feof(infile))
    {
        return -1;
    }
    len = 0;
    while (len < maxlen)
    {
        c = getc(infile);
        if (c == EOF || c == '\032')
        {
            if (len)
            {
                break;
            }
            return -1;
        }
        if (c == '\n')
        {
            break;             // linefeed is end of line
        }
        if (c != '\r')
        {
            buf[len++] = (char)c;    // ignore c/r
        }
    }
    buf[len] = 0;
    return len;
}

void roundfloatd(double *x) // make double converted from float look ok
{
    char buf[30];
    std::sprintf(buf, "%-10.7g", *x);
    *x = atof(buf);
}

void fix_inversion(double *x) // make double converted from string look ok
{
    char buf[30];
    std::sprintf(buf, "%-1.15lg", *x);
    *x = atof(buf);
}
