/*
		Overlayed odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef XFRACT
#if !defined(_WIN32)
#include <malloc.h>
#endif
#include <process.h>
#include <io.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

/* routines in this module      */

void write_batch_parms(char *colorinf, int colorsonly, int maxcolor, int i, int j);
void expand_comments(char *target, char *source);

#ifndef USE_VARARGS
static void put_parm(char *parm, ...);
#else
static void put_parm();
#endif

static void put_parm_line(void);
static int getprec(double, double, double);
int getprecbf(int);
static void put_float(int, double, int);
static void put_bf(int slash, bf_t r, int prec);
static void put_filename(char *keyword, char *fname);
#ifndef XFRACT
static int check_modekey(int curkey, int choice);
#endif
static int entcompare(VOIDCONSTPTR p1, VOIDCONSTPTR p2);
static void update_fractint_cfg(void);
static void strip_zeros(char *buf);

char par_comment[4][MAXCMT];

/* JIIM */

FILE *parmfile;

#define PAR_KEY(x)  (x < 10 ? '0' + x : 'a' - 10 + x)

#ifdef _MSC_VER
#pragma optimize("e", off)  /* MSC 6.00A messes up next rtn with "e" on */
#endif

void make_batch_file()
{
#define MAXPROMPTS 18
	int colorsonly = 0;
	/** added for pieces feature **/
	double pdelx = 0.0;
	double pdely = 0.0;
	double pdelx2 = 0.0;
	double pdely2 = 0.0;
	unsigned int pxdots, pydots, xm, ym;
	double pxxmin = 0.0, pyymax = 0.0;
	char vidmde[5];
	int promptnum;
	int piecespromts;
	int have3rd = 0;
	/****/

	int i, j;
	char inpcommandfile[80], inpcommandname[ITEMNAMELEN + 1];
	char inpcomment[4][MAXCMT];
	struct fullscreenvalues paramvalues[18];
	char *choices[MAXPROMPTS];
	int gotinfile;
	char outname[FILE_MAX_PATH + 1], buf[256], buf2[128];
	FILE *infile = NULL;
	FILE *fpbat = NULL;
	char colorspec[14];
	int maxcolor;
	int maxcolorindex = 0;
	char *sptr = NULL, *sptr2;
	int oldhelpmode;

	if (s_makepar[1] == 0) /* makepar map case */
		colorsonly = 1;

	driver_stack_screen();
	oldhelpmode = helpmode;
	helpmode = HELPPARMFILE;

	maxcolor = colors;
	strcpy(colorspec, "y");
#ifndef XFRACT
	if ((g_got_real_dac) || (g_is_true_color && !truemode))
#else
	if ((g_got_real_dac) || (g_is_true_color && !truemode) || fake_lut)
#endif
	{
		--maxcolor;
/*    if (maxit < maxcolor)  remove 2 lines */
/*       maxcolor = maxit;   so that whole palette is always saved */
		if (inside > 0 && inside > maxcolor)
			maxcolor = inside;
		if (outside > 0 && outside > maxcolor)
			maxcolor = outside;
		if (distest < 0 && -distest > maxcolor)
			maxcolor = (int) -distest;
		if (decomp[0] > maxcolor)
			maxcolor = decomp[0] - 1;
		if (potflag && potparam[0] >= maxcolor)
			maxcolor = (int)potparam[0];
		if (++maxcolor > 256)
			maxcolor = 256;
		if (colorstate == 0)
		{                         /* default colors */
			if (mapdacbox)
			{
				colorspec[0] = '@';
				sptr = MAP_name;
			}
		}
		else if (colorstate == 2)
		{                         /* colors match colorfile */
			colorspec[0] = '@';
			sptr = colorfile;
		}
		else                      /* colors match no .map that we know of */
			strcpy (colorspec, "y");

		if (colorspec[0] == '@')
		{
			sptr2 = strrchr(sptr, SLASHC);
			if (sptr2 != NULL)
				sptr = sptr2 + 1;
			sptr2 = strrchr(sptr, ':');
			if (sptr2 != NULL)
				sptr = sptr2 + 1;
			strncpy(&colorspec[1], sptr, 12);
			colorspec[13] = 0;
		}
	}
	strcpy(inpcommandfile, CommandFile);
	strcpy(inpcommandname, CommandName);
	for (i = 0; i < 4; i++)
	{
		expand_comments(CommandComment[i], par_comment[i]);
		strcpy(inpcomment[i], CommandComment[i]);
	}

	if (CommandName[0] == 0)
		strcpy(inpcommandname, "test");
	/* TW added these  - and Bert moved them */
	pxdots = xdots;
	pydots = ydots;
	xm = ym = 1;
	if (*s_makepar == 0)
		goto skip_UI;

	vidmode_keyname(g_video_entry.keynum, vidmde);
	while (1)
	{
prompt_user:
		promptnum = 0;
		choices[promptnum] = "Parameter file";
		paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
		paramvalues[promptnum++].uval.sbuf = inpcommandfile;
		choices[promptnum] = "Name";
		paramvalues[promptnum].type = 0x100 + ITEMNAMELEN;
		paramvalues[promptnum++].uval.sbuf = inpcommandname;
		choices[promptnum] = "Main comment";
		paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
		paramvalues[promptnum++].uval.sbuf = inpcomment[0];
		choices[promptnum] = "Second comment";
		paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
		paramvalues[promptnum++].uval.sbuf = inpcomment[1];
		choices[promptnum] = "Third comment";
		paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
		paramvalues[promptnum++].uval.sbuf = inpcomment[2];
		choices[promptnum] = "Fourth comment";
		paramvalues[promptnum].type = 0x100 + MAXCMT - 1;
		paramvalues[promptnum++].uval.sbuf = inpcomment[3];
#ifndef XFRACT
		if (g_got_real_dac || (g_is_true_color && !truemode))
#else
		if (g_got_real_dac || (g_is_true_color && !truemode) || fake_lut)
#endif
		{
			choices[promptnum] = "Record colors?";
			paramvalues[promptnum].type = 0x100 + 13;
			paramvalues[promptnum++].uval.sbuf = colorspec;
			choices[promptnum] = "    (no | yes | only for full info | @filename to point to a map file)";
			paramvalues[promptnum++].type = '*';
			choices[promptnum] = "# of colors";
			maxcolorindex = promptnum;
			paramvalues[promptnum].type = 'i';
			paramvalues[promptnum++].uval.ival = maxcolor;
			choices[promptnum] = "    (if recording full color info)";
			paramvalues[promptnum++].type = '*';
		}
		choices[promptnum] = "Maximum line length";
		paramvalues[promptnum].type = 'i';
		paramvalues[promptnum++].uval.ival = maxlinelength;
		choices[promptnum] = "";
		paramvalues[promptnum++].type = '*';
		choices[promptnum] = "    **** The following is for generating images in pieces ****";
		paramvalues[promptnum++].type = '*';
		choices[promptnum] = "X Multiples";
		piecespromts = promptnum;
		paramvalues[promptnum].type = 'i';
		paramvalues[promptnum++].uval.ival = xm;
		choices[promptnum] = "Y Multiples";
		paramvalues[promptnum].type = 'i';
		paramvalues[promptnum++].uval.ival = ym;
#ifndef XFRACT
		choices[promptnum] = "Video mode";
		paramvalues[promptnum].type = 0x100 + 4;
		paramvalues[promptnum++].uval.sbuf = vidmde;
#endif

		if (fullscreen_prompt("Save Current Parameters", promptnum, choices, paramvalues, 0, NULL) < 0)
			break;

		if (*colorspec == 'o' || s_makepar[1] == 0)
		{
			strcpy(colorspec, "y");
			colorsonly = 1;
		}

		strcpy(CommandFile, inpcommandfile);
		if (has_ext(CommandFile) == NULL)
			strcat(CommandFile, ".par");   /* default extension .par */
		strcpy(CommandName, inpcommandname);
		for (i = 0; i < 4; i++)
			strncpy(CommandComment[i], inpcomment[i], MAXCMT);
#ifndef XFRACT
		if (g_got_real_dac || (g_is_true_color && !truemode))
#else
		if (g_got_real_dac || (g_is_true_color && !truemode) || fake_lut)
#endif
			if (paramvalues[maxcolorindex].uval.ival > 0 &&
					paramvalues[maxcolorindex].uval.ival <= 256)
				maxcolor = paramvalues[maxcolorindex].uval.ival;
		promptnum = piecespromts;
		{
			int newmaxlinelength;
			newmaxlinelength = paramvalues[promptnum-3].uval.ival;
			if (maxlinelength != newmaxlinelength &&
					newmaxlinelength >= MINMAXLINELENGTH &&
					newmaxlinelength <= MAXMAXLINELENGTH)
				maxlinelength = newmaxlinelength;
		}
		xm = paramvalues[promptnum++].uval.ival;

		ym = paramvalues[promptnum++].uval.ival;

		/* sanity checks */
		{
			long xtotal, ytotal;
#ifndef XFRACT
			int i;

			/* get resolution from the video name (which must be valid) */
			pxdots = pydots = 0;
			i = check_vidmode_keyname(vidmde);
			if (i > 0)
			{
				i = check_vidmode_key(0, i);
				if (i >= 0)
				{
					/* get the resolution of this video mode */
					pxdots = g_video_table[i].xdots;
					pydots = g_video_table[i].ydots;
				}
			}
			if (pxdots == 0 && (xm > 1 || ym > 1))
			{
				/* no corresponding video mode! */
				stopmsg(0, "Invalid video mode entry!");
				goto prompt_user;
			}
#endif

			/* bounds range on xm, ym */
			if (xm < 1 || xm > 36 || ym < 1 || ym > 36)
			{
				stopmsg(0, "X and Y components must be 1 to 36");
				goto prompt_user;
			}

			/* another sanity check: total resolution cannot exceed 65535 */
			xtotal = xm;  ytotal = ym;
			xtotal *= pxdots;  ytotal *= pydots;
			if (xtotal > 65535L || ytotal > 65535L)
			{
				stopmsg(0, "Total resolution (X or Y) cannot exceed 65535");
				goto prompt_user;
			}
		}
skip_UI:
		if (*s_makepar == 0)
		{
			if (filecolors > 0)
				strcpy(colorspec, "y");
			else
				strcpy(colorspec, "n");
			if (s_makepar[1] == 0)
				maxcolor = 256;
			else
				maxcolor = filecolors;
		}
		strcpy(outname, CommandFile);
		gotinfile = 0;
		if (access(CommandFile, 0) == 0)
		{                         /* file exists */
			gotinfile = 1;
			if (access(CommandFile, 6))
			{
				sprintf(buf, "Can't write %s", CommandFile);
				stopmsg(0, buf);
				continue;
			}
			i = (int) strlen(outname);
			while (--i >= 0 && outname[i] != SLASHC)
				outname[i] = 0;
			strcat(outname, "fractint.tmp");
			infile = fopen(CommandFile, "rt");
#ifndef XFRACT
			setvbuf(infile, tstack, _IOFBF, 4096); /* improves speed */
#endif
		}
		parmfile = fopen(outname, "wt");
		if (parmfile == NULL)
		{
			sprintf(buf, "Can't create %s", outname);
			stopmsg(0, buf);
			if (gotinfile)
				fclose(infile);
			continue;
		}

		if (gotinfile)
		{
			while (file_gets(buf, 255, infile) >= 0)
			{
				if (strchr(buf, '{')/* entry heading? */
					&& sscanf(buf, " %40[^ \t({]", buf2)
					&& stricmp(buf2, CommandName) == 0)
				{                   /* entry with same name */
					_snprintf(buf2, NUM_OF(buf2), "File already has an entry named %s\n%s",
						CommandName, (*s_makepar == 0) ?
						"... Replacing ..." : "Continue to replace it, Cancel to back out");
					if (stopmsg(STOPMSG_CANCEL | STOPMSG_INFO_ONLY, buf2) < 0)
					{                /* cancel */
						fclose(infile);
						fclose(parmfile);
						unlink(outname);
						goto prompt_user;
					}
					while (strchr(buf, '}') == NULL
							&& file_gets(buf, 255, infile) > 0)
					{
						/* skip to end of set */
					}
					break;
				}
				fputs(buf, parmfile);
				fputc('\n', parmfile);
			}
		}
/***** start here*/
		if (xm > 1 || ym > 1)
		{
			if (xxmin != xx3rd || yymin != yy3rd)
				have3rd = 1;
			else
				have3rd = 0;
			fpbat = dir_fopen(workdir, "makemig.bat", "w");
			if (fpbat == NULL)
				xm = ym = 0;
			pdelx  = (xxmax - xx3rd) / (xm*pxdots - 1);   /* calculate stepsizes */
			pdely  = (yymax - yy3rd) / (ym*pydots - 1);
			pdelx2 = (xx3rd - xxmin) / (ym*pydots - 1);
			pdely2 = (yy3rd - yymin) / (xm*pxdots - 1);

			/* save corners */
			pxxmin = xxmin;
			pyymax = yymax;
		}
		for (i = 0; i < (int)xm; i++)  /* columns */
		for (j = 0; j < (int)ym; j++)  /* rows    */
		{
			if (xm > 1 || ym > 1)
			{
				int w;
				char c;
				char PCommandName[80];
				w = 0;
				while (w < (int)strlen(CommandName))
				{
					c = CommandName[w];
					if (isspace(c) || c == 0)
						break;
					PCommandName[w] = c;
					w++;
				}
				PCommandName[w] = 0;
				{
					char buf[20];
					sprintf(buf, "_%c%c", PAR_KEY(i), PAR_KEY(j));
					strcat(PCommandName, buf);
				}
				fprintf(parmfile, "%-19s{", PCommandName);
				xxmin = pxxmin + pdelx*(i*pxdots) + pdelx2*(j*pydots);
				xxmax = pxxmin + pdelx*((i + 1)*pxdots - 1) + pdelx2*((j + 1)*pydots - 1);
				yymin = pyymax - pdely*((j + 1)*pydots - 1) - pdely2*((i + 1)*pxdots - 1);
				yymax = pyymax - pdely*(j*pydots) - pdely2*(i*pxdots);
				if (have3rd)
				{
					xx3rd = pxxmin + pdelx*(i*pxdots) + pdelx2*((j + 1)*pydots - 1);
					yy3rd = pyymax - pdely*((j + 1)*pydots - 1) - pdely2*(i*pxdots);
				}
				else
				{
					xx3rd = xxmin;
					yy3rd = yymin;
				}
				fprintf(fpbat, "Fractint batch=yes overwrite=yes @%s/%s\n", CommandFile, PCommandName);
				fprintf(fpbat, "If Errorlevel 2 goto oops\n");
			}
			else
				fprintf(parmfile, "%-19s{", CommandName);
			{
				/* guarantee that there are no blank comments above the last
					non-blank par_comment */
				int i, last;
				for (last=-1, i = 0; i < 4; i++)
					if (*par_comment[i])
						last=i;
				for (i = 0; i < last; i++)
					if (*CommandComment[i] == '\0')
						strcpy(CommandComment[i], ";");
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
				for (k = 1; k < 4; k++)
					if (CommandComment[k][0])
						fprintf(parmfile, "%s%s\n", buf, CommandComment[k]);
				if (g_patch_level != 0 && colorsonly == 0)
					fprintf(parmfile, "%s %s Version %d Patchlevel %d\n", buf,
						Fractint, g_release, g_patch_level);
			}
			write_batch_parms(colorspec, colorsonly, maxcolor, i, j);
			if (xm > 1 || ym > 1)
			{
				fprintf(parmfile, "  video=%s", vidmde);
				fprintf(parmfile, " savename=frmig_%c%c\n", PAR_KEY(i), PAR_KEY(j));
			}
			fprintf(parmfile, "  }\n\n");
		}
		if (xm > 1 || ym > 1)
			{
			fprintf(fpbat, "Fractint makemig=%d/%d\n", xm, ym);
			fprintf(fpbat, "Rem Simplgif fractmig.gif simplgif.gif  in case you need it\n");
			fprintf(fpbat, ":oops\n");
			fclose(fpbat);
			}
/*******end here */

		if (gotinfile)
		{                         /* copy the rest of the file */
			do
			{
				i = file_gets(buf, 255, infile);
			}
			while (i == 0); /* skip blanks */
			while (i >= 0)
			{
				fputs(buf, parmfile);
				fputc('\n', parmfile);
				i = file_gets(buf, 255, infile);
			}
			fclose(infile);
		}
		fclose(parmfile);
		if (gotinfile)
		{                         /* replace the original file with the new */
			unlink(CommandFile);   /* success assumed on these lines       */
			rename(outname, CommandFile);  /* since we checked earlier with access */
		}
		break;
	}
	helpmode = oldhelpmode;
	driver_unstack_screen();
}

