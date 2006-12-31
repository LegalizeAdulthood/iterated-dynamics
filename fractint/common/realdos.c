/*
        Miscellaneous C routines used only in DOS Fractint.
*/

#include <string.h>
#ifndef XFRACT
#include <io.h>
#include <process.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

static int menu_checkkey(int curkey,int choice);

/* uncomment following for production version */
/*
#define PRODUCTION
*/
int g_release = 2100;	/* this has 2 implied decimals; increment it every synch */
int g_patchlevel = 0;	/* patchlevel for DOS version */
#ifdef XFRACT
int xrelease=304;
#endif

/* int stopmsg(flags,message) displays message and waits for a key:
     message should be a max of 9 lines with \n's separating them;
       no leading or trailing \n's in message;
       no line longer than 76 chars for best appearance;
     flag options:
       &1 if already in text display mode, stackscreen is not called
          and message is displayed at (12,0) instead of (4,0)
       &2 if continue/cancel indication is to be returned;
          when not set, "Any key to continue..." is displayed
          when set, "Escape to cancel, any other key to continue..."
          -1 is returned for cancel, 0 for continue
       &4 set to suppress buzzer
       &8 for Fractint for Windows & parser - use a fixed pitch font
      &16 for info only message (green box instead of red in DOS vsn)
   */
