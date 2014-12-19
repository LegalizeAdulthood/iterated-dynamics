/*
        Resident odds and ends that don't fit anywhere else.
*/
#include <algorithm>

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if defined(XFRACT)
#include <malloc.h>
#include <unistd.h>
#else
#include <io.h>
#endif

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

// routines in this module

static  void trigdetails(char *);
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
    length = (int) strlen(str);
    padding = 3; // space between col1 and decimal.
    // find decimal point
    for (decpt = 0; decpt < length; decpt++)
        if (str[decpt] == '.')
            break;
    if (decpt >= length)
        decpt = 0;
    if (decpt < padding)
        padding -= decpt;
    else
        padding = 0;
    col1 += padding;
    decpt += col1+1; // column just past where decimal is
    while (length > 0)
    {
        if (col2-col1 < length)
        {
            if ((*row - startrow + 1) >= maxrow)
                done = true;
            else
                done = false;
            save1 = str[col2-col1+1];
            save2 = str[col2-col1+2];
            if (done)
                str[col2-col1+1]   = '+';
            else
                str[col2-col1+1]   = '\\';
            str[col2-col1+2] = 0;
            driver_put_string(*row,col1,color,str);
            if (done)
                break;
            str[col2-col1+1] = save1;
            str[col2-col1+2] = save2;
            str += col2-col1;
            (*row)++;
        } else
            driver_put_string(*row,col1,color,str);
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
    if (xx3rd == xxmin && yy3rd == yymin)
    {   // no rotation or skewing, but stretching is allowed
        double Width = xxmax - xxmin;
        Height = yymax - yymin;
        *Xctr = (xxmin + xxmax)/2.0;
        *Yctr = (yymin + yymax)/2.0;
        *Magnification  = 2.0/Height;
        *Xmagfactor =  Height / (DEFAULTASPECT * Width);
        *Rotation = 0.0;
        *Skew = 0.0;
    }
    else
    {
        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        double tmpx1 = xxmax - xxmin;
        double tmpy1 = yymax - yymin;
        double c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        tmpx1 = xxmax - xx3rd;
        tmpy1 = yymin - yy3rd;
        double a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        double a = sqrt(a2);
        *Rotation = -rad_to_deg(atan2(tmpy1, tmpx1));   // negative for image rotation

        double tmpx2 = xxmin - xx3rd;
        double tmpy2 = yymax - yy3rd;
        double b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        double b = sqrt(b2);

        double tmpa = acos((a2+b2-c2)/(2*a*b)); // save tmpa for later use
        *Skew = 90.0 - rad_to_deg(tmpa);

        *Xctr = (xxmin + xxmax)*0.5;
        *Yctr = (yymin + yymax)*0.5;

        Height = b * sin(tmpa);

        *Magnification  = 2.0/Height; // 1/(h/2)
        *Xmagfactor = Height / (DEFAULTASPECT * a);

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0 && debugflag != 4010)
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
            showcornersdbl("cvtcentermag problem");
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
        Xmagfactor = 1.0;

    h = (double)(1/Magnification);
    w = h / (DEFAULTASPECT * Xmagfactor);

    if (Rotation == 0.0 && Skew == 0.0)
    {   // simple, faster case
        xxmin = Xctr - w;
        xx3rd = xxmin;
        xxmax = Xctr + w;
        yymin = Yctr - h;
        yy3rd = yymin;
        yymax = Yctr + h;
        return;
    }

    // in unrotated, untranslated coordinate system
    tanskew = tan(deg_to_rad(Skew));
    xxmin = -w + h*tanskew;
    xxmax =  w - h*tanskew;
    xx3rd = -w - h*tanskew;
    yymax = h;
    yymin = -h;
    yy3rd = yymin;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    sinrot = sin(Rotation);
    cosrot = cos(Rotation);

    // top left
    x = xxmin * cosrot + yymax *  sinrot;
    y = -xxmin * sinrot + yymax *  cosrot;
    xxmin = x + Xctr;
    yymax = y + Yctr;

    // bottom right
    x = xxmax * cosrot + yymin *  sinrot;
    y = -xxmax * sinrot + yymin *  cosrot;
    xxmax = x + Xctr;
    yymin = y + Yctr;

    // bottom left
    x = xx3rd * cosrot + yy3rd *  sinrot;
    y = -xx3rd * sinrot + yy3rd *  cosrot;
    xx3rd = x + Xctr;
    yy3rd = y + Yctr;

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
    if (!cmp_bf(bfx3rd, bfxmin) && !cmp_bf(bfy3rd, bfymin))
    {   // no rotation or skewing, but stretching is allowed
        bfWidth  = alloc_stack(bflength+2);
        bf_t bfHeight = alloc_stack(bflength+2);
        // Width  = xxmax - xxmin;
        sub_bf(bfWidth, bfxmax, bfxmin);
        LDBL Width = bftofloat(bfWidth);
        // Height = yymax - yymin;
        sub_bf(bfHeight, bfymax, bfymin);
        Height = bftofloat(bfHeight);
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, bfxmin, bfxmax);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, bfymin, bfymax);
        half_a_bf(Yctr);
        *Magnification  = 2/Height;
        *Xmagfactor = (double)(Height / (DEFAULTASPECT * Width));
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
        sub_bf(bftmpx, bfxmax, bfxmin);
        LDBL tmpx1 = bftofloat(bftmpx);
        // tmpy = yymax - yymin;
        sub_bf(bftmpy, bfymax, bfymin);
        LDBL tmpy1 = bftofloat(bftmpy);
        LDBL c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        // tmpx = xxmax - xx3rd;
        sub_bf(bftmpx, bfxmax, bfx3rd);
        tmpx1 = bftofloat(bftmpx);

        // tmpy = yymin - yy3rd;
        sub_bf(bftmpy, bfymin, bfy3rd);
        tmpy1 = bftofloat(bftmpy);
        LDBL a2 = tmpx1*tmpx1 + tmpy1*tmpy1;
        LDBL a = sqrtl(a2);

        // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
        // atan2() only depends on the ratio, this puts it in double's range
        int signx = sign(tmpx1);
        LDBL tmpy = 0.0;
        if (signx)
            tmpy = tmpy1/tmpx1 * signx;    // tmpy = tmpy / |tmpx|
        *Rotation = (double)(-rad_to_deg(atan2((double)tmpy, signx)));   // negative for image rotation

        // tmpx = xxmin - xx3rd;
        sub_bf(bftmpx, bfxmin, bfx3rd);
        LDBL tmpx2 = bftofloat(bftmpx);
        // tmpy = yymax - yy3rd;
        sub_bf(bftmpy, bfymax, bfy3rd);
        LDBL tmpy2 = bftofloat(bftmpy);
        LDBL b2 = tmpx2*tmpx2 + tmpy2*tmpy2;
        LDBL b = sqrtl(b2);

        double tmpa = acos((double)((a2+b2-c2)/(2*a*b))); // save tmpa for later use
        *Skew = 90 - rad_to_deg(tmpa);

        // these are the only two variables that must use big precision
        // *Xctr = (xxmin + xxmax)/2;
        add_bf(Xctr, bfxmin, bfxmax);
        half_a_bf(Xctr);
        // *Yctr = (yymin + yymax)/2;
        add_bf(Yctr, bfymin, bfymax);
        half_a_bf(Yctr);

        Height = b * sin(tmpa);
        *Magnification  = 2/Height; // 1/(h/2)
        *Xmagfactor = (double)(Height / (DEFAULTASPECT * a));

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0 && debugflag != 4010)
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
        Xmagfactor = 1.0;

    h = 1/Magnification;
    floattobf(bfh, h);
    w = h / (DEFAULTASPECT * Xmagfactor);
    floattobf(bfw, w);

    if (Rotation == 0.0 && Skew == 0.0)
    {   // simple, faster case
        // xx3rd = xxmin = Xctr - w;
        sub_bf(bfxmin, Xctr, bfw);
        copy_bf(bfx3rd, bfxmin);
        // xxmax = Xctr + w;
        add_bf(bfxmax, Xctr, bfw);
        // yy3rd = yymin = Yctr - h;
        sub_bf(bfymin, Yctr, bfh);
        copy_bf(bfy3rd, bfymin);
        // yymax = Yctr + h;
        add_bf(bfymax, Yctr, bfh);
        restore_stack(saved);
        return;
    }

    bftmp = alloc_stack(bflength+2);
    // in unrotated, untranslated coordinate system
    tanskew = tan(deg_to_rad(Skew));
    xmin = -w + h*tanskew;
    xmax =  w - h*tanskew;
    x3rd = -w - h*tanskew;
    ymax = h;
    ymin = -h;
    y3rd = ymin;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    sinrot = sin(Rotation);
    cosrot = cos(Rotation);

    // top left
    x =  xmin * cosrot + ymax *  sinrot;
    y = -xmin * sinrot + ymax *  cosrot;
    // xxmin = x + Xctr;
    floattobf(bftmp, x);
    add_bf(bfxmin, bftmp, Xctr);
    // yymax = y + Yctr;
    floattobf(bftmp, y);
    add_bf(bfymax, bftmp, Yctr);

    // bottom right
    x =  xmax * cosrot + ymin *  sinrot;
    y = -xmax * sinrot + ymin *  cosrot;
    // xxmax = x + Xctr;
    floattobf(bftmp, x);
    add_bf(bfxmax, bftmp, Xctr);
    // yymin = y + Yctr;
    floattobf(bftmp, y);
    add_bf(bfymin, bftmp, Yctr);

    // bottom left
    x =  x3rd * cosrot + y3rd *  sinrot;
    y = -x3rd * sinrot + y3rd *  cosrot;
    // xx3rd = x + Xctr;
    floattobf(bftmp, x);
    add_bf(bfx3rd, bftmp, Xctr);
    // yy3rd = y + Yctr;
    floattobf(bftmp, y);
    add_bf(bfy3rd, bftmp, Yctr);

    restore_stack(saved);
    return;
}

