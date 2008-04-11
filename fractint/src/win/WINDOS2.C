
/* Windows versions of DOS functions needed by PROMPTS.C */

#define STRICT

#include "port.h"
#include "prototyp.h"

#include <string.h>
#include <dos.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <io.h>

#include "fractype.h"
#include "helpdefs.h"

/* routines in this module        */
void movecursor(int row, int col);
void setattr(int row, int col, int attr, int count);
int  putstringcenter(int row, int col, int width, int attr, char *msg);
void putstring(int row, int col, int attr, unsigned char *buf);
int  fullscreen_choice(
             int options, char *hdg, char *hdg2, char *instr, int numchoices,
             char * *choices, int *attributes, int boxwidth, int boxdepth,
             int colwidth, int current, void (*formatitem)(),
             char *speedstring, int (*speedprompt)(), int (*checkkey)());
int  strncasecmp(char *s,char *t,int ct);
int  input_field(int options, int attr, char *fld, int len, int row, int col, int (*checkkey)(int) );
int  field_prompt(int options, char *hdg, char *instr, char *fld, int len, int (*checkkey)(int) );
void helptitle(void);
void stackscreen(void);
void unstackscreen(void);
void discardscreen(void);
int load_palette(void);
void save_palette(void);
void fractint_help(void);
void setfortext(void);
void setforgraphics(void);
void setclear(void);

extern  VOIDFARPTR cdecl malloc(long);

/* faked/unimplemented routines */

struct videoinfo *vidtbl;
int vidtbllen = 0;
int badconfig = 0;

int in_fractint_help = 0;

int win_release = 2004;
int patchlevel = 4;
char win_comment[] =
     {" "};                /*  publicly-released version */
/*   {"Test Version - Not for Release"};   /* interim test versions */
/*   {"'Public Beta' Release"};   /* interim test versions */

extern int time_to_resume;             /* time to resume? */

int text_type = 0;
int textrow = 0, textcol = 0;
int textrbase = 0, textcbase = 0;
int lookatmouse = LOOK_MOUSE_NONE;

/* fullscreen_choice options */
#define CHOICERETURNKEY   1
#define CHOICEMENU        2
#define CHOICEHELP        4
#define CHOICESCRUNCH    16
#define CHOICESNOTSORTED 32

static char par_comment[4][MAXCMT];

char speed_prompt[]="Speed key string";