#ifdef C6
#pragma optimize("e", on)  /* back to normal */
#endif

static struct write_batch_data /* buffer for parms to break lines nicely */
{
	int len;
	char buf[10000];
} s_wbdata;

void write_batch_parms(char *colorinf, int colorsonly, int maxcolor, int ii, int jj)
{
	int i, j, k;
	double Xctr, Yctr;
	LDBL Magnification;
	double Xmagfactor, Rotation, Skew;
	char *sptr;
	char buf[81];
	bf_t bfXctr=NULL, bfYctr=NULL;
	int saved;
	saved = save_stack();
	if (bf_math)
	{
		bfXctr = alloc_stack(bflength + 2);
		bfYctr = alloc_stack(bflength + 2);
	}

	s_wbdata.len = 0; /* force first parm to start on new line */

	/* Using near string boxx for buffer after saving to extraseg */

	if (colorsonly)
		goto docolors;
	if (display3d <= 0)  /* a fractal was generated */
	{
		/****** fractal only parameters in this section *******/
		put_parm(" reset=%d", check_back() ?
			min(save_release, g_release) : g_release);

		sptr = curfractalspecific->name;
		if (*sptr == '*') ++sptr;
		put_parm(" type=%s", sptr);

		if (fractype == JULIBROT || fractype == JULIBROTFP)
		{
			put_parm(" julibrotfromto=%.15g/%.15g/%.15g/%.15g",
				mxmaxfp, mxminfp, mymaxfp, myminfp);
			/* these rarely change */
			if (originfp != 8 || heightfp != 7 || widthfp != 10 || distfp != 24
					|| depthfp != 8 || zdots != 128)
				put_parm(" julibrot3d=%d/%g/%g/%g/%g/%g",
					zdots, originfp, depthfp, heightfp, widthfp, distfp);
			if (eyesfp != 0)
				put_parm(" julibroteyes=%g", eyesfp);
			if (neworbittype != JULIA)
			{
				char *name;
				name = fractalspecific[neworbittype].name;
				if (*name == '*')
					name++;
				put_parm(" orbitname=%s", name);
			}
			if (juli3Dmode != 0)
				put_parm(" 3dmode=%s", juli3Doptions[juli3Dmode]);
		}
		if (fractype == FORMULA || fractype == FFORMULA)
		{
			put_filename("formulafile", FormFileName);
			put_parm(" formulaname=%s", FormName);
			if (uses_ismand)
				put_parm(" ismand=%c", ismand?'y':'n');
		}
		if (fractype == LSYSTEM)
		{
			put_filename("lfile", LFileName);
			put_parm(" lname=%s", LName);
		}
		if (fractype == IFS || fractype == IFS3D)
		{
			put_filename("ifsfile", IFSFileName);
			put_parm(" ifs=%s", IFSName);
		}
		if (fractype == INVERSEJULIA || fractype == INVERSEJULIAFP)
			put_parm(" miim=%s/%s", JIIMmethod[major_method], JIIMleftright[minor_method]);

		showtrig(buf); /* this function is in miscres.c */
		if (buf[0])
			put_parm(buf);

		if (usr_stdcalcmode != 'g')
			put_parm(" passes=%c", usr_stdcalcmode);

		if (stoppass != 0)
			put_parm(" passes=%c%c", usr_stdcalcmode, (char)stoppass + '0');

		if (usemag)
		{
			if (bf_math)
			{
				int digits;
				cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
				digits = getprecbf(MAXREZ);
				put_parm(" center-mag=");
				put_bf(0, bfXctr, digits);
				put_bf(1, bfYctr, digits);
			}
			else /* !bf_math */
			{
				cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
				put_parm(" center-mag=");
				/* convert 1000 fudged long to double, 1000/1<<24 = 6e-5 */
				put_parm(ddelmin > 6e-5 ? "%g/%g" : "%+20.17lf/%+20.17lf", Xctr, Yctr);
			}
#ifdef USE_LONG_DOUBLE
			put_parm("/%.7Lg", Magnification); /* precision of magnification not critical, but magnitude is */
#else
			put_parm("/%.7lg", Magnification); /* precision of magnification not critical, but magnitude is */
#endif
			/* Round to avoid ugly decimals, precision here is not critical */
			/* Don't round Xmagfactor if it's small */
			if (fabs(Xmagfactor) > 0.5) /* or so, exact value isn't important */
				Xmagfactor = (sign(Xmagfactor)*(long)(fabs(Xmagfactor)*1e4 + 0.5)) / 1e4;
			/* Just truncate these angles.  Who cares about 1/1000 of a degree */
			/* Somebody does.  Some rotated and/or skewed images are slightly */
			/* off when recreated from a PAR using 1/1000. */
			/* JCO 08052001 */
#if 0
			Rotation   = (long)(Rotation*1e3)/1e3;
			Skew       = (long)(Skew*1e3)/1e3;
#endif
			if (Xmagfactor != 1 || Rotation != 0 || Skew != 0)
			{	/* Only put what is necessary */
				/* The difference with Xmagfactor is that it is normally */
				/* near 1 while the others are normally near 0 */
				if (fabs(Xmagfactor) >= 1)
					put_float(1, Xmagfactor, 5); /* put_float() uses %g */
				else /* abs(Xmagfactor) is < 1 */
					put_float(1, Xmagfactor, 4); /* put_float() uses %g */
				if (Rotation != 0 || Skew != 0)
				{
					/* Use precision = 6 here.  These angle have already been rounded        */
					/* to 3 decimal places, but angles like 123.456 degrees need 6         */
					/* sig figs to get 3 decimal places.  Trailing 0's are dropped anyway. */
					/* Changed to 18 to address rotated and skewed problem w/ PARs */
					/* JCO 08052001 */
					put_float(1, Rotation, 18);
					if (Skew != 0)
					{
						put_float(1, Skew, 18);
					}
				}
			}
		}
		else /* not usemag */
		{
			put_parm(" corners=");
			if (bf_math)
			{
				int digits;
				digits = getprecbf(MAXREZ);
				put_bf(0, bfxmin, digits);
				put_bf(1, bfxmax, digits);
				put_bf(1, bfymin, digits);
				put_bf(1, bfymax, digits);
				if (cmp_bf(bfx3rd, bfxmin) || cmp_bf(bfy3rd, bfymin))
				{
					put_bf(1, bfx3rd, digits);
					put_bf(1, bfy3rd, digits);
				}
			}
			else
			{
				int xdigits, ydigits;
				xdigits = getprec(xxmin, xxmax, xx3rd);
				ydigits = getprec(yymin, yymax, yy3rd);
				put_float(0, xxmin, xdigits);
				put_float(1, xxmax, xdigits);
				put_float(1, yymin, ydigits);
				put_float(1, yymax, ydigits);
				if (xx3rd != xxmin || yy3rd != yymin)
				{
					put_float(1, xx3rd, xdigits);
					put_float(1, yy3rd, ydigits);
				}
			}
		}

		for (i = (MAXPARAMS-1); i >= 0; --i)
			if (typehasparm((fractype == JULIBROT || fractype == JULIBROTFP)
					? neworbittype : fractype, i, NULL))
				break;

		if (i >= 0)
		{
			if (fractype == CELLULAR || fractype == ANT)
				put_parm(" params=%.1f", param[0]);
			else
			{
#ifdef USE_LONG_DOUBLE
				if (debugflag == 750)
					put_parm(" params=%.17Lg", (long double)param[0]);
				else
#endif
					put_parm(" params=%.17g", param[0]);
			}
			for (j = 1; j <= i; ++j)
				if (fractype == CELLULAR || fractype == ANT)
					put_parm("/%.1f", param[j]);
				else
				{
#ifdef USE_LONG_DOUBLE
					if (debugflag == 750)
						put_parm("/%.17Lg", (long double)param[j]);
					else
#endif
						put_parm("/%.17g", param[j]);
				}
		}

		if (useinitorbit == 2)
			put_parm(" initorbit=pixel");
		else if (useinitorbit == 1)
			put_parm(" initorbit=%.15g/%.15g", initorbit.x, initorbit.y);

		if (floatflag)
			put_parm(" float=y");

		if (maxit != 150)
			put_parm(" maxiter=%ld", maxit);

		if (bailout && (potflag == 0 || potparam[2] == 0.0))
			put_parm(" bailout=%ld", bailout);

		if (bailoutest != Mod)
		{
			put_parm(" bailoutest=");
			if (bailoutest == Real)
				put_parm("real");
			else if (bailoutest == Imag)
				put_parm("imag");
			else if (bailoutest == Or)
				put_parm("or");
			else if (bailoutest == And)
				put_parm("and");
			else if (bailoutest == Manh)
				put_parm("manh");
			else if (bailoutest == Manr)
				put_parm("manr");
			else
				put_parm("mod"); /* default, just in case */
		}
		if (fillcolor != -1)
		{
			put_parm(" fillcolor=%d", fillcolor);
		}
		if (inside != 1)
		{
			put_parm(" inside=");
			if (inside == -1)
				put_parm("maxiter");
			else if (inside == ZMAG)
				put_parm("zmag");
			else if (inside == BOF60)
				put_parm("bof60");
			else if (inside == BOF61)
				put_parm("bof61");
			else if (inside == EPSCROSS)
				put_parm("epsiloncross");
			else if (inside == STARTRAIL)
				put_parm("startrail");
			else if (inside == PERIOD)
				put_parm("period");
			else if (inside == FMODI)
				put_parm("fmod");
			else if (inside == ATANI)
				put_parm("atan");
			else
				put_parm("%d", inside);
		}
		if (closeprox != 0.01 && (inside == EPSCROSS || inside == FMODI
			|| outside == FMOD))
		{
			put_parm(" proximity=%.15g", closeprox);
		}
		if (outside != -1)
		{
			put_parm(" outside=");
			if (outside == REAL)
				put_parm("real");
			else if (outside == IMAG)
				put_parm("imag");
			else if (outside == MULT)
				put_parm("mult");
			else if (outside == SUM)
				put_parm("summ");
			else if (outside == ATAN)
				put_parm("atan");
			else if (outside == FMOD)
				put_parm("fmod");
			else if (outside == TDIS)
				put_parm("tdis");
			else
				put_parm("%d", outside);
		}

		if (LogFlag && !rangeslen)
		{
			put_parm(" logmap=");
			if (LogFlag == -1)
				put_parm("old");
			else if (LogFlag == 1)
				put_parm("yes");
			else
				put_parm("%ld", LogFlag);
		}

		if (Log_Fly_Calc && LogFlag && !rangeslen)
		{
			put_parm(" logmode=");
			if (Log_Fly_Calc == 1)
				put_parm("fly");
			else if (Log_Fly_Calc == 2)
				put_parm("table");
		}

		if (potflag)
		{
			put_parm(" potential=%d/%g/%d",
				(int)potparam[0], potparam[1], (int)potparam[2]);
			if (pot16bit)
				put_parm("/16bit");
		}
		if (invert)
			put_parm(" invert=%-1.15lg/%-1.15lg/%-1.15lg",
				inversion[0], inversion[1], inversion[2]);
		if (decomp[0])
			put_parm(" decomp=%d", decomp[0]);
		if (distest)
		{
			put_parm(" distest=%ld/%d/%d/%d", distest, distestwidth,
							pseudox?pseudox:xdots, pseudoy?pseudoy:ydots);
		}
		if (old_demm_colors)
			put_parm(" olddemmcolors=y");
		if (usr_biomorph != -1)
			put_parm(" biomorph=%d", usr_biomorph);
		if (finattract)
			put_parm(" finattract=y");

		if (forcesymmetry != 999)
		{
			if (forcesymmetry == 1000 && ii == 1 && jj == 1)
				stopmsg(0, "Regenerate before <b> to get correct symmetry");
			put_parm(" symmetry=");
			if (forcesymmetry == XAXIS)
				put_parm("xaxis");
			else if (forcesymmetry == YAXIS)
				put_parm("yaxis");
			else if (forcesymmetry == XYAXIS)
				put_parm("xyaxis");
			else if (forcesymmetry == ORIGIN)
				put_parm("origin");
			else if (forcesymmetry == PI_SYM)
				put_parm("pi");
			else
				put_parm("none");
		}

		if (periodicitycheck != 1)
			put_parm(" periodicity=%d", periodicitycheck);

		if (rflag)
			put_parm(" rseed=%d", rseed);

		if (rangeslen)
		{
			put_parm(" ranges=");
			i = 0;
			while (i < rangeslen)
			{
				if (i)
					put_parm("/");
				if (ranges[i] == -1)
				{
					put_parm("-%d/", ranges[++i]);
					++i;
				}
				put_parm("%d", ranges[i++]);
			}
		}
	}

	if (display3d >= 1)
	{
		/***** 3d transform only parameters in this section *****/
		if (display3d == 2)
			put_parm(" 3d=overlay");
		else
			put_parm(" 3d=yes");
		if (loaded3d == 0)
			put_filename("filename", readname);
		if (SPHERE)
		{
			put_parm(" sphere=y");
			put_parm(" latitude=%d/%d", THETA1, THETA2);
			put_parm(" longitude=%d/%d", PHI1, PHI2);
			put_parm(" radius=%d", RADIUS);
		}
		put_parm(" scalexyz=%d/%d", XSCALE, YSCALE);
		put_parm(" roughness=%d", ROUGH);
		put_parm(" waterline=%d", WATERLINE);
		if (FILLTYPE)
			put_parm(" filltype=%d", FILLTYPE);
		if (transparent[0] || transparent[1])
			put_parm(" transparent=%d/%d", transparent[0], transparent[1]);
		if (preview)
		{
			put_parm(" preview=yes");
			if (showbox)
				put_parm(" showbox=yes");
			put_parm(" coarse=%d", previewfactor);
		}
		if (RAY)
		{
			put_parm(" ray=%d", RAY);
			if (BRIEF)
				put_parm(" brief=y");
		}
		if (FILLTYPE > 4)
		{
			put_parm(" lightsource=%d/%d/%d", XLIGHT, YLIGHT, ZLIGHT);
			if (LIGHTAVG)
				put_parm(" smoothing=%d", LIGHTAVG);
		}
		if (RANDOMIZE)
			put_parm(" randomize=%d", RANDOMIZE);
		if (Targa_Out)
			put_parm(" fullcolor=y");
		if (grayflag)
			put_parm(" usegrayscale=y");
		if (Ambient)
			put_parm(" ambient=%d", Ambient);
		if (haze)
			put_parm(" haze=%d", haze);
		if (back_color[0] != 51 || back_color[1] != 153 || back_color[2] != 200)
			put_parm(" background=%d/%d/%d", back_color[0], back_color[1], back_color[2]);
	}

	if (display3d)  /* universal 3d */
	{
		/***** common (fractal & transform) 3d parameters in this section *****/
		if (!SPHERE || display3d < 0)
			put_parm(" rotation=%d/%d/%d", XROT, YROT, ZROT);
		put_parm(" perspective=%d", ZVIEWER);
		put_parm(" xyshift=%d/%d", XSHIFT, YSHIFT);
		if (xtrans || ytrans)
			put_parm(" xyadjust=%d/%d", xtrans, ytrans);
		if (g_glasses_type)
		{
			put_parm(" stereo=%d", g_glasses_type);
			put_parm(" interocular=%d", g_eye_separation);
			put_parm(" converge=%d", xadjust);
			put_parm(" crop=%d/%d/%d/%d",
				red_crop_left, red_crop_right, blue_crop_left, blue_crop_right);
			put_parm(" bright=%d/%d", red_bright, blue_bright);
		}
	}

	/***** universal parameters in this section *****/

	if (viewwindow == 1)
	{
		put_parm(" viewwindows=%g/%g", viewreduction, finalaspectratio);
		if (viewcrop)
			put_parm("/yes");
		else
			put_parm("/no");
		put_parm("/%d/%d", viewxdots, viewydots);
	}

	if (colorsonly == 0)
	{
		if (rotate_lo != 1 || rotate_hi != 255)
			put_parm(" cyclerange=%d/%d", rotate_lo, rotate_hi);

		if (basehertz != 440)
			put_parm(" hertz=%d", basehertz);

		if (soundflag != (SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER))
		{
			if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_OFF)
				put_parm(" sound=off");
			else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_BEEP)
				put_parm(" sound=beep");
			else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)
				put_parm(" sound=x");
			else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)
				put_parm(" sound=y");
			else if ((soundflag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)
				put_parm(" sound=z");
#ifndef XFRACT
			if ((soundflag & SOUNDFLAG_ORBITMASK) && (soundflag & SOUNDFLAG_ORBITMASK) <= SOUNDFLAG_Z)
			{
				if (soundflag & SOUNDFLAG_SPEAKER)
					put_parm("/pc");
				if (soundflag & SOUNDFLAG_OPL3_FM)
					put_parm("/fm");
				if (soundflag & SOUNDFLAG_MIDI)
					put_parm("/midi");
				if (soundflag & SOUNDFLAG_QUANTIZED)
					put_parm("/quant");
			}
#endif
		}