void updatesavename(char *filename) // go to the next file name
{
    char *save, *hold;
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];

    splitpath(filename ,drive,dir,fname,ext);

    hold = fname + strlen(fname) - 1; // start at the end
    while (hold >= fname && (*hold == ' ' || isdigit(*hold))) // skip backwards
        hold--;
    hold++;                      // recover first digit
    while (*hold == '0')         // skip leading zeros
        hold++;
    save = hold;
    while (*save) {              // check for all nines
        if (*save != '9')
            break;
        save++;
    }
    if (!*save)                  // if the whole thing is nines then back
        save = hold - 1;          // up one place. Note that this will eat
    // your last letter if you go to far.
    else
        save = hold;
    sprintf(save,"%ld",atol(hold)+1); // increment the number
    makepath(filename,drive,dir,fname,ext);
}

int check_writefile(char *name, const char *ext)
{
    // after v16 release, change encoder.c to also use this routine
    char openfile[FILE_MAX_DIR];
    char opentype[20];
    char *period;
nextname:
    strcpy(openfile,name);
    strcpy(opentype,ext);
    period = has_ext(openfile);
    if (period != nullptr)
    {
        strcpy(opentype,period);
        *period = 0;
    }
    strcat(openfile,opentype);
    if (access(openfile,0) != 0) // file doesn't exist
    {
        strcpy(name,openfile);
        return 0;
    }
    // file already exists
    if (!fract_overwrite)
    {
        updatesavename(name);
        goto nextname;
    }
    return 1;
}