int fullscreen_choice(
        int options,             /* &2 use menu coloring scheme               */
                             /* &4 include F1 for help in instructions */
                             /* &8 add caller's instr after normal set */
        char *hdg,             /* heading info, \n delimited               */
        char *hdg2,             /* column heading or NULL                       */
        char *instr,     /* instructions, \n delimited, or NULL    */
        int numchoices,      /* How many choices in list               */
        char * *choices,      /* array of choice strings                */
        int *attributes,     /* &3: 0 normal color, 1,3 highlight      */
                             /* &256 marks a dummy entry               */
        int boxwidth,             /* box width, 0 for calc (in items)       */
        int boxdepth,             /* box depth, 0 for calc, 99 for max      */
        int colwidth,             /* data width of a column, 0 for calc     */
        int current,             /* start with this item                       */
        void (*formatitem)(),/* routine to display an item or NULL     */
        char *speedstring,   /* returned speed key value, or NULL >[30]*/
        int (*speedprompt)(),/* routine to display prompt or NULL      */
        int (*checkkey)()    /* routine to check keystroke or NULL     */
)
   /* return is: n>=0 for choice n selected,
                 -1 for escape
                  k for checkkey routine return value k (if not 0 nor -1)
      speedstring[0] != 0 on return if string is present
      */
{
static char choiceinstr1a[]="Use the cursor keys to highlight your selection";
static char choiceinstr1b[]="Use the cursor keys or type a value to make a selection";
static char choiceinstr2a[]="Press ENTER for highlighted choice, or ESCAPE to back out";
static char choiceinstr2b[]="Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help";
static char choiceinstr2c[]="Press ENTER for highlighted choice, or F1 for help";

   int titlelines,titlewidth;
   int reqdrows;
   int topleftrow,topleftcol;
   int topleftchoice;
   int speedrow;  /* speed key prompt */
   int boxitems;  /* boxwidth*boxdepth */
   int curkey,increment,rev_increment;
   int redisplay;
   int i,j,k;
   char *charptr;
   char buf[81];
   int speed_match = 0;
   char curitem[81];
   char *itemptr;
   int ret,savelookatmouse;

   savelookatmouse = lookatmouse;
   lookatmouse = LOOK_MOUSE_NONE;
   ret = -1;
   if (speedstring
     && (i = strlen(speedstring)) > 0) { /* preset current to passed string */
      current = 0;
      while (current < numchoices
        && (k = strncasecmp(speedstring,choices[current],i)) > 0)
         ++current;
      if (k < 0 && current > 0)  /* oops - overshot */
         --current;
      if (current >= numchoices) /* bumped end of list */
         current = numchoices - 1;
      }

   while (1) {
      if (current >= numchoices)  /* no real choice in the list? */
         goto fs_choice_end;
      if ((attributes[current] & 256) == 0)
         break;
      ++current;                  /* scan for a real choice */
      }

   titlelines = titlewidth = 0;
   if (hdg) {
      charptr = hdg;                  /* count title lines, find widest */
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

   if (colwidth == 0)                  /* find widest column */
      for (i = 0; i < numchoices; ++i)
         if ((int)_fstrlen(choices[i]) > colwidth)
            colwidth = _fstrlen(choices[i]);

   /* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
   reqdrows = 3;                  /* calc rows available */
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
      reqdrows += 2;                  /* standard instructions */
   if (speedstring) ++reqdrows;   /* a row for speedkey prompt */
   if (boxdepth > (i = 25 - reqdrows)) /* limit the depth to max */
      boxdepth = i;
   if (boxwidth == 0) {           /* pick box width and depth */
      if (numchoices <= i - 2) {  /* single column is 1st choice if we can */
         boxdepth = numchoices;
         boxwidth = 1;
         }
      else {                          /* sort-of-wide is 2nd choice */
         boxwidth = 60 / (colwidth + 1);
         if (boxwidth == 0
           || (boxdepth = (numchoices+boxwidth-1)/boxwidth) > i - 2) {
            boxwidth = 80 / (colwidth + 1); /* last gasp, full width */
            if ((boxdepth = (numchoices+boxwidth-1)/boxwidth) > i)
               boxdepth = i;
            }
         }
      }
   if ((i = 77 / boxwidth - colwidth) > 3) /* spaces to add @ left each choice */
      i = 3;
   j = boxwidth * (colwidth += i) + i;           /* overall width of box */
   if (j < titlewidth+2)
      j = titlewidth + 2;
   if (j > 80)
      j = 80;
   if (j <= 70 && boxwidth == 2) {           /* special case makes menus nicer */
      ++j;
      ++colwidth;
      }
   k = (80 - j) / 2;                           /* center the box */
   k -= (90 - j) / 20;
   topleftcol = k + i;                           /* column of topleft choice */
   i = (25 - reqdrows - boxdepth) / 2;
   i -= i / 4;                                   /* higher is better if lots extra */
   topleftrow = 3 + titlelines + i;           /* row of topleft choice */

   /* now set up the overall display */
   helptitle();                            /* clear, display title line */
   setattr(1,0,C_PROMPT_BKGRD,24*80);           /* init rest to background */
   for (i = topleftrow-1-titlelines; i < topleftrow+boxdepth+1; ++i)
      setattr(i,k,C_PROMPT_LO,j);           /* draw empty box */
   if (hdg) {
      textcbase = (80 - titlewidth) / 2;   /* set left margin for putstring */
      textcbase -= (90 - titlewidth) / 20; /* put heading into box */
      putstring(topleftrow-titlelines-1,0,C_PROMPT_HI,hdg);
      textcbase = 0;
      }
   if (hdg2)                                   /* display 2nd heading */
      putstring(topleftrow-1,topleftcol,C_PROMPT_MED,hdg2);
   i = topleftrow + boxdepth + 1;
   if (instr == NULL || (options & 8)) {   /* display default instructions */
      if (i < 20) ++i;
      if (speedstring) {
         speedrow = i;
         *speedstring = 0;
         if (++i < 22) ++i;
         }
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
            (speedstring) ? choiceinstr1b : choiceinstr1a);
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
            (options&CHOICEMENU) ? choiceinstr2c
            : ((options&CHOICEHELP) ? choiceinstr2b : choiceinstr2a));
      }
   if (instr) {                            /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)))
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }

   boxitems = boxwidth * boxdepth;
   topleftchoice = 0;                           /* pick topleft for init display */
   while (current - topleftchoice >= boxitems
     || (current - topleftchoice > boxitems/2
         && topleftchoice + boxitems < numchoices))
      topleftchoice += boxwidth;
   redisplay = 1;

   while (1) { /* main loop */

      if (redisplay) {                             /* display the current choices */
         if ((options & CHOICEMENU) == 0) {
            memset(buf,' ',80);
            buf[boxwidth*colwidth] = 0;
            for (i = (hdg2) ? 0 : -1; i <= boxdepth; ++i)  /* blank the box */
               putstring(topleftrow+i,topleftcol,C_PROMPT_LO,buf);
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
               (*formatitem)(j,charptr=buf);
            else
               charptr = choices[j];
            putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                      k,charptr);
            }
         /***
         ... format differs for summary/detail, whups, force box width to
         ...  be 72 when detail toggle available?  (2 grey margin each
         ...  side, 1 blue margin each side)
         ***/
         if (topleftchoice > 0 && hdg2 == NULL)
            putstring(topleftrow-1,topleftcol,C_PROMPT_LO,"(more)");
         if (topleftchoice + boxitems < numchoices)
            putstring(topleftrow+boxdepth,topleftcol,C_PROMPT_LO,"(more)");
         redisplay = 0;
         }

      i = current - topleftchoice;             /* highlight the current choice */
      if (formatitem)
         (*formatitem)(current,itemptr=curitem);
      else
         itemptr = choices[current];
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                C_CHOICE_CURRENT,itemptr);

      if (speedstring) {                     /* show speedstring if any */
         memset(buf,' ',80);
         buf[80] = 0;
         putstring(speedrow,0,C_PROMPT_BKGRD,buf);
         if (*speedstring) {                     /* got a speedstring on the go */
            putstring(speedrow,15,C_CHOICE_SP_INSTR," ");
            if (speedprompt)
               j = speedprompt(speedrow,16,C_CHOICE_SP_INSTR,speedstring,speed_match);
            else {
               putstring(speedrow,16,C_CHOICE_SP_INSTR,speed_prompt);
               j = strlen(speed_prompt);
               }
            strcpy(buf,speedstring);
            i = strlen(buf);
            while (i < 30)
               buf[i++] = ' ';
            buf[i] = 0;
            putstring(speedrow,16+j,C_CHOICE_SP_INSTR," ");
            putstring(speedrow,17+j,C_CHOICE_SP_KEYIN,buf);
            movecursor(speedrow,17+j+strlen(speedstring));
            }
         else
            movecursor(25,80);
         }
      else
         movecursor(25,80);