#ifndef XFRACT
		if (fm_vol != 63)
			put_parm(" volume=%d", fm_vol);

		if (hi_atten != 0)
		{
			if (hi_atten == 1)
				put_parm(" attenuate=low");
			else if (hi_atten == 2)
				put_parm(" attenuate=mid");
			else if (hi_atten == 3)
				put_parm(" attenuate=high");
			else   /* just in case */
				put_parm(" attenuate=none");
		}

		if (polyphony != 0)
			put_parm(" polyphony=%d", polyphony + 1);

		if (fm_wavetype != 0)
			put_parm(" wavetype=%d", fm_wavetype);

		if (fm_attack != 5)
			put_parm(" attack=%d", fm_attack);

		if (fm_decay != 10)
			put_parm(" decay=%d", fm_decay);

		if (fm_sustain != 13)
			put_parm(" sustain=%d", fm_sustain);

		if (fm_release != 5)
			put_parm(" srelease=%d", fm_release);

		if (soundflag & SOUNDFLAG_QUANTIZED)  /* quantize turned on */
		{
			for (i = 0; i <= 11; i++) if (scale_map[i] != i + 1) i = 15;
			if (i > 12)
				put_parm(" scalemap=%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", scale_map[0], scale_map[1], scale_map[2], scale_map[3]
					, scale_map[4], scale_map[5], scale_map[6], scale_map[7], scale_map[8]
					, scale_map[9], scale_map[10], scale_map[11]);
		}
