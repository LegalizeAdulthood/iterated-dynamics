/*  Printer.c
 *      Simple screen printing functions for FRACTINT
 *      By Matt Saucier CIS: [72371,3101]      7/2/89
 *      "True-to-the-spirit" of FRACTINT, this code makes few checks that you
 *      have specified a valid resolution for the printer (just in case yours
 *      has more dots/line than the Standard HP and IBM/EPSON,
 *      (eg, Wide Carriage, etc.))
 *
 *      PostScript support by Scott Taylor [72401,410] / (DGWM18A)   10/8/90
 *      For PostScript, use 'printer=PostScript/resolution' where resolution
 *      is ANY NUMBER between 10 and 600. Common values: 300,150,100,75.
 *      Default resolution for PostScript is 150 pixels/inch.
 *      At 200 DPI, a fractal that is 640x480 prints as a 3.2"x2.4" picture.
 *      PostScript printer names:
 *
 *      PostScript/PS                   = Portrait printing
 *      PostScriptH/PostScriptL/PSH/PSL = Landscape printing
 *
 *      This code supports printers attached to a LPTx (1-3) parallel port.
 *      It also now supports serial printers AFTER THEY ARE CONFIGURED AND
 *      WORKING WITH THE DOS MODE COMMAND, eg. MODE COM1:9600,n,8,1 (for HP)
 *      (NOW you can also configure the serial port with the comport= command)
 *      Printing calls are made directly to the BIOS for DOS can't handle fast
 *      transfer of data to the HP.  (Or maybe visa-versa, HP can't handle the
 *      slow transfer of data from DOS)
 *
 *      I just added direct port access for COM1 and COM2 **ONLY**. This method
 *      does a little more testing than BIOS, and may work (especially on
 *      serial printer sharing devices) where the old method doesn't. I noticed
 *      maybe a 5% speed increase at 9600 baud. These are selected in the
 *      printer=.../.../31 for COM1 or 32 for COM2.
 *
 *      I also added direct parallel port access for LPT1 and LPT2 **ONLY**.
 *      This toggles the "INIT" line of the parallel port to reset the printer
 *      for each print session. It will also WAIT for a error / out of paper /
 *      not selected condition instead of quitting with an error.
 *
 *      Supported Printers:     Tested Ok:
 *       HP LaserJet
 *          LJ+,LJII             MDS
 *       Toshiba PageLaser       MDS (Set FRACTINT to use HP)
 *       IBM Graphics            MDS
 *       EPSON
 *          Models?              Untested.
 *       IBM LaserPrinter
 *          with PostScript      SWT
 *       HP Plotter              SWT
 *
 *      Future support to include OKI 20 (color) printer, and just about
 *      any printer you request.
 *
 *      Future modifications to include a more flexible, standard interface
 *      with the surrounding program, for easier portability to other
 *      programs.
 *
 * PostScript Styles:
 *  0  Dot
 *  1  Dot*            [Smoother]
 *  2  Inverted Dot
 *  3  Ring
 *  4  Inverted Ring
 *  5  Triangle        [45-45-90]
 *  6  Triangle*       [30-75-75]
 *  7  Grid
 *  8  Diamond
 *  9  Line
 * 10  Microwaves
 * 11  Ellipse
 * 12  RoundBox
 * 13  Custom
 * 14  Star
 * 15  Random
 * 16  Line*           [Not much different]
 *
 *  *  Alternate style
 *

 */


#ifndef XFRACT
#include <bios.h>
#include <io.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#ifndef XFRACT
#include <conio.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <string.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

/* macros for near-space-saving purposes */
/* CAE 9211 changed these for BC++ */

#define PRINTER_PRINTF1(X) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp);\
}

#define PRINTER_PRINTF2(X,Y) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y));\
}
#define PRINTER_PRINTF3(X,Y,Z) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y),(Z));\
}
#define PRINTER_PRINTF4(X,Y,Z,W) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y),(Z),(W));\
}
#define PRINTER_PRINTF5(X,Y,Z,W,V) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y),(Z),(W),(V));\
}
#define PRINTER_PRINTF6(X,Y,Z,W,V,U) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y),(Z),(W),(V),(U));\
}
#define PRINTER_PRINTF7(X,Y,Z,W,V,U,T) {\
   static FCODE tmp[] = X;\
   Printer_printf(tmp,(Y),(Z),(W),(V),(U),(T));\
}

/********      PROTOTYPES     ********/

#ifndef USE_VARARGS
static void Printer_printf(char far *fmt,...);
#else
static void Printer_printf();
#endif
static int  _fastcall printer(int c);
static void _fastcall print_title(int,int,char *);
static void printer_reset(void);
static void rleprolog(int x,int y);
static void _fastcall graphics_init(int,int,char *);

/********       GLOBALS       ********/

int Printer_Resolution,        /* 75,100,150,300 for HP;                   */
                               /* 60,120,240 for IBM;                      */
                               /* 90 or 180 for the PaintJet;              */
                               /* 10-600 for PS                            */
                               /* 1-20 for Plotter                         */
    LPTNumber,                 /* ==1,2,3 LPTx; or 11,12,13,14 for COM1-4  */
                               /* 21,22 for direct port access for LPT1-2  */
                               /* 31,32 for direct port access for COM1-2  */
    Printer_Type,                      /* ==1 HP,
                                          ==2 IBM/EPSON,
                                          ==3 Epson color,
                                          ==4 HP PaintJet,
                                          ==5,6 PostScript,
                                          ==7 HP Plotter                   */
    Printer_Titleblock,       /* Print info about the fractal?             */
    Printer_Compress,         /* PostScript only - rle encode output       */
    Printer_ColorXlat,        /* PostScript only - invert colors           */
    Printer_SetScreen,        /* PostScript only - reprogram halftone ?    */
    Printer_SFrequency,       /* PostScript only - Halftone Frequency K    */
    Printer_SAngle,           /* PostScript only - Halftone angle     K    */
    Printer_SStyle,           /* PostScript only - Halftone style     K    */
    Printer_RFrequency,       /* PostScript only - Halftone Frequency R    */
    Printer_RAngle,           /* PostScript only - Halftone angle     R    */
    Printer_RStyle,           /* PostScript only - Halftone style     R    */
    Printer_GFrequency,       /* PostScript only - Halftone Frequency G    */
    Printer_GAngle,           /* PostScript only - Halftone angle     G    */
    Printer_GStyle,           /* PostScript only - Halftone style     G    */
    Printer_BFrequency,       /* PostScript only - Halftone Frequency B    */
    Printer_BAngle,           /* PostScript only - Halftone angle     B    */
    Printer_BStyle,           /* PostScript only - Halftone style     B    */
    Print_To_File,            /* Print to file toggle                      */
    EPSFileType,              /* EPSFileType -
                                               1 = well-behaved,
                                               2 = much less behaved,
                                               3 = not well behaved        */
    Printer_CRLF,             /* (0) CRLF (1) CR (2) LF                    */
    ColorPS;                  /* (0) B&W  (1) Color                        */
int pj_width;
double ci,ck;

static int repeat, item, count, repeatitem, itembuf[128], rlebitsperitem,
    rlebitshift, bitspersample, rleitem, repeatcount, itemsperline, items,
    /* bitsperitem, */ bitshift2;
/*
 *  The tables were copied from Lee Crocker's PGIF program, with
 *  the 8 undithered colors moved to the first 8 table slots.
 *
 *  This file contains various lookup tables used by PJGIF.  Patterns contains
 *  unsigned values representing each of the 330 HP PaintJet colors.  Each color
 *  at 90 DPI is composed of four dots in 8 colors.  Each hex digit of these
 *  unsigned values represents one of the four dots.  Although the PaintJet will
 *  produce these patterns automatically in 90 DPI mode, it is much faster to do
 *  it in software with the PaintJet in 8-color 180 DPI mode.
 *
 *  920501 Hans Wolfgang Schulze converted from printera.asm for xfractint.
 *         (hans@garfield.metal2.polymtl.ca)
 */