/*    while (!keypressed()) { } /* enables help */
/*      curkey = getakey(); */
      curkey = keycursor(-2,-2);

      i = current - topleftchoice;             /* unhighlight current choice */
      if ((k = attributes[current] & 3) == 1)
         k = C_PROMPT_LO;
      else if (k == 3)
         k = C_PROMPT_HI;
      else
         k = C_PROMPT_MED;
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                k,itemptr);

      increment = 0;
      switch (curkey) {                      /* deal with input key */
         case ENTER:
         case ENTER_2:
            ret = current;
            goto fs_choice_end;
         case ESC:
            goto fs_choice_end;
         case DOWN_ARROW:
         case DOWN_ARROW_2:
            rev_increment = 0 - (increment = boxwidth);
            break;
         case UP_ARROW:
         case UP_ARROW_2:
            increment = 0 - (rev_increment = boxwidth);
            break;
         case RIGHT_ARROW:
         case RIGHT_ARROW_2:
            if (boxwidth == 1) break;
            increment = 1; rev_increment = -1;
            break;
         case LEFT_ARROW:
         case LEFT_ARROW_2:
            if (boxwidth == 1) break;
            increment = -1; rev_increment = 1;
            break;
         case PAGE_UP:
            if (numchoices > boxitems) {
               topleftchoice -= boxitems;
               increment = -boxitems;
               rev_increment = boxwidth;
               redisplay = 1;
               }
            break;
         case PAGE_DOWN:
            if (numchoices > boxitems) {
               topleftchoice += boxitems;
               increment = boxitems;
               rev_increment = -boxwidth;
               redisplay = 1;
               }
            break;
         case CTL_HOME:
            current = -1;
            increment = rev_increment = 1;
            break;
         case CTL_END:
            current = numchoices;
            increment = rev_increment = -1;
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
               i = strlen(speedstring);
               if (curkey == 8 && i > 0) /* backspace */
                  speedstring[--i] = 0;
               if (33 <= curkey && curkey <= 126 && i < 30) {
                  curkey = tolower(curkey);
                  speedstring[i] = curkey;
                  speedstring[++i] = 0;
                  }
               if (i > 0) {                 /* locate matching type */
                  current = 0;
                  while (current < numchoices
                    && (speed_match = strncasecmp(speedstring,choices[current],i)) > 0)
                     ++current;
                  if (speed_match < 0 && current > 0)  /* oops - overshot */
                     --current;
                  if (current >= numchoices) /* bumped end of list */
                     current = numchoices - 1;
                  }
               }
            break;
         }

      if (increment) {                        /* apply cursor movement */
         current += increment;
         if (speedstring)                /* zap speedstring */
            speedstring[0] = 0;
         }
      while (1) {                        /* adjust to a non-comment choice */
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

int input_field(
        int options,              /* &1 numeric, &2 integer, &4 double */
        int attr,              /* display attribute */
        char *fld,              /* the field itself */
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
   lookatmouse = LOOK_MOUSE_NONE;
   ret = -1;
   strcpy(savefld,fld);
   insert = started = offset = 0;
   display = 1;
   while (1) {
      strcpy(buf,fld);
      i = strlen(buf);
      while (i < len)
         buf[i++] = ' ';
      buf[len] = 0;
      if (display) {                                    /* display current value */
         putstring(row,col,attr,buf);
         display = 0;
         }
      curkey = keycursor(row+insert,col+offset);  /* get a keystroke */
      switch (curkey) {
         case ENTER:
         case ENTER_2:
            ret = 0;
            goto inpfld_end;
         case ESC:
            goto inpfld_end;
         case RIGHT_ARROW:
         case RIGHT_ARROW_2:
            if (offset < len) ++offset;
            started = 1;
            break;
         case LEFT_ARROW:
         case LEFT_ARROW_2:
            if (offset > 0) --offset;
            started = 1;
            break;
         case HOME:
            offset = 0;
            started = 1;
            break;
         case END:
            offset = strlen(fld);
            started = 1;
            break;
         case 8:
         case 127:                                /* backspace */
            if (offset > 0) {
               j = strlen(fld);
               for (i = offset-1; i < j; ++i)
                  fld[i] = fld[i+1];
               --offset;
               }
            started = display = 1;
            break;
         case FIK_DELETE:                                /* delete */
            j = strlen(fld);
            for (i = offset; i < j; ++i)
               fld[i] = fld[i+1];
            started = display = 1;
            break;
         case 1082:                                /* insert */
            insert ^= 0x8000;
            started = 1;
            break;
         case F5:
            strcpy(fld,savefld);
            insert = started = offset = 0;
            display = 1;
            break;
         default:
            if (curkey < 32 || curkey >= 127) {
               if (checkkey && (ret = (*checkkey)(curkey)))
                  goto inpfld_end;
               break;                                     /* non alphanum char */
               }
            if (offset >= len) break;                     /* at end of field */
            if ((int)(insert && started && strlen(fld)) >= len)
               break;                                     /* insert & full */
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
               j = strlen(fld);
               while (j >= offset) {
                  fld[j+1] = fld[j];
                  --j;
                  }
               }
            if (offset >= (int)strlen(fld))
               fld[offset+1] = 0;
            fld[offset++] = curkey;
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
        int options,            /* &1 numeric value, &2 integer */
        char *hdg,            /* heading, \n delimited lines */
        char *instr,            /* additional instructions or NULL */
        char *fld,            /* the field itself */
        int len,            /* field length (declare as 1 larger for \0) */
        int (*checkkey)(int)   /* routine to check non data keys, or NULL */
        )
{
   char *charptr;
   int boxwidth,titlelines,titlecol,titlerow;
   int promptcol;
   int i,j;
   char buf[81];
   helptitle();                           /* clear screen, display title */
   setattr(1,0,C_PROMPT_BKGRD,24*80);          /* init rest to background */
   charptr = hdg;                          /* count title lines, find widest */
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
   i = titlelines + 4;                          /* total rows in box */
   titlerow = (25 - i) / 2;                  /* top row of it all when centered */
   titlerow -= titlerow / 4;                  /* higher is better if lots extra */
   titlecol = (80 - boxwidth) / 2;          /* center the box */
   titlecol -= (90 - boxwidth) / 20;
   promptcol = titlecol - (boxwidth-len)/2;
   j = titlecol;                          /* add margin at each side of box */
   if ((i = (82-boxwidth)/4) > 3)
      i = 3;
   j -= i;
   boxwidth += i * 2;
   for (i = -1; i < titlelines+3; ++i)          /* draw empty box */
      setattr(titlerow+i,j,C_PROMPT_LO,boxwidth);
   textcbase = titlecol;                  /* set left margin for putstring */
   putstring(titlerow,0,C_PROMPT_HI,hdg); /* display heading */
   textcbase = 0;
   i = titlerow + titlelines + 4;
   if (instr) {                           /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)))
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }
   else                                   /* default instructions */
      putstringcenter(i,0,80,C_PROMPT_BKGRD,
              "Press ENTER when finished (or ESCAPE to back out)");
   return(input_field(options,C_PROMPT_INPUT,fld,len,
                      titlerow+titlelines+1,promptcol,checkkey));
}

