/** targa.c **/

#ifdef __TURBOC__
#       pragma  warn -par
#endif

#define TARGA_DATA

#include        <string.h>
#ifndef XFRACT
#include        <conio.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "targa.h"
#include "drivers.h"

/*************  ****************/

void    WriteTGA( int x, int y, int index );
int     ReadTGA ( int x, int y );
void    EndTGA  ( void );
void    StartTGA( void );
void    ReopenTGA( void );

/*************  ****************/

static unsigned _fastcall near Row16Calculate(unsigned,unsigned);
static void     _fastcall near PutPix16(int,int,int);
static unsigned _fastcall near GetPix16(int,int);
static unsigned _fastcall near Row32Calculate(unsigned,unsigned);
static void     _fastcall near PutPix32(int,int,int);
static unsigned _fastcall near GetPix32(int,int);
static void     _fastcall near DoFirstPixel(int,int,int);
static void _fastcall fatalerror(char far *);
static int  GetLine(int);
static void _fastcall near SetDispReg(int,int);
static int  VWait(void);
static void _fastcall SetVBorder(int,int);
static void _fastcall SetBorderColor(long);
static void _fastcall SetVertShift(int);
static void _fastcall SetInterlace(int);
static void _fastcall SetBlndReg(int);
static void _fastcall SetRGBorCV(int);
static void _fastcall SetVCRorCamera(int);
static void _fastcall SetMask(int);
static void _fastcall SetBlndReg(int);
static void _fastcall SetContrast(int);
static void _fastcall SetHue(int);
static void _fastcall SetSaturation(int);
static void _fastcall SetHBorder(int,int);
static void SetFixedRegisters(void);
static void _fastcall VCenterDisplay(int);
static void _fastcall SetOverscan(int);
static void _fastcall near TSetMode(int);
static int  GraphInit(void);
static void GraphEnd(void);

/*************  ****************/

int          xorTARGA;
unsigned far *tga16 = NULL;  /* [256] */
long     far *tga32;         /* [256] */
static int   last = 0;

/*************  ****************/

static int      initialized;

/*************  ****************/

static void     (near _fastcall *DoPixel) ( int x, int y, int index );
static void     (near _fastcall *PutPixel)( int x, int y, int index );
static unsigned (near _fastcall *GetPixel)( int x, int y );

/**************************************************************************/
#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn -eff
#endif
#endif

static unsigned _fastcall near Row16Calculate( unsigned line, unsigned x1 )
{
        outp( DESTREG, (line >> 5) );
        return( ((line & 31) << 10) | (x1 << 1) ); /* calc the pixel offset */
}

/**************************************************************************/

static void _fastcall near PutPix16( int x, int y, int index )
{
unsigned far * ip;

        /**************/
        ip = MK_FP( MEMSEG, Row16Calculate( y, x ) );
        if( ! xorTARGA )
                *ip = tga16[index];
        else
                *ip = *ip ^ 0x7fff;
}

/**************************************************************************/

static unsigned _fastcall near GetPix16( int x, int y )
{
register unsigned pixel, index;
unsigned far * ip;
        /**************/
        ip = MK_FP( MEMSEG, Row16Calculate( y, x ) );
        pixel = *ip & 0x7FFF;
        if( pixel == tga16[last] ) return( last );
        for( index = 0; index < 256; index++ )
                if( pixel == tga16[index] ) {
                        last = index;
                        return( index );
                }
        return( 0 );
}

/**************************************************************************/

static unsigned _fastcall near Row32Calculate( unsigned line, unsigned x1 )
{
        outp( DESTREG, (line >> 4) );
        return ( ((line & 15) << 11) | (x1 << 2) ); /* calc the pixel offset */
}

/**************************************************************************/

static void _fastcall near PutPix32( int x, int y, int index )
{
long far * lp;
        lp = MK_FP( MEMSEG, Row32Calculate( y, x ) );
        if( ! xorTARGA )
                *lp = tga32[index];
        else
                *lp = *lp ^ 0x00FFFFFFL;
}