#ifdef XFRACT
static char s_errorstart[] = {"*** Error during startup:"};
#endif
static char s_escape_cancel[] = {"Escape to cancel, any other key to continue..."};
static char s_anykey[] = {"Any key to continue..."};
#if !defined(PRODUCTION) && !defined(XFRACT)
static char s_custom[] = {"Customized Version"};
static char s_notpublic[] = {"Not for Public Release"};
static char s_incremental[] = {"Incremental release"};
#endif
int stopmsg (int flags, char *msg)
{
   int ret,toprow,color,savelookatmouse;
   static unsigned char batchmode = 0;
   if(debugflag != 0 || initbatch >= 1)
   {
      static FILE *fp = NULL;
      if(fp==NULL && initbatch == 0)
         fp=dir_fopen(workdir,"stopmsg.txt","w");
      else
         fp=dir_fopen(workdir,"stopmsg.txt","a");
      if(fp != NULL)
#ifndef XFRACT
      fprintf(fp,"%Fs\n",msg);
#else
      fprintf(fp,"%s\n",msg);
#endif
      fclose(fp);
   }
   if (active_system == 0 /* DOS */
     && first_init) {     /* & cmdfiles hasn't finished 1st try */
#ifdef XFRACT
      driver_set_for_text();
      driver_buzzer(2);
      driver_put_string(0,0,15,s_errorstart);
      driver_put_string(2,0,15,msg);
      driver_move_cursor(8,0);
#if !defined(WIN32)
      sleep(1);
#endif
      close_drivers();
      exit(1);
#else
      printf("%Fs\n",msg);
      dopause(1); /* pause deferred until after cmdfiles */
      return(0);
#endif
      }
   if (initbatch >= 1 || batchmode) { /* in batch mode */
      initbatch = 4; /* used to set errorlevel */
      batchmode = 1; /* fixes *second* stopmsg in batch mode bug */
      return (-1);
      }
   ret = 0;
   savelookatmouse = lookatmouse;
   lookatmouse = -13;
   if ((flags & STOPMSG_NO_STACK))
      blankrows(toprow=12,10,7);
   else {
      driver_stack_screen();
      toprow = 4;
      driver_move_cursor(4,0);
      }
   g_text_cbase = 2; /* left margin is 2 */
   driver_put_string(toprow,0,7,msg);
   if (flags & STOPMSG_CANCEL)
      driver_put_string(g_text_row+2,0,7,s_escape_cancel);
   else
      driver_put_string(g_text_row+2,0,7,s_anykey);
   g_text_cbase = 0; /* back to full line */
   color = (flags & STOPMSG_INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
   driver_set_attr(toprow,0,color,(g_text_row+1-toprow)*80);
   driver_hide_text_cursor();   /* cursor off */
   if ((flags & STOPMSG_NO_BUZZER) == 0)
      driver_buzzer((flags & 16) ? 0 : 2);
   while (driver_key_pressed()) /* flush any keyahead */
      driver_get_key();
   if(debugflag != 324)
      if (getakeynohelp() == FIK_ESC)
         ret = -1;
   if ((flags & STOPMSG_NO_STACK))
      blankrows(toprow,10,7);
   else
      driver_unstack_screen();
   lookatmouse = savelookatmouse;
   return ret;
}


static U16 temptextsave = 0;
static int  textxdots,textydots;

/* texttempmsg(msg) displays a text message of up to 40 characters, waits
      for a key press, restores the prior display, and returns (without
      eating the key).
      It works in almost any video mode - does nothing in some very odd cases
      (HCGA hi-res with old bios), or when there isn't 10k of temp mem free. */
int texttempmsg(char *msgparm)
{
   if (showtempmsg(msgparm))
      return(-1);
   driver_wait_key_pressed(0); /* wait for a keystroke but don't eat it */
   cleartempmsg();
   return(0);
}

void freetempmsg()
{
   if(temptextsave != 0)
      MemoryRelease(temptextsave);
   temptextsave = 0;
}

int showtempmsg(char *msgparm)
{
   static long size = 0;
   char msg[41];
   BYTE buffer[640];
   BYTE *fontptr;
   BYTE *bufptr;
   int i,j,k,fontchar,charnum;
   int xrepeat = 0;
   int yrepeat = 0;
   int save_sxoffs,save_syoffs;
   strncpy(msg,msgparm,40);
   msg[40] = 0; /* ensure max message len of 40 chars */
   if (driver_diskp()) { /* disk video, screen in text mode, easy */
      dvid_status(0,msg);
      return(0);
      }
   if (active_system == 0 /* DOS */
     && first_init) {     /* & cmdfiles hasn't finished 1st try */
      printf("%s\n",msg);
      return(0);
      }

   if ((fontptr = driver_find_font(0)) == NULL) { /* old bios, no font table? */
      if (g_ok_to_print == 0               /* can't printf */
        || sxdots > 640 || sydots > 200) /* not willing to trust char cell size */
         return(-1); /* sorry, message not displayed */
      textydots = 8;
      textxdots = sxdots;
      }
   else {
      xrepeat = (sxdots >= 640) ? 2 : 1;
      yrepeat = (sydots >= 300) ? 2 : 1;
      textxdots = (int) strlen(msg) * xrepeat * 8;
      textydots = yrepeat * 8;
      }
   /* worst case needs 10k */
   if(temptextsave != 0)
      if(size != (long)textxdots * (long)textydots)
         freetempmsg();
   size = (long)textxdots * (long)textydots;
   save_sxoffs = sxoffs;
   save_syoffs = syoffs;
   if (g_video_scroll) {
      sxoffs = g_video_start_x;
      syoffs = g_video_start_y;
   }
   else
      sxoffs = syoffs = 0;
   if(temptextsave == 0) /* only save screen first time called */
   {
		/* TODO: MemoryAlloc, MoveToMemory */
      if ((temptextsave = MemoryAlloc((U16)textxdots,(long)textydots,MEMORY)) == 0)
         return(-1); /* sorry, message not displayed */
      for (i = 0; i < textydots; ++i) {
         get_line(i,0,textxdots-1,buffer);
         MoveToMemory(buffer,(U16)textxdots,1L,(long)i,temptextsave);
         }
      }
   if (fontptr == NULL) { /* bios must do it for us */
      home();
      printf(msg);
      }
   else { /* generate the characters */
      find_special_colors(); /* get g_color_dark & g_color_medium set */
      for (i = 0; i < 8; ++i) {
         memset(buffer,g_color_dark,640);
         bufptr = buffer;
         charnum = -1;
         while (msg[++charnum] != 0) {
            fontchar = *(fontptr + msg[charnum]*8 + i);
            for (j = 0; j < 8; ++j) {
               for (k = 0; k < xrepeat; ++k) {
                  if ((fontchar & 0x80) != 0)
                     *bufptr = (BYTE)g_color_medium;
                  ++bufptr;
                  }
               fontchar <<= 1;
               }
            }
         for (j = 0; j < yrepeat; ++j)
            put_line(i*yrepeat+j,0,textxdots-1,buffer);
         }
      }
   sxoffs = save_sxoffs;
   syoffs = save_syoffs;
   return(0);
}

void cleartempmsg()
{
   BYTE buffer[640];
   int i;
   int save_sxoffs,save_syoffs;
   if (driver_diskp()) /* disk video, easy */
      dvid_status(0,"");
   else if (temptextsave != 0) {
      save_sxoffs = sxoffs;
      save_syoffs = syoffs;
      if (g_video_scroll) {
         sxoffs = g_video_start_x;
         syoffs = g_video_start_y;
      }
      else
         sxoffs = syoffs = 0;
      for (i = 0; i < textydots; ++i) {
         MoveFromMemory(buffer,(U16)textxdots,1L,(long)i,temptextsave);
         put_line(i,0,textxdots-1,buffer);
         }
     if(using_jiim == 0)  /* jiim frees memory with freetempmsg() */
     {
         MemoryRelease(temptextsave);
         temptextsave = 0;
      }
      sxoffs = save_sxoffs;
      syoffs = save_syoffs;
      }
}


void blankrows(int row,int rows,int attr)
{
   char buf[81];
   memset(buf,' ',80);
   buf[80] = 0;
   while (--rows >= 0)
      driver_put_string(row++,0,attr,buf);
}

void helptitle()
{
   char msg[MSGLEN],buf[MSGLEN];
   driver_set_clear(); /* clear the screen */
#ifdef XFRACT
   sprintf(msg,"XFRACTINT  Version %d.%02d (FRACTINT Version %d.%02d)",
           xrelease/100,xrelease%100, g_release/100,g_release%100);
#else
   *msg=0;
#endif   
   sprintf(buf,"FRACTINT Version %d.%01d",g_release/100,(g_release%100)/10);
   strcat(msg,buf);
   if (g_release%10) {
      sprintf(buf,"%01d",g_release%10);
      strcat(msg,buf);
      }
   if (g_patchlevel) {
      sprintf(buf,".%d",g_patchlevel);
      strcat(msg,buf);
      }
   putstringcenter(0,0,80,C_TITLE,msg);
   
/* uncomment next for production executable: */
#if defined(PRODUCTION) || defined(XFRACT)
    return;
   /*NOTREACHED*/
#else
   if (debugflag == 3002) return;
#define DEVELOPMENT
#ifdef DEVELOPMENT
   driver_put_string(0,2,C_TITLE_DEV,"Development Version");
#else
   driver_put_string(0,3,C_TITLE_DEV, s_custom);
#endif
   driver_put_string(0,55,C_TITLE_DEV,s_notpublic);
#endif
}


void footer_msg(int *i, int options, char *speedstring)
{
   static char choiceinstr1a[]="Use the cursor keys to highlight your selection";
   static char choiceinstr1b[]="Use the cursor keys or type a value to make a selection";
   static char choiceinstr2a[]="Press ENTER for highlighted choice, or ESCAPE to back out";
   static char choiceinstr2b[]="Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help";
   static char choiceinstr2c[]="Press ENTER for highlighted choice, or "FK_F1" for help";
   putstringcenter((*i)++,0,80,C_PROMPT_BKGRD,
      (speedstring) ? choiceinstr1b : choiceinstr1a);
   putstringcenter(*(i++),0,80,C_PROMPT_BKGRD,
         (options&CHOICE_MENU) ? choiceinstr2c
      : ((options&CHOICE_HELP) ? choiceinstr2b : choiceinstr2a));
}

int putstringcenter(int row, int col, int width, int attr, char *msg)
{
   char buf[81];
   int i,j,k;
   i = 0;
#ifdef XFRACT
   if (width>=80) width=79; /* Some systems choke in column 80 */
#endif
   while (msg[i]) ++i; /* strlen for a */
   if (i == 0) return(-1);
   if (i >= width) i = width - 1; /* sanity check */
   j = (width - i) / 2;
   j -= (width + 10 - i) / 20; /* when wide a bit left of center looks better */
   memset(buf,' ',width);
   buf[width] = 0;
   i = 0;
   k = j;
   while (msg[i]) buf[k++] = msg[i++]; /* strcpy for a */
   driver_put_string(row,col,attr,buf);
   return j;
}

/* ------------------------------------------------------------------------ */

char speed_prompt[]="Speed key string";

/* For file list purposes only, it's a directory name if first 
   char is a dot or last char is a slash */
static int isadirname(char *name)
{
   if(*name == '.' || endswithslash(name))
      return 1;
   else
      return 0;
}

void show_speedstring(int speedrow,
                   char *speedstring,
                   int (*speedprompt)(int,int,int,char *,int))
{
   int speed_match = 0;
   int i,j;
   char buf[81];
   memset(buf,' ',80);
   buf[80] = 0;
   driver_put_string(speedrow,0,C_PROMPT_BKGRD,buf);
   if (*speedstring) {                 /* got a speedstring on the go */
      driver_put_string(speedrow,15,C_CHOICE_SP_INSTR," ");
      if (speedprompt)
         j = speedprompt(speedrow,16,C_CHOICE_SP_INSTR,speedstring,speed_match);
      else {
         driver_put_string(speedrow,16,C_CHOICE_SP_INSTR,speed_prompt);
         j = sizeof(speed_prompt)-1;
         }
      strcpy(buf,speedstring);
      i = (int) strlen(buf);
      while (i < 30)
         buf[i++] = ' ';
      buf[i] = 0;
      driver_put_string(speedrow,16+j,C_CHOICE_SP_INSTR," ");
      driver_put_string(speedrow,17+j,C_CHOICE_SP_KEYIN,buf);
      driver_move_cursor(speedrow,17+j+(int) strlen(speedstring));
      }
   else
      driver_hide_text_cursor();
}

void process_speedstring(char    *speedstring,
                        char **choices,         /* array of choice strings                */
                        int       curkey,
                        int      *pcurrent,
                        int       numchoices,
                        int       is_unsorted)
{
   int i, comp_result;

   i = (int) strlen(speedstring);
   if (curkey == 8 && i > 0) /* backspace */
      speedstring[--i] = 0;
   if (33 <= curkey && curkey <= 126 && i < 30)
   {
#ifndef XFRACT
      curkey = tolower(curkey);
#endif
      speedstring[i] = (char)curkey;
      speedstring[++i] = 0;
   }
   if (i > 0)  {    /* locate matching type */
      *pcurrent = 0;
      while (*pcurrent < numchoices
        && (comp_result = strncasecmp(speedstring,choices[*pcurrent],i))!=0) {
         if (comp_result < 0 && !is_unsorted) {
            *pcurrent -= *pcurrent ? 1 : 0;
            break;
         }
         else
           ++*pcurrent;
      }
      if (*pcurrent >= numchoices) /* bumped end of list */
         *pcurrent = numchoices - 1;
            /*if the list is unsorted, and the entry found is not the exact
              entry, then go looking for the exact entry.
            */
      else if (is_unsorted && choices[*pcurrent][i]) {
         int temp = *pcurrent;
         while(++temp < numchoices) {
            if (!choices[temp][i] && !strncasecmp(speedstring, choices[temp], i)) {
               *pcurrent = temp;
               break;
            }
         }
      }
   }
}


int fullscreen_choice(
    int options,                  /* &2 use menu coloring scheme            */
                                  /* &4 include F1 for help in instructions */
                                  /* &8 add caller's instr after normal set */
                                  /* &16 menu items up one line             */
    char *hdg,                /* heading info, \n delimited             */
    char *hdg2,               /* column heading or NULL                 */
    char *instr,              /* instructions, \n delimited, or NULL    */
    int numchoices,               /* How many choices in list               */
    char **choices,         /* array of choice strings                */
    int *attributes,          /* &3: 0 normal color, 1,3 highlight      */
                                  /* &256 marks a dummy entry               */
    int boxwidth,                 /* box width, 0 for calc (in items)       */
    int boxdepth,                 /* box depth, 0 for calc, 99 for max      */
    int colwidth,                 /* data width of a column, 0 for calc     */
    int current,                  /* start with this item                   */
    void (*formatitem)(int,char*),/* routine to display an item or NULL     */
    char *speedstring,            /* returned speed key value, or NULL      */
    int (*speedprompt)(int,int,int,char *,int),/* routine to display prompt or NULL      */
    int (*checkkey)(int,int)      /* routine to check keystroke or NULL     */
)
    /* return is: n>=0 for choice n selected,
                  -1 for escape
                  k for checkkey routine return value k (if not 0 nor -1)
                  speedstring[0] != 0 on return if string is present
    */
{

   int titlelines,titlewidth;
   int reqdrows;
   int topleftrow,topleftcol;
   int topleftchoice;
   int speedrow = 0;  /* speed key prompt */
   int boxitems;      /* boxwidth*boxdepth */
   int curkey,increment,rev_increment = 0;
   int redisplay;
   int i,j,k = 0;
   char *charptr;
   char buf[81];
   char curitem[81];
   char *itemptr;
   int ret,savelookatmouse;
   int scrunch;  /* scrunch up a line */

   if(options&CHOICE_CRUNCH)
      scrunch = 1;
   else
      scrunch = 0;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   ret = -1;
   if (speedstring
     && (i = (int) strlen(speedstring)) > 0) { /* preset current to passed string */
      current = 0;
      if(options&CHOICE_NOT_SORTED)
      {
         while (current < numchoices
             && (k = strncasecmp(speedstring,choices[current],i)) != 0)
            ++current;
         if(k != 0)
            current = 0;
      }
      else
      {
         while (current < numchoices
             && (k = strncasecmp(speedstring,choices[current],i)) > 0)
            ++current;
         if (k < 0 && current > 0)  /* oops - overshot */
            --current;
      }
      if (current >= numchoices) /* bumped end of list */
         current = numchoices - 1;
   }

   for(;;) {
      if (current >= numchoices)  /* no real choice in the list? */
         goto fs_choice_end;
      if ((attributes[current] & 256) == 0)
         break;
      ++current;                  /* scan for a real choice */
      }

   titlelines = titlewidth = 0;
   if (hdg) {
      charptr = hdg;              /* count title lines, find widest */
      i = 0;
      titlelines = 1;
      while (*charptr) {
         if (*(charptr++) == '\n') {
            ++titlelines;
            i = -1;
            }
         if (++i > titlewidth)
            titlewidth = i;
         }
      }

   if (colwidth == 0)             /* find widest column */
      for (i = 0; i < numchoices; ++i)
      {
         int len;
         if ((len=(int) strlen(choices[i])) > colwidth)
            colwidth = len;
      }
   /* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
   reqdrows = 3 - scrunch;                /* calc rows available */
   if (hdg)
      reqdrows += titlelines + 1;
   if (instr) {                   /* count instructions lines */
      charptr = instr;
      ++reqdrows;
      while (*charptr)
         if (*(charptr++) == '\n')
            ++reqdrows;
      if ((options & 8))          /* show std instr too */
         reqdrows += 2;
      }
   else
      reqdrows += 2;              /* standard instructions */
   if (speedstring) ++reqdrows;   /* a row for speedkey prompt */
   if (boxdepth > (i = 25 - reqdrows)) /* limit the depth to max */
      boxdepth = i;
   if (boxwidth == 0) {           /* pick box width and depth */
      if (numchoices <= i - 2) {  /* single column is 1st choice if we can */
         boxdepth = numchoices;
         boxwidth = 1;
         }
      else {                      /* sort-of-wide is 2nd choice */
         boxwidth = 60 / (colwidth + 1);
         if (boxwidth == 0
           || (boxdepth = (numchoices+boxwidth-1)/boxwidth) > i - 2) {
            boxwidth = 80 / (colwidth + 1); /* last gasp, full width */
            if ((boxdepth = (numchoices+boxwidth-1)/boxwidth) > i)
               boxdepth = i;
            }
         }
      }
#if 0
   if ((i = 77 / boxwidth - colwidth) > 3) /* spaces to add @ left each choice */
      i = 3;
   if (i == 0)
      i = 1;
#else
   if ((i = (80 / boxwidth - colwidth) / 2 - 1) == 0) /* to allow wider prompts */
      i = 1;
   if (i < 0)
      i = 0;
   if (i > 3)
      i = 3;
#endif
   j = boxwidth * (colwidth += i) + i;     /* overall width of box */
   if (j < titlewidth+2)
      j = titlewidth + 2;
   if (j > 80)
      j = 80;
   if (j <= 70 && boxwidth == 2) {         /* special case makes menus nicer */
      ++j;
      ++colwidth;
      }
   k = (80 - j) / 2;                       /* center the box */
   k -= (90 - j) / 20;
   topleftcol = k + i;                     /* column of topleft choice */
   i = (25 - reqdrows - boxdepth) / 2;
   i -= i / 4;                             /* higher is better if lots extra */
   topleftrow = 3 + titlelines + i;        /* row of topleft choice */

   /* now set up the overall display */
   helptitle();                            /* clear, display title line */
   driver_set_attr(1,0,C_PROMPT_BKGRD,24*80);      /* init rest to background */
   for (i = topleftrow-1-titlelines; i < topleftrow+boxdepth+1; ++i)
      driver_set_attr(i,k,C_PROMPT_LO,j);          /* draw empty box */
   if (hdg) {
      g_text_cbase = (80 - titlewidth) / 2;   /* set left margin for putstring */
      g_text_cbase -= (90 - titlewidth) / 20; /* put heading into box */
      driver_put_string(topleftrow-titlelines-1,0,C_PROMPT_HI,hdg);
      g_text_cbase = 0;
      }
   if (hdg2)                               /* display 2nd heading */
      driver_put_string(topleftrow-1,topleftcol,C_PROMPT_MED,hdg2);
   i = topleftrow + boxdepth + 1;
   if (instr == NULL || (options & 8)) {   /* display default instructions */
      if (i < 20) ++i;
      if (speedstring) {
         speedrow = i;
         *speedstring = 0;
         if (++i < 22) ++i;
         }
      i -= scrunch;
      footer_msg(&i,options,speedstring);
      }
   if (instr) {                            /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)) != 0)
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }

   boxitems = boxwidth * boxdepth;
   topleftchoice = 0;                      /* pick topleft for init display */
   while (current - topleftchoice >= boxitems
     || (current - topleftchoice > boxitems/2
         && topleftchoice + boxitems < numchoices))
      topleftchoice += boxwidth;
   redisplay = 1;
   topleftrow -= scrunch;
   for(;;) { /* main loop */
      if (redisplay) {                       /* display the current choices */
         if ((options & CHOICE_MENU) == 0) {
            memset(buf,' ',80);
            buf[boxwidth*colwidth] = 0;
            for (i = (hdg2) ? 0 : -1; i <= boxdepth; ++i)  /* blank the box */
               driver_put_string(topleftrow+i,topleftcol,C_PROMPT_LO,buf);
            }
         for (i = 0; i+topleftchoice < numchoices && i < boxitems; ++i) {
            /* display the choices */
            if ((k = attributes[j = i+topleftchoice] & 3) == 1)
               k = C_PROMPT_LO;
            else if (k == 3)
               k = C_PROMPT_HI;
            else
               k = C_PROMPT_MED;
            if (formatitem)
            {
               (*formatitem)(j,buf);
               charptr=buf;
            }
            else
               charptr = choices[j];
            driver_put_string(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                      k,charptr);
            }
         /***
         ... format differs for summary/detail, whups, force box width to
         ...  be 72 when detail toggle available?  (2 grey margin each
         ...  side, 1 blue margin each side)
         ***/
         if (topleftchoice > 0 && hdg2 == NULL)
            driver_put_string(topleftrow-1,topleftcol,C_PROMPT_LO,"(more)");
         if (topleftchoice + boxitems < numchoices)
            driver_put_string(topleftrow+boxdepth,topleftcol,C_PROMPT_LO,"(more)");
         redisplay = 0;
         }

      i = current - topleftchoice;           /* highlight the current choice */
      if (formatitem)
      {
         (*formatitem)(current,curitem);
         itemptr=curitem;
      }
      else
         itemptr = choices[current];
      driver_put_string(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                C_CHOICE_CURRENT,itemptr);

      if (speedstring) {                     /* show speedstring if any */
         show_speedstring(speedrow,speedstring,speedprompt);
         }
      else
         driver_hide_text_cursor();

      driver_wait_key_pressed(0); /* enables help */
      curkey = driver_get_key();
#ifdef XFRACT
      if (curkey==FIK_F10) curkey=')';
      if (curkey==FIK_F9) curkey='(';
      if (curkey==FIK_F8) curkey='*';
#endif

      i = current - topleftchoice;           /* unhighlight current choice */
      if ((k = attributes[current] & 3) == 1)
         k = C_PROMPT_LO;
      else if (k == 3)
         k = C_PROMPT_HI;
      else
         k = C_PROMPT_MED;
      driver_put_string(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                k,itemptr);

      increment = 0;
      switch (curkey) {                      /* deal with input key */
         case FIK_ENTER:
         case FIK_ENTER_2:
            ret = current;
            goto fs_choice_end;
         case FIK_ESC:
            goto fs_choice_end;
         case FIK_DOWN_ARROW:
            rev_increment = 0 - (increment = boxwidth);
            break;
         case FIK_DOWN_ARROW_2:
            rev_increment = 0 - (increment = boxwidth);
            {
               int newcurrent = current;
               while((newcurrent+=boxwidth) != current) {
                  if(newcurrent >= numchoices)
                     newcurrent = (newcurrent % boxwidth) - boxwidth;
                  else if(!isadirname(choices[newcurrent])) {
                     if(current != newcurrent)
                        current = newcurrent - boxwidth;
                     break;  /* breaks the while loop */
                  }
               }
            }
            break;
         case FIK_UP_ARROW:
            increment = 0 - (rev_increment = boxwidth);
            break;
         case FIK_UP_ARROW_2:
            increment = 0 - (rev_increment = boxwidth);
            {
               int newcurrent = current;
               while((newcurrent-=boxwidth) != current) {
                  if(newcurrent < 0) {
                     newcurrent = (numchoices - current) % boxwidth;
                     newcurrent =  numchoices + (newcurrent ? boxwidth - newcurrent: 0);
                  }
                  else if(!isadirname(choices[newcurrent])) {
                     if(current != newcurrent)
                        current = newcurrent + boxwidth;
                     break;  /* breaks the while loop */
                  }
               }
            }
            break;
         case FIK_RIGHT_ARROW:
            increment = 1; rev_increment = -1;
            break;
         case FIK_RIGHT_ARROW_2:  /* move to next file; if at last file, go to
                                 first file */
            increment = 1; rev_increment = -1;
            {
               int newcurrent = current;
               while(++newcurrent != current) {
                  if(newcurrent >= numchoices)
                     newcurrent = -1;
                  else if(!isadirname(choices[newcurrent])) {
                     if(current != newcurrent)
                        current = newcurrent - 1;
                     break;  /* breaks the while loop */
                  }
               }
            }
            break;
         case FIK_LEFT_ARROW:
            increment = -1; rev_increment = 1;
            break;
         case FIK_LEFT_ARROW_2: /* move to previous file; if at first file, go to
                               last file */
            increment = -1; rev_increment = 1;
            {
               int newcurrent = current;
               while(--newcurrent != current) {
                  if(newcurrent < 0)
                     newcurrent = numchoices;
                  else if(!isadirname(choices[newcurrent])) {
                     if(current != newcurrent)
                        current = newcurrent + 1;
                     break;  /* breaks the while loop */
                  }
               }
            }
            break;
         case FIK_PAGE_UP:
            if (numchoices > boxitems) {
               topleftchoice -= boxitems;
               increment = -boxitems;
               rev_increment = boxwidth;
               redisplay = 1;
               }
            break;
         case FIK_PAGE_DOWN:
            if (numchoices > boxitems) {
               topleftchoice += boxitems;
               increment = boxitems;
               rev_increment = -boxwidth;
               redisplay = 1;
               }
            break;
         case FIK_HOME:
            current = -1;
            increment = rev_increment = 1;
            break;
         case FIK_CTL_HOME:
            current = -1;
            increment = rev_increment = 1;
            {
               int newcurrent;
               for(newcurrent = 0; newcurrent < numchoices; ++newcurrent) {
                  if(!isadirname(choices[newcurrent])) {
                     current = newcurrent - 1;
                     break;  /* breaks the for loop */
                  }
               }
            }
            break;
         case FIK_END:
            current = numchoices;
            increment = rev_increment = -1;
            break;
         case FIK_CTL_END:
            current = numchoices;
            increment = rev_increment = -1;
            {
               int newcurrent;
               for(newcurrent = numchoices - 1; newcurrent >= 0; --newcurrent) {
                  if(!isadirname(choices[newcurrent])) {
                     current = newcurrent + 1;
                     break;  /* breaks the for loop */
                  }
               }
            }
            break;
         default:
            if (checkkey) {
               if ((ret = (*checkkey)(curkey,current)) < -1 || ret > 0)
                  goto fs_choice_end;
               if (ret == -1)
                  redisplay = -1;
               }
            ret = -1;
            if (speedstring) {
               process_speedstring(speedstring,choices,curkey,&current,
                        numchoices,options&CHOICE_NOT_SORTED);
               }
            break;
      }
      if (increment) {                  /* apply cursor movement */
         current += increment;
         if (speedstring)               /* zap speedstring */
            speedstring[0] = 0;
         }
      for(;;) {                 /* adjust to a non-comment choice */
         if (current < 0 || current >= numchoices)
             increment = rev_increment;
         else if ((attributes[current] & 256) == 0)
             break;
         current += increment;
         }
      if (topleftchoice > numchoices - boxitems)
         topleftchoice = ((numchoices+boxwidth-1)/boxwidth)*boxwidth - boxitems;
      if (topleftchoice < 0)
         topleftchoice = 0;
      while (current < topleftchoice) {
         topleftchoice -= boxwidth;
         redisplay = 1;
         }
      while (current >= topleftchoice + boxitems) {
         topleftchoice += boxwidth;
         redisplay = 1;
         }
      }

