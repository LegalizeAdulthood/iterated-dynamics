/* targa.h */


#ifndef TARGA_H
#define TARGA_H


extern unsigned int _dataseg_xx;

/****************************************************************/

#ifdef __TURBOC__
#       define          PEEK(a,b,c,d)           movedata( b, a, _DS, c, d)
#       define          POKE(a,b,c,d)           movedata( _DS, c, b, a, d )
#       define          OUTPORTB        outportb
#       define          INPORTB         inportb
#       define          OUTPORTW        outport
#       define          INPORTW         inport
#else
#       define          PEEK(a,b,c,d)           movedata( b, a, _dataseg_xx, c, d)
#       define          POKE(a,b,c,d)           movedata( _dataseg_xx, c, b, a, d )
#       define          OUTPORTB        outp
#       define          INPORTB         inp
#       define          OUTPORTW        outpw
#       define          INPORTW         inpw
#endif

#define  FALSE  0
#ifdef TRUE
#undef TRUE
#endif
#define  TRUE   1

/****************************************************************/

#define TSEG                    0xA000
#define TIOBASE         0x220


/****************************************************************/


#define         TYPE_8          8
#define         TYPE_16         16
#define         TYPE_24         24
#define         TYPE_32         32
#define         TYPE_M8         -8

/*
 * TARGA: 400 to 482 rows x 512 pixels/row X 16 bits/pixel
 */

#define XMIN            0
#define YMIN            0
#define XMAX            512                     /* maximum X value */
#define YMAX            512                     /* maximum Y value */
#define XRES            512                     /* X resolution */
#define YRES            512                     /* Y Resolution */
#define YVISMAX         (2*targa.LinesPerField) /* Maximum visible Y coordinate */
#define YVISMIN         0                       /* Minimum visible Y coordiate */
#define DEF_ROWS        400                     /* Default number of rows */

#define IOBASE          targa.iobase            /* io base location of graphics registers */
#define MEMSEG          targa.memloc            /*  use the variable so we can use */
#define SCNSEG          targa.memloc            /*  the one defined in TARGA */
#define SRCBANK         (targa.memloc+0x0800)   /*  use high-bank as source bank */
#define DESTBANK        targa.memloc            /*  use lo-bank as destination bank */

/*      Output register definitions      */
#define MODEREG         (IOBASE+0xC00)  /* Mode Register address */
#define MASKREG         (IOBASE+0x800)  /* Mask Registers */
#define UNDERREG        (IOBASE+0x800)  /* Underscan register */
#define OVERREG         (IOBASE+0x802)  /* overscan register */
#define DESTREG         (IOBASE+0x802)  /* Address of Page Select Lower Register */
#define SRCREG          (IOBASE+0x803)  /* Address of Page Select Upper Register */
#define VCRCON          (IOBASE+0x400)  /* Address of Contrast/VidSrc Register */
#define BLNDREG         VCRCON
#define SATHUE          (IOBASE+0x402)  /* Satuation/Hue Register address */
#define DRREG           (IOBASE+0x401)  /* ADDRESS OF Controller Write Register */
#define VERTPAN         (IOBASE+0x403)  /* Address of Vertical Pan Register */
#define BORDER          (IOBASE)        /* Address of Page Select Lower Register */

/*      Input register definitions  */
#define VIDEOSTATUS     (IOBASE+0xC02)  /* Video Status Register */
#define RASTERREG       (IOBASE+0xC00)  /* Raster counter register */

/*      Default register values         */
#define DEF_MODE 1                      /* Default mode register value */
                                                                                                                        /*      Memory selected, 512x512, 1x */
                                                                                                                        /*      Display mode */
#define DEF_MASK        0               /* default memory mask */
#define DEF_SATURATION  0x4             /* default saturation value */
#define DEF_HUE         0x10            /* default hue value */
#define DEF_CONTRAST    0x10            /* default contrast value */
#define DEF_VIDSRC      0               /* default video source value - Composite */
#define DEF_VERTPAN     56              /* assumes 400-line output */
#define DEF_BORDER      0               /* default border color */


/*      MASK AND SHIFT VALUE FOR REGISTERS CONTAINING SUBFIELDS  */
/*
 *              ******************************************************
 *                                      MODE REGISTERS
 *              ******************************************************
 */
#define MSK_MSEL        0xfffC          /* memory select bits */
#define SHF_MSEL        0x0000
#define MSEL            1

#define MSK_IBIT        0xfffb          /* Interlace bit */
#define SHF_IBIT        2

#define MSK_RES         0xFFC7          /* disp. resolution and screen select bits */
#define SHF_RES         3
#define S0_512X512_0    0               /* 512x512 resolution screen */
#define S1_512X512_1    1
#define S2_512X256_0    2               /* 512x256 resolution screen 0 */
#define S3_512X256_1    3               /* 512x256 resolution screen 1 */
#define S4_256X256_0    4               /* 256x256 resolution screen 0 */
#define S5_256X256_1    5               /*                      ....            */
#define S6_256X256_2    6
#define S7_256X256_3    7

#define MSK_REGWRITE    0xFFBF          /* mask for display register write */
#define SHF_REGWRITE    6
#define REGINDEX        0               /* to write an index value */
#define REGVALUE        1               /* to write a value */

#define MSK_BIT9        0xFF7F          /* maks for high-order bit of DR's */
#define SHF_BIT9        7

#define MSK_TAPBITS     0xFCFF          /* mask for setting the tap bits */
#define SHF_TAPBITS     8

#define MSK_ZOOM        0xF3FF          /* Mask for zoom factor */
#define SHF_ZOOM        10