/**************************************************************************/

static unsigned _fastcall near GetPix32( int x, int y )
{
register int index;
long    pixel;
long    far * lp;

        lp = MK_FP( MEMSEG, Row32Calculate( y, x ) );
        pixel = *lp & 0x00FFFFFFL;
        if( pixel == tga32[last] ) return( last );
        for( index = 0; index < 256; index++ )
                if( pixel == tga32[index] ) {
                        last = index;
                        return( index );
                }
        return( 0 );
}

/**************************************************************************/

static void _fastcall near DoFirstPixel( int x, int y, int index )
{
int cnt;
        TSetMode( targa.mode | 1 );
        for( cnt = 0; cnt < targa.MaxBanks; cnt += 2 ) { /* erase */
                outp( DESTREG, cnt );
                outp( SRCREG,  cnt + 1 );
                erasesegment(targa.memloc,0);  /** general.asm **/
        }
        TSetMode( targa.mode & 0xFFFE );
        PutPixel = DoPixel;
        (*PutPixel)( x, y, index );
}

#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn +eff
#endif
#endif

/***************************************************************************/

void WriteTGA( int x, int y, int index )
{
        OUTPORTB(MODEREG, targa.mode |= 1 );    /* TSetMode inline for speed */
        (*PutPixel)( x, sydots-y, index&0xFF ); /* fix origin to match EGA/VGA */
        OUTPORTB(MODEREG, targa.mode &= 0xFFFE );
}

/***************************************************************************/

int ReadTGA( int x, int y )
{
int val;
        OUTPORTB(MODEREG, targa.mode |= 1 );    /* TSetMode inline for speed */
        val = (*GetPixel)( x, sydots-y );
        OUTPORTB(MODEREG, targa.mode &= 0xFFFE );
        return( val );
}

/***************************************************************************/

void EndTGA( void )
{
        if( initialized ) {
                GraphEnd();
                initialized = 0;
        }
}

/***************************************************************************/

void StartTGA()
{
int     i;
/*
   This overlayed data safe because used in this file, any only used for
   fatal error message!
*/
static FCODE couldntfind[]={"Could not find Targa card"};
static FCODE noenvvar[]={"TARGA environment variable missing"};
static FCODE insuffmem[]={"Insufficient memory for Targa"};

        /****************/
        if( initialized ) return;
        initialized = 1;

        /****************/
        /* note that video.asm has already set the regualar video adapter */
        /* to text mode (ax in Targa table entries is 3);                 */
        /* that's necessary because TARGA can live at 0xA000, we DO NOT   */
        /* want to have an EGA/VGA in graphics mode!!                     */
        ReopenTGA(); /* clear text screen and display message */

        /****************/
        /*** look for and activate card ***/
        if ((i = GraphInit()) != 0)
                fatalerror((i == -1) ? couldntfind : noenvvar);

        VCenterDisplay( sydots + 1 );

        if (tga16 == NULL)
            if ( (tga16 = (unsigned far *)malloc(512L)) == NULL
              || (tga32 = (long     far *)malloc(1024L)) == NULL)
                fatalerror(insuffmem);

        SetTgaColors();

        if( targa.boardType == 16 ) {
                GetPixel = (unsigned (near _fastcall *)(int, int))GetPix16;
                DoPixel = PutPix16;
        }
        else {
                GetPixel = (unsigned (near _fastcall *)(int, int))GetPix32;
                DoPixel = PutPix32;
        }
        PutPixel = DoFirstPixel;        /* on first pixel --> erase */

        if( sydots == 482 ) SetOverscan( 1 );

        TSetMode( targa.mode & 0xFFFE );

        /****************/
        if (mapdacbox == NULL && SetColorPaletteName("default") != 0)
                exit( 1 ); /* stopmsg has already been issued */

}