#endif

		if (nobof > 0)
			put_parm(" nobof=yes");

		if (orbit_delay > 0)
			put_parm(" orbitdelay=%d", orbit_delay);

		if (orbit_interval != 1)
			put_parm(" orbitinterval=%d", orbit_interval);

		if (start_showorbit > 0)
			put_parm(" showorbit=yes");

		if (keep_scrn_coords)
			put_parm(" screencoords=yes");

		if (usr_stdcalcmode == 'o' && set_orbit_corners && keep_scrn_coords)
		{
			int xdigits, ydigits;
			put_parm(" orbitcorners=");
			xdigits = getprec(oxmin, oxmax, ox3rd);
			ydigits = getprec(oymin, oymax, oy3rd);
			put_float(0, oxmin, xdigits);
			put_float(1, oxmax, xdigits);
			put_float(1, oymin, ydigits);
			put_float(1, oymax, ydigits);
			if (ox3rd != oxmin || oy3rd != oymin)
			{
				put_float(1, ox3rd, xdigits);
				put_float(1, oy3rd, ydigits);
			}
		}

		if (drawmode != 'r')
			put_parm(" orbitdrawmode=%c", drawmode);

		if (math_tol[0] != 0.05 || math_tol[1] != 0.05)
			put_parm(" mathtolerance=%g/%g", math_tol[0], math_tol[1]);
	}

	if (*colorinf != 'n')
	{
		if (recordcolors == 'c' && *colorinf == '@')
		{
			put_parm_line();
			put_parm("; colors=");
			put_parm(colorinf);
			put_parm_line();
		}
docolors:
		put_parm(" colors=");
		if (recordcolors != 'c' && recordcolors != 'y' && *colorinf == '@')
			put_parm(colorinf);
		else
		{
			int curc, scanc, force, diffmag = -1;
			int delta, diff1[4][3], diff2[4][3];
			curc = force = 0;
#ifdef XFRACT
			if (fake_lut && !truemode) loaddac(); /* stupid kludge JCO 6/23/2001 */
#endif
			while (1)
			{
				/* emit color in rgb 3 char encoded form */
				for (j = 0; j < 3; ++j)
				{
					k = g_dac_box[curc][j];
					if (k < 10)
						k += '0';
					else if (k < 36)
						k += ('A' - 10);
					else
						k += ('_' - 36);
					buf[j] = (char)k;
					}
				buf[3] = 0;
				put_parm(buf);
				if (++curc >= maxcolor)      /* quit if done last color */
					break;
				if (debugflag == 920)  /* lossless compression */
					continue;
				/* Next a P Branderhorst special, a tricky scan for smooth-shaded
					ranges which can be written as <nn> to compress .par file entry.
					Method used is to check net change in each color value over
					spans of 2 to 5 color numbers.  First time for each span size
					the value change is noted.  After first time the change is
					checked against noted change.  First time it differs, a
					a difference of 1 is tolerated and noted as an alternate
					acceptable change.  When change is not one of the tolerated
					values, loop exits. */
				if (force)
				{
					--force;
					continue;
					}
				scanc = curc;
				while (scanc < maxcolor)  /* scan while same diff to next */
				{
					if ((i = scanc - curc) > 3) /* check spans up to 4 steps */
						i = 3;
					for (k = 0; k <= i; ++k)
					{
						for (j = 0; j < 3; ++j)  /* check pattern of chg per color */
						{
							/* Sylvie Gallet's fix */
							if (debugflag != 910 && scanc > (curc + 4) && scanc < maxcolor-5)
								if (abs(2*g_dac_box[scanc][j] - g_dac_box[scanc-5][j]
										- g_dac_box[scanc + 5][j]) >= 2)
									break;
							/* end Sylvie's fix */
							delta = (int)g_dac_box[scanc][j] - (int)g_dac_box[scanc-k-1][j];
							if (k == scanc - curc)
								diff1[k][j] = diff2[k][j] = delta;
							else
								if (delta != diff1[k][j] && delta != diff2[k][j])
								{
									diffmag = abs(delta - diff1[k][j]);
									if (diff1[k][j] != diff2[k][j] || diffmag != 1)
										break;
									diff2[k][j] = delta;
									}
							}
						if (j < 3) break; /* must've exited from inner loop above */
						}
					if (k <= i) break;   /* must've exited from inner loop above */
					++scanc;
					}
				/* now scanc-1 is next color which must be written explicitly */
				if (scanc - curc > 2)  /* good, we have a shaded range */
				{
					if (scanc != maxcolor)
					{
						if (diffmag < 3)  /* not a sharp slope change? */
						{
							force = 2;       /* force more between ranges, to stop  */
							--scanc;         /* "drift" when load/store/load/store/ */
							}
						if (k)  /* more of the same                    */
						{
							force += k;
							--scanc;
							}
						}
					if (--scanc - curc > 1)
					{
						put_parm("<%d>", scanc-curc);
						curc = scanc;
						}
					else                /* changed our mind */
						force = 0;
					}
				}
			}
		}

	while (s_wbdata.len) /* flush the buffer */
		put_parm_line();

	restore_stack(saved);
}