fs_choice_end:
   lookatmouse = savelookatmouse;
   return(ret);

}

/* squeeze space out of string */
char *despace(char *str)
{
      char *obuf, *nbuf;

      for (obuf = str, nbuf = str; *obuf && obuf; ++obuf)
      {
            if (!isspace(*obuf))
                  *nbuf++ = *obuf;
      }
      *nbuf = 0;
      return str;
}

#ifndef XFRACT
/* case independent version of strncmp */
int strncasecmp(char *s,char *t,int ct)
{
   for(; (tolower(*s) == tolower(*t)) && --ct ; s++,t++)
      if(*s == '\0')
         return(0);
   return(tolower(*s) - tolower(*t));
}
#endif

#define LOADPROMPTSCHOICES(X,Y)     {\
   static char tmp[] = { Y };\
   choices[X]= (char *)tmp;\
   }

static int menutype;
#define MENU_HDG 3
#define MENU_ITEM 1

int main_menu(int fullmenu)
{
   char *choices[44]; /* 2 columns * 22 rows */
   int attributes[44];
   int choicekey[44];
   int i;
   int nextleft,nextright;
   int oldtabmode /* ,oldhelpmode */;
   static char MAIN_MENU[] = {"MAIN MENU"};
   int showjuliatoggle;
   oldtabmode = tabmode;
   /* oldhelpmode = helpmode; */
top:
   menutype = fullmenu;
   tabmode = 0;
   showjuliatoggle = 0;
   for (i = 0; i < 44; ++i) {
      attributes[i] = 256;
      choices[i] = "";
      choicekey[i] = -1;
      }
   nextleft = -2;
   nextright = -1;

   if (fullmenu) {
      LOADPROMPTSCHOICES(nextleft+=2,"      CURRENT IMAGE         ");
      attributes[nextleft] = 256+MENU_HDG;
      choicekey[nextleft+=2] = 13; /* enter */
      attributes[nextleft] = MENU_ITEM;
      if (calc_status == 2)
      {
         LOADPROMPTSCHOICES(nextleft,"continue calculation        ");
      }
      else
      {
         LOADPROMPTSCHOICES(nextleft,"return to image             ");
      }
      choicekey[nextleft+=2] = 9; /* tab */
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"info about image      <tab> ");
      choicekey[nextleft+=2] = 'o';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"orbits window          <o>  ");
      if(!(fractype==JULIA || fractype==JULIAFP || fractype==INVERSEJULIA))
          nextleft+=2;
      }
   LOADPROMPTSCHOICES(nextleft+=2,"      NEW IMAGE             ");
   attributes[nextleft] = 256+MENU_HDG;