static UIFCODE pj_patterns [] = {
      0x7777,0x0000,0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,
             0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
      0x0110,0x0120,0x0130,0x0140,0x0150,0x0160,0x0170,0x0220,
      0x0230,0x0240,0x0250,0x0260,0x0270,0x0330,0x0340,0x0350,
      0x0360,0x0370,0x0440,0x0450,0x0460,0x0470,0x0550,0x0560,
      0x0570,0x0660,0x0670,0x0770,0x0111,0x0112,0x0113,0x0114,
      0x0115,0x0116,0x0117,0x2012,0x0123,0x0124,0x0125,0x0126,
      0x0127,0x3013,0x0134,0x0135,0x0136,0x0137,0x4014,0x0145,
      0x0146,0x0147,0x5015,0x0156,0x0157,0x6016,0x0167,0x7017,
      0x0222,0x0223,0x0224,0x0225,0x0226,0x0227,0x3023,0x0234,
      0x0235,0x0236,0x0237,0x4024,0x0245,0x0246,0x0247,0x5025,
      0x0256,0x0257,0x6026,0x0267,0x7027,0x0333,0x0334,0x0335,
      0x0336,0x0337,0x4034,0x0345,0x0346,0x0347,0x5035,0x0356,
      0x0357,0x6036,0x0367,0x7037,0x0444,0x0445,0x0446,0x0447,
      0x5045,0x0456,0x0457,0x6046,0x0467,0x7047,0x0555,0x0556,
      0x0557,0x6056,0x0567,0x7057,0x0666,0x0667,0x7067,0x0777,
             0x1112,0x1113,0x1114,0x1115,0x1116,0x1117,0x2112,
      0x1123,0x2114,0x2115,0x2116,0x2117,0x3113,0x3114,0x3115,
      0x3116,0x3117,0x4114,0x4115,0x4116,0x4117,0x5115,0x5116,
      0x5117,0x6116,0x6117,0x7117,0x1222,0x1223,0x1224,0x1225,
      0x1226,0x1227,0x3123,0x1234,0x1235,0x1236,0x1237,0x4124,
      0x1245,0x1246,0x1247,0x5125,0x1256,0x1257,0x6126,0x1267,
      0x7127,0x1333,0x1334,0x1335,0x1336,0x1337,0x4134,0x1345,
      0x1346,0x1347,0x5135,0x1356,0x1357,0x6136,0x1367,0x7137,
      0x1444,0x1445,0x1446,0x1447,0x5145,0x1456,0x1457,0x6146,
      0x1467,0x7147,0x1555,0x1556,0x1557,0x6156,0x1567,0x7157,
      0x1666,0x1667,0x7167,0x1777,       0x2223,0x2224,0x2225,
      0x2226,0x2227,0x3223,0x3224,0x3225,0x3226,0x3227,0x4224,
      0x4225,0x4226,0x4227,0x5225,0x5226,0x5227,0x6226,0x6227,
      0x7227,0x2333,0x2334,0x2335,0x2336,0x2337,0x4234,0x2345,
      0x2346,0x2347,0x5235,0x2356,0x2357,0x6236,0x2367,0x7237,
      0x2444,0x2445,0x2446,0x2447,0x5245,0x2456,0x2457,0x6246,
      0x2467,0x7247,0x2555,0x2556,0x2557,0x6256,0x2567,0x7257,
      0x2666,0x2667,0x7267,0x2777,       0x3334,0x3335,0x3336,
      0x3337,0x4334,0x4335,0x4336,0x4337,0x5335,0x5336,0x5337,
      0x6336,0x6337,0x7337,0x3444,0x3445,0x3446,0x3447,0x5345,
      0x3456,0x3457,0x6346,0x3467,0x7347,0x3555,0x3556,0x3557,
      0x6356,0x3567,0x7357,0x3666,0x3667,0x7367,0x3777,
      0x4445,0x4446,0x4447,0x5445,0x5446,0x5447,0x6446,0x6447,
      0x7447,0x4555,0x4556,0x4557,0x6456,0x4567,0x7457,0x4666,
      0x4667,0x7467,0x4777,       0x5556,0x5557,0x6556,0x6557,
      0x7557,0x5666,0x5667,0x7567,0x5777,       0x6667,0x7667,
      0x6777};

/*
 * The 3 tables below contain the red, green, and blue values (on a scale of
 *  0..255) of each of the 330 PaintJet colors.  These values are based on data
 *  generously provided by HP customer service.
 *       11 <- changed black's value from this, seemed wrong
 *         135 <- changed red's value from this
 *           11 <- changed blue's value from this
 */
#ifndef XFRACT
static BFCODE  pj_reds[] = {
        229,  2,145,  7,227,  9,136, 5,
             17, 10, 17, 10, 16, 10, 16, 29, 16, 32, 15, 30, 15, 31,  9,
         15, 10, 15,  9, 13, 37, 15, 32, 16, 36, 10, 15,  9, 13, 30, 15,
         31,  8, 13, 38, 62, 26, 68, 26, 63, 26, 68, 16, 35, 16, 33, 16,
         33, 77, 26, 69, 29, 77, 16, 31, 16, 31, 64, 27, 71, 16, 36, 81,
          9, 15, 10, 15,  8, 13, 37, 15, 31, 15, 33, 10, 15,  9, 13, 29,
         15, 28,  8, 12, 28, 98, 28, 79, 32, 94, 16, 34, 17, 35, 73, 30,
         82, 17, 43,101, 11, 15, 10, 13, 29, 15, 27,  9, 13, 25, 65, 27,
         71, 16, 35, 88,  7, 12, 39,110,     54,146, 53,136, 58,144, 29,
         57, 28, 53, 29, 56,159, 54,144, 61,160, 27, 51, 28, 52,135, 55,
        144, 30, 60,159, 14, 23, 15, 22, 14, 21, 64, 30, 58, 32, 64, 15,
         22, 15, 21, 54, 31, 56, 14, 22, 64,185, 59,160, 69,185, 29, 57,
         31, 60,145, 63,162, 33, 71,186, 15, 22, 16, 21, 50, 30, 52, 15,
         21, 54,134, 58,145, 30, 60,161, 15, 22, 69,187,     13,  9, 14,
          6, 11, 31, 14, 27, 12, 27, 10, 14,  9, 12, 24,  9, 23,  6,  9,
         22, 76, 23, 61, 25, 74, 15, 29, 14, 28, 55, 23, 62, 12, 30, 73,
         11, 15, 10, 12, 25, 14, 23,  8, 11, 20, 50, 22, 53, 13, 26, 61,
          5,  8, 21, 71,     71,189, 87,227, 30, 63, 32, 69,164, 76,190,
         37, 89,227, 15, 22, 14, 20, 54, 31, 57, 14, 21, 63,147, 67,163,
         33, 72,191, 13, 24, 94,228,     15, 10, 13, 26, 14, 23, 10, 13,
         20, 50, 23, 50, 15, 26, 52,  8, 11, 23, 65,     60,147, 32, 67,
        166, 14, 24, 77,194,      8, 32, 97};

/*
 *                   11 <- changed black's value from this, seemed wrong
 *                           65 <- changed green from this
 */

static BFCODE pj_greens[] = {
        224,  2, 20, 72,211, 10, 11, 55,
             12, 15, 19, 11, 11, 14, 17, 14, 18, 22, 12, 13, 16, 19, 24,
         29, 16, 17, 23, 27, 41, 17, 22, 29, 39, 11, 10, 14, 14, 11, 14,
         17, 21, 25, 40, 16, 21, 28, 14, 16, 19, 25, 28, 37, 18, 20, 26,
         33, 48, 20, 26, 33, 46, 13, 12, 16, 18, 14, 18, 22, 24, 30, 42,
         40, 49, 25, 27, 39, 50, 69, 27, 33, 48, 66, 17, 17, 24, 27, 19,
         28, 35, 38, 48, 68,100, 32, 46, 65, 98, 18, 22, 29, 36, 27, 39,
         54, 49, 71,105, 11, 10, 14, 12, 10, 14, 13, 20, 20, 25, 11, 15,
         18, 22, 29, 49, 36, 46, 69,111,     23, 31, 16, 19, 22, 28, 30,
         37, 20, 22, 28, 34, 54, 22, 29, 36, 53, 14, 15, 17, 19, 17, 19,
         26, 25, 32, 46, 43, 50, 27, 28, 41, 49, 68, 29, 37, 51, 68, 19,
         19, 25, 28, 22, 30, 36, 40, 47, 66,104, 35, 51, 68,105, 20, 24,
         31, 37, 30, 38, 56, 50, 69,103, 13, 12, 15, 14, 13, 15, 16, 21,
         21, 26, 14, 16, 22, 23, 28, 44, 35, 42, 62,102,     78, 40, 44,
         65, 78, 98, 43, 53, 76, 99, 26, 27, 36, 40, 29, 43, 50, 63, 75,
         99,136, 49, 69, 98,142, 28, 32, 42, 51, 39, 52, 73, 77,103,145,
         17, 17, 21, 21, 18, 22, 24, 34, 37, 43, 19, 23, 30, 40, 48, 69,
         62, 76,101,147,     72,113,145,218, 33, 42, 52, 71, 61, 77,116,
        105,148,221, 18, 17, 21, 23, 21, 26, 30, 37, 43, 64, 30, 35, 48,
         50, 69,115, 77, 99,149,224,     10, 13, 11, 10, 12, 11, 17, 16,
         15,  9, 11, 12, 17, 17, 22, 26, 27, 36, 61,     14, 18, 21, 26,
         48, 34, 41, 68,115,     69, 99,149};

/*                    15 <- changed black's value from this, seemed wrong
 *                           56 <- changed green from this
 *                                          163 <- changed cyan from this
 */
