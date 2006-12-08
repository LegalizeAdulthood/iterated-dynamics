/** loadmap.c **/


#include        <string.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

/***************************************************************************/

#define dac ((Palettetype *)dacbox)

#ifndef WINFRACT
void SetTgaColors() {
unsigned        r, g, b, index;
    if (tga16 != NULL)
        for( index = 0; index < 256; index++ ) {
                r = dac[index].red      << 2;
                g = dac[index].green << 2;
                b = dac[index].blue     << 2;
                tga16[index] = ((r&248)<<7) | ((g&248)<<2) | (b>>3);
                tga32[index] = ((long)r<<16) | (g<<8) | b;
        }
}
#endif

int ValidateLuts( char * fn )
{
FILE * f;
unsigned        r, g, b, index;
char    line[160];
char    temp[FILE_MAX_PATH+1];
char    temp_fn[FILE_MAX_PATH];
        strcpy(temp,MAP_name);
        strcpy(temp_fn,fn);
#ifdef XFRACT
        merge_pathnames(temp,temp_fn,3);
#else
        merge_pathnames(temp,temp_fn,0);
#endif
        if (has_ext(temp) == NULL) /* Did name have an extension? */
                strcat(temp,".map");  /* No? Then add .map */
        findpath( temp, line);        /* search the dos path */
        f = fopen( line, "r" );
        if (f == NULL) {
                sprintf(line,"Could not load color map %s",fn);
                stopmsg(0,line);
                return 1;
                }
        for( index = 0; index < 256; index++ ) {
                if (fgets(line,100,f) == NULL)
                        break;
                sscanf( line, "%u %u %u", &r, &g, &b );
                /** load global dac values **/
                dac[index].red   = (BYTE)((r%256) >> 2);/* maps default to 8 bits */
                dac[index].green = (BYTE)((g%256) >> 2);/* DAC wants 6 bits */
                dac[index].blue  = (BYTE)((b%256) >> 2);
        }
        fclose( f );
        while (index < 256)  { /* zap unset entries */
                dac[index].red = dac[index].blue = dac[index].green = 40;
                ++index;
        }
        SetTgaColors();
        colorstate = 2;
        strcpy(colorfile,fn);
        return 0;
}


/***************************************************************************/

int SetColorPaletteName( char * fn )
{
        if( ValidateLuts( fn ) != 0)
                return 1;
        if( mapdacbox == NULL && (mapdacbox = (char far *)farmemalloc(768L)) == NULL) {
                static FCODE o_msg[]={"Insufficient memory for color map."};
                char msg[sizeof(o_msg)];
                strcpy(msg,o_msg);
                stopmsg(0,msg);
                return 1;
                }
        memcpy((char far *)mapdacbox,(char far *)dacbox,768);
        /* PB, 900829, removed atexit(RestoreMap) stuff, goodbye covers it */
        return 0;
}