#ifdef XFRACT
   choicekey[nextleft+=2] = FIK_DELETE;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"draw fractal           <D>  ");
#else
   choicekey[nextleft+=2] = FIK_DELETE;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"select video mode...  <del> ");
#endif
   choicekey[nextleft+=2] = 't';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"select fractal type    <t>  ");
   if (fullmenu) {
      if ((curfractalspecific->tojulia != NOFRACTAL
          && param[0] == 0.0 && param[1] == 0.0)
          || curfractalspecific->tomandel != NOFRACTAL) {
             choicekey[nextleft+=2] = ' ';
             attributes[nextleft] = MENU_ITEM;
             LOADPROMPTSCHOICES(nextleft,"toggle to/from julia <space>");
             showjuliatoggle = 1;
          }
      if(fractype==JULIA || fractype==JULIAFP || fractype==INVERSEJULIA) {
             choicekey[nextleft+=2] = 'j';
             attributes[nextleft] = MENU_ITEM;
             LOADPROMPTSCHOICES(nextleft,"toggle to/from inverse <j>  ");
             showjuliatoggle = 1;
          }
      choicekey[nextleft+=2] = 'h';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"return to prior image  <h>   ");

      choicekey[nextleft+=2] = 8;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"reverse thru history <ctl-h> ");
   }
   else
      nextleft += 2;
   LOADPROMPTSCHOICES(nextleft+=2,"      OPTIONS                ");
   attributes[nextleft] = 256+MENU_HDG;
   choicekey[nextleft+=2] = 'x';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"basic options...       <x>  ");
   choicekey[nextleft+=2] = 'y';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"extended options...    <y>  ");
   choicekey[nextleft+=2] = 'z';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"type-specific parms... <z>  ");
   choicekey[nextleft+=2] = 'p';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"passes options...      <p>  ");
   choicekey[nextleft+=2] = 'v';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"view window options... <v>  ");
   if(showjuliatoggle == 0)
   {
      choicekey[nextleft+=2] = 'i';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"fractal 3D parms...    <i>  ");
   }
   choicekey[nextleft+=2] = 2;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"browse parms...      <ctl-b>");

   if (fullmenu) {
      choicekey[nextleft+=2] = 5;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"evolver parms...     <ctl-e>");
   }