void ReopenTGA()
{
static FCODE runningontarga[]={"Running On TrueVision TARGA Card"};
        helptitle();
        driver_put_string(2,20,7,runningontarga);
        driver_move_cursor(6,0); /* in case of brutal exit */
}

static void _fastcall fatalerror(char far *msg)
{
static FCODE abortmsg[]={"...aborting!"};
        driver_put_string(4,20,15,msg);
        driver_put_string(5,20,15,abortmsg);
        driver_move_cursor(8,0);
        exit(1);
}



/***  the rest of this module used to be separate, in tgasubs.c,  ***/
/***  has now been merged into a single source                    ***/

/*******************************************************************/

static void _fastcall VCenterDisplay( int nLines )
{
int     lines;
int top, bottom;
long color;

        lines  = nLines >> 1;           /* half value of last line 0..x */
        top    = 140 - (lines >> 1);
        bottom = top + lines;
        SetVBorder( top, bottom );
        SetVertShift( 255 - lines );    /* skip lines we're not using */

        if( targa.boardType == 16 )
                color = (12 << 10) | (12 << 5) | 12;
        else
                color = ((long)80 << 16) | (80 << 8) | 80;
        SetBorderColor( color );
}


/*****************************************************************/

static void _fastcall near SetDispReg(int reg, int value)
{
        targa.DisplayRegister[reg] = value;

        TSetMode(targa.mode&MSK_REGWRITE);  /* select Index Register write */
        OUTPORTB(DRREG, reg);   /* select sync register */
        /*
         *              Set Mask register to write value to
         *              display register and to set Bit 9 in the DR
         */
        TSetMode( ((targa.mode|(~MSK_REGWRITE))     /* turn on write bit */
                                   & MSK_BIT9      )     /* turn off Bit 9 */
                           | ((value&0x0100)>>1)); /* set bit 9 for value */
        OUTPORTB(DRREG, value); /* select sync register */
 }

/*****************************************************************/

#define   WAITCOUNT  60000L

static int VWait()
{
int     rasterreg;
unsigned GiveUp;

        rasterreg = RASTERREG;

        /*
         *      If beyond bottom of frame wait for next field
         */
        GiveUp= WAITCOUNT;
        while ( (--GiveUp) && (GetLine(rasterreg) == 0) ) { }
        if (GiveUp) {
           /*
            *      Wait for the bottom of the border
            */
           GiveUp= WAITCOUNT;
           while ( (--GiveUp) && (GetLine(rasterreg) > 0) ) { }
           }

        return ( ( GiveUp ) ? 0 : -1);
}


/*****************************************************************/

static void _fastcall SetVBorder(int top, int bottom)
{
        /* top border */
        if ( top < MIN_TOP ) top=MIN_TOP;
        SetDispReg(TOPBORDER,top);
        /* bottom border */
        if ( bottom > MAX_BOTTOM ) bottom=MAX_BOTTOM;
        SetDispReg(BOTTOMBORDER,bottom);

        SetDispReg(DR10,top);
        SetDispReg(DR11,bottom);
}


/*****************************************************************/

static void _fastcall SetRGBorCV(int type)
{
        /*  set the contrast level */
        targa.RGBorCV = type;
        targa.VCRCon = ( targa.VCRCon  & MSK_RGBORCV ) |
                                        (targa.RGBorCV<<SHF_RGBORCV) ;
        OUTPORTB(VCRCON, targa.VCRCon );
}

/*****************************************************************/

static void _fastcall SetVCRorCamera(int type)
{
        targa.VCRorCamera = type&1;
        targa.VCRCon = ( targa.VCRCon  & MSK_VCRORCAMERA ) |
                                        (targa.VCRorCamera<<SHF_VCRORCAMERA) ;
        OUTPORTB(VCRCON, targa.VCRCon );
}


/*****************************************************************/