static void put_filename(char *keyword, char *fname)
{
	char *p;
	if (*fname && !endswithslash(fname))
	{
		p = strrchr(fname, SLASHC);
		if (p != NULL)
		{
			fname = p + 1;
			if (*fname == 0) return;
		}
		put_parm(" %s=%s", keyword, fname);
	}
}

#ifndef USE_VARARGS
static void put_parm(char *parm, ...)
#else
static void put_parm(va_alist)
va_dcl
#endif
{
	char *bufptr;
	va_list args;

#ifndef USE_VARARGS
	va_start(args, parm);
#else
	char *parm;

	va_start(args);
	parm = va_arg(args, char *);
#endif
	if (*parm == ' '             /* starting a new parm */
			&& s_wbdata.len == 0)       /* skip leading space */
		++parm;
	bufptr = s_wbdata.buf + s_wbdata.len;
	vsprintf(bufptr, parm, args);
	while (*(bufptr++))
		++s_wbdata.len;
	while (s_wbdata.len > 200)
		put_parm_line();
}

int maxlinelength = 72;
#define MAXLINELEN  maxlinelength
#define NICELINELEN (MAXLINELEN-4)

static void put_parm_line()
{
	int len, c;
	len = s_wbdata.len;
	if (len > NICELINELEN)
	{
		len = NICELINELEN + 1;
		while (--len != 0 && s_wbdata.buf[len] != ' ')
		{ }
		if (len == 0)
		{
			len = NICELINELEN-1;
			while (++len < MAXLINELEN
				&& s_wbdata.buf[len] && s_wbdata.buf[len] != ' ')
			{
			}
		}
	}
	c = s_wbdata.buf[len];
	s_wbdata.buf[len] = 0;
	fputs("  ", parmfile);
	fputs(s_wbdata.buf, parmfile);
	if (c && c != ' ')
		fputc('\\', parmfile);
	fputc('\n', parmfile);
	s_wbdata.buf[len] = (char)c;
	if (c == ' ')
		++len;
	s_wbdata.len -= len;
	strcpy(s_wbdata.buf, s_wbdata.buf + len);
}

int getprecbf_mag()
{
	double Xmagfactor, Rotation, Skew;
	LDBL Magnification;
	bf_t bXctr, bYctr;
	int saved, dec;

	saved = save_stack();
	bXctr            = alloc_stack(bflength + 2);
	bYctr            = alloc_stack(bflength + 2);
	/* this is just to find Magnification */
	cvtcentermagbf(bXctr, bYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
	restore_stack(saved);

	/* I don't know if this is portable, but something needs to */
	/* be used in case compiler's LDBL_MAX is not big enough    */
	if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
		return -1;

	dec = getpower10(Magnification) + 4; /* 4 digits of padding sounds good */
	return dec;
}

static int getprec(double a, double b, double c)
{
	double diff, temp;
	int digits;
	double highv = 1.0E20;
	diff = fabs(a - b);
	if (diff == 0.0) diff = highv;
	temp = fabs(a - c);
	if (temp == 0.0) temp = highv;
	if (temp < diff) diff = temp;
	temp = fabs(b - c);
	if (temp == 0.0) temp = highv;
	if (temp < diff) diff = temp;
	digits = 7;
	if (debugflag >= 700 && debugflag < 720)
		digits =  debugflag - 700;
	while (diff < 1.0 && digits <= DBL_DIG + 1)
	{
		diff *= 10;
		++digits;
		}
	return digits;
}

/* This function calculates the precision needed to distiguish adjacent
	pixels at Fractint's maximum resolution of MAXPIXELS by MAXPIXELS
	(if rez == MAXREZ) or at current resolution (if rez == CURRENTREZ)    */
int getprecbf(int rezflag)
{
	bf_t del1, del2, one, bfxxdel, bfxxdel2, bfyydel, bfyydel2;
	int digits, dec;
	int saved;
	int rez;
	saved    = save_stack();
	del1     = alloc_stack(bflength + 2);
	del2     = alloc_stack(bflength + 2);
	one      = alloc_stack(bflength + 2);
	bfxxdel   = alloc_stack(bflength + 2);
	bfxxdel2  = alloc_stack(bflength + 2);
	bfyydel   = alloc_stack(bflength + 2);
	bfyydel2  = alloc_stack(bflength + 2);
	floattobf(one, 1.0);
	if (rezflag == MAXREZ)
		rez = OLDMAXPIXELS -1;
	else
		rez = xdots-1;

	/* bfxxdel = (bfxmax - bfx3rd)/(xdots-1) */
	sub_bf(bfxxdel, bfxmax, bfx3rd);
	div_a_bf_int(bfxxdel, (U16)rez);

	/* bfyydel2 = (bfy3rd - bfymin)/(xdots-1) */
	sub_bf(bfyydel2, bfy3rd, bfymin);
	div_a_bf_int(bfyydel2, (U16)rez);

	if (rezflag == CURRENTREZ)
		rez = ydots-1;

	/* bfyydel = (bfymax - bfy3rd)/(ydots-1) */
	sub_bf(bfyydel, bfymax, bfy3rd);
	div_a_bf_int(bfyydel, (U16)rez);

	/* bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1) */
	sub_bf(bfxxdel2, bfx3rd, bfxmin);
	div_a_bf_int(bfxxdel2, (U16)rez);

	abs_a_bf(add_bf(del1, bfxxdel, bfxxdel2));
	abs_a_bf(add_bf(del2, bfyydel, bfyydel2));
	if (cmp_bf(del2, del1) < 0)
		copy_bf(del1, del2);
	if (cmp_bf(del1, clear_bf(del2)) == 0)
	{
		restore_stack(saved);
		return -1;
	}
	digits = 1;
	while (cmp_bf(del1, one) < 0)
	{
		digits++;
		mult_a_bf_int(del1, 10);
	}
	digits = max(digits, 3);
	restore_stack(saved);
	dec = getprecbf_mag();
	return max(digits, dec);
}

#ifdef _MSC_VER
#pragma optimize("e", off)  /* MSC 7.00 messes up next with "e" on */
#endif

/* This function calculates the precision needed to distiguish adjacent
	pixels at Fractint's maximum resolution of MAXPIXELS by MAXPIXELS
	(if rez == MAXREZ) or at current resolution (if rez == CURRENTREZ)    */
int getprecdbl(int rezflag)
{
	LDBL del1, del2, xdel, xdel2, ydel, ydel2;
	int digits;
	LDBL rez;
	if (rezflag == MAXREZ)
		rez = OLDMAXPIXELS -1;
	else
		rez = xdots-1;

	xdel =  ((LDBL)xxmax - (LDBL)xx3rd)/rez;
	ydel2 = ((LDBL)yy3rd - (LDBL)yymin)/rez;

	if (rezflag == CURRENTREZ)
		rez = ydots-1;

	ydel = ((LDBL)yymax - (LDBL)yy3rd)/rez;
	xdel2 = ((LDBL)xx3rd - (LDBL)xxmin)/rez;

	del1 = fabsl(xdel) + fabsl(xdel2);
	del2 = fabsl(ydel) + fabsl(ydel2);
	if (del2 < del1)
		del1 = del2;
	if (del1 == 0)
	{
#ifdef DEBUG
		showcornersdbl("getprecdbl");
#endif
		return -1;
	}
	digits = 1;
	while (del1 < 1.0)
	{
		digits++;
		del1 *= 10;
	}
	digits = max(digits, 3);
	return digits;
}

#ifdef _MSC_VER
#pragma optimize("e", on)
#endif

/*
	Strips zeros from the non-exponent part of a number. This logic
	was originally in put_bf(), but is split into this routine so it can be
	shared with put_float(), which had a bug in Fractint 19.2 (used to strip
	zeros from the exponent as well.)
*/

static void strip_zeros(char *buf)
{
	char *dptr, *bptr, *exptr;
	strlwr(buf);
	dptr = strchr(buf, '.');
	if (dptr != 0)
	{
		++dptr;
		exptr = strchr(buf, 'e');
		if (exptr != 0)  /* scientific notation with 'e'? */
			bptr = exptr;
		else
			bptr = buf + strlen(buf);
		while (--bptr > dptr && *bptr == '0')
			*bptr = 0;
		if (exptr && bptr < exptr -1)
		strcat(buf, exptr);
	}
}

static void put_float(int slash, double fnum, int prec)
{  char buf[40];
	char *bptr;
	bptr = buf;
	if (slash)
		*(bptr++) = '/';
/*   sprintf(bptr, "%1.*f", prec, fnum); */
#ifdef USE_LONG_DOUBLE
	/* Idea of long double cast is to squeeze out another digit or two
		which might be needed (we have found cases where this digit makes
		a difference.) But lets not do this at lower precision */
	if (prec > 15)
		sprintf(bptr, "%1.*Lg", prec, (long double)fnum);
	else
#endif
		sprintf(bptr, "%1.*g", prec, (double)fnum);
	strip_zeros(bptr);
	put_parm(buf);
}

static void put_bf(int slash, bf_t r, int prec)
{
	char *buf; /* "/-1.xxxxxxE-1234" */
	char *bptr;
	/* buf = malloc(decimals + 11); */
	buf = s_wbdata.buf + 5000;  /* end of use suffix buffer, 5000 bytes safe */
	bptr = buf;
	if (slash)
		*(bptr++) = '/';
	bftostr(bptr, prec, r);
	strip_zeros(bptr);
	put_parm(buf);
}

void edit_text_colors()
{
	int	save_debugflag, 	save_lookatmouse;
	int	row, col, bkgrd;
	int	rowf, colf, rowt, colt;
	int	i, j, k;

	save_debugflag = debugflag;
	save_lookatmouse = lookatmouse;
	debugflag =	0;	 /*	don't get called recursively */
	lookatmouse	= LOOK_MOUSE_TEXT; /* text mouse sensitivity */
	row	= col =	bkgrd =	rowt = rowf	= colt = colf =	0;

	while (1)
	{
		if (row	< 0)
		{
			row = 0;
		}
		else if (row > 24)
		{
			row =	24;
		}
		if (col < 0)
		{
			col = 0;
		}
		else if (col > 79)
		{
			col =	79;
		}
		driver_move_cursor(row, col);
		i =	toupper(driver_get_key());

		switch (i)
		{
		case FIK_ESC:
			debugflag =	save_debugflag;
			lookatmouse	= save_lookatmouse;
			driver_hide_text_cursor();
			return;
		case '/':
			driver_hide_text_cursor();
			driver_stack_screen();
			for	(i = 0;	i <	8; ++i)		  /* 8 bkgrd attrs */
			{
				for	(j = 0;	j <	16;	++j) /*	16 fgrd	attrs */
				{
#if defined(_WIN32)
					_ASSERTE(_CrtCheckMemory());
#endif
					k =	(i*16 + j);
					driver_put_char_attr_rowcol(i*2, j*5, (' ' << 8) | k);
					driver_put_char_attr_rowcol(i*2, j*5 + 1, ((i + '0') << 8)| k);
					driver_put_char_attr_rowcol(i*2, j*5 + 2, (((j < 10) ? j + '0' : j + 'A'-10) << 8) | k);
					driver_put_char_attr_rowcol(i*2, j*5 + 3, (' ' << 8) | k);
				}
			}
			driver_get_key();
			driver_unstack_screen();
			driver_move_cursor(row, col);
			break;
		case ',':
			rowf = row;
			colf = col;
			break;
		case '.':
			rowt = row;
			colt = col;
			break;
		case ' ': /* next color is	background */
			bkgrd =	1;
			break;
		case FIK_LEFT_ARROW: /* cursor	left  */
			--col;
			break;
		case FIK_RIGHT_ARROW: /* cursor right */
			++col;
			break;
		case FIK_UP_ARROW:	/* cursor up	*/
			--row;
			break;
		case FIK_DOWN_ARROW: /* cursor	down  */
			++row;
			break;
		case FIK_ENTER:   /* enter	*/
			{
				int char_attr = driver_get_char_attr_rowcol(row, col);
				char_attr &= ~0xFF;
				char_attr |= driver_get_key() & 0xFF;
				driver_put_char_attr_rowcol(row, col, char_attr);
			}
			break;
		default:
			if (i >= '0' &&	i <= '9')
			{
				i -= '0';
			}
			else if	(i >= 'A' && i <= 'F')
			{
				i -= 'A' - 10;
			}
			else
			{
				break;
			}
			for	(j = rowf; j <=	rowt; ++j)
			{
				for (k = colf; k <= colt; ++k)
				{
					int char_attr;
					char_attr = driver_get_char_attr_rowcol(j, k);
					if (bkgrd)
					{
						char_attr &= 0xFF0F;
						char_attr |= (i & 0xF) << 4;
					}
					else
					{
						char_attr &= 0xFFF0;
						char_attr |= i & 0xF;
					}
					driver_put_char_attr_rowcol(j, k, char_attr);
				}
			}
			bkgrd =	0;
		}
	}
}

static int *entsptr;
static int modes_changed;

int select_video_mode(int curmode)
{
	int entnums[MAXVIDEOMODES];
	int attributes[MAXVIDEOMODES];
	int i, k, ret;
#ifndef XFRACT
	int oldtabmode, oldhelpmode;
#endif

	for (i = 0; i < g_video_table_len; ++i)  /* init tables */
	{
		entnums[i] = i;
		attributes[i] = 1;
	}
	entsptr = entnums;           /* for indirectly called subroutines */

	qsort(entnums, g_video_table_len, sizeof(entnums[0]), entcompare); /* sort modes */

	/* pick default mode */
	if (curmode < 0)
	{
		g_video_entry.videomodeax = 19;  /* vga */
		g_video_entry.colors = 256;
	}
	else
	{
		memcpy((char *) &g_video_entry, (char *) &g_video_table[curmode], sizeof(g_video_entry));
	}
#ifndef XFRACT
	for (i = 0; i < g_video_table_len; ++i)  /* find default mode */
	{
		if (g_video_entry.videomodeax == g_video_table[entnums[i]].videomodeax &&
			g_video_entry.colors      == g_video_table[entnums[i]].colors &&
			(curmode < 0 ||
			memcmp((char *) &g_video_entry, (char *) &g_video_table[entnums[i]], sizeof(g_video_entry)) == 0))
		{
			break;
		}
	}
	if (i >= g_video_table_len) /* no match, default to first entry */
	{
		i = 0;
	}

	oldtabmode = tabmode;
	oldhelpmode = helpmode;
	modes_changed = 0;
	tabmode = 0;
	helpmode = HELPVIDSEL;
	i = fullscreen_choice(CHOICE_HELP,
		"Select Video Mode",
		"key...name.......................xdot..ydot.colr.driver......comment......",
		NULL, g_video_table_len, NULL, attributes,
		1, 16, 74, i, format_vid_table, NULL, NULL, check_modekey);
	tabmode = oldtabmode;
	helpmode = oldhelpmode;
	if (i == -1)
	{
		/* update fractint.cfg for new key assignments */
		if (modes_changed && g_bad_config == 0 &&
			stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER | STOPMSG_INFO_ONLY,
				"Save new function key assignments or cancel changes?") == 0)
		{
			update_fractint_cfg();
		}
		return -1;
	}
	/* picked by function key or ENTER key */
	i = (i < 0) ? (-1 - i) : entnums[i];
#endif
	/* the selected entry now in g_video_entry */
	memcpy((char *) &g_video_entry, (char *) &g_video_table[i], sizeof(g_video_entry));

#ifndef XFRACT
	/* copy fractint.cfg table to resident table, note selected entry */
	k = 0;
	for (i = 0; i < g_video_table_len; ++i)
	{
		if (g_video_table[i].keynum > 0)
		{
			if (memcmp((char *)&g_video_entry, (char *)&g_video_table[i],
							sizeof(g_video_entry)) == 0)
			{
				k = g_video_table[i].keynum;
			}
		}
	}
#else
	k = g_video_table[0].keynum;
#endif
	ret = k;
	if (k == 0)  /* selected entry not a copied (assigned to key) one */
	{
		memcpy((char *)&g_video_table[MAXVIDEOMODES-1],
					(char *)&g_video_entry, sizeof(*g_video_table));
		ret = 1400; /* special value for check_vidmode_key */
	}

	/* update fractint.cfg for new key assignments */
	if (modes_changed && g_bad_config == 0)
	{
		update_fractint_cfg();
	}

	return ret;
}

