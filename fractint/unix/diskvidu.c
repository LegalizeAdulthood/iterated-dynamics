/*
   "Disk-Video" for Unix routines.

   All the expanded memory caching stuff has been removed for the Unix
   version.  We just keep the data in memory and write it out to a file
   when we're done.  (Let virtual memory look after the details.)

*/

#include <stdio.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"

#define BOXROW	 6
#define BOXCOL	 11
#define BOXWIDTH 57
#define BOXDEPTH 12

#define TIMETODISPLAY 10000

int disk16bit=0;	   /* storing 16 bit values for continuous potential */

static int timetodisplay;
static FILE *fp = NULL;
int disktarga;

static int headerlength;
static unsigned int rowsize = 0;   /* doubles as a disk video not ok flag */
static unsigned int colsize;	   /* sydots, *2 when pot16bit */

static BYTE *dataPtr = NULL;

int startdisk(),pot_startdisk();
void enddisk();
int targa_startdisk(FILE *, int);
void targa_readdisk (unsigned int, unsigned int,
		     BYTE *, BYTE *, BYTE *);
void targa_writedisk(unsigned int, unsigned int,
		     BYTE, BYTE, BYTE);
void dvid_status(int,char *);

int made_dsktemp = 0;

int startdisk()
{
   headerlength = disktarga = 0;
   return (common_startdisk(sxdots,sydots,colors));
   }

int pot_startdisk()
{
   int i;
   if (dotmode == 11) /* ditch the original disk file */
      enddisk();
   else
      showtempmsg("clearing 16bit pot work area");
   headerlength = disktarga = 0;
   i = common_startdisk(sxdots,sydots<<1,colors);
   cleartempmsg();
   disk16bit = 1;
   return (i);
   }

int targa_startdisk(FILE *targafp,int overhead)
{
   int i;
   if (dotmode == 11) { /* ditch the original disk file, make just the targa */
      enddisk();      /* close the 'screen' */
      setnullvideo(); /* set readdot and writedot routines to do nothing */
      }
   headerlength = overhead;
   fp = targafp;
   disktarga = 1;
   i = common_startdisk(sxdots*3,sydots,colors);
   return (i);
}

int _fastcall  common_startdisk(long newrowsize, long newcolsize, int colors)
{
   int i;
   long memorysize;

   if (g_disk_flag)
      enddisk();
   if (dotmode == 11) { /* otherwise, real screen also in use, don't hit it */
      char buf[20];
      helptitle();
      setattr(1,0,C_DVID_BKGRD,24*80);	/* init rest to background */
      for (i = 0; i < BOXDEPTH; ++i)
	 setattr(BOXROW+i,BOXCOL,C_DVID_LO,BOXWIDTH);  /* init box */
      putstring(BOXROW+2,BOXCOL+4,C_DVID_HI,"'Disk-Video' mode");
      putstring(BOXROW+4,BOXCOL+4,C_DVID_LO,"Screen resolution: ");
      sprintf(buf,"%d x %d",sxdots,sydots);
      putstring(-1,-1,C_DVID_LO,buf);
      if (disktarga)
	 putstring(-1,-1,C_DVID_LO,"  24 bit Targa");
      else {
	 putstring(-1,-1,C_DVID_LO,"  Colors: ");
	 sprintf(buf,"%d",colors);
	 putstring(-1,-1,C_DVID_LO,buf);
	 }
      putstring(BOXROW+8,BOXCOL+4,C_DVID_LO,"Status:");
      dvid_status(0,"clearing the 'screen'");
      }
   timetodisplay = TIMETODISPLAY;  /* time-to-display-status counter */

   memorysize = (long)(newcolsize) * newrowsize;
   g_disk_flag = 1;
   rowsize = newrowsize;
   colsize = newcolsize;

   if (dataPtr != NULL) {
       free(dataPtr);
   }
   dataPtr = (BYTE *)malloc(memorysize);

  bzero(dataPtr,memorysize);

/* common_okend: */
   if (dotmode == 11)
      dvid_status(0,"");
   return(0);
}

void enddisk()
{
   g_disk_flag = rowsize = disk16bit = 0;
   fp	       = NULL;
}

int readdisk(int col, int row)
{
   char buf[41];
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," reading line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = TIMETODISPLAY;
      }
   if (row>=colsize || col>=rowsize) {
       return 0;
   }
   return dataPtr[row*rowsize+col];
}

int FromMemDisk(long offset, int size, void *dest)
{
   memcpy(dest, (void *) (dataPtr+offset), size);
   return 1;
}

void targa_readdisk(unsigned int col, unsigned int row,
		    BYTE *red, BYTE *green, BYTE *blue)
{
   col *= 3;
   *blue  = readdisk(col,row);
   *green = readdisk(++col,row);
   *red   = readdisk(col+1,row);
}

void writedisk(int col, int row, int color)
{
   char buf[41];
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," writing line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = TIMETODISPLAY;
      }
   if (row>=colsize || col>=rowsize) {
       return;
   }
   dataPtr[row*rowsize+col] = color;
}

int ToMemDisk(long offset, int size, void *src)
{
    memcpy((void *) (dataPtr+offset), src, size);
    return 1;
}

void targa_writedisk(unsigned int col, unsigned int row,
		    BYTE red, BYTE green, BYTE blue)
{
   writedisk(col*=3,row,blue);
   writedisk(++col, row,green);
   writedisk(col+1, row,red);
}

void dvid_status(int line,char *msg)
{
   char buf[41];
   int attrib;
   memset(buf,' ',40);
   memcpy(buf,msg,strlen(msg));
   buf[40] = 0;
   attrib = C_DVID_HI;
   if (line >= 100) {
      line -= 100;
      attrib = C_STOP_ERR;
      }
   putstring(BOXROW+8+line,BOXCOL+12,attrib,buf);
   movecursor(25,80);
}