static BFCODE pj_blues[] = {
        216,  2, 34, 48, 33, 73, 64,168,
             18, 19, 18, 20, 19, 22, 21, 22, 24, 22, 26, 24, 27, 24, 27,
         24, 29, 27, 31, 29, 22, 27, 25, 30, 28, 31, 29, 33, 33, 28, 32,
         32, 41, 40, 46, 28, 32, 28, 34, 30, 36, 31, 35, 32, 38, 34, 41,
         35, 27, 35, 31, 39, 34, 40, 37, 44, 40, 34, 42, 37, 49, 47, 45,
         40, 36, 43, 40, 47, 43, 33, 40, 36, 45, 41, 44, 41, 49, 46, 40,
         49, 45, 58, 56, 58, 30, 38, 34, 44, 40, 42, 39, 49, 46, 38, 49,
         46, 59, 62, 67, 49, 46, 55, 52, 44, 55, 52, 64, 64, 66, 43, 55,
         53, 66, 70, 78, 87, 91,101,115,     39, 34, 42, 37, 43, 36, 45,
         38, 47, 42, 49, 43, 34, 41, 36, 44, 38, 49, 45, 52, 46, 40, 47,
         42, 56, 51, 45, 49, 45, 52, 48, 56, 50, 40, 47, 44, 52, 47, 54,
         51, 59, 55, 47, 58, 50, 66, 60, 56, 34, 44, 38, 48, 42, 52, 47,
         56, 50, 42, 51, 46, 64, 59, 57, 60, 56, 64, 61, 52, 61, 57, 72,
         67, 64, 48, 58, 53, 69, 65, 65, 87, 83, 87, 94,     53, 59, 55,
         64, 60, 46, 53, 49, 59, 54, 60, 56, 65, 62, 53, 62, 58, 76, 71,
         68, 41, 50, 45, 56, 51, 58, 53, 63, 59, 49, 60, 56, 74, 71, 71,
         66, 63, 73, 70, 60, 69, 67, 84, 81, 79, 55, 67, 64, 84, 81, 83,
        104,104,106,116,     32, 40, 53, 48, 54, 50, 61, 57, 46, 59, 56,
         76, 75, 80, 64, 59, 70, 67, 57, 69, 65, 83, 81, 85, 54, 68, 66,
         86, 88, 96,110,114,125,137,     71, 81, 78, 68, 77, 76, 93, 92,
         90, 65, 77, 75, 92, 93, 96,117,119,126,138,     78, 79, 98,102,
        110,124,131,143,157,    173,185,200};
#endif

static void putitem(void);
static void rleputxelval(int);
static void rleflush(void);
static void rleputrest(void);

static int LPTn;                   /* printer number we're gonna use */

static FILE *PRFILE;

#define TONES 17                   /* Number of PostScript halftone styles */

#if 1
static FCODE ht00[] = {"D mul exch D mul add 1 exch sub"};
static FCODE ht01[] = {"abs exch abs 2 copy add 1 gt {1 sub D mul exch 1 sub D mul add 1 sub} {D mul exch D mul add 1 exch sub} ifelse"};
static FCODE ht02[] = {"D mul exch D mul add 1 sub"};
static FCODE ht03[] = {"D mul exch D mul add 0.6 exch sub abs -0.5 mul"};
static FCODE ht04[] = {"D mul exch D mul add 0.6 exch sub abs 0.5 mul"};
static FCODE ht05[] = {"add 2 div"};
static FCODE ht06[] = {"2 exch sub exch abs 2 mul sub 3 div"};
static FCODE ht07[] = {"2 copy abs exch abs gt {exch} if pop 2 mul 1 exch sub 3.5 div"};
static FCODE ht08[] = {"abs exch abs add 1 exch sub"};
static FCODE ht09[] = {"pop"};
static FCODE ht10[] = {"/wy exch def 180 mul cos 2 div wy D D D mul mul sub mul wy add 180 mul cos"};
static FCODE ht11[] = {"D 5 mul 8 div mul exch D mul exch add sqrt 1 exch sub"};
static FCODE ht12[] = {"D mul D mul exch D mul D mul add 1 exch sub"};
static FCODE ht13[] = {"D mul exch D mul add sqrt 1 exch sub"};
static FCODE ht14[] = {"abs exch abs 2 copy gt {exch} if 1 sub D 0 eq {0.01 add} if atan 360 div"};
static FCODE ht15[] = {"pop pop rand 1 add 10240 mod 5120 div 1 exch sub"};
static FCODE ht16[] = {"pop abs 2 mul 1 exch sub"};
#endif

static FCODE *HalfTone[TONES]=  {ht00,ht01,ht02,ht03,ht04,ht05,ht06,ht07,
    ht08,ht09,ht10,ht11,ht12,ht13,ht14,ht15,ht16};

#if 0
                         "D mul exch D mul add 1 exch sub",
                         "abs exch abs 2 copy add 1 gt {1 sub D mul exch 1 sub D mul add 1 sub} {D mul exch D mul add 1 exch sub} ifelse",
                         "D mul exch D mul add 1 sub",
                         "D mul exch D mul add 0.6 exch sub abs -0.5 mul",
                         "D mul exch D mul add 0.6 exch sub abs 0.5 mul",
                         "add 2 div",
                         "2 exch sub exch abs 2 mul sub 3 div",
                         "2 copy abs exch abs gt {exch} if pop 2 mul 1 exch sub 3.5 div",
                         "abs exch abs add 1 exch sub",
                         "pop",
                         "/wy exch def 180 mul cos 2 div wy D D D mul mul sub mul wy add 180 mul cos",
                         "D 5 mul 8 div mul exch D mul exch add sqrt 1 exch sub",
                         "D mul D mul exch D mul D mul add 1 exch sub",
                         "D mul exch D mul add sqrt 1 exch sub",
                         "abs exch abs 2 copy gt {exch} if 1 sub D 0 eq {0.01 add} if atan 360 div",
                         "pop pop rand 1 add 10240 mod 5120 div 1 exch sub",
                         "pop abs 2 mul 1 exch sub"
                        };
#endif

#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn -eff
#endif
#endif

static char EndOfLine[3];

/* workaround for the old illicit decflaration of dstack */

typedef int (*TRIPLE)[2][3][400];
#define triple   (*((TRIPLE)dstack))

