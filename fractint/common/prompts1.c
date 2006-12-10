/*
        Various routines that prompt for things.
*/

#include <string.h>
#include <ctype.h>
#ifdef   XFRACT
#ifndef  __386BSD__
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#ifdef __hpux
#include <sys/param.h>
#define getwd(a) getcwd(a,MAXPATHLEN)
#endif

#ifdef __SVR4
#include <sys/param.h>
#define getwd(a) getcwd(a,MAXPATHLEN)
#endif
#include "drivers.h"

/* Routines used in prompts2.c */

   int prompt_checkkey(int curkey);
   int prompt_checkkey_scroll(int curkey);
   long get_file_entry(int,char *,char *,char *,char *);

/* Routines in this module      */

int prompt_valuestring(char *buf,struct fullscreenvalues *val);
static  int input_field_list(int attr,char *fld,int vlen,char **list,int llen,
                             int row,int col,int (*checkkey)(int));
static  int select_fracttype(int t);
static  int sel_fractype_help(int curkey, int choice);
        int select_type_params(int newfractype,int oldfractype);
        void set_default_parms(void);
static  long gfe_choose_entry(int,char *,char *,char *);
static  int check_gfe_key(int curkey,int choice);
static  void load_entry_text(FILE *entfile,char *buf,int maxlines, int startrow, int startcol);
static  void format_parmfile_line(int,char *);
static  int get_light_params(void );
static  int check_mapfile(void );
static  int get_funny_glasses_params(void );

/* fullscreen_choice options */
#define CHOICEHELP      4

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS     2
#define GETPARM    3

static char funnyglasses_map_name[16];
char ifsmask[13]     = {"*.ifs"};
char formmask[13]    = {"*.frm"};
char lsysmask[13]    = {"*.l"};
char Glasses1Map[] = "glasses1.map";
char MAP_name[FILE_MAX_DIR] = "";
int  mapset = 0;
int julibrot;   /* flag for julibrot */

/* --------------------------------------------------------------------- */

int promptfkeys;

   /* These need to be global because F6 exits fullscreen_prompt() */
int scroll_row_status;    /* will be set to first line of extra info to
                             be displayed ( 0 = top line) */
int scroll_column_status; /* will be set to first column of extra info to
                             be displayed ( 0 = leftmost column )*/