#ifndef XFRACT
   if (fullmenu) {
      choicekey[nextleft+=2] = 6;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"sound parms...       <ctl-f>");
   }
#endif
   LOADPROMPTSCHOICES(nextright+=2,"        FILE                  ");
   attributes[nextright] = 256+MENU_HDG;
   choicekey[nextright+=2] = '@';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"run saved command set... <@>  ");
   if (fullmenu) {
      choicekey[nextright+=2] = 's';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"save image to file       <s>  ");
      }
   choicekey[nextright+=2] = 'r';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"load image from file...  <r>  ");
   choicekey[nextright+=2] = '3';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"3d transform from file...<3>  ");
   if (fullmenu) {
      choicekey[nextright+=2] = '#';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"3d overlay from file.....<#>  ");
      choicekey[nextright+=2] = 'b';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"save current parameters..<b>  ");
      choicekey[nextright+=2] = 16;
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"print image          <ctl-p>  ");
      }
#ifdef XFRACT
   choicekey[nextright+=2] = 'd';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"shell to Linux/Unix      <d>  ");
#else
   choicekey[nextright+=2] = 'd';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"shell to dos             <d>  ");
#endif
   choicekey[nextright+=2] = 'g';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"give command string      <g>  ");
   choicekey[nextright+=2] = FIK_ESC;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"quit "FRACTINT"           <esc> ");
   choicekey[nextright+=2] = FIK_INSERT;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"restart "FRACTINT"        <ins> ");
