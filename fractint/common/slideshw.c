/***********************************************************************/
/* These routines are called by getakey to allow keystrokes to control */
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

static void sleep_secs(int);
static int  showtempmsg_txt(int,int,int,int,char *);
static void message(int secs, char far *buf);
static void slideshowerr(char far *msg);
static int  get_scancode(char *mn);
static void get_mnemonic(int code, char *mnemonic);

static FCODE s_ENTER     [] = "ENTER"     ;
static FCODE s_INSERT    [] = "INSERT"    ;
static FCODE s_DELETE    [] = "DELETE"    ;
static FCODE s_ESC       [] = "ESC"       ;
static FCODE s_TAB       [] = "TAB"       ;
static FCODE s_PAGEUP    [] = "PAGEUP"    ;
static FCODE s_PAGEDOWN  [] = "PAGEDOWN"  ;
static FCODE s_HOME      [] = "HOME"      ;
static FCODE s_END       [] = "END"       ;
static FCODE s_LEFT      [] = "LEFT"      ;
static FCODE s_RIGHT     [] = "RIGHT"     ;
static FCODE s_UP        [] = "UP"        ;
static FCODE s_DOWN      [] = "DOWN"      ;
static FCODE s_F1        [] = "F1"        ;
static FCODE s_CTRL_RIGHT[] = "CTRL_RIGHT";
static FCODE s_CTRL_LEFT [] = "CTRL_LEFT" ;
static FCODE s_CTRL_DOWN [] = "CTRL_DOWN" ;
static FCODE s_CTRL_UP   [] = "CTRL_UP"   ;
static FCODE s_CTRL_END  [] = "CTRL_END"  ;
static FCODE s_CTRL_HOME [] = "CTRL_HOME" ;

#define MAX_MNEMONIC    20   /* max size of any mnemonic string */

struct scancodes
{
   int code;
   FCODE *mnemonic;
};

static struct scancodes far scancodes[] =
{
   {  ENTER,         s_ENTER     },
   {  INSERT,        s_INSERT    },
   {  DELETE,        s_DELETE    },
   {  ESC,           s_ESC       },
   {  TAB,           s_TAB       },
   {  PAGE_UP,       s_PAGEUP    },
   {  PAGE_DOWN,     s_PAGEDOWN  },
   {  HOME,          s_HOME      },
   {  END,           s_END       },
   {  LEFT_ARROW,    s_LEFT      },
   {  RIGHT_ARROW,   s_RIGHT     },
   {  UP_ARROW,      s_UP        },
   {  DOWN_ARROW,    s_DOWN      },
   {  F1,            s_F1        },
   {  RIGHT_ARROW_2, s_CTRL_RIGHT},
   {  LEFT_ARROW_2,  s_CTRL_LEFT },
   {  DOWN_ARROW_2,  s_CTRL_DOWN },
   {  UP_ARROW_2,    s_CTRL_UP   },
   {  CTL_END,       s_CTRL_END  },
   {  CTL_HOME,      s_CTRL_HOME },
   {  -1,             NULL       }
};
#define stop sizeof(scancodes)/sizeof(struct scancodes)-1

static int get_scancode(char *mn)
{
   int i;
   i = 0;
   for(i=0;i< stop;i++)
      if(far_strcmp((char far *)mn,scancodes[i].mnemonic)==0)
         break;
   return(scancodes[i].code);
}