static void _fastcall SetBorderColor(long color)
{
        targa.BorderColor = color;
        OUTPORTB(BORDER, (int)(0x0000ffffL&(color)));
        OUTPORTB((BORDER+2), (int)((color)>>16));
}

/*****************************************************************/

static void _fastcall SetMask(int mask)
{
        /* mask to valid values and output to mode register */
        targa.Mask = mask;
        OUTPORTB(MASKREG, mask);
}


/*****************************************************************/

static void _fastcall SetVertShift(int preshift)
{
        /*  set the Vertical Preshift count  level */
        targa.VertShift = preshift;
        OUTPORTB(VERTPAN, preshift);
}


/*****************************************************************/

static void _fastcall SetOverscan(int mode)
{
long tempColor;

        targa.ovrscnOn = mode;
        if ( mode == 0 ) {
                        INPORTB(UNDERREG);  /*  select underscan mode */
                        SetHBorder(     (DEF_LEFT+targa.xOffset),
                                                (DEF_RIGHT+targa.xOffset));
                        SetDispReg(4,352);
                        SetDispReg(5,1);
                        SetBorderColor(targa.BorderColor);
        }
        else    {
                        INPORTB(OVERREG);   /*  select overrscan mode */
                        SetDispReg(0,64);   /*  Set four of the display registers */
                        SetDispReg(1,363);  /*  to values required for Overscan */
                        SetDispReg(4,363);
                        SetDispReg(5,17);
                        tempColor = targa.BorderColor;
                        SetBorderColor(0L);
                        targa.BorderColor = tempColor;
        }
}


/*****************************************************************/

static void _fastcall SetInterlace(int type)
{
        targa.InterlaceMode= type & MSK_INTERLACE;
        SetDispReg(INTREG, targa.InterlaceMode);
        /*
         *      SET THE INTERLACE BIT TO MATCH THE INTERLACE MODE AND
         *      SCREEN RESOLUTION -  SCREEN PAGE
         */
        if ( ( targa.InterlaceMode >= 2 ) &&
                 ( targa.PageMode> 1 )  &&
                 ( (targa.PageMode&1) != 0 )  )
                TSetMode(targa.mode|(~MSK_IBIT) );
        else
                TSetMode(targa.mode& MSK_IBIT);
}


/*****************************************************************/

static void _fastcall SetBlndReg(int value)
{
        /*  set the Vertical Preshift count  level */
        if ( targa.boardType == 32 ) {
                targa.VCRCon = (targa.VCRCon&0xfe) | value;
                OUTPORTB(BLNDREG, value);
                }
}


/*****************************************************************/

static void _fastcall near TSetMode(int mode)
{
        /* mask to valid values and output to mode register */
        OUTPORTB(MODEREG, mode );
        targa.mode = mode;
}


/*****************************************************************/

static void _fastcall SetContrast(int level)
{
        /*  set the contrast level */
        targa.Contrast = level &((~MSK_CONTRAST)>>SHF_CONTRAST);
        targa.VCRCon = ( targa.VCRCon  & MSK_CONTRAST ) |
                        (targa.Contrast<<SHF_CONTRAST) ;
        OUTPORTB(VCRCON, targa.VCRCon );
}


/*****************************************************************/

static void _fastcall SetHue(int level)
{
        /*  set the hue level -  Mask to valid value */
        targa.Hue = level&((~MSK_HUE)>>SHF_HUE);
        /* mask to valid range */
        targa.SatHue = (targa.SatHue&MSK_HUE) | (targa.Hue<<SHF_HUE);
        OUTPORTB(SATHUE, targa.SatHue );
}


/*****************************************************************/

static void _fastcall SetSaturation(int level)
{
        /*  set the saturation level */
        targa.Saturation= level&( (~MSK_SATURATION)>>SHF_SATURATION);
        targa.SatHue =  (targa.SatHue&MSK_SATURATION) |
                         (targa.Saturation<<SHF_SATURATION);
        OUTPORTB(SATHUE , targa.SatHue );
}