#ifdef XFRACT
   if (fullmenu && (g_got_real_dac || fake_lut) && colors >= 16) {
#else
   if (fullmenu && g_got_real_dac && colors >= 16) {
#endif
      /* nextright += 2; */
      LOADPROMPTSCHOICES(nextright+=2,"       COLORS                 ");
      attributes[nextright] = 256+MENU_HDG;
      choicekey[nextright+=2] = 'c';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"color cycling mode       <c>  ");
      choicekey[nextright+=2] = '+';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"rotate palette      <+>, <->  ");
      if (colors > 16) {
         if (!g_really_ega) {
            choicekey[nextright+=2] = 'e';
            attributes[nextright] = MENU_ITEM;
            LOADPROMPTSCHOICES(nextright,"palette editing mode     <e>  ");
         }
         choicekey[nextright+=2] = 'a';
         attributes[nextright] = MENU_ITEM;
         LOADPROMPTSCHOICES(nextright,"make starfield           <a>  ");
      }
   }
   choicekey[nextright+=2] = 1;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,   "ant automaton          <ctl-a>");

   choicekey[nextright+=2] = 19;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,   "stereogram             <ctl-s>");

   i = (driver_key_pressed()) ? driver_get_key() : 0;
   if (menu_checkkey(i,0) == 0) {
      helpmode = HELPMAIN;         /* switch help modes */
      if ((nextleft += 2) < nextright)
         nextleft = nextright + 1;
      i = fullscreen_choice(CHOICE_MENU+CHOICE_CRUNCH,
          MAIN_MENU,
          NULL,NULL,nextleft,(char * *)choices,attributes,
          2,nextleft/2,29,0,NULL,NULL,NULL,menu_checkkey);
      if (i == -1)     /* escape */
         i = FIK_ESC;
      else if (i < 0)
         i = 0 - i;
      else {                      /* user selected a choice */
         i = choicekey[i];
         switch (i) {             /* check for special cases */
            case -10:             /* zoombox functions */
               helpmode = HELPZOOM;
               help(0);
               i = 0;
               break;
            }
         }
      }
   if (i == FIK_ESC) {             /* escape from menu exits Fractint */
#ifdef XFRACT
      static char s[] = "Exit from Xfractint (y/n)? y";
#else
      static char s[] = "Exit from Fractint (y/n)? y";
#endif
      helptitle();
      driver_set_attr(1,0,C_GENERAL_MED,24*80);
      for (i = 9; i <= 11; ++i)
         driver_set_attr(i,18,C_GENERAL_INPUT,40);
      putstringcenter(10,18,40,C_GENERAL_INPUT,s);
      driver_hide_text_cursor();
      while ((i = driver_get_key()) != 'y' && i != 'Y' && i != 13) {
         if (i == 'n' || i == 'N')
            goto top;
         }
      goodbye();
      }
   if (i == FIK_TAB) {
      tab_display();
      i = 0;
      }
   if (i == FIK_ENTER || i == FIK_ENTER_2)
      i = 0;                 /* don't trigger new calc */
   tabmode = oldtabmode;
   return(i);
}

static int menu_checkkey(int curkey,int choice)
{ /* choice is dummy used by other routines called by fullscreen_choice() */
   int testkey;
   testkey = choice; /* for warning only */
   testkey = (curkey>='A' && curkey<='Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
   /* We use F2 for shift-@, annoyingly enough */
   if (testkey == FIK_F2) return(0-testkey);
#endif
   if(testkey == '2')
      testkey = '@';
   if (strchr("#@2txyzgvir3dj",testkey) || testkey == FIK_INSERT || testkey == 2
     || testkey == FIK_ESC || testkey == FIK_DELETE || testkey == 6) /*RB 6== ctrl-F for sound menu */
      return(0-testkey);
   if (menutype) {
      if (strchr("\\sobpkrh",testkey) || testkey == FIK_TAB
        || testkey == 1 || testkey == 5 || testkey == 8
        || testkey == 16
        || testkey == 19 || testkey == 21) /* ctrl-A, E, H, P, S, U */
         return(0-testkey);
      if (testkey == ' ')
         if ((curfractalspecific->tojulia != NOFRACTAL
              && param[0] == 0.0 && param[1] == 0.0)
           || curfractalspecific->tomandel != NOFRACTAL)
         return(0-testkey);
      if (g_got_real_dac && colors >= 16) {
         if (strchr("c+-",testkey))
            return(0-testkey);
         if (colors > 16
           && (testkey == 'a' || (!g_really_ega && testkey == 'e')))
            return(0-testkey);
         }
      /* Alt-A and Alt-S */
      if (testkey == 1030 || testkey == 1031 )
         return(0-testkey);
      }
   if (check_vidmode_key(0,testkey) >= 0)
      return(0-testkey);
   return(0);
}


int input_field(
        int options,          /* &1 numeric, &2 integer, &4 double */
        int attr,             /* display attribute */
        char *fld,            /* the field itself */
        int len,              /* field length (declare as 1 larger for \0) */
        int row,              /* display row */
        int col,              /* display column */
        int (*checkkey)(int)  /* routine to check non data keys, or NULL */
        )
{
   char savefld[81];
   char buf[81];
   int insert, started, offset, curkey, display;
   int i, j;
   int ret,savelookatmouse;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   ret = -1;
   strcpy(savefld,fld);
   insert = started = offset = 0;
   display = 1;
   for(;;) {
      strcpy(buf,fld);
      i = (int) strlen(buf);
      while (i < len)
         buf[i++] = ' ';
      buf[len] = 0;
      if (display) {                                /* display current value */
         driver_put_string(row,col,attr,buf);
         display = 0;
         }
      curkey = driver_key_cursor(row+insert,col+offset);  /* get a keystroke */
      if(curkey == 1047) curkey = 47; /* numeric slash */
      switch (curkey) {
         case FIK_ENTER:
         case FIK_ENTER_2:
            ret = 0;
            goto inpfld_end;
         case FIK_ESC:
            goto inpfld_end;
         case FIK_RIGHT_ARROW:
            if (offset < len-1) ++offset;
            started = 1;
            break;
         case FIK_LEFT_ARROW:
            if (offset > 0) --offset;
            started = 1;
            break;
         case FIK_HOME:
            offset = 0;
            started = 1;
            break;
         case FIK_END:
            offset = (int) strlen(fld);
            started = 1;
            break;
         case 8:
         case 127:                              /* backspace */
            if (offset > 0) {
               j = (int) strlen(fld);
               for (i = offset-1; i < j; ++i)
                  fld[i] = fld[i+1];
               --offset;
               }
            started = display = 1;
            break;
         case FIK_DELETE:                           /* delete */
            j = (int) strlen(fld);
            for (i = offset; i < j; ++i)
               fld[i] = fld[i+1];
            started = display = 1;
            break;
         case FIK_INSERT:                           /* insert */
            insert ^= 0x8000;
            started = 1;
            break;
         case FIK_F5:
            strcpy(fld,savefld);
            insert = started = offset = 0;
            display = 1;
            break;
         default:
            if (nonalpha(curkey)) {
               if (checkkey && (ret = (*checkkey)(curkey)) != 0)
                  goto inpfld_end;
               break;                                /* non alphanum char */
               }
            if (offset >= len) break;                /* at end of field */
            if (insert && started && strlen(fld) >= (size_t)len)
               break;                                /* insert & full */
            if ((options & 1)
              && (curkey < '0' || curkey > '9')
              && curkey != '+' && curkey != '-') {
               if ((options & 2))
                  break;
               /* allow scientific notation, and specials "e" and "p" */
               if ( ((curkey != 'e' && curkey != 'E') || offset >= 18)
                 && ((curkey != 'p' && curkey != 'P') || offset != 0 )
                 && curkey != '.')
                  break;
               }
            if (started == 0) /* first char is data, zap field */
               fld[0] = 0;
            if (insert) {
               j = (int) strlen(fld);
               while (j >= offset) {
                  fld[j+1] = fld[j];
                  --j;
                  }
               }
            if ((size_t)offset >= strlen(fld))
               fld[offset+1] = 0;
            fld[offset++] = (char)curkey;
            /* if "e" or "p" in first col make number e or pi */
            if ((options & 3) == 1) { /* floating point */
               double tmpd;
               int specialv;
               char tmpfld[30];
               specialv = 0;
               if (*fld == 'e' || *fld == 'E') {
                  tmpd = exp(1.0);
                  specialv = 1;
                  }
               if (*fld == 'p' || *fld == 'P') {
                  tmpd = atan(1.0) * 4;
                  specialv = 1;
                  }
               if (specialv) {
                  if ((options & 4) == 0)
                     roundfloatd(&tmpd);
                  sprintf(tmpfld,"%.15g",tmpd);
                  tmpfld[len-1] = 0; /* safety, field should be long enough */
                  strcpy(fld,tmpfld);
                  offset = 0;
                  }
               }
            started = display = 1;
         }
      }
inpfld_end:
   lookatmouse = savelookatmouse;
   return(ret);
}

int field_prompt(
        int options,        /* &1 numeric value, &2 integer */
        char *hdg,      /* heading, \n delimited lines */
        char *instr,    /* additional instructions or NULL */
        char *fld,          /* the field itself */
        int len,            /* field length (declare as 1 larger for \0) */
        int (*checkkey)(int)   /* routine to check non data keys, or NULL */
        )
{
   char *charptr;
   int boxwidth,titlelines,titlecol,titlerow;
   int promptcol;
   int i,j;
   char buf[81];
   static char DEFLT_INST[] = {"Press ENTER when finished (or ESCAPE to back out)"};
   helptitle();                           /* clear screen, display title */
   driver_set_attr(1,0,C_PROMPT_BKGRD,24*80);     /* init rest to background */
   charptr = hdg;                         /* count title lines, find widest */
   i = boxwidth = 0;
   titlelines = 1;
   while (*charptr) {
      if (*(charptr++) == '\n') {
         ++titlelines;
         i = -1;
         }
      if (++i > boxwidth)
         boxwidth = i;
      }
   if (len > boxwidth)
      boxwidth = len;
   i = titlelines + 4;                    /* total rows in box */
   titlerow = (25 - i) / 2;               /* top row of it all when centered */
   titlerow -= titlerow / 4;              /* higher is better if lots extra */
   titlecol = (80 - boxwidth) / 2;        /* center the box */
   titlecol -= (90 - boxwidth) / 20;
   promptcol = titlecol - (boxwidth-len)/2;
   j = titlecol;                          /* add margin at each side of box */
   if ((i = (82-boxwidth)/4) > 3)
      i = 3;
   j -= i;
   boxwidth += i * 2;
   for (i = -1; i < titlelines+3; ++i)    /* draw empty box */
      driver_set_attr(titlerow+i,j,C_PROMPT_LO,boxwidth);
   g_text_cbase = titlecol;                  /* set left margin for putstring */
   driver_put_string(titlerow,0,C_PROMPT_HI,hdg); /* display heading */
   g_text_cbase = 0;
   i = titlerow + titlelines + 4;
   if (instr) {                           /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)) != 0)
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }
   else                                   /* default instructions */
      putstringcenter(i,0,80,C_PROMPT_BKGRD,DEFLT_INST);
   return(input_field(options,C_PROMPT_INPUT,fld,len,
                      titlerow+titlelines+1,promptcol,checkkey));
}


