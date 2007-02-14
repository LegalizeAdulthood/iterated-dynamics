/***********************************************************************/
/* These routines are called by driver_get_key to allow keystrokes to control */
/* Fractint to be read from a file.                                    */
/***********************************************************************/

#include <ctype.h>
#include <time.h>
#include <string.h>
#ifndef XFRACT
#include <conio.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

static void sleep_secs(int);
static int  showtempmsg_txt(int,int,int,int,char *);
static void message(int secs, char *buf);
static void slideshowerr(char *msg);
static int  get_scancode(char *mn);
static void get_mnemonic(int code, char *mnemonic);

#define MAX_MNEMONIC    20   /* max size of any mnemonic string */

struct scancodes
{
   int code;
   char *mnemonic;
};

static struct scancodes scancodes[] =
{
	{ FIK_ENTER,			"ENTER"     },
	{ FIK_INSERT,			"INSERT"    },
	{ FIK_DELETE,			"DELETE"    },
	{ FIK_ESC,				"ESC"       },
	{ FIK_TAB,				"TAB"       },
	{ FIK_PAGE_UP,			"PAGEUP"    },
	{ FIK_PAGE_DOWN,		"PAGEDOWN"  },
	{ FIK_HOME,				"HOME"      },
	{ FIK_END,				"END"       },
	{ FIK_LEFT_ARROW,		"LEFT"      },
	{ FIK_RIGHT_ARROW,		"RIGHT"     },
	{ FIK_UP_ARROW,			"UP"        },
	{ FIK_DOWN_ARROW,		"DOWN"      },
	{ FIK_F1,				"F1"        },
	{ FIK_CTL_RIGHT_ARROW,	"CTRL_RIGHT"},
	{ FIK_CTL_LEFT_ARROW,	"CTRL_LEFT" },
	{ FIK_CTL_DOWN_ARROW,	"CTRL_DOWN" },
	{ FIK_CTL_UP_ARROW,		"CTRL_UP"   },
	{ FIK_CTL_END,			"CTRL_END"  },
	{ FIK_CTL_HOME,			"CTRL_HOME" },
	{ -1,					NULL		}
};
#define stop sizeof(scancodes)/sizeof(struct scancodes)-1

static int get_scancode(char *mn)
{
   int i;
   i = 0;
   for (i=0; i< stop; i++)
      if (strcmp((char *)mn,scancodes[i].mnemonic)==0)
         break;
   return(scancodes[i].code);
}

static void get_mnemonic(int code,char *mnemonic)
{
   int i;
   i = 0;
   *mnemonic = 0;
   for (i=0; i< stop; i++)
      if (code == scancodes[i].code)
      {
         strcpy(mnemonic,scancodes[i].mnemonic);
         break;
      }   
}
#undef stop

char busy = 0;
static FILE *fpss = NULL;
static long starttick;
static long ticks;
static int slowcount;
static unsigned int quotes;
static char calcwait = 0;
static int repeats = 0;
static int last1 = 0;

/* places a temporary message on the screen in text mode */
static int showtempmsg_txt(int row, int col, int attr,int secs,char *txt)
{
   int savescrn[80];
   int i;

   for (i=0; i<80; i++)
   {
      driver_move_cursor(row,i);
      savescrn[i] = driver_get_char_attr();
   }
   driver_put_string(row,col,attr,txt);
   driver_hide_text_cursor();
   sleep_secs(secs);
   for (i=0; i<80; i++)
   {
      driver_move_cursor(row,i);
      driver_put_char_attr(savescrn[i]);
   }
   return(0);
}

static void message(int secs, char *buf)
{
	char nearbuf[41] = { 0 };
	strncpy(nearbuf, buf, NUM_OF(nearbuf)-1);
	showtempmsg_txt(0,0,7,secs,nearbuf);
	if (showtempmsg(nearbuf) == 0)
    {
		sleep_secs(secs);
		cleartempmsg();
    }
}

