#ifndef TARGA_LC_H
#define TARGA_LC_H

#define HEADERSIZE 18           /* Size, offsets, and masks for the */
#define O_COMMENTLEN 0          /* TGA file header.  This is not a      */
#define O_MAPTYPE 1                     /* structure to avoid problems with     */
#define O_FILETYPE 2            /* byte-packing and such.                       */
#define O_MAPORG 3
#define O_MAPLEN 5
#define O_MAPSIZE 7
#define O_XORIGIN 8
#define O_YORIGIN 10
#define O_HSIZE 12
#define O_VSIZE 14
#define O_ESIZE 16
#define O_FLAGS 17
#define M_ORIGIN 0x20

#define T_NODATA 0
#define T_RAWMAP 1
#define T_RAWRGB 2
#define T_RAWMON 3
#define T_RLEMAP 9
#define T_RLERGB 10
#define T_RLEMON 11

#endif