BYTE trigndx[] = {SIN,SQR,SINH,COSH};
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

void showtrig(char *buf) // return display form of active trig functions
{
    char tmpbuf[30];
    *buf = 0; // null string if none
    trigdetails(tmpbuf);
    if (tmpbuf[0])
        sprintf(buf," function=%s",tmpbuf);
}

static void trigdetails(char *buf)
{
    int numfn;
    char tmpbuf[20];
    if (fractype == JULIBROT || fractype == JULIBROTFP)
        numfn = (fractalspecific[neworbittype].flags >> 6) & 7;
    else
        numfn = (curfractalspecific->flags >> 6) & 7;
    if (curfractalspecific == &fractalspecific[FORMULA] ||
            curfractalspecific == &fractalspecific[FFORMULA])
        numfn = maxfn;
    *buf = 0; // null string if none
    if (numfn > 0) {
        strcpy(buf,trigfn[trigndx[0]].name);
        int i = 0;
        while (++i < numfn) {
            sprintf(tmpbuf,"/%s",trigfn[trigndx[i]].name);
            strcat(buf,tmpbuf);
        }
    }
}

// set array of trig function indices according to "function=" command
int set_trig_array(int k, const char *name)
{
    char trigname[10];
    char *slash;
    strncpy(trigname,name,6);
    trigname[6] = 0; // safety first

    slash = strchr(trigname,'/');
    if (slash != nullptr)
        *slash = 0;

    strlwr(trigname);

    for (int i = 0; i < numtrigfn; i++)
    {
        if (strcmp(trigname,trigfn[i].name) == 0)
        {
            trigndx[k] = (BYTE)i;
            set_trig_pointers(k);
            break;
        }
    }
    return (0);
}
void set_trig_pointers(int which)
{
    // set trig variable functions to avoid array lookup time
    switch (which)
    {
    case 0:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig0 = trigfn[trigndx[0]].lfunct;
        mtrig0 = trigfn[trigndx[0]].mfunct;
#endif
        dtrig0 = trigfn[trigndx[0]].dfunct;
        break;
    case 1:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig1 = trigfn[trigndx[1]].lfunct;
        mtrig1 = trigfn[trigndx[1]].mfunct;
#endif
        dtrig1 = trigfn[trigndx[1]].dfunct;
        break;
    case 2:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig2 = trigfn[trigndx[2]].lfunct;
        mtrig2 = trigfn[trigndx[2]].mfunct;
#endif
        dtrig2 = trigfn[trigndx[2]].dfunct;
        break;
    case 3:
#if !defined(XFRACT) && !defined(_WIN32)
        ltrig3 = trigfn[trigndx[3]].lfunct;
        mtrig3 = trigfn[trigndx[3]].mfunct;
#endif
        dtrig3 = trigfn[trigndx[3]].dfunct;
        break;
    default: // do 'em all
        for (int i = 0; i < 4; i++)
            set_trig_pointers(i);
        break;
    }
}

#ifdef XFRACT
static char spressanykey[] = {"Press any key to continue, F6 for area, F7 for next page"};
#else
static char spressanykey[] = {"Press any key to continue, F6 for area, CTRL-TAB for next page"};
#endif

void get_calculation_time(char *msg, long ctime)
{
    if (ctime >= 0)
    {
        sprintf(msg,"%3ld:%02ld:%02ld.%02ld", ctime/360000L,
                (ctime%360000L)/6000, (ctime%6000)/100, ctime%100);
    }
    else
        strcpy(msg, "A long time! (> 24.855 days)");
}

static void show_str_var(const char *name, const char *var, int *row, char *msg)
{
    if (var == nullptr)
        return;
    if (*var != 0)
    {
        sprintf(msg, "%s=%s", name, var);
        driver_put_string((*row)++, 2, C_GENERAL_HI, msg);
    }
}

static void
write_row(int row, const char *format, ...)
{
    char text[78] = { 0 };
    va_list args;

    va_start(args, format);
    _vsnprintf(text, NUM_OF(text), format, args);
    va_end(args);

    driver_put_string(row, 2, C_GENERAL_HI, text);
}

