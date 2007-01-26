#ifndef EXTERNS_H
#define EXTERNS_H

#if 0
/* #ifndef DEBUG */
#define DEBUG 1
#endif


/* keep var names in column 30 for sorting via sort /+30 <in >out */
extern int                   active_system;						/* = 0 for DOS, = WINFRACT for WinFract */
extern int                   g_adapter;							/* index into g_video_table[] */
extern AlternateMath         alternatemath[];					/* alternate math function pointers */
extern int                   Ambient;							/* Ambient= parameter value */
extern int                   g_and_color;						/* AND mask for iteration to get color index */
extern struct MP             Ans;
extern int                   Ap1deg;
extern int                   AplusOne;
extern int                   askvideo;
extern float                 aspectdrift;
extern int                   attractors;
extern int                   attrperiod[];
extern _CMPLX                attr[];
extern int                   autobrowse;
extern char                  autoname[];
extern char                  autoshowdot;
extern int                   AutoStereo_depth;
extern double                AutoStereo_width;
extern BYTE                  back_color[];
extern int                   g_bad_config;
extern int                   bad_code_count;
extern int                   bad_outside;
extern int                   bad_value;
extern long                  bailout;
extern enum bailouts         bailoutest;
extern int                   basehertz;
extern int                   basin;
extern int                   bf_save_len;
extern int                   bfdigits;
extern int                   biomorph;
extern unsigned int          bits;
extern int                   bitshift;
extern int                   bitshiftless1;
extern BYTE                  block[];
extern int                   blue_bright;
extern int                   blue_crop_left;
extern int                   blue_crop_right;
extern int                   boxcolor;
extern int                   boxcount;
extern int                   boxvalues[];
extern int                   boxx[];
extern int                   boxy[];
extern int                   BRIEF;
extern char                  browsemask[13];
extern char                  browsename[];
extern int                   browsing;
extern char                  brwscheckparms;
extern char                  brwschecktype;
extern char                  busy;
extern long                  calctime;
extern int (*                calctype)(void);
extern int                   calc_status;
extern char                  calibrate;
extern int                   checkcurdir;
extern int                   g_checked_vvs;
extern long                  cimag;
extern double                closenuff;
extern double                closeprox;
extern _CMPLX                coefficient;
extern int                   col;
extern int                   color;
extern char                  colorfile[];
extern long                  coloriter;
extern int                   colorpreloaded;
extern int                   ColorPS;
extern int                   colors;
extern int                   colorstate;
extern int                   g_color_bright;					/* brightest color in palette */
extern int                   g_color_dark;						/* darkest color in palette */
extern int                   g_color_medium;					/* nearest to medbright grey in palette */
extern char                  CommandComment[4][MAXCMT];
extern char                  CommandFile[FILE_MAX_PATH];
extern char                  CommandName[ITEMNAMELEN + 1];
extern int                   comparegif;
extern long                  con;
extern double                cosx;
extern int                   cpu;
extern long                  creal;
extern int                   curcol;
extern int                   curpass;
extern int                   currow;
extern int                   cyclelimit;
extern int                   c_exp;
extern double                d1overd;
extern BYTE                  g_dac_box[256][3];
extern int                   g_dac_count;
extern int                   g_dac_learn;
extern double                ddelmin;
extern int                   debugflag;
extern int                   decimals;
extern BYTE                  decoderline[];
extern int                   decomp[];
extern int                   degree;
extern long                  delmin;
extern long                  delx2;
extern long                  delx;
extern LDBL                  delxx2;
extern LDBL                  delxx;
extern long                  dely2;
extern long                  dely;
extern LDBL                  delyy2;
extern LDBL                  delyy;
extern float                 depthfp;
extern unsigned long         dif_counter;
extern unsigned long         dif_limit;
extern int                   disk16bit;
extern int                   g_disk_flag;						/* disk video active flag */
extern int                   diskisactive;
extern int                   disktarga;
extern int                   display3d;
extern long                  distest;
extern int                   distestwidth;
extern float                 distfp;
extern int                   Distribution;
extern int                   dither_flag;
extern char                  dontreadcolor;
extern int                   dotmode;
extern int                   doublecaution;
extern double                dpx;
extern double                dpy;
extern char                  drawmode;
extern BYTE                  dstack[];
extern U16                   dv_handle;
extern double *				 dx0;
extern double *				 dx1;
extern double (_fastcall *   dxpixel)(void); /* set in FRACTALS.C */
extern double                dxsize;
extern double *				 dy0;
extern double *				 dy1;
extern double (_fastcall *   dypixel)(void); /* set in FRACTALS.C */
extern double                dysize;
extern int                   EPSFileType;
extern int                   escape_exit;
extern BYTE                  exitmode;
extern void *				 extraseg;
extern int                   evolving;
extern U16                   evolve_handle;
extern int                   g_eye_separation;
extern float                 eyesfp;
extern int                   fastrestore;
extern long                  FgHalf;
extern double                fgLimit;
extern long                  FgOne;
extern long                  FgTwo;
extern int                   gridsz;
extern double                fiddlefactor;
extern double                fiddle_reduction;
extern float                 fileaspectratio;
extern int                   filecolors;
extern int                   filetype;
extern int                   filexdots;
extern int                   fileydots;
extern char                  file_name_stack[16][13];
extern int                   fillcolor;
extern float                 finalaspectratio;
extern int                   finattract;
extern int                   finishrow;
extern int                   first_init;
extern char                  floatflag;
extern double                floatmax;
extern double                floatmin;
extern _CMPLX *              floatparm;
extern int                   fm_attack;
extern int                   fm_decay;
extern int                   fm_release;
extern int                   fm_sustain;
extern int                   fm_wavetype;
extern int                   fm_vol; /*volume of OPL-3 soundcard output*/
extern int                   forcesymmetry;
extern char                  FormFileName[];
extern char                  FormName[];
extern int                   fpu;
extern int                   fractype;
extern char *                fract_dir1;
extern char *                fract_dir2;
extern char                  fromtext_flag;
extern long                  fudge;
extern int                   functionpreloaded;
extern double                f_at_rad;
extern double                f_radius;
extern double                f_xcenter;
extern double                f_ycenter;
extern U16                   gene_handle;
extern int                   get_corners(void);
extern int                   gif87a_flag;
extern char                  gifmask[];
extern char                  Glasses1Map[];
extern int                   g_glasses_type;
extern int                   g_good_mode;						/* video mode ok? */
extern int                   g_got_real_dac;					/* loaddac worked, really got a dac */
extern int                   got_status;
extern char                  grayflag;
extern char                  GreyFile[];
extern int                   hasinverse;
extern int                   haze;
extern unsigned int          height;
extern float                 heightfp;
extern int                   helpmode;
extern int                   hi_atten;
extern U16                   history;
extern char                  IFSFileName[];
extern char                  IFSName[];
extern float *           ifs_defn;
extern int                   ifs_type;
extern int                   imgboxcount;
extern U16                   imgboxhandle;
extern char                  image_map;
extern int                   init3d[20];
extern _CMPLX                init;
extern int                   initbatch;
extern int                   initcyclelimit;
extern int                   g_init_mode;
extern _CMPLX                initorbit;
extern int                   initsavetime;
extern int                   inside;
extern char                 insufficient_ifs_mem[];
extern int                   integerfractal;
extern double                inversion[];
extern int                   invert;
extern int                   g_is_true_color;
extern short                 ismand;
extern int                   ixstart;
extern int                   ixstop;
extern int                   iystart;
extern int                   iystop;
extern char *                JIIMleftright[];
extern char *                JIIMmethod[];
extern int                   juli3Dmode;
extern char *                juli3Doptions[];
extern int                   julibrot;
extern int                   kbdcount;
extern int                   keep_scrn_coords;
extern int                   keybuffer;
extern long                  l16triglim;
extern int                   LastInitOp;
extern unsigned              LastOp;
extern int                   lastorbittype;
extern _LCMPLX               lattr[];
extern long                  lclosenuff;
extern _LCMPLX               lcoefficient;
extern int                   ldcheck;
extern char                  LFileName[];
extern char                  light_name[];
extern BYTE *                line_buff;
extern _LCMPLX               linit;
extern _LCMPLX               linitorbit;
extern long                  linitx;
extern long                  linity;
extern long                  llimit2;
extern long                  llimit;
extern long                  lmagnitud;
extern char                  LName[];
extern _LCMPLX               lnew;
extern int                   loaded3d;
extern int                   LodPtr;
extern int                   Log_Auto_Calc;
extern int                   Log_Calc;
extern int                   Log_Fly_Calc;
extern long                  LogFlag;
extern BYTE *            LogTable;
extern _LCMPLX               lold;
extern _LCMPLX *             longparm;
extern int                   lookatmouse;
extern _LCMPLX               lparm2;
extern _LCMPLX               lparm;
extern int                   LPTNumber;
extern long                  ltempsqrx;
extern long                  ltempsqry;
extern _LCMPLX               ltmp;
extern long *            lx0;
extern long *            lx1;
extern long (_fastcall *     lxpixel)(void); /* set in FRACTALS.C */
extern long *            ly0;
extern long *            ly1;
extern long (_fastcall *     lypixel)(void); /* set in FRACTALS.C */
extern int                   lzw[2];
extern long                  l_at_rad;
extern MATRIX                m;
extern double                magnitude;
extern enum Major            major_method;
extern BYTE *            mapdacbox;
extern int                   mapset;
extern char                  MAP_name[];
extern int                   matherr_ct;
extern double                math_tol[2];
extern int                   maxcolor;
extern long                  maxct;
extern char                  maxfn;
extern long                  maxit;
extern int                   maxlinelength;
extern long                  MaxLTSize;
extern unsigned              Max_Args;
extern unsigned              Max_Ops;
extern long                  maxptr;
extern int                   max_colors;
extern int                   max_kbdcount;
extern int                   maxhistory;
extern int                   max_rhombus_depth;
extern int                   minbox;
extern enum Minor            minor_method;
extern int                   minstack;
extern int                   minstackavail;
extern int                   g_mode_7_text;						/* for egamono and hgc */
extern MOREPARAMS            moreparams[];
extern struct MP             mpAp1deg;
extern struct MP             mpAplusOne;
extern struct MPC            MPCone;
extern struct MPC *          MPCroots;
extern struct MPC            mpctmpparm;
extern struct MP             mpd1overd;
extern struct MP             mpone;
extern int                   MPOverflow;
extern struct MP             mproverd;
extern struct MP             mpt2;
extern struct MP             mpthreshold;
extern struct MP             mptmpparm2x;
extern double                mxmaxfp;
extern double                mxminfp;
extern double                mymaxfp;
extern double                myminfp;
extern int                   name_stack_ptr;
extern _CMPLX                g_new;
extern char                  newodpx;
extern char                  newodpy;
extern double                newopx;
extern double                newopy;
extern int                   neworbittype;
extern int                   nextsavedincr;
extern int                   no_sub_images;
extern int                   no_mag_calc;
extern int                   nobof;
extern int                   numaffine;
extern unsigned              numcolors;
extern const int             numtrigfn;
extern int                   num_fractal_types;
extern int                   num_worklist;
extern int                   nxtscreenflag;
extern int                   Offset;
extern int                   g_ok_to_print;
extern _CMPLX                old;
extern long                  oldcoloriter;
extern BYTE                  olddacbox[256][3];
extern U16                   oldhistory_handle;
extern int                   old_demm_colors;
extern char                  old_stdcalcmode;
extern char                  odpx;
extern char                  odpy;
extern double                opx;
extern double                opy;
extern int                   orbitsave;
extern int                   orbit_color;
extern int                   orbit_delay;
extern long                  orbit_interval;
extern int                   orbit_ptr;
extern char                  orgfrmdir[];
extern int                   orgfrmsearch;
extern float                 originfp;
extern int (*                outln) (BYTE *, int);
extern void (*               outln_cleanup) (void);
extern int                   outside;
extern int                   overflow;
extern int                   overlay3d;
extern char                  fract_overwrite;
extern double                ox3rd;
extern double                oxmax;
extern double                oxmin;
extern double                oy3rd;
extern double                oymax;
extern double                oymin;
extern double                param[];
extern double                paramrangex;
extern double                paramrangey;
extern double                parmzoom;
extern _CMPLX                parm2;
extern _CMPLX                parm;
extern int                   passes;
extern int                   g_patch_level;
extern int                   periodicitycheck;
extern struct fls *      pfls;
extern int                   pixelpi;
extern void (_fastcall *     plot)(int,int,int);
extern double                plotmx1;
extern double                plotmx2;
extern double                plotmy1;
extern double                plotmy2;
extern int                   polyphony;
extern unsigned              posp;
extern int                   pot16bit;
extern int                   potflag;
extern double                potparam[];
#ifndef XFRACT
extern U16                   prefix[];
#endif
extern char                  preview;
extern int                   previewfactor;
extern int                   px;
extern int                   py;
extern int                   Printer_BAngle;
extern int                   Printer_BFrequency;
extern int                   Printer_BStyle;
extern int                   Printer_ColorXlat;
extern int                   Printer_Compress;
extern int                   Printer_CRLF;
extern int                   Printer_GAngle;
extern int                   Printer_GFrequency;
extern int                   Printer_GStyle;
extern int                   Printer_RAngle;
extern int                   Printer_Resolution;
extern int                   Printer_RFrequency;
extern int                   Printer_RStyle;
extern int                   Printer_SAngle;
extern int                   Printer_SetScreen;
extern int                   Printer_SFrequency;
extern int                   Printer_SStyle;
extern int                   Printer_Titleblock;
extern int                   Printer_Type;
extern char                  PrintName[];
extern int                   Print_To_File;
extern U16                   prmboxhandle;
extern int                   prmboxcount;
extern int                   pseudox;
extern int                   pseudoy;
extern void (_fastcall *     putcolor)(int,int,int);
extern _CMPLX                pwr;
extern double                qc;
extern double                qci;
extern double                qcj;
extern double                qck;
extern int                   quick_calc;
extern int                   RANDOMIZE;
extern int *             ranges;
extern int                   rangeslen;
extern int                   RAY;
extern char                  ray_name[];
extern char                  readname[];
extern long                  realcoloriter;
extern int                   g_really_ega;						/* "really an EGA" (faking a VGA) flag */
extern char                  recordcolors;
extern int                   red_bright;
extern int                   red_crop_left;
extern int                   red_crop_right;
extern int                   g_release;
extern int                   resave_flag;
extern int                   reset_periodicity;
extern U16                   resume_info;
extern int                   resume_len;
extern int                   resuming;
extern int                   rflag;
extern char                  rlebuf[];
extern int                   rhombus_stack[];
extern int                   root;
extern _CMPLX *              roots;
extern int                   rotate_hi;
extern int                   rotate_lo;
extern double                roverd;
extern int                   row;
extern int                   g_row_count;						/* row-counter for decoder and out_line */
extern double                rqlim2;
extern double                rqlim;
extern int                   rseed;
extern long                  savebase;
extern _CMPLX                SaveC;
extern int                   savedac;
extern char                  savename[];
extern long                  saveticks;
extern int *				 save_orbit;
extern int                   save_release;
extern int                   save_system;
extern int                   scale_map[];
extern float                 screenaspect;
extern char                  scrnfile[];
extern struct SearchPath     searchfor;
extern int                   set_orbit_corners;
extern char                  showbox;
extern int                   showdot;
extern int                   showfile;
extern int                   show_orbit;
extern double                sinx;
extern int                   sizedot;
extern short				 sizeofstring[];
extern short                 skipxdots;
extern short                 skipydots;
extern int                   g_slides;
extern int                   Slope;
extern int                   soundflag;
extern int                   sound_rollover;
extern char                  speed_prompt[];
extern void (_fastcall*      standardplot)(int,int,int);
extern char                  start_showorbit;
extern int                   started_resaves;
extern _CMPLX                staticroots[];
extern char                  stdcalcmode;
extern char                  stereomapname[];
extern int                   StoPtr;
extern int                   stoppass;
extern unsigned int          strlocn[];
extern BYTE                  suffix[];
#if defined(_WIN32)
extern char					 supervga_list[];
#else
extern char                  supervga_list;
#endif
extern int                   g_svga_type;						/* SuperVGA video adapter type */
extern double                sx3rd;
extern int                   sxdots;
extern double                sxmax;
extern double                sxmin;
extern int                   sxoffs;
extern double                sy3rd;
extern int                   sydots;
extern double                symax;
extern double                symin;
extern int                   symmetry;
extern int                   syoffs;
extern char                  s_afi[];
extern char                  s_cantcreate[];
extern char                  s_cantfind[];
extern char                  s_cantunderstand[];
extern char                  s_cantwrite[];
extern char                  s_makepar[];
extern char                  s_no[];
extern char                  s_numframes[];
extern char                  s_targa_out[];
extern char                  s_textsafe[];
extern char                  s_vesadetect[];
extern char                  s_x[];
extern char                  s_yes[];
extern char                  s_y[];
extern char                  s_z[];
extern int                   tabmode;
extern int                   taborhelp;
extern int                   Targa_Out;
extern int                   Targa_Overlay;
extern char                  temp1[];
extern double                tempsqrx;
extern double                tempsqry;
extern BYTE                  teststring[];
extern int                   g_text_cbase;						/* g_text_col is relative to this */
extern int                   g_text_col;						/* current column in text mode */
extern int                   g_text_rbase;						/* g_text_row is relative to this */
extern int                   g_text_row;						/* current row in text mode */
extern int                   g_text_type;						/* current mode's type of text */
extern unsigned int          this_gen_rseed;
extern unsigned *        tga16;
extern long *            tga32;
extern char                  three_pass;
extern double                threshold;
extern int                   timedsave;
extern int                   timerflag;
extern long                  timer_interval;
extern long                  timer_start;
extern _CMPLX                tmp;
extern char                  tempdir[];
extern double                toosmall;
extern int                   totpasses;
extern long                  total_formula_mem;
extern int                   transparent[];
extern BYTE                  trigndx[];
extern int                   truecolor;
extern int                   truemode;
extern char                  tstack[];
extern double                twopi;
extern VOIDPTR               typespecific_workarea;
extern char                  useinitorbit;
extern BYTE                  used_extra;
extern int                   use_grid;	
extern BYTE                  usemag;
extern short                 uses_ismand;
extern short                 uses_p1;
extern short                 uses_p2;
extern short                 uses_p3;
extern short                 uses_p4;
extern short                 uses_p5;
extern int                   use_old_distest;
extern int                   use_old_period;
extern int                   using_jiim;
extern int                   usr_biomorph;
extern long                  usr_distest;
extern char                  usr_floatflag;
extern int                   usr_periodicitycheck;
extern char                  usr_stdcalcmode;
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern struct videoinfo      g_video_entry;
extern VIDEOINFO             g_video_table[];
extern int					 g_video_table_len;
extern int                   video_cutboth;
extern int                   g_video_scroll;
extern int                   g_video_start_x;
extern int                   g_video_start_y;
extern int                   g_video_type;						/* video adapter type */
extern int                   g_video_vram;
extern VECTOR                view;
extern int                   viewcrop;
extern float                 viewreduction;
extern int                   viewwindow;
extern int                   viewxdots;
extern int                   viewydots;
extern int                   g_virtual_screens;
extern unsigned              vsp;
extern int                   g_vxdots;
extern int                   g_which_image;
extern float                 widthfp;
extern char                  workdir[];
extern WORKLIST              worklist[MAXCALCWORK];
extern int                   workpass;
extern int                   worksym;
extern long                  x3rd;
extern int                   xadjust;
extern double                xcjul;
extern int                   xdots;
extern long                  xmax;
extern long                  xmin;
extern int                   xshift1;
extern int                   xshift;
extern int                   xtrans;
extern double                xx3rd;
extern int                   xxadjust1;
extern int                   xxadjust;
extern double                xxmax;
extern double                xxmin;
extern long                  XXOne;
extern int                   xxstart;
extern int                   xxstop;
extern long                  y3rd;
extern int                   yadjust;
extern double                ycjul;
extern int                   ydots;
extern long                  ymax;
extern long                  ymin;
extern int                   yshift1;
extern int                   yshift;
extern int                   ytrans;
extern double                yy3rd;
extern int                   yyadjust1;
extern int                   yyadjust;
extern int                   yyadjust;
extern double                yymax;
extern double                yymin;
extern int                   yystart;
extern int                   yystop;
extern double                zbx;
extern double                zby;
extern double                zdepth;
extern int                   zdots;
extern int                   zoomoff;
extern int                   zrotate;
extern int                   zscroll;
extern double                zskew;
extern double                zwidth;

#ifdef XFRACT
extern  int                  fake_lut;
#endif

#endif
