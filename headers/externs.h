#ifndef EXTERNS_H
#define EXTERNS_H
#include <string>
#include <vector>

struct AlternateMath;
enum class bailouts;
enum class batch_modes;
enum class calc_status_value;
struct DComplex;
enum display_3d_modes;
struct EVOLUTION_INFO;
enum class fractal_type;
struct GENEBASE;
struct LComplex;
enum class Major;
enum class Minor;
struct MOREPARAMS;
struct MP;
struct MPC;
namespace id
{
struct SearchPath;
}
enum class slides_mode;
enum class stereo_images;
enum class symmetry_type;
struct VIDEOINFO;
struct WORKLIST;

// keep var names in column 30 for sorting via sort /+30 <in >out
extern int                   g_adapter;             // index into g_video_table[]
extern AlternateMath         g_alternate_math[];    // alternate math function pointers
extern int                   g_ambient;             // Ambient= parameter value
extern int                   g_and_color;           // AND mask for iteration to get color index
extern int                   g_halley_a_plus_one_times_degree;
extern int                   g_halley_a_plus_one;
extern bool                  g_ask_video;
extern float                 g_aspect_drift;
extern int                   g_attractors;
extern int                   g_attractor_period[];
extern DComplex              g_attractor[];
extern bool                  g_auto_browse;
extern std::string           g_auto_name;
extern char                  g_auto_show_dot;
extern int                   g_auto_stereo_depth;
extern double                g_auto_stereo_width;
extern BYTE                  g_background_color[];
extern int                   g_bad_config;
extern bool                  g_bad_outside;
extern int const             g_bad_value;
extern long                  g_bail_out;
extern bailouts              g_bail_out_test;
extern int                   g_base_hertz;
extern int                   g_basin;
extern int                   g_bf_save_len;
extern int                   g_bf_digits;
extern int                   g_biomorph;
extern unsigned int          g_diffusion_bits;
extern int                   bitshift;
extern int                   bitshiftless1;
extern BYTE                  g_block[];
extern int                   g_blue_bright;
extern int                   g_blue_crop_left;
extern int                   g_blue_crop_right;
extern int                   g_box_color;
extern int                   g_box_count;
extern int                   g_box_values[];
extern int                   g_box_x[];
extern int                   g_box_y[];
extern bool                  g_brief;
extern std::string           g_browse_mask;
extern std::string           g_browse_name;
extern bool                  g_browsing;
extern bool                  g_browse_check_fractal_params;
extern bool                  g_browse_check_fractal_type;
extern bool                  g_busy;
extern long                  g_calc_time;
extern int                 (*calctype)();
extern calc_status_value     g_calc_status;
extern char                  g_calibrate;
extern bool                  g_check_cur_dir;
extern double                g_close_enough;
extern double                g_close_proximity;
extern DComplex              g_marks_coefficient;
extern int                   col;
extern int                   g_color;
extern std::string           g_color_file;
extern long                  g_color_iter;
extern bool                  g_colors_preloaded;
extern int                   g_colors;
extern int                   g_color_state;
extern int                   g_color_bright;    // brightest color in palette
extern int                   g_color_dark;      // darkest color in palette
extern int                   g_color_medium;    // nearest to medbright grey in palette
extern std::string           g_command_comment[4];
extern std::string           g_command_file;
extern std::string           g_command_name;
extern bool                  g_compare_gif;
extern long                  con;
extern double                cosx;
extern int                   g_current_column;
extern int                   g_current_pass;
extern int                   g_current_row;
extern int                   g_cycle_limit;
extern int                   g_c_exponent;
extern double                g_degree_minus_1_over_degree;
extern BYTE                  g_dac_box[256][3];
extern int                   g_dac_count;
extern bool                  g_dac_learn;
extern double                ddelmin;
extern int                   g_debug_flag;
extern int                   g_decimals;
extern int                   g_decomp[];
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
extern float                 g_depth_fp;
extern unsigned long         g_diffusion_counter;
extern unsigned long         g_diffusion_limit;
extern bool                  g_disk_16_bit;
extern bool                  g_disk_flag;       // disk video active flag
extern bool                  g_disk_targa;
extern display_3d_modes      g_display_3d;
extern long                  g_distance_estimator;
extern int                   g_distance_estimator_width_factor;
extern float                 g_dist_fp;
extern int                   g_distribution;
extern bool                  g_dither_flag;
extern bool                  g_read_color;
extern int                   g_dot_mode;
extern bool                  g_confirm_file_deletes;
extern double                g_evolve_dist_per_x;
extern double                g_evolve_dist_per_y;
extern char                  g_draw_mode;
extern std::vector<double>   dx0;
extern std::vector<double>   dx1;
extern double              (*dxpixel)();
extern double                x_size_d;
extern std::vector<double>   dy0;
extern std::vector<double>   dy1;
extern double              (*dypixel)();
extern double                y_size_d;
extern bool                  g_escape_exit;
extern BYTE                  g_exit_video_mode;
extern int                   g_evolving;
extern bool                  g_have_evolve_info;
extern EVOLUTION_INFO        g_evolve_info;
extern int                   g_eye_separation;
extern float                 g_eyes_fp;
extern bool                  g_fast_restore;
extern long                  g_fudge_half;
extern double                g_fudge_limit;
extern long                  g_fudge_one;
extern long                  g_fudge_two;
extern int                   g_evolve_image_grid_size;
extern double                g_evolve_max_random_mutation;
extern double                g_evolve_mutation_reduction_factor;
extern float                 g_file_aspect_ratio;
extern int                   g_file_colors;
extern int                   g_file_x_dots;
extern int                   fileydots;
extern std::string           file_name_stack[16];
extern int                   fillcolor;
extern float                 finalaspectratio;
extern bool                  finattract;
extern int                   finishrow;
extern bool                  first_init;
extern bool                  floatflag;
extern double                floatmax;
extern double                floatmin;
extern DComplex *            floatparm;
extern int                   fm_attack;
extern int                   fm_decay;
extern int                   fm_release;
extern int                   fm_sustain;
extern int                   fm_wavetype;
extern int                   fm_vol;            // volume of OPL-3 soundcard output
extern symmetry_type         forcesymmetry;
extern std::string           FormFileName;
extern std::string           FormName;
extern char const *          fract_dir1;
extern char const *          fract_dir2;
extern long                  fudge;
extern bool new_bifurcation_functions_loaded;
extern double                f_at_rad;
extern double                f_radius;
extern double                f_xcenter;
extern double                f_ycenter;
extern GENEBASE              gene_bank[NUMGENES];
extern int                   get_corners();
extern bool                  gif87a_flag;
extern std::string           gifmask;
extern std::string const     Glasses1Map;
extern int                   g_glasses_type;
extern bool                  g_good_mode;       // video mode ok?
extern bool                  g_got_real_dac;    // loaddac worked, really got a dac
extern int                   got_status;
extern bool                  grayflag;
extern std::string const     GreyFile;
extern bool                  hasinverse;
extern int                   haze;
extern unsigned int          height;
extern float                 heightfp;
extern int help_mode;
extern int                   hi_atten;
extern std::string           IFSFileName;
extern std::string           IFSName;
extern std::vector<float>    ifs_defn;
extern bool                  ifs_type;
extern int image_box_count;
extern bool                  image_map;
extern int                   init3d[20];
extern DComplex              init;
extern batch_modes init_batch;
extern int                   initcyclelimit;
extern int                   g_init_mode;
extern DComplex              initorbit;
extern int                   initsavetime;
extern int                   inside;
extern int                   integerfractal;
extern double                inversion[];
extern int                   invert;
extern bool                  g_is_true_color;
extern bool                  ismand;
extern int                   ixstart;
extern int                   ixstop;
extern int                   iystart;
extern int                   iystop;
extern std::string const     JIIMleftright[];
extern std::string const     JIIMmethod[];
extern int                   juli3Dmode;
extern std::string const     juli3Doptions[];
extern bool                  julibrot;
extern int keyboard_check_interval;
extern bool                  keep_scrn_coords;
extern long                  l16triglim;
extern int                   LastInitOp;
extern unsigned              LastOp;
extern int                   lastorbittype;
extern LComplex              lattr[];
extern long                  lclosenuff;
extern LComplex              lcoefficient;
extern bool                  ldcheck;
extern std::string           LFileName;
extern std::string           light_name;
extern std::vector<BYTE>     line_buff;
extern LComplex              linit;
extern LComplex              linitorbit;
extern long                  linitx;
extern long                  linity;
extern long                  llimit2;
extern long                  llimit;
extern long                  lmagnitud;
extern std::string           LName;
extern LComplex              lnew;
extern bool                  loaded3d;
extern int                   LodPtr;
extern bool                  Log_Auto_Calc;
extern bool                  Log_Calc;
extern int                   Log_Fly_Calc;
extern long                  LogFlag;
extern std::vector<BYTE>     LogTable;
extern LComplex              lold;
extern LComplex *            longparm;
extern int look_at_mouse;
extern LComplex              lparm2;
extern LComplex              lparm;
extern long                  ltempsqrx;
extern long                  ltempsqry;
extern LComplex              ltmp;
extern std::vector<long>     lx0;
extern std::vector<long>     lx1;
extern long                (*lxpixel)();
extern std::vector<long>     ly0;
extern std::vector<long>     ly1;
extern long                (*lypixel)();
extern int                   lzw[2];
extern long                  l_at_rad;
extern MATRIX                m;
extern double                magnitude;
extern Major                 major_method;
extern bool map_specified;
extern BYTE map_clut[256][3];
extern bool                  mapset;
extern std::string           MAP_name;
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
extern int max_keyboard_check_interval;
extern int                   maxhistory;
extern int                   max_rhombus_depth;
extern int smallest_box_size_shown;
extern Minor                 minor_method;
extern int                   minstack;
extern int                   minstackavail;
extern MOREPARAMS            moreparams[];
extern MP                    mpAp1deg;
extern MP                    mpAplusOne;
extern MPC                   MPCone;
extern std::vector<MPC>      MPCroots;
extern MPC                   mpctmpparm;
extern MP                    mpd1overd;
extern MP                    mpone;
extern int                   MPOverflow;
extern MP                    mproverd;
extern MP                    mpt2;
extern MP                    mpthreshold;
extern MP                    mptmpparm2x;
extern double                mxmaxfp;
extern double                mxminfp;
extern double                mymaxfp;
extern double                myminfp;
extern int                   name_stack_ptr;
extern DComplex              g_new;
extern char evolve_new_discrete_x_parameter_offset;
extern char evolve_new_discrete_y_parameter_offset;
extern double evolve_new_x_parameter_offset;
extern double evolve_new_y_parameter_offset;
extern fractal_type          neworbittype;
extern int                   nextsavedincr;
extern bool                  no_sub_images;
extern bool                  no_mag_calc;
extern bool                  nobof;
extern int                   numaffine;
extern unsigned              numcolors;
extern const int             numtrigfn;
extern int                   num_fractal_types;
extern int                   num_worklist;
extern bool                  nxtscreenflag;
extern int                   Offset;
extern DComplex              old;
extern long                  oldcoloriter;
extern BYTE                  old_dac_box[256][3];
extern bool                  old_demm_colors;
extern char                  old_stdcalcmode;
extern char evolve_discrete_x_parameter_offset;
extern char evolve_discrete_y_parameter_offset;
extern double evolve_x_parameter_offset;
extern double evolve_y_parameter_offset;
extern int                   orbitsave;
extern int                   orbit_color;
extern int                   orbit_delay;
extern long                  orbit_interval;
extern int                   orbit_ptr;
extern std::string           orgfrmdir;
extern bool                  orgfrmsearch;
extern float                 originfp;
extern int                 (*outln)(BYTE *, int);
extern void                (*outln_cleanup)();
extern int                   outside;
extern bool                  overflow;
extern bool overlay_3d;
extern bool                  fract_overwrite;
extern double                ox3rd;
extern double                oxmax;
extern double                oxmin;
extern double                oy3rd;
extern double                oymax;
extern double                oymin;
extern double                param[];
extern double evolve_x_parameter_range;
extern double evolve_y_parameter_range;
extern double                parmzoom;
extern DComplex              parm2;
extern DComplex              parm;
extern int                   passes;
extern int                   g_patch_level;
extern int                   periodicitycheck;
struct fls;
extern std::vector<fls>      pfls;
extern int                   pixelpi;
extern void                (*plot)(int, int, int);
extern double                plotmx1;
extern double                plotmx2;
extern double                plotmy1;
extern double                plotmy2;
extern int                   polyphony;
extern unsigned              posp;
extern bool                  pot16bit;
extern bool                  potflag;
extern double                potparam[];
extern bool                  preview;
extern int                   previewfactor;
extern int                   px;
extern int                   py;
extern int param_box_count;
extern int                   pseudox;
extern int                   pseudoy;
extern void                (*putcolor)(int, int, int);
extern DComplex              pwr;
extern double                qc;
extern double                qci;
extern double                qcj;
extern double                qck;
extern bool                  quick_calc;
extern int                   RANDOMIZE;
extern std::vector<int>      ranges;
extern int                   rangeslen;
extern int                   RAY;
extern std::string           ray_name;
extern std::string           readname;
extern long                  realcoloriter;
extern char                  recordcolors;
extern int                   red_bright;
extern int                   red_crop_left;
extern int                   red_crop_right;
extern int                   g_release;
extern int                   resave_flag;
extern bool                  reset_periodicity;
extern std::vector<BYTE>     resume_data;
extern int                   resume_len;
extern bool                  resuming;
extern bool                  rflag;
extern int                   rhombus_stack[];
extern int                   root;
extern std::vector<DComplex> roots;
extern int                   rotate_hi;
extern int                   rotate_lo;
extern double                roverd;
extern int                   row;
extern int                   g_row_count;       // row-counter for decoder and out_line
extern double                rqlim2;
extern double                rqlim;
extern int                   rseed;
extern long                  savebase;
extern DComplex              SaveC;
extern int                   savedac;
extern std::string           savename;
extern long                  saveticks;
extern int                   save_release;
extern int                   save_system;
extern int                   scale_map[];
extern float                 screenaspect;
extern id::SearchPath        searchfor;
extern bool                  set_orbit_corners;
extern bool                  showbox;
extern int show_dot;
extern int show_file;
extern bool                  show_orbit;
extern double                sinx;
extern int                   sizedot;
extern short                 skipxdots;
extern short                 skipydots;
extern slides_mode           g_slides;
extern int                   Slope;
extern int                   soundflag;
extern std::string const     speed_prompt;
extern void                (*standardplot)(int, int, int);
extern bool start_show_orbit;
extern bool                  started_resaves;
extern char                  stdcalcmode;
extern std::string           stereomapname;
extern int                   StoPtr;
extern int                   stoppass;
extern double                sx3rd;
extern int                   sxdots;
extern double                sxmax;
extern double                sxmin;
extern int                   sxoffs;
extern double                sy3rd;
extern int                   sydots;
extern double                symax;
extern double                symin;
extern symmetry_type         symmetry;
extern int                   syoffs;
extern bool make_parameter_file;
extern bool make_parameter_file_map;
extern bool tab_mode;
extern bool                  taborhelp;
extern bool                  Targa_Out;
extern bool                  Targa_Overlay;
extern double                tempsqrx;
extern double                tempsqry;
extern int                   g_text_cbase;      // g_text_col is relative to this
extern int                   g_text_col;        // current column in text mode
extern int                   g_text_rbase;      // g_text_row is relative to this
extern int                   g_text_row;        // current row in text mode
extern unsigned int evolve_this_generation_random_seed;
extern unsigned *            tga16;
extern long *                tga32;
extern bool                  three_pass;
extern double                threshold;
extern int                   timedsave;
extern bool                  timerflag;
extern long                  timer_interval;
extern long                  timer_start;
extern DComplex              tmp;
extern std::string           tempdir;
extern double smallest_window_display_size;
extern int                   totpasses;
extern int                   transparent[];
extern bool                  truecolor;
extern int                   truemode;
extern double                twopi;
extern char                  useinitorbit;
extern bool                  use_grid;
extern bool                  usemag;
extern bool                  uses_ismand;
extern bool                  uses_p1;
extern bool                  uses_p2;
extern bool                  uses_p3;
extern bool                  uses_p4;
extern bool                  uses_p5;
extern bool                  use_old_distest;
extern bool                  use_old_period;
extern bool                  using_jiim;
extern int                   usr_biomorph;
extern long                  usr_distest;
extern bool                  usr_floatflag;
extern int                   usr_periodicitycheck;
extern char                  usr_stdcalcmode;
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern VIDEOINFO             g_video_entry;
extern VIDEOINFO             g_video_table[];
extern int                   g_video_table_len;
extern bool                  video_cutboth;
extern bool                  g_video_scroll;
extern int                   g_video_start_x;
extern int                   g_video_start_y;
extern int                   g_video_type;      // video adapter type
extern VECTOR                view;
extern bool                  viewcrop;
extern float                 viewreduction;
extern bool                  viewwindow;
extern int                   viewxdots;
extern int                   viewydots;
extern bool                  g_virtual_screens;
extern unsigned              vsp;
extern int                   g_vxdots;
extern stereo_images         g_which_image;
extern float                 widthfp;
extern std::string           workdir;
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
extern double                zoom_box_height;
extern int                   zdots;
extern bool                  zoomoff;
extern int                   zoom_box_rotation;
extern bool                  zscroll;
extern double                zoom_box_skew;
extern double                zoom_box_width;
#if defined(XFRACT)
extern bool                  fake_lut;
extern bool                  XZoomWaiting;
#endif
#endif