extern long maxstack;
extern long startstack;

bool tab_display_2(char *msg)
{
    extern long maxptr;
    int row, key = 0;

    helptitle();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80); // init rest to background

    row = 1;
    putstringcenter(row++, 0, 80, C_PROMPT_HI, "Top Secret Developer's Screen");

    write_row(++row, "Version %d patch %d", g_release, g_patch_level);
    write_row(++row, "%ld of %ld bignum memory used", maxptr, maxstack);
    write_row(++row, "   %ld used for bignum globals", startstack);
    write_row(++row, "   %ld stack used == %ld variables of length %d",
              maxptr-startstack, (long)((maxptr-startstack)/(rbflength+2)), rbflength+2);
    if (bf_math)
    {
        write_row(++row, "intlength %-d bflength %-d ", intlength, bflength);
    }
    row++;
    show_str_var("tempdir",     tempdir,      &row, msg);
    show_str_var("workdir",     workdir,      &row, msg);
    show_str_var("filename",    readname,     &row, msg);
    show_str_var("formulafile", FormFileName, &row, msg);
    show_str_var("savename",    savename,     &row, msg);
    show_str_var("parmfile",    CommandFile,  &row, msg);
    show_str_var("ifsfile",     IFSFileName,  &row, msg);
    show_str_var("autokeyname", autoname,     &row, msg);
    show_str_var("lightname",   light_name,   &row, msg);
    show_str_var("map",         MAP_name,     &row, msg);
    write_row(row++, "Sizeof fractalspecific array %d",
              num_fractal_types*(int)sizeof(fractalspecificstuff));
    write_row(row, "calc_status %d pixel [%d, %d]", calc_status, col, row);
    ++row;
    if (fractype == FORMULA || fractype == FFORMULA)
    {
        write_row(row++, "Max_Ops (posp) %u Max_Args (vsp) %u",
                  posp, vsp);
        write_row(row++, "   Store ptr %d Loadptr %d Max_Ops var %u Max_Args var %u LastInitOp %d",
                  StoPtr, LodPtr, Max_Ops, Max_Args, LastInitOp);
    }
    else if (rhombus_stack[0])
    {
        write_row(row++, "SOI Recursion %d stack free %d %d %d %d %d %d %d %d %d %d",
                  max_rhombus_depth+1,
                  rhombus_stack[0], rhombus_stack[1], rhombus_stack[2],
                  rhombus_stack[3], rhombus_stack[4], rhombus_stack[5],
                  rhombus_stack[6], rhombus_stack[7], rhombus_stack[8],
                  rhombus_stack[9]);
    }

    /*
        write_row(row++, "xdots %d ydots %d sxdots %d sydots %d", xdots, ydots, sxdots, sydots);
    */
    write_row(row++, "%dx%d dm=%d %s (%s)", xdots, ydots, dotmode,
              g_driver->name, g_driver->description);
    write_row(row++, "xxstart %d xxstop %d yystart %d yystop %d %s uses_ismand %d",
              xxstart, xxstop, yystart, yystop,
#if !defined(XFRACT) && !defined(_WIN32)
              curfractalspecific->orbitcalc == fFormula ? "fast parser" :
#endif
              curfractalspecific->orbitcalc ==  Formula ? "slow parser" :
              curfractalspecific->orbitcalc ==  BadFormula ? "bad formula" :
              "", uses_ismand ? 1 : 0);
    /*
        write_row(row++, "ixstart %d ixstop %d iystart %d iystop %d bitshift %d",
            ixstart, ixstop, iystart, iystop, bitshift);
    */
    write_row(row++, "minstackavail %d llimit2 %ld use_grid %d",
              minstackavail, llimit2, use_grid ? 1 : 0);
    putstringcenter(24, 0, 80, C_GENERAL_LO, "Press Esc to continue, Backspace for first screen");
    *msg = 0;

    // display keycodes while waiting for ESC, BACKSPACE or TAB
    while ((key != FIK_ESC) && (key != FIK_BACKSPACE) && (key != FIK_TAB))
    {
        driver_put_string(row, 2, C_GENERAL_HI, msg);
        key = getakeynohelp();
        sprintf(msg, "%d (0x%04x)      ", key, key);
    }
    return (key != FIK_ESC);
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
    const char *msgptr;
    int key;
    int saved = 0;
    int k;
    int hasformparam = 0;

    if (calc_status < CALCSTAT_PARAMS_CHANGED)        // no active fractal image
    {
        return 0;                // (no TAB on the credits screen)
    }
    if (calc_status == CALCSTAT_IN_PROGRESS)        // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    {
        calctime += (clock_ticks() - timer_start) / (CLOCKS_PER_SEC/100);
    }
    driver_stack_screen();
    if (bf_math)
    {
        saved = save_stack();
        bfXctr = alloc_stack(bflength+2);
        bfYctr = alloc_stack(bflength+2);
    }
    if (fractype == FORMULA || fractype == FFORMULA)
    {
        for (int i = 0; i < MAXPARAMS; i += 2)
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
    if (display3d > 0)
    {
        driver_put_string(s_row, 16, C_GENERAL_HI, "3D Transform");
    }
    else
    {
        driver_put_string(s_row, 16, C_GENERAL_HI,
                          curfractalspecific->name[0] == '*' ?
                          &curfractalspecific->name[1] : curfractalspecific->name);
        int i = 0;
        if (fractype == FORMULA || fractype == FFORMULA)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, FormName);
            i = (int) strlen(FormName)+1;
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) strlen(FormFileName) >= 29)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, FormFileName);
        }
        trigdetails(msg);
        driver_put_string(s_row+1, 16+i, C_GENERAL_HI, msg);
        if (fractype == LSYSTEM)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, LName);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) strlen(LFileName) >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, LFileName);
        }
        if (fractype == IFS || fractype == IFS3D)
        {
            driver_put_string(s_row+1, 3, C_GENERAL_MED, "Item name:");
            driver_put_string(s_row+1, 16, C_GENERAL_HI, IFSName);
            driver_put_string(s_row+2, 3, C_GENERAL_MED, "Item file:");
            if ((int) strlen(IFSFileName) >= 28)
            {
                addrow = 1;
            }
            driver_put_string(s_row+2+addrow, 16, C_GENERAL_HI, IFSFileName);
        }
    }

    switch (calc_status)
    {
    case 0:
        msgptr = "Parms chgd since generated";
        break;
    case 1:
        msgptr = "Still being generated";
        break;
    case 2:
        msgptr = "Interrupted, resumable";
        break;
    case 3:
        msgptr = "Interrupted, non-resumable";
        break;
    case 4:
        msgptr = "Image completed";
        break;
    default:
        msgptr = "";
    }
    driver_put_string(s_row, 45, C_GENERAL_HI, msgptr);
    if (initbatch && calc_status != CALCSTAT_PARAMS_CHANGED)
    {
        driver_put_string(-1, -1, C_GENERAL_HI, " (Batch mode)");
    }

    if (helpmode == HELPCYCLING)
    {
        driver_put_string(s_row+1, 45, C_GENERAL_HI, "You are in color-cycling mode");
    }
    ++s_row;
    // if (bf_math == 0)
    ++s_row;

    int j = 0;
    if (display3d > 0)
    {
        if (usr_floatflag)
        {
            j = 1;
        }
    }
    else if (floatflag)
    {
        j = usr_floatflag ? 1 : 2;
    }

    if (bf_math == 0)
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
        sprintf(msg, "(%-d decimals)", decimals /*getprecbf(CURRENTREZ)*/);
        driver_put_string(s_row, 45, C_GENERAL_HI, "Arbitrary precision ");
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }
    s_row += 1;

    if (calc_status == CALCSTAT_IN_PROGRESS || calc_status == CALCSTAT_RESUMABLE)
    {
        if (curfractalspecific->flags&NORESUME)
        {
            driver_put_string(s_row++, 2, C_GENERAL_HI,
                              "Note: can't resume this type after interrupts other than <tab> and <F1>");
        }
    }
    s_row += addrow;
    driver_put_string(s_row, 2, C_GENERAL_MED, "Savename: ");
    driver_put_string(s_row, -1, C_GENERAL_HI, savename);

    ++s_row;

    if (got_status >= 0 && (calc_status == CALCSTAT_IN_PROGRESS || calc_status == CALCSTAT_RESUMABLE))
    {
        switch (got_status)
        {
        case 0:
            sprintf(msg, "%d Pass Mode", totpasses);
            driver_put_string(s_row, 2, C_GENERAL_HI, msg);
            if (usr_stdcalcmode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case 1:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Solid Guessing");
            if (usr_stdcalcmode == '3')
            {
                driver_put_string(s_row, -1, C_GENERAL_HI, " (threepass)");
            }
            break;
        case 2:
            driver_put_string(s_row, 2, C_GENERAL_HI, "Boundary Tracing");
            break;
        case 3:
            sprintf(msg, "Processing row %d (of %d) of input image", currow, fileydots);
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
        if (got_status == 5)
        {
            sprintf(msg, "%2.2f%% done, counter at %lu of %lu (%u bits)",
                    (100.0 * dif_counter)/dif_limit,
                    dif_counter, dif_limit, bits);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            ++s_row;
        }
        else if (got_status != 3)
        {
            sprintf(msg, "Working on block (y, x) [%d, %d]...[%d, %d], ",
                    yystart, xxstart, yystop, xxstop);
            driver_put_string(s_row, 2, C_GENERAL_MED, msg);
            if (got_status == 2 || got_status == 4)  // btm or tesseral
            {
                driver_put_string(-1, -1, C_GENERAL_MED, "at ");
                sprintf(msg, "[%d, %d]", currow, curcol);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            else
            {
                if (totpasses > 1)
                {
                    driver_put_string(-1, -1, C_GENERAL_MED, "pass ");
                    sprintf(msg, "%d", curpass);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, " of ");
                    sprintf(msg, "%d", totpasses);
                    driver_put_string(-1, -1, C_GENERAL_HI, msg);
                    driver_put_string(-1, -1, C_GENERAL_MED, ", ");
                }
                driver_put_string(-1, -1, C_GENERAL_MED, "at row ");
                sprintf(msg, "%d", currow);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                driver_put_string(-1, -1, C_GENERAL_MED, " col ");
                sprintf(msg, "%d", col);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
            }
            ++s_row;
        }
    }
    driver_put_string(s_row, 2, C_GENERAL_MED, "Calculation time:");
    get_calculation_time(msg, calctime);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    if ((got_status == 5) && (calc_status == CALCSTAT_IN_PROGRESS))  // estimate total time
    {
        driver_put_string(-1, -1, C_GENERAL_MED, " estimated total time: ");
        get_calculation_time(msg, (long)(calctime*((dif_limit*1.0)/dif_counter)));
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if ((curfractalspecific->flags&INFCALC) && (coloriter != 0))
    {
        driver_put_string(s_row, -1, C_GENERAL_MED, " 1000's of points:");
        sprintf(msg, " %ld of %ld", coloriter-2, maxct);
        driver_put_string(s_row, -1, C_GENERAL_HI, msg);
    }

    ++s_row;
    if (bf_math == 0)
    {
        ++s_row;
    }
    _snprintf(msg, NUM_OF(msg), "Driver: %s, %s", g_driver->name, g_driver->description);
    driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    if (g_video_entry.xdots && bf_math == 0)
    {
        sprintf(msg, "Video: %dx%dx%d %s %s",
                g_video_entry.xdots, g_video_entry.ydots, g_video_entry.colors,
                g_video_entry.name, g_video_entry.comment);
        driver_put_string(s_row++, 2, C_GENERAL_MED, msg);
    }
    if (!(curfractalspecific->flags&NOZOOM))
    {
        adjust_corner(); // make bottom left exact if very near exact
        if (bf_math)
        {
            int truncaterow;
            int dec = std::min(320, decimals);
            adjust_cornerbf(); // make bottom left exact if very near exact
            cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            // find alignment information
            msg[0] = 0;
            bool truncate = false;
            if (dec < decimals)
            {
                truncate = true;
            }
            truncaterow = row;
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
            sprintf(msg, "%10.8Le", Magnification);
            driver_put_string(-1, 11, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
        else // bf != 1
        {
            driver_put_string(s_row, 2, C_GENERAL_MED, "Corners:                X                     Y");
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Top-l");
            sprintf(msg, "%20.16f  %20.16f", xxmin, yymax);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-r");
            sprintf(msg, "%20.16f  %20.16f", xxmax, yymin);
            driver_put_string(-1, 17, C_GENERAL_HI, msg);

            if (xxmin != xx3rd || yymin != yy3rd)
            {
                driver_put_string(++s_row, 3, C_GENERAL_MED, "Bot-l");
                sprintf(msg, "%20.16f  %20.16f", xx3rd, yy3rd);
                driver_put_string(-1, 17, C_GENERAL_HI, msg);
            }
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Ctr");
            sprintf(msg, "%20.16f %20.16f  ", Xctr, Yctr);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Mag");
            sprintf(msg, " %10.8Le", Magnification);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(++s_row, 2, C_GENERAL_MED, "X-Mag-Factor");
            sprintf(msg, "%11.4f   ", Xmagfactor);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Rotation");
            sprintf(msg, "%9.3f   ", Rotation);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
            driver_put_string(-1, -1, C_GENERAL_MED, "Skew");
            sprintf(msg, "%9.3f", Skew);
            driver_put_string(-1, -1, C_GENERAL_HI, msg);
        }
    }

    if (typehasparm(fractype, 0, msg) || hasformparam)
    {
        for (int i = 0; i < MAXPARAMS; i++)
        {
            char p[50];
            if (typehasparm(fractype, i, p))
            {
                int col;
                if (k%4 == 0)
                {
                    s_row++;
                    col = 9;
                }
                else
                    col = -1;
                if (k == 0) // only true with first displayed parameter
                    driver_put_string(++s_row, 2, C_GENERAL_MED, "Params ");
                sprintf(msg, "%3d: ", i+1);
                driver_put_string(s_row, col, C_GENERAL_MED, msg);
                if (*p == '+')
                    sprintf(msg, "%-12d", (int)param[i]);
                else if (*p == '#')
                    sprintf(msg, "%-12lu", (U32)param[i]);
                else
                    sprintf(msg, "%-12.9f", param[i]);
                driver_put_string(-1, -1, C_GENERAL_HI, msg);
                k++;
            }
        }
    }
    driver_put_string(s_row += 2, 2, C_GENERAL_MED, "Current (Max) Iteration: ");
    sprintf(msg, "%ld (%ld)", coloriter, maxit);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);
    driver_put_string(-1, -1, C_GENERAL_MED, "     Effective bailout: ");
    sprintf(msg, "%f", rqlim);
    driver_put_string(-1, -1, C_GENERAL_HI, msg);

    if (fractype == PLASMA || fractype == ANT || fractype == CELLULAR)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Current 'rseed': ");
        sprintf(msg, "%d", rseed);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
    }

    if (invert)
    {
        driver_put_string(++s_row, 2, C_GENERAL_MED, "Inversion radius: ");
        sprintf(msg, "%12.9f", f_radius);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  xcenter: ");
        sprintf(msg, "%12.9f", f_xcenter);
        driver_put_string(-1, -1, C_GENERAL_HI, msg);
        driver_put_string(-1, -1, C_GENERAL_MED, "  ycenter: ");
        sprintf(msg, "%12.9f", f_ycenter);
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
    timer_start = clock_ticks(); // tab display was "time out"
    if (bf_math)
    {
        restore_stack(saved);
    }
    return 0;
}

static void area()
{
    const char *msg;
    char buf[160];
    long cnt = 0;
    if (inside < 0) {
        stopmsg(STOPMSG_NONE, "Need solid inside to compute area");
        return;
    }
    for (int y = 0; y < ydots; y++) {
        for (int x = 0; x < xdots; x++) {
            if (getcolor(x,y) == inside) {
                cnt++;
            }
        }
    }
    if (inside > 0 && outside < 0 && maxit > inside) {
        msg = "Warning: inside may not be unique\n";
    } else {
        msg = "";
    }
    sprintf(buf,"%s%ld inside pixels of %ld%s%f",
            msg,cnt,(long)xdots*(long)ydots,".  Total area ",
            cnt/((float)xdots*(float)ydots)*(xxmax-xxmin)*(yymax-yymin));
    stopmsg(STOPMSG_NO_BUZZER, buf);
}

int endswithslash(const char *fl)
{
    int len;
    len = (int) strlen(fl);
    if (len)
        if (fl[--len] == SLASHC)
            return (1);
    return (0);
}

// ---------------------------------------------------------------------
static char seps[] = {"' ','\t',\n',\r'"};
char *get_ifs_token(char *buf,FILE *ifsfile)
{
    char *bufptr;
    while (1)
    {
        if (file_gets(buf,200,ifsfile) < 0)
            return (nullptr);
        else
        {
            bufptr = strchr(buf,';');
            if (bufptr != nullptr) // use ';' as comment to eol
                *bufptr = 0;
            bufptr = strtok(buf, seps);
            if (bufptr != nullptr)
                return (bufptr);
        }
    }
}

char insufficient_ifs_mem[] = {"Insufficient memory for IFS"};
int numaffine;
int ifsload()                   // read in IFS parameters
{
    FILE *ifsfile;
    char buf[201];
    char *bufptr;
    int ret,rowsize;

    if (!ifs_defn.empty())
    {                           // release prior parms
        ifs_defn.clear();
    }

    ifs_type = false;
    rowsize = IFSPARM;
    if (find_file_item(IFSFileName,IFSName,&ifsfile, 3))
        return (-1);

    file_gets(buf,200,ifsfile);
    bufptr = strchr(buf,';');
    if (bufptr != nullptr) // use ';' as comment to eol
        *bufptr = 0;

    strlwr(buf);
    bufptr = &buf[0];
    while (*bufptr) {
        if (strncmp(bufptr,"(3d)",4) == 0) {
            ifs_type = true;
            rowsize = IFS3DPARM;
        }
        ++bufptr;
    }

    ret = 0;
    int i = ret;
    bufptr = get_ifs_token(buf,ifsfile);
    while (bufptr != nullptr)
    {
        float value = 0.0f;
        if (sscanf(bufptr," %f ", &value) != 1)
            break;
        ifs_defn.push_back(value);
        if (++i >= NUMIFS*rowsize)
        {
            stopmsg(STOPMSG_NONE, "IFS definition has too many lines");
            ret = -1;
            break;
        }
        bufptr = strtok(nullptr, seps);
        if (bufptr == nullptr)
        {
            bufptr = get_ifs_token(buf,ifsfile);
            if (bufptr == nullptr)
            {
                ret = -1;
                break;
            }
        }
        if (ret == -1)
            break;
        if (*bufptr == '}')
            break;
    }

    if ((i % rowsize) != 0 || (bufptr && *bufptr != '}')) {
        stopmsg(STOPMSG_NONE, "invalid IFS definition");
        ret = -1;
    }
    if (i == 0 && ret == 0) {
        stopmsg(STOPMSG_NONE, "Empty IFS definition");
        ret = -1;
    }
    fclose(ifsfile);

    if (ret == 0) {
        numaffine = i/rowsize;
    }
    return (ret);
}

bool find_file_item(char *filename,char *itemname,FILE **fileptr, int itemtype)
{
    FILE *infile = nullptr;
    bool found = false;
    char parsearchname[ITEMNAMELEN + 6];
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char fullpath[FILE_MAX_PATH];
    char defaultextension[5];


    splitpath(filename,drive,dir,fname,ext);
    makepath(fullpath,"","",fname,ext);
    if (stricmp(filename, CommandFile)) {
        infile = fopen(filename, "rb");
        if (infile != nullptr) {
            if (scan_entries(infile, nullptr, itemname) == -1) {
                found = true;
            }
            else {
                fclose(infile);
                infile = nullptr;
            }
        }

        if (!found && checkcurdir) {
            makepath(fullpath,"",DOTSLASH,fname,ext);
            infile = fopen(fullpath, "rb");
            if (infile != nullptr) {
                if (scan_entries(infile, nullptr, itemname) == -1) {
                    strcpy(filename, fullpath);
                    found = true;
                }
                else {
                    fclose(infile);
                    infile = nullptr;
                }
            }
        }
    }

    switch (itemtype) {
    case 1:
        strcpy(parsearchname, "frm:");
        strcat(parsearchname, itemname);
        parsearchname[ITEMNAMELEN + 5] = (char) 0; //safety
        strcpy(defaultextension, ".frm");
        splitpath(searchfor.frm,drive,dir,nullptr,nullptr);
        break;
    case 2:
        strcpy(parsearchname, "lsys:");
        strcat(parsearchname, itemname);
        parsearchname[ITEMNAMELEN + 5] = (char) 0; //safety
        strcpy(defaultextension, ".l");
        splitpath(searchfor.lsys,drive,dir,nullptr,nullptr);
        break;
    case 3:
        strcpy(parsearchname, "ifs:");
        strcat(parsearchname, itemname);
        parsearchname[ITEMNAMELEN + 5] = (char) 0; //safety
        strcpy(defaultextension, ".ifs");
        splitpath(searchfor.ifs,drive,dir,nullptr,nullptr);
        break;
    default:
        strcpy(parsearchname, itemname);
        parsearchname[ITEMNAMELEN + 5] = (char) 0; //safety
        strcpy(defaultextension, ".par");
        splitpath(searchfor.par,drive,dir,nullptr,nullptr);
        break;
    }

    if (!found) {
        infile = fopen(CommandFile, "rb");
        if (infile != nullptr) {
            if (scan_entries(infile, nullptr, parsearchname) == -1) {
                strcpy(filename, CommandFile);
                found = true;
            }
            else {
                fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found) {
        makepath(fullpath,drive,dir,fname,ext);
        infile = fopen(fullpath, "rb");
        if (infile != nullptr) {
            if (scan_entries(infile, nullptr, itemname) == -1) {
                strcpy(filename, fullpath);
                found = true;
            }
            else {
                fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found) {  // search for file
        int out;
        makepath(fullpath,drive,dir,"*",defaultextension);
        out = fr_findfirst(fullpath);
        while (out == 0) {
            char msg[200];
            DTA.filename[FILE_MAX_FNAME+FILE_MAX_EXT-2] = 0;
            sprintf(msg,"Searching %13s for %s      ",DTA.filename,itemname);
            showtempmsg(msg);
            if (!(DTA.attribute & SUBDIR) &&
                    strcmp(DTA.filename,".") &&
                    strcmp(DTA.filename,"..")) {
                splitpath(DTA.filename,nullptr,nullptr,fname,ext);
                makepath(fullpath,drive,dir,fname,ext);
                infile = fopen(fullpath, "rb");
                if (infile != nullptr) {
                    if (scan_entries(infile, nullptr, itemname) == -1) {
                        strcpy(filename, fullpath);
                        found = true;
                        break;
                    }
                    else {
                        fclose(infile);
                        infile = nullptr;
                    }
                }
            }
            out = fr_findnext();
        }
        cleartempmsg();
    }

    if (!found && orgfrmsearch && itemtype == 1) {
        splitpath(orgfrmdir,drive,dir,nullptr,nullptr);
        fname[0] = '_';
        fname[1] = (char) 0;
        if (isalpha(itemname[0])) {
            if (strnicmp(itemname, "carr", 4)) {
                fname[1] = itemname[0];
                fname[2] = (char) 0;
            }
            else if (isdigit(itemname[4])) {
                strcat(fname, "rc");
                fname[3] = itemname[4];
                fname[4] = (char) 0;
            }
            else {
                strcat(fname, "rc");
            }
        }
        else if (isdigit(itemname[0])) {
            strcat(fname, "num");
        }
        else {
            strcat(fname, "chr");
        }
        makepath(fullpath,drive,dir,fname,defaultextension);
        infile = fopen(fullpath, "rb");
        if (infile != nullptr) {
            if (scan_entries(infile, nullptr, itemname) == -1) {
                strcpy(filename, fullpath);
                found = true;
            }
            else {
                fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found) {
        sprintf(fullpath,"'%s' file entry item not found",itemname);
        stopmsg(STOPMSG_NONE, fullpath);
        return true;
    }
    // found file
    if (fileptr != nullptr)
        *fileptr = infile;
    else if (infile != nullptr)
        fclose(infile);
    return false;
}


int file_gets(char *buf,int maxlen,FILE *infile)
{
    int len,c;
    // similar to 'fgets', but file may be in either text or binary mode
    // returns -1 at eof, length of string otherwise
    if (feof(infile))
        return -1;
    len = 0;
    while (len < maxlen) {
        c = getc(infile);
        if (c == EOF || c == '\032') {
            if (len)
                break;
            return -1;
        }
        if (c == '\n')
            break;             // linefeed is end of line
        if (c != '\r')
            buf[len++] = (char)c;    // ignore c/r
    }
    buf[len] = 0;
    return len;
}

void roundfloatd(double *x) // make double converted from float look ok
{
    char buf[30];
    sprintf(buf,"%-10.7g",*x);
    *x = atof(buf);
}

void fix_inversion(double *x) // make double converted from string look ok
{
    char buf[30];
    sprintf(buf,"%-1.15lg",*x);
    *x = atof(buf);
}
