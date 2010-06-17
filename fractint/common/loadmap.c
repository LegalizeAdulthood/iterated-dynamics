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

void get_map_name( char * from_str, char * to_str)
{ /* called once we know from_str contains ".map" */
  /* returns xxxxxxxx.map in to_str */
char   *temp_map_name, *dot_position;
unsigned int position, name_length;

        temp_map_name = strstr(from_str,".map");
        dot_position = temp_map_name;
        while(*temp_map_name != ' ' &&  /* find space before map name */
              *temp_map_name != '@')    /* find @ symbol before map name */
           temp_map_name--;
        temp_map_name++;                           /* start of map name */
        name_length = (unsigned int)(dot_position - temp_map_name + 4);
        for( position = 0; position < (name_length); position++)
           to_str[position] = *temp_map_name++;
        to_str[position] = '\0';           /* add NULL termination */
}

int ValidateLuts( char * fn )
{
FILE * f;
unsigned        r, g, b, index;
char    line[160];
char    temp[FILE_MAX_PATH+1];
char    temp_fn[FILE_MAX_PATH];
char   *dummy; /* to quiet compiler */

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
        dummy = fgets(line,100,f);
        if (strstr(line,".map") != NULL) { /* found a map name */
                get_map_name(line, fn);
                }
        sscanf( line, "%u %u %u", &r, &g, &b );
        /** load global dac values **/
        dac[0].red   = (BYTE)((r%256) >> 2);/* maps default to 8 bits */
        dac[0].green = (BYTE)((g%256) >> 2);/* DAC wants 6 bits */
        dac[0].blue  = (BYTE)((b%256) >> 2);
        for( index = 1; index < 256; index++ ) {
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
                far_strcpy(msg,o_msg);
                stopmsg(0,msg);
                return 1;
                }
        far_memcpy((char far *)mapdacbox,(char far *)dacbox,768);
        /* PB, 900829, removed atexit(RestoreMap) stuff, goodbye covers it */
        return 0;
}