void format_vid_table(int choice, char *buf)
{
	char local_buf[81];
	char kname[5];
	int truecolorbits;
	memcpy((char *)&g_video_entry, (char *)&g_video_table[entsptr[choice]],
		sizeof(g_video_entry));
	vidmode_keyname(g_video_entry.keynum, kname);
	sprintf(buf, "%-5s %-25s %5d %5d ",  /* 44 chars */
		kname, g_video_entry.name, g_video_entry.xdots, g_video_entry.ydots);
	truecolorbits = g_video_entry.dotmode/1000;
	if (truecolorbits == 0)
		sprintf(local_buf, "%s%3d",  /* 47 chars */
			buf, g_video_entry.colors);
	else
		sprintf(local_buf, "%s%3s",  /* 47 chars */
			buf, (truecolorbits == 4)?" 4g":
				(truecolorbits == 3)?"16m":
				(truecolorbits == 2)?"64k":
				(truecolorbits == 1)?"32k":"???");
	sprintf(buf, "%s %.12s %.12s",  /* 74 chars */
		local_buf, g_video_entry.driver->name, g_video_entry.comment);
}

#ifndef XFRACT
static int check_modekey(int curkey, int choice)
{
	int i, j, k, ret;
	i = check_vidmode_key(1, curkey);
	if (i >= 0)
		return -1-i;
	i = entsptr[choice];
	ret = 0;
	if ((curkey == '-' || curkey == '+')
		&& (g_video_table[i].keynum == 0 || g_video_table[i].keynum >= 1084))
	{
		if (g_bad_config)
			stopmsg(0, "Missing or bad FRACTINT.CFG file. Can't reassign keys.");
		else
		{
			if (curkey == '-')  /* deassign key? */
			{
				if (g_video_table[i].keynum >= 1084)
				{
					g_video_table[i].keynum = 0;
					modes_changed = 1;
					}
				}
			else  /* assign key? */
			{
				j = getakeynohelp();
				if (j >= 1084 && j <= 1113)
				{
					for (k = 0; k < g_video_table_len; ++k)
					{
						if (g_video_table[k].keynum == j)
						{
							g_video_table[k].keynum = 0;
							ret = -1; /* force redisplay */
							}
						}
					g_video_table[i].keynum = j;
					modes_changed = 1;
					}
				}
			}
		}
	return ret;
}
#endif

static int entcompare(VOIDCONSTPTR p1, VOIDCONSTPTR p2)
{
	int i, j;
	i = g_video_table[*((int *)p1)].keynum;
	if (i == 0) i = 9999;
	j = g_video_table[*((int *)p2)].keynum;
	if (j == 0) j = 9999;
	if (i < j || (i == j && *((int *)p1) < *((int *)p2)))
		return -1;
	return 1;
}