void helptitle()
{
   char msg[80],buf[10];
   setclear(); /* clear the screen */
   sprintf(msg,"WINFRACT  Version %d.%01d",win_release/100,
       (win_release%100)/10);
   if (win_release%10) {
      sprintf(buf,"%01d",win_release%10);
      strcat(msg,buf);
      }
   putstringcenter(0,0,80,C_TITLE,msg);
/* uncomment next for production executable: */
/* return; */
   putstring(0,2,C_TITLE_DEV,"Development Version");
/* putstring(0,2,C_TITLE_DEV,"'Public-Beta' Release"); */
/* replace above by next after creating production release, for release source */
/* putstring(0,3,C_TITLE_DEV, "Customized Version"); */
   putstring(0,53,C_TITLE_DEV,"Not for Public Release");
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

int savegraphics(void) {return 0;}
int restoregraphics(void) {return 0;}

static int screenctr=-1;
#define MAXSCREENS 3
static unsigned char *savescreen[MAXSCREENS];
static int saverc[MAXSCREENS+1];
static FILE *savescf=NULL;
static char scsvfile[]="fractscr.tmp";

void stackscreen()
{
   extern unsigned char wintext_chars[25][80];
   extern unsigned char wintext_attrs[25][80];
   int savebytes;
   int i;
   unsigned char *ptr;

   saverc[screenctr+1] = textrow*80 + textcol;
   if (++screenctr) { /* already have some stacked */
         static char msg[]={"stackscreen overflow"};
      if ((i = screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
         stopmsg(1,msg);
         exit(1);
         }
      savebytes = 25*80;
      if ((ptr = savescreen[i] = malloc((long)(2*savebytes)))) {
         memcpy(ptr,wintext_chars,savebytes);
         memcpy(ptr+savebytes,wintext_attrs,savebytes);
         }
      else {
            static char msg[]={"insufficient memory, aborting"};
fileproblem:   stopmsg(1,msg);
               exit(1);
         }
      setclear();
      }
   else
      setfortext();
}

void unstackscreen()
{
   extern unsigned char wintext_chars[25][80];
   extern unsigned char wintext_attrs[25][80];
   int savebytes;
   unsigned char *ptr;
   textrow = saverc[screenctr] / 80;
   textcol = saverc[screenctr] % 80;
   if (--screenctr >= 0) { /* unstack */
      savebytes = 25*80;
      if ((ptr = savescreen[screenctr])) {
         memcpy(wintext_chars,ptr,savebytes);
         memcpy(wintext_attrs,ptr+savebytes,savebytes);
         wintext_paintscreen(0,80,0,25);
         farmemfree(ptr);
         }
      }
   else
      setforgraphics();
   movecursor(-1,-1);
}

void discardscreen(void)
{
   if (--screenctr >= 0) { /* unstack */
      if (savescreen[screenctr])
         farmemfree(savescreen[screenctr]);
      }
   else
      discardgraphics();
}

void discardgraphics(void)
{
}

void setfortext(void)
{
wintext_texton();
}

void setforgraphics(void)
{
wintext_textoff();
}

void setclear(void)
{
extern int wintext_buffer_init;

    wintext_buffer_init = 0;
    wintext_paintscreen(0,80,0,25);
}

int putstringcenter(int row, int col, int width, int attr, char *msg)
{
   char buf[81];
   int i,j,k;
   i = 0;
   while (msg[i]) ++i; /* strlen for a */
   if (i == 0) return(-1);
   j = (width - i) / 2;
   j -= (width + 10 - i) / 20; /* when wide a bit left of center looks better */
   memset(buf,' ',width);
   buf[width] = 0;
   i = 0;
   k = j;
   while (msg[i]) buf[k++] = msg[i++]; /* strcpy for a far */
   putstring(row,col,attr,buf);
   return j;
}

void putstring(int row, int col, int attr, unsigned char *buf)
{
    extern unsigned char wintext_chars[25][80];
    extern unsigned char wintext_attrs[25][80];
    int i, j, k, maxrow, maxcol;
    char xc, xa;

    if (row == -1) row = textrow;
    if (col == -1) col = textcol;

    j = maxrow = row;
    k = maxcol = col-1;

    for (i = 0; (xc = buf[i]) != 0; i++) {
        if (xc == 13 || xc == 10) {
            j++;
            k = -1;
            }
        else {
            if ((++k) + textcbase >= 80) {
                j++;
                k = 0;
                }
            if (j+textrbase >= 25) j = 24-textrbase;
            if (k+textcbase >= 80) k = 79-textcbase;
            if (maxrow < j) maxrow = j;
            if (maxcol < k) maxcol = k;
            xa = (attr & 0x0ff);
            wintext_chars[j+textrbase][k+textcbase] = xc;
            wintext_attrs[j+textrbase][k+textcbase] = xa;
            }
        }
    if (i > 0) {
        textrow = j;
        textcol = k + 1;
        wintext_paintscreen(
            col+textcbase, maxcol+textcbase,
            row+textrbase, maxrow+textrbase);
        }
}

void setattr(int row, int col, int attr, int count)
{
extern unsigned char wintext_attrs[25][80];
    int i, j, k, maxrow, maxcol;
    char xa;

    j = maxrow = row;
    k = maxcol = col-1;

    xa = (attr & 0x0ff);
    for (i = 0; i < count; i++) {
        if ((++k + textcbase) >= 80) {
            j++;
            k = 0;
            }
        if (j+textrbase >= 25) j = 24-textrbase;
        if (k+textcbase >= 80) k = 79-textcbase;
        if (maxrow < j) maxrow = j;
        if (maxcol < k) maxcol = k;
        wintext_attrs[j+textrbase][k+textcbase] = xa;
        }
    if (count > 0)
        wintext_paintscreen(
            col+textcbase, maxcol+textcbase,
            row+textrbase, maxrow+textrbase);
}

void movecursor(int row, int col)
{
int cursor_type;

    cursor_type = -1;
    if (row >= 25 || col >= 80) {
        row=1;
        col=1;
        cursor_type = 0;
        }
    if (row >= 0)
        textrow = row;
    if (col >= 0)
        textcol = col;
    wintext_cursor(col, row, -1);
}

int keycursor(int row, int col)
{
int i, cursor_style;

if (row == -2 && col == -2)
    return(fractint_getkeypress(1));

if (row == -1)
    row = textrow;
if (col == -1)
    col = textcol;

cursor_style = 1;
if (row < 0) {
    cursor_style = 2;
    row = row & 0x7fff;
    }

i = fractint_getkeypress(0);
if (i == 0)
    wintext_cursor(col, row, cursor_style);
i = fractint_getkeypress(1);
wintext_cursor(col, row, 0);

return(i);

}

/* get a "fractint-style" keystroke, with "help" sensitivity */
int fractint_getkeypress(int option)
{
int i;

restart:
    i = wintext_getkeypress(option);
    if (i == 0)
        return(i);
    /* "fractint-i-size" the keystroke */
    if (i != 0 && (i & 255) == 0)  /* function key? */
        i = (i >> 8) + 1000;
    else
        i = (i & 255);
    if (i == F1) {    /* F1  - bring up Windows-style help */
        if (option == 0) wintext_getkeypress(1);
        winfract_help();
        goto restart;
        }
    if (i == (F1+35)) {  /* Control-F1  - bring up Fractint-style help */
        if (option == 0) wintext_getkeypress(1);
        if (! in_fractint_help) {
            fractint_help();
            goto restart;
            }
        }

return(i);

}

int getakeynohelp(void) {
return(fractint_getkeypress(1));
}

int strncasecmp(char *s,char *t,int ct)
{
   return((int)_fstrnicmp(s,t,ct));
}

extern char temp1[];
extern        int        colors;
extern unsigned char dacbox[256][3];
extern unsigned char olddacbox[256][3];
extern int colorstate;
extern char        colorfile[];
char mapmask[13] = {"*.map"};

void save_palette(void)
{
   FILE *dacfile;
   int i,oldhelpmode;
   oldhelpmode = helpmode;
   stackscreen();
   temp1[0] = 0;
   helpmode = HELPCOLORMAP;
   i = field_prompt(0,"Name of map file to write",NULL,temp1,60,NULL);
   unstackscreen();
   if (i != -1 && temp1[0]) {
      if (strchr(temp1,'.') == NULL)
         strcat(temp1,".map");
      dacfile = fopen(temp1,"w");
      if (dacfile == NULL)
         buzzer(2);
      else {
         for (i = 0; i < colors; i++)
            fprintf(dacfile, "%3d %3d %3d\n",
                    dacbox[i][0] << 2,
                    dacbox[i][1] << 2,
                    dacbox[i][2] << 2);
         memcpy(olddacbox,dacbox,256*3);
         colorstate = 2;
         strcpy(colorfile,temp1);
         }
      fclose(dacfile);
      }
   helpmode = oldhelpmode;
}


int load_palette(void)
{
   int i,oldhelpmode;
   char filename[80];
   oldhelpmode = helpmode;
   strcpy(filename,colorfile);
   stackscreen();
   helpmode = HELPCOLORMAP;
   i = getafilename("Select a MAP File",mapmask,filename);
   unstackscreen();
   if (i >= 0)
      if (ValidateLuts(filename) == 0) {
         memcpy(olddacbox,dacbox,256*3);
         colorstate = 2;
         strcpy(colorfile,filename);
         }
   helpmode = oldhelpmode;
   return (0);
}

void fractint_help(void)
{
   int oldhelpmode;

   in_fractint_help = 1;
   oldhelpmode = helpmode;
   helpmode = FIHELP_INDEX;
   help(0);
   in_fractint_help = 0;
   helpmode = oldhelpmode;
}

extern FILE *parmfile;

int win_make_batch_file(void)
{
   int i;
   int gotinfile;
   char outname[81],buf[256],buf2[128];
   FILE *infile;
   char colorspec[14];
   int colorsonly = 0;
   int maxcolor;
   char *sptr,*sptr2;
   extern char CommandFile[];
   extern char CommandName[];
   extern int  colorstate;
   extern int win_temp1, win_temp2;
   extern char colorfile[];
   extern int  mapset;
   extern char MAP_name[];

   extern char s_cantopen[];
   extern char s_cantwrite[];
   extern char s_cantcreate[];
   extern char s_cantunderstand[];
   extern char s_cantfind[];

      if (strchr(CommandFile,'.') == NULL)
         strcat(CommandFile,".par"); /* default extension .par */
      if (win_temp2 > 0 && win_temp2 <= 256)
            maxcolor = win_temp2;
      strcpy(colorspec,"n");
      if (win_temp1 == 0) {          /* default colors */
         }
      else if (win_temp1 == 2) { /* colors match colorfile */
         strcpy(colorspec,"@");
         sptr = colorfile;
         }
      else {                        /* colors match no .map that we know of */
         strcpy(colorspec,"y");
      }
      if (colorspec[0] == '@') {
         if ((sptr2 = strrchr(sptr,'\\'))) sptr = sptr2 + 1;
         if ((sptr2 = strrchr(sptr,':')))  sptr = sptr2 + 1;
         strncpy(&colorspec[1],sptr,12);
         colorspec[13] = 0;
         }

      strcpy(outname,CommandFile);
      gotinfile = 0;
      if (access(CommandFile,0) == 0) { /* file exists */
         gotinfile = 1;
         if (access(CommandFile,6)) {
            sprintf(buf,s_cantwrite,CommandFile);
            stopmsg(0,buf);
            return(0);
            }
         i = strlen(outname);
         while (--i >= 0 && outname[i] != '\\')
         outname[i] = 0;
         strcat(outname,"fractint.tmp");
         infile = fopen(CommandFile,"rt");
         setvbuf(infile,suffix,_IOFBF,4096); /* improves speed */
         }
      if ((parmfile = fopen(outname,"wt")) == NULL) {
         sprintf(buf,s_cantcreate,outname);
         stopmsg(0,buf);
         if (gotinfile) fclose(infile);
         return(0);
         }

      if (gotinfile) {
         while (file_gets(buf,255,infile) >= 0) {
            if (strchr(buf,'{')                    /* entry heading? */
              && sscanf(buf," %40[^ \t({]",buf2)
              && stricmp(buf2,CommandName) == 0) { /* entry with same name */
               sprintf(buf2,"File already has an entry named %s\n\
OK to replace it, Cancel to back out",CommandName);
               if (stopmsg(18,buf2) < 0) {            /* cancel */
                  fclose(infile);
                  fclose(parmfile);
                  unlink(outname);
                  return(0);
                  }
               while (strchr(buf,'}') == NULL
                 && file_gets(buf,255,infile) > 0 ) { } /* skip to end of set */
               break;
               }
            fputs(buf,parmfile);
            fputc('\n',parmfile);
            }
         }

      fprintf(parmfile,"%-19s{",CommandName);

         {
            /* guarantee that there are no blank comments above the last
               non-blank par_comment */
            int i, last;
            for(last=-1,i=0;i<4;i++)
               if(*par_comment[i])
                  last=i;
            for(i=0;i<last;i++)
               if(*CommandComment[i]=='\0')
                  strcpy(CommandComment[i],";");
         }
         if (CommandComment[0][0])
            fprintf(parmfile, " ; %s", CommandComment[0]);
         fputc('\n', parmfile);
         {
            int k;
            char buf[25];
            memset(buf, ' ', 23);
            buf[23] = 0;
            buf[21] = ';';
            for(k=1;k<4;k++)
               if (CommandComment[k][0])
                  fprintf(parmfile, "%s%s\n", buf, CommandComment[k]);
            if (patchlevel != 0 && colorsonly == 0)
               fprintf(parmfile, "%s %s Version %d Patchlevel %d\n", buf,
                  Fractint, release, patchlevel);
         }
      write_batch_parms(colorspec,colorsonly,maxcolor,0,0); /* write the parameters */
      fprintf(parmfile,"  }\n\n");

      if (gotinfile) {        /* copy the rest of the file */
         while ((i = file_gets(buf,255,infile)) == 0) { } /* skip blanks */
         while (i >= 0) {
            fputs(buf,parmfile);
            fputc('\n',parmfile);
            i = file_gets(buf,255,infile);
            }
         fclose(infile);
         }
      fclose(parmfile);
      if (gotinfile) {        /* replace the original file with the new */
         unlink(CommandFile);              /* success assumed on these lines       */
         rename(outname,CommandFile); /* since we checked earlier with access */
         }

return(1);
}


static int menutype;
#define MENU_HDG 3
#define MENU_ITEM 1

static int menu_checkkey(int curkey,int choice)
{ /* choice is dummy used by other routines called by fullscreen_choice() */
   int testkey;
   testkey = choice; /* for warning only */
   testkey = (curkey>='A' && curkey<='Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
   /* We use F2 for shift-@, annoyingly enough */
   if (testkey == F2) return(0-testkey);
#endif
   if(testkey == '2')
      testkey = '@';
   if (strchr("#@2txyzgvir3dj",testkey) || testkey == INSERT || testkey == 2
     || testkey == ESC || testkey == FIK_DELETE || testkey == 6) /*RB 6== ctrl-F for sound menu */
      return(0-testkey);
   if (menutype) {
      if (strchr("\\sobpkrh",testkey) || testkey == TAB
        || testkey == 1 || testkey == 5 || testkey == 8
        || testkey == 16
        || testkey == 19 || testkey == 21) /* ctrl-A, E, H, P, S, U */
         return(0-testkey);
      if (testkey == ' ')
         if ((curfractalspecific->tojulia != NOFRACTAL
              && param[0] == 0.0 && param[1] == 0.0)
           || curfractalspecific->tomandel != NOFRACTAL)
         return(0-testkey);
      if (gotrealdac && colors >= 16) {
         if (strchr("c+-",testkey))
            return(0-testkey);
         if (colors > 16
           && (testkey == 'a' || (!reallyega && testkey == 'e')))
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

#define LOADPROMPTSCHOICES(X,Y)     {\
   static char tmp[] = { Y };\
   choices[X]= (char *)tmp;\
   }

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
   choicekey[nextleft+=2] = FIK_DELETE;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"draw fractal           <D>  ");
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
/* History not implemented in WinFract
      choicekey[nextleft+=2] = 'h';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"return to prior image  <h>   ");

      choicekey[nextleft+=2] = 8;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"reverse thru history <ctl-h> ");
 */
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
/* view window not implemented in WinFract
   choicekey[nextleft+=2] = 'v';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"view window options... <v>  ");
 */
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
/* Sound not yet implemented in WinFract
   if (fullmenu) {
      choicekey[nextleft+=2] = 6;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"sound parms...       <ctl-f>");
   }
 */
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
/* No place to shell to in WinFract
   choicekey[nextright+=2] = 'd';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"shell to dos             <d>  ");
*/
   choicekey[nextright+=2] = 'g';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"give command string      <g>  ");
   choicekey[nextright+=2] = ESC;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"quit "FRACTINT"           <esc> ");
   choicekey[nextright+=2] = INSERT;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"restart "FRACTINT"        <ins> ");
   if (fullmenu && gotrealdac && colors >= 16) {
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
         if (!reallyega) {
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

   i = (keypressed()) ? getakey() : 0;
   if (menu_checkkey(i,0) == 0) {
      helpmode = HELPMAIN;         /* switch help modes */
      if ((nextleft += 2) < nextright)
         nextleft = nextright + 1;
      i = fullscreen_choice(CHOICEMENU+CHOICESCRUNCH,
          MAIN_MENU,
          NULL,NULL,nextleft,(char * *)choices,attributes,
          2,nextleft/2,29,0,NULL,NULL,NULL,menu_checkkey);
      if (i == -1)     /* escape */
         i = ESC;
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
   if (i == ESC) {             /* escape from menu exits Fractint */
      static char s[] = "Exit from WinFract (y/n)? y";
      helptitle();
      setattr(1,0,C_GENERAL_MED,24*80);
      for (i = 9; i <= 11; ++i)
         setattr(i,18,C_GENERAL_INPUT,40);
      putstringcenter(10,18,40,C_GENERAL_INPUT,s);
      movecursor(25,80);
      while ((i = getakey()) != 'y' && i != 'Y' && i != 13) {
         if (i == 'n' || i == 'N')
            goto top;
         }
      goodbye(); /* this needs to be a Windows command JCO */
      }
   if (i == TAB) {
      tab_display();
      i = 0;
      }
   if (i == ENTER || i == ENTER_2)
      i = 0;                 /* don't trigger new calc */
   tabmode = oldtabmode;
   return(i);
}