void
Print_Screen (void)
{
    int y,j;
    char buff[192];             /* buffer for 192 sets of pixels  */
                                /* This is very large so that we can*/
                                /* get reasonable times printing  */
                                /* from modes like MAXPIXELSxMAXPIXELS disk-*/
                                /* video.  When this was 24, a MAXPIXELS*/
                                /* by MAXPIXELS pic took over 2 hours to*/
                                /* print.  It takes about 15 min now*/
    int BuffSiz;                /* how much of buff[] we'll use   */
    char far *es;               /* pointer to extraseg for buffer */
    int i,x,k,                  /* more indices                   */
        imax,                   /* maximum i value (ydots/8)      */
        res,                    /* resolution we're gonna' use    */
        high,                   /* if LPTn>10 COM == com port to use*/
        low,                    /* misc                           */
                                /************************************/
        ptrid;                  /* Printer Id code.               */
                                /* Currently, the following are   */
                                /* assigned:                      */
                                /*            1. HPLJ (all)       */
                                /*               Toshiba PageLaser*/
                                /*            2. IBM Graphics     */
                                /*            3. Color Printer    */
                                /*            4. HP PaintJet      */
                                /*            5. PostScript       */
                                /************************************/
    int pj_color_ptr[256];      /* Paintjet color translation */

                                /********   SETUP VARIABLES  ********/
    memset(buff,0,192);
    i = 0;

    EndOfLine[0]=(char)(((Printer_CRLF==1) || (Printer_CRLF==0)) ? 0x0D : 0x0A);
    EndOfLine[1]=(char)((Printer_CRLF==0) ? 0x0A : 0x00);
    EndOfLine[2]=0x00;

    if (Print_To_File>0)
      {
      while ((PRFILE = fopen(PrintName,"r")) != NULL) {
         j = fgetc(PRFILE);
         fclose(PRFILE);
         if (j == EOF) break;
         updatesavename((char *)PrintName);
         }
      if ((PRFILE = fopen(PrintName,"wb")) == NULL) Print_To_File = 0;
      }

#ifdef XFRACT
      driver_put_string(3,0,0,"Printing to:");
      driver_put_string(4,0,0,PrintName);
      driver_put_string(5,0,0,"               ");
#endif

    es=MK_FP(extraseg,0);

    LPTn=LPTNumber-1;
    if (((LPTn>2)&&(LPTn<10))||
        ((LPTn>13)&&(LPTn<20))||
        ((LPTn>21)&&(LPTn<30))||
        (LPTn<0)||(LPTn>31)) LPTn=0;   /* default of LPT1 (==0)          */
    ptrid=Printer_Type;
    if ((ptrid<1)||(ptrid>7)) ptrid=2; /* default of IBM/EPSON           */
    res=Printer_Resolution;
#ifndef XFRACT
    if ((LPTn==20)||(LPTn==21))
        {
        k = (inp((LPTn==20) ? 0x37A : 0x27A)) & 0xF7;
        outp((LPTn==20) ? 0x37A : 0x27A,k);
        k = k & 0xFB;
        outp((LPTn==20) ? 0x37A : 0x27A,k);
        k = k | 0x0C;
        outp((LPTn==20) ? 0x37A : 0x27A,k);
        }
    if ((LPTn==30)||(LPTn==31))
        {
        outp((LPTn==30) ? 0x3F9 : 0x2F9,0x00);
        outp((LPTn==30) ? 0x3FC : 0x2FC,0x00);
        outp((LPTn==30) ? 0x3FC : 0x2FC,0x03);
        }
#endif

    switch (ptrid) {

        case 1:
            if (res<75) res=75;
            if ( (res<= 75)&&(ydots> 600)) res=100;
            if ( (res<=100)&&(ydots> 800)) res=150;
            if (((res<=150)&&(ydots>1200))||(res>300)) res=300;
            break;

        case 2:
        case 3:
            if (res<60) res=60;
            if ((res<=60)&&(ydots>480)) res=120;
            if (((res<=120)&&(ydots>960))||(res>240)) res=240;
            break;

        case 4: /****** PaintJet  *****/
            {
#ifndef XFRACT
            /* Pieter Branderhorst:
               My apologies if the numbers and approach here seem to be
               picked out of a hat.  They were.  They happen to result in
               a tolerable mapping of screen colors to printer colors on
               my machine.  There are two sources of error in getting colors
               to come out right.
               1) Must match some dacbox values to the 330 PaintJet dithered
                  colors so that they look the same.  For this we use HP's
                  color values in printera.asm and modify by gamma separately
                  for each of red/green/blue.  This mapping is ok if the
                  preview shown on screen is a fairly close match to what
                  gets printed. The defaults are what work for me.
               2) Must find nearest color in HP palette to each color in
                  current image. For this we use Lee Crocker's least sum of
                  differences squared approach, modified to spread the
                  values using gamma 1.7.  This mods was arrived at by
                  trial and error, just because it improves the mapping.
               */
            long ldist;
            int r,g,b;
            double gamma_val,gammadiv;
            BYTE convert[256];
            BYTE scale[64];

            BYTE far *table_ptr = NULL;
            res = (res < 150) ? 90 : 180;   /* 90 or 180 dpi */
            if (Printer_SetScreen == 0) {
                Printer_SFrequency = 21;  /* default red gamma */
                Printer_SAngle     = 19;  /*       green gamma */
                Printer_SStyle     = 16;  /*        blue gamma */
            }
            /* Convert the values in printera.asm.  We might do this just   */
            /* once per run, but we'd need separate memory for that - can't */
            /* just convert table in-place cause it could be in an overlay, */
            /* might be paged out and then back in in original form.  Also, */
            /* user might change gammas with a .par file entry mid-run.     */
            for (j = 0; j < 3; ++j) {
                switch (j) {
                    case 0: table_ptr = pj_reds;
                            i = Printer_SFrequency;
                            break;
                    case 1: table_ptr = pj_greens;
                            i = Printer_SAngle;
                            break;
                    case 2: table_ptr = pj_blues;
                            i = Printer_SStyle;
                    }
                gamma_val = 10.0 / i;
                gammadiv = pow(255,gamma_val) / 255;
                for (i = 0; i < 256; ++i) { /* build gamma conversion table */
                    static FCODE msg[]={"Calculating color translation"};
                    if ((i & 15) == 15)
                        thinking(1,msg);
                    convert[i] = (BYTE)((pow((double)i,gamma_val) / gammadiv) + 0.5);
                    }
                for (i = 0; i < 330; ++i) {
                    k = convert[table_ptr[i]];
                    if (k > 252) k = 252;
                    triple[0][j][i] = (k + 2) >> 2;
                }
            }
            /* build comparison lookup table */
            gamma_val = 1.7;
            gammadiv = pow(63,gamma_val) / 63;
            for (i = 0; i < 64; ++i) {
               if ((j = (int)((pow((double)i,gamma_val) / gammadiv) * 4 + 0.5)) < i)
                  j = i;
               scale[i] = (char)j;
            }
            for (i = 0; i < 3; ++i) /* convert values via lookup */
                for (j = 0; j < 330; ++j)
                    triple[1][i][j] = scale[triple[0][i][j]];
            /* Following code and the later code which writes to Paintjet    */
            /* using pj_patterns was adapted from Lee Crocker's PGIF program */
            for (i = 0; i < colors; ++i) { /* find nearest match colors */
                r = scale[dacbox[i][0]];
                g = scale[dacbox[i][1]];
                b = scale[dacbox[i][2]];
                ldist = 9999999L;
                /* check variance vs each PaintJet color */
                /* if high-res 8 color mode, consider only 1st 8 colors */
                j = (res == 90) ? 330 : 8;
                while (--j >= 0) {
                    long dist;
                    dist  = (unsigned)(r-triple[1][0][j]) * (r-triple[1][0][j]);
                    dist += (unsigned)(g-triple[1][1][j]) * (g-triple[1][1][j]);
                    dist += (unsigned)(b-triple[1][2][j]) * (b-triple[1][2][j]);
                    if (dist < ldist) {
                        ldist = dist;
                        k = j;
                    }
                }
                pj_color_ptr[i] = k; /* remember best fit */
            }
            thinking(0,NULL);
        /*  if (debugflag == 900 || debugflag == 902) {
                color_test();
                return;
            }  */
            if (!driver_diskp()) { /* preview */
                static char far msg[] = {"Preview. Enter=go, Esc=cancel, k=keep"};
                memcpy(triple[1],dacbox,768);
                for (i = 0; i < colors; ++i)
                    for (j = 0; j < 3; ++j)
                        dacbox[i][j] = (BYTE)triple[0][j][pj_color_ptr[i]];
                spindac(0,1);
                texttempmsg(msg);
                i = getakeynohelp();
                if (i == 'K' || i == 'k') {
                    return;
                }
                memcpy(dacbox,triple[1],768);
                spindac(0,1);
                if (i == 0x1B) {
                    return;
                }
            }
            break;
#endif
            }

        case 5:
        case 6: /***** PostScript *****/
            if ( res < 10 && res != 0 ) res = 10; /* PostScript scales... */
            if ( res > 600 ) res = 600; /* it can handle any range! */
            if ((Printer_SStyle < 0) || (Printer_SStyle >= TONES))
                Printer_SStyle = 0;
            break;
    }

    /*****  Set up buffer size for immediate user gratification *****/
    /*****    AKA, if we don't have to, don't buffer the data   *****/
    BuffSiz=8;
    if (xdots>1024) BuffSiz=192;

    /*****   Initialize printer  *****/
    if (Print_To_File < 1) {
        printer_reset();
        /* wait a bit, some printers need time after reset */
        delay((ptrid == 4) ? 2000 : 500);
    }

    /******  INITIALIZE GRAPHICS MODES  ******/

    graphics_init(ptrid,res,EndOfLine);

    if (keypressed()) {         /* one last chance before we start...*/
        return;
        }

    memset(buff,0,192);

                                /*****  Get And Print Screen **** */
    switch (ptrid) {

        case 1:                        /* HP LaserJet (et al)            */
            imax=(ydots/8)-1;
            for (x=0;((x<xdots)&&(!keypressed()));x+=BuffSiz) {
                for (i=imax;((i>=0)&&(!keypressed()));i--) {
                    for (y=7;((y>=0)&&(!keypressed()));y--) {
                        for (j=0;j<BuffSiz;j++) {
                            if ((x+j)<xdots) {
                                buff[j]<<=1;
                                buff[j]=(char)(buff[j]+(char)(getcolor(x+j,i*8+y)&1));
                                }
                            }
                        }
                    for (j=0;j<BuffSiz;j++) {
                        *(es+j+BuffSiz*i)=buff[j];
                        buff[j]=0;
                        }
                    }
                for (j=0;((j<BuffSiz)&&(!keypressed()));j++) {
                    if ((x+j)<xdots) {
                        PRINTER_PRINTF2("\033*b%iW",imax+1);
                        for (i=imax;((i>=0)&&(!keypressed()));i--) {
                            printer(*(es+j+BuffSiz*i));
                            }
                        }
                    }
                }
            if (!keypressed()) PRINTER_PRINTF1("\033*rB\014");
            break;

        case 2:                        /* IBM Graphics/Epson             */
            for (x=0;((x<xdots)&&(!keypressed()));x+=8) {
                switch (res) {
                    case 60:  Printer_printf("\033K"); break;
                    case 120: Printer_printf("\033L"); break;
                    case 240: Printer_printf("\033Z"); break;
                    }
                high=ydots/256;
                low=ydots-(high*256);
                printer(low);
                printer(high);
                for (y=ydots-1;(y>=0);y--) {
                    buff[0]=0;
                    for (i=0;i<8;i++) {
                        buff[0]<<=1;
                        buff[0]=(char)(buff[0]+(char)(getcolor(x+i,y)&1));
                        }
                    printer(buff[0]);
                    }
                if (keypressed()) break;
                Printer_printf(EndOfLine);
                }
            if (!keypressed()) printer(12);
            break;

        case 3:                        /* IBM Graphics/Epson Color      */
            high=ydots/256;
            low=ydots%256;
            for (x=0;((x<xdots)&&(!keypressed()));x+=8)
                {
                for (k=0; k<8; k++)  /* colors */
                    {
                    Printer_printf("\033r%d",k); /* set printer color */
                    switch (res)
                        {
                        case 60:  Printer_printf("\033K"); break;
                        case 120: Printer_printf("\033L"); break;
                        case 240: Printer_printf("\033Z"); break;
                        }
                    printer(low);
                    printer(high);
                    for (y=ydots-1;y>=0;y--)
                        {
                        buff[0]=0;
                        for (i=0;i<8;i++)
                            {
                            buff[0]<<=1;
                            if ((getcolor(x+i,y)%8)==k)
                                buff[0]++;
                            }
                        printer(buff[0]);
                        }
                    if (Printer_CRLF<2) printer(13);
                    }
                if ((Printer_CRLF==0) || (Printer_CRLF==2)) printer(10);
                }
            printer(12);
            printer(12);
            printer_reset();
            break;

        case 4:                       /* HP PaintJet       */
            {
            unsigned int fetchrows,fetched;
            BYTE far *pixels = NULL, far *nextpixel = NULL;
            /* for reasonable speed when using disk video, try to fetch
               and store the info for 8 columns at a time instead of
               doing getcolor calls down each column in separate passes */
            fetchrows = 16;
            for(;;) {
                if ((pixels = (BYTE far *)farmemalloc((long)(fetchrows)*ydots)) != NULL)
                   break;
                if ((fetchrows >>= 1) == 0) {
                    static char far msg[]={"insufficient memory"};
                    stopmsg(0,msg);
                    break;
                }
            }
            if (!pixels) break;
            fetched = 0;
            for (x = 0; (x < xdots && !keypressed()); ++x) {
                if (fetched == 0) {
                    if ((fetched = xdots-x) > fetchrows)
                        fetched = fetchrows;
                    for (y = ydots-1; y >= 0; --y) {
                        if (debugflag == 602) /* flip image */
                            nextpixel = pixels + y;
                        else                  /* reverse order for unflipped */
                            nextpixel = pixels + ydots-1 - y;
                        for (i = 0; i < (int)fetched; ++i) {
                            *nextpixel = (BYTE)getcolor(x+i,y);
                            nextpixel += ydots;
                        }
                    }
                    nextpixel = pixels;
                }
                --fetched;
                if (res == 180) { /* high-res 8 color mode */
                    int offset;
                    BYTE bitmask;
                    offset = -1;
                    bitmask = 0;
                    for (y = ydots - 1; y >= 0; --y) {
                        BYTE color;
                        if ((bitmask >>= 1) == 0) {
                            ++offset;
                            triple[0][0][offset] = triple[0][1][offset]
                                                 = triple[0][2][offset] = 0;
                            bitmask = 0x80;
                        }
                        /* translate 01234567 to 70123456 */
                        color = (BYTE)(pj_color_ptr[*(nextpixel++)] - 1);
                        if ((color & 1)) triple[0][0][offset] += bitmask;
                        if ((color & 2)) triple[0][1][offset] += bitmask;
                        if ((color & 4)) triple[0][2][offset] += bitmask;
                    }
                }
                else { /* 90 dpi, build 2 lines, 2 dots per pixel */
                    int bitct,offset;
                    bitct = offset = 0;
                    for (y = ydots - 1; y >= 0; --y) {
                        unsigned int color;
                        color = pj_patterns[pj_color_ptr[*(nextpixel++)]];
                        for (i = 0; i < 3; ++i) {
                            BYTE *bufptr;
                            bufptr = (BYTE *)&triple[0][i][offset];
                            *bufptr <<= 2;
                            if ((color & 0x1000)) *bufptr += 2;
                            if ((color & 0x0100)) ++*bufptr;
                            bufptr = (BYTE *)&triple[1][i][offset];
                            *bufptr <<= 2;
                            if ((color & 0x0010)) *bufptr += 2;
                            if ((color & 0x0001)) ++*bufptr;
                            color >>= 1;
                        }
                        if (++bitct == 4) {
                            bitct = 0;
                            ++offset;
                        }
                    }
                }
                for (i = 0; i < ((res == 90) ? 2 : 1); ++i) {
                    for (j = 0; j < 3; ++j) {
                        BYTE *bufptr,*bufend;
                        Printer_printf((j < 2) ? "\033*b%dV" : "\033*b%dW",
                                       pj_width);
                        bufend = pj_width + (bufptr = (BYTE *)(triple[i][j]));
                        do {
                            while (printer(*bufptr)) { }
                        } while (++bufptr < bufend);
                    }
                }
            }
            PRINTER_PRINTF1("\033*r0B"); /* end raster graphics */
            if (!keypressed()) {
               if (debugflag != 600)
                  printer(12); /* form feed */
               else
                  Printer_printf("\n\n");
            }
            farmemfree(pixels);
            break;
            }

        case 5:
        case 6:         /***** PostScript Portrait & Landscape *****/
            {
            char convert[513];
            if (!ColorPS) {
              for (i=0; i<256; ++i)
                if (Printer_Compress) {
                    convert[i] = (char)((.3*255./63. * (double)dacbox[i][0])+
                                        (.59*255./63. * (double)dacbox[i][1])+
                                        (.11*255./63. * (double)dacbox[i][2]));
                } else
                {
                    sprintf(&convert[2*i], "%02X",
                              (int)((.3*255./63. * (double)dacbox[i][0])+
                                    (.59*255./63. * (double)dacbox[i][1])+
                                    (.11*255./63. * (double)dacbox[i][2])));
                }
            }
            i=0;
            j=0;
            for (y=0;((y<ydots)&&(!keypressed()));y++)
            {   unsigned char bit8 = 0;
                if (Printer_Compress) {
                    if (ColorPS) {
                        for (x=0;x<xdots;x++) {
                            k=getcolor(x,y);
                            rleputxelval((int)dacbox[k][0]<<2);
                        }
                        rleflush();
                        for (x=0;x<xdots;x++) {
                            k=getcolor(x,y);
                            rleputxelval((int)dacbox[k][1]<<2);
                        }
                        rleflush();
                        for (x=0;x<xdots;x++) {
                            k=getcolor(x,y);
                            rleputxelval((int)dacbox[k][2]<<2);
                        }
                        rleflush();
                     } else {
                        if (Printer_ColorXlat==-2 || Printer_ColorXlat==2) {
                           for (x=0;x<xdots;x++) {
                              k=getcolor(x,y);
                              k=getcolor(x,y) & 1;
                              if (x % 8 == 0) {
                                 if (x) rleputxelval((int)bit8);
                                 if (k) bit8 = 1; else bit8 = 0;
                              }
                              else
                                 bit8 = (unsigned char)((bit8 << 1) + ((k) ? 1 : 0));
                           }
                           if (xdots % 8) bit8 <<= (8 - (xdots % 8));
                           rleputxelval((int)bit8);
                           rleflush();
                        } else {
                           for (x=0;x<xdots;x++) {
                              k=getcolor(x,y);
                              rleputxelval((int)(unsigned char)convert[k]);
                           }
                           rleflush();
                        }
                     }
                } else
                {
                    for (x=0;x<xdots;x++)
                    {
                        k=getcolor(x,y);
                        if (ColorPS)
                          {
                          sprintf(&buff[i], "%02X%02X%02X", dacbox[k][0]<<2,
                                                            dacbox[k][1]<<2,
                                                            dacbox[k][2]<<2);
                          i+=6;
                          }
                        else
                          {
                          k*=2;
                          buff[i++]=convert[k];
                          buff[i++]=convert[k+1];
                          }
                        if (i>=64)
                        {
                            strcpy(&buff[i],"  ");
                            Printer_printf("%s%s",buff,EndOfLine);
                            i=0;
                            j++;
                            if (j>9)
                            {
                                j=0;
                                Printer_printf(EndOfLine);
                            }
                        }
                    }
                }
            }
            if (Printer_Compress) {
                rleputrest();
            } else {
                strcpy(&buff[i],"  ");
                Printer_printf("%s%s",buff,EndOfLine);
                i=0;
                j++;
                if (j>9)
                {
                    j=0;
                    Printer_printf(EndOfLine);
                }
            }
            if ( (EPSFileType > 0) && (EPSFileType <3) )
            {
                PRINTER_PRINTF4("%s%%%%Trailer%sEPSFsave restore%s",EndOfLine,
                        EndOfLine,EndOfLine);
            }
            else
            {
#ifndef XFRACT
                PRINTER_PRINTF4("%sshowpage%s%c",EndOfLine,EndOfLine,4);
#else
                PRINTER_PRINTF3("%sshowpage%s",EndOfLine,EndOfLine);
#endif
            }
            break;
            }

        case 7: /* HP Plotter */
            {
            double parm1=0,parm2=0;
            for (i=0;i<3;i++)
            {
              PRINTER_PRINTF4("%sSP %d;%s\0",EndOfLine,(i+1),EndOfLine);
              for (y=0;(y<ydots)&&(!keypressed());y++)
              {
                for (x=0;x<xdots;x++)
                {
                  j=dacbox[getcolor(x,y)][i];
                  if (j>0)
                  {
                    switch(Printer_SStyle)
                    {
                      case 0:
                        ci=0.004582144*(double)j;
                        ck= -.007936057*(double)j;
                        parm1 = (double)x+.5+ci+(((double)i-1.0)/3);
                        parm2 = (double)y+.5+ck;
                        break;
                      case 1:
                        ci= -.004582144*(double)j+(((double)i+1.0)/8.0);
                        ck= -.007936057*(double)j;
                        parm1 = (double)x+.5+ci;
                        parm2 = (double)y+.5+ck;
                        break;
                      case 2:
                        ci= -.0078125*(double)j+(((double)i+1.0)*.003906250);
                        ck= -.0078125*(double)j;
                        parm1 = (double)x+.5+ci;
                        parm2 = (double)y+.5+ck;
                        break;
                    }
                    PRINTER_PRINTF5("PA %f,%f;PD;PR %f,%f;PU;\0",
                        parm1,parm2, ci*((double)-2), ck*((double)-2));
                  }
                }
              }
            }
            PRINTER_PRINTF3("%s;SC;PA 0,0;SP0;%s\0",EndOfLine,EndOfLine);
            PRINTER_PRINTF2(";;SP 0;%s\0",EndOfLine);
            break;
            }
    }

    if (Print_To_File > 0) fclose(PRFILE);
#ifndef XFRACT
    if ((LPTn==30)||(LPTn==31))
        {
        for (x=0;x<2000;x++);
        outp((LPTn==30) ? 0x3FC : 0x2FC,0x00);
        outp((LPTn==30) ? 0x3F9 : 0x2F9,0x00);
        }
#else
    driver_put_string(5,0,0,"Printing done\n");
#endif
}