/*************************************************************/

/*** UNUSED
static void _fastcall SetPageMode(int pageMode)
{
        pageMode &= 0x07;
        targa.PageMode = pageMode;
        VWait();
        TSetMode( (targa.mode)&(MSK_RES) |((pageMode<<SHF_RES)&(~MSK_RES)) ) ;
        if ( ( targa.DisplayRegister[20] >= 2 ) &&
                 ( pageMode> 1 )  &&
                 ( (pageMode&1) != 0 )    )     TSetMode(targa.mode|(~MSK_IBIT) );
        else
                TSetMode(targa.mode& MSK_IBIT);
}
***/


/*****************************************************************/

static void _fastcall SetHBorder(int left, int right)
{
        SetDispReg(LEFTBORDER, left);   /* set horizontal left border */
        SetDispReg(RIGHTBORDER,right);  /* set horizontal right border */
/*
 *                                      Set DR 8 and 9 since they
 *                                      default to tracking DR0 and DR 1
 */
        SetDispReg(DR8,left);
        SetDispReg(DR9,left);
}


/*****************************************************************/

/*** UNUSED
static void _fastcall SetGenlock(int OnOrOff)
{
        TSetMode( (targa.mode)&(MSK_GENLOCK)
                        |((OnOrOff<<SHF_GENLOCK)&(~MSK_GENLOCK)) );
}
***/


/*****************************************************************/
/* was asm, TC fast enough on AT */

static int GetLine( int port )
{
int cnt;
int val1, val2;

        val1 = INPORTB( port );
        for( cnt = 0; cnt < 20; cnt++ ) {
                val2 = INPORTB( port );
                if( val1 == val2 )
                        break;
                val1 = val2;
        }
        return( val1 );
}


/**********************************************************************
                          TINIT
**********************************************************************/

