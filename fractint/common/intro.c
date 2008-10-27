
/*
 * intro.c
 *
 * FRACTINT intro screen (authors & credits)
 *
 *
 */

#include <time.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

/* stuff from fractint */

#ifdef XFRACT
extern int slowdisplay;
#endif

void intro(void)
   {
   /* following overlayed data safe if "putstrings" are resident */
   static FCODE PRESS_ENTER[] = {"Press ESC for main menu, F1 for help."};
   int       toprow, botrow, i, j, delaymax;
   char      oldchar;
   int       authors[100];              /* this should be enough for awhile */
   char far *credits;
   char far *screen_text;
   int       oldlookatmouse;
   int       oldhelpmode;

   timer_start -= clock_ticks();                /* "time out" during help */
   oldlookatmouse = lookatmouse;
   oldhelpmode = helpmode;
   lookatmouse = 0;                     /* de-activate full mouse checking */

   screen_text = MK_FP(extraseg, 0);

   i = 32767 + read_help_topic(INTRO_AUTHORS, 0, 32767, screen_text);
   screen_text[i++] = '\0';
   credits = screen_text + i;
   i = 32767 + read_help_topic(INTRO_CREDITS, 0, 32767, credits);
   credits[i++] = '\0';

   j = 0;
   authors[j] = 0;              /* find the start of each credit-line */
   for (i = 0; credits[i] != 0; i++)
      if (credits[i] == 10)
         authors[++j] = i+1;
   authors[j+1] = i;

   helptitle();
#define END_MAIN_AUTHOR 5
   toprow = END_MAIN_AUTHOR+1;
#ifndef XFRACT
   botrow = 21;
#else
   botrow = 20;
   putstringcenter(botrow+1,0,80,C_TITLE,
   "Unix/X port of fractint by Ken Shirriff");
#endif
   putstringcenter(1,0,80,C_TITLE, PRESS_ENTER);
   putstring(2,0,C_CONTRIB,screen_text);
   setattr(2,0,C_AUTHDIV1,80);
   setattr(END_MAIN_AUTHOR,0,C_AUTHDIV1,80);
   setattr(3,0,C_PRIMARY,80*(END_MAIN_AUTHOR-3));
   for (i = 3; i < END_MAIN_AUTHOR; ++i)
      setattr(i,21,C_CONTRIB,58);
   setattr(toprow,0,C_CONTRIB,(botrow-END_MAIN_AUTHOR)*80);
   setattr(botrow+1,0,C_AUTHDIV2,80);
   setattr(botrow+2,0,C_PROMPT_BKGRD,80);
   setattr(botrow+3,0,C_TITLE_LOW,160);
   i = botrow - toprow;
   srand((unsigned int)clock_ticks());
   j = rand()%(j-(botrow-toprow)); /* first to use */
   i = j+botrow-toprow; /* last to use */
   oldchar = credits[authors[i+1]];
   credits[authors[i+1]] = 0;
   putstring(toprow,0,C_CONTRIB,credits+authors[j]);
   credits[authors[i+1]] = oldchar;
   delaymax = 10;
   movecursor(25,80); /* turn it off */
   helpmode = HELPMENU;

#ifdef XFRACT
   if (slowdisplay) delaymax *= 15;
#endif
loop_intro:
   for (j = 0; j < delaymax && !(keypressed()); j++)
      delay(100);
   if (j = keypressed()) /* set j to returned key */
      getakey();
   if (menu_checkkey(j,0) || j == 109) /* menu key or 'm' */
      goto intro_end;
   if (j == 32) { /* spacebar pauses */
wait_again:
#ifndef XFRACT
      while (!keypressed()) ;
#else
      waitkeypressed(0);
#endif
      if (j = keypressed()) /* set j to returned key */
         getakey();
      if (menu_checkkey(j,0) || j == 109) /* menu key or 'm' */
         goto intro_end;
      if (j!= 32) /* spacebar */
         goto wait_again;
   }

   scrollup(toprow, botrow);
   i++;
   if (credits[authors[i]] == 0)
      i = 0;
   oldchar = credits[authors[i+1]];
   credits[authors[i+1]] = 0;
   putstring(botrow,0,C_CONTRIB,&credits[authors[i]]);
   credits[authors[i+1]] = oldchar;
   movecursor(25,80); /* turn it off */
   goto loop_intro;

intro_end:
   ungetakey(j);
   lookatmouse = oldlookatmouse;                /* restore the mouse-checking */
   helpmode = oldhelpmode;
   return ;
   }
