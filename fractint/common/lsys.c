
#include <string.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "lsys.h"
#include "drivers.h"

struct lsys_cmd {
    void (*f)(struct lsys_turtlestatei *);
    long n;
    char ch;
};

static int _fastcall readLSystemFile(char *);
static void _fastcall free_rules_mem(void);
static int _fastcall rule_present(char symbol);
static int _fastcall save_rule(char *,char **);
static int _fastcall append_rule(char *rule, int index);
static void free_lcmds(void);
static struct lsys_cmd * _fastcall findsize(struct lsys_cmd *,struct lsys_turtlestatei *, struct lsys_cmd **,int);
static struct lsys_cmd * drawLSysI(struct lsys_cmd *command,struct lsys_turtlestatei *ts, struct lsys_cmd **rules,int depth);
static int lsysi_findscale(struct lsys_cmd *command, struct lsys_turtlestatei *ts, struct lsys_cmd **rules, int depth);
static struct lsys_cmd *LSysISizeTransform(char *s, struct lsys_turtlestatei *ts);
static struct lsys_cmd *LSysIDrawTransform(char *s, struct lsys_turtlestatei *ts);
static void _fastcall lsysi_dosincos(void);

static void lsysi_doslash(struct lsys_turtlestatei *cmd);
static void lsysi_dobslash(struct lsys_turtlestatei *cmd);
static void lsysi_doat(struct lsys_turtlestatei *cmd);
static void lsysi_dopipe(struct lsys_turtlestatei *cmd);
static void lsysi_dosizedm(struct lsys_turtlestatei *cmd);
static void lsysi_dosizegf(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawd(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawm(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawg(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawf(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawc(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawgt(struct lsys_turtlestatei *cmd);
static void lsysi_dodrawlt(struct lsys_turtlestatei *cmd);

/* Some notes to Adrian from PB, made when I integrated with v15:
     printfs changed to work with new user interface
     bug at end of readLSystemFile, the line which said rulind=0 fixed
       to say *rulind=0
     the calloc was not worthwhile, it was just for a 54 byte area, cheaper
       to keep it as a static;  but there was a static 201 char buffer I
       changed to not be static
     use of strdup was a nono, caused problems running out of space cause
       the memory allocated each time was never freed; I've changed to
       use memory and to free when done
   */

#define sins ((long *)(boxy))
#define coss (((long *)(boxy)+50)) /* 50 after the start of sins */
static char *ruleptrs[MAXRULES];
static struct lsys_cmd *rules2[MAXRULES];
char maxangle;
static char loaded=0;


int _fastcall ispow2(int n)
{
  return (n == (n & -n));
}

LDBL _fastcall getnumber(char **str)
{
   char numstr[30];
   LDBL ret;
   int i,root,inverse;

   root=0;
   inverse=0;
   strcpy(numstr,"");
   (*str)++;
   switch (**str)
   {
   case 'q':
      root=1;
      (*str)++;
      break;
   case 'i':
      inverse=1;
      (*str)++;
      break;
   }
   switch (**str)
   {
   case 'q':
      root=1;
      (*str)++;
      break;
   case 'i':
      inverse=1;
      (*str)++;
      break;
   }
   i=0;
   while ((**str<='9' && **str>='0') || **str=='.')
   {
      numstr[i++]= **str;
      (*str)++;
   }
   (*str)--;
   numstr[i]=0;
   ret=atof(numstr);
   if (ret <= 0.0) /* this is a sanity check, JCO 8/31/94 */
      return 0;
   if (root)
     ret=sqrtl(ret);
   if (inverse)
     ret = 1.0/ret;
   return ret;
}

static int _fastcall readLSystemFile(char *str)
{
   int c;
   char **rulind;
   int err=0;
   int linenum,check=0;
   char inline1[MAX_LSYS_LINE_LEN+1],fixed[MAX_LSYS_LINE_LEN+1],*word;
   FILE *infile;
   char msgbuf[481]; /* enough for 6 full lines */

   if (find_file_item(LFileName,str,&infile, 2) < 0)
      return -1;
   while ((c = fgetc(infile)) != '{')
      if (c == EOF) return -1;
   maxangle=0;
   for(linenum=0;linenum<MAXRULES;++linenum) ruleptrs[linenum]=NULL;
   rulind= &ruleptrs[1];
   msgbuf[0]=(char)(linenum=0);

   while(file_gets(inline1,MAX_LSYS_LINE_LEN,infile) > -1)  /* Max line length chars */
   {
      linenum++;
      if ((word = strchr(inline1,';')) != NULL) /* strip comment */
         *word = 0;
      strlwr(inline1);

      if ((int)strspn(inline1," \t\n") < (int)strlen(inline1)) /* not a blank line */
      {
         word=strtok(inline1," =\t\n");
         if (!strcmp(word,"axiom"))
         {
            if (save_rule(strtok(NULL," \t\n"),&ruleptrs[0])) {
                strcat(msgbuf,"Error:  out of memory\n");
                ++err;
                break;
            }
            check=1;
         }
         else if (!strcmp(word,"angle"))
         {
            maxangle=(char)atoi(strtok(NULL," \t\n"));
            check=1;
         }
         else if (!strcmp(word,"}"))
            break;
         else if (!word[1])
         {
            char *temp;
            int index, memerr = 0;

            if (strchr("+-/\\@|!c<>][", *word))
            {
               sprintf(&msgbuf[strlen(msgbuf)],
               "Syntax error line %d: Redefined reserved symbol %s\n",linenum,word);
               ++err;
               break;
            }
            temp = strtok(NULL," =\t\n");
            index = rule_present(*word);

            if (!index)
            {
               strcpy(fixed,word);
               if (temp)
                  strcat(fixed,temp);
               memerr = save_rule(fixed,rulind++);
            }
            else if (temp)
            {
               strcpy(fixed,temp);
               memerr = append_rule(fixed,index);
            }
            if (memerr) {
                strcat(msgbuf, "Error:  out of memory\n");
                ++err;
                break;
            }
            check=1;
         }
         else
            if (err<6)
            {
               sprintf(&msgbuf[strlen(msgbuf)],
                       "Syntax error line %d: %s\n",linenum,word);
               ++err;
            }
         if (check)
         {
            check=0;
            if((word=strtok(NULL," \t\n"))!=NULL)
               if (err<6)
               {
                  sprintf(&msgbuf[strlen(msgbuf)],
                         "Extra text after command line %d: %s\n",linenum,word);
                  ++err;
               }
         }
      }
   }
   fclose(infile);
   if (!ruleptrs[0] && err<6)
   {
      strcat(msgbuf,"Error:  no axiom\n");
      ++err;
   }
   if ((maxangle<3||maxangle>50) && err<6)
   {
      strcat(msgbuf,"Error:  illegal or missing angle\n");
      ++err;
   }
   if (err)
   {
      msgbuf[strlen(msgbuf)-1]=0; /* strip trailing \n */
      stopmsg(0,msgbuf);
      return -1;
   }
   *rulind=NULL;
   return 0;
}

int Lsystem(void)
{
   int order;
   char **rulesc;
   struct lsys_cmd **sc;
   int stackoflow = 0;

   if ( (!loaded) && LLoad())
     return -1;

   overflow = 0;                /* reset integer math overflow flag */

   order=(int)param[0];
   if (order<=0)
     order=0;
   if (usr_floatflag)
        overflow = 1;
   else {
        struct lsys_turtlestatei ts;

        ts.stackoflow = 0;
        ts.maxangle = maxangle;
        ts.dmaxangle = (char)(maxangle - 1);

        sc = rules2;
        for (rulesc = ruleptrs; *rulesc; rulesc++)
            *sc++ = LSysISizeTransform(*rulesc, &ts);
        *sc = NULL;

        lsysi_dosincos();
        if (lsysi_findscale(rules2[0], &ts, &rules2[1], order)) {
            ts.realangle = ts.angle = ts.reverse = 0;

            free_lcmds();
            sc = rules2;
            for (rulesc = ruleptrs; *rulesc; rulesc++)
                *sc++ = LSysIDrawTransform(*rulesc, &ts);
            *sc = NULL;

            /* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
            if ((ts.curcolor=15) > colors)
                ts.curcolor=(char)(colors-1);
            drawLSysI(rules2[0], &ts, &rules2[1], order);
        }
        stackoflow = ts.stackoflow;
   }

   if (stackoflow) {
      stopmsg(0, "insufficient memory, try a lower order");
   }
   else if (overflow) {
        struct lsys_turtlestatef ts;

        overflow = 0;

        ts.stackoflow = 0;
        ts.maxangle = maxangle;
        ts.dmaxangle = (char)(maxangle - 1);

        sc = rules2;
        for (rulesc = ruleptrs; *rulesc; rulesc++)
            *sc++ = LSysFSizeTransform(*rulesc, &ts);
        *sc = NULL;

        lsysf_dosincos();
        if (lsysf_findscale(rules2[0], &ts, &rules2[1], order)) {
            ts.realangle = ts.angle = ts.reverse = 0;

            free_lcmds();
            sc = rules2;
            for (rulesc = ruleptrs; *rulesc; rulesc++)
                *sc++ = LSysFDrawTransform(*rulesc, &ts);
            *sc = NULL;

            /* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
            if ((ts.curcolor=15) > colors)
                ts.curcolor=(char)(colors-1);
            lsys_prepfpu(&ts);
            drawLSysF(rules2[0], &ts, &rules2[1], order);
            lsys_donefpu(&ts);
        }
        overflow = 0;
   }
   free_rules_mem();
   free_lcmds();
   loaded=0;
   return 0;
}

int LLoad(void)
{
   if (readLSystemFile(LName)) { /* error occurred */
      free_rules_mem();
      loaded=0;
      return -1;
   }
   loaded=1;
   return 0;
}

static void _fastcall free_rules_mem(void)
{
   int i;
   for(i=0;i<MAXRULES;++i)
      if(ruleptrs[i]) free(ruleptrs[i]);
}

static int _fastcall rule_present(char symbol)
{
   int i;

   for (i = 1; i < MAXRULES && ruleptrs[i] && *ruleptrs[i] != symbol ; i++)
      ;
   return (i < MAXRULES && ruleptrs[i]) ? i : 0;
}

static int _fastcall save_rule(char *rule,char **saveptr)
{
   int i;
   char *tmpfar;
   i=(int) strlen(rule)+1;
   if((tmpfar=(char *)malloc((long)i))==NULL) {
       return -1;
   }
   *saveptr=tmpfar;
   while(--i>=0) *(tmpfar++)= *(rule++);
   return 0;
}

static int _fastcall append_rule(char *rule, int index)
{
   char *dst, *old, *sav;
   int i, j;

   old = sav = ruleptrs[index];
   for (i = 0; *(old++); i++)
      ;
   j = (int) strlen(rule) + 1;
   if ((dst = (char *)malloc((long)(i + j))) == NULL)
      return -1;

   old = sav;
   ruleptrs[index] = dst;
   while (i-- > 0) *(dst++) = *(old++);
   while (j-- > 0) *(dst++) = *(rule++);
   free(sav);
   return 0;
}

static void free_lcmds(void)
{
  struct lsys_cmd **sc = rules2;

  while (*sc)
    free(*sc++);
}

#if defined(XFRACT) || defined(_WIN32)
#define lsysi_doslash_386 lsysi_doslash
#define lsysi_dobslash_386 lsysi_dobslash
#define lsys_doat lsysi_doat
#define lsys_dosizegf lsysi_dosizegf
#define lsys_dodrawg lsysi_dodrawg
void lsys_prepfpu(struct lsys_turtlestatef *x) { }
void lsys_donefpu(struct lsys_turtlestatef *x) { }
#endif

/* integer specific routines */

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_doplus(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse) {
        if (++cmd->angle == cmd->maxangle)
            cmd->angle = 0;
    }
    else {
        if (cmd->angle)
            cmd->angle--;
        else
            cmd->angle = cmd->dmaxangle;
    }
}
#else
extern void lsysi_doplus(struct lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
/* This is the same as lsys_doplus, except maxangle is a power of 2. */
static void lsysi_doplus_pow2(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse) {
        cmd->angle++;
        cmd->angle &= cmd->dmaxangle;
    }
    else {
        cmd->angle--;
        cmd->angle &= cmd->dmaxangle;
    }
}
#else
extern void lsysi_doplus_pow2(struct lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dominus(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse) {
        if (cmd->angle)
            cmd->angle--;
        else
            cmd->angle = cmd->dmaxangle;
    }
    else {
        if (++cmd->angle == cmd->maxangle)
            cmd->angle = 0;
    }
}
#else
extern void lsysi_dominus(struct lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dominus_pow2(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse) {
        cmd->angle--;
        cmd->angle &= cmd->dmaxangle;
    }
    else {
        cmd->angle++;
        cmd->angle &= cmd->dmaxangle;
    }
}
#else
extern void lsysi_dominus_pow2(struct lsys_turtlestatei *cmd);
#endif

static void lsysi_doslash(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse)
        cmd->realangle -= cmd->num;
    else
        cmd->realangle += cmd->num;
}

#if !defined(XFRACT) && !defined(_WIN32)
extern void lsysi_doslash_386(struct lsys_turtlestatei *cmd);
#endif

static void lsysi_dobslash(struct lsys_turtlestatei *cmd)
{
    if (cmd->reverse)
        cmd->realangle += cmd->num;
    else
        cmd->realangle -= cmd->num;
}

#if !defined(XFRACT) && !defined(_WIN32)
extern void lsysi_dobslash_386(struct lsys_turtlestatei *cmd);
#endif

static void lsysi_doat(struct lsys_turtlestatei *cmd)
{
    cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_dopipe(struct lsys_turtlestatei *cmd)
{
    cmd->angle = (char)(cmd->angle + (char)(cmd->maxangle / 2));
    cmd->angle %= cmd->maxangle;
}

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dopipe_pow2(struct lsys_turtlestatei *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}
#else
extern void lsysi_dopipe_pow2(struct lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dobang(struct lsys_turtlestatei *cmd)
{
    cmd->reverse = ! cmd->reverse;
}
#else
extern void lsysi_dobang(struct lsys_turtlestatei *cmd);
#endif

static void lsysi_dosizedm(struct lsys_turtlestatei *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s, c;
    long fixedsin, fixedcos;

    FPUsincos(&angle, &s, &c);
    fixedsin = (long) (s * FIXEDLT1);
    fixedcos = (long) (c * FIXEDLT1);

    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));

/* xpos+=size*aspect*cos(realangle*PI/180); */
/* ypos+=size*sin(realangle*PI/180); */
    if (cmd->xpos>cmd->xmax) cmd->xmax=cmd->xpos;
    if (cmd->ypos>cmd->ymax) cmd->ymax=cmd->ypos;
    if (cmd->xpos<cmd->xmin) cmd->xmin=cmd->xpos;
    if (cmd->ypos<cmd->ymin) cmd->ymin=cmd->ypos;
}

static void lsysi_dosizegf(struct lsys_turtlestatei *cmd)
{
  cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
  cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
/* xpos+=size*coss[angle]; */
/* ypos+=size*sins[angle]; */
  if (cmd->xpos>cmd->xmax) cmd->xmax=cmd->xpos;
  if (cmd->ypos>cmd->ymax) cmd->ymax=cmd->ypos;
  if (cmd->xpos<cmd->xmin) cmd->xmin=cmd->xpos;
  if (cmd->ypos<cmd->ymin) cmd->ymin=cmd->ypos;
}

static void lsysi_dodrawd(struct lsys_turtlestatei *cmd)
{
  double angle = (double) cmd->realangle * ANGLE2DOUBLE;
  double s, c;
  long fixedsin, fixedcos;
  int lastx, lasty;

  FPUsincos(&angle, &s, &c);
  fixedsin = (long) (s * FIXEDLT1);
  fixedcos = (long) (c * FIXEDLT1);

  lastx=(int) (cmd->xpos >> 19);
  lasty=(int) (cmd->ypos >> 19);
  cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
  cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
/* xpos+=size*aspect*cos(realangle*PI/180); */
/* ypos+=size*sin(realangle*PI/180); */
  driver_draw_line(lastx,lasty,(int)(cmd->xpos >> 19),(int)(cmd->ypos >> 19),cmd->curcolor);
}

static void lsysi_dodrawm(struct lsys_turtlestatei *cmd)
{
  double angle = (double) cmd->realangle * ANGLE2DOUBLE;
  double s, c;
  long fixedsin, fixedcos;

  FPUsincos(&angle, &s, &c);
  fixedsin = (long) (s * FIXEDLT1);
  fixedcos = (long) (c * FIXEDLT1);

/* xpos+=size*aspect*cos(realangle*PI/180); */
/* ypos+=size*sin(realangle*PI/180); */
  cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
  cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
}

static void lsysi_dodrawg(struct lsys_turtlestatei *cmd)
{
  cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
  cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
/* xpos+=size*coss[angle]; */
/* ypos+=size*sins[angle]; */
}

static void lsysi_dodrawf(struct lsys_turtlestatei *cmd)
{
  int lastx = (int) (cmd->xpos >> 19);
  int lasty = (int) (cmd->ypos >> 19);
  cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
  cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
/* xpos+=size*coss[angle]; */
/* ypos+=size*sins[angle]; */
  driver_draw_line(lastx,lasty,(int)(cmd->xpos >> 19),(int)(cmd->ypos >> 19),cmd->curcolor);
}

static void lsysi_dodrawc(struct lsys_turtlestatei *cmd)
{
  cmd->curcolor = (char)(((int) cmd->num) % colors);
}

static void lsysi_dodrawgt(struct lsys_turtlestatei *cmd)
{
  cmd->curcolor = (char)(cmd->curcolor - (char)cmd->num);
  if ((cmd->curcolor %= colors) == 0)
    cmd->curcolor = (char)(colors-1);
}

static void lsysi_dodrawlt(struct lsys_turtlestatei *cmd)
{
  cmd->curcolor = (char)(cmd->curcolor + (char)cmd->num);
  if ((cmd->curcolor %= colors) == 0)
    cmd->curcolor = 1;
}

static struct lsys_cmd * _fastcall
findsize(struct lsys_cmd *command, struct lsys_turtlestatei *ts, struct lsys_cmd **rules, int depth)
{
   struct lsys_cmd **rulind;
   int tran;

if (overflow)     /* integer math routines overflowed */
    return NULL;

   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      ts->stackoflow = 1;
      return NULL;
   }

   while (command->ch && command->ch !=']') {
      if (! (ts->counter++)) {
         /* let user know we're not dead */
         if (thinking(1, "L-System thinking (higher orders take longer)")) {
            ts->counter--;
            return NULL;
         }
      }
      tran=0;
      if (depth) {
         for(rulind=rules;*rulind;rulind++)
            if ((*rulind)->ch==command->ch) {
               tran=1;
               if (findsize((*rulind)+1,ts,rules,depth-1) == NULL)
                  return(NULL);
            }
      }
      if (!depth || !tran) {
        if (command->f) {
          ts->num = command->n;
          (*command->f)(ts);
          }
        else if (command->ch == '[') {
          char saveang,saverev;
          long savesize,savex,savey;
          unsigned long saverang;

          saveang=ts->angle;
          saverev=ts->reverse;
          savesize=ts->size;
          saverang=ts->realangle;
          savex=ts->xpos;
          savey=ts->ypos;
          if ((command=findsize(command+1,ts,rules,depth)) == NULL)
             return(NULL);
          ts->angle=saveang;
          ts->reverse=saverev;
          ts->size=savesize;
          ts->realangle=saverang;
          ts->xpos=savex;
          ts->ypos=savey;
        }
      }
      command++;
   }
   return command;
}

static int
lsysi_findscale(struct lsys_cmd *command, struct lsys_turtlestatei *ts, struct lsys_cmd **rules, int depth)
{
   float horiz,vert;
   double xmin, xmax, ymin, ymax;
   double locsize;
   double locaspect;
   struct lsys_cmd *fsret;

   locaspect=screenaspect*xdots/ydots;
   ts->aspect = FIXEDPT(locaspect);
   ts->xpos =
   ts->ypos =
   ts->xmin =
   ts->xmax =
   ts->ymax =
   ts->ymin =
   ts->realangle =
   ts->angle =
   ts->reverse =
   ts->counter = 0;
   ts->size=FIXEDPT(1L);
   fsret = findsize(command,ts,rules,depth);
   thinking(0, NULL); /* erase thinking message if any */
   xmin = (double) ts->xmin / FIXEDMUL;
   xmax = (double) ts->xmax / FIXEDMUL;
   ymin = (double) ts->ymin / FIXEDMUL;
   ymax = (double) ts->ymax / FIXEDMUL;
   if (fsret == NULL)
      return 0;
   if (xmax == xmin)
      horiz = (float)1E37;
   else
      horiz = (float)((xdots-10)/(xmax-xmin));
   if (ymax == ymin)
      vert = (float)1E37;
   else
      vert = (float)((ydots-6) /(ymax-ymin));
   locsize = (vert<horiz) ? vert : horiz;

   if (horiz == 1E37)
      ts->xpos = FIXEDPT(xdots/2);
   else
/*    ts->xpos = FIXEDPT(-xmin*(locsize)+5+((xdots-10)-(locsize)*(xmax-xmin))/2); */
      ts->xpos = FIXEDPT((xdots-locsize*(xmax+xmin))/2);
   if (vert == 1E37)
      ts->ypos = FIXEDPT(ydots/2);
   else
/*    ts->ypos = FIXEDPT(-ymin*(locsize)+3+((ydots-6)-(locsize)*(ymax-ymin))/2); */
      ts->ypos = FIXEDPT((ydots-locsize*(ymax+ymin))/2);
   ts->size = FIXEDPT(locsize);

   return 1;
}

static struct lsys_cmd *
drawLSysI(struct lsys_cmd *command,struct lsys_turtlestatei *ts, struct lsys_cmd **rules,int depth)
{
   struct lsys_cmd **rulind;
   int tran;

   if (overflow)     /* integer math routines overflowed */
      return NULL;

   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      ts->stackoflow = 1;
      return NULL;
   }


   while (command->ch && command->ch !=']') {
      if (!(ts->counter++)) {
         if (driver_key_pressed()) {
            ts->counter--;
            return NULL;
         }
      }
      tran=0;
      if (depth) {
         for(rulind=rules;*rulind;rulind++)
            if ((*rulind)->ch == command->ch) {
               tran=1;
               if (drawLSysI((*rulind)+1,ts,rules,depth-1) == NULL)
                  return NULL;
            }
      }
      if (!depth||!tran) {
        if (command->f) {
          ts->num = command->n;
          (*command->f)(ts);
          }
        else if (command->ch == '[') {
          char saveang,saverev,savecolor;
          long savesize,savex,savey;
          unsigned long saverang;

          saveang=ts->angle;
          saverev=ts->reverse;
          savesize=ts->size;
          saverang=ts->realangle;
          savex=ts->xpos;
          savey=ts->ypos;
          savecolor=ts->curcolor;
          if ((command=drawLSysI(command+1,ts,rules,depth)) == NULL)
             return(NULL);
          ts->angle=saveang;
          ts->reverse=saverev;
          ts->size=savesize;
          ts->realangle=saverang;
          ts->xpos=savex;
          ts->ypos=savey;
          ts->curcolor=savecolor;
        }
      }
      command++;
   }
   return command;
}

static struct lsys_cmd *
LSysISizeTransform(char *s, struct lsys_turtlestatei *ts)
{
  struct lsys_cmd *ret;
  struct lsys_cmd *doub;
  int maxval = 10;
  int n = 0;
  void (*f)();
  long num;

  void (*plus)() = (ispow2(ts->maxangle)) ? lsysi_doplus_pow2 : lsysi_doplus;
  void (*minus)() = (ispow2(ts->maxangle)) ? lsysi_dominus_pow2 : lsysi_dominus;
  void (*pipe)() = (ispow2(ts->maxangle)) ? lsysi_dopipe_pow2 : lsysi_dopipe;

  void (*slash)() =  (cpu >= 386) ? lsysi_doslash_386 : lsysi_doslash;
  void (*bslash)() = (cpu >= 386) ? lsysi_dobslash_386 : lsysi_dobslash;
  void (*at)() =     (cpu >= 386) ? lsysi_doat_386 : lsysi_doat;
  void (*dogf)() =   (cpu >= 386) ? lsysi_dosizegf_386 : lsysi_dosizegf;

  ret = (struct lsys_cmd *) malloc((long) maxval * sizeof(struct lsys_cmd));
  if (ret == NULL) {
       ts->stackoflow = 1;
       return NULL;
       }
  while (*s) {
    f = NULL;
    num = 0;
    ret[n].ch = *s;
    switch (*s) {
      case '+': f = plus;            break;
      case '-': f = minus;           break;
      case '/': f = slash;           num = (long) (getnumber(&s) * 11930465L);    break;
      case '\\': f = bslash;         num = (long) (getnumber(&s) * 11930465L);    break;
      case '@': f = at;              num = FIXEDPT(getnumber(&s));    break;
      case '|': f = pipe;            break;
      case '!': f = lsysi_dobang;     break;
      case 'd':
      case 'm': f = lsysi_dosizedm;   break;
      case 'g':
      case 'f': f = dogf;       break;
      case '[': num = 1;        break;
      case ']': num = 2;        break;
      default:
        num = 3;
        break;
    }
#if defined(XFRACT)
    ret[n].f = (void (*)())f;
#else
    ret[n].f = (void (*)(struct lsys_turtlestatei *))f;
#endif
    ret[n].n = num;
    if (++n == maxval) {
      doub = (struct lsys_cmd *) malloc((long) maxval*2*sizeof(struct lsys_cmd));
      if (doub == NULL) {
         free(ret);
         ts->stackoflow = 1;
         return NULL;
         }
      memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
      free(ret);
      ret = doub;
      maxval <<= 1;
    }
    s++;
  }
  ret[n].ch = 0;
  ret[n].f = NULL;
  ret[n].n = 0;
  n++;

  doub = (struct lsys_cmd *) malloc((long) n*sizeof(struct lsys_cmd));
  if (doub == NULL) {
       free(ret);
       ts->stackoflow = 1;
       return NULL;
       }
  memcpy(doub, ret, n*sizeof(struct lsys_cmd));
  free(ret);
  return doub;
}

static struct lsys_cmd *
LSysIDrawTransform(char *s, struct lsys_turtlestatei *ts)
{
  struct lsys_cmd *ret;
  struct lsys_cmd *doub;
  int maxval = 10;
  int n = 0;
  void (*f)();
  long num;

  void (*plus)() = (ispow2(ts->maxangle)) ? lsysi_doplus_pow2 : lsysi_doplus;
  void (*minus)() = (ispow2(ts->maxangle)) ? lsysi_dominus_pow2 : lsysi_dominus;
  void (*pipe)() = (ispow2(ts->maxangle)) ? lsysi_dopipe_pow2 : lsysi_dopipe;

  void (*slash)() =  (cpu >= 386) ? lsysi_doslash_386 : lsysi_doslash;
  void (*bslash)() = (cpu >= 386) ? lsysi_dobslash_386 : lsysi_dobslash;
  void (*at)() =     (cpu >= 386) ? lsysi_doat_386 : lsysi_doat;
  void (*drawg)() =  (cpu >= 386) ? lsysi_dodrawg_386 : lsysi_dodrawg;

  ret = (struct lsys_cmd *) malloc((long) maxval * sizeof(struct lsys_cmd));
  if (ret == NULL) {
       ts->stackoflow = 1;
       return NULL;
       }
  while (*s) {
    f = NULL;
    num = 0;
    ret[n].ch = *s;
    switch (*s) {
      case '+': f = plus;            break;
      case '-': f = minus;           break;
      case '/': f = slash;           num = (long) (getnumber(&s) * 11930465L);    break;
      case '\\': f = bslash;         num = (long) (getnumber(&s) * 11930465L);    break;
      case '@': f = at;              num = FIXEDPT(getnumber(&s));    break;
      case '|': f = pipe;            break;
      case '!': f = lsysi_dobang;     break;
      case 'd': f = lsysi_dodrawd;    break;
      case 'm': f = lsysi_dodrawm;    break;
      case 'g': f = drawg;           break;
      case 'f': f = lsysi_dodrawf;    break;
      case 'c': f = lsysi_dodrawc;    num = (long) getnumber(&s);    break;
      case '<': f = lsysi_dodrawlt;   num = (long) getnumber(&s);    break;
      case '>': f = lsysi_dodrawgt;   num = (long) getnumber(&s);    break;
      case '[': num = 1;        break;
      case ']': num = 2;        break;
      default:
        num = 3;
        break;
    }
#ifdef XFRACT
    ret[n].f = (void (*)())f;
#else
    ret[n].f = (void (*)(struct lsys_turtlestatei *))f;
#endif
    ret[n].n = num;
    if (++n == maxval) {
      doub = (struct lsys_cmd *) malloc((long) maxval*2*sizeof(struct lsys_cmd));
      if (doub == NULL) {
           free(ret);
           ts->stackoflow = 1;
           return NULL;
           }
      memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
      free(ret);
      ret = doub;
      maxval <<= 1;
    }
    s++;
  }
  ret[n].ch = 0;
  ret[n].f = NULL;
  ret[n].n = 0;
  n++;

  doub = (struct lsys_cmd *) malloc((long) n*sizeof(struct lsys_cmd));
  if (doub == NULL) {
       free(ret);
       ts->stackoflow = 1;
       return NULL;
       }
  memcpy(doub, ret, n*sizeof(struct lsys_cmd));
  free(ret);
  return doub;
}

static void _fastcall lsysi_dosincos(void)
{
   double locaspect;
   double TWOPI = 2.0 * PI;
   double twopimax;
   double twopimaxi;
   double s, c;
   int i;

   locaspect=screenaspect*xdots/ydots;
   twopimax = TWOPI / maxangle;
   for(i=0;i<maxangle;i++) {
      twopimaxi = i * twopimax;
      FPUsincos(&twopimaxi, &s, &c);
      sins[i] = (long) (s * FIXEDLT1);
      coss[i] = (long) ((locaspect * c) * FIXEDLT1);
   }
}