static void update_fractint_cfg()
{
	extern int g_cfg_line_nums[];
#ifndef XFRACT
	char cfgname[100], outname[100], buf[121], kname[5];
	FILE *cfgfile, *outfile;
	int i, j, linenum, nextlinenum, nextmode;
	struct videoinfo vident;

	findpath("fractint.cfg", cfgname);

	if (access(cfgname, 6))
	{
		sprintf(buf, "Can't write %s", cfgname);
		stopmsg(0, buf);
		return;
		}
	strcpy(outname, cfgname);
	i = (int) strlen(outname);
	while (--i >= 0 && outname[i] != SLASHC)
	outname[i] = 0;
	strcat(outname, "fractint.tmp");
	outfile = fopen(outname, "w");
	if (outfile == NULL)
	{
		sprintf(buf, "Can't create %s", outname);
		stopmsg(0, buf);
		return;
		}
	cfgfile = fopen(cfgname, "r");

	linenum = nextmode = 0;
	nextlinenum = g_cfg_line_nums[0];
	while (fgets(buf, 120, cfgfile))
	{
		int truecolorbits;
		char colorsbuf[10];
		++linenum;
		if (linenum == nextlinenum)  /* replace this line */
		{
			memcpy((char *)&vident, (char *)&g_video_table[nextmode],
					sizeof(g_video_entry));
			vidmode_keyname(vident.keynum, kname);
			strcpy(buf, vident.name);
			i = (int) strlen(buf);
			while (i && buf[i-1] == ' ') /* strip trailing spaces to compress */
				--i;
			j = i + 5;
			while (j < 32)  /* tab to column 33 */
			{
				buf[i++] = '\t';
				j += 8;
				}
			buf[i] = 0;
			truecolorbits = vident.dotmode/1000;
			if (truecolorbits == 0)
				sprintf(colorsbuf, "%3d", vident.colors);
			else
				sprintf(colorsbuf, "%3s",
					(truecolorbits == 4)?" 4g":
					(truecolorbits == 3)?"16m":
					(truecolorbits == 2)?"64k":
					(truecolorbits == 1)?"32k":"???");
			fprintf(outfile, "%-4s,%s,%4x,%4x,%4x,%4x,%4d,%5d,%5d,%s,%s\n",
				kname,
				buf,
				vident.videomodeax,
				vident.videomodebx,
				vident.videomodecx,
				vident.videomodedx,
				vident.dotmode%1000, /* remove true-color flag, keep g_text_safe */
				vident.xdots,
				vident.ydots,
				colorsbuf,
				vident.comment);
			if (++nextmode >= g_video_table_len)
				nextlinenum = 32767;
			else
				nextlinenum = g_cfg_line_nums[nextmode];
			}
		else
			fputs(buf, outfile);
		}

	fclose(cfgfile);
	fclose(outfile);
	unlink(cfgname);         /* success assumed on these lines       */
	rename(outname, cfgname); /* since we checked earlier with access */
#endif
}

/* make_mig() takes a collection of individual GIF images (all
	presumably the same resolution and all presumably generated
	by Fractint and its "divide and conquer" algorithm) and builds
	a single multiple-image GIF out of them.  This routine is
	invoked by the "batch=stitchmode/x/y" option, and is called
	with the 'x' and 'y' parameters
*/

void make_mig(unsigned int xmult, unsigned int ymult)
{
	unsigned int xstep, ystep;
	unsigned int xres, yres;
	unsigned int allxres, allyres, xtot, ytot;
	unsigned int xloc, yloc;
	unsigned char ichar;
	unsigned int allitbl, itbl;
	unsigned int i;
	char gifin[15], gifout[15];
	int errorflag, inputerrorflag;
	unsigned char *temp;
	FILE *out, *in;

	errorflag = 0;                          /* no errors so */
	inputerrorflag = 0;
	allxres = allyres = allitbl = 0;
	out = in = NULL;

	strcpy(gifout, "fractmig.gif");

	temp= &olddacbox[0][0];                 /* a safe place for our temp data */

	gif87a_flag = 1;                        /* for now, force this */

	/* process each input image, one at a time */
	for (ystep = 0; ystep < ymult; ystep++)
	{
		for (xstep = 0; xstep < xmult; xstep++)
		{
			if (xstep == 0 && ystep == 0)          /* first time through? */
			{
				printf(" \n Generating multi-image GIF file %s using", gifout);
				printf(" %d X and %d Y components\n\n", xmult, ymult);
				/* attempt to create the output file */
				out = fopen(gifout, "wb");
				if (out == NULL)
				{
					printf("Cannot create output file %s!\n", gifout);
					exit(1);
				}
			}

			sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));

			in = fopen(gifin, "rb");
			if (in == NULL)
			{
				printf("Can't open file %s!\n", gifin);
				exit(1);
				}

			/* (read, but only copy this if it's the first time through) */
			if (fread(temp, 13, 1, in) != 1)   /* read the header and LDS */
			{
				inputerrorflag = 1;
			}
			memcpy(&xres, &temp[6], 2);     /* X-resolution */
			memcpy(&yres, &temp[8], 2);     /* Y-resolution */

			if (xstep == 0 && ystep == 0)  /* first time through? */
			{
				allxres = xres;             /* save the "master" resolution */
				allyres = yres;
				xtot = xres*xmult;        /* adjust the image size */
				ytot = yres*ymult;
				memcpy(&temp[6], &xtot, 2);
				memcpy(&temp[8], &ytot, 2);
				if (gif87a_flag)
				{
					temp[3] = '8';
					temp[4] = '7';
					temp[5] = 'a';
				}
				temp[12] = 0; /* reserved */
				if (fwrite(temp, 13, 1, out) != 1)     /* write out the header */
				{
					errorflag = 1;
				}
			}                           /* end of first-time-through */

			ichar = (char)(temp[10] & 0x07);        /* find the color table size */
			itbl = 1 << (++ichar);
			ichar = (char)(temp[10] & 0x80);        /* is there a global color table? */
			if (xstep == 0 && ystep == 0)   /* first time through? */
			{
				allitbl = itbl;             /* save the color table size */
			}
			if (ichar != 0)                /* yup */
			{
				/* (read, but only copy this if it's the first time through) */
				if (fread(temp, 3*itbl, 1, in) != 1)    /* read the global color table */
				{
					inputerrorflag = 2;
				}
				if (xstep == 0 && ystep == 0)       /* first time through? */
				{
					if (fwrite(temp, 3*itbl, 1, out) != 1)     /* write out the GCT */
					{
						errorflag = 2;
					}
				}
			}

			if (xres != allxres || yres != allyres || itbl != allitbl)
			{
				/* Oops - our pieces don't match */
				printf("File %s doesn't have the same resolution as its predecessors!\n", gifin);
				exit(1);
				}

			while (1)                       /* process each information block */
			{
				memset(temp, 0, 10);
				if (fread(temp, 1, 1, in) != 1)    /* read the block identifier */
				{
					inputerrorflag = 3;
				}

				if (temp[0] == 0x2c)           /* image descriptor block */
				{
					if (fread(&temp[1], 9, 1, in) != 1)    /* read the Image Descriptor */
					{
						inputerrorflag = 4;
					}
					memcpy(&xloc, &temp[1], 2); /* X-location */
					memcpy(&yloc, &temp[3], 2); /* Y-location */
					xloc += (xstep*xres);     /* adjust the locations */
					yloc += (ystep*yres);
					memcpy(&temp[1], &xloc, 2);
					memcpy(&temp[3], &yloc, 2);
					if (fwrite(temp, 10, 1, out) != 1)     /* write out the Image Descriptor */
					{
						errorflag = 4;
					}

					ichar = (char)(temp[9] & 0x80);     /* is there a local color table? */
					if (ichar != 0)            /* yup */
					{
						if (fread(temp, 3*itbl, 1, in) != 1)       /* read the local color table */
						{
							inputerrorflag = 5;
						}
						if (fwrite(temp, 3*itbl, 1, out) != 1)     /* write out the LCT */
						{
							errorflag = 5;
						}
					}

					if (fread(temp, 1, 1, in) != 1)        /* LZH table size */
					{
						inputerrorflag = 6;
					}
					if (fwrite(temp, 1, 1, out) != 1)
					{
						errorflag = 6;
					}
					while (1)
					{
						if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
						{
							break;
						}
						if (fread(temp, 1, 1, in) != 1)    /* block size */
						{
							inputerrorflag = 7;
						}
						if (fwrite(temp, 1, 1, out) != 1)
						{
							errorflag = 7;
						}
						i = temp[0];
						if (i == 0)
						{
							break;
						}
						if (fread(temp, i, 1, in) != 1)    /* LZH data block */
						{
							inputerrorflag = 8;
						}
						if (fwrite(temp, i, 1, out) != 1)
						{
							errorflag = 8;
						}
					}
				}

				if (temp[0] == 0x21)           /* extension block */
				{
					/* (read, but only copy this if it's the last time through) */
					if (fread(&temp[2], 1, 1, in) != 1)    /* read the block type */
					{
						inputerrorflag = 9;
					}
					if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
					{
						if (fwrite(temp, 2, 1, out) != 1)
						{
							errorflag = 9;
						}
					}
					while (1)
					{
						if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
						{
							break;
						}
						if (fread(temp, 1, 1, in) != 1)    /* block size */
						{
							inputerrorflag = 10;
						}
						if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
						{
							if (fwrite(temp, 1, 1, out) != 1)
							{
								errorflag = 10;
							}
						}
						i = temp[0];
						if (i == 0)
						{
							break;
						}
						if (fread(temp, i, 1, in) != 1)    /* data block */
						{
							inputerrorflag = 11;
						}
						if ((!gif87a_flag) && xstep == xmult-1 && ystep == ymult-1)
						{
							if (fwrite(temp, i, 1, out) != 1)
							{
								errorflag = 11;
							}
						}
					}
				}

				if (temp[0] == 0x3b)           /* end-of-stream indicator */
				{
					break;                      /* done with this file */
				}

				if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
				{
					break;
				}
			}
			fclose(in);                     /* done with an input GIF */

			if (errorflag != 0 || inputerrorflag != 0)      /* oops - did something go wrong? */
			{
				break;
			}
		}

		if (errorflag != 0 || inputerrorflag != 0)  /* oops - did something go wrong? */
		{
			break;
		}
	}

	temp[0] = 0x3b;                 /* end-of-stream indicator */
	if (fwrite(temp, 1, 1, out) != 1)
	{
		errorflag = 12;
	}
	fclose(out);                    /* done with the output GIF */

	if (inputerrorflag != 0)       /* uh-oh - something failed */
	{
		printf("\007 Process failed = early EOF on input file %s\n", gifin);
		/* following line was for debugging
			printf("inputerrorflag = %d\n", inputerrorflag);
		*/
	}

	if (errorflag != 0)            /* uh-oh - something failed */
	{
		printf("\007 Process failed = out of disk space?\n");
		/* following line was for debugging
			printf("errorflag = %d\n", errorflag);
		*/
	}

	/* now delete each input image, one at a time */
	if (errorflag == 0 && inputerrorflag == 0)
	{
		for (ystep = 0; ystep < ymult; ystep++)
		{
			for (xstep = 0; xstep < xmult; xstep++)
			{
				sprintf(gifin, "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));
				remove(gifin);
			}
		}
	}

	/* tell the world we're done */
	if (errorflag == 0 && inputerrorflag == 0)
	{
		printf("File %s has been created (and its component files deleted)\n", gifout);
	}
}