static void _fastcall graphics_init(int ptrid,int res,char *EndOfLine)
{
    int i,j;

    switch (ptrid) {

        case 1:
            print_title(ptrid,res,EndOfLine);
            PRINTER_PRINTF2("\033*t%iR\033*r0A",res);/* HP           */
            break;

        case 2:
        case 3:
            print_title(ptrid,res,EndOfLine);
            PRINTER_PRINTF1("\033\063\030");/* IBM                   */
            break;

        case 4: /****** PaintJet *****/
            print_title(ptrid,res,EndOfLine);
            pj_width = ydots;
            if (res == 90) pj_width <<= 1;
            PRINTER_PRINTF2("\033*r0B\033*t180R\033*r3U\033*r%dS\033*b0M\033*r0A",
                pj_width);
            pj_width >>= 3;
            break;

        case 5:   /***** PostScript *****/
        case 6:   /***** PostScript Landscape *****/
            if (!((EPSFileType > 0) && (ptrid==5)))
                PRINTER_PRINTF2("%%!PS-Adobe%s",EndOfLine);
            if ((EPSFileType > 0) &&     /* Only needed if saving to .EPS */
                (ptrid == 5))
                {
                PRINTER_PRINTF2("%%!PS-Adobe-1.0 EPSF-2.0%s",EndOfLine);

                if (EPSFileType==1)
                    i=xdots+78;
                else
                    i=(int)((double)xdots * (72.0 / (double)res))+78;

                if (Printer_Titleblock==0)
                    {
                    if (EPSFileType==1) { j = ydots + 78; }
                    else { j = (int)(((double)ydots * (72.0 / (double)res) / (double)finalaspectratio)+78); }
                    }
                else
                    {
                    if (EPSFileType==1) { j = ydots + 123; }
                    else { j = (int)(((double)ydots * (72.0 / (double)res))+123); }
                    }
                PRINTER_PRINTF4("%%%%TemplateBox: 12 12 %d %d%s",i,j,EndOfLine);
                PRINTER_PRINTF4("%%%%BoundingBox: 12 12 %d %d%s",i,j,EndOfLine);
                PRINTER_PRINTF4("%%%%PrinterRect: 12 12 %d %d%s",i,j,EndOfLine);
                PRINTER_PRINTF2("%%%%Creator: Fractint PostScript%s",EndOfLine);
                PRINTER_PRINTF5("%%%%Title: A %s fractal - %s - Fractint EPSF Type %d%s",
                                       curfractalspecific->name[0]=='*' ?
                                       &curfractalspecific->name[1] :
                                       curfractalspecific->name,
                                       PrintName,
                                       EPSFileType,
                                       EndOfLine);
                if (Printer_Titleblock==1)
                    PRINTER_PRINTF2("%%%%DocumentFonts: Helvetica%s",EndOfLine);
                PRINTER_PRINTF2("%%%%EndComments%s",EndOfLine);
                PRINTER_PRINTF2("/EPSFsave save def%s",EndOfLine);
                PRINTER_PRINTF2("0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin%s",EndOfLine);
                PRINTER_PRINTF2("10 setmiterlimit [] 0 setdash newpath%s",EndOfLine);
                }

            /* Common code for all PostScript */
            PRINTER_PRINTF2("/Tr {translate} def%s",EndOfLine);
            PRINTER_PRINTF2("/Mv {moveto} def%s",EndOfLine);
            PRINTER_PRINTF2("/D {dup} def%s",EndOfLine);
            PRINTER_PRINTF2("/Rh {readhexstring} def%s",EndOfLine);
            PRINTER_PRINTF2("/Cf {currentfile} def%s",EndOfLine);
            PRINTER_PRINTF2("/Rs {readstring} def%s",EndOfLine);

            if (Printer_Compress) {
                rleprolog(xdots,ydots);
            } else
            {
                PRINTER_PRINTF3("/picstr %d string def%s",
                        ColorPS?xdots*3:xdots,EndOfLine);
                PRINTER_PRINTF7("/dopic { gsave %d %d 8 [%d 0 0 %d 0 %d]%s",
                                         xdots, ydots, xdots, -ydots, ydots,
                                         EndOfLine);
                PRINTER_PRINTF2("{ Cf picstr Rh pop }%s", EndOfLine);
                if (ColorPS)
                {
                    PRINTER_PRINTF2(" false 3 colorimage grestore } def%s",
                     EndOfLine);
                }
                else
                {
                    PRINTER_PRINTF2(" image grestore } def%s", EndOfLine);
                }
            }
            if (Printer_Titleblock==1)
                {
                PRINTER_PRINTF2("/Helvetica findfont 8 scalefont setfont%s",EndOfLine);
                if (ptrid==5) {PRINTER_PRINTF1("30 60 Mv ");}
                else          {PRINTER_PRINTF1("552 30 Mv 90 rotate ");}
                print_title(ptrid,res,EndOfLine);
                if (ptrid==6) {PRINTER_PRINTF1("-90 rotate ");}
                }

            if (EPSFileType != 1) /* Do not use on a WELL BEHAVED .EPS */
              {
              if (ptrid == 5 && EPSFileType==2
                             && (Printer_ColorXlat || Printer_SetScreen))
                        PRINTER_PRINTF2("%%%%BeginFeature%s",EndOfLine);
              if (ColorPS)
                {
                if (Printer_ColorXlat==1)
                    PRINTER_PRINTF2("{1 exch sub} D D D setcolortransfer%s",EndOfLine);
                if (Printer_ColorXlat>1)
                    PRINTER_PRINTF4("{%d mul round %d div} D D D setcolortransfer%s",
                                       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);
                if (Printer_ColorXlat<-1)
                    PRINTER_PRINTF4("{%d mul round %d div 1 exch sub} D D D setcolortransfer",
                                       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);

                if (Printer_SetScreen==1)
                    {
#ifndef XFRACT
                    static char far fmt_str[] = "%d %d {%Fs}%s";
#else
                    static char fmt_str[] = "%d %d {%s}%s";
#endif
                    Printer_printf(fmt_str,
                                       Printer_RFrequency,
                                       Printer_RAngle,
                                       (char far *)HalfTone[Printer_RStyle],
                                       EndOfLine);
                    Printer_printf(fmt_str,
                                       Printer_GFrequency,
                                       Printer_GAngle,
                                       (char far *)HalfTone[Printer_GStyle],
                                       EndOfLine);
                    Printer_printf(fmt_str,
                                       Printer_BFrequency,
                                       Printer_BAngle,
                                       (char far *)HalfTone[Printer_BStyle],
                                       EndOfLine);
                    Printer_printf(fmt_str,
                                       Printer_SFrequency,
                                       Printer_SAngle,
                                       (char far *)HalfTone[Printer_SStyle],
                                       EndOfLine);
                    PRINTER_PRINTF2("setcolorscreen%s", EndOfLine);
                    }
                }
              else
              {
                 if (Printer_ColorXlat!=-2 && Printer_ColorXlat!=2) {
                    /* b&w case requires no mask building */
                   if (Printer_ColorXlat==1)
                      PRINTER_PRINTF2("{1 exch sub} settransfer%s",EndOfLine);
                   if (Printer_ColorXlat>1)
                      PRINTER_PRINTF4("{%d mul round %d div} settransfer%s",
                                      Printer_ColorXlat,Printer_ColorXlat,EndOfLine);
                   if (Printer_ColorXlat<-1)
                      PRINTER_PRINTF4("{%d mul round %d div 1 exch sub} settransfer",
                                      Printer_ColorXlat,Printer_ColorXlat,EndOfLine);

                   if (Printer_SetScreen==1)
                   {
#ifndef XFRACT
                      PRINTER_PRINTF5("%d %d {%Fs} setscreen%s",
                                     Printer_SFrequency,
                                     Printer_SAngle,
                                     (char far *)HalfTone[Printer_SStyle],
                                     EndOfLine);
#else
                      Printer_printf("%d %d {%s} setscreen%s",
                                     Printer_SFrequency,
                                     Printer_SAngle,
                                     (char far *)HalfTone[Printer_SStyle],
                                     EndOfLine);
#endif
                   }
                }
              }

              if (ptrid == 5)
                 {
                    if ((EPSFileType==2) && (Printer_ColorXlat || Printer_SetScreen))
                        PRINTER_PRINTF2("%%%%EndFeature%s",EndOfLine);
                    if (res == 0)
                    {
                        PRINTER_PRINTF2("30 191.5 Tr 552 %4.1f",
                                        (552.0*(double)finalaspectratio));
                    }
                    else
                    {
                        PRINTER_PRINTF4("30 %d Tr %f %f",
                                     75 - ((Printer_Titleblock==1) ? 0 : 45),
                                     ((double)xdots*(72.0/(double)res)),
                                     ((double)xdots*(72.0/(double)res)*(double)finalaspectratio));
                    }
                  }
                else                         /* For Horizontal PostScript */
                {
                    if (res == 0)
                    {
                        PRINTER_PRINTF2("582 30 Tr 90 rotate 732 %4.1f",
                                        (732.0*(double)finalaspectratio));
                    }
                    else
                    {
                        PRINTER_PRINTF4("%d 30 Tr 90 rotate %f %f",
                                     537 + ((Printer_Titleblock==1) ? 0 : 45),
                                     ((double)xdots*(72.0/(double)res)),
                                     ((double)xdots*(72.0/(double)res)*(double)finalaspectratio));
                    }
                }
                PRINTER_PRINTF2(" scale%s",EndOfLine);
              }

            else if (ptrid == 5)       /* To be used on WELL-BEHAVED .EPS */
                PRINTER_PRINTF5("30 %d Tr %d %d scale%s",
                                    75 - ((Printer_Titleblock==1) ? 0 : 45),
                                    xdots,ydots,EndOfLine);

            PRINTER_PRINTF2("dopic%s",EndOfLine);
            break;

        case 7: /* HP Plotter */
            if (res<1) res=1;
            if (res>10) res=10;
            ci = (((double)xdots*((double)res-1.0))/2.0);
            ck = (((double)ydots*((double)res-1.0))/2.0);
            PRINTER_PRINTF6(";IN;SP0;SC%d,%d,%d,%d;%s\0",
                (int)(-ci),(int)((double)xdots+ci),
                (int)((double)ydots+ck),(int)(-ck),EndOfLine);
            break;
        }
}