static void get_mnemonic(int code,char *mnemonic)
{
   int i;
   i = 0;
   *mnemonic = 0;
   for(i=0;i< stop;i++)
      if(code == scancodes[i].code)
      {
         far_strcpy(mnemonic,scancodes[i].mnemonic);
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
static FCODE smsg[] = "MESSAGE";
static FCODE sgoto[] = "GOTO";
static FCODE scalcwait[] = "CALCWAIT";
static FCODE swait[] = "WAIT";

/* places a temporary message on the screen in text mode */
static int showtempmsg_txt(int row, int col, int attr,int secs,char *txt)
{
   int savescrn[80];
   int i;
   if(text_type > 1)
      return(1);
   for(i=0;i<80;i++)
   {
      movecursor(row,i);
      savescrn[i] = get_a_char();
   }
   putstring(row,col,attr,txt);
   movecursor(25,80);
   sleep_secs(secs);
   for(i=0;i<80;i++)
   {
      movecursor(row,i);
      put_a_char(savescrn[i]);
   }
   return(0);
}

static void message(int secs, char far *buf)
{
   int i;
   char nearbuf[41];
   i = -1;
   while(buf[++i] && i< 40)
      nearbuf[i] = buf[i];
   nearbuf[i] = 0;
   if(text_type < 2)
      showtempmsg_txt(0,0,7,secs,nearbuf);
   else if (showtempmsg(nearbuf) == 0)
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
   if(calcwait)
   {
      if(calc_status == 1 || busy) /* restart timer - process not done */
         return(0); /* wait for calc to finish before reading more keystrokes */
      calcwait = 0;
   }
   if(fpss==NULL)   /* open files first time through */
      if(startslideshow()==0)
         {
         stopslideshow();
         return (0);
         }

   if(ticks) /* if waiting, see if waited long enough */
   {
      if(clock_ticks() - starttick < ticks) /* haven't waited long enough */
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
   if(repeats>0)
   {
      repeats--;
      return(last1);
   }
start:
   if(quotes) /* reading a quoted string */
   {
      if((out=fgetc(fpss)) != '\"' && out != EOF)
         return(last1 = out);
      quotes = 0;
   }
   /* skip white space: */
   while ((out=fgetc(fpss)) == ' ' || out == '\t' || out == '\n') { }
   switch(out)
   {
      case EOF:
         stopslideshow();
         return(0);
      case '\"':        /* begin quoted string */
         quotes = 1;
         goto start;
      case ';':         /* comment from here to end of line, skip it */
         while((out=fgetc(fpss)) != '\n' && out != EOF) { }
         goto start;
      case '*':
         if (fscanf(fpss,"%d",&repeats) != 1
           || repeats <= 1 || repeats >= 256 || feof(fpss))
         {
            static FCODE msg[] = "error in * argument";
            slideshowerr(msg);
            last1 = repeats = 0;
         }
         repeats -= 2;
         return(out = last1);
   }

   i = 0;
   for(;;) /* get a token */
   {
      if(i < 80)
         buffer[i++] = (char)out;
      if((out=fgetc(fpss)) == ' ' || out == '\t' || out == '\n' || out == EOF)
         break;
   }
   buffer[i] = 0;
   if(buffer[i-1] == ':')
      goto start;
   out = -12345;
   if(isdigit(buffer[0]))       /* an arbitrary scan code number - use it */
         out=atoi(buffer);
   else if(far_strcmp((char far *)buffer,smsg)==0)
      {
         int secs;
         out = 0;
         if (fscanf(fpss,"%d",&secs) != 1)
         {
            static FCODE msg[] = "MESSAGE needs argument";
            slideshowerr(msg);
         }
         else
         {
            int len;
            char buf[41];
            char *dummy; /* to quiet compiler */
            buf[40] = 0;
            dummy = fgets(buf,40,fpss);
            len = strlen(buf);
            buf[len-1]=0; /* zap newline */
            message(secs,(char far *)buf);
         }
         out = 0;
      }
   else if(far_strcmp((char far *)buffer,sgoto)==0)
      {
         if (fscanf(fpss,"%s",buffer) != 1)
         {
            static FCODE msg[] = "GOTO needs target";
            slideshowerr(msg);
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
            } while( err == 1 && strcmp(buffer1,buffer) != 0);
            if(feof(fpss))
            {
               static FCODE msg[] = "GOTO target not found";
               slideshowerr(msg);
               return(0);
            }
            goto start;
         }
      }
   else if((i = get_scancode(buffer)) > 0)
         out = i;
   else if(far_strcmp(swait,(char far *)buffer)==0)
      {
         float fticks;
         err = fscanf(fpss,"%f",&fticks); /* how many ticks to wait */
         fticks *= CLK_TCK;             /* convert from seconds to ticks */
         if(err==1)
         {
            ticks = (long)fticks;
            starttick = clock_ticks();  /* start timing */
         }
         else
         {
            static FCODE msg[] = "WAIT needs argument";
            slideshowerr(msg);
         }
         slowcount = out = 0;
      }
   else if(far_strcmp(scalcwait,(char far *)buffer)==0) /* wait for calc to finish */
      {
         calcwait = 1;
         slowcount = out = 0;
      }
   else if((i=check_vidmode_keyname(buffer)) != 0)
      out = i;
   if(out == -12345)
   {
      char msg[MSGLEN];
      sprintf(msg,s_cantunderstand,buffer);
      slideshowerr(msg);
      out = 0;
   }
   return(last1 = out);
}

int
startslideshow()
{
   if((fpss=fopen(autoname,"r"))==NULL)
      slides = 0;
   ticks = 0;
   quotes = 0;
   calcwait = 0;
   slowcount = 0;
   return(slides);
}

void stopslideshow()
{
   if(fpss)
      fclose(fpss);
   fpss = NULL;
   slides = 0;
}

void recordshw(int key)
{
   char mn[MAX_MNEMONIC];
   float dt;
   dt = (float)ticks;      /* save time of last call */
   ticks=clock_ticks();  /* current time */
   if(fpss==NULL)
      if((fpss=fopen(autoname,"w"))==NULL)
         return;
   dt = ticks-dt;
   dt /= CLK_TCK;  /* dt now in seconds */
   if(dt > .5) /* don't bother with less than half a second */
   {
      if(quotes) /* close quotes first */
      {
         quotes=0;
         fprintf(fpss,"\"\n");
      }
      fprintf(fpss,"WAIT %4.1f\n",dt);
   }
   if(key >= 32 && key < 128)
   {
      if(!quotes)
      {
         quotes=1;
         fputc('\"',fpss);
      }
      fputc(key,fpss);
   }
   else
   {
      if(quotes) /* not an ASCII character - turn off quotes */
      {
         fprintf(fpss,"\"\n");
         quotes=0;
      }
      get_mnemonic(key,mn);
      if(*mn)
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
   while(clock_ticks() < stop && kbhit() == 0) { } /* bailout if key hit */
}

static void slideshowerr(char far *msg)
{
   char msgbuf[300];
   static FCODE errhdg[] = "Slideshow error:\n";
   stopslideshow();
   far_strcpy(msgbuf,errhdg);
   far_strcat(msgbuf,msg);
   stopmsg(0,msgbuf);
}