static int GraphInit()
{
int i;
int bottom, top;
char *envptr;
unsigned switches, got_switches;

        memset( &targa, 0, sizeof(targa) );

        targa.boardType = TYPE_16;              /* default to T16 */
        targa.xOffset = 0;  targa.yOffset = 0;  /* default to no offset */
        targa.LinesPerField= DEF_ROWS/2;        /*  number of lines per field */
        targa.AlwaysGenLock = DEF_GENLOCK;      /* default to genlock off */
        targa.PageMode = 0;
        targa.InterlaceMode = DEF_INT;          /* Defalut:  Interlace Mode 0 */
        targa.Contrast= DEF_CONTRAST;
        targa.Saturation = DEF_SATURATION;
        targa.Hue = DEF_HUE;
        targa.RGBorCV = CV;                     /* default to Composite video */
        targa.VCRorCamera = CAMERA;
        targa.PanXOrig = 0; targa.PanYOrig = 0;
        targa.PageUpper= 0xffff;                /* set the bank flags to illega& values */
        targa.PageLower= 0xffff;                /* so that they will be set the first time */
        targa.ovrscnAvail = 0;                  /* Assume no Overscan option */
        targa.ovrscnOn = 0;

        if ((envptr = getenv("TARGA")) == NULL)
           return(-2);
        switches = got_switches = 0;
        while (*envptr) {
           if (*envptr != ' ') ++got_switches;
           if (*envptr >= '2' && *envptr <= '8')
              switches |= (1 << ('8' - *envptr));
           ++envptr;
           }
        if (got_switches == 0) { /* all blanks, use default */
           targa.memloc = (signed int)0xA000;
           targa.iobase = 0x220;
           }
        else {
           targa.memloc = 0x8000 + ((switches & 0x70) << 8);
           targa.iobase = 0x200  + ((switches & 0x0f) << 4);
           }

        if ((envptr = getenv("TARGASET")) != NULL) {
           for(;;) { /* parse next parameter */
              while (*envptr == ' ' || *envptr == ',') ++envptr;
              if (*envptr == 0) break;
              if (*envptr >= 'a' && *envptr <= 'z') *envptr -= ('a'-'A');
              i = atoi(envptr+1);
              switch (*envptr) {
                 case 'T':
                    if (i == 16) targa.boardType = TYPE_16;
                    if (i == 24) targa.boardType = TYPE_24;
                    if (i == 32) targa.boardType = TYPE_32;
                    break;
                 /* case 'E' not done, meaning not clear */
                 case 'X':
                    targa.xOffset = i;
                    break;
                 case 'Y':
                    targa.yOffset = i;
                    break;
                 case 'I':
                    targa.InterlaceMode = i;
                    break;
                 /* case 'N' not done, I don't know how to handle it */
                 case 'R':
                    targa.RGBorCV = RGB;
                    break;
                 case 'B':
                    targa.VCRorCamera = CAMERA;
                    break;
                 case 'V':
                    targa.VCRorCamera = VCR;
                    break;
                 case 'G':
                    targa.AlwaysGenLock = 1;
                    break;
                 case 'C':
                    targa.Contrast = i * 31 / 100;
                    break;
                 case 'S':
                    targa.Saturation = i * 7 / 100;
                    break;
                 case 'H':
                    targa.Hue = i * 31 / 100;
                    break;
                 /* note: 'A' and 'O' defined but apply only to type M8 */
                 /* case 'P' not handled cause I don't know how */
                 }
              while (*(++envptr) >= '0' && *envptr <= '9') { }
              }
           }

        if ( targa.boardType == TYPE_16 ) {
                targa.MaxBanks = 16;
                targa.BytesPerPixel = 2;
        }
        if ( targa.boardType == TYPE_24 ) {
                targa.MaxBanks = 32;
                targa.BytesPerPixel = 3;
        }
        if ( targa.boardType == TYPE_32 ) {
                targa.MaxBanks = 32;
                targa.BytesPerPixel = 4;
        }

        /****** Compute # of rows per 32K bank  ********/
        targa.RowsPerBank = 512/(targa.MaxBanks);
        targa.AddressShift =  targa.MaxBanks>>4;

        /*      if initializing CVA:  set these before we quit  */
        SetSaturation(targa.Saturation);
        SetHue(targa.Hue);
        SetContrast( targa.Contrast);

        /*      Set Genlock bit if always genlocked */
        /*      Set before flipping and jerking screen */
        TSetMode( (targa.AlwaysGenLock<<SHF_GENLOCK) | DEF_MODE);

        SetBlndReg(0);          /*  disable blend mode on TARGA 32 */

        SetInterlace(targa.InterlaceMode);
        SetFixedRegisters();
        SetOverscan( 0 );

        top    = 140 - (targa.LinesPerField / 2);
        bottom = top + targa.LinesPerField;
        SetVBorder(top,bottom);
        SetVertShift(256-targa.LinesPerField);

        SetMask(DEF_MASK);
        SetRGBorCV (targa.RGBorCV );
        SetVCRorCamera(targa.VCRorCamera);

        /*      See if the raster register is working correctly and
                return error flag if its not         */
        return (( VWait() == -1) ? -1 : 0);

}


/**************************************************************/

static void GraphEnd()
{
        TSetMode( (targa.mode)&MSK_MSEL );     /*  disable memory */
}


/**************************************************************/
/* Set the registers which have required values */

#define FIXED_REGS      10

static int FixedRegs[] = {
        DR6,DR7,DR12,DR13,DR14,DR15,DR16,DR17,DR18,DR19
};

static int FixedValue[] = {
        DEF_DR6,DEF_DR7,DEF_DR12,DEF_DR13,DEF_DR14,
        DEF_DR15,DEF_DR16,DEF_DR17,DEF_DR18,DEF_DR19
};

static void SetFixedRegisters()
{
int reg;

        for ( reg=0; reg<FIXED_REGS; reg++)
                SetDispReg(FixedRegs[reg],FixedValue[reg]);
}