/* thinking(1,message):
      if thinking message not yet on display, it is displayed;
      otherwise the wheel is updated
      returns 0 to keep going, -1 if keystroke pending
   thinking(0,NULL):
      call this when thinking phase is done
   */

int thinking(int options,char *msg)
{
   static int thinkstate = -1;
   char *wheel[] = {"-","\\","|","/"};
   static int thinkcol;
   static int count = 0;
   char buf[81];
   if (options == 0) {
      if (thinkstate >= 0) {
         thinkstate = -1;
         driver_unstack_screen();
         }
      return(0);
      }
   if (thinkstate < 0) {
      driver_stack_screen();
      thinkstate = 0;
      helptitle();
      strcpy(buf,"  ");
      strcat(buf,msg);
      strcat(buf,"    ");
      driver_put_string(4,10,C_GENERAL_HI,buf);
      thinkcol = g_text_col - 3;
      count = 0;
      }
   if ((count++)<100) {
       return 0;
   }
   count = 0;
   driver_put_string(4,thinkcol,C_GENERAL_HI,wheel[thinkstate]);
   driver_hide_text_cursor(); /* turn off cursor */
   thinkstate = (thinkstate + 1) & 3;
   return (driver_key_pressed());
}


int clear_screen(int dummy)  /* a stub for a windows only subroutine */
{
   dummy=0; /* quite the warning */
   return 0;
}


/* savegraphics/restoregraphics: video.asm subroutines */

unsigned long swaptotlen;
unsigned long swapoffset;
BYTE *swapvidbuf;
int swaplength;

#define SWAPBLKLEN 4096 /* must be a power of 2 */
U16 memhandle = 0;

#ifdef XFRACT
BYTE suffix[10000];
#endif