int fullscreen_prompt(  /* full-screen prompting routine */
        char *hdg,          /* heading, lines separated by \n */
        int numprompts,         /* there are this many prompts (max) */
        char **prompts,     /* array of prompting pointers */
        struct fullscreenvalues *values, /* array of values */
        int fkeymask,           /* bit n on if Fn to cause return */
        char *extrainfo     /* extra info box to display, \n separated */
        )
{
   char *hdgscan;
   int titlelines,titlewidth,titlerow;
   int maxpromptwidth,maxfldwidth,maxcomment;
   int boxrow,boxlines;
   int boxcol,boxwidth;
   int extralines,extrawidth,extrarow;
   int instrrow;
   int promptrow,promptcol,valuecol;
   int curchoice = 0;
   int done, i, j;
   int anyinput;
   int savelookatmouse;
   int curtype, curlen;
   char buf[81];

      /* scrolling related variables */
   FILE * scroll_file = NULL;     /* file with extrainfo entry to scroll   */
   long scroll_file_start = 0;    /* where entry starts in scroll_file     */
   int in_scrolling_mode = 0;     /* will be 1 if need to scroll extrainfo */
   int lines_in_entry = 0;        /* total lines in entry to be scrolled   */
   int vertical_scroll_limit = 0; /* don't scroll down if this is top line */
   int widest_entry_line = 0;     /* length of longest line in entry       */
   int rewrite_extrainfo = 0;     /* if 1: rewrite extrainfo to text box   */
   char blanks[78];               /* used to clear text box                */

static FCODE instr1[]  = {"Use " UPARR1 " and " DNARR1 " to select values to change"};
static FCODE instr2a[]  = {"Type in replacement value for selected field"};
static FCODE instr2b[]  = {"Use " LTARR1 " or " RTARR1 " to change value of selected field"};
static FCODE instr3a[] = {"Press ENTER when finished (or ESCAPE to back out)"};
static FCODE instr3b[] = {"Press ENTER when finished, ESCAPE to back out, or "FK_F1" for help"};

static FCODE instr0[] = {"No changeable parameters;"};
static FCODE instr0a[] = {"Press ENTER to exit"};
static FCODE instr0b[] = {"Press ENTER to exit, ESC to back out, "FK_F1" for help"};

   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   promptfkeys = fkeymask;
   memset(blanks,' ',77);   /* initialize string of blanks */
   blanks[77] = (char) 0;

      /* If applicable, open file for scrolling extrainfo. The function
         find_file_item() opens the file and sets the file pointer to the
         beginning of the entry.
      */

   if((fractype == FORMULA || fractype == FFORMULA) && extrainfo && *extrainfo) {
      find_file_item(FormFileName, FormName, &scroll_file, 1);
      in_scrolling_mode = 1;
      scroll_file_start = ftell(scroll_file);
   }

   else if(fractype == LSYSTEM && extrainfo && *extrainfo) {
      find_file_item(LFileName, LName, &scroll_file, 2);
      in_scrolling_mode = 1;
      scroll_file_start = ftell(scroll_file);
   }

   else if((fractype == IFS || fractype == IFS3D) && extrainfo && *extrainfo) {
      find_file_item(IFSFileName, IFSName, &scroll_file, 3);
      in_scrolling_mode = 1;
      scroll_file_start = ftell(scroll_file);
   }

      /* initialize widest_entry_line and lines_in_entry */
   if(in_scrolling_mode && scroll_file != NULL) {
      int comment = 0;
      int c = 0;
      int widthct = 0;
      while((c = fgetc(scroll_file)) != EOF && c != '\032') {
         if(c == ';')
            comment = 1;
         else if(c == '\n') {
            comment = 0;
            lines_in_entry++;
            widthct =  -1;
         }
         else if (c == '\t')
            widthct += 7 - widthct % 8;
         else if ( c == '\r')
            continue;
         if(++widthct > widest_entry_line)
            widest_entry_line = widthct;
         if (c == '}' && !comment) {
            lines_in_entry++;
            break;
         }
      }
      if(c == EOF || c == '\032') { /* should never happen */
         fclose(scroll_file);
         in_scrolling_mode = 0;
      }
   }



   helptitle();                        /* clear screen, display title line  */
   driver_set_attr(1,0,C_PROMPT_BKGRD,24*80);  /* init rest of screen to background */


   hdgscan = hdg;                      /* count title lines, find widest */
   i = titlewidth = 0;
   titlelines = 1;
   while (*hdgscan) {
      if (*(hdgscan++) == '\n') {
         ++titlelines;
         i = -1;
      }
      if (++i > titlewidth)
         titlewidth = i;
   }
   extralines = extrawidth = i = 0;
   if ((hdgscan = extrainfo) != 0) {
      if (*hdgscan == 0)
         extrainfo = NULL;
      else { /* count extra lines, find widest */
         extralines = 3;
         while (*hdgscan) {
            if (*(hdgscan++) == '\n') {
               if (extralines + numprompts + titlelines >= 20) {
                   *hdgscan = 0; /* full screen, cut off here */
                   break;
               }
               ++extralines;
               i = -1;
            }
            if (++i > extrawidth)
               extrawidth = i;
         }
      }
   }

      /* if entry fits in available space, shut off scrolling */
   if(in_scrolling_mode && scroll_row_status == 0
             && lines_in_entry == extralines - 2
             && scroll_column_status == 0
             && strchr(extrainfo, '\021') == NULL) {
      in_scrolling_mode = 0;
      fclose(scroll_file);
      scroll_file = NULL;
   }

      /*initialize vertical scroll limit. When the top line of the text
        box is the vertical scroll limit, the bottom line is the end of the
        entry, and no further down scrolling is necessary.
      */
   if (in_scrolling_mode)
      vertical_scroll_limit = lines_in_entry - (extralines - 2);

   /* work out vertical positioning */
   i = numprompts + titlelines + extralines + 3; /* total rows required */
   j = (25 - i) / 2;                   /* top row of it all when centered */
   j -= j / 4;                         /* higher is better if lots extra */
   boxlines = numprompts;
   titlerow = 1 + j;
   promptrow = boxrow = titlerow + titlelines;
   if (titlerow > 2) {                 /* room for blank between title & box? */
      --titlerow;
      --boxrow;
      ++boxlines;
      }
   instrrow = boxrow+boxlines;
   if (instrrow + 3 + extralines < 25) {
      ++boxlines;    /* blank at bottom of box */
      ++instrrow;
      if (instrrow + 3 + extralines < 25)
         ++instrrow; /* blank before instructions */
      }
   extrarow = instrrow + 2;
   if (numprompts > 1) /* 3 instructions lines */
      ++extrarow;
   if (extrarow + extralines < 25)
      ++extrarow;

   if(in_scrolling_mode)  /* set box to max width if in scrolling mode */
      extrawidth = 76;

   /* work out horizontal positioning */
   maxfldwidth = maxpromptwidth = maxcomment = anyinput = 0;
   for (i = 0; i < numprompts; i++) {
      if (values[i].type == 'y') {
         static char *noyes[2] = {s_no,s_yes};
         values[i].type = 'l';
         values[i].uval.ch.vlen = 3;
         values[i].uval.ch.list = noyes;
         values[i].uval.ch.llen = 2;
         }
      j = (int) strlen(prompts[i]);
      if (values[i].type == '*') {
         if (j > maxcomment)     maxcomment = j;
         }
      else {
         anyinput = 1;
         if (j > maxpromptwidth) maxpromptwidth = j;
         j = prompt_valuestring(buf,&values[i]);
         if (j > maxfldwidth)    maxfldwidth = j;
         }
      }
   boxwidth = maxpromptwidth + maxfldwidth + 2;
   if (maxcomment > boxwidth) boxwidth = maxcomment;
   if ((boxwidth += 4) > 80) boxwidth = 80;
   boxcol = (80 - boxwidth) / 2;       /* center the box */
   promptcol = boxcol + 2;
   valuecol = boxcol + boxwidth - maxfldwidth - 2;
   if (boxwidth <= 76) {               /* make margin a bit wider if we can */
      boxwidth += 2;
      --boxcol;
      }
   if ((j = titlewidth) < extrawidth)
      j = extrawidth;
   if ((i = j + 4 - boxwidth) > 0) {   /* expand box for title/extra */
      if (boxwidth + i > 80)
         i = 80 - boxwidth;
      boxwidth += i;
      boxcol -= i / 2;
      }
   i = (90 - boxwidth) / 20;
   boxcol    -= i;
   promptcol -= i;
   valuecol  -= i;

   /* display box heading */
   for (i = titlerow; i < boxrow; ++i)
      driver_set_attr(i,boxcol,C_PROMPT_HI,boxwidth);
   {
      char *hdgline = hdg;
      /* center each line of heading independently */
      int i;
      for(i=0;i<titlelines-1;i++)
      {
         char *next;
         if((next = strchr(hdgline,'\n')) == NULL)
            break; /* shouldn't happen */
         *next = '\0';
         titlewidth = (int) strlen(hdgline);
         textcbase = boxcol + (boxwidth - titlewidth) / 2;
         driver_put_string(titlerow+i,0,C_PROMPT_HI,hdgline);
         *next = '\n';
         hdgline = next+1;
      }
        /* add scrolling key message, if applicable */
      if(in_scrolling_mode) {
         *(hdgline + 31) = (char) 0;   /* replace the ')' */
         strcat(hdgline, ". CTRL+(direction key) to scroll text.)");
      }

      titlewidth = (int) strlen(hdgline);
      textcbase = boxcol + (boxwidth - titlewidth) / 2;
      driver_put_string(titlerow+i,0,C_PROMPT_HI,hdgline);
   }

   /* display extra info */
   if (extrainfo) {
#ifndef XFRACT
#define S1 '\xC4'
#define S2 "\xC0"
#define S3 "\xD9"
#define S4 "\xB3"
#define S5 "\xDA"
#define S6 "\xBF"
#else
#define S1 '-'
#define S2 "+" /* ll corner */
#define S3 "+" /* lr corner */
#define S4 "|"
#define S5 "+" /* ul corner */
#define S6 "+" /* ur corner */
#endif
      memset(buf,S1,80); buf[boxwidth-2] = 0;
      textcbase = boxcol + 1;
      driver_put_string(extrarow,0,C_PROMPT_BKGRD,buf);
      driver_put_string(extrarow+extralines-1,0,C_PROMPT_BKGRD,buf);
      --textcbase;
      driver_put_string(extrarow,0,C_PROMPT_BKGRD,S5);
      driver_put_string(extrarow+extralines-1,0,C_PROMPT_BKGRD,S2);
      textcbase += boxwidth - 1;
      driver_put_string(extrarow,0,C_PROMPT_BKGRD,S6);
      driver_put_string(extrarow+extralines-1,0,C_PROMPT_BKGRD,S3);

      textcbase = boxcol;

      for (i = 1; i < extralines-1; ++i) {
         driver_put_string(extrarow+i,0,C_PROMPT_BKGRD,S4);
         driver_put_string(extrarow+i,boxwidth-1,C_PROMPT_BKGRD,S4);
      }
      textcbase += (boxwidth - extrawidth) / 2;
      driver_put_string(extrarow+1,0,C_PROMPT_TEXT,extrainfo);
   }

   textcbase = 0;

   /* display empty box */
   for (i = 0; i < boxlines; ++i)
      driver_set_attr(boxrow+i,boxcol,C_PROMPT_LO,boxwidth);

   /* display initial values */
   for (i = 0; i < numprompts; i++) {
      driver_put_string(promptrow+i, promptcol, C_PROMPT_LO, prompts[i]);
      prompt_valuestring(buf,&values[i]);
      driver_put_string(promptrow+i, valuecol, C_PROMPT_LO, buf);
   }


   if (!anyinput) {
      putstringcenter(instrrow++,0,80,C_PROMPT_BKGRD,
        instr0);
      putstringcenter(instrrow,0,80,C_PROMPT_BKGRD,
        (helpmode > 0) ? instr0b : instr0a);
      driver_hide_text_cursor();
      textcbase = 2;
      for(;;) {
         if(rewrite_extrainfo) {
            rewrite_extrainfo = 0;
            fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(scroll_file, extrainfo, extralines - 2,
                        scroll_row_status, scroll_column_status);
            for(i=1; i <= extralines - 2; i++)
               driver_put_string(extrarow+i,0,C_PROMPT_TEXT,blanks);
            driver_put_string(extrarow+1,0,C_PROMPT_TEXT,extrainfo);
         }
        while (!keypressed()) { }
        done = getakey();
        switch(done) {
            case ESC:
               done = -1;
            case ENTER:
            case ENTER_2:
               goto fullscreen_exit;
            case DOWN_ARROW_2:    /* scrolling key - down one row */
               if(in_scrolling_mode && scroll_row_status < vertical_scroll_limit) {
                  scroll_row_status++;
                  rewrite_extrainfo = 1;
               }
               break;
            case UP_ARROW_2:      /* scrolling key - up one row */
               if(in_scrolling_mode && scroll_row_status > 0) {
                  scroll_row_status--;
                  rewrite_extrainfo = 1;
              }
              break;
            case LEFT_ARROW_2:    /* scrolling key - left one column */
               if(in_scrolling_mode && scroll_column_status > 0) {
                  scroll_column_status--;
                  rewrite_extrainfo = 1;
               }
               break;
            case RIGHT_ARROW_2:   /* scrolling key - right one column */
               if(in_scrolling_mode && strchr(extrainfo, '\021') != NULL) {
                  scroll_column_status++;
                  rewrite_extrainfo = 1;
               }
               break;
            case CTL_PAGE_DOWN:   /* scrolling key - down one screen */
               if(in_scrolling_mode && scroll_row_status < vertical_scroll_limit) {
                  scroll_row_status += extralines - 2;
                  if(scroll_row_status > vertical_scroll_limit)
                     scroll_row_status = vertical_scroll_limit;
                  rewrite_extrainfo = 1;
               }
               break;
            case CTL_PAGE_UP:     /* scrolling key - up one screen */
               if(in_scrolling_mode && scroll_row_status > 0) {
                  scroll_row_status -= extralines - 2;
                  if(scroll_row_status < 0)
                     scroll_row_status = 0;
                  rewrite_extrainfo = 1;
               }
               break;
            case CTL_END:         /* scrolling key - to end of entry */
               if(in_scrolling_mode) {
                  scroll_row_status = vertical_scroll_limit;
                  scroll_column_status = 0;
                  rewrite_extrainfo = 1;
               }
               break;
            case CTL_HOME:        /* scrolling key - to beginning of entry */
               if(in_scrolling_mode) {
                  scroll_row_status = scroll_column_status = 0;
                  rewrite_extrainfo = 1;
               }
               break;
            case F2:
            case F3:
            case F4:
            case F5:
            case F6:
            case F7:
            case F8:
            case F9:
            case F10:
               if (promptfkeys & (1<<(done+1-F1)) )
                  goto fullscreen_exit;
         }
      }
   }


   /* display footing */
   if (numprompts > 1)
      putstringcenter(instrrow++,0,80,C_PROMPT_BKGRD,instr1);
   putstringcenter(instrrow+1,0,80,C_PROMPT_BKGRD,
         (helpmode > 0) ? instr3b : instr3a);

   done = 0;
   while (values[curchoice].type == '*') ++curchoice;

   while (!done) {
      if(rewrite_extrainfo) {
         j = textcbase;
         textcbase = 2;
         fseek(scroll_file, scroll_file_start, SEEK_SET);
         load_entry_text(scroll_file, extrainfo, extralines - 2,
                             scroll_row_status, scroll_column_status);
         for(i=1; i <= extralines - 2; i++)
            driver_put_string(extrarow+i,0,C_PROMPT_TEXT,blanks);
         driver_put_string(extrarow+1,0,C_PROMPT_TEXT,extrainfo);
         textcbase = j;
      }

      curtype = values[curchoice].type;
      curlen = prompt_valuestring(buf,&values[curchoice]);
      if(!rewrite_extrainfo)
         putstringcenter(instrrow,0,80,C_PROMPT_BKGRD,
                   (curtype == 'l') ? instr2b : instr2a);
      else
         rewrite_extrainfo = 0;
      driver_put_string(promptrow+curchoice,promptcol,C_PROMPT_HI,prompts[curchoice]);

      if (curtype == 'l') {
         i = input_field_list(
                C_PROMPT_CHOOSE, buf, curlen,
                values[curchoice].uval.ch.list, values[curchoice].uval.ch.llen,
                promptrow+curchoice,valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
         for (j = 0; j < values[curchoice].uval.ch.llen; ++j)
            if (strcmp(buf,values[curchoice].uval.ch.list[j]) == 0) break;
         values[curchoice].uval.ch.val = j;
         }
      else {
         j = 0;
         if (curtype == 'i') j = 3;
         if (curtype == 'L') j = 3;
         if (curtype == 'd') j = 5;
         if (curtype == 'D') j = 7;
         if (curtype == 'f') j = 1;
         i = input_field(j, C_PROMPT_INPUT, buf, curlen,
                promptrow+curchoice,valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
         switch (values[curchoice].type) {
            case 'd':
            case 'D':
               values[curchoice].uval.dval = atof(buf);
               break;
            case 'f':
               values[curchoice].uval.dval = atof(buf);
               roundfloatd(&values[curchoice].uval.dval);
               break;
            case 'i':
               values[curchoice].uval.ival = atoi(buf);
               break;
            case 'L':
               values[curchoice].uval.Lval = atol(buf);
               break;
            case 's':
               strncpy(values[curchoice].uval.sval,buf,16);
               break;
            default: /* assume 0x100+n */
               strcpy(values[curchoice].uval.sbuf,buf);
            }
         }

      driver_put_string(promptrow+curchoice,promptcol,C_PROMPT_LO,prompts[curchoice]);
      j = (int) strlen(buf);
      memset(&buf[j],' ',80-j); buf[curlen] = 0;
      driver_put_string(promptrow+curchoice, valuecol, C_PROMPT_LO,  buf);

      switch(i) {
         case 0:  /* enter  */
            done = 13;
            break;
         case -1: /* escape */
         case F2:
         case F3:
         case F4:
         case F5:
         case F6:
         case F7:
         case F8:
         case F9:
         case F10:
            done = i;
            break;
         case PAGE_UP:
            curchoice = -1;
         case DOWN_ARROW:
            do {
               if (++curchoice >= numprompts) curchoice = 0;
               } while (values[curchoice].type == '*');
            break;
         case PAGE_DOWN:
            curchoice = numprompts;
         case UP_ARROW:
            do {
               if (--curchoice < 0) curchoice = numprompts - 1;
               } while (values[curchoice].type == '*');
            break;
         case DOWN_ARROW_2:     /* scrolling key - down one row */
            if(in_scrolling_mode && scroll_row_status < vertical_scroll_limit) {
               scroll_row_status++;
               rewrite_extrainfo = 1;
            }
            break;
         case UP_ARROW_2:       /* scrolling key - up one row */
            if(in_scrolling_mode && scroll_row_status > 0) {
               scroll_row_status--;
               rewrite_extrainfo = 1;
            }
            break;
         case LEFT_ARROW_2:     /*scrolling key - left one column */
            if(in_scrolling_mode && scroll_column_status > 0) {
               scroll_column_status--;
               rewrite_extrainfo = 1;
            }
            break;
         case RIGHT_ARROW_2:    /* scrolling key - right one column */
            if(in_scrolling_mode && strchr(extrainfo, '\021') != NULL) {
               scroll_column_status++;
               rewrite_extrainfo = 1;
            }
            break;
         case CTL_PAGE_DOWN:    /* scrolling key - down on screen */
            if(in_scrolling_mode && scroll_row_status < vertical_scroll_limit) {
               scroll_row_status += extralines - 2;
               if(scroll_row_status > vertical_scroll_limit)
                  scroll_row_status = vertical_scroll_limit;
               rewrite_extrainfo = 1;
            }
            break;
         case CTL_PAGE_UP:      /* scrolling key - up one screen */
            if(in_scrolling_mode && scroll_row_status > 0) {
                 scroll_row_status -= extralines - 2;
               if(scroll_row_status < 0)
                  scroll_row_status = 0;
               rewrite_extrainfo = 1;
            }
            break;
         case CTL_END:          /* scrolling key - go to end of entry */
            if(in_scrolling_mode) {
               scroll_row_status = vertical_scroll_limit;
               scroll_column_status = 0;
               rewrite_extrainfo = 1;
            }
            break;
         case CTL_HOME:         /* scrolling key - go to beginning of entry */
            if(in_scrolling_mode) {
               scroll_row_status = scroll_column_status = 0;
               rewrite_extrainfo = 1;
            }
            break;
      }
   }

fullscreen_exit:
   driver_hide_text_cursor();
   lookatmouse = savelookatmouse;
   if(scroll_file) {
      fclose(scroll_file);
      scroll_file = NULL;
   }
   return(done);
}

int prompt_valuestring(char *buf,struct fullscreenvalues *val)
{  /* format value into buf, return field width */
   int i,ret;
   switch (val->type) {
      case 'd':
         ret = 20;
         i = 16;    /* cellular needs 16 (was 15)*/
         for(;;) {
            sprintf(buf,"%.*g",i,val->uval.dval);
            if ((int)strlen(buf) <= ret) break;
            --i;
            }
         break;
      case 'D':
         if (val->uval.dval<0) { /* We have to round the right way */
             sprintf(buf,"%ld",(long)(val->uval.dval-.5));
         }
         else {
             sprintf(buf,"%ld",(long)(val->uval.dval+.5));
         }
         ret = 20;
         break;
      case 'f':
         sprintf(buf,"%.7g",val->uval.dval);
         ret = 14;
         break;
      case 'i':
         sprintf(buf,"%d",val->uval.ival);
         ret = 6;
         break;
      case 'L':
         sprintf(buf,"%ld",val->uval.Lval);
         ret = 10;
         break;
      case '*':
         *buf = (char)(ret = 0);
         break;
      case 's':
         strncpy(buf,val->uval.sval,16);
         buf[15] = 0;
         ret = 15;
         break;
      case 'l':
         strcpy(buf,val->uval.ch.list[val->uval.ch.val]);
         ret = val->uval.ch.vlen;
         break;
      default: /* assume 0x100+n */
         strcpy(buf,val->uval.sbuf);
         ret = val->type & 0xff;
      }
   return ret;
}

int prompt_checkkey(int curkey)
{
   switch(curkey) {
      case PAGE_UP:
      case DOWN_ARROW:
      case PAGE_DOWN:
      case UP_ARROW:
         return(curkey);
      case F2:
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
      case F10:
         if (promptfkeys & (1<<(curkey+1-F1)) )
            return(curkey);
      }
   return(0);
}

int prompt_checkkey_scroll(int curkey)
{
   switch(curkey) {
      case PAGE_UP:
      case DOWN_ARROW:
      case DOWN_ARROW_2:
      case PAGE_DOWN:
      case UP_ARROW:
      case UP_ARROW_2:
      case LEFT_ARROW_2:
      case RIGHT_ARROW_2:
      case CTL_PAGE_DOWN:
      case CTL_PAGE_UP:
      case CTL_END:
      case CTL_HOME:
         return(curkey);
      case F2:
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
      case F10:
         if (promptfkeys & (1<<(curkey+1-F1)) )
            return(curkey);
      }
   return(0);
}

static int input_field_list(
        int attr,             /* display attribute */
        char *fld,            /* display form field value */
        int vlen,             /* field length */
        char **list,          /* list of values */
        int llen,             /* number of entries in list */
        int row,              /* display row */
        int col,              /* display column */
        int (*checkkey)(int)  /* routine to check non data keys, or NULL */
        )
{
   int initval,curval;
   char buf[81];
   int curkey;
   int i, j;
   int ret,savelookatmouse;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   for (initval = 0; initval < llen; ++initval)
      if (strcmp(fld,list[initval]) == 0) break;
   if (initval >= llen) initval = 0;
   curval = initval;
   ret = -1;
   for(;;) {
      strcpy(buf,list[curval]);
      i = (int) strlen(buf);
      while (i < vlen)
         buf[i++] = ' ';
      buf[vlen] = 0;
      driver_put_string(row,col,attr,buf);
      curkey = keycursor(row,col); /* get a keystroke */
      switch (curkey) {
         case ENTER:
         case ENTER_2:
            ret = 0;
            goto inpfldl_end;
         case ESC:
            goto inpfldl_end;
         case RIGHT_ARROW:
            if (++curval >= llen)
               curval = 0;
            break;
         case LEFT_ARROW:
            if (--curval < 0)
               curval = llen - 1;
            break;
         case F5:
            curval = initval;
            break;
         default:
            if (nonalpha(curkey)) {
               if (checkkey && (ret = (*checkkey)(curkey)) != 0)
                  goto inpfldl_end;
               break;                                /* non alphanum char */
               }
            j = curval;
            for (i = 0; i < llen; ++i) {
               if (++j >= llen)
                  j = 0;
               if ( (*list[j] & 0xdf) == (curkey & 0xdf)) {
                  curval = j;
                  break;
                  }
               }
         }
      }
inpfldl_end:
   strcpy(fld,list[curval]);
   lookatmouse = savelookatmouse;
   return(ret);
}


/* --------------------------------------------------------------------- */

/* MCP 7-7-91, This is static code, but not called anywhere */
#ifdef DELETE_UNUSED_CODE

/* compare for sort of type table */
static int compare(const VOIDPTR i, const VOIDPTR j)
{
   return(strcmp(fractalspecific[(int)*((BYTE*)i)].name,
               fractalspecific[(int)*((BYTE*)j)].name));
}

/* --------------------------------------------------------------------- */

static void clear_line(int row, int start, int stop, int color) /* clear part of a line */
{
   int col;
   for(col=start;col<= stop;col++)
      driver_put_string(row,col,color," ");
}

#endif

/* --------------------------------------------------------------------- */

int get_fracttype()             /* prompt for and select fractal type */
{
   int done,i,oldfractype,t;
   done = -1;
   oldfractype = fractype;
   for(;;) {
      if ((t = select_fracttype(fractype)) < 0)
         break;
      if ((i = select_type_params(t, fractype)) == 0) { /* ok, all done */
         done = 0;
         break;
         }
      if (i > 0) /* can't return to prior image anymore */
         done = 1;
      }
   if (done < 0)
      fractype = oldfractype;
   curfractalspecific = &fractalspecific[fractype];
   return(done);
}

struct FT_CHOICE {
      char name[15];
      int  num;
      };
static struct FT_CHOICE **ft_choices; /* for sel_fractype_help subrtn */

static int select_fracttype(int t) /* subrtn of get_fracttype, separated */
                                   /* so that storage gets freed up      */
{
   static FCODE head1[] = {"Select a Fractal Type"};
   static FCODE head2[] = {"Select Orbit Algorithm for Julibrot"};
   static FCODE o_instr[] = {"Press "FK_F2" for a description of the highlighted type"};
   char instr[sizeof(o_instr)];
   char head[40];
   int oldhelpmode;
   int numtypes, done;
   int i, j;
#define MAXFTYPES 200
   char tname[40];
   struct FT_CHOICE *choices[MAXFTYPES];
   int attributes[MAXFTYPES];

   /* steal existing array for "choices" */
   choices[0] = (struct FT_CHOICE *)boxy;
   attributes[0] = 1;
   for (i = 1; i < MAXFTYPES; ++i) {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
      }
   ft_choices = &choices[0];

   /* setup context sensitive help */
   oldhelpmode = helpmode;
   helpmode = HELPFRACTALS;
   strcpy(instr,o_instr);
   if(julibrot)
      strcpy(head,head2);
   else
      strcpy(head,head1);
   if (t == IFS3D) t = IFS;
   i = j = -1;
   while(fractalspecific[++i].name) {
      if(julibrot)
        if (!((fractalspecific[i].flags & OKJB) && *fractalspecific[i].name != '*'))
           continue;
      if (fractalspecific[i].name[0] == '*')
         continue;
      strcpy(choices[++j]->name,fractalspecific[i].name);
      choices[j]->name[14] = 0; /* safety */
      choices[j]->num = i;      /* remember where the real item is */
      }
   numtypes = j + 1;
   shell_sort(choices,numtypes,sizeof(char *),lccompare); /* sort list */
   j = 0;
   for (i = 0; i < numtypes; ++i) /* find starting choice in sorted list */
      if (choices[i]->num == t || choices[i]->num == fractalspecific[t].tofloat)
         j = i;

   tname[0] = 0;
   done = fullscreen_choice(CHOICEHELP+8,head,NULL,instr,numtypes,
         (char **)choices,attributes,0,0,0,j,NULL,tname,NULL,sel_fractype_help);
   if (done >= 0) {
      done = choices[done]->num;
      if((done == FORMULA || done == FFORMULA) && !strcmp(FormFileName, CommandFile))
         strcpy(FormFileName, searchfor.frm);
      if(done == LSYSTEM && !strcmp(LFileName, CommandFile))
         strcpy(LFileName, searchfor.lsys);
      if((done == IFS || done == IFS3D) && !strcmp(IFSFileName, CommandFile))
         strcpy(IFSFileName, searchfor.ifs);
   }


   helpmode = oldhelpmode;
   return(done);
}

static int sel_fractype_help(int curkey,int choice)
{
   int oldhelpmode;
   if (curkey == F2) {
      oldhelpmode = helpmode;
      helpmode = fractalspecific[(*(ft_choices+choice))->num].helptext;
      help(0);
      helpmode = oldhelpmode;
      }
   return(0);
}

int select_type_params( /* prompt for new fractal type parameters */
        int newfractype,        /* new fractal type */
        int oldfractype         /* previous fractal type */
        )
{
   int ret,oldhelpmode;

   oldhelpmode = helpmode;
sel_type_restart:
   ret = 0;
   fractype = newfractype;
   curfractalspecific = &fractalspecific[fractype];

   if (fractype == LSYSTEM) {
      helpmode = HT_LSYS;
      if (get_file_entry(GETLSYS,"L-System",lsysmask,LFileName,LName) < 0) {
         ret = 1;
         goto sel_type_exit;
         }
      }
   if (fractype == FORMULA || fractype == FFORMULA) {
      helpmode = HT_FORMULA;
      if (get_file_entry(GETFORMULA,"Formula",formmask,FormFileName,FormName) < 0) {
         ret = 1;
         goto sel_type_exit;
         }
      }
   if (fractype == IFS || fractype == IFS3D) {
      helpmode = HT_IFS;
      if (get_file_entry(GETIFS,"IFS",ifsmask,IFSFileName,IFSName) < 0) {
        ret = 1;
        goto sel_type_exit;
        }
      }

/* Added the following to accommodate fn bifurcations.  JCO 7/2/92 */
   if(((fractype == BIFURCATION) || (fractype == LBIFURCATION)) &&
     !((oldfractype == BIFURCATION) || (oldfractype == LBIFURCATION)))
        set_trig_array(0,s_ident);
   if(((fractype == BIFSTEWART) || (fractype == LBIFSTEWART)) &&
     !((oldfractype == BIFSTEWART) || (oldfractype == LBIFSTEWART)))
        set_trig_array(0,s_ident);
   if(((fractype == BIFLAMBDA) || (fractype == LBIFLAMBDA)) &&
     !((oldfractype == BIFLAMBDA) || (oldfractype == LBIFLAMBDA)))
        set_trig_array(0,s_ident);
   if(((fractype == BIFEQSINPI) || (fractype == LBIFEQSINPI)) &&
     !((oldfractype == BIFEQSINPI) || (oldfractype == LBIFEQSINPI)))
        set_trig_array(0,s_sin);
   if(((fractype == BIFADSINPI) || (fractype == LBIFADSINPI)) &&
     !((oldfractype == BIFADSINPI) || (oldfractype == LBIFADSINPI)))
        set_trig_array(0,s_sin);

   /* 
    * Next assumes that user going between popcorn and popcornjul
    * might not want to change function variables 
    */
   if(((fractype    == FPPOPCORN   ) || (fractype    == LPOPCORN   ) ||
       (fractype    == FPPOPCORNJUL) || (fractype    == LPOPCORNJUL)) &&
     !((oldfractype == FPPOPCORN   ) || (oldfractype == LPOPCORN   ) ||
       (oldfractype == FPPOPCORNJUL) || (oldfractype == LPOPCORNJUL)))
      set_function_parm_defaults();
        
   /* set LATOO function defaults */     
   if(fractype == LATOO && oldfractype != LATOO)
   {
      set_function_parm_defaults();
   }
   set_default_parms();

   if (get_fract_params(0) < 0)
      if (fractype == FORMULA || fractype == FFORMULA ||
          fractype == IFS || fractype == IFS3D ||
          fractype == LSYSTEM)
         goto sel_type_restart;
      else
         ret = 1;
   else {
      if (newfractype != oldfractype) {
         invert = 0;
         inversion[0] = inversion[1] = inversion[2] = 0;
         }
      }

sel_type_exit:
   helpmode = oldhelpmode;
   return(ret);

}

void set_default_parms()
{
   int i,extra;
   xxmin = curfractalspecific->xmin;
   xxmax = curfractalspecific->xmax;
   yymin = curfractalspecific->ymin;
   yymax = curfractalspecific->ymax;
   xx3rd = xxmin;
   yy3rd = yymin;

   if (viewcrop && finalaspectratio != screenaspect)
      aspectratio_crop(screenaspect,finalaspectratio);
   for (i = 0; i < 4; i++) {
      param[i] = curfractalspecific->paramvalue[i];
      if (fractype != CELLULAR && fractype != FROTH && fractype != FROTHFP &&
          fractype != ANT)
         roundfloatd(&param[i]); /* don't round cellular, frothybasin or ant */
   }
   if((extra=find_extra_param(fractype)) > -1)
      for(i=0;i<MAXPARAMS-4;i++)
         param[i+4] = moreparams[extra].paramvalue[i];
   if(debugflag != 3200)
      bf_math = 0;
   else if(bf_math)
      fractal_floattobf();
}

#define MAXFRACTALS 25

int build_fractal_list(int fractals[], int *last_val, char *nameptr[])
{
    int numfractals,i;

    numfractals = 0;
    for (i = 0; i < num_fractal_types; i++)
    {
        if ((fractalspecific[i].flags & OKJB) && *fractalspecific[i].name != '*')
        {
            fractals[numfractals] = i;
            if (i == neworbittype || i == fractalspecific[neworbittype].tofloat)
                *last_val = numfractals;
            nameptr[numfractals] = fractalspecific[i].name;
            numfractals++;
            if (numfractals >= MAXFRACTALS)
                break;
        }
    }
    return (numfractals);
}

static FCODE v0a[] = {"From cx (real part)"};
static FCODE v1a[] = {"From cy (imaginary part)"};
static FCODE v2a[] = {"To   cx (real part)"};
static FCODE v3a[] = {"To   cy (imaginary part)"};

/* 4D Mandelbrot */
static FCODE v0b[] = {"From cj (3rd dim)"};
static FCODE v1b[] = {"From ck (4th dim)"};
static FCODE v2b[] = {"To   cj (3rd dim)"};
static FCODE v3b[] = {"To   ck (4th dim)"};

/* 4D Julia */
static FCODE v0c[] = {"From zj (3rd dim)"};
static FCODE v1c[] = {"From zk (4th dim)"};
static FCODE v2c[] = {"To   zj (3rd dim)"};
static FCODE v3c[] = {"To   zk (4th dim)"};

static FCODE v4[] = {"Number of z pixels"};
static FCODE v5[] = {"Location of z origin"};
static FCODE v6[] = {"Depth of z"};
static FCODE v7[] = {"Screen height"};
static FCODE v8[] = {"Screen width"};
static FCODE v9[] = {"Distance to Screen"};
static FCODE v10[] = {"Distance between eyes"};
static FCODE v11[] = {"3D Mode"};
char *juli3Doptions[] = {"monocular","lefteye","righteye","red-blue"};

/* JIIM */
#ifdef RANDOM_RUN
static FCODE JIIMstr1[] = "Breadth first, Depth first, Random Walk, Random Run?";
char *JIIMmethod[] = {"breadth", "depth", "walk", "run"};
#else
static FCODE JIIMstr1[] = "Breadth first, Depth first, Random Walk";
char *JIIMmethod[] = {"breadth", "depth", "walk"};
#endif
static FCODE JIIMstr2[] = "Left first or Right first?";
char *JIIMleftright[] = {"left", "right"};

/* moved from miscres.c so sizeof structure can be accessed here */
struct trig_funct_lst trigfn[] =
/* changing the order of these alters meaning of *.fra file */
/* maximum 6 characters in function names or recheck all related code */
{
#if !defined(XFRACT) && !defined(_WIN32)
   {s_sin,   lStkSin,   dStkSin,   mStkSin   },
   {s_cosxx, lStkCosXX, dStkCosXX, mStkCosXX },
   {s_sinh,  lStkSinh,  dStkSinh,  mStkSinh  },
   {s_cosh,  lStkCosh,  dStkCosh,  mStkCosh  },
   {s_exp,   lStkExp,   dStkExp,   mStkExp   },
   {s_log,   lStkLog,   dStkLog,   mStkLog   },
   {s_sqr,   lStkSqr,   dStkSqr,   mStkSqr   },
   {s_recip, lStkRecip, dStkRecip, mStkRecip }, /* from recip on new in v16 */
   {s_ident, StkIdent,  StkIdent,  StkIdent  },
   {s_cos,   lStkCos,   dStkCos,   mStkCos   },
   {s_tan,   lStkTan,   dStkTan,   mStkTan   },
   {s_tanh,  lStkTanh,  dStkTanh,  mStkTanh  },
   {s_cotan, lStkCoTan, dStkCoTan, mStkCoTan },
   {s_cotanh,lStkCoTanh,dStkCoTanh,mStkCoTanh},
   {s_flip,  lStkFlip,  dStkFlip,  mStkFlip  },
   {s_conj,  lStkConj,  dStkConj,  mStkConj  },
   {s_zero,  lStkZero,  dStkZero,  mStkZero  },
   {s_asin,  lStkASin,  dStkASin,  mStkASin  },
   {s_asinh, lStkASinh, dStkASinh, mStkASinh },
   {s_acos,  lStkACos,  dStkACos,  mStkACos  },
   {s_acosh, lStkACosh, dStkACosh, mStkACosh },
   {s_atan,  lStkATan,  dStkATan,  mStkATan  },
   {s_atanh, lStkATanh, dStkATanh, mStkATanh },
   {s_cabs,  lStkCAbs,  dStkCAbs,  mStkCAbs  },
   {s_abs,   lStkAbs,   dStkAbs,   mStkAbs   },
   {s_sqrt,  lStkSqrt,  dStkSqrt,  mStkSqrt  },
   {s_floor, lStkFloor, dStkFloor, mStkFloor },
   {s_ceil,  lStkCeil,  dStkCeil,  mStkCeil  },
   {s_trunc, lStkTrunc, dStkTrunc, mStkTrunc },
   {s_round, lStkRound, dStkRound, mStkRound },
   {s_one,   lStkOne,   dStkOne,   mStkOne   },
#else
   {s_sin,   dStkSin,   dStkSin,   dStkSin   },
   {s_cosxx, dStkCosXX, dStkCosXX, dStkCosXX },
   {s_sinh,  dStkSinh,  dStkSinh,  dStkSinh  },
   {s_cosh,  dStkCosh,  dStkCosh,  dStkCosh  },
   {s_exp,   dStkExp,   dStkExp,   dStkExp   },
   {s_log,   dStkLog,   dStkLog,   dStkLog   },
   {s_sqr,   dStkSqr,   dStkSqr,   dStkSqr   },
   {s_recip, dStkRecip, dStkRecip, dStkRecip }, /* from recip on new in v16 */
   {s_ident, StkIdent,  StkIdent,  StkIdent  },
   {s_cos,   dStkCos,   dStkCos,   dStkCos   },
   {s_tan,   dStkTan,   dStkTan,   dStkTan   },
   {s_tanh,  dStkTanh,  dStkTanh,  dStkTanh  },
   {s_cotan, dStkCoTan, dStkCoTan, dStkCoTan },
   {s_cotanh,dStkCoTanh,dStkCoTanh,dStkCoTanh},
   {s_flip,  dStkFlip,  dStkFlip,  dStkFlip  },
   {s_conj,  dStkConj,  dStkConj,  dStkConj  },
   {s_zero,  dStkZero,  dStkZero,  dStkZero  },
   {s_asin,  dStkASin,  dStkASin,  dStkASin  },
   {s_asinh, dStkASinh, dStkASinh, dStkASinh },
   {s_acos,  dStkACos,  dStkACos,  dStkACos  },
   {s_acosh, dStkACosh, dStkACosh, dStkACosh },
   {s_atan,  dStkATan,  dStkATan,  dStkATan  },
   {s_atanh, dStkATanh, dStkATanh, dStkATanh },
   {s_cabs,  dStkCAbs,  dStkCAbs,  dStkCAbs  },
   {s_abs,   dStkAbs,   dStkAbs,   dStkAbs   },
   {s_sqrt,  dStkSqrt,  dStkSqrt,  dStkSqrt  },
   {s_floor, dStkFloor, dStkFloor, dStkFloor },
   {s_ceil,  dStkCeil,  dStkCeil,  dStkCeil  },
   {s_trunc, dStkTrunc, dStkTrunc, dStkTrunc },
   {s_round, dStkRound, dStkRound, dStkRound },
   {s_one,   dStkOne,   dStkOne,   dStkOne   },
#endif
};

#define NUMTRIGFN  sizeof(trigfn)/sizeof(struct trig_funct_lst)

const int numtrigfn = NUMTRIGFN;

/* --------------------------------------------------------------------- */
int get_fract_params(int caller)        /* prompt for type-specific parms */
{
   char *v0 = v0a;
   char *v1 = v1a;
   char *v2 = v2a;
   char *v3 = v3a;
   char *juliorbitname = NULL;
   int i,j,k;
   int curtype,numparams,numtrig;
   struct fullscreenvalues paramvalues[30];
   char *choices[30];
   long oldbailout = 0L;
   int promptnum;
   char msg[120];
   char *typename, *tmpptr;
   char bailoutmsg[50];
   int ret = 0;
   int oldhelpmode;
   char parmprompt[MAXPARAMS][55];
   static FCODE t1[] = {"First Function"};
   static FCODE t2[] = {"Second Function"};
   static FCODE t3[] = {"Third Function"};
   static FCODE t4[] = {"Fourth Function"};
   static FCODE *trg[] = {t1, t2, t3, t4};
   char *filename,*entryname;
   FILE *entryfile;
   char *trignameptr[NUMTRIGFN];
#ifdef XFRACT
   static /* Can't initialize aggregates on the stack */
#endif
   char *bailnameptr[] = {s_mod,s_real,s_imag,s_or,s_and,s_manh,s_manr};
   struct fractalspecificstuff *jborbit = NULL;
   struct fractalspecificstuff *savespecific;
   int firstparm = 0;
   int lastparm  = MAXPARAMS;
   double oldparam[MAXPARAMS];
   int fkeymask = 0x40;
   oldbailout = bailout;
   if(fractype==JULIBROT || fractype==JULIBROTFP)
      julibrot = 1;
   else
      julibrot = 0;
   curtype = fractype;
   if (curfractalspecific->name[0] == '*'
     && (i = curfractalspecific->tofloat) != NOFRACTAL  /* FIXED BUG HERE!! */
     && fractalspecific[i].name[0] != '*')
      curtype = i;
   curfractalspecific = &fractalspecific[curtype];
   tstack[0] = 0;
   if ((i = curfractalspecific->helpformula) < -1) {
      if (i == -2) { /* special for formula */
         filename = FormFileName;
         entryname = FormName;
         }
      else if (i == -3)  {       /* special for lsystem */
         filename = LFileName;
         entryname = LName;
         }
      else if (i == -4)  {       /* special for ifs */
         filename = IFSFileName;
         entryname = IFSName;
         }
      else { /* this shouldn't happen */
         filename = NULL;
         entryname = NULL;
      }
      if (find_file_item(filename,entryname,&entryfile, -1-i) == 0) {
         load_entry_text(entryfile,tstack,17, 0, 0);
         fclose(entryfile);
         if(fractype == FORMULA || fractype == FFORMULA)
           frm_get_param_stuff(entryname); /* no error check, should be okay, from above */
         }
      }
   else if (i >= 0) {
      int c,lines;
      read_help_topic(i,0,2000,tstack); /* need error handling here ?? */
      tstack[2000-i] = 0;
      i = j = lines = 0; k = 1;
      while ((c = tstack[i++]) != 0) {
         /* stop at ctl, blank, or line with col 1 nonblank, max 16 lines */
         if (k && c == ' ' && ++k <= 5) { } /* skip 4 blanks at start of line */
         else {
            if (c == '\n') {
               if (k) break; /* blank line */
               if (++lines >= 16) break;
               k = 1;
               }
            else if (c < 16) /* a special help format control char */
               break;
            else {
               if (k == 1) /* line starts in column 1 */
                  break;
               k = 0;
               }
            tstack[j++] = (char)c;
            }
         }
      while (--j >= 0 && tstack[j] == '\n') { }
      tstack[j+1] = 0;
      }
gfp_top:
   promptnum = 0;
   if (julibrot)
   {
      i = select_fracttype(neworbittype);
      if (i < 0)
      {
         if (ret == 0)
            ret = -1;
         julibrot = 0;
         goto gfp_exit;
      }
      else
         neworbittype = i;
      jborbit = &fractalspecific[neworbittype];
      juliorbitname = jborbit->name;
   }

   if(fractype == FORMULA || fractype == FFORMULA) {
      if(uses_p1)  /* set first parameter */
         firstparm = 0;
      else if(uses_p2)
         firstparm = 2;
      else if(uses_p3)
         firstparm = 4;
      else if(uses_p4)
         firstparm = 6;
      else
         firstparm = 8; /* uses_p5 or no parameter */

      if(uses_p5)  /* set last parameter */
         lastparm = 10;
      else if(uses_p4)
         lastparm = 8;
      else if(uses_p3)
         lastparm = 6;
      else if(uses_p2)
         lastparm = 4;
      else
         lastparm = 2; /* uses_p1 or no parameter */
   }

   savespecific = curfractalspecific;
   if(julibrot)
   {
      curfractalspecific = jborbit;
      firstparm = 2; /* in most case Julibrot does not need first two parms */
      if(neworbittype == QUATJULFP     ||   /* all parameters needed */
         neworbittype == HYPERCMPLXJFP)
      {
         firstparm = 0;
         lastparm = 4;
      }
      if(neworbittype == QUATFP        ||   /* no parameters needed */
         neworbittype == HYPERCMPLXFP)
         firstparm = 4;
   }
   numparams = 0;
   j = 0;
   for (i = firstparm; i < lastparm; i++)
   {
      char tmpbuf[30];
      if (!typehasparm(julibrot?neworbittype:fractype,i,parmprompt[j])) {
         if(curtype == FORMULA || curtype == FFORMULA)
           if(paramnotused(i))
              continue;
         break;
      }
      numparams++;
      choices[promptnum] = parmprompt[j++];
      paramvalues[promptnum].type = 'd';

      if (choices[promptnum][0] == '+')
      {
         choices[promptnum]++;
         paramvalues[promptnum].type = 'D';
      }
      else if (choices[promptnum][0] == '#')
         choices[promptnum]++;
      sprintf(tmpbuf,"%.17g",param[i]);
      paramvalues[promptnum].uval.dval = atof(tmpbuf);
      oldparam[i] = paramvalues[promptnum++].uval.dval;
   }

/* The following is a goofy kludge to make reading in the formula
 * parameters work.
 */
   if(curtype == FORMULA || curtype == FFORMULA)
      numparams = lastparm - firstparm;

   numtrig = (curfractalspecific->flags >> 6) & 7;
   if(curtype==FORMULA || curtype==FFORMULA ) {
      numtrig = maxfn;
      }

   i = NUMTRIGFN;
   while (--i >= 0)
      trignameptr[i] = trigfn[i].name;
   for (i = 0; i < numtrig; i++) {
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = trigndx[i];
      paramvalues[promptnum].uval.ch.llen = NUMTRIGFN;
      paramvalues[promptnum].uval.ch.vlen = 6;
      paramvalues[promptnum].uval.ch.list = trignameptr;
      choices[promptnum++] = (char *)trg[i];
      }
   if (*(typename = curfractalspecific->name) == '*')
        ++typename;

   i = curfractalspecific->orbit_bailout;

   if( i != 0 && curfractalspecific->calctype == StandardFractal &&
       (curfractalspecific->flags & BAILTEST) ) {
        static FCODE bailteststr[] = {"Bailout Test (mod, real, imag, or, and, manh, manr)"};
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = (int)bailoutest;
      paramvalues[promptnum].uval.ch.llen = 7;
      paramvalues[promptnum].uval.ch.vlen = 6;
      paramvalues[promptnum].uval.ch.list = bailnameptr;
      choices[promptnum++] = bailteststr;
   }

   if (i) {
      if (potparam[0] != 0.0 && potparam[2] != 0.0)
      {
        static FCODE bailpotstr[] = {"Bailout: continuous potential (Y screen) value in use"};
         paramvalues[promptnum].type = '*';
         choices[promptnum++] = bailpotstr;
      }
      else
      {
         static FCODE bailoutstr[] = {"Bailout value (0 means use default)"};
         choices[promptnum] = bailoutstr;
         paramvalues[promptnum].type = 'L';
         paramvalues[promptnum++].uval.Lval = (oldbailout = bailout);
         paramvalues[promptnum].type = '*';
         tmpptr = typename;
         if (usr_biomorph != -1)
         {
            i = 100;
            tmpptr = "biomorph";
         }
         sprintf(bailoutmsg,"    (%s default is %d)",tmpptr,i);
         choices[promptnum++] = bailoutmsg;
      }
   }
   if (julibrot)
   {
      switch(neworbittype)
      {
      case QUATFP:
      case HYPERCMPLXFP:
          v0 = v0b; v1 = v1b; v2 = v2b; v3 = v3b;
          break;
      case QUATJULFP:
      case HYPERCMPLXJFP:
          v0 = v0c; v1 = v1c; v2 = v2c; v3 = v3c;
          break;
      default:
          v0 = v0a; v1 = v1a; v2 = v2a; v3 = v3a;
         break;
      }

      curfractalspecific = savespecific;
      paramvalues[promptnum].uval.dval = mxmaxfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v0;
      paramvalues[promptnum].uval.dval = mymaxfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v1;
      paramvalues[promptnum].uval.dval = mxminfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v2;
      paramvalues[promptnum].uval.dval = myminfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v3;
      paramvalues[promptnum].uval.ival = zdots;
      paramvalues[promptnum].type = 'i';
      choices[promptnum++] = v4;

      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.val  = juli3Dmode;
      paramvalues[promptnum].uval.ch.llen = 4;
      paramvalues[promptnum].uval.ch.vlen = 9;
      paramvalues[promptnum].uval.ch.list = juli3Doptions;
      choices[promptnum++] = v11;

      paramvalues[promptnum].uval.dval = eyesfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v10;
      paramvalues[promptnum].uval.dval = originfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v5;
      paramvalues[promptnum].uval.dval = depthfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v6;
      paramvalues[promptnum].uval.dval = heightfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v7;
      paramvalues[promptnum].uval.dval = widthfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v8;
      paramvalues[promptnum].uval.dval = distfp;
      paramvalues[promptnum].type = 'f';
      choices[promptnum++] = v9;
   }

   if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
   {
      choices[promptnum] = JIIMstr1;
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.list = JIIMmethod;
      paramvalues[promptnum].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
      paramvalues[promptnum].uval.ch.llen = 4;
#else
      paramvalues[promptnum].uval.ch.llen = 3; /* disable random run */
#endif
      paramvalues[promptnum++].uval.ch.val  = major_method;

      choices[promptnum] = JIIMstr2;
      paramvalues[promptnum].type = 'l';
      paramvalues[promptnum].uval.ch.list = JIIMleftright;
      paramvalues[promptnum].uval.ch.vlen = 5;
      paramvalues[promptnum].uval.ch.llen = 2;
      paramvalues[promptnum++].uval.ch.val  = minor_method;
   }

   if((curtype==FORMULA || curtype==FFORMULA) && uses_ismand) {
      choices[promptnum] = (char *)s_ismand;
      paramvalues[promptnum].type = 'y';
      paramvalues[promptnum++].uval.ch.val = ismand?1:0;
   }

   if (caller                           /* <z> command ? */
/*      && (display3d > 0 || promptnum == 0)) */
      && (display3d > 0))
      {
       static FCODE msg[]={"Current type has no type-specific parameters"};
       stopmsg(20,msg);
       goto gfp_exit;
       }
   if(julibrot)
      sprintf(msg,"Julibrot Parameters (orbit= %s)",juliorbitname);
   else
      sprintf(msg,"Parameters for fractal type %s",typename);
   if(bf_math == 0)
   {
      static FCODE pressf6[] = {"\n(Press "FK_F6" for corner parameters)"};
      strcat(msg,pressf6);
   }
   else
      fkeymask = 0;
   scroll_row_status = 0; /* make sure we start at beginning of entry */
   scroll_column_status = 0;
   for(;;)
   {
      oldhelpmode = helpmode;
      helpmode = curfractalspecific->helptext;
      i = fullscreen_prompt(msg,promptnum,choices,paramvalues,fkeymask,tstack);
      helpmode = oldhelpmode;
      if (i < 0)
      {
         if(julibrot)
           goto gfp_top;
         if (ret == 0)
            ret = -1;
         goto gfp_exit;
      }
      if (i != F6)
         break;
      if(bf_math == 0)
         if (get_corners() > 0)
            ret = 1;
     }
     promptnum = 0;
     for ( i = firstparm; i < numparams+firstparm; i++)
     {
        if(curtype == FORMULA || curtype == FFORMULA)
           if(paramnotused(i))
              continue;
        if (oldparam[i] != paramvalues[promptnum].uval.dval)
        {
           param[i] = paramvalues[promptnum].uval.dval;
           ret = 1;
        }
        ++promptnum;
    }

   for ( i = 0; i < numtrig; i++)
   {
      if (paramvalues[promptnum].uval.ch.val != (int)trigndx[i])
      {
         set_trig_array(i,trigfn[paramvalues[promptnum].uval.ch.val].name);
         ret = 1;
      }
      ++promptnum;
   }

   if(julibrot)
   {
      savespecific = curfractalspecific;
      curfractalspecific = jborbit;
   }

   i = curfractalspecific->orbit_bailout;

   if( i != 0 && curfractalspecific->calctype == StandardFractal &&
       (curfractalspecific->flags & BAILTEST) ) {
      if (paramvalues[promptnum].uval.ch.val != (int)bailoutest) {
        bailoutest = (enum bailouts)paramvalues[promptnum].uval.ch.val;
        ret = 1;
      }
      promptnum++;
   }
   else
      bailoutest = Mod;
   setbailoutformula(bailoutest);

   if (i) {
      if (potparam[0] != 0.0 && potparam[2] != 0.0)
         promptnum++;
      else
      {
         bailout = paramvalues[promptnum++].uval.Lval;
         if (bailout != 0 && (bailout < 1 || bailout > 2100000000L))
            bailout = oldbailout;
         if (bailout != oldbailout)
            ret = 1;
         promptnum++;
      }
   }
   if (julibrot)
     {
        mxmaxfp    = paramvalues[promptnum++].uval.dval;
        mymaxfp    = paramvalues[promptnum++].uval.dval;
        mxminfp    = paramvalues[promptnum++].uval.dval;
        myminfp    = paramvalues[promptnum++].uval.dval;
        zdots      = paramvalues[promptnum++].uval.ival;
        juli3Dmode = paramvalues[promptnum++].uval.ch.val;
        eyesfp     = (float)paramvalues[promptnum++].uval.dval;
        originfp   = (float)paramvalues[promptnum++].uval.dval;
        depthfp    = (float)paramvalues[promptnum++].uval.dval;
        heightfp   = (float)paramvalues[promptnum++].uval.dval;
        widthfp    = (float)paramvalues[promptnum++].uval.dval;
        distfp     = (float)paramvalues[promptnum++].uval.dval;
        ret = 1;  /* force new calc since not resumable anyway */
     }
      if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
      {
         if (paramvalues[promptnum].uval.ch.val != major_method ||
             paramvalues[promptnum+1].uval.ch.val != minor_method)
            ret = 1;
         major_method = (enum Major)paramvalues[promptnum++].uval.ch.val;
         minor_method = (enum Minor)paramvalues[promptnum++].uval.ch.val;
      }
     if((curtype==FORMULA || curtype==FFORMULA) && uses_ismand) 
     {
        if (ismand != (short int)paramvalues[promptnum].uval.ch.val)
        {
           ismand = (short int)paramvalues[promptnum].uval.ch.val;
           ret = 1;
        }
        ++promptnum;
     }
gfp_exit:
   curfractalspecific = &fractalspecific[fractype];
   return(ret);
}

int find_extra_param(int type)
{
   int i,ret,curtyp;
   ret = -1;
   i= -1;

   if(fractalspecific[type].flags&MORE)
   {
      while((curtyp=moreparams[++i].type) != type && curtyp != -1);
      if(curtyp == type)
        ret = i;
   }
   return(ret);
}

void load_params(int fractype)
{
   int i, extra;
   for (i = 0; i < 4; ++i)
   {
      param[i] = fractalspecific[fractype].paramvalue[i];
      if(fractype != CELLULAR && fractype != ANT)
        roundfloatd(&param[i]); /* don't round cellular or ant */
   }
   if((extra=find_extra_param(fractype)) > -1)
      for(i=0;i<MAXPARAMS-4;i++)
         param[i+4] = moreparams[extra].paramvalue[i];
}

int check_orbit_name(char *orbitname)
{
   int i, numtypes, bad;
   char *nameptr[MAXFRACTALS];
   int fractals[MAXFRACTALS];
   int last_val;

   numtypes = build_fractal_list(fractals, &last_val, nameptr);
   bad = 1;
   for(i=0;i<numtypes;i++)
   {
      if(strcmp(orbitname,nameptr[i]) == 0)
      {
         neworbittype = fractals[i];
         bad = 0;
         break;
      }
   }
   return(bad);
}

/* --------------------------------------------------------------------- */

static FILE *gfe_file;

long get_file_entry(int type,char *title,char *fmask,
                          char *filename,char *entryname)
{
   /* Formula, LSystem, etc type structure, select from file */
   /* containing definitions in the form    name { ... }     */
   int newfile,firsttry;
   long entry_pointer;
   newfile = 0;
   for(;;) {
      firsttry = 0;
      /* pb: binary mode used here - it is more work, but much faster, */
      /*     especially when ftell or fgetpos is used                  */
      while (newfile || (gfe_file = fopen(filename, "rb")) == NULL) {
         char buf[60];
         newfile = 0;
         if (firsttry) {
            sprintf(temp1,s_cantfind, filename);
            stopmsg(0,temp1);
            }
         sprintf(buf,"Select %s File",title);
         if (getafilename(buf,fmask,filename) < 0)
            return -1;

         firsttry = 1; /* if around open loop again it is an error */
         }
      setvbuf(gfe_file,tstack,_IOFBF,4096); /* improves speed when file is big */
      newfile = 0;
      if ((entry_pointer = gfe_choose_entry(type,title,filename,entryname)) == -2) {
         newfile = 1; /* go to file list, */
         continue;    /* back to getafilename */
         }
      if (entry_pointer == -1)
         return -1;
      switch (type) {
         case GETFORMULA:
            if (RunForm(entryname, 1) == 0) return 0;
            break;
         case GETLSYS:
            if (LLoad() == 0) return 0;
            break;
         case GETIFS:
            if (ifsload() == 0) {
               fractype = (ifs_type == 0) ? IFS : IFS3D;
               curfractalspecific = &fractalspecific[fractype];
               set_default_parms(); /* to correct them if 3d */
               return 0;
               }
            break;
         case GETPARM:
            return entry_pointer;
         }
      }
}

struct entryinfo {
   char name[ITEMNAMELEN+2];
   long point; /* points to the ( or the { following the name */
   };
static struct entryinfo **gfe_choices; /* for format_getparm_line */
static char *gfe_title;

/* skip to next non-white space character and return it */
int skip_white_space(FILE *infile, long *file_offset)
{
   int c;
   do
   {
      c = getc(infile);
      (*file_offset)++;
   }
   while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
   return(c);
}

/* skip to end of line */
int skip_comment(FILE *infile, long *file_offset)
{
   int c;
   do
   {
      c = getc(infile);
      (*file_offset)++;
   }
   while (c != '\n' && c != '\r' && c != EOF && c != '\032');
   return(c);
}

#define MAXENTRIES 2000L

int scan_entries(FILE * infile, void * ch, char *itemname)
{
      /*
      function returns the number of entries found; if a
      specific entry is being looked for, returns -1 if
      the entry is found, 0 otherwise.
      */
   struct entryinfo ** choices;
   char buf[101];
   int exclude_entry;
   long name_offset, temp_offset;   /*rev 5/23/96 to add temp_offset,
                                      used below to skip any '{' that
                                      does not have a corresponding
                                      '}' - GGM */
   long file_offset = -1;
   int numentries = 0;

   choices = (struct entryinfo * *) ch;

   for (;;)
   {                            /* scan the file for entry names */
      int c, len;
top:
      if ((c = skip_white_space(infile, &file_offset)) == ';')
      {
         c = skip_comment(infile, &file_offset);
         if (c == EOF || c == '\032')
            break;
         continue;
      }
      name_offset = temp_offset = file_offset;
      /* next equiv roughly to fscanf(..,"%40[^* \n\r\t({\032]",buf) */
      len = 0;
      /* allow spaces in entry names in next JCO 9/2/2003 */
      while (c != ' ' && c != '\t' && c != '(' && c != ';'
           && c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032')
      {
         if (len < 40)
            buf[len++] = (char) c;
         c = getc(infile);
         ++file_offset;
         if(c == '\n' || c == '\r')
            goto top;
      }
      buf[len] = 0;
      while (c != '{' &&  c != EOF && c != '\032')
      {
         if(c == ';')
            c = skip_comment(infile, &file_offset);
         else
         {
            c = getc(infile);
            ++file_offset;
            if(c == '\n' || c == '\r')
               goto top;
         }
      }
      if (c == '{')
      {
         while (c != '}' && c != EOF && c != '\032')
         {
            if(c == ';')
               c = skip_comment(infile, &file_offset);
            else
            {
               if(c == '\n' || c == '\r')     /* reset temp_offset to  */
                  temp_offset = file_offset;  /* beginning of new line */
               c = getc(infile);
               ++file_offset;
            }
            if(c == '{') /*second '{' found*/
            {
               if (temp_offset == name_offset) /*if on same line, skip line*/
               {
                  c = skip_comment(infile, &file_offset);
                  goto top;
               }
               else
               {
                  fseek(infile, temp_offset, SEEK_SET); /*else, go back to */
                  file_offset = temp_offset - 1;        /*beginning of line*/
                  goto top;
               }
            }
         }
         if (c != '}')   /* i.e. is EOF or '\032'*/
            break;

         if (strnicmp(buf, "frm:", 4) == 0 ||
                   strnicmp(buf, "ifs:", 4) == 0 ||
                   strnicmp(buf, "par:", 4) == 0)
            exclude_entry = 4;
         else if (strnicmp(buf, "lsys:", 5) == 0)
            exclude_entry = 5;
         else
            exclude_entry = 0;

         buf[ITEMNAMELEN + exclude_entry] = 0;
         if (itemname != NULL)  /* looking for one entry */
         {
            if (stricmp(buf, itemname) == 0)
            {
               fseek(infile, name_offset + (long) exclude_entry, SEEK_SET);
               return (-1);
            }
         }
         else /* make a whole list of entries */
         {
            if (buf[0] != 0 && stricmp(buf, "comment") != 0 && !exclude_entry)
            {
               strcpy(choices[numentries]->name, buf);
               choices[numentries]->point = name_offset;
               if (++numentries >= MAXENTRIES)
               {
                  sprintf(buf, "Too many entries in file, first %ld used", MAXENTRIES);
                  stopmsg(0, buf);
                  break;
               }
            }
         }
      }
      else if (c == EOF || c == '\032')
         break;
   }
   return (numentries);
}

static long gfe_choose_entry(int type,char *title,char *filename,char *entryname)
/* subrtn of get_file_entry, separated so that storage gets freed up */
{
#ifdef XFRACT
   static FCODE o_instr[]={"Press "FK_F6" to select file, "FK_F2" for details, "FK_F4" to toggle sort "};
/* keep the above line length < 80 characters */
#else
   static FCODE o_instr[]={"Press "FK_F6" to select different file, "FK_F2" for details, "FK_F4" to toggle sort "};
#endif
   int numentries, i;
   char buf[101];
   struct entryinfo * *choices;
   int *attributes;
   void (*formatitem)(int, char *);
   int boxwidth,boxdepth,colwidth;
   static int dosort = 1;
   int options = 8;
   char *instr;
   /* steal existing array for "choices" */
   /* TODO: allocate real memory, not reuse shared segment */
   choices = (struct entryinfo **)MK_FP(extraseg,0);
   /* leave room for details F2 */
   choices = choices + (2048/sizeof(struct entryinfo **));
   choices[0] = (struct entryinfo *)(choices + MAXENTRIES+1);
   attributes = (int *)(choices[0] + MAXENTRIES+1);
   instr = (char *)(attributes + MAXENTRIES +1);
   gfe_choices = &choices[0];
   gfe_title = title;
retry:
   attributes[0] = 1;
   for(i=1;i<MAXENTRIES+1;i++)
   {
      choices[i] = choices[i-1] + 1;
      attributes[i] = 1;
   }

   numentries = 0;
   helptitle(); /* to display a clue when file big and next is slow */

   numentries=scan_entries(gfe_file,choices,NULL);
   if (numentries == 0) {
      static FCODE msg[]={"File doesn't contain any valid entries"};
      stopmsg(0,msg);
      fclose(gfe_file);
      return -2; /* back to file list */
      }
   strcpy(instr,o_instr);
   if(dosort)
   {
      strcat(instr,"off");
      shell_sort((char *)choices,numentries,sizeof(char *),lccompare);
   }
   else
      strcat(instr,"on");

   strcpy(buf,entryname); /* preset to last choice made */
   sprintf(temp1,"%s Selection\nFile: %s",title,filename);
   formatitem = NULL;
   boxwidth = colwidth = boxdepth = 0;
   if (type == GETPARM) {
      formatitem = format_parmfile_line;
      boxwidth = 1;
      boxdepth = 16;
      colwidth = 76;
      }
   if(dosort)
      options = 8;
   else
      options = 8+32;
   i = fullscreen_choice(options,temp1,NULL,instr,numentries,(char **)choices,
                           attributes,boxwidth,boxdepth,colwidth,0,
                           formatitem,buf,NULL,check_gfe_key);
   if (i == 0-F4)
   {
     rewind(gfe_file);
     dosort = 1-dosort;
     goto retry;
   }
   fclose(gfe_file);
   if (i < 0) {
      if (i == 0-F6)
         return -2; /* go back to file list */
      return -1;    /* cancel */
      }
   strcpy(entryname, choices[i]->name);
   return(choices[i]->point);
}


static int check_gfe_key(int curkey,int choice)
{
   char infhdg[60];
   char *infbuf;
   int in_scrolling_mode = 0; /* 1 if entry doesn't fit available space */
   int top_line = 0;
   int left_column = 0;
   int i;
   int done = 0;
   int rewrite_infbuf = 0;  /* if 1: rewrite the entry portion of screen */
   char blanks[79];         /* used to clear the entry portion of screen */
   memset(blanks, ' ', 78);
   blanks[78] = (char) 0;

   if (curkey == F6)
      return 0-F6;
   if (curkey == F4)
      return 0-F4;
   if (curkey == F2) {
      int widest_entry_line = 0;
      int lines_in_entry = 0;
      int comment = 0;
      int c = 0;
      int widthct = 0;
   /* TODO: allocate real memory, not reuse shared segment */
      infbuf = MK_FP(extraseg,0);
      fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
      while((c = fgetc(gfe_file)) != EOF && c != '\032') {
         if(c == ';')
            comment = 1;
         else if(c == '\n') {
            comment = 0;
            lines_in_entry++;
            widthct =  -1;
         }
         else if (c == '\t')
            widthct += 7 - widthct % 8;
         else if ( c == '\r')
            continue;
         if(++widthct > widest_entry_line)
            widest_entry_line = widthct;
         if (c == '}' && !comment) {
            lines_in_entry++;
            break;
         }
      }
      if(c == EOF || c == '\032') { /* should never happen */
         fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
         in_scrolling_mode = 0;
      }
      fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
      load_entry_text(gfe_file,infbuf, 17, 0, 0);
      if(lines_in_entry > 17 || widest_entry_line > 74)
         in_scrolling_mode = 1;
      strcpy(infhdg,gfe_title);
      strcat(infhdg," file entry:\n\n");
 /* ... instead, call help with buffer?  heading added */
      driver_stack_screen();
      helptitle();
      driver_set_attr(1,0,C_GENERAL_MED,24*80);

      textcbase = 0;
      driver_put_string(2,1,C_GENERAL_HI,infhdg);
      textcbase = 2; /* left margin is 2 */
      driver_put_string(4,0,C_GENERAL_MED,infbuf);

      {
      static FCODE msg[]  = {"\n\n Use "UPARR1", "DNARR1", "RTARR1", "LTARR1", PgUp, PgDown, Home, and End to scroll text\nAny other key to return to selection list"};
      driver_put_string(-1,0,C_GENERAL_LO,msg);
      }

      while(!done) {
         if(rewrite_infbuf) {
            rewrite_infbuf = 0;
            fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
            load_entry_text(gfe_file, infbuf, 17, top_line, left_column);
            for(i = 4; i < (lines_in_entry < 17 ? lines_in_entry + 4 : 21); i++)
               driver_put_string(i,0,C_GENERAL_MED,blanks);
            driver_put_string(4,0,C_GENERAL_MED,infbuf);
         }
         if((i = getakeynohelp()) == DOWN_ARROW || i == DOWN_ARROW_2
                             || i == UP_ARROW || i == UP_ARROW_2
                             || i == LEFT_ARROW || i == LEFT_ARROW_2
                             || i == RIGHT_ARROW || i == RIGHT_ARROW_2
                             || i == HOME || i == CTL_HOME
                             || i == END || i == CTL_END
                             || i == PAGE_UP || i == CTL_PAGE_UP
                             || i == PAGE_DOWN || i == CTL_PAGE_DOWN) {
            switch(i) {
               case DOWN_ARROW: case DOWN_ARROW_2: /* down one line */
                  if(in_scrolling_mode && top_line < lines_in_entry - 17) {
                     top_line++;
                     rewrite_infbuf = 1;
                  }
                  break;
               case UP_ARROW: case UP_ARROW_2:  /* up one line */
                  if(in_scrolling_mode && top_line > 0) {
                     top_line--;
                     rewrite_infbuf = 1;
                  }
                  break;
               case LEFT_ARROW: case LEFT_ARROW_2:  /* left one column */
                  if(in_scrolling_mode && left_column > 0) {
                     left_column--;
                     rewrite_infbuf = 1;
                  }
                  break;
               case RIGHT_ARROW: case RIGHT_ARROW_2: /* right one column */
                  if(in_scrolling_mode && strchr(infbuf, '\021') != NULL) {
                     left_column++;
                     rewrite_infbuf = 1;
                  }
                  break;
               case PAGE_DOWN: case CTL_PAGE_DOWN: /* down 17 lines */
                  if(in_scrolling_mode && top_line < lines_in_entry - 17) {
                     top_line += 17;
                     if(top_line > lines_in_entry - 17)
                        top_line = lines_in_entry - 17;
                     rewrite_infbuf = 1;
                  }
                  break;
               case PAGE_UP: case CTL_PAGE_UP: /* up 17 lines */
                  if(in_scrolling_mode && top_line > 0) {
                     top_line -= 17;
                     if(top_line < 0)
                        top_line = 0;
                     rewrite_infbuf = 1;
                  }
                  break;
               case END: case CTL_END:       /* to end of entry */
                  if(in_scrolling_mode) {
                     top_line = lines_in_entry - 17;
                     left_column = 0;
                     rewrite_infbuf = 1;
                  }
                  break;
               case HOME: case CTL_HOME:     /* to beginning of entry */
                  if(in_scrolling_mode) {
                     top_line = left_column = 0;
                     rewrite_infbuf = 1;
                  }
                  break;
               default:
                  break;
            }
         }
         else
            done = 1;  /* a key other than scrolling key was pressed */
      }
      textcbase = 0;
      driver_hide_text_cursor();
      driver_unstack_screen();
   }
   return 0;
}

static void load_entry_text(
      FILE *entfile,
      char *buf,
      int maxlines,
      int startrow,
      int startcol)
{
       /* Revised 12/14/96 by George Martin. Up to maxlines of an entry
          is copied to *buf starting from row "startrow", and skipping
          characters in each line up to "startcol". The terminating '\n'
          is deleted if maxlines is reached before the end of the entry.
       */

   int linelen, i;
   int comment=0;
   int c = 0;
   int tabpos = 7 - (startcol % 8);

   if(maxlines <= 0) { /* no lines to get! */
      *buf = (char) 0;
      return;
   }

      /*move down to starting row*/
   for(i = 0; i < startrow; i++) {
      while((c=fgetc(entfile)) != '\n' && c != EOF && c != '\032') {
         if(c == ';')
            comment = 1;
         if(c == '}' && !comment)  /* end of entry before start line */
            break;                 /* this should never happen       */
      }
      if(c == '\n')
         comment = 0;
      else {                       /* reached end of file or end of entry */
         *buf = (char) 0;
         return;
      }
   }

      /* write maxlines of entry */
   while(maxlines-- > 0) {
      comment = linelen = i = c = 0;

         /* skip line up to startcol */
      while (i++ < startcol && (c = fgetc(entfile)) != EOF && c != '\032') {
         if(c == ';')
            comment = 1;
         if(c == '}' && !comment) { /*reached end of entry*/
            *buf = (char) 0;
            return;
         }
         if ( c == '\r') {
            i--;
            continue;
         }
         if(c == '\t')
            i += 7 - (i % 8);
         if(c == '\n') {  /*need to insert '\n', even for short lines*/
            *(buf++) = (char)c;
            break;
         }
      }
      if(c == EOF || c == '\032') { /* unexpected end of file */
         *buf = (char) 0;
         return;
      }
      if(c == '\n')       /* line is already completed */
         continue;

      if(i > startcol) {  /* can happen because of <tab> character */
         while(i-- > startcol) {
            *(buf++) = ' ';
            linelen++;
         }
      }

         /*process rest of line into buf */
      while ((c = fgetc(entfile)) != EOF && c != '\032') {
         if (c == ';')
            comment = 1;
         else if (c == '\n' || c == '\r')
            comment = 0;
         if (c != '\r') {
            if (c == '\t') {
               while ((linelen % 8) != tabpos && linelen < 75) { /* 76 wide max */
                  *(buf++) = ' ';
                  ++linelen;
               }
               c = ' ';
            }
            if (c == '\n') {
               *(buf++) = '\n';
               break;
            }
            if (++linelen > 75) {
               if (linelen == 76)
                  *(buf++) = '\021';
            }
            else
               *(buf++) = (char)c;
            if (c == '}' && !comment) { /*reached end of entry*/
               *(buf) = (char) 0;
               return;
            }
         }
      }
      if(c == EOF || c == '\032') { /* unexpected end of file */
         *buf = (char) 0;
         return;
      }
   }
   if(*(buf-1) == '\n') /* specified that buf will not end with a '\n' */
      buf--;
   *buf = (char) 0;
}

static void format_parmfile_line(int choice,char *buf)
{
   int c,i;
   char line[80];
   fseek(gfe_file,gfe_choices[choice]->point,SEEK_SET);
   while (getc(gfe_file) != '{') { }
   while ((c = getc(gfe_file)) == ' ' || c == '\t' || c == ';') { }
   i = 0;
   while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032') {
      line[i++] = (char)((c == '\t') ? ' ' : c);
      c = getc(gfe_file);
      }
   line[i] = 0;
#ifndef XFRACT
   sprintf(buf,"%-20Fs%-56s",gfe_choices[choice]->name,line);
#else
   sprintf(buf,"%-20s%-56s",gfe_choices[choice]->name,line);
#endif
}

/* --------------------------------------------------------------------- */

int get_fract3d_params() /* prompt for 3D fractal parameters */
{
   int i,k,ret,oldhelpmode;
   static FCODE hdg[] = {"3D Parameters"};
   static FCODE p1[] = {"X-axis rotation in degrees"};
   static FCODE p2[] = {"Y-axis rotation in degrees"};
   static FCODE p3[] = {"Z-axis rotation in degrees"};
   static FCODE p4[] = {"Perspective distance [1 - 999, 0 for no persp]"};
   static FCODE p5[] = {"X shift with perspective (positive = right)"};
   static FCODE p6[] = {"Y shift with perspective (positive = up   )"};
   static FCODE p7[] = {"Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)"};
   struct fullscreenvalues uvalues[20];
   char *ifs3d_prompts[8];

   driver_stack_screen();
   ifs3d_prompts[0] = p1;
   ifs3d_prompts[1] = p2;
   ifs3d_prompts[2] = p3;
   ifs3d_prompts[3] = p4;
   ifs3d_prompts[4] = p5;
   ifs3d_prompts[5] = p6;
   ifs3d_prompts[6] = p7;
   k = 0;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = XROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = YROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ZROT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = ZVIEWER;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = XSHIFT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = YSHIFT;
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = glassestype;

   oldhelpmode = helpmode;
   helpmode = HELP3DFRACT;
   i = fullscreen_prompt(hdg,k,ifs3d_prompts,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (i < 0) {
      ret = -1;
      goto get_f3d_exit;
      }

   ret = k = 0;
   XROT    =  uvalues[k++].uval.ival;
   YROT    =  uvalues[k++].uval.ival;
   ZROT    =  uvalues[k++].uval.ival;
   ZVIEWER =  uvalues[k++].uval.ival;
   XSHIFT  =  uvalues[k++].uval.ival;
   YSHIFT  =  uvalues[k++].uval.ival;
   glassestype = uvalues[k++].uval.ival;
   if (glassestype < 0 || glassestype > 4) glassestype = 0;
   if (glassestype)
      if (get_funny_glasses_params() || check_mapfile())
         ret = -1;

get_f3d_exit:
   driver_unstack_screen();
   return(ret);
}

/* --------------------------------------------------------------------- */
/* These macros streamline the "save near space" campaign */

#define LOADPROMPTS3D(X)     {\
   static FCODE tmp[] = { X };\
   prompts3d[++k]= tmp;\
   }

#define LOADPROMPTSCHOICES(X)     {\
   static FCODE tmp[] = { X };\
   choices[k++]= tmp;\
   }

int get_3d_params()     /* prompt for 3D parameters */
{
   static FCODE hdg[]={"3D Mode Selection"};
   static FCODE hdg1[]={"Select 3D Fill Type"};
   char *choices[11];
   int attributes[21];
   int sphere;
   char *s;
   static FCODE s1[] = {"Sphere 3D Parameters\n\
Sphere is on its side; North pole to right\n\
Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right"};
   static FCODE s2[]={"Planar 3D Parameters\n\
Pre-rotation X axis is screen top; Y axis is left side\n\
Pre-rotation Z axis is coming at you out of the screen!"};
   char *prompts3d[21];
   struct fullscreenvalues uvalues[21];
   int i, k;
   int oldhelpmode;

#ifdef WINFRACT
     {
     extern int wintext_textmode;
     if (wintext_textmode != 2)  /* are we in textmode? */
         return(0);              /* no - prompts are already handled */
     }
#endif
restart_1:
        if (Targa_Out && overlay3d)
                Targa_Overlay = 1;

   k= -1;

   LOADPROMPTS3D("Preview Mode?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = preview;

   LOADPROMPTS3D("    Show Box?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = showbox;

   LOADPROMPTS3D("Coarseness, preview/grid/ray (in y dir)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = previewfactor;

   LOADPROMPTS3D("Spherical Projection?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = sphere = SPHERE;

   LOADPROMPTS3D("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = glassestype;

   LOADPROMPTS3D("                  3=photo,4=stereo pair)");
   uvalues[k].type = '*';

   LOADPROMPTS3D("Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = RAY;

   LOADPROMPTS3D("                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)");
   uvalues[k].type = '*';

   LOADPROMPTS3D("    Brief output?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = BRIEF;

   check_writefile(ray_name,".ray");
   LOADPROMPTS3D("    Output File Name");
   uvalues[k].type = 's';
   strcpy(uvalues[k].uval.sval,ray_name);

   LOADPROMPTS3D("Targa output?");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = Targa_Out;

   LOADPROMPTS3D("Use grayscale value for depth? (if \"no\" uses color number)");
   uvalues[k].type = 'y';
   uvalues[k].uval.ch.val = grayflag;

   oldhelpmode = helpmode;
   helpmode = HELP3DMODE;

   k = fullscreen_prompt(hdg,k+1,prompts3d,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0) {
      return(-1);
      }

   k=0;

   preview = (char)uvalues[k++].uval.ch.val;

   showbox = (char)uvalues[k++].uval.ch.val;

   previewfactor  = uvalues[k++].uval.ival;

   sphere = uvalues[k++].uval.ch.val;

   glassestype = uvalues[k++].uval.ival;
   k++;

   RAY = uvalues[k++].uval.ival;
   k++;
   {
      static FCODE msg[] = {
"DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n\
the online documentation."};
      if(RAY == 1)
         stopmsg(0,msg);
   }
   BRIEF = uvalues[k++].uval.ch.val;

   strcpy(ray_name,uvalues[k++].uval.sval);

   Targa_Out = uvalues[k++].uval.ch.val;
   grayflag  = (char)uvalues[k++].uval.ch.val;

   /* check ranges */
   if(previewfactor < 2)
      previewfactor = 2;
   if(previewfactor > 2000)
      previewfactor = 2000;

   if(sphere && !SPHERE)
   {
      SPHERE = TRUE;
      set_3d_defaults();
   }
   else if(!sphere && SPHERE)
   {
      SPHERE = FALSE;
      set_3d_defaults();
   }

   if(glassestype < 0)
      glassestype = 0;
   if(glassestype > 4)
      glassestype = 4;
   if(glassestype)
      whichimage = 1;

   if (RAY < 0)
      RAY = 0;
   if (RAY > 7)
      RAY = 7;

   if (!RAY)
   {
      k = 0;
      LOADPROMPTSCHOICES("make a surface grid");
      LOADPROMPTSCHOICES("just draw the points");
      LOADPROMPTSCHOICES("connect the dots (wire frame)");
      LOADPROMPTSCHOICES("surface fill (colors interpolated)");
      LOADPROMPTSCHOICES("surface fill (colors not interpolated)");
      LOADPROMPTSCHOICES("solid fill (bars up from \"ground\")");
      if(SPHERE)
      {
             LOADPROMPTSCHOICES("light source");
      }
      else
      {
             LOADPROMPTSCHOICES("light source before transformation");
             LOADPROMPTSCHOICES("light source after transformation");
      }
      for (i = 0; i < k; ++i)
         attributes[i] = 1;
      helpmode = HELP3DFILL;
      i = fullscreen_choice(CHOICEHELP,hdg1,NULL,NULL,k,(char * *)choices,attributes,
                              0,0,0,FILLTYPE+1,NULL,NULL,NULL,NULL);
      helpmode = oldhelpmode;
      if (i < 0)
         goto restart_1;
      FILLTYPE = i-1;

      if(glassestype)
      {
         if(get_funny_glasses_params())
            goto restart_1;
         }
         if (check_mapfile())
             goto restart_1;
      }
   restart_3:

   if(SPHERE)
   {
      k = -1;
      LOADPROMPTS3D("Longitude start (degrees)");
      LOADPROMPTS3D("Longitude stop  (degrees)");
      LOADPROMPTS3D("Latitude start  (degrees)");
      LOADPROMPTS3D("Latitude stop   (degrees)");
      LOADPROMPTS3D("Radius scaling factor in pct");
   }
   else
   {
      k = -1;
      if (!RAY)
      {
             LOADPROMPTS3D("X-axis rotation in degrees");
         LOADPROMPTS3D("Y-axis rotation in degrees");
             LOADPROMPTS3D("Z-axis rotation in degrees");
      }
      LOADPROMPTS3D("X-axis scaling factor in pct");
      LOADPROMPTS3D("Y-axis scaling factor in pct");
   }
   k = -1;
   if (!(RAY && !SPHERE))
   {
      uvalues[++k].uval.ival   = XROT    ;
      uvalues[k].type = 'i';
      uvalues[++k].uval.ival   = YROT    ;
      uvalues[k].type = 'i';
      uvalues[++k].uval.ival   = ZROT    ;
      uvalues[k].type = 'i';
   }
   uvalues[++k].uval.ival   = XSCALE    ;
   uvalues[k].type = 'i';

   uvalues[++k].uval.ival   = YSCALE    ;
   uvalues[k].type = 'i';

   LOADPROMPTS3D("Surface Roughness scaling factor in pct");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = ROUGH     ;

   LOADPROMPTS3D("'Water Level' (minimum color value)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = WATERLINE ;

   if(!RAY)
   {
      LOADPROMPTS3D("Perspective distance [1 - 999, 0 for no persp])");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = ZVIEWER     ;

      LOADPROMPTS3D("X shift with perspective (positive = right)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = XSHIFT    ;

      LOADPROMPTS3D("Y shift with perspective (positive = up   )");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = YSHIFT    ;

      LOADPROMPTS3D("Image non-perspective X adjust (positive = right)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = xtrans    ;

      LOADPROMPTS3D("Image non-perspective Y adjust (positive = up)");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = ytrans    ;

      LOADPROMPTS3D("First transparent color");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = transparent[0];

      LOADPROMPTS3D("Last transparent color");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = transparent[1];
   }

   LOADPROMPTS3D("Randomize Colors      (0 - 7, '0' disables)");
   uvalues[k].type = 'i';
   uvalues[k++].uval.ival = RANDOMIZE;

   if (SPHERE)
      s = s1;
   else
      s = s2;

   helpmode = HELP3DPARMS;
   k = fullscreen_prompt(s,k,prompts3d,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      goto restart_1;

   k = 0;
   if (!(RAY && !SPHERE))
   {
      XROT    = uvalues[k++].uval.ival;
      YROT    = uvalues[k++].uval.ival;
      ZROT    = uvalues[k++].uval.ival;
   }
   XSCALE     = uvalues[k++].uval.ival;
   YSCALE     = uvalues[k++].uval.ival;
   ROUGH      = uvalues[k++].uval.ival;
   WATERLINE  = uvalues[k++].uval.ival;
   if (!RAY)
   {
      ZVIEWER = uvalues[k++].uval.ival;
   XSHIFT     = uvalues[k++].uval.ival;
   YSHIFT     = uvalues[k++].uval.ival;
   xtrans     = uvalues[k++].uval.ival;
   ytrans     = uvalues[k++].uval.ival;
   transparent[0] = uvalues[k++].uval.ival;
   transparent[1] = uvalues[k++].uval.ival;
   }
   RANDOMIZE  = uvalues[k++].uval.ival;
   if (RANDOMIZE >= 7) RANDOMIZE = 7;
   if (RANDOMIZE <= 0) RANDOMIZE = 0;

   if ((Targa_Out || ILLUMINE || RAY))
        if(get_light_params())
            goto restart_3;
return(0);
}

/* --------------------------------------------------------------------- */
static int get_light_params()
{
   static FCODE hdg[]={"Light Source Parameters"};
   char *prompts3d[13];
   struct fullscreenvalues uvalues[13];

   int k;
   int oldhelpmode;

   /* defaults go here */

   k = -1;

   if (ILLUMINE || RAY)
   {
   LOADPROMPTS3D("X value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = XLIGHT    ;

   LOADPROMPTS3D("Y value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = YLIGHT    ;

   LOADPROMPTS3D("Z value light vector");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = ZLIGHT    ;

                if (!RAY)
                {
   LOADPROMPTS3D("Light Source Smoothing Factor");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = LIGHTAVG  ;

   LOADPROMPTS3D("Ambient");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = Ambient;
                }
   }

   if (Targa_Out && !RAY)
   {
        LOADPROMPTS3D("Haze Factor        (0 - 100, '0' disables)");
        uvalues[k].type = 'i';
        uvalues[k].uval.ival= haze;

                if (!Targa_Overlay)
        check_writefile(light_name,".tga");
      LOADPROMPTS3D("Targa File Name  (Assume .tga)");
        uvalues[k].type = 's';
        strcpy(uvalues[k].uval.sval,light_name);

      LOADPROMPTS3D("Back Ground Color (0 - 255)");
      uvalues[k].type = '*';

      LOADPROMPTS3D("   Red");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[0];

      LOADPROMPTS3D("   Green");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[1];

      LOADPROMPTS3D("   Blue");
      uvalues[k].type = 'i';
      uvalues[k].uval.ival = (int)back_color[2];

      LOADPROMPTS3D("Overlay Targa File? (Y/N)");
      uvalues[k].type = 'y';
      uvalues[k].uval.ch.val = Targa_Overlay;

   }

   LOADPROMPTS3D("");

   oldhelpmode = helpmode;
   helpmode = HELP3DLIGHT;
   k = fullscreen_prompt(hdg,k,prompts3d,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      return(-1);

   k = 0;
   if (ILLUMINE)
   {
      XLIGHT   = uvalues[k++].uval.ival;
      YLIGHT   = uvalues[k++].uval.ival;
      ZLIGHT   = uvalues[k++].uval.ival;
      if (!RAY)
                {
      LIGHTAVG = uvalues[k++].uval.ival;
      Ambient  = uvalues[k++].uval.ival;
      if (Ambient >= 100) Ambient = 100;
      if (Ambient <= 0) Ambient = 0;
                }
   }

   if (Targa_Out && !RAY)
   {
        haze  =  uvalues[k++].uval.ival;
        if (haze >= 100) haze = 100;
        if (haze <= 0) haze = 0;
        strcpy(light_name,uvalues[k++].uval.sval);
                /* In case light_name conflicts with an existing name it is checked
                        again in line3d */
                k++;
        back_color[0] = (char)(uvalues[k++].uval.ival % 255);
        back_color[1] = (char)(uvalues[k++].uval.ival % 255);
        back_color[2] = (char)(uvalues[k++].uval.ival % 255);
        Targa_Overlay = uvalues[k].uval.ch.val;
   }
   return(0);
}

/* --------------------------------------------------------------------- */


static int check_mapfile()
{
   int askflag = 0;
   int i,oldhelpmode;
   if(dontreadcolor)
      return(0);
   strcpy(temp1,"*");
   if (mapset)
      strcpy(temp1,MAP_name);
   if (!(glassestype == 1 || glassestype == 2))
      askflag = 1;
   else
      merge_pathnames(temp1,funnyglasses_map_name,0);

   for(;;) {
      if (askflag) {
         static FCODE msg[] = {"\
Enter name of .MAP file to use,\n\
or '*' to use palette from the image to be loaded."};
         oldhelpmode = helpmode;
         helpmode = -1;
         i = field_prompt(0,msg,NULL,temp1,60,NULL);
         helpmode = oldhelpmode;
         if (i < 0)
            return(-1);
         if (temp1[0] == '*') {
            mapset = 0;
            break;
         }
      }
      memcpy(olddacbox,dacbox,256*3); /* save the DAC */
      i = ValidateLuts(temp1);
      memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
      if (i != 0) { /* Oops, somethings wrong */
         askflag = 1;
         continue;
         }
      mapset = 1;
      merge_pathnames(MAP_name,temp1,0);
      break;
      }
   return(0);
}

static int get_funny_glasses_params()
{
   static FCODE hdg[]={"Funny Glasses Parameters"};
   char *prompts3d[10];

   struct fullscreenvalues uvalues[10];

   int k;
   int oldhelpmode;

   /* defaults */
   if(ZVIEWER == 0)
      ZVIEWER = 150;
   if(eyeseparation == 0)
   {
      if(fractype==IFS3D || fractype==LLORENZ3D || fractype==FPLORENZ3D)
      {
         eyeseparation =  2;
         xadjust       = -2;
      }
      else
      {
         eyeseparation =  3;
         xadjust       =  0;
      }
   }

   if(glassestype == 1)
      strcpy(funnyglasses_map_name,Glasses1Map);
   else if(glassestype == 2)
   {
      if(FILLTYPE == -1)
         strcpy(funnyglasses_map_name,"grid.map");
      else
      {
         strcpy(funnyglasses_map_name,Glasses1Map);
         funnyglasses_map_name[7] = '2';
      }
   }

   k = -1;
   LOADPROMPTS3D("Interocular distance (as % of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival= eyeseparation;

   LOADPROMPTS3D("Convergence adjust (positive = spread greater)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = xadjust;

   LOADPROMPTS3D("Left  red image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_left;

   LOADPROMPTS3D("Right red image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_crop_right;

   LOADPROMPTS3D("Left  blue image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_left;

   LOADPROMPTS3D("Right blue image crop (% of screen)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_crop_right;

   LOADPROMPTS3D("Red brightness factor (%)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = red_bright;

   LOADPROMPTS3D("Blue brightness factor (%)");
   uvalues[k].type = 'i';
   uvalues[k].uval.ival = blue_bright;

   if(glassestype == 1 || glassestype == 2)
   {
      LOADPROMPTS3D("Map File name");
      uvalues[k].type = 's';
      strcpy(uvalues[k].uval.sval,funnyglasses_map_name);
   }

   oldhelpmode = helpmode;
   helpmode = HELP3DGLASSES;
   k = fullscreen_prompt(hdg,k+1,prompts3d,uvalues,0,NULL);
   helpmode = oldhelpmode;
   if (k < 0)
      return(-1);

   k = 0;
   eyeseparation   =  uvalues[k++].uval.ival;
   xadjust         =  uvalues[k++].uval.ival;
   red_crop_left   =  uvalues[k++].uval.ival;
   red_crop_right  =  uvalues[k++].uval.ival;
   blue_crop_left  =  uvalues[k++].uval.ival;
   blue_crop_right =  uvalues[k++].uval.ival;
   red_bright      =  uvalues[k++].uval.ival;
   blue_bright     =  uvalues[k++].uval.ival;

   if(glassestype == 1 || glassestype == 2)
      strcpy(funnyglasses_map_name,uvalues[k].uval.sval);
   return(0);
}

void setbailoutformula(enum bailouts test) {

   switch(test) {
     case Mod:
     default:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpMODbailout;
         else
           floatbailout = (int ( *)(void))fpMODbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lMODbailout;
         else
           longbailout = (int ( *)(void))asmlMODbailout;
         bignumbailout = (int ( *)(void))bnMODbailout;
         bigfltbailout = (int ( *)(void))bfMODbailout;
         break;}
     case Real: {
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpREALbailout;
         else
           floatbailout = (int ( *)(void))fpREALbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lREALbailout;
         else
           longbailout = (int ( *)(void))asmlREALbailout;
         bignumbailout = (int ( *)(void))bnREALbailout;
         bigfltbailout = (int ( *)(void))bfREALbailout;
         break;}
     case Imag:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpIMAGbailout;
         else
           floatbailout = (int ( *)(void))fpIMAGbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lIMAGbailout;
         else
           longbailout = (int ( *)(void))asmlIMAGbailout;
         bignumbailout = (int ( *)(void))bnIMAGbailout;
         bigfltbailout = (int ( *)(void))bfIMAGbailout;
         break;}
     case Or:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpORbailout;
         else
           floatbailout = (int ( *)(void))fpORbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lORbailout;
         else
           longbailout = (int ( *)(void))asmlORbailout;
         bignumbailout = (int ( *)(void))bnORbailout;
         bigfltbailout = (int ( *)(void))bfORbailout;
         break;}
     case And:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpANDbailout;
         else
           floatbailout = (int ( *)(void))fpANDbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lANDbailout;
         else
           longbailout = (int ( *)(void))asmlANDbailout;
         bignumbailout = (int ( *)(void))bnANDbailout;
         bigfltbailout = (int ( *)(void))bfANDbailout;
         break;}
     case Manh:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpMANHbailout;
         else
           floatbailout = (int ( *)(void))fpMANHbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lMANHbailout;
         else
           longbailout = (int ( *)(void))asmlMANHbailout;
         bignumbailout = (int ( *)(void))bnMANHbailout;
         bigfltbailout = (int ( *)(void))bfMANHbailout;
         break;}
     case Manr:{
         if (fpu >= 287 && debugflag != 72)     /* Fast 287 math */
           floatbailout = (int ( *)(void))asmfpMANRbailout;
         else
           floatbailout = (int ( *)(void))fpMANRbailout;
         if (cpu >=386 && debugflag != 8088)    /* Fast 386 math */
           longbailout = (int ( *)(void))asm386lMANRbailout;
         else
           longbailout = (int ( *)(void))asmlMANRbailout;
         bignumbailout = (int ( *)(void))bnMANRbailout;
         bigfltbailout = (int ( *)(void))bfMANRbailout;
         break;}
   }
}