/* this routine reads the file autoname and returns keystrokes */
int slideshw()
{
   int out,err,i;
   char buffer[81];
   if (calcwait)
   {
      if (calc_status == CALCSTAT_IN_PROGRESS || busy) /* restart timer - process not done */
         return(0); /* wait for calc to finish before reading more keystrokes */
      calcwait = 0;
   }
   if (fpss==NULL)   /* open files first time through */
      if (startslideshow()==0)
         {
         stopslideshow();
         return (0);
         }

   if (ticks) /* if waiting, see if waited long enough */
   {
      if (clock_ticks() - starttick < ticks) /* haven't waited long enough */
         return(0);
      ticks = 0;
   }
   if (++slowcount <= 18)
   {
      starttick = clock_ticks();
      ticks = CLK_TCK/5; /* a slight delay so keystrokes are visible */
      if (slowcount > 10)
         ticks /= 2;
   }
   if (repeats>0)
   {
      repeats--;
      return(last1);
   }
start:
   if (quotes) /* reading a quoted string */
   {
      if ((out=fgetc(fpss)) != '\"' && out != EOF)
         return(last1 = out);
      quotes = 0;
   }
   /* skip white space: */
   while ((out=fgetc(fpss)) == ' ' || out == '\t' || out == '\n') { }
   switch (out)
   {
      case EOF:
         stopslideshow();
         return(0);
      case '\"':        /* begin quoted string */
         quotes = 1;
         goto start;
      case ';':         /* comment from here to end of line, skip it */
         while ((out=fgetc(fpss)) != '\n' && out != EOF) { }
         goto start;
      case '*':
         if (fscanf(fpss,"%d",&repeats) != 1
           || repeats <= 1 || repeats >= 256 || feof(fpss))
         {
            slideshowerr("error in * argument");
            last1 = repeats = 0;
         }
         repeats -= 2;
         return(out = last1);
   }

   i = 0;
   while (1) /* get a token */
   {
      if (i < 80)
         buffer[i++] = (char)out;
      out=fgetc(fpss);
	  if (out == ' ' || out == '\t' || out == '\n' || out == EOF)
         break;
   }
   buffer[i] = 0;
   if (buffer[i-1] == ':')
      goto start;
   out = -12345;
   if (isdigit(buffer[0]))       /* an arbitrary scan code number - use it */
         out=atoi(buffer);
   else if (strcmp((char *)buffer,"MESSAGE")==0)
      {
         int secs;
         out = 0;
         if (fscanf(fpss,"%d",&secs) != 1)
         {
            slideshowerr("MESSAGE needs argument");
         }
         else
         {
            int len;
            char buf[41];
            buf[40] = 0;
            fgets(buf,40,fpss);
            len = (int) strlen(buf);
            buf[len-1]=0; /* zap newline */
            message(secs,(char *)buf);
         }
         out = 0;
      }
   else if (strcmp((char *)buffer,"GOTO")==0)
      {
         if (fscanf(fpss,"%s",buffer) != 1)
         {
            slideshowerr("GOTO needs target");
            out = 0;
         }
         else
         {
            char buffer1[80];
            rewind(fpss);
            strcat(buffer,":");
            do
            {
               err = fscanf(fpss,"%s",buffer1);
            } while ( err == 1 && strcmp(buffer1,buffer) != 0);
            if (feof(fpss))
            {
               slideshowerr("GOTO target not found");
               return(0);
            }
            goto start;
         }
      }
   else if ((i = get_scancode(buffer)) > 0)
         out = i;
   else if (strcmp("WAIT",(char *)buffer)==0)
      {
         float fticks;
         err = fscanf(fpss,"%f",&fticks); /* how many ticks to wait */
		 driver_set_keyboard_timeout((int) (fticks*1000.f));
         fticks *= CLK_TCK;             /* convert from seconds to ticks */
         if (err==1)
         {
            ticks = (long)fticks;
            starttick = clock_ticks();  /* start timing */
         }
         else
         {
            slideshowerr("WAIT needs argument");
         }
         slowcount = out = 0;
      }
   else if (strcmp("CALCWAIT",(char *)buffer)==0) /* wait for calc to finish */
      {
         calcwait = 1;
         slowcount = out = 0;
      }
   else if ((i=check_vidmode_keyname(buffer)) != 0)
      out = i;
   if (out == -12345)
   {
      char msg[MSGLEN];
      sprintf(msg,"Can't understand %s",buffer);
      slideshowerr(msg);
      out = 0;
   }
   return(last1 = out);
}

int
startslideshow()
{
   fpss=fopen(autoname,"r");
   if (fpss==NULL)
      g_slides = SLIDES_OFF;
   ticks = 0;
   quotes = 0;
   calcwait = 0;
   slowcount = 0;
   return(g_slides);
}

void stopslideshow()
{
   if (fpss)
      fclose(fpss);
   fpss = NULL;
   g_slides = SLIDES_OFF;
}

void recordshw(int key)
{
   char mn[MAX_MNEMONIC];
   float dt;
   dt = (float)ticks;      /* save time of last call */
   ticks=clock_ticks();  /* current time */
   if (fpss==NULL)
   {
	   fpss=fopen(autoname,"w");
      if (fpss==NULL)
         return;
   }
   dt = ticks-dt;
   dt /= CLK_TCK;  /* dt now in seconds */
   if (dt > .5) /* don't bother with less than half a second */
   {
      if (quotes) /* close quotes first */
      {
         quotes=0;
         fprintf(fpss,"\"\n");
      }
      fprintf(fpss,"WAIT %4.1f\n",dt);
   }
   if (key >= 32 && key < 128)
   {
      if (!quotes)
      {
         quotes=1;
         fputc('\"',fpss);
      }
      fputc(key,fpss);
   }
   else
   {
      if (quotes) /* not an ASCII character - turn off quotes */
      {
         fprintf(fpss,"\"\n");
         quotes=0;
      }
      get_mnemonic(key,mn);
      if (*mn)
          fprintf(fpss,"%s",mn);
      else if (check_vidmode_key(0,key) >= 0)
         {
            char buf[10];
            vidmode_keyname(key,buf);
            fprintf(fpss,buf);
         }
      else /* not ASCII and not FN key */
         fprintf(fpss,"%4d",key);
      fputc('\n',fpss);
   }
}

/* suspend process # of seconds */
static void sleep_secs(int secs)
{
   long stop;
   stop = clock_ticks() + (long)secs*CLK_TCK;
   while (clock_ticks() < stop && kbhit() == 0) { } /* bailout if key hit */
}

static void slideshowerr(char *msg)
{
	char msgbuf[300] = { "Slideshow error:\n" };
   stopslideshow();
   strcat(msgbuf,msg);
   stopmsg(0,msgbuf);
}