static void _fastcall print_title(int ptrid,int res,char *EndOfLine)
{
    char buff[80];
    int postscript;
    if (Printer_Titleblock == 0)
        return;
    postscript = (ptrid == 5 || ptrid ==6);
    if (!postscript)
        Printer_printf(EndOfLine);
    else
        Printer_printf("(");
    Printer_printf((curfractalspecific->name[0]=='*')
                     ? &curfractalspecific->name[1]
                     : curfractalspecific->name);
    if (fractype == FORMULA || fractype == FFORMULA)
        Printer_printf(" %s",FormName);
    if (fractype == LSYSTEM)
        Printer_printf(" %s",LName);
    if (fractype == IFS || fractype == IFS3D)
        Printer_printf(" %s",IFSName);
    PRINTER_PRINTF4(" - %dx%d - %d DPI", xdots, ydots, res);
    if (!postscript)
        Printer_printf(EndOfLine);
    else {
        PRINTER_PRINTF2(") show%s",EndOfLine);
        if (ptrid==5) {PRINTER_PRINTF1("30 50 moveto (");}
        else          PRINTER_PRINTF1("-90 rotate 562 30 moveto 90 rotate (");
        }
    PRINTER_PRINTF5("Corners: Top-Left=%.16g/%.16g Bottom-Right=%.16g/%.16g",
                   xxmin,yymax,xxmax,yymin);
    if (xx3rd != xxmin || yy3rd != yymin) {
        if (!postscript)
            Printer_printf("%s        ",EndOfLine);
        PRINTER_PRINTF3(" Bottom-Left=%4.4f/%4.4f",xx3rd,yy3rd);
        }
    if (!postscript)
        Printer_printf(EndOfLine);
    else {
        PRINTER_PRINTF2(") show%s",EndOfLine);
        if (ptrid==5) {PRINTER_PRINTF1("30 40 moveto (");}
        else          PRINTER_PRINTF1("-90 rotate 572 30 moveto 90 rotate (");
        }
    showtrig(buff);
    PRINTER_PRINTF6("Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s",
                   param[0],param[1],param[2],param[3],buff);
    if (!postscript)
        Printer_printf(EndOfLine);
    else
        PRINTER_PRINTF2(") show%s",EndOfLine);
}