/* This routine copies the current screen to by flipping x-axis, y-axis,
	or both. Refuses to work if calculation in progress or if fractal
	non-resumable. Clears zoombox if any. Resets corners so resulting fractal
	is still valid. */
void flip_image(int key)
{
	int i, j, ixhalf, iyhalf, tempdot;

	/* fractal must be rotate-able and be finished */
	if ((curfractalspecific->flags&NOROTATE) != 0
			|| calc_status == CALCSTAT_IN_PROGRESS
			|| calc_status == CALCSTAT_RESUMABLE)
		return;
	if (bf_math)
		clear_zoombox(); /* clear, don't copy, the zoombox */
	ixhalf = xdots / 2;
	iyhalf = ydots / 2;
	switch (key)
	{
	case 24:            /* control-X - reverse X-axis */
		for (i = 0; i < ixhalf; i++)
		{
			if (driver_key_pressed())
				break;
			for (j = 0; j < ydots; j++)
			{
				tempdot=getcolor(i, j);
				putcolor(i, j, getcolor(xdots-1-i, j));
				putcolor(xdots-1-i, j, tempdot);
			}
		}
		sxmin = xxmax + xxmin - xx3rd;
		symax = yymax + yymin - yy3rd;
		sxmax = xx3rd;
		symin = yy3rd;
		sx3rd = xxmax;
		sy3rd = yymin;
		if (bf_math)
		{
			add_bf(bfsxmin, bfxmax, bfxmin); /* sxmin = xxmax + xxmin - xx3rd; */
			sub_a_bf(bfsxmin, bfx3rd);
			add_bf(bfsymax, bfymax, bfymin); /* symax = yymax + yymin - yy3rd; */
			sub_a_bf(bfsymax, bfy3rd);
			copy_bf(bfsxmax, bfx3rd);        /* sxmax = xx3rd; */
			copy_bf(bfsymin, bfy3rd);        /* symin = yy3rd; */
			copy_bf(bfsx3rd, bfxmax);        /* sx3rd = xxmax; */
			copy_bf(bfsy3rd, bfymin);        /* sy3rd = yymin; */
		}
		break;
	case 25:            /* control-Y - reverse Y-aXis */
		for (j = 0; j < iyhalf; j++)
		{
			if (driver_key_pressed())
				break;
			for (i = 0; i < xdots; i++)
			{
				tempdot=getcolor(i, j);
				putcolor(i, j, getcolor(i, ydots-1-j));
				putcolor(i, ydots-1-j, tempdot);
			}
		}
		sxmin = xx3rd;
		symax = yy3rd;
		sxmax = xxmax + xxmin - xx3rd;
		symin = yymax + yymin - yy3rd;
		sx3rd = xxmin;
		sy3rd = yymax;
		if (bf_math)
		{
			copy_bf(bfsxmin, bfx3rd);        /* sxmin = xx3rd; */
			copy_bf(bfsymax, bfy3rd);        /* symax = yy3rd; */
			add_bf(bfsxmax, bfxmax, bfxmin); /* sxmax = xxmax + xxmin - xx3rd; */
			sub_a_bf(bfsxmax, bfx3rd);
			add_bf(bfsymin, bfymax, bfymin); /* symin = yymax + yymin - yy3rd; */
			sub_a_bf(bfsymin, bfy3rd);
			copy_bf(bfsx3rd, bfxmin);        /* sx3rd = xxmin; */
			copy_bf(bfsy3rd, bfymax);        /* sy3rd = yymax; */
		}
		break;
	case 26:            /* control-Z - reverse X and Y aXis */
		for (i = 0; i < ixhalf; i++)
		{
			if (driver_key_pressed())
				break;
			for (j = 0; j < ydots; j++)
			{
				tempdot=getcolor(i, j);
				putcolor(i, j, getcolor(xdots-1-i, ydots-1-j));
				putcolor(xdots-1-i, ydots-1-j, tempdot);
			}
		}
		sxmin = xxmax;
		symax = yymin;
		sxmax = xxmin;
		symin = yymax;
		sx3rd = xxmax + xxmin - xx3rd;
		sy3rd = yymax + yymin - yy3rd;
		if (bf_math)
		{
			copy_bf(bfsxmin, bfxmax);        /* sxmin = xxmax; */
			copy_bf(bfsymax, bfymin);        /* symax = yymin; */
			copy_bf(bfsxmax, bfxmin);        /* sxmax = xxmin; */
			copy_bf(bfsymin, bfymax);        /* symin = yymax; */
			add_bf(bfsx3rd, bfxmax, bfxmin); /* sx3rd = xxmax + xxmin - xx3rd; */
			sub_a_bf(bfsx3rd, bfx3rd);
			add_bf(bfsy3rd, bfymax, bfymin); /* sy3rd = yymax + yymin - yy3rd; */
			sub_a_bf(bfsy3rd, bfy3rd);
		}
		break;
	}
	reset_zoom_corners();
	calc_status = CALCSTAT_PARAMS_CHANGED;
}
static char *expand_var(char *var, char *buf)
{
	time_t ltime;
	char *str, *out;

	time(&ltime);
	str = ctime(&ltime);

	/* ctime format             */
	/* Sat Aug 17 21:34:14 1996 */
	/* 012345678901234567890123 */
	/*           1         2    */
	if (strcmp(var, "year") == 0)       /* 4 chars */
	{
		str[24] = '\0';
		out = &str[20];
	}
	else if (strcmp(var, "month") == 0) /* 3 chars */
	{
		str[7] = '\0';
		out = &str[4];
	}
	else if (strcmp(var, "day") == 0)   /* 2 chars */
	{
		str[10] = '\0';
		out = &str[8];
	}
	else if (strcmp(var, "hour") == 0)  /* 2 chars */
	{
		str[13] = '\0';
		out = &str[11];
	}
	else if (strcmp(var, "min") == 0)   /* 2 chars */
	{
		str[16] = '\0';
		out = &str[14];
	}
	else if (strcmp(var, "sec") == 0)   /* 2 chars */
	{
		str[19] = '\0';
		out = &str[17];
	}
	else if (strcmp(var, "time") == 0)  /* 8 chars */
	{
		str[19] = '\0';
		out = &str[11];
	}
	else if (strcmp(var, "date") == 0)
	{
		str[10] = '\0';
		str[24] = '\0';
		out = &str[4];
		strcat(out, ", ");
		strcat(out, &str[20]);
	}
	else if (strcmp(var, "calctime") == 0)
	{
		get_calculation_time(buf, calctime);
		out = buf;
	}
	else if (strcmp(var, "version") == 0)  /* 4 chars */
	{
		sprintf(buf, "%d", g_release);
		out = buf;
	}
	else if (strcmp(var, "patch") == 0)   /* 1 or 2 chars */
	{
		sprintf(buf, "%d", g_patch_level);
		out = buf;
	}
	else if (strcmp(var, "xdots") == 0)   /* 2 to 4 chars */
	{
		sprintf(buf, "%d", xdots);
		out = buf;
	}
	else if (strcmp(var, "ydots") == 0)   /* 2 to 4 chars */
	{
		sprintf(buf, "%d", ydots);
		out = buf;
	}
	else if (strcmp(var, "vidkey") == 0)   /* 2 to 3 chars */
	{
		char vidmde[5];
		vidmode_keyname(g_video_entry.keynum, vidmde);
		sprintf(buf, "%s", vidmde);
		out = buf;
	}
	else
	{
		char buff[80];
		_snprintf(buff, NUM_OF(buff), "Unknown comment variable %s", var);
		stopmsg(0, buff);
		out = "";
	}
	return out;
}

#define MAXVNAME  13

static const char esc_char = '$';

/* extract comments from the comments= command */
void expand_comments(char *target, char *source)
{
	int i, j, k, escape = 0;
	char c, oldc, varname[MAXVNAME];
	i=j=k = 0;
	c = oldc = 0;
	while (i < MAXCMT && j < MAXCMT && (c = *(source + i++)) != '\0')
	{
		if (c == '\\' && oldc != '\\')
		{
			oldc = c;
			continue;
		}
		/* expand underscores to blanks */
		if (c == '_' && oldc != '\\')
			c = ' ';
		/* esc_char marks start and end of variable names */
		if (c == esc_char && oldc != '\\')
			escape = 1 - escape;
		if (c != esc_char && escape != 0) /* if true, building variable name */
		{
			if (k < MAXVNAME-1)
				varname[k++] = c;
		}
		/* got variable name */
		else if (c == esc_char && escape == 0 && oldc != '\\')
		{
			char buf[100];
			char *varstr;
			varname[k] = 0;
			varstr = expand_var(varname, buf);
			strncpy(target + j, varstr, MAXCMT-j-1);
			j += (int) strlen(varstr);
		}
		else if (c == esc_char && escape != 0 && oldc != '\\')
			k = 0;
		else if ((c != esc_char || oldc == '\\') && escape == 0)
			*(target + j++) = c;
		oldc = c;
	}
	if (*source != '\0')
		*(target + min(j, MAXCMT-1)) = '\0';
}

/* extract comments from the comments= command */
void parse_comments(char *value)
{
	int i;
	char *next, save;
	for (i = 0; i < 4; i++)
	{
		save = '\0';
		if (*value == 0)
			break;
		next = strchr(value, '/');
		if (*value != '/')
		{
			if (next != NULL)
			{
				save = *next;
				*next = '\0';
			}
			strncpy(par_comment[i], value, MAXCMT);
		}
		if (next == NULL)
			break;
		if (save != '\0')
			*next = save;
		value = next + 1;
	}
}

void init_comments()
{
	int i;
	for (i = 0; i < 4; i++)
		par_comment[i][0] = '\0';
}