#define MSK_DISPLAY     0xCFFF          /* Mask for display mode */
#define SHF_DISPLAY     12
#define MEMORY_MODE     0
#define LIVE_FIXED      1
#define OVERLAY_MODE    2
#define LIVE_LIVE       3
#define DEF_DISPLAY     0

#define MSK_CAPTURE     0xBFFF          /* Mask for capture bit */
#define SHF_CAPTURE     14

#define MSK_GENLOCK     0x7FFF          /* MASK FOR GENLOCK */
#define SHF_GENLOCK     15
#define DEF_GENLOCK     0

/*      Video status input register      */
#define FIELDBIT        0x0001
#define VIDEOLOSS       0x0002

/*      VIDEO SOURCE/CONTROL REGISTER    */
#define MSK_CONTRAST    0xFFC1
#define SHF_CONTRAST    1
#define MAX_CONTRAST    0x1f

#define MSK_RGBORCV     0xBF
#define SHF_RGBORCV     6
#define RGB             1
#define CV              0

#define MSK_VCRORCAMERA 0x7F
#define SHF_VCRORCAMERA 7
#define VCR             1
#define CAMERA          0

/*      HUE/SATUATION REGISTER  */
#define MSK_HUE         0xE0
#define SHF_HUE         0
#define MAX_HUE         0x1f

#define MSK_SATURATION  0x1F
#define SHF_SATURATION  5
#define MAX_SATURATION  0x07


/*
 *      *********************************************
 *                      Display register settings
 *      *********************************************
 *
 *      Screen Positioning Registers:
 *              DR 0-3
 */
#define LEFTBORDER      0
#define DEF_LEFT        85
#define MIN_LEFT        75
#define MAX_LEFT        95
#define RIGHTBORDER     1
#define DEF_RIGHT       (DEF_LEFT+256)
#define TOPBORDER       2
#define DEF_TOP         40
#define MIN_TOP         20
#define BOTTOMBORDER    3
#define DEF_BOTTOM      (DEFTOP+DEFROWS/2)
#define MAX_BOTTOM      261

/*      REgisters which track 0-3        */
#define DR8             8
#define PRESHIFT        DR8
#define EQU_DR8         DR0
#define DR9             9
#define EQU_DR9         DR1
#define DR10            10
#define EQU_DR10        DR2
#define DR11            11
#define EQU_DR11        DR3

/*      REQUIRED REGISTERS      */
#define DR4             4
#define DEF_DR4         352
#define DR5             5
#define DEF_DR5         1
#define DR6             6
#define DEF_DR6         0
#define DR7             7
#define DEF_DR7         511
#define DR12            12
#define DEF_DR12        20
#define DR13            13
#define DEF_DR13        22
#define DR14            14
#define DEF_DR14        0
#define DR15            15
#define DEF_DR15        511
#define DR16            16
#define DEF_DR16        0
#define DR17            17
#define DEF_DR17        0
#define DR18            18
#define DEF_DR18        0
#define DR19            19
#define DEF_DR19        4

/* interlace mode register & parameters */
#define DR20            20
#define INTREG          0x14
#define DEF_INT         0               /* default to interlace mode 0 */
#define MSK_INTERLACE   0x0003

/**************************************************************/


typedef struct {
                        /*      Board Configuration */
        int             memloc;         /* memory segment */
        int             iobase;         /*  IOBASE segment */
        int             BytesPerPixel;  /* number of words per pixel */
        int             RowsPerBank;    /* number of row per 64K bank */
        int             MaxBanks;       /*   maximum bank id */
        int             AddressShift;   /*   number of bits to shift address */

                /*      Control registers */
        int             mode;           /*  mode register */
        int             Mask;           /*  mask register */
        int             PageMode;       /*  current page mode (screen res. and page) */
        unsigned        PageLower;      /*  Lower Page Select register */
        unsigned        PageUpper;      /*  upper Page select register */
        int             VCRCon;         /*  VCRContract register */
        int             SatHue;         /* Hue and Saturation register */
        long            BorderColor;    /*  Border color register */
        int             VertShift;      /*  Vertical Pan Register */
        int             PanXOrig, PanYOrig;     /* x,y pan origin */

                /* TARGA-SET PARAMETERS */
        int     boardType;              /*  See TYPE_XX  IN THIS FILE */
                /*  FOR DEFINITION OF Board Types */
        int     xOffset;                /*  X-offset */
        int     yOffset;                /*  Y-Offset */
        int     LinesPerField;          /*  maximum visible row count */
        int     InterlaceMode;          /*  desired interlace mode */
        int     AlwaysGenLock;          /*  Genlock always on or not  */
        int     Contrast;               /*  Desired Contrast */
        int     Hue;                    /*  Desired Hue */
        int     Saturation;             /*  Desired Satuation */
        int     RGBorCV;                /*  CV or RGB Input */
        int     VCRorCamera;            /*  VCR or Camera */
        int     ovrscnAvail, ovrscnOn;  /*  ovrscnAvail  1  if Overscan installed */
                                                                                                                        /*  ovrscnOn  1 if overscan is stretching */
                /*      Display Registers */
        int     DisplayRegister[22];

} TARStruct;


/****************************************************************/
/*  Data Definitions */

#ifdef TARGA_DATA
#               define  _x_
#               define  eq( val )       =val
#else
#               define  _x_     extern
#               define  eq( val )
#endif

_x_ int tseg            eq(     TSEG );
_x_ int tiobase eq(     TIOBASE );

_x_ TARStruct           targa;

#undef  _x_
#undef  eq

#endif