/* This function prints a string to the the printer with BIOS calls. */

#ifndef USE_VARARGS
static void Printer_printf(char far *fmt,...)
#else
static void Printer_printf(va_alist)
va_dcl
#endif
{
int i;
char s[500];
int x=0;
va_list arg;

#ifndef USE_VARARGS
va_start(arg,fmt);
#else
char far *fmt;
va_start(arg);
fmt = va_arg(arg,char far *);
#endif

{
   /* copy far to near string */
   char fmt1[100];
   i=0;
   while(fmt[i]) {
     fmt1[i] = fmt[i];
     i++;
   }
   fmt1[i] = '\0';
   vsprintf(s,fmt1,arg);
}

if (Print_To_File>0)    /* This is for printing to file */
    fprintf(PRFILE,"%s",s);
else                    /* And this is for printing to printer */
    while (s[x])
        if (printer(s[x++]) != 0)
            while (!keypressed()) { if (printer(s[x-1])==0) break; }
}

/* This function standardizes both _bios_printer and _bios_serialcom
 * in one function.  It takes its arguments and rearranges them and calls
 * the appropriate bios call.  If it then returns result !=0, there is a
 * problem with the printer.
 */
static int _fastcall printer(int c)
{
    if (Print_To_File>0) return ((fprintf(PRFILE,"%c",c))<1);
#ifndef XFRACT
    if (LPTn<9)  return (((_bios_printer(0,LPTn,c))+0x0010)&0x0010);
    if (LPTn<19) return ((_bios_serialcom(1,(LPTn-10),c))&0x9E00);
    if ((LPTn==20)||(LPTn==21))
        {
        int PS=0;
        while ((PS & 0xF8) != 0xD8)
            { PS = inp((LPTn==20) ? 0x379 : 0x279);
              if (keypressed()) return(1); }
        outp((LPTn==20) ? 0x37C : 0x27C,c);
        PS = inp((LPTn==20) ? 0x37A : 0x27A);
        outp((LPTn==20) ? 0x37A : 0x27A,(PS | 0x01));
        outp((LPTn==20) ? 0x37A : 0x27A,PS);
        return(0);
        }
    if ((LPTn==30)||(LPTn==31))
        {
        while (((inp((LPTn==30) ? 0x3FE : 0x2FE)&0x30)!=0x30) ||
               ((inp((LPTn==30) ? 0x3FD : 0x2FD)&0x60)!=0x60))
            { if (keypressed()) return (1); }
        outp((LPTn==30) ? 0x3F8 : 0x2F8,c);
        return(0);
        }
#endif

    /* MCP 7-7-91, If we made it down to here, we may as well error out. */
    return(-1);
}

#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn +eff
#endif
#endif

static void printer_reset(void)
{
#ifndef XFRACT
    if (Print_To_File < 1)
        if (LPTn<9)       _bios_printer(1,LPTn,0);
        else if (LPTn<19) _bios_serialcom(3,(LPTn-10),0);
#endif
}


/** debug code for pj_ color table checkout
color_test()
{
   int x,y,color,i,j,xx,yy;
   int bw,cw,bh,ch;
   setvideomode(videoentry.videomodeax,
                videoentry.videomodebx,
                videoentry.videomodecx,
                videoentry.videomodedx);
   bw = xdots/25; cw = bw * 2 / 3;
   bh = ydots/10; ch = bh * 2 / 3;
   dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 60;
   if (debugflag == 902)
      dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
   for (x = 0; x < 25; ++x)
      for (y = 0; y < 10; ++y) {
         if (x < 11) i = (32 - x) * 10 + y;
             else    i = (24 - x) * 10 + y;
         color = x * 10 + y + 1;
         dacbox[color][0] = triple[0][0][i];
         dacbox[color][1] = triple[0][1][i];
         dacbox[color][2] = triple[0][2][i];
         for (i = 0; i < cw; ++i) {
            xx = x * bw + bw / 6 + i;
            yy = y * bh + bh / 6;
            for (j = 0; j < ch; ++j)
               putcolor(xx,yy++,color);
            }
         }
   spindac(0,1);
   getakey();
}
**/


