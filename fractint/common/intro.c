
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
#include "drivers.h"

/* stuff from fractint */

#ifdef XFRACT
extern int slowdisplay;
#endif

void intro(void)
   {
   /* following overlayed data safe if "putstrings" are resident */
#ifdef XFRACT
   static FCODE PRESS_ENTER[] = {"Press ENTER for main menu, Shift-1 for help."};
#else
   static FCODE PRESS_ENTER[] = {"Press ENTER for main menu, F1 for help."};
#endif
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
   putstringcenter(21,0,80,C_TITLE,
   "Unix/X port of fractint by Ken Shirriff");
#endif
   putstringcenter(1,0,80,C_TITLE, PRESS_ENTER);
   driver_put_string(2,0,C_CONTRIB,screen_text);
   driver_set_attr(2,0,C_AUTHDIV1,80);
   driver_set_attr(END_MAIN_AUTHOR,0,C_AUTHDIV1,80);
   driver_set_attr(22,0,C_AUTHDIV2,80);
   driver_set_attr(3,0,C_PRIMARY,80*(END_MAIN_AUTHOR-3));
   driver_set_attr(23,0,C_TITLE_LOW,160);
   for (i = 3; i < END_MAIN_AUTHOR; ++i)
      driver_set_attr(i,21,C_CONTRIB,58);
   driver_set_attr(toprow,0,C_CONTRIB,(21-END_MAIN_AUTHOR)*80);
   i = botrow - toprow;
   srand((unsigned int)clock_ticks());
   j = rand()%(j-(botrow-toprow)); /* first to use */
   i = j+botrow-toprow; /* last to use */
   oldchar = credits[authors[i+1]];
   credits[authors[i+1]] = 0;
   driver_put_string(toprow,0,C_CONTRIB,credits+authors[j]);
   credits[authors[i+1]] = oldchar;
   delaymax = 10;
   driver_hide_text_cursor();
   helpmode = HELPMENU;
   while (! keypressed())
      {
#ifdef XFRACT
      if (slowdisplay) delaymax *= 15;
#endif
      for (j = 0; j < delaymax && !(keypressed()); j++)
         delay(100);
      if (keypressed() == 32)
         {      /* spacebar pauses */
         getakey();
#ifndef XFRACT
         while (!keypressed()) ;
#else
         waitkeypressed(0);
#endif
         if (keypressed() == 32)
            getakey();
         }
      delaymax = 15;
      driver_scroll_up(toprow, botrow);
      i++;
      if (credits[authors[i]] == 0)
         i = 0;
      oldchar = credits[authors[i+1]];
      credits[authors[i+1]] = 0;
      driver_put_string(botrow,0,C_CONTRIB,&credits[authors[i]]);
      driver_set_attr(botrow,0,C_CONTRIB,80);
      credits[authors[i+1]] = oldchar;
      driver_hide_text_cursor(); /* turn it off */
      }

   lookatmouse = oldlookatmouse;                /* restore the mouse-checking */
   helpmode = oldhelpmode;
   return ;
   }