#ifndef XFRACT
int savegraphics()
{
   int i;
   long count;
   unsigned long swaptmpoff;

   swaptotlen = (long)(g_vxdots > sxdots ? g_vxdots : sxdots) * (long)sydots;
   i = colors;
   while (i <= 16) {
      swaptotlen >>= 1;
      i = i * i;
      }
   count = (long)((swaptotlen / SWAPBLKLEN) + 1);
   swapoffset = 0;
   if (memhandle != 0)
      discardgraphics(); /* if any emm/xmm in use from prior call, release it */
			 /* TODO: MemoryAlloc */
   memhandle = MemoryAlloc((U16)SWAPBLKLEN, count, MEMORY);

   while (swapoffset < swaptotlen) {
      swaplength = SWAPBLKLEN;
      if ((swapoffset & (SWAPBLKLEN-1)) != 0)
         swaplength = (int)(SWAPBLKLEN - (swapoffset & (SWAPBLKLEN-1)));
      if ((unsigned long)swaplength > (swaptotlen - swapoffset))
         swaplength = (int)(swaptotlen - swapoffset);
      if (swapoffset == 0)
         swaptmpoff = 0;
      else
         swaptmpoff = swapoffset/swaplength;
      (*g_swap_setup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */

      MoveToMemory(swapvidbuf,(U16)swaplength,1L,swaptmpoff,memhandle);

      swapoffset += swaplength;
      }
   return 0;
}

int restoregraphics()
{
   unsigned long swaptmpoff;

   swapoffset = 0;
   /* TODO: allocate real memory, not reuse shared segment */
   swapvidbuf = extraseg; /* for swapnormwrite case */

   while (swapoffset < swaptotlen) {
      swaplength = SWAPBLKLEN;
      if ((swapoffset & (SWAPBLKLEN-1)) != 0)
         swaplength = (int)(SWAPBLKLEN - (swapoffset & (SWAPBLKLEN-1)));
      if ((unsigned long)swaplength > (swaptotlen - swapoffset))
         swaplength = (int)(swaptotlen - swapoffset);
      if (swapoffset == 0)
         swaptmpoff = 0;
      else
         swaptmpoff = swapoffset/swaplength;
      if (g_swap_setup != swapnormread)
         (*g_swap_setup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */

      MoveFromMemory(swapvidbuf,(U16)swaplength,1L,swaptmpoff,memhandle);

      if (g_swap_setup == swapnormread)
         swapnormwrite();
      swapoffset += swaplength;
      }

   discardgraphics();
   return(0);
}

#else
int savegraphics() {return 0;}
int restoregraphics() {return 0;}
#endif

void discardgraphics() /* release expanded/extended memory if any in use */
{
#ifndef XFRACT
   MemoryRelease(memhandle);
   memhandle = 0;
#endif
}

/*VIDEOINFO *g_video_table;  /* temporarily loaded fractint.cfg info */
int g_video_table_len;                 /* number of entries in above           */

int showvidlength()
{
   int sz;
   sz = (sizeof(VIDEOINFO)+sizeof(int))*MAXVIDEOMODES;
   return(sz);
}

int g_cfg_line_nums[MAXVIDEOMODES] = { 0 };

/* load_fractint_config
 *
 * Reads fractint.cfg, loading videoinfo entries into g_video_table.
 * Sets the number of entries, sets g_video_table_len.
 * Past g_video_table, g_cfg_line_nums are stored for update_fractint_cfg.
 * If fractint.cfg is not found or invalid, issues a message
 * (first time the problem occurs only, and only if options is
 * zero) and uses the hard-coded table.
 */
void load_fractint_config(void)
{
	FILE *cfgfile;
	VIDEOINFO vident;
	int linenum;
	long xdots, ydots;
	int i, j, keynum, ax, bx, cx, dx, dotmode, colors;
	char *fields[11];
	int textsafe2;
	char tempstring[150];
	int truecolorbits;

	findpath("fractint.cfg",tempstring);
	if (tempstring[0] == 0                            /* can't find the file */
		|| (cfgfile = fopen(tempstring,"r")) == NULL)   /* can't open it */
	{
		goto bad_fractint_cfg;
	}

	linenum = 0;
	while (g_video_table_len < MAXVIDEOMODES
		&& fgets(tempstring, 120, cfgfile))
	{
		if(strchr(tempstring,'\n') == NULL)
		{
			/* finish reading the line */
			while (fgetc(cfgfile) != '\n' && !feof(cfgfile));
		}
		++linenum;
		if (tempstring[0] == ';')
		{
			continue;   /* comment line */
		}
		tempstring[120] = 0;
		tempstring[(int) strlen(tempstring)-1] = 0; /* zap trailing \n */
		i = j = -1;
		/* key, mode name, ax, bx, cx, dx, dotmode, x, y, colors, comments, driver */
		for (;;)
		{
			if (tempstring[++i] < ' ')
			{
				if (tempstring[i] == 0)
				{
					break;
				}
				tempstring[i] = ' '; /* convert tab (or whatever) to blank */
			}
			else if (tempstring[i] == ',' && ++j < 11)
			{
				_ASSERTE(j >= 0 && j < 11);
				fields[j] = &tempstring[i+1]; /* remember start of next field */
				tempstring[i] = 0;   /* make field a separate string */
			}
		}
		keynum = check_vidmode_keyname(tempstring);
		sscanf(fields[1],"%x",&ax);
		sscanf(fields[2],"%x",&bx);
		sscanf(fields[3],"%x",&cx);
		sscanf(fields[4],"%x",&dx);
		dotmode     = atoi(fields[5]);
		xdots       = atol(fields[6]);
		ydots       = atol(fields[7]);
		colors      = atoi(fields[8]);
		if (colors == 4 && strchr(strlwr(fields[8]),'g'))
		{
			colors = 256;
			truecolorbits = 4; /* 32 bits */
		}
		else if (colors == 16 && strchr(fields[8],'m'))
		{
			colors = 256;
			truecolorbits = 3; /* 24 bits */
		}
		else if (colors == 64 && strchr(fields[8],'k'))
		{
			colors = 256;
			truecolorbits = 2; /* 16 bits */
		}
		else if (colors == 32 && strchr(fields[8],'k'))
		{
			colors = 256;
			truecolorbits = 1; /* 15 bits */
		}
		else
		{
			truecolorbits = 0;
		}

		textsafe2   = dotmode / 100;
		dotmode    %= 100;
		if (j < 9 ||
				keynum < 0 ||
				dotmode < 0 || dotmode > 30 ||
				textsafe2 < 0 || textsafe2 > 4 ||
				xdots < MINPIXELS || xdots > MAXPIXELS ||
				ydots < MINPIXELS || ydots > MAXPIXELS ||
				(colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
				colors != 256)
			)
		{
			goto bad_fractint_cfg;
		}
		g_cfg_line_nums[g_video_table_len] = linenum; /* for update_fractint_cfg */

		memset(&vident, 0, sizeof(vident));
		strncpy(&vident.name[0], fields[0], NUM_OF(vident.name));
		strncpy(&vident.comment[0], fields[9], NUM_OF(vident.comment));
		vident.name[25] = vident.comment[25] = 0;
		vident.keynum      = keynum;
		vident.videomodeax = ax;
		vident.videomodebx = bx;
		vident.videomodecx = cx;
		vident.videomodedx = dx;
		vident.dotmode     = truecolorbits * 1000 + textsafe2 * 100 + dotmode;
		vident.xdots       = (short)xdots;
		vident.ydots       = (short)ydots;
		vident.colors      = colors;

		/* if valid, add to supported modes */
		vident.driver = driver_find_by_name(fields[10]);
		if (vident.driver != NULL)
		{
			if (vident.driver->validate_mode(vident.driver, &vident))
			{
				/* look for a synonym mode and if found, overwite its key */
				int m;
				int synonym_found = FALSE;
				for (m = 0; m < g_video_table_len; m++)
				{
					VIDEOINFO *mode = &g_video_table[m];
					if ((mode->driver == vident.driver) && (mode->colors == vident.colors) &&
						(mode->xdots == vident.xdots) && (mode->ydots == vident.ydots) &&
						(mode->dotmode == vident.dotmode))
					{
						mode->keynum = vident.keynum;
						synonym_found = TRUE;
						break;
					}
				}
				/* no synonym found, append it to current list of video modes */
				if (FALSE == synonym_found)
				{
					memcpy(&g_video_table[g_video_table_len], &vident, sizeof(VIDEOINFO));
					g_video_table_len++;
				}
			}
		}
	}
	fclose(cfgfile);
	return;

bad_fractint_cfg:
	g_bad_config = -1; /* bad, no message issued yet */
}

void bad_fractint_cfg_msg()
{
	stopmsg(0,"\
File FRACTINT.CFG is missing or invalid.\n\
See Hardware Support and Video Modes in the full documentation for help.\n\
I will continue with only the built-in video modes available.");
	g_bad_config = 1; /* bad, message issued */
}

int check_vidmode_key(int option,int k)
{
   int i;
   /* returns g_video_table entry number if the passed keystroke is a  */
   /* function key currently assigned to a video mode, -1 otherwise */
   if (k == 1400)              /* special value from select_vid_mode  */
      return(MAXVIDEOMODES-1); /* for last entry with no key assigned */
   if (k != 0) {
      if (option == 0) { /* check resident video mode table */
         for (i = 0; i < MAXVIDEOMODES; ++i) {
            if (g_video_table[i].keynum == k)
               return(i);
            }
         }
      else { /* check full g_video_table */
         for (i = 0; i < g_video_table_len; ++i) {
            if (g_video_table[i].keynum == k)
               return(i);
            }
         }
   }
   return(-1);
}

int check_vidmode_keyname(char *kname)
{
   /* returns key number for the passed keyname, 0 if not a keyname */
   int i,keyset;
   keyset = 1058;
   if (*kname == 'S' || *kname == 's') {
      keyset = 1083;
      ++kname;
      }
   else if (*kname == 'C' || *kname == 'c') {
      keyset = 1093;
      ++kname;
      }
   else if (*kname == 'A' || *kname == 'a') {
      keyset = 1103;
      ++kname;
      }
   if (*kname != 'F' && *kname != 'f')
      return(0);
   if (*++kname < '1' || *kname > '9')
      return(0);
   i = *kname - '0';
   if (*++kname != 0 && *kname != ' ') {
      if (*kname != '0' || i != 1)
         return(0);
      i = 10;
      ++kname;
      }
   while (*kname)
      if (*(kname++) != ' ')
         return(0);
   if ((i += keyset) < 2)
      i = 0;
   return(i);
}

void vidmode_keyname(int k,char *buf)
{
   /* set buffer to name of passed key number */
   *buf = 0;
   if (k > 0) {
      if (k > 1103) {
         *(buf++) = 'A';
         k -= 1103;
         }
      else if (k > 1093) {
         *(buf++) = 'C';
         k -= 1093;
         }
      else if (k > 1083) {
         *(buf++) = 'S';
         k -= 1083;
         }
      else
         k -= 1058;
      sprintf(buf,"F%d",k);
      }
}