/*
 * The following code for compressed PostScript is based on pnmtops.c.  It is
 * copyright (C) 1989 by Jef Poskanzer, and carries the following notice:
 * "Permission to use, copy, modify, and distribute this software and its
 *  documentation for any purpose and without fee is hereby granted, provided
 *  that the above copyright notice appear in all copies and that both that
 *  copyright notice and this permission notice appear in supporting
 *  documentation.  This software is provided "as is" without express or
 *  implied warranty."
 */


static void rleprolog(int x,int y)
{
    itemsperline = 0;
    items = 0;
    bitspersample = 8;
    repeat = 1;
    rlebitsperitem = 0;
    rlebitshift = 0;
    count = 0;

    PRINTER_PRINTF2( "/rlestr1 1 string def%s", EndOfLine );
    PRINTER_PRINTF2( "/rdrlestr {%s", EndOfLine );      /* s -- nr */
    PRINTER_PRINTF2( "  /rlestr exch def%s", EndOfLine );    /* - */
    PRINTER_PRINTF2( "  Cf rlestr1 Rh pop%s", EndOfLine );  /* s1 */
    PRINTER_PRINTF2( "  0 get%s", EndOfLine );                     /* c */
    PRINTER_PRINTF2( "  D 127 le {%s", EndOfLine );              /* c */
    PRINTER_PRINTF2( "    Cf rlestr 0%s", EndOfLine );    /* c f s 0 */
    PRINTER_PRINTF2( "    4 3 roll%s", EndOfLine );                /* f s 0 c */
    PRINTER_PRINTF2( "    1 add  getinterval%s", EndOfLine );      /* f s */
    PRINTER_PRINTF2( "    Rh pop%s", EndOfLine );       /* s */
    PRINTER_PRINTF2( "    length%s", EndOfLine );                  /* nr */
    PRINTER_PRINTF2( "  } {%s", EndOfLine );                       /* c */
    PRINTER_PRINTF2( "    256 exch sub D%s", EndOfLine );        /* n n */
    PRINTER_PRINTF2( "    Cf rlestr1 Rh pop%s", EndOfLine );/* n n s1 */
    PRINTER_PRINTF2( "    0 get%s", EndOfLine );                   /* n n c */
    PRINTER_PRINTF2( "    exch 0 exch 1 exch 1 sub {%s", EndOfLine );           /* n c 0 1 n-1*/
    PRINTER_PRINTF2( "      rlestr exch 2 index put%s", EndOfLine );
    PRINTER_PRINTF2( "    } for%s", EndOfLine );                   /* n c */
    PRINTER_PRINTF2( "    pop%s", EndOfLine );                     /* nr */
    PRINTER_PRINTF2( "  } ifelse%s", EndOfLine );                  /* nr */
    PRINTER_PRINTF2( "} bind def%s", EndOfLine );
    PRINTER_PRINTF2( "/Rs {%s", EndOfLine );               /* s -- s */
    PRINTER_PRINTF2( "  D length 0 {%s", EndOfLine );            /* s l 0 */
    PRINTER_PRINTF2( "    3 copy exch%s", EndOfLine );          /* s l n s n l*/
    PRINTER_PRINTF2( "    1 index sub%s", EndOfLine );          /* s l n s n r*/
    PRINTER_PRINTF2( "    getinterval%s", EndOfLine );          /* s l n ss */
    PRINTER_PRINTF2( "    rdrlestr%s", EndOfLine );        /* s l n nr */
    PRINTER_PRINTF2( "    add%s", EndOfLine );                     /* s l n */
    PRINTER_PRINTF2( "    2 copy le { exit } if%s", EndOfLine );   /* s l n */
    PRINTER_PRINTF2( "  } loop%s", EndOfLine );                    /* s l l */
    PRINTER_PRINTF2( "  pop pop%s", EndOfLine );                   /* s */
    PRINTER_PRINTF2( "} bind def%s", EndOfLine );
    if (ColorPS) {
        PRINTER_PRINTF3( "/rpicstr %d string def%s", x,EndOfLine );
        PRINTER_PRINTF3( "/gpicstr %d string def%s", x,EndOfLine );
        PRINTER_PRINTF3( "/bpicstr %d string def%s", x,EndOfLine );
    } else {
        PRINTER_PRINTF3( "/picstr %d string def%s", x,EndOfLine );
    }
    PRINTER_PRINTF2( "/dopic {%s", EndOfLine);
    PRINTER_PRINTF2( "/gsave%s", EndOfLine);
    if (ColorPS) {
       PRINTER_PRINTF4( "%d %d 8%s", x, y, EndOfLine);
    } else { /* b&w */
       if (Printer_ColorXlat==-2) {
          PRINTER_PRINTF4( "%d %d true%s", x, y, EndOfLine);
       } else if (Printer_ColorXlat==2) {
          PRINTER_PRINTF4( "%d %d false%s", x, y, EndOfLine);
       } else {
          PRINTER_PRINTF4( "%d %d 8%s", x, y, EndOfLine);
       }
    }
    PRINTER_PRINTF5( "[%d 0 0 %d 0 %d]%s", x, -y, y, EndOfLine);
    if (ColorPS) {
        PRINTER_PRINTF2( "{rpicstr Rs}%s", EndOfLine);
        PRINTER_PRINTF2( "{gpicstr Rs}%s", EndOfLine);
        PRINTER_PRINTF2( "{bpicstr Rs}%s", EndOfLine);
        PRINTER_PRINTF2( "true 3 colorimage%s", EndOfLine);
    } else {
       if (Printer_ColorXlat==-2 || Printer_ColorXlat==2) {
          /* save file space and printing time (if scaling is right) */
          PRINTER_PRINTF2( "{picstr Rs} imagemask%s", EndOfLine);
       } else {
          PRINTER_PRINTF2( "{picstr Rs} image%s", EndOfLine);
       }
    }
    PRINTER_PRINTF2( "} def%s", EndOfLine);
}

static void
rleputbuffer(void)
    {
    int i;

    if ( repeat )
        {
        item = 256 - count;
        putitem();
        item = repeatitem;
        putitem();
        }
    else
        {
        item = count - 1;
        putitem();
        for ( i = 0; i < count; ++i )
            {
            item = itembuf[i];
            putitem();
            }
        }
    repeat = 1;
    count = 0;
    }

static void
rleputitem(void)
    {
    int i;

    if ( count == 128 )
        rleputbuffer();

    if ( repeat && count == 0 )
        { /* Still initializing a repeat buf. */
        itembuf[count] = repeatitem = rleitem;
        ++count;
        }
    else if ( repeat )
        { /* Repeating - watch for end of run. */
        if ( rleitem == repeatitem )
            { /* Run continues. */
            itembuf[count] = rleitem;
            ++count;
            }
        else
            { /* Run ended - is it long enough to dump? */
            if ( count > 2 )
                { /* Yes, dump a repeat-mode buffer and start a new one. */
                rleputbuffer();
                itembuf[count] = repeatitem = rleitem;
                ++count;
                }
            else
                { /* Not long enough - convert to non-repeat mode. */
                repeat = 0;
                itembuf[count] = repeatitem = rleitem;
                ++count;
                repeatcount = 1;
                }
            }
        }
    else
        { /* Not repeating - watch for a run worth repeating. */
        if ( rleitem == repeatitem )
            { /* Possible run continues. */
            ++repeatcount;
            if ( repeatcount > 3 )
                { /* Long enough - dump non-repeat part and start repeat. */
                count = count - ( repeatcount - 1 );
                rleputbuffer();
                count = repeatcount;
                for ( i = 0; i < count; ++i )
                    itembuf[i] = rleitem;
                }
            else
                { /* Not long enough yet - continue as non-repeat buf. */
                itembuf[count] = rleitem;
                ++count;
                }
            }
        else
            { /* Broken run. */
            itembuf[count] = repeatitem = rleitem;
            ++count;
            repeatcount = 1;
            }
        }

    rleitem = 0;
    rlebitsperitem = 0;
    }

static void
putitem (void)
    {
    char* hexits = "0123456789abcdef";

    if ( itemsperline == 30 )
        {
        Printer_printf("%s",EndOfLine);
        itemsperline = 0;
        }
    Printer_printf("%c%c", hexits[item >> 4], hexits[item & 15] );
    ++itemsperline;
    ++items;
    item = 0;
    /* bitsperitem = 0; */
    bitshift2 = 8 - bitspersample;
    }

static void
rleputxelval( int xv )
    {
    if ( rlebitsperitem == 8 )
        rleputitem();
    rleitem += xv<<bitshift2;
    rlebitsperitem += bitspersample;
    rlebitshift -= bitspersample;
    }

static void
rleflush(void)
    {
    if ( rlebitsperitem > 0 )
        rleputitem();
    if ( count > 0 )
        rleputbuffer();
    }

static void
rleputrest(void)
    {
    rleflush();
    Printer_printf( "%s",EndOfLine );
    Printer_printf( "grestore%s",EndOfLine );
    }
