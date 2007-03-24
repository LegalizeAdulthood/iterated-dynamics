/*

		Roll-Your-Own video mode (DOTMODE 19) routines.

Even if you don't have an assembler, you can add your own video-mode
routines to FRACTINT by adding a video mode of the appropriate resolution
to FRACTINT.CFG that uses dotmode 19 (which calls these routines to
perform all the dirty work) and modifying these routines accordingly.
The four routines are:

 startvideo()    Do whatever you have to do to throw your adapter into
                 the appropriate video mode (in case it can't be accomplished
                 the "normal" way, with INT 10H and the AX/BX/CX/DX values
                 available via FRACTINT.CFG or FARVIDEO.ASM).  This routine
                 will typically be empty (in which case the AX/BX/CX/DX values
                 in FRACTINT.CFG or FARVIDEO.ASM must be encoded appropriately
                 to accomplish the task), but some adapters like the 8514/A
                 and TARGA need special handling which would go here.
                 If you DO have to put something here, you should encode
                 AX = 0xFF so as to effectively convert the regular
                 video-switching code inside VIDEO.ASM to use
                 an invalid INT 10H call - "do-nothing" logic.

 endvideo()      do whatever you have to do to get it out of that
                 special video mode (in case 'setvideo(3,0,0,0)'
                 won't do it) - this routine will typically be empty,
                 but some adapters like the 8514/A and TARGA need
                 special handling which would go here.

 writevideo(int x, int y, int color)  write a pixel using color number
                 'color' at screen coordinates x,y (where 0,0 is the
                 top left corner, and sxdots,0 is the top right corner)

 int readvideo(int x, int y)  return the color number of pixel x,y
                 using the same coordinate logic as 'writevideo()'

 int readvideopalette() read the contents of the adapter's video
                 palette into the 'BYTE dacbox[256][3]' array
                 (up to 256 R/G/B triplets, each with values from 0 to 63).
                 Set dacbox[0][0] = 255 if there is no such palette.
                 Return a -1 if you want the normal internal EGA/VGA
                 routines to handle this function.

 int writevideopalette() write the contents of the adapter's video
                 palette from the 'BYTE dacbox[256][3]' array
                 (up to 256 R/G/B triplets, each with values from 0 to 63).
                 Return a -1 if you want the normal internal EGA/VGA
                 routines to handle this function.

Finally, note that, although these example routines are written in "C",
they could just as easily (or maybe more easily!) have been written
in assembler.

*/

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

/* external variables (set in the FRACTINT.CFG file, but findable here */
/* these are declared in PROTOTYPE.H */

#if 0
	int  dotmode;                /* video access method (= 19)      */
	int  sxdots, sydots;         /* total # of dots on the screen   */
	int  colors;                 /* maximum colors available        */

/* the video-palette array (named after the VGA adapter's video-DAC) */

	BYTE dacbox[256][3];

#endif

/* for demo purposes, these routines use VGA mode 13h - 320x200x256 */

int startvideo()
{

/* assume that the encoded values in FRACTINT.CFG or FARVIDEO.ASM
	have been set to accomplish this (AX = 0x13, BX = CX = DX = 0)  */

return 0;                              /* set flag: video started */

/*   or, we could have done this instead and encoded AX = 0xFF
     in FRACTINT.CFG/FARVIDEO.ASM:

union REGS regs;

regs.x.ax = 0x13;
int86(0x10,&regs,&regs);

*/

}

int endvideo()
{

return 0;                              /* set flag: video ended */

}

void writevideo(int x, int y, int color)
{
#if !defined(_WIN32)
union REGS regs;

regs.h.ah = 0x0c;                       /* invoke INT 10H with AH = 0CH */
regs.h.al = (char)color;
regs.x.bx = 0;
regs.x.cx = x;
regs.x.dx = y;
int86(0x10,&regs,&regs);
#endif
}

int readvideo(int x, int y)
{
#if defined(_WIN32)
	return -1;
#else
union REGS regs;

regs.x.ax = 0x0d00;                     /* invoke INT 10H with AH = 0DH */
regs.x.bx = 0;
regs.x.cx = x;
regs.x.dx = y;
int86(0x10,&regs,&regs);

return (unsigned int)regs.h.al;        /* return pixel color */
#endif
}

int readvideopalette(void)
{

return (-1);                            /* let the internal routines do it */

}

int writevideopalette(void)
{

return (-1);                            /* let the internal routines do it */

}
