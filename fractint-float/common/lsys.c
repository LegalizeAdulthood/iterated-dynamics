
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

struct lsys_cmd {
    void (*f)(struct lsys_turtlestatei *);
    long n;
    char ch;
};

static int _fastcall readLSystemFile(char *);
static void _fastcall free_rules_mem(void);
static int _fastcall rule_present(char symbol);
static int _fastcall save_rule(char *,char far **);
static int _fastcall append_rule(char *rule, int index);
static void free_lcmds(void);
static struct lsys_cmd far * _fastcall findsize(struct lsys_cmd far *,struct lsys_turtlestatei *, struct lsys_cmd far **,int);

/* Some notes to Adrian from PB, made when I integrated with v15:
     printfs changed to work with new user interface
     bug at end of readLSystemFile, the line which said rulind=0 fixed
       to say *rulind=0
     the calloc was not worthwhile, it was just for a 54 byte area, cheaper
       to keep it as a static;  but there was a static 201 char buffer I
       changed to not be static
     use of strdup was a nono, caused problems running out of space cause
       the memory allocated each time was never freed; I've changed to
       use far memory and to free when done
   */

#define sins ((long *)(boxy))
#define coss (((long *)(boxy)+50)) /* 50 after the start of sins */
static char far *ruleptrs[MAXRULES];
static struct lsys_cmd far *rules2[MAXRULES];
char maxangle;
static char loaded=0;


int _fastcall ispow2(int n)
{
  return (n == (n & -n));
}

LDBL _fastcall getnumber(char far **str)
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
   char far **rulind;
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
      static FCODE out_of_mem[] = {"Error:  out of memory\n"};
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
                far_strcat(msgbuf,out_of_mem);
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
                far_strcat(msgbuf, out_of_mem);
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
      static FCODE no_axiom[] = {"Error:  no axiom\n"};
      far_strcat(msgbuf,no_axiom);
      ++err;
   }
   if ((maxangle<3||maxangle>50) && err<6)
   {
      static FCODE missing_angle[] = {"Error:  illegal or missing angle\n"};
      far_strcat(msgbuf,missing_angle);
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
   char far **rulesc;
   struct lsys_cmd far **sc;

   if ( (!loaded) && LLoad())
     return -1;

   overflow = 0;                /* reset integer math overflow flag */

   order=(int)param[0];
   if (order<=0)
     order=0;
   overflow = 1;

   if (overflow) {
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
      if(ruleptrs[i]) farmemfree(ruleptrs[i]);
}

static int _fastcall rule_present(char symbol)
{
   int i;

   for (i = 1; i < MAXRULES && ruleptrs[i] && *ruleptrs[i] != symbol ; i++)
      ;
   return (i < MAXRULES && ruleptrs[i]) ? i : 0;
}

static int _fastcall save_rule(char *rule,char far **saveptr)
{
   int i;
   char far *tmpfar;
   i=strlen(rule)+1;
   if((tmpfar=farmemalloc((long)i))==NULL) {
       return -1;
   }
   *saveptr=tmpfar;
   while(--i>=0) *(tmpfar++)= *(rule++);
   return 0;
}

static int _fastcall append_rule(char *rule, int index)
{
   char far *dst, far *old, far *sav;
   int i, j;

   old = sav = ruleptrs[index];
   for (i = 0; *(old++); i++)
      ;
   j = strlen(rule) + 1;
   if ((dst = farmemalloc((long)(i + j))) == NULL)
      return -1;

   old = sav;
   ruleptrs[index] = dst;
   while (i-- > 0) *(dst++) = *(old++);
   while (j-- > 0) *(dst++) = *(rule++);
   farmemfree(sav);
   return 0;
}

static void free_lcmds(void)
{
  struct lsys_cmd far **sc = rules2;

  while (*sc)
    farmemfree(*sc++);
}

#ifdef XFRACT
void lsys_prepfpu(struct lsys_turtlestatef *x) { }
void lsys_donefpu(struct lsys_turtlestatef *x) { }
#endif

static struct lsys_cmd far * _fastcall
findsize(struct lsys_cmd far *command, struct lsys_turtlestatei *ts, struct lsys_cmd far **rules, int depth)
{
   struct lsys_cmd far **rulind;
   int tran;

if (overflow)     /* integer math routines overflowed */
    return NULL;

   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      ts->stackoflow = 1;
      return NULL;
   }

   while (command->ch && command->ch !=']') {
      static FCODE thinking_msg[] =
         {"L-System thinking (higher orders take longer)"};
      if (! (ts->counter++)) {
         /* let user know we're not dead */
         if (thinking(1,thinking_msg)) {
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
