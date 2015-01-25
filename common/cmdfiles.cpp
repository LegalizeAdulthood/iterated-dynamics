/*
        Command-line / Command-File Parser Routines
*/
#include <algorithm>
#include <cassert>
#include <string>
#include <system_error>
#include <vector>

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <printf.h>
#include <stdio.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"
#include "helpcom.h"

#ifdef XFRACT
#define DEFAULT_PRINTER 5       // Assume a Postscript printer
#define PRT_RESOLUTION  100     // Assume medium resolution
#else
#define DEFAULT_PRINTER 2       // Assume an IBM/Epson printer
#define PRT_RESOLUTION  60      // Assume low resolution
#endif

static int  cmdfile(FILE *handle, cmd_file mode);
static int  next_command(
    char *cmdbuf,
    int maxlen,
    FILE *handle,
    char *linebuf,
    int *lineoffset,
    cmd_file mode);
static bool next_line(FILE *handle, char *linebuf, cmd_file mode);
int cmdarg(char *argument, cmd_file mode);
static void argerror(char const *);
static void initvars_run();
static void initvars_restart();
static void initvars_fractal();
static void initvars_3d();
static void reset_ifs_defn();
static void parse_textcolors(char const *value);
static int  parse_colors(char const *value);
static int  get_bf(bf_t bf, char const *curarg);
static bool isabigfloat(char const *str);

// variables defined by the command line/files processor
int     stoppass = 0;           // stop at this guessing pass early
int     pseudox = 0;            // xdots to use for video independence
int     pseudoy = 0;            // ydots to use for video independence
int     bfdigits = 0;           // digits to use (force) for bf_math
int     show_dot = -1;           // color to show crawling graphics cursor
int     sizedot = 0;            // size of dot crawling cursor
char    recordcolors = 0;       // default PAR color-writing method
char    autoshowdot = 0;        // dark, medium, bright
bool    start_show_orbit = false;        // show orbits on at start of fractal
std::string readname;           // name of fractal input file
std::string tempdir;            // name of temporary directory
std::string workdir;            // name of directory for misc files
std::string orgfrmdir;          // name of directory for orgfrm files
std::string gifmask;
char    PrintName[FILE_MAX_PATH] = {"fract001.prn"}; // Name for print-to-file
std::string savename{"fract001"}; // save files using this name
std::string autoname{"auto.key"}; // record auto keystrokes here
bool    potflag = false;        // continuous potential enabled?
bool    pot16bit = false;               // store 16 bit continuous potential values
bool    gif87a_flag = false;    // true if GIF87a format, false otherwise
bool    dither_flag = false;    // true if want to dither GIFs
bool    askvideo = false;       // flag for video prompting
bool    floatflag = false;
int     biomorph = 0;           // flag for biomorph
int     usr_biomorph = 0;
symmetry_type forcesymmetry = symmetry_type::NONE;      // force symmetry
int     show_file = 0;           // zero if file display pending
bool    rflag = false;
int     rseed = 0;              // Random number seeding flag and value
int     decomp[2] = { 0 };      // Decomposition coloring
long    distest = 0;
int     distestwidth = 0;
bool    fract_overwrite = false;// true if file overwrite allowed
int     soundflag = 0;          // sound control bitfield... see sound.c for useage
int     basehertz = 0;          // sound=x/y/x hertz value
int     debugflag = debug_flags::none; // internal use only - you didn't see this
bool    timerflag = false;      // you didn't see this, either
int     cyclelimit = 0;         // color-rotator upper limit
int     inside = 0;             // inside color: 1=blue
int     fillcolor = 0;          // fillcolor: -1=normal
int     outside = COLOR_BLACK;  // outside color
bool finattract = false;        // finite attractor logic
display_3d_modes display_3d = display_3d_modes::NONE; // 3D display flag: 0 = OFF
bool    overlay_3d = false;      // 3D overlay flag
int     init3d[20] = { 0 };     // '3d=nn/nn/nn/...' values
bool    checkcurdir = false;    // flag to check current dir for files
batch_modes init_batch = batch_modes::NONE; // 1 if batch run (no kbd)
int     initsavetime = 0;       // autosave minutes
DComplex  initorbit = { 0.0 };  // initial orbitvalue
char    useinitorbit = 0;       // flag for initorbit
int     g_init_mode = 0;        // initial video mode
int     initcyclelimit = 0;     // initial cycle limit
bool    usemag = false;         // use center-mag corners
long    bailout = 0;            // user input bailout value
bailouts bailoutest;            // test used for determining bailout
double  inversion[3] = { 0.0 }; // radius, xcenter, ycenter
int     rotate_lo = 0;
int     rotate_hi = 0;          // cycling color range
std::vector<int> ranges;        // iter->color ranges mapping
int     rangeslen = 0;          // size of ranges array
BYTE map_clut[256][3];          // map= (default colors)
bool map_specified = false;     // map= specified
BYTE *mapdacbox = nullptr;      // map= (default colors)
int     colorstate = 0;         // 0, g_dac_box matches default (bios or map=)
                                // 1, g_dac_box matches no known defined map
                                // 2, g_dac_box matches the colorfile map
bool    colors_preloaded = false; // if g_dac_box preloaded for next mode select
int     save_release = 0;       // release creating PAR file
bool    dontreadcolor = false;  // flag for reading color from GIF
double  math_tol[2] = {.05, .05}; // For math transition
bool Targa_Out = false;                 // 3D fullcolor flag
bool truecolor = false;                 // escape time truecolor flag
int truemode = 0;               // truecolor coloring scheme
std::string colorfile;          // from last <l> <s> or colors=@filename
bool new_bifurcation_functions_loaded = false; // if function loaded for new bifs
float   screenaspect = DEFAULTASPECT;   // aspect ratio of the screen
float   aspectdrift = DEFAULTASPECTDRIFT;  // how much drift is allowed and
                                // still forced to screenaspect
bool fastrestore = false;       /* true - reset viewwindows prior to a restore
                                     and do not display warnings when video
                                     mode changes during restore */

bool orgfrmsearch = false;      /* 1 - user has specified a directory for
                                     Orgform formula compilation files */

int     orbitsave = 0;          // for IFS and LORENZ to output acrospin file
int orbit_delay = 0;            // clock ticks delating orbit release
int     transparent[2] = { 0 }; // transparency min/max values
long    LogFlag = 0;            // Logarithmic palette flag: 0 = no

BYTE exitmode = 3;              // video mode on exit

int     Log_Fly_Calc = 0;       // calculate logmap on-the-fly
bool    Log_Auto_Calc = false;          // auto calculate logmap
bool    nobof = false;                  // Flag to make inside=bof options not duplicate bof images

bool    escape_exit = false;    // set to true to avoid the "are you sure?" screen
bool first_init = true;                 // first time into cmdfiles?
static int init_rseed = 0;
static bool initcorners = false;
static bool initparams = false;
fractalspecificstuff *curfractalspecific = nullptr;

std::string FormFileName;               // file to find (type=)formulas in
std::string FormName;                   // Name of the Formula (if not null)
std::string LFileName;                  // file to find (type=)L-System's in
std::string LName;                      // Name of L-System
std::string CommandFile;                // file to find command sets in
std::string CommandName;                // Name of Command set
std::string CommandComment[4];          // comments for command set
std::string IFSFileName;                // file to find (type=)IFS in
std::string IFSName;                    // Name of the IFS def'n (if not null)
SearchPath searchfor = { 0 };
std::vector<float> ifs_defn;            // ifs parameters
bool ifs_type = false;                  // false=2d, true=3d
slides_mode g_slides = slides_mode::OFF; // PLAY autokey=play, RECORD autokey=record

BYTE txtcolor[] = {
    BLUE*16+L_WHITE,    // C_TITLE           title background
    BLUE*16+L_GREEN,    // C_TITLE_DEV       development vsn foreground
    GREEN*16+YELLOW,    // C_HELP_HDG        help page title line
    WHITE*16+BLACK,     // C_HELP_BODY       help page body
    GREEN*16+GRAY,      // C_HELP_INSTR      help page instr at bottom
    WHITE*16+BLUE,      // C_HELP_LINK       help page links
    CYAN*16+BLUE,       // C_HELP_CURLINK    help page current link
    WHITE*16+GRAY,      // C_PROMPT_BKGRD    prompt/choice background
    WHITE*16+BLACK,     // C_PROMPT_TEXT     prompt/choice extra info
    BLUE*16+WHITE,      // C_PROMPT_LO       prompt/choice text
    BLUE*16+L_WHITE,    // C_PROMPT_MED      prompt/choice hdg2/...
    BLUE*16+YELLOW,     // C_PROMPT_HI       prompt/choice hdg/cur/...
    GREEN*16+L_WHITE,   // C_PROMPT_INPUT    fullscreen_prompt input
    CYAN*16+L_WHITE,    // C_PROMPT_CHOOSE   fullscreen_prompt choice
    MAGENTA*16+L_WHITE, // C_CHOICE_CURRENT  fullscreen_choice input
    BLACK*16+WHITE,     // C_CHOICE_SP_INSTR speed key bar & instr
    BLACK*16+L_MAGENTA, // C_CHOICE_SP_KEYIN speed key value
    WHITE*16+BLUE,      // C_GENERAL_HI      tab, thinking, IFS
    WHITE*16+BLACK,     // C_GENERAL_MED
    WHITE*16+GRAY,      // C_GENERAL_LO
    BLACK*16+L_WHITE,   // C_GENERAL_INPUT
    WHITE*16+BLACK,     // C_DVID_BKGRD      disk video
    BLACK*16+YELLOW,    // C_DVID_HI
    BLACK*16+L_WHITE,   // C_DVID_LO
    RED*16+L_WHITE,     // C_STOP_ERR        stop message, error
    GREEN*16+BLACK,     // C_STOP_INFO       stop message, info
    BLUE*16+WHITE,      // C_TITLE_LOW       bottom lines of title screen
    GREEN*16+BLACK,     // C_AUTHDIV1        title screen dividers
    GREEN*16+GRAY,      // C_AUTHDIV2        title screen dividers
    BLACK*16+L_WHITE,   // C_PRIMARY         primary authors
    BLACK*16+WHITE      // C_CONTRIB         contributing authors
};

int lzw[2] = { 0 };

/*
        cmdfiles(argc,argv) process the command-line arguments
                it also processes the 'sstools.ini' file and any
                indirect files ('fractint @myfile')
*/

// This probably ought to go somewhere else, but it's used here.
// getpower10(x) returns the magnitude of x.  This rounds
// a little so 9.95 rounds to 10, but we're using a binary base anyway,
// so there's nothing magic about changing to the next power of 10.
int getpower10(LDBL x)
{
    char string[11]; // space for "+x.xe-xxxx"
    int p;

    sprintf(string, "%+.1Le", x);
    p = atoi(string+5);
    return p;
}

namespace
{
std::string findpath(char const *filename)
{
    char buffer[FILE_MAX_PATH] = { 0 };
    ::findpath(filename, buffer);
    return buffer;
}

void process_sstools_ini()
{
    std::string const sstools_ini = findpath("sstools.ini"); // look for SSTOOLS.INI
    if (!sstools_ini.empty())              // found it!
    {
        FILE *initfile = fopen(sstools_ini.c_str(), "r");
        if (initfile != nullptr)
        {
            cmdfile(initfile, cmd_file::SSTOOLS_INI);           // process it
        }
    }
}

void process_simple_command(char *curarg)
{
    bool processed = false;
    if (strchr(curarg, '=') == nullptr)
    {
        // not xxx=yyy, so check for gif
        std::string filename = curarg;
        if (has_ext(curarg) == nullptr)
        {
            filename += ".gif";
        }
        if (FILE *initfile = fopen(filename.c_str(), "rb"))
        {
            char tempstring[101];
            if (fread(tempstring, 6, 1, initfile) != 6)
            {
                throw std::system_error(errno, std::system_category(), "process_simple_command failed fread");
            }
            if (tempstring[0] == 'G'
                    && tempstring[1] == 'I'
                    && tempstring[2] == 'F'
                    && tempstring[3] >= '8' && tempstring[3] <= '9'
                    && tempstring[4] >= '0' && tempstring[4] <= '9')
            {
                readname = curarg;
                browse_name = extract_filename(readname.c_str());
                show_file = 0;
                processed = true;
            }
            fclose(initfile);
        }
    }
    if (!processed)
    {
        cmdarg(curarg, cmd_file::AT_CMD_LINE);           // process simple command
    }
}

void process_file_setname(char *curarg, char *sptr)
{
    *sptr = 0;
    if (merge_pathnames(CommandFile, &curarg[1], cmd_file::AT_CMD_LINE) < 0)
        init_msg("", CommandFile.c_str(), cmd_file::AT_CMD_LINE);
    CommandName = &sptr[1];
    FILE *initfile = nullptr;
    if (find_file_item(CommandFile, CommandName.c_str(), &initfile, 0) || initfile == nullptr)
        argerror(curarg);
    cmdfile(initfile, cmd_file::AT_CMD_LINE_SET_NAME);
}

void process_file(char *curarg)
{
    FILE *initfile = fopen(&curarg[1], "r");
    if (initfile == nullptr)
        argerror(curarg);
    cmdfile(initfile, cmd_file::AT_CMD_LINE);
}

}

int cmdfiles(int argc, char const *const *argv)
{
    if (first_init)
    {
        initvars_run();                 // once per run initialization
    }
    initvars_restart();                  // <ins> key initialization
    initvars_fractal();                  // image initialization

    process_sstools_ini();

    // cycle through args
    for (int i = 1; i < argc; i++)
    {
        char curarg[141];
        strcpy(curarg, argv[i]);
        if (curarg[0] == ';')             // start of comments?
        {
            break;
        }
        if (curarg[0] != '@')
        {
            process_simple_command(curarg);
        }
        // @filename/setname?
        else if (char *sptr = strchr(curarg, '/'))
        {
            process_file_setname(curarg, sptr);
        }
        // @filename
        else
        {
            process_file(curarg);
        }
    }

    if (!first_init)
    {
        g_init_mode = -1; // don't set video when <ins> key used
        show_file = 1;  // nor startup image file
    }

    init_msg("", nullptr, cmd_file::AT_CMD_LINE);  // this causes driver_get_key if init_msg called on runup

    if (debugflag != debug_flags::allow_init_commands_anytime)
    {
        first_init = false;
    }

    // PAR reads a file and sets color, don't read colors from GIF
    dontreadcolor = colors_preloaded && show_file == 0;

    //set structure of search directories
    strcpy(searchfor.par, CommandFile.c_str());
    strcpy(searchfor.frm, FormFileName.c_str());
    strcpy(searchfor.lsys, LFileName.c_str());
    strcpy(searchfor.ifs, IFSFileName.c_str());
    return 0;
}


int load_commands(FILE *infile)
{
    // when called, file is open in binary mode, positioned at the
    // '(' or '{' following the desired parameter set's name
    int ret;
    initcorners = false;
    initparams = false; // reset flags for type=
    ret = cmdfile(infile, cmd_file::AT_AFTER_STARTUP);

    // PAR reads a file and sets color, don't read colors from GIF
    dontreadcolor = colors_preloaded && show_file == 0;

    return ret;
}


static void initvars_run()              // once per run init
{
    init_rseed = (int)time(nullptr);
    init_comments();
    char const *p = getenv("TMP");
    if (p == nullptr)
    {
        p = getenv("TEMP");
    }
    if (p != nullptr)
    {
        if (isadirectory(p))
        {
            tempdir = p;
            fix_dirname(tempdir);
        }
    }
    else
    {
        tempdir.clear();
    }
}

static void initvars_restart()          // <ins> key init
{
    recordcolors = 'a';                 // don't use mapfiles in PARs
    save_release = g_release;           // this release number
    gif87a_flag = false;                // turn on GIF89a processing
    dither_flag = false;                // no dithering
    askvideo = true;                    // turn on video-prompt flag
    fract_overwrite = false;            // don't overwrite
    soundflag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; // sound is on to PC speaker
    init_batch = batch_modes::NONE;                      // not in batch mode
    checkcurdir = false;                // flag to check current dire for files
    initsavetime = 0;                   // no auto-save
    g_init_mode = -1;                   // no initial video mode
    viewwindow = false;                 // no view window
    viewreduction = 4.2F;
    viewcrop = true;
    g_virtual_screens = true;           // virtual screen modes on
    finalaspectratio = screenaspect;
    viewydots = 0;
    viewxdots = viewydots;
    video_cutboth = true;               // keep virtual aspect
    zscroll = true;                     // relaxed screen scrolling
    orbit_delay = 0;                    // full speed orbits
    orbit_interval = 1;                 // plot all orbits
    debugflag = debug_flags::none;      // debugging flag(s) are off
    timerflag = false;                  // timer flags are off
    FormFileName = "fractint.frm";      // default formula file
    FormName = "";
    LFileName = "fractint.l";
    LName = "";
    CommandFile = "fractint.par";
    CommandName = "";
    for (auto &elem : CommandComment)
        elem.clear();
    IFSFileName = "fractint.ifs";
    IFSName = "";
    reset_ifs_defn();
    rflag = false;                      // not a fixed srand() seed
    rseed = init_rseed;
    readname = DOTSLASH;                // initially current directory
    show_file = 1;
    // next should perhaps be fractal re-init, not just <ins> ?
    initcyclelimit = 55;                   // spin-DAC default speed limit
    mapset = false;                     // no map= name active
    map_specified = false;
    major_method = Major::breadth_first;    // default inverse julia methods
    minor_method = Minor::left_first;       // default inverse julia methods
    truecolor = false;                  // truecolor output flag
    truemode = 0;               // set to default color scheme
}

static void initvars_fractal()          // init vars affecting calculation
{
    escape_exit = false;                // don't disable the "are you sure?" screen
    usr_periodicitycheck = 1;           // turn on periodicity
    inside = 1;                         // inside color = blue
    fillcolor = -1;                     // no special fill color
    usr_biomorph = -1;                  // turn off biomorph flag
    outside = ITER;                     // outside color = -1 (not used)
    maxit = 150;                        // initial maxiter
    usr_stdcalcmode = 'g';              // initial solid-guessing
    stoppass = 0;                       // initial guessing stoppass
    quick_calc = false;
    closeprox = 0.01;
    ismand = true;                      // default formula mand/jul toggle
#ifndef XFRACT
    usr_floatflag = false;              // turn off the float flag
#else
    usr_floatflag = true;               // turn on the float flag
#endif
    finattract = false;                 // disable finite attractor logic
    fractype = fractal_type::MANDEL;    // initial type Set flag
    curfractalspecific = &fractalspecific[0];
    initcorners = false;
    initparams = false;
    bailout = 0;                        // no user-entered bailout
    nobof = false;                      // use normal bof initialization to make bof images
    useinitorbit = 0;
    for (int i = 0; i < MAXPARAMS; i++)
    {
        param[i] = 0.0;     // initial parameter values
    }
    for (int i = 0; i < 3; i++)
    {
        potparam[i]  = 0.0; // initial potential values
    }
    for (auto &elem : inversion)
    {
        elem = 0.0;  // initial invert values
    }
    initorbit.y = 0.0;
    initorbit.x = initorbit.y;     // initial orbit values
    invert = 0;
    decomp[1] = 0;
    decomp[0] = decomp[1];
    usr_distest = 0;
    pseudox = 0;
    pseudoy = 0;
    distestwidth = 71;
    forcesymmetry = symmetry_type::NOT_FORCED;
    xxmin = -2.5;
    xx3rd = xxmin;
    xxmax = 1.5;   // initial corner values
    yymin = -1.5;
    yy3rd = yymin;
    yymax = 1.5;   // initial corner values
    bf_math = bf_math_type::NONE;
    pot16bit = false;
    potflag = false;
    LogFlag = 0;                         // no logarithmic palette
    set_trig_array(0, "sin");             // trigfn defaults
    set_trig_array(1, "sqr");
    set_trig_array(2, "sinh");
    set_trig_array(3, "cosh");
    if (rangeslen)
    {
        ranges.clear();
        rangeslen = 0;
    }
    usemag = true;                      // use center-mag, not corners

    colorstate = 0;
    colors_preloaded = false;
    rotate_lo = 1;
    rotate_hi = 255;      // color cycling default range
    orbit_delay = 0;                     // full speed orbits
    orbit_interval = 1;                  // plot all orbits
    keep_scrn_coords = false;
    drawmode = 'r';                      // passes=orbits draw mode
    set_orbit_corners = false;
    oxmin = curfractalspecific->xmin;
    oxmax = curfractalspecific->xmax;
    ox3rd = curfractalspecific->xmin;
    oymin = curfractalspecific->ymin;
    oymax = curfractalspecific->ymax;
    oy3rd = curfractalspecific->ymin;

    math_tol[0] = 0.05;
    math_tol[1] = 0.05;

    display_3d = display_3d_modes::NONE;                       // 3D display is off
    overlay_3d = false;                  // 3D overlay is off

    old_demm_colors = false;
    bailoutest    = bailouts::Mod;
    floatbailout  = fpMODbailout;
    longbailout   = asmlMODbailout;
    bignumbailout = bnMODbailout;
    bigfltbailout = bfMODbailout;

    new_bifurcation_functions_loaded = false; // for old bifs
    mxminfp = -.83;
    myminfp = -.25;
    mxmaxfp = -.83;
    mymaxfp =  .25;
    originfp = 8;
    heightfp = 7;
    widthfp = 10;
    distfp = 24;
    eyesfp = 2.5F;
    depthfp = 8;
    neworbittype = fractal_type::JULIA;
    zdots = 128;
    initvars_3d();
    basehertz = 440;                     // basic hertz rate
#ifndef XFRACT
    fm_vol = 63;                         // full volume on soundcard o/p
    hi_atten = 0;                        // no attenuation of hi notes
    fm_attack = 5;                       // fast attack
    fm_decay = 10;                        // long decay
    fm_sustain = 13;                      // fairly high sustain level
    fm_release = 5;                      // short release
    fm_wavetype = 0;                     // sin wave
    polyphony = 0;                       // no polyphony
    for (int i = 0; i <= 11; i++)
        scale_map[i] = i+1;    // straight mapping of notes in octave
#endif
}

static void initvars_3d()               // init vars affecting 3d
{
    RAY     = 0;
    BRIEF   = false;
    SPHERE = FALSE;
    preview = false;
    showbox = false;
    xadjust = 0;
    yadjust = 0;
    g_eye_separation = 0;
    g_glasses_type = 0;
    previewfactor = 20;
    red_crop_left   = 4;
    red_crop_right  = 0;
    blue_crop_left  = 0;
    blue_crop_right = 4;
    red_bright     = 80;
    blue_bright   = 100;
    transparent[1] = 0;
    transparent[0] = transparent[1]; // no min/max transparency
    set_3d_defaults();
}

static void reset_ifs_defn()
{
    if (!ifs_defn.empty())
    {
        ifs_defn.clear();
    }
}


// mode = 0 command line @filename
//        1 sstools.ini
//        2 <@> command after startup
//        3 command line @filename/setname
static int cmdfile(FILE *handle, cmd_file mode)
{
    // note that cmdfile could be open as text OR as binary
    // binary is used in @ command processing for reasonable speed note/point
    int i;
    int lineoffset = 0;
    int changeflag = 0; // &1 fractal stuff chgd, &2 3d stuff chgd
    char linebuf[513];
    char cmdbuf[10000] = { 0 };

    if (mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME)
    {
        while ((i = getc(handle)) != '{' && i != EOF)
        {
        }
        for (auto &elem : CommandComment)
        {
            elem.clear();
        }
    }
    linebuf[0] = 0;
    while (next_command(cmdbuf, 10000, handle, linebuf, &lineoffset, mode) > 0)
    {
        if ((mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME) && strcmp(cmdbuf, "}") == 0)
        {
            break;
        }
        i = cmdarg(cmdbuf, mode);
        if (i == CMDARG_ERROR)
        {
            break;
        }
        changeflag |= i;
    }
    fclose(handle);
    if (changeflag & CMDARG_FRACTAL_PARAM)
    {
        backwards_v18();
        backwards_v19();
        backwards_v20();
    }
    return changeflag;
}

static int next_command(
    char *cmdbuf,
    int maxlen,
    FILE *handle,
    char *linebuf,
    int *lineoffset,
    cmd_file mode)
{
    int cmdlen = 0;
    char *lineptr;
    lineptr = linebuf + *lineoffset;
    while (1)
    {
        while (*lineptr <= ' ' || *lineptr == ';')
        {
            if (cmdlen)                 // space or ; marks end of command
            {
                cmdbuf[cmdlen] = 0;
                *lineoffset = (int)(lineptr - linebuf);
                return cmdlen;
            }
            while (*lineptr && *lineptr <= ' ')
            {
                ++lineptr;                  // skip spaces and tabs
            }
            if (*lineptr == ';' || *lineptr == 0)
            {
                if (*lineptr == ';'
                        && (mode == cmd_file::AT_AFTER_STARTUP || mode == cmd_file::AT_CMD_LINE_SET_NAME)
                        && (CommandComment[0].empty() || CommandComment[1].empty() ||
                            CommandComment[2].empty() || CommandComment[3].empty()))
                {
                    // save comment
                    while (*(++lineptr)
                            && (*lineptr == ' ' || *lineptr == '\t'))
                    {
                    }
                    if (*lineptr)
                    {
                        if ((int)strlen(lineptr) >= MAXCMT)
                        {
                            *(lineptr+MAXCMT-1) = 0;
                        }
                        for (auto &elem : CommandComment)
                        {
                            if (elem.empty())
                            {
                                elem = lineptr;
                                break;
                            }
                        }
                    }
                }
                if (next_line(handle, linebuf, mode))
                {
                    return -1; // eof
                }
                lineptr = linebuf; // start new line
            }
        }
        if (*lineptr == '\\'              // continuation onto next line?
                && *(lineptr+1) == 0)
        {
            if (next_line(handle, linebuf, mode))
            {
                argerror(cmdbuf);           // missing continuation
                return -1;
            }
            lineptr = linebuf;
            while (*lineptr && *lineptr <= ' ')
            {
                ++lineptr;                  // skip white space @ start next line
            }
            continue;                      // loop to check end of line again
        }
        cmdbuf[cmdlen] = *(lineptr++);    // copy character to command buffer
        if (++cmdlen >= maxlen)         // command too long?
        {
            argerror(cmdbuf);
            return -1;
        }
    }
}

static bool next_line(FILE *handle, char *linebuf, cmd_file mode)
{
    int toolssection;
    char tmpbuf[11];
    toolssection = 0;
    while (file_gets(linebuf, 512, handle) >= 0)
    {
        if (mode == cmd_file::SSTOOLS_INI && linebuf[0] == '[')   // check for [fractint]
        {
#ifndef XFRACT
            strncpy(tmpbuf, &linebuf[1], 9);
            tmpbuf[9] = 0;
            strlwr(tmpbuf);
            toolssection = strncmp(tmpbuf, "fractint]", 9);
#else
            strncpy(tmpbuf, &linebuf[1], 10);
            tmpbuf[10] = 0;
            strlwr(tmpbuf);
            toolssection = strncmp(tmpbuf, "xfractint]", 10);
#endif
            continue;                              // skip tools section heading
        }
        if (toolssection == 0)
        {
            return false;
        }
    }
    return true;
}

class parameter_processor
{
public:
    parameter_processor(char *curarg, cmd_file mode);

    int process();

private:
    int     valuelen;                   // length of value
    int     numval;                     // numeric value of arg
    char    charval[16];                // first character of arg
    int     yesnoval[16];               // 0 if 'n', 1 if 'y', -1 if not
    double  ftemp;
    int     totparms;                   // # of / delimited parms
    int     intparms;                   // # of / delimited ints
    int     floatparms;                 // # of / delimited floats
    int     intval[64];                 // pre-parsed integer parms
    double  floatval[16];               // pre-parsed floating parms
    char const *floatvalstr[16];        // pointers to float vals
    char    tmpc;
    int     lastarg;
    double Xctr;
    double Yctr;
    double Xmagfactor;
    double Rotation;
    double Skew;
    LDBL Magnification;
    bf_t bXctr;
    bf_t bYctr;
    char *curarg;
    cmd_file mode;
    char *value;
    std::string variable;

    void convert_argument_to_lower_case();
    int parse_parameter();
    int bad_parameter()
    {
        argerror(curarg);
        return CMDARG_ERROR;
    }

    int startup_parameter();
    int param_batch();
    int param_max_history();
    int param_adapter();
    int param_afi();
    int param_text_safe();
    int param_vesa_detect();
    int param_bios_palette();
    int param_fpu();
    int param_exit_no_ask();
    int param_make_doc();
    int param_make_par();

    int parameter();
    int param_reset();
    int param_filename();
    int param_video();
    int param_map();
    int param_colors();
    int param_record_colors();
    int param_max_line_length();
    int param_comment();
    int param_tplus();
    int param_non_interlaced();
    int param_max_color_res();
    int param_pixel_zoom();
    int param_overwrite();
    int param_gif87a();
    int param_dither();
    int param_save_time();
    int param_auto_key();
    int param_auto_key_name();
    int param_type();
    int param_inside();
    int param_proximity();
    int param_fill_color();
    int param_fin_attract();
    int param_no_bof();
    int param_function();
    int param_outside();
    int param_bf_digits();
    int param_max_iter();
    int param_passes();
    int param_is_mand();
    int param_cycle_limit();
    int param_make_mig();
    int param_cycle_range();
    int param_ranges();
    int param_save_name();
    int param_tweak_lzw();
    int param_min_stack();
    int param_math_tolerance();
    int param_temp_dir();
    int param_work_dir();
    int param_exit_mode();
    int param_text_colors();
    int param_potential();
    int param_params();
    int param_miim();
    int param_init_orbit();
    int param_orbit_name();
    int param_3d_mode();
    int param_julibrot_3d();
    int param_julibrot_eyes();
    int param_julibrot_from_to();
    int param_corners();
    int param_orbit_corners();
    int param_screen_coords();
    int param_orbit_draw_mode();
    int param_view_windows();
    int param_center_mag();
    int param_aspect_drift();
    int param_invert();
    int param_old_demm_colors();
    int param_ask_video();
    int param_float();
    int param_fast_restore();
    int param_org_frm_dir();
    int param_biomorph();
    int param_orbit_save();
    int param_bailout();
    int param_bailout_test();
    int param_symmetry();
    bool is_print_parameter();
    int param_sound();
    int param_hertz();
    int param_volume();
    int param_attenuate();
    int param_polyphony();
    int param_wave_type();
    int param_attack();
    int param_decay();
    int param_sustain();
    int param_s_release();
    int param_scale_map();
    int param_periodicity();
    int param_log_map();
    int param_log_mode();
    int param_debug_flag();
    int param_rseed();
    int param_orbit_delay();
    int param_orbit_interval();
    int param_show_dot();
    int param_show_orbit();
    int param_decomp();
    int param_dist_test();
    int param_formula_file();
    int param_formula_name();
    int param_l_file();
    int param_l_name();
    int param_ifs_file();
    int param_ifs();
    int param_parm_file();
    int param_stereo();
    int param_rotation();
    int param_perspective();
    int param_x_y_shift();
    int param_interocular();
    int param_converge();
    int param_crop();
    int param_bright();
    int param_x_y_adjust();
    int param_3d();
    int param_sphere();
    int param_scale_x_y_z();
    int param_roughness();
    int param_waterline();
    int param_fill_type();
    int param_light_source();
    int param_smoothing();
    int param_latitude();
    int param_longitude();
    int param_radius();
    int param_transparent();
    int param_preview();
    int param_show_box();
    int param_coarse();
    int param_randomize();
    int param_ambient();
    int param_haze();
    int param_full_color();
    int param_true_color();
    int param_true_mode();
    int param_use_gray_scale();
    int param_monitor_width();
    int param_targa_overlay();
    int param_background();
    int param_light_name();
    int param_ray();
    int param_brief();
    int param_release();
    int param_cur_dir();
    int param_virtual();
};

namespace
{
int const NON_NUMERIC = -32767;
}

 /*
  cmdarg(string,mode) processes a single command-line/command-file argument
    return:
      -1 error, >= 0 ok
      if ok, return value:
        | 1 means fractal parm has been set
        | 2 means 3d parm has been set
        | 4 means 3d=yes specified
        | 8 means reset specified
*/
int cmdarg(char *curarg, cmd_file mode) // process a single argument
{
    return parameter_processor(curarg, mode).process();
}

parameter_processor::parameter_processor(char *curarg_, cmd_file mode_)
    : valuelen(0),
    numval(0),
    ftemp(0.0),
    totparms(0),
    intparms(0),
    floatparms(0),
    tmpc(0),
    lastarg(0),
    Xctr(0.0),
    Yctr(0.0),
    Xmagfactor(0.0),
    Rotation(0.0),
    Skew(0.0),
    Magnification(0.0),
    bXctr(nullptr),
    bYctr(nullptr),
    curarg(curarg_),
    mode(mode_),
    value(nullptr)
{
    std::fill_n(charval, 16, 0);
    std::fill_n(yesnoval, 16, 0);
    std::fill_n(intval, 64, 0 );
    std::fill_n(floatval, 16, 0.0);
    std::fill_n(floatvalstr, 16, nullptr);
}

void parameter_processor::convert_argument_to_lower_case()
{
    char *argptr = curarg;
    while (*argptr)
    {   // convert to lower case
        if (*argptr >= 'A' && *argptr <= 'Z')
        {
            *argptr += 'a' - 'A';
        }
        else if (*argptr == '=')
        {
            // don't convert colors=value or comment=value
            if ((strncmp(curarg, "colors=", 7) == 0) || (strncmp(curarg, "comment", 7) == 0))
            {
                break;
            }
        }
        ++argptr;
    }
}

int parameter_processor::parse_parameter()
{
    int j;
    value = strchr(&curarg[1], '=');
    if (value != nullptr)
    {
        j = (int)((value++) - curarg);
        if (j > 1 && curarg[j-1] == ':')
        {
            --j;                           // treat := same as =
        }
    }
    else
    {
        j = (int) strlen(curarg);
        value = curarg + j;
    }
    if (j > 20)
    {
        return bad_parameter();               // keyword too long
    }
    variable = std::string(curarg, j);
    valuelen = (int) strlen(value);            // note value's length
    charval[0] = value[0];               // first letter of value
    yesnoval[0] = -1;                    // note yes|no value
    if (charval[0] == 'n')
    {
        yesnoval[0] = 0;
    }
    if (charval[0] == 'y')
    {
        yesnoval[0] = 1;
    }

    char *argptr = value;
    floatparms = 0;
    intparms = 0;
    totparms = 0;
    numval = 0;
    while (*argptr)                    // count and pre-parse parms
    {
        long ll;
        lastarg = 0;
        char *argptr2 = strchr(argptr, '/');
        if (argptr2 == nullptr)     // find next '/'
        {
            argptr2 = argptr + strlen(argptr);
            *argptr2 = '/';
            lastarg = 1;
        }
        if (totparms == 0)
        {
            numval = NON_NUMERIC;
        }
        if (totparms < 16)
        {
            charval[totparms] = *argptr;                      // first letter of value
            if (charval[totparms] == 'n')
            {
                yesnoval[totparms] = 0;
            }
            if (charval[totparms] == 'y')
            {
                yesnoval[totparms] = 1;
            }
        }
        char next = 0;
        if (sscanf(argptr, "%c%c", &next, &tmpc) > 0    // NULL entry
                && (next == '/' || next == '=') && tmpc == '/')
        {
            j = 0;
            ++floatparms;
            ++intparms;
            if (totparms < 16)
            {
                floatval[totparms] = j;
                floatvalstr[totparms] = "0";
            }
            if (totparms < 64)
            {
                intval[totparms] = j;
            }
            if (totparms == 0)
            {
                numval = j;
            }
        }
        else if (sscanf(argptr, "%ld%c", &ll, &tmpc) > 0       // got an integer
                 && tmpc == '/')        // needs a long int, ll, here for lyapunov
        {
            ++floatparms;
            ++intparms;
            if (totparms < 16)
            {
                floatval[totparms] = ll;
                floatvalstr[totparms] = argptr;
            }
            if (totparms < 64)
            {
                intval[totparms] = (int) ll;
            }
            if (totparms == 0)
            {
                numval = (int) ll;
            }
        }
#ifndef XFRACT
        else if (sscanf(argptr, "%lg%c", &ftemp, &tmpc) > 0  // got a float
#else
        else if (sscanf(argptr, "%lf%c", &ftemp, &tmpc) > 0  // got a float
#endif
                 && tmpc == '/')
        {
            ++floatparms;
            if (totparms < 16)
            {
                floatval[totparms] = ftemp;
                floatvalstr[totparms] = argptr;
            }
        }
        // using arbitrary precision and above failed
        else if (((int) strlen(argptr) > 513)  // very long command
                 || (totparms > 0 && floatval[totparms-1] == FLT_MAX
                     && totparms < 6)
                 || isabigfloat(argptr))
        {
            ++floatparms;
            floatval[totparms] = FLT_MAX;
            floatvalstr[totparms] = argptr;
        }
        ++totparms;
        argptr = argptr2;                                 // on to the next
        if (lastarg)
        {
            *argptr = 0;
        }
        else
        {
            ++argptr;
        }
    }
    return CMDARG_NONE;
}

// these commands are allowed only at startup
int parameter_processor::startup_parameter()
{
    if (variable == "batch")
    {
        return param_batch();
    }
    if (variable == "maxhistory")
    {
        return param_max_history();
    }

    if (variable == "adapter")
    {
        return param_adapter();
    }

    if (variable == "afi")
    {
        return param_afi();
    }

    if (variable == "textsafe")   // textsafe==?
    {
        return param_text_safe();
    }

    if (variable == "vesadetect")
    {
        return param_vesa_detect();
    }

    if (variable == "biospalette")
    {
        return param_bios_palette();
    }

    if (variable == "fpu")
    {
        return param_fpu();
    }

    if (variable == "exitnoask")
    {
        return param_exit_no_ask();
    }

    if (variable == "makedoc")
    {
        return param_make_doc();
    }

    if (variable == "makepar")
    {
        return param_make_par();
    }

    return CMDARG_NONE;
}

// batch=?
int parameter_processor::param_batch()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
#ifdef XFRACT
    g_init_mode = yesnoval[0] ? 0 : -1; // skip credits for batch mode
#endif
    init_batch = static_cast<batch_modes>(yesnoval[0]);
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// maxhistory=?
int parameter_processor::param_max_history()
{
    if (numval == NON_NUMERIC)
    {
        return bad_parameter();
    }
    else if (numval < 0)
    {
        return bad_parameter();
    }
    else
    {
        maxhistory = numval;
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// adapter==?
// adapter= no longer used
// adapter parameter no longer used; check for bad argument anyway
int parameter_processor::param_adapter()
{
    if ((strcmp(value, "egamono") != 0) && (strcmp(value, "hgc") != 0) &&
            (strcmp(value, "ega") != 0)     && (strcmp(value, "cga") != 0) &&
            (strcmp(value, "mcga") != 0)    && (strcmp(value, "vga") != 0))
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// 8514 API no longer used; silently gobble any argument
int parameter_processor::param_afi()
{
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// textsafe no longer used, do validity checking, but gobble argument
int parameter_processor::param_text_safe()
{
    if (first_init)
    {
        if (!((charval[0] == 'n')   // no
                || (charval[0] == 'y')  // yes
                || (charval[0] == 'b')  // bios
                || (charval[0] == 's'))) // save
        {
            return bad_parameter();
        }
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// vesadetect no longer used, do validity checks, but gobble argument
int parameter_processor::param_vesa_detect()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// biospalette no longer used, do validity checks, but gobble argument
int parameter_processor::param_bios_palette()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

int parameter_processor::param_fpu()
{
    if (strcmp(value, "387") == 0)
    {
        return CMDARG_NONE;
    }
    return bad_parameter();
}

int parameter_processor::param_exit_no_ask()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    escape_exit = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

int parameter_processor::param_make_doc()
{
    print_document(*value ? value : "fractint.doc", makedoc_msg_func, 0);
    goodbye();
    return CMDARG_NONE;
}

int parameter_processor::param_make_par()
{
    char *slash, *next = nullptr;
    if (totparms < 1 || totparms > 2)
    {
        return bad_parameter();
    }
    slash = strchr(value, '/');
    if (slash != nullptr)
    {
        *slash = 0;
        next = slash+1;
    }

    CommandFile = value;
    if (strchr(CommandFile.c_str(), '.') == nullptr)
    {
        CommandFile += ".par";
    }
    if (readname == DOTSLASH)
    {
        readname = "";
    }
    if (next == nullptr)
    {
        if (!readname.empty())
        {
            CommandName = extract_filename(readname.c_str());
        }
        else if (!MAP_name.empty())
        {
            CommandName = extract_filename(MAP_name.c_str());
        }
        else
        {
            argerror(curarg);
            return CMDARG_ERROR;
        }
    }
    else
    {
        CommandName = next;
        assert(CommandName.length() <= ITEMNAMELEN);
        if (CommandName.length() > ITEMNAMELEN)
        {
            CommandName.resize(ITEMNAMELEN);
        }
    }
    make_parameter_file = true;
    if (!readname.empty())
    {
        if (read_overlay() != 0)
        {
            goodbye();
        }
    }
    else if (!MAP_name.empty())
    {
        make_parameter_file_map = true;
    }
    xdots = filexdots;
    ydots = fileydots;
    x_size_d = xdots - 1;
    y_size_d = ydots - 1;
    calcfracinit();
    make_batch_file();
#if !defined(XFRACT)
    ABORT(0, "Don't call standard I/O without a console on Windows");
    _ASSERTE(0 && "Don't call standard I/O without a console on Windows");
#endif
    goodbye();
    return CMDARG_NONE;
}

int parameter_processor::process()
{
    convert_argument_to_lower_case();

    int result = parse_parameter();
    if (result != CMDARG_NONE)
    {
        return result;
    }

    if (mode != cmd_file::AT_AFTER_STARTUP || debugflag == debug_flags::allow_init_commands_anytime)
    {
        result = startup_parameter();
        if (result != CMDARG_NONE)
        {
            return result;
        }
    }

    return parameter();
}

int parameter_processor::parameter()
{
    if (variable == "reset")
    {
        return param_reset();
    }

    if (variable == "filename")
    {
        return param_filename();
    }

    if (variable == "video")
    {
        return param_video();
    }

    if (variable == "map")
    {
        return param_map();
    }

    if (variable == "colors")
    {
        return param_colors();
    }

    if (variable == "recordcolors")
    {
        return param_record_colors();
    }

    if (variable == "maxlinelength")
    {
        return param_max_line_length();
    }

    if (variable == "comment")
    {
        return param_comment();
    }

    if (variable == "tplus")
    {
        return param_tplus();
    }

    if (variable == "noninterlaced")
    {
        return param_non_interlaced();
    }

    if (variable == "maxcolorres")
    {
        return param_max_color_res();
    }

    if (variable == "pixelzoom")
    {
        return param_pixel_zoom();
    }

    if ((variable == "warn")            // keep this for backward compatibility
            || (variable == "overwrite"))
    {
        return param_overwrite();
    }

    if (variable == "gif87a")
    {
        return param_gif87a();
    }

    if (variable == "dither")
    {
        return param_dither();
    }

    if (variable == "savetime")
    {
        return param_save_time();
    }

    if (variable == "autokey")
    {
        return param_auto_key();
    }

    if (variable == "autokeyname")
    {
        return param_auto_key_name();
    }

    if (variable == "type")
    {
        return param_type();
    }

    if (variable == "inside")
    {
        return param_inside();
    }

    if (variable == "proximity")
    {
        return param_proximity();
    }

    if (variable == "fillcolor")
    {
        return param_fill_color();
    }

    if (variable == "finattract")
    {
        return param_fin_attract();
    }

    if (variable == "nobof")
    {
        return param_no_bof();
    }

    if (variable == "function")
    {
        return param_function();
    }

    if (variable == "outside")
    {
        return param_outside();
    }

    if (variable == "bfdigits")
    {
        return param_bf_digits();
    }

    if (variable == "maxiter")
    {
        return param_max_iter();
    }

    if (variable == "iterincr")
    {
        // iterincr=?
        return CMDARG_NONE;
    }

    if (variable == "passes")
    {
        return param_passes();
    }

    if (variable == "ismand")
    {
        return param_is_mand();
    }

    if (variable == "cyclelimit")
    {
        return param_cycle_limit();
    }

    if (variable == "makemig")
    {
        return param_make_mig();
    }

    if (variable == "cyclerange")
    {
        return param_cycle_range();
    }

    if (variable == "ranges")
    {
        return param_ranges();
    }

    if (variable == "savename")
    {
        return param_save_name();
    }

    if (variable == "tweaklzw")
    {
        return param_tweak_lzw();
    }

    if (variable == "minstack")
    {
        return param_min_stack();
    }

    if (variable == "mathtolerance")
    {
        return param_math_tolerance();
    }

    if (variable == "tempdir")
    {
        return param_temp_dir();
    }

    if (variable == "workdir")
    {
        return param_work_dir();
    }

    if (variable == "exitmode")
    {
        return param_exit_mode();
    }

    if (variable == "textcolors")
    {
        return param_text_colors();
    }

    if (variable == "potential")
    {
        return param_potential();
    }

    if (variable == "params")
    {
        return param_params();
    }

    if (variable == "miim")
    {
        return param_miim();
    }

    if (variable == "initorbit")
    {
        return param_init_orbit();
    }

    if (variable == "orbitname")
    {
        return param_orbit_name();
    }

    if (variable == "3dmode")
    {
        return param_3d_mode();
    }

    if (variable == "julibrot3d")
    {
        return param_julibrot_3d();
    }

    if (variable == "julibroteyes")
    {
        return param_julibrot_eyes();
    }

    if (variable == "julibrotfromto")
    {
        return param_julibrot_from_to();
    }

    if (variable == "corners")
    {
        return param_corners();
    }

    if (variable == "orbitcorners")
    {
        return param_orbit_corners();
    }

    if (variable == "screencoords")
    {
        return param_screen_coords();
    }

    if (variable == "orbitdrawmode")
    {
        return param_orbit_draw_mode();
    }

    if (variable == "viewwindows")
    {
        return param_view_windows();
    }

    if (variable == "center-mag")
    {
        return param_center_mag();
    }

    if (variable == "aspectdrift")
    {
        return param_aspect_drift();
    }

    if (variable == "invert")
    {
        return param_invert();
    }

    if (variable == "olddemmcolors")
    {
        return param_old_demm_colors();
    }

    if (variable == "askvideo")
    {
        return param_ask_video();
    }

    if (variable == "ramvideo")
    {
        // ramvideo=?
        return CMDARG_NONE; // just ignore and return, for old time's sake
    }

    if (variable == "float")
    {
        return param_float();
    }

    if (variable == "fastrestore")
    {
        return param_fast_restore();
    }

    if (variable == "orgfrmdir")
    {
        return param_org_frm_dir();
    }

    if (variable == "biomorph")
    {
        return param_biomorph();
    }

    if (variable == "orbitsave")
    {
        return param_orbit_save();
    }

    if (variable == "bailout")
    {
        return param_bailout();
    }

    if (variable == "bailoutest")
    {
        return param_bailout_test();
    }

    if (variable == "symmetry")
    {
        return param_symmetry();
    }

    if (is_print_parameter())
    {
        return CMDARG_NONE;
    }

    if (variable == "sound")
    {
        return param_sound();
    }

    if (variable == "hertz")
    {
        return param_hertz();
    }

    if (variable == "volume")
    {
        return param_volume();
    }

    if (variable == "attenuate")
    {
        return param_attenuate();
    }

    if (variable == "polyphony")
    {
        return param_polyphony();
    }

    if (variable == "wavetype")
    {
        return param_wave_type();
    }

    if (variable == "attack")
    {
        return param_attack();
    }

    if (variable == "decay")
    {
        return param_decay();
    }

    if (variable == "sustain")
    {
        return param_sustain();
    }

    if (variable == "srelease")
    {
        return param_s_release();
    }

    if (variable == "scalemap")
    {
        return param_scale_map();
    }

    if (variable == "periodicity")
    {
        return param_periodicity();
    }

    if (variable == "logmap")
    {
        return param_log_map();
    }

    if (variable == "logmode")
    {
        return param_log_mode();
    }

    if (variable == "debugflag" || variable == "debug")
    {
        return param_debug_flag();
    }

    if (variable == "rseed")
    {
        return param_rseed();
    }

    if (variable == "orbitdelay")
    {
        return param_orbit_delay();
    }

    if (variable == "orbitinterval")
    {
        return param_orbit_interval();
    }

    if (variable == "showdot")
    {
        return param_show_dot();
    }

    if (variable == "showorbit")
    {
        return param_show_orbit();
    }

    if (variable == "decomp")
    {
        return param_decomp();
    }

    if (variable == "distest")
    {
        return param_dist_test();
    }

    if (variable == "formulafile")
    {
        return param_formula_file();
    }

    if (variable == "formulaname")
    {
        return param_formula_name();
    }

    if (variable == "lfile")
    {
        return param_l_file();
    }

    if (variable == "lname")
    {
        return param_l_name();
    }

    if (variable == "ifsfile")
    {
        return param_ifs_file();
    }

    if (variable == "ifs" || variable == "ifs3d") // ifs3d for old time's sake
    {
        return param_ifs();
    }

    if (variable == "parmfile")
    {
        return param_parm_file();
    }

    if (variable == "stereo")
    {
        return param_stereo();
    }

    if (variable == "rotation")
    {
        return param_rotation();
    }

    if (variable == "perspective")
    {
        return param_perspective();
    }

    if (variable == "xyshift")
    {
        return param_x_y_shift();
    }

    if (variable == "interocular")
    {
        return param_interocular();
    }

    if (variable == "converge")
    {
        return param_converge();
    }

    if (variable == "crop")
    {
        return param_crop();
    }

    if (variable == "bright")
    {
        return param_bright();
    }

    if (variable == "xyadjust")
    {
        return param_x_y_adjust();
    }

    if (variable == "3d")
    {
        return param_3d();
    }

    if (variable == "sphere")
    {
        return param_sphere();
    }

    if (variable == "scalexyz")
    {
        return param_scale_x_y_z();
    }

    if (variable == "roughness")
    {
        return param_roughness();
    }

    if (variable == "waterline")
    {
        return param_waterline();
    }

    if (variable == "filltype")
    {
        return param_fill_type();
    }

    if (variable == "lightsource")
    {
        return param_light_source();
    }

    if (variable == "smoothing")
    {
        return param_smoothing();
    }

    if (variable == "latitude")
    {
        return param_latitude();
    }

    if (variable == "longitude")
    {
        return param_longitude();
    }

    if (variable == "radius")
    {
        return param_radius();
    }

    if (variable == "transparent")
    {
        return param_transparent();
    }

    if (variable == "preview")
    {
        return param_preview();
    }

    if (variable == "showbox")
    {
        return param_show_box();
    }

    if (variable == "coarse")
    {
        return param_coarse();
    }

    if (variable == "randomize")
    {
        return param_randomize();
    }

    if (variable == "ambient")
    {
        return param_ambient();
    }

    if (variable == "haze")
    {
        return param_haze();
    }

    if (variable == "fullcolor")
    {
        return param_full_color();
    }

    if (variable == "truecolor")
    {
        return param_true_color();
    }

    if (variable == "truemode")
    {
        return param_true_mode();
    }

    if (variable == "usegrayscale")
    {
        return param_use_gray_scale();
    }

    if (variable == "monitorwidth")
    {
        return param_monitor_width();
    }

    if (variable == "targa_overlay")
    {
        return param_targa_overlay();
    }

    if (variable == "background")
    {
        return param_background();
    }

    if (variable == "lightname")
    {
        return param_light_name();
    }

    if (variable == "ray")
    {
        return param_ray();
    }

    if (variable == "brief")
    {
        return param_brief();
    }

    if (variable == "release")
    {
        return param_release();
    }

    if (variable == "curdir")
    {
        return param_cur_dir();
    }

    if (variable == "virtual")
    {
        return param_virtual();
    }

    return bad_parameter();
}

int parameter_processor::param_reset()
{
    initvars_fractal();

    // PAR release unknown unless specified
    if (numval >= 0)
    {
        save_release = numval;
    }
    else
    {
        return bad_parameter();
    }
    if (save_release == 0)
    {
        save_release = 1730; // before start of lyapunov wierdness
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_RESET;
}

// filename=?
int parameter_processor::param_filename()
{
    int existdir;
    if (charval[0] == '.' && value[1] != SLASHC)
    {
        if (valuelen > 4)
        {
            return bad_parameter();
        }
        // cppcheck-suppress constStatement
        gifmask = std::string{"*"} + value;
        return CMDARG_NONE;
    }
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (mode == cmd_file::AT_AFTER_STARTUP && display_3d == NONE) // can't do this in @ command
    {
        return bad_parameter();
    }

    existdir = merge_pathnames(readname, value, mode);
    if (existdir == 0)
    {
        show_file = 0;
    }
    else if (existdir < 0)
    {
        init_msg(variable.c_str(), value, mode);
    }
    else
    {
        browse_name = extract_filename(readname.c_str());
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// video=?
int parameter_processor::param_video()
{
    int k = check_vidmode_keyname(value);
    if (k == 0)
    {
        return bad_parameter();
    }
    g_init_mode = -1;
    for (int i = 0; i < MAXVIDEOMODES; ++i)
    {
        if (g_video_table[i].keynum == k)
        {
            g_init_mode = i;
            break;
        }
    }
    if (g_init_mode == -1)
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// map=, set default colors
int parameter_processor::param_map()
{
    int existdir;
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    existdir = merge_pathnames(MAP_name, value, mode);
    if (existdir > 0)
    {
        return CMDARG_NONE;    // got a directory
    }
    else if (existdir < 0)
    {
        init_msg(variable.c_str(), value, mode);
        return CMDARG_NONE;
    }
    SetColorPaletteName(MAP_name.c_str());
    return CMDARG_NONE;
}

// colors=, set current colors
int parameter_processor::param_colors()
{
    if (parse_colors(value) < 0)
    {
        return bad_parameter();
    }
    return CMDARG_NONE;
}

// recordcolors=
int parameter_processor::param_record_colors()
{
    if (*value != 'y' && *value != 'c' && *value != 'a')
    {
        return bad_parameter();
    }
    recordcolors = *value;
    return CMDARG_NONE;
}

// maxlinelength=
int parameter_processor::param_max_line_length()
{
    if (numval < MINMAXLINELENGTH || numval > MAXMAXLINELENGTH)
    {
        return bad_parameter();
    }
    maxlinelength = numval;
    return CMDARG_NONE;
}

// comment=
int parameter_processor::param_comment()
{
    parse_comments(value);
    return CMDARG_NONE;
}

// tplus no longer used, validate value and gobble argument
int parameter_processor::param_tplus()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    return CMDARG_NONE;
}

// noninterlaced no longer used, validate value and gobble argument
int parameter_processor::param_non_interlaced()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    return CMDARG_NONE;
}

// Change default color resolution
// maxcolorres no longer used, validate value and gobble argument
int parameter_processor::param_max_color_res()
{
    if (numval == 1 || numval == 4 || numval == 8 ||
            numval == 16 || numval == 24)
    {
        return CMDARG_NONE;
    }
    return bad_parameter();
}

// pixelzoom no longer used, validate value and gobble argument
int parameter_processor::param_pixel_zoom()
{
    if (numval >= 5)
    {
        return bad_parameter();
    }
    return CMDARG_NONE;
}

// warn=?
// overwrite=?
int parameter_processor::param_overwrite()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    fract_overwrite = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// gif87a=?
int parameter_processor::param_gif87a()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    gif87a_flag = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// dither=?
int parameter_processor::param_dither()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    dither_flag = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// savetime=?
int parameter_processor::param_save_time()
{
    initsavetime = numval;
    // TODO: restore periodic saving
    return CMDARG_NONE;
}

// autokey=?
int parameter_processor::param_auto_key()
{
    if (strcmp(value, "record") == 0)
    {
        g_slides = slides_mode::RECORD;
    }
    else if (strcmp(value, "play") == 0)
    {
        g_slides = slides_mode::PLAY;
    }
    else
    {
        return bad_parameter();
    }
    return CMDARG_NONE;
}

// autokeyname=?
int parameter_processor::param_auto_key_name()
{
    std::string buff;
    if (merge_pathnames(buff, value, mode) < 0)
    {
        init_msg(variable.c_str(), value, mode);
    }
    else
    {
        autoname = buff;
    }
    return CMDARG_NONE;
}

// type=?
int parameter_processor::param_type()
{
    if (value[valuelen -1] == '*')
    {
        value[--valuelen] = 0;
    }
    // kludge because type ifs3d has an asterisk in front
    if (strcmp(value, "ifs3d") == 0)
    {
        value[3] = 0;
    }
    int k;
    for (k = 0; fractalspecific[k].name != nullptr; k++)
    {
        if (strcmp(value, fractalspecific[k].name) == 0)
        {
            break;
        }
    }
    if (fractalspecific[k].name == nullptr)
    {
        return bad_parameter();
    }
    fractype = static_cast<fractal_type>(k);
    curfractalspecific = &fractalspecific[static_cast<int>(fractype)];
    if (!initcorners)
    {
        xxmin = curfractalspecific->xmin;
        xx3rd = xxmin;
        xxmax = curfractalspecific->xmax;
        yymin = curfractalspecific->ymin;
        yy3rd = yymin;
        yymax = curfractalspecific->ymax;
    }
    if (!initparams)
    {
        load_params(fractype);
    }
    return CMDARG_FRACTAL_PARAM;
}

// inside=?
int parameter_processor::param_inside()
{
    struct
    {
        char const *arg;
        int inside;
    }
    const args[] =
    {
        { "zmag", ZMAG },
        { "bof60", BOF60 },
        { "bof61", BOF61 },
        { "epsiloncross", EPSCROSS },
        { "startrail", STARTRAIL },
        { "period", PERIOD },
        { "fmod", FMODI },
        { "atan", ATANI },
        { "maxiter", -1 }
    };
    for (auto &arg : args)
    {
        if (strcmp(value, arg.arg) == 0)
        {
            inside = arg.inside;
            return CMDARG_FRACTAL_PARAM;
        }
    }
    if (numval == NON_NUMERIC)
    {
        return bad_parameter();
    }
    else
    {
        inside = numval;
    }
    return CMDARG_FRACTAL_PARAM;
}

// proximity=?
int parameter_processor::param_proximity()
{
    closeprox = floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// fillcolor
int parameter_processor::param_fill_color()
{
    if (strcmp(value, "normal") == 0)
    {
        fillcolor = -1;
    }
    else if (numval == NON_NUMERIC)
    {
        return bad_parameter();
    }
    else
    {
        fillcolor = numval;
    }
    return CMDARG_FRACTAL_PARAM;
}

// finattract=?
int parameter_processor::param_fin_attract()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    finattract = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM;
}

// nobof=?
int parameter_processor::param_no_bof()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    nobof = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM;
}

// function=?,?
int parameter_processor::param_function()
{
    int k = 0;
    while (*value && k < 4)
    {
        if (set_trig_array(k++, value))
        {
            return bad_parameter();
        }
        value = strchr(value, '/');
        if (value == nullptr)
        {
            break;
        }
        ++value;
    }
    new_bifurcation_functions_loaded = true; // for old bifs
    return CMDARG_FRACTAL_PARAM;
}

// outside=?
int parameter_processor::param_outside()
{
    struct
    {
        char const *arg;
        int outside;
    }
    args[] =
    {
        { "iter", ITER },
        { "real", REAL },
        { "imag", IMAG },
        { "mult", MULT },
        { "summ", SUM },
        { "atan", ATAN },
        { "fmod", FMOD },
        { "tdis", TDIS }
    };
    for (auto &arg : args)
    {
        if (strcmp(value, arg.arg) == 0)
        {
            outside = arg.outside;
            return CMDARG_FRACTAL_PARAM;
        }
    }
    if ((numval == NON_NUMERIC) || (numval < TDIS || numval > 255))
    {
        return bad_parameter();
    }
    outside = numval;
    return CMDARG_FRACTAL_PARAM;
}

// bfdigits=?
int parameter_processor::param_bf_digits()
{
    if ((numval == NON_NUMERIC) || (numval < 0 || numval > 2000))
    {
        return bad_parameter();
    }
    bfdigits = numval;
    return CMDARG_FRACTAL_PARAM;
}

// maxiter=?
int parameter_processor::param_max_iter()
{
    if (floatval[0] < 2)
    {
        return bad_parameter();
    }
    maxit = (long) floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// passes=?
int parameter_processor::param_passes()
{
    if (charval[0] != '1' && charval[0] != '2' && charval[0] != '3'
            && charval[0] != 'g' && charval[0] != 'b'
            && charval[0] != 't' && charval[0] != 's'
            && charval[0] != 'd' && charval[0] != 'o')
    {
        return bad_parameter();
    }
    usr_stdcalcmode = charval[0];
    if (charval[0] == 'g')
    {
        stoppass = ((int) value[1] - (int)'0');
        if (stoppass < 0 || stoppass > 6)
        {
            stoppass = 0;
        }
    }
    return CMDARG_FRACTAL_PARAM;
}

// ismand=?
int parameter_processor::param_is_mand()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    ismand = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM;
}

// cyclelimit=?
int parameter_processor::param_cycle_limit()
{
    if (numval <= 1 || numval > 256)
    {
        return bad_parameter();
    }
    initcyclelimit = numval;
    return CMDARG_NONE;
}

int parameter_processor::param_make_mig()
{
    int xmult, ymult;
    if (totparms < 2)
    {
        return bad_parameter();
    }
    xmult = intval[0];
    ymult = intval[1];
    make_mig(xmult, ymult);
    exit(0);
    return CMDARG_NONE;
}

int parameter_processor::param_cycle_range()
{
    if (totparms < 2)
    {
        intval[1] = 255;
    }
    if (totparms < 1)
    {
        intval[0] = 1;
    }
    if (totparms != intparms
            || intval[0] < 0 || intval[1] > 255 || intval[0] > intval[1])
    {
        return bad_parameter();
    }
    rotate_lo = intval[0];
    rotate_hi = intval[1];
    return CMDARG_NONE;
}

int parameter_processor::param_ranges()
{
    int i, j, entries, prev;
    int tmpranges[128];

    if (totparms != intparms)
    {
        return bad_parameter();
    }
    i = 0;
    prev = i;
    entries = prev;
    LogFlag = 0; // ranges overrides logmap
    while (i < totparms)
    {
        j = intval[i++];
        if (j < 0) // striping
        {
            j = -j;
            if (j < 1 || j >= 16384 || i >= totparms)
            {
                return bad_parameter();
            }
            tmpranges[entries++] = -1; // {-1,width,limit} for striping
            tmpranges[entries++] = j;
            j = intval[i++];
        }
        if (j < prev)
        {
            return bad_parameter();
        }
        prev = j;
        tmpranges[entries++] = prev;
    }
    if (prev == 0)
    {
        return bad_parameter();
    }
    bool resized = false;
    try
    {
        ranges.resize(entries);
        resized = true;
    }
    catch (std::bad_alloc const &)
    {
    }
    if (!resized)
    {
        stopmsg(STOPMSG_NO_STACK, "Insufficient memory for ranges=");
        return CMDARG_ERROR;
    }
    rangeslen = entries;
    for (int i = 0; i < rangeslen; ++i)
    {
        ranges[i] = tmpranges[i];
    }
    return CMDARG_FRACTAL_PARAM;
}

// savename=?
int parameter_processor::param_save_name()
{
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (first_init || mode == cmd_file::AT_AFTER_STARTUP)
    {
        if (merge_pathnames(savename, value, mode) < 0)
        {
            init_msg(variable.c_str(), value, mode);
        }
    }
    return CMDARG_NONE;
}

// tweaklzw=?
int parameter_processor::param_tweak_lzw()
{
    if (totparms >= 1)
    {
        lzw[0] = intval[0];
    }
    if (totparms >= 2)
    {
        lzw[1] = intval[1];
    }
    return CMDARG_NONE;
}

// minstack=?
int parameter_processor::param_min_stack()
{
    if (totparms != 1)
    {
        return bad_parameter();
    }
    minstack = intval[0];
    return CMDARG_NONE;
}

// mathtolerance=?
int parameter_processor::param_math_tolerance()
{
    if (charval[0] == '/')
    {
        ; // leave math_tol[0] at the default value
    }
    else if (totparms >= 1)
    {
        math_tol[0] = floatval[0];
    }
    if (totparms >= 2)
    {
        math_tol[1] = floatval[1];
    }
    return CMDARG_NONE;
}

// tempdir=?
int parameter_processor::param_temp_dir()
{
    if (valuelen > (FILE_MAX_DIR-1))
    {
        return bad_parameter();
    }
    if (!isadirectory(value))
    {
        return bad_parameter();
    }
    tempdir = value;
    fix_dirname(tempdir);
    return CMDARG_NONE;
}

// workdir=?
int parameter_processor::param_work_dir()
{
    if (valuelen > (FILE_MAX_DIR-1))
    {
        return bad_parameter();
    }
    if (!isadirectory(value))
    {
        return bad_parameter();
    }
    workdir = value;
    fix_dirname(workdir);
    return CMDARG_NONE;
}

// exitmode=?
int parameter_processor::param_exit_mode()
{
    sscanf(value, "%x", &numval);
    exitmode = (BYTE) numval;
    return CMDARG_NONE;
}

int parameter_processor::param_text_colors()
{
    parse_textcolors(value);
    return CMDARG_NONE;
}

// potential=?
int parameter_processor::param_potential()
{
    int k = 0;
    while (k < 3 && *value)
    {
        if (k == 1)
        {
            potparam[k] = atof(value);
        }
        else
        {
            potparam[k] = atoi(value);
        }
        k++;
        value = strchr(value, '/');
        if (value == nullptr)
        {
            k = 99;
        }
        ++value;
    }
    pot16bit = false;
    if (k < 99)
    {
        if (strcmp(value, "16bit"))
        {
            return bad_parameter();
        }
        pot16bit = true;
    }
    return CMDARG_FRACTAL_PARAM;
}

// params=?,?
int parameter_processor::param_params()
{
    if (totparms != floatparms || totparms > MAXPARAMS)
    {
        return bad_parameter();
    }
    initparams = true;
    for (int k = 0; k < MAXPARAMS; ++k)
    {
        param[k] = (k < totparms) ? floatval[k] : 0.0;
    }
    if (bf_math != bf_math_type::NONE)
    {
        for (int k = 0; k < MAXPARAMS; k++)
        {
            floattobf(bfparms[k], param[k]);
        }
    }
    return CMDARG_FRACTAL_PARAM;
}

// miim=?[/?[/?[/?]]]
int parameter_processor::param_miim()
{
    if (totparms > 6)
    {
        return bad_parameter();
    }
    if (charval[0] == 'b')
    {
        major_method = Major::breadth_first;
    }
    else if (charval[0] == 'd')
    {
        major_method = Major::depth_first;
    }
    else if (charval[0] == 'w')
    {
        major_method = Major::random_walk;
    }
#ifdef RANDOM_RUN
    else if (charval[0] == 'r')
    {
        major_method = Major::random_run;
    }
#endif
    else
    {
        return bad_parameter();
    }

    if (charval[1] == 'l')
    {
        minor_method = Minor::left_first;
    }
    else if (charval[1] == 'r')
    {
        minor_method = Minor::right_first;
    }
    else
    {
        return bad_parameter();
    }

    // keep this next part in for backwards compatibility with old PARs ???

    if (totparms > 2)
    {
        for (int k = 2; k < 6; ++k)
        {
            param[k-2] = (k < totparms) ? floatval[k] : 0.0;
        }
    }

    return CMDARG_FRACTAL_PARAM;
}

// initorbit=?,?
int parameter_processor::param_init_orbit()
{
    if (strcmp(value, "pixel") == 0)
    {
        useinitorbit = 2;
    }
    else
    {
        if (totparms != 2 || floatparms != 2)
        {
            return bad_parameter();
        }
        initorbit.x = floatval[0];
        initorbit.y = floatval[1];
        useinitorbit = 1;
    }
    return CMDARG_FRACTAL_PARAM;
}

// orbitname=?
int parameter_processor::param_orbit_name()
{
    if (check_orbit_name(value))
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM;
}

// orbitname=?
int parameter_processor::param_3d_mode()
{
    int j = -1;
    for (int i = 0; i < 4; i++)
    {
        if (juli3Doptions[i] == value)
        {
            j = i;
        }
    }
    if (j < 0)
    {
        return bad_parameter();
    }
    else
    {
        juli3Dmode = j;
    }
    return CMDARG_FRACTAL_PARAM;
}

// julibrot3d=?,?,?,?
int parameter_processor::param_julibrot_3d()
{
    if (floatparms != totparms)
    {
        return bad_parameter();
    }
    if (totparms > 0)
    {
        zdots = (int) floatval[0];
    }
    if (totparms > 1)
    {
        originfp = (float) floatval[1];
    }
    if (totparms > 2)
    {
        depthfp = (float) floatval[2];
    }
    if (totparms > 3)
    {
        heightfp = (float) floatval[3];
    }
    if (totparms > 4)
    {
        widthfp = (float) floatval[4];
    }
    if (totparms > 5)
    {
        distfp = (float) floatval[5];
    }
    return CMDARG_FRACTAL_PARAM;
}

// julibroteyes=?,?,?,?
int parameter_processor::param_julibrot_eyes()
{
    if (floatparms != totparms || totparms != 1)
    {
        return bad_parameter();
    }
    eyesfp = (float) floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// julibrotfromto=?,?,?,?
int parameter_processor::param_julibrot_from_to()
{
    if (floatparms != totparms || totparms != 4)
    {
        return bad_parameter();
    }
    mxmaxfp = floatval[0];
    mxminfp = floatval[1];
    mymaxfp = floatval[2];
    myminfp = floatval[3];
    return CMDARG_FRACTAL_PARAM;
}

// corners=?,?,?,?
int parameter_processor::param_corners()
{
    int dec;
    if (fractype == fractal_type::CELLULAR)
    {
        return CMDARG_FRACTAL_PARAM; // skip setting the corners
    }
    if (floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
    {
        return bad_parameter();
    }
    usemag = false;
    if (totparms == 0)
    {
        return CMDARG_NONE; // turns corners mode on
    }
    initcorners = true;
    // good first approx, but dec could be too big
    dec = get_max_curarg_len(floatvalstr, totparms) + 1;
    if ((dec > DBL_DIG+1 || debugflag == debug_flags::force_arbitrary_precision_math)
            && debugflag != debug_flags::prevent_arbitrary_precision_math)
    {
        bf_math_type old_bf_math = bf_math;
        if (bf_math == bf_math_type::NONE || dec > decimals)
        {
            init_bf_dec(dec);
        }
        if (old_bf_math == bf_math_type::NONE)
        {
            for (int k = 0; k < MAXPARAMS; k++)
            {
                floattobf(bfparms[k], param[k]);
            }
        }

        // xx3rd = xxmin = floatval[0];
        get_bf(bfxmin, floatvalstr[0]);
        get_bf(bfx3rd, floatvalstr[0]);

        // xxmax = floatval[1];
        get_bf(bfxmax, floatvalstr[1]);

        // yy3rd = yymin = floatval[2];
        get_bf(bfymin, floatvalstr[2]);
        get_bf(bfy3rd, floatvalstr[2]);

        // yymax = floatval[3];
        get_bf(bfymax, floatvalstr[3]);

        if (totparms == 6)
        {
            // xx3rd = floatval[4];
            get_bf(bfx3rd, floatvalstr[4]);

            // yy3rd = floatval[5];
            get_bf(bfy3rd, floatvalstr[5]);
        }

        // now that all the corners have been read in, get a more
        // accurate value for dec and do it all again

        dec = getprecbf_mag();
        if (dec < 0)
        {
            return bad_parameter();
        }

        if (dec > decimals)  // get corners again if need more precision
        {
            init_bf_dec(dec);

            /* now get parameters and corners all over again at new
            decimal setting */
            for (int k = 0; k < MAXPARAMS; k++)
            {
                floattobf(bfparms[k], param[k]);
            }

            // xx3rd = xxmin = floatval[0];
            get_bf(bfxmin, floatvalstr[0]);
            get_bf(bfx3rd, floatvalstr[0]);

            // xxmax = floatval[1];
            get_bf(bfxmax, floatvalstr[1]);

            // yy3rd = yymin = floatval[2];
            get_bf(bfymin, floatvalstr[2]);
            get_bf(bfy3rd, floatvalstr[2]);

            // yymax = floatval[3];
            get_bf(bfymax, floatvalstr[3]);

            if (totparms == 6)
            {
                // xx3rd = floatval[4];
                get_bf(bfx3rd, floatvalstr[4]);

                // yy3rd = floatval[5];
                get_bf(bfy3rd, floatvalstr[5]);
            }
        }
    }
    xxmin = floatval[0];
    xx3rd = xxmin;
    xxmax = floatval[1];
    yymin = floatval[2];
    yy3rd = yymin;
    yymax = floatval[3];

    if (totparms == 6)
    {
        xx3rd =      floatval[4];
        yy3rd =      floatval[5];
    }
    return CMDARG_FRACTAL_PARAM;
}

// orbit corners=?,?,?,?
int parameter_processor::param_orbit_corners()
{
    set_orbit_corners = false;
    if (floatparms != totparms
            || (totparms != 0 && totparms != 4 && totparms != 6))
    {
        return bad_parameter();
    }
    oxmin = floatval[0];
    ox3rd = oxmin;
    oxmax = floatval[1];
    oymin = floatval[2];
    oy3rd = oymin;
    oymax = floatval[3];

    if (totparms == 6)
    {
        ox3rd =      floatval[4];
        oy3rd =      floatval[5];
    }
    set_orbit_corners = true;
    keep_scrn_coords = true;
    return CMDARG_FRACTAL_PARAM;
}

// screencoords=?
int parameter_processor::param_screen_coords()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    keep_scrn_coords = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM;
}

// orbitdrawmode=?
int parameter_processor::param_orbit_draw_mode()
{
    if (charval[0] != 'l' && charval[0] != 'r' && charval[0] != 'f')
    {
        return bad_parameter();
    }
    drawmode = charval[0];
    return CMDARG_FRACTAL_PARAM;
}

// viewwindows=?,?,?,?,?
int parameter_processor::param_view_windows()
{
    if (totparms > 5 || floatparms - intparms > 2 || intparms > 4)
    {
        return bad_parameter();
    }
    viewwindow = true;
    viewreduction = 4.2F;  // reset default values
    finalaspectratio = screenaspect;
    viewcrop = true;
    viewydots = 0;
    viewxdots = viewydots;

    if ((totparms > 0) && (floatval[0] > 0.001))
        viewreduction = (float) floatval[0];
    if ((totparms > 1) && (floatval[1] > 0.001))
        finalaspectratio = (float) floatval[1];
    if ((totparms > 2) && (yesnoval[2] == 0))
        viewcrop = yesnoval[2] != 0;
    if ((totparms > 3) && (intval[3] > 0))
        viewxdots = intval[3];
    if ((totparms == 5) && (intval[4] > 0))
        viewydots = intval[4];
    return CMDARG_FRACTAL_PARAM;
}

// center-mag=?,?,?[,?,?,?]
int parameter_processor::param_center_mag()
{
    int dec;

    if ((totparms != floatparms)
            || (totparms != 0 && totparms < 3)
            || (totparms >= 3 && floatval[2] == 0.0))
    {
        return bad_parameter();
    }
    if (fractype == fractal_type::CELLULAR)
        return CMDARG_FRACTAL_PARAM; // skip setting the corners
    usemag = true;
    if (totparms == 0)
        return CMDARG_NONE; // turns center-mag mode on
    initcorners = true;
    // dec = get_max_curarg_len(floatvalstr, totparms);
    sscanf(floatvalstr[2], "%Lf", &Magnification);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
    {
        return bad_parameter();
    }

    dec = getpower10(Magnification) + 4; // 4 digits of padding sounds good

    if ((dec <= DBL_DIG+1 && debugflag != debug_flags::force_arbitrary_precision_math)
            || debugflag == debug_flags::prevent_arbitrary_precision_math)
    {
        // rough estimate that double is OK
        Xctr = floatval[0];
        Yctr = floatval[1];
        Xmagfactor = 1;
        Rotation = 0;
        Skew = 0;
        if (floatparms > 3)
            Xmagfactor = floatval[3];
        if (Xmagfactor == 0)
            Xmagfactor = 1;
        if (floatparms > 4)
            Rotation = floatval[4];
        if (floatparms > 5)
            Skew = floatval[5];
        // calculate bounds
        cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        return CMDARG_FRACTAL_PARAM;
    }
    else
    {
        // use arbitrary precision
        int saved;
        initcorners = true;
        bf_math_type old_bf_math = bf_math;
        if (bf_math == bf_math_type::NONE || dec > decimals)
            init_bf_dec(dec);
        if (old_bf_math == bf_math_type::NONE)
        {
            for (int k = 0; k < MAXPARAMS; k++)
                floattobf(bfparms[k], param[k]);
        }
        usemag = true;
        saved = save_stack();
        bXctr            = alloc_stack(bflength+2);
        bYctr            = alloc_stack(bflength+2);
        get_bf(bXctr, floatvalstr[0]);
        get_bf(bYctr, floatvalstr[1]);
        Xmagfactor = 1;
        Rotation = 0;
        Skew = 0;
        if (floatparms > 3)
            Xmagfactor = floatval[3];
        if (Xmagfactor == 0)
            Xmagfactor = 1;
        if (floatparms > 4)
            Rotation = floatval[4];
        if (floatparms > 5)
            Skew = floatval[5];
        // calculate bounds
        cvtcornersbf(bXctr, bYctr, Magnification, Xmagfactor, Rotation, Skew);
        bfcornerstofloat();
        restore_stack(saved);
        return CMDARG_FRACTAL_PARAM;
    }
}

// aspectdrift=?
int parameter_processor::param_aspect_drift()
{
    if (floatparms != 1 || floatval[0] < 0)
    {
        return bad_parameter();
    }
    aspectdrift = (float) floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// invert=?,?,?
int parameter_processor::param_invert()
{
    if (totparms != floatparms || (totparms != 1 && totparms != 3))
    {
        return bad_parameter();
    }
    inversion[0] = floatval[0];
    invert = (inversion[0] != 0.0) ? totparms : 0;
    if (totparms == 3)
    {
        inversion[1] = floatval[1];
        inversion[2] = floatval[2];
    }
    return CMDARG_FRACTAL_PARAM;
}

// olddemmcolors=?
int parameter_processor::param_old_demm_colors()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    old_demm_colors = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// askvideo=?
int parameter_processor::param_ask_video()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    askvideo = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// float=?
int parameter_processor::param_float()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
#ifndef XFRACT
    usr_floatflag = yesnoval[0] != 0;
#else
    usr_floatflag = true; // must use floating point
#endif
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// fastrestore=?
int parameter_processor::param_fast_restore()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    fastrestore = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// orgfrmdir=?
int parameter_processor::param_org_frm_dir()
{
    if (valuelen > (FILE_MAX_DIR-1))
    {
        return bad_parameter();
    }
    if (!isadirectory(value))
    {
        return bad_parameter();
    }
    orgfrmsearch = true;
    orgfrmdir = value;
    fix_dirname(orgfrmdir);
    return CMDARG_NONE;
}

// biomorph=?
int parameter_processor::param_biomorph()
{
    usr_biomorph = numval;
    return CMDARG_FRACTAL_PARAM;
}

// orbitsave=?
int parameter_processor::param_orbit_save()
{
    if (charval[0] == 's')
        orbitsave |= 2;
    else if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    orbitsave |= yesnoval[0];
    return CMDARG_FRACTAL_PARAM;
}

// bailout=?
int parameter_processor::param_bailout()
{
    if (floatval[0] < 1 || floatval[0] > 2100000000L)
    {
        return bad_parameter();
    }
    bailout = (long) floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// bailoutest=?
int parameter_processor::param_bailout_test()
{
    if (strcmp(value, "mod") == 0)
        bailoutest = bailouts::Mod;
    else if (strcmp(value, "real") == 0)
        bailoutest = bailouts::Real;
    else if (strcmp(value, "imag") == 0)
        bailoutest = bailouts::Imag;
    else if (strcmp(value, "or") == 0)
        bailoutest = bailouts::Or;
    else if (strcmp(value, "and") == 0)
        bailoutest = bailouts::And;
    else if (strcmp(value, "manh") == 0)
        bailoutest = bailouts::Manh;
    else if (strcmp(value, "manr") == 0)
        bailoutest = bailouts::Manr;
    else
    {
        return bad_parameter();
    }
    setbailoutformula(bailoutest);
    return CMDARG_FRACTAL_PARAM;
}

// symmetry=?
int parameter_processor::param_symmetry()
{
    if (strcmp(value, "xaxis") == 0)
        forcesymmetry = symmetry_type::X_AXIS;
    else if (strcmp(value, "yaxis") == 0)
        forcesymmetry = symmetry_type::Y_AXIS;
    else if (strcmp(value, "xyaxis") == 0)
        forcesymmetry = symmetry_type::XY_AXIS;
    else if (strcmp(value, "origin") == 0)
        forcesymmetry = symmetry_type::ORIGIN;
    else if (strcmp(value, "pi") == 0)
        forcesymmetry = symmetry_type::PI_SYM;
    else if (strcmp(value, "none") == 0)
        forcesymmetry = symmetry_type::NONE;
    else
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM;
}

// deprecated print parameters
bool parameter_processor::is_print_parameter()
{
    return (variable == "printer")
            || (variable == "printfile")
            || (variable == "rleps")
            || (variable == "colorps")
            || (variable == "epsf")
            || (variable == "title")
            || (variable == "translate")
            || (variable == "plotstyle")
            || (variable == "halftone")
            || (variable == "linefeed")
            || (variable == "comport");
}

// sound=?,?,?
int parameter_processor::param_sound()
{
    if (totparms > 5)
    {
        return bad_parameter();
    }
    soundflag = SOUNDFLAG_OFF; // start with a clean slate, add bits as we go
    if (totparms == 1)
        soundflag = SOUNDFLAG_SPEAKER; // old command, default to PC speaker

    /* soundflag is used as a bitfield... bit 0,1,2 used for whether sound
       is modified by an orbits x,y,or z component. and also to turn it on
       or off (0==off, 1==beep (or yes), 2==x, 3==y, 4==z),
       Bit 3 is used for flagging the PC speaker sound,
       Bit 4 for OPL3 FM soundcard output,
       Bit 5 will be for midi output (not yet),
       Bit 6 for whether the tone is quantised to the nearest 'proper' note
        (according to the western, even tempered system anyway) */

    if (charval[0] == 'n' || charval[0] == 'o')
        soundflag &= ~SOUNDFLAG_ORBITMASK;
    else if ((strncmp(value, "ye", 2) == 0) || (charval[0] == 'b'))
        soundflag |= SOUNDFLAG_BEEP;
    else if (charval[0] == 'x')
        soundflag |= SOUNDFLAG_X;
    else if (charval[0] == 'y' && strncmp(value, "ye", 2) != 0)
        soundflag |= SOUNDFLAG_Y;
    else if (charval[0] == 'z')
        soundflag |= SOUNDFLAG_Z;
    else
    {
        return bad_parameter();
    }
    if (totparms > 1)
    {
        soundflag &= SOUNDFLAG_ORBITMASK; // reset options
        for (int i = 1; i < totparms; i++)
        {
            // this is for 2 or more options at the same time
            if (charval[i] == 'f')
            { // (try to)switch on opl3 fm synth
                if (driver_init_fm())
                    soundflag |= SOUNDFLAG_OPL3_FM;
                else
                    soundflag &= ~SOUNDFLAG_OPL3_FM;
            }
            else if (charval[i] == 'p')
                soundflag |= SOUNDFLAG_SPEAKER;
            else if (charval[i] == 'm')
                soundflag |= SOUNDFLAG_MIDI;
            else if (charval[i] == 'q')
                soundflag |= SOUNDFLAG_QUANTIZED;
            else
                return bad_parameter();
        }
    }
    return CMDARG_NONE;
}

// Hertz=?
int parameter_processor::param_hertz()
{
    basehertz = numval;
    return CMDARG_NONE;
}

// Volume =?
int parameter_processor::param_volume()
{
    fm_vol = numval & 0x3F; // 63
    return CMDARG_NONE;
}

int parameter_processor::param_attenuate()
{
    if (charval[0] == 'n')
        hi_atten = 0;
    else if (charval[0] == 'l')
        hi_atten = 1;
    else if (charval[0] == 'm')
        hi_atten = 2;
    else if (charval[0] == 'h')
        hi_atten = 3;
    else
        return bad_parameter();
    return CMDARG_NONE;
}

int parameter_processor::param_polyphony()
{
    if (numval > 9)
        return bad_parameter();
    polyphony = abs(numval -1);
    return CMDARG_NONE;
}

// wavetype = ?
int parameter_processor::param_wave_type()
{
    fm_wavetype = numval & 0x0F;
    return CMDARG_NONE;
}

// attack = ?
int parameter_processor::param_attack()
{
    fm_attack = numval & 0x0F;
    return CMDARG_NONE;
}

// decay = ?
int parameter_processor::param_decay()
{
    fm_decay = numval & 0x0F;
    return CMDARG_NONE;
}

// sustain = ?
int parameter_processor::param_sustain()
{
    fm_sustain = numval & 0x0F;
    return CMDARG_NONE;
}

// srelease = ?
int parameter_processor::param_s_release()
{
    fm_release = numval & 0x0F;
    return CMDARG_NONE;
}

// Scalemap=?,?,?,?,?,?,?,?,?,?,?
int parameter_processor::param_scale_map()
{
    if (totparms != intparms)
        return bad_parameter();
    for (int counter = 0; counter <= 11; counter++)
        if ((totparms > counter) && (intval[counter] > 0)
                && (intval[counter] < 13))
            scale_map[counter] = intval[counter];
    return CMDARG_NONE;
}

// periodicity=?
int parameter_processor::param_periodicity()
{
    usr_periodicitycheck = 1;
    if ((charval[0] == 'n') || (numval == 0))
        usr_periodicitycheck = 0;
    else if (charval[0] == 'y')
        usr_periodicitycheck = 1;
    else if (charval[0] == 's')   // 's' for 'show'
        usr_periodicitycheck = -1;
    else if (numval == NON_NUMERIC)
    {
        return bad_parameter();
    }
    else if (numval != 0)
        usr_periodicitycheck = numval;
    if (usr_periodicitycheck > 255)
        usr_periodicitycheck = 255;
    if (usr_periodicitycheck < -255)
        usr_periodicitycheck = -255;
    return CMDARG_FRACTAL_PARAM;
}

// logmap=?
int parameter_processor::param_log_map()
{
    Log_Auto_Calc = false;          // turn this off if loading a PAR
    if (charval[0] == 'y')
        LogFlag = 1;                           // palette is logarithmic
    else if (charval[0] == 'n')
        LogFlag = 0;
    else if (charval[0] == 'o')
        LogFlag = -1;                          // old log palette
    else
        LogFlag = (long) floatval[0];
    return CMDARG_FRACTAL_PARAM;
}

// logmode=?
int parameter_processor::param_log_mode()
{
    Log_Fly_Calc = 0;                         // turn off if error
    Log_Auto_Calc = false;
    if (charval[0] == 'f')
        Log_Fly_Calc = 1;                      // calculate on the fly
    else if (charval[0] == 't')
        Log_Fly_Calc = 2;                      // force use of LogTable
    else if (charval[0] == 'a')
    {
        Log_Auto_Calc = true;       // force auto calc of logmap
    }
    else
    {
        return bad_parameter();
    }
    return CMDARG_FRACTAL_PARAM;
}

// internal use only
int parameter_processor::param_debug_flag()
{
    debugflag = numval;
    timerflag = (debugflag & benchmark_timer) != 0;       // separate timer flag
    debugflag &= ~benchmark_timer;
    return CMDARG_NONE;
}

int parameter_processor::param_rseed()
{
    rseed = numval;
    rflag = true;
    return CMDARG_FRACTAL_PARAM;
}

int parameter_processor::param_orbit_delay()
{
    orbit_delay = numval;
    return CMDARG_NONE;
}

int parameter_processor::param_orbit_interval()
{
    orbit_interval = numval;
    if (orbit_interval < 1)
        orbit_interval = 1;
    if (orbit_interval > 255)
        orbit_interval = 255;
    return CMDARG_NONE;
}

int parameter_processor::param_show_dot()
{
    show_dot = 15;
    if (totparms > 0)
    {
        autoshowdot = (char)0;
        if (isalpha(charval[0]))
        {
            if (strchr("abdm", (int) charval[0]) != nullptr)
                autoshowdot = charval[0];
            else
            {
                return bad_parameter();
            }
        }
        else
        {
            show_dot = numval;
            if (show_dot < 0)
                show_dot = -1;
        }
        if (totparms > 1 && intparms > 0)
            sizedot = intval[1];
        if (sizedot < 0)
            sizedot = 0;
    }
    return CMDARG_NONE;
}

// showorbit=yes|no
int parameter_processor::param_show_orbit()
{
    start_show_orbit = yesnoval[0] != 0;
    return CMDARG_NONE;
}

int parameter_processor::param_decomp()
{
    if (totparms != intparms || totparms < 1)
    {
        return bad_parameter();
    }
    decomp[0] = intval[0];
    decomp[1] = 0;
    if (totparms > 1) // backward compatibility
    {
        decomp[1] = intval[1];
        bailout = decomp[1];
    }
    return CMDARG_FRACTAL_PARAM;
}

int parameter_processor::param_dist_test()
{
    if (totparms != intparms || totparms < 1)
    {
        return bad_parameter();
    }
    usr_distest = (long) floatval[0];
    distestwidth = 71;
    if (totparms > 1)
        distestwidth = intval[1];
    if (totparms > 3 && intval[2] > 0 && intval[3] > 0)
    {
        pseudox = intval[2];
        pseudoy = intval[3];
    }
    else
    {
        pseudoy = 0;
        pseudox = pseudoy;
    }
    return CMDARG_FRACTAL_PARAM;
}

// formulafile=?
int parameter_processor::param_formula_file()
{
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (merge_pathnames(FormFileName, value, mode) < 0)
        init_msg(variable.c_str(), value, mode);
    return CMDARG_FRACTAL_PARAM;
}

// formulaname=?
int parameter_processor::param_formula_name()
{
    if (valuelen > ITEMNAMELEN)
    {
        return bad_parameter();
    }
    FormName = value;
    return CMDARG_FRACTAL_PARAM;
}

// lfile=?
int parameter_processor::param_l_file()
{
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (merge_pathnames(LFileName, value, mode) < 0)
        init_msg(variable.c_str(), value, mode);
    return CMDARG_FRACTAL_PARAM;
}

int parameter_processor::param_l_name()
{
    if (valuelen > ITEMNAMELEN)
    {
        return bad_parameter();
    }
    LName = value;
    return CMDARG_FRACTAL_PARAM;
}

// ifsfile=??
int parameter_processor::param_ifs_file()
{
    int existdir;
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    existdir = merge_pathnames(IFSFileName, value, mode);
    if (existdir == 0)
        reset_ifs_defn();
    else if (existdir < 0)
        init_msg(variable.c_str(), value, mode);
    return CMDARG_FRACTAL_PARAM;
}

int parameter_processor::param_ifs()
{
    if (valuelen > ITEMNAMELEN)
    {
        return bad_parameter();
    }
    IFSName = value;
    reset_ifs_defn();
    return CMDARG_FRACTAL_PARAM;
}

// parmfile=?
int parameter_processor::param_parm_file()
{
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (merge_pathnames(CommandFile, value, mode) < 0)
        init_msg(variable.c_str(), value, mode);
    return CMDARG_FRACTAL_PARAM;
}

// stereo=?
int parameter_processor::param_stereo()
{
    if ((numval < 0) || (numval > 4))
    {
        return bad_parameter();
    }
    g_glasses_type = numval;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// rotation=?/?/?
int parameter_processor::param_rotation()
{
    if (totparms != 3 || intparms != 3)
    {
        return bad_parameter();
    }
    XROT = intval[0];
    YROT = intval[1];
    ZROT = intval[2];
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// perspective=?
int parameter_processor::param_perspective()
{
    if (numval == NON_NUMERIC)
    {
        return bad_parameter();
    }
    ZVIEWER = numval;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// xyshift=?/?
int parameter_processor::param_x_y_shift()
{
    if (totparms != 2 || intparms != 2)
    {
        return bad_parameter();
    }
    XSHIFT = intval[0];
    YSHIFT = intval[1];
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// interocular=?
int parameter_processor::param_interocular()
{
    g_eye_separation = numval;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// converg=?
int parameter_processor::param_converge()
{
    xadjust = numval;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// crop=?
int parameter_processor::param_crop()
{
    if (totparms != 4 || intparms != 4
            || intval[0] < 0 || intval[0] > 100
            || intval[1] < 0 || intval[1] > 100
            || intval[2] < 0 || intval[2] > 100
            || intval[3] < 0 || intval[3] > 100)
    {
        return bad_parameter();
    }
    red_crop_left   = intval[0];
    red_crop_right  = intval[1];
    blue_crop_left  = intval[2];
    blue_crop_right = intval[3];
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// bright=?
int parameter_processor::param_bright()
{
    if (totparms != 2 || intparms != 2)
    {
        return bad_parameter();
    }
    red_bright  = intval[0];
    blue_bright = intval[1];
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// xyadjust=?/?
int parameter_processor::param_x_y_adjust()
{
    if (totparms != 2 || intparms != 2)
    {
        return bad_parameter();
    }
    xtrans = intval[0];
    ytrans = intval[1];
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// 3d=?/?/..
int parameter_processor::param_3d()
{
    if (strcmp(value, "overlay") == 0)
    {
        yesnoval[0] = 1;
        if (calc_status > calc_status_value::NO_FRACTAL) // if no image, treat same as 3D=yes
            overlay_3d = true;
    }
    else if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    display_3d = yesnoval[0] != 0 ? YES : NONE;
    initvars_3d();
    return display_3d != NONE ? (CMDARG_3D_PARAM | CMDARG_3D_YES) : CMDARG_3D_PARAM;
}

// sphere=?
int parameter_processor::param_sphere()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    SPHERE = yesnoval[0];
    return CMDARG_3D_PARAM;
}

// scalexyz=?/?/?
int parameter_processor::param_scale_x_y_z()
{
    if (totparms < 2 || intparms != totparms)
    {
        return bad_parameter();
    }
    XSCALE = intval[0];
    YSCALE = intval[1];
    if (totparms > 2)
        ROUGH = intval[2];
    return CMDARG_3D_PARAM;
}

// roughness=?
int parameter_processor::param_roughness()
{
    // "rough" is really scale z, but we add it here for convenience
    ROUGH = numval;
    return CMDARG_3D_PARAM;
}

// waterline=?
int parameter_processor::param_waterline()
{
    if (numval < 0)
    {
        return bad_parameter();
    }
    WATERLINE = numval;
    return CMDARG_3D_PARAM;
}

// filltype=?
int parameter_processor::param_fill_type()
{
    if (numval < -1 || numval > 6)
    {
        return bad_parameter();
    }
    FILLTYPE = numval;
    return CMDARG_3D_PARAM;
}

// lightsource=?/?/?
int parameter_processor::param_light_source()
{
    if (totparms != 3 || intparms != 3)
    {
        return bad_parameter();
    }
    XLIGHT = intval[0];
    YLIGHT = intval[1];
    ZLIGHT = intval[2];
    return CMDARG_3D_PARAM;
}

// smoothing=?
int parameter_processor::param_smoothing()
{
    if (numval < 0)
    {
        return bad_parameter();
    }
    LIGHTAVG = numval;
    return CMDARG_3D_PARAM;
}

// latitude=?/?
int parameter_processor::param_latitude()
{
    if (totparms != 2 || intparms != 2)
    {
        return bad_parameter();
    }
    THETA1 = intval[0];
    THETA2 = intval[1];
    return CMDARG_3D_PARAM;
}

// longitude=?/?
int parameter_processor::param_longitude()
{
    if (totparms != 2 || intparms != 2)
    {
        return bad_parameter();
    }
    PHI1 = intval[0];
    PHI2 = intval[1];
    return CMDARG_3D_PARAM;
}

// radius=?
int parameter_processor::param_radius()
{
    if (numval < 0)
    {
        return bad_parameter();
    }
    RADIUS = numval;
    return CMDARG_3D_PARAM;
}

// transparent?
int parameter_processor::param_transparent()
{
    if (totparms != intparms || totparms < 1)
    {
        return bad_parameter();
    }
    transparent[0] = intval[0];
    transparent[1] = transparent[0];
    if (totparms > 1)
        transparent[1] = intval[1];
    return CMDARG_3D_PARAM;
}

// preview?
int parameter_processor::param_preview()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    preview = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// showbox?
int parameter_processor::param_show_box()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    showbox = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// coarse=?
int parameter_processor::param_coarse()
{
    if (numval < 3 || numval > 2000)
    {
        return bad_parameter();
    }
    previewfactor = numval;
    return CMDARG_3D_PARAM;
}

// RANDOMIZE=?
int parameter_processor::param_randomize()
{
    if (numval < 0 || numval > 7)
    {
        return bad_parameter();
    }
    RANDOMIZE = numval;
    return CMDARG_3D_PARAM;
}

// ambient=?
int parameter_processor::param_ambient()
{
    if (numval < 0 || numval > 100)
    {
        return bad_parameter();
    }
    Ambient = numval;
    return CMDARG_3D_PARAM;
}

// haze=?
int parameter_processor::param_haze()
{
    if (numval < 0 || numval > 100)
    {
        return bad_parameter();
    }
    haze = numval;
    return CMDARG_3D_PARAM;
}

// fullcolor=?
int parameter_processor::param_full_color()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    Targa_Out = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// truecolor=?
int parameter_processor::param_true_color()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    truecolor = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// truemode=?
int parameter_processor::param_true_mode()
{
    truemode = 0;                               // use default if error
    if (charval[0] == 'd')
        truemode = 0;                            // use default color output
    if (charval[0] == 'i' || intval[0] == 1)
        truemode = 1;                            // use iterates output
    if (intval[0] == 2)
        truemode = 2;
    if (intval[0] == 3)
        truemode = 3;
    return CMDARG_FRACTAL_PARAM | CMDARG_3D_PARAM;
}

// usegrayscale?
int parameter_processor::param_use_gray_scale()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    grayflag = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// monitorwidth=?
int parameter_processor::param_monitor_width()
{
    if (totparms != 1 || floatparms != 1)
    {
        return bad_parameter();
    }
    AutoStereo_width  = floatval[0];
    return CMDARG_3D_PARAM;
}

// Targa Overlay?
int parameter_processor::param_targa_overlay()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    Targa_Overlay = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// background=?/?
int parameter_processor::param_background()
{
    if (totparms != 3 || intparms != 3)
    {
        return bad_parameter();
    }
    for (int i = 0; i < 3; i++)
    {
        if (intval[i] & ~0xff)
        {
            return bad_parameter();
        }
    }
    back_color[0] = (BYTE) intval[0];
    back_color[1] = (BYTE) intval[1];
    back_color[2] = (BYTE) intval[2];
    return CMDARG_3D_PARAM;
}

// lightname=?
int parameter_processor::param_light_name()
{
    if (valuelen > (FILE_MAX_PATH-1))
    {
        return bad_parameter();
    }
    if (first_init || mode == cmd_file::AT_AFTER_STARTUP)
    {
        light_name = value;
    }
    return CMDARG_NONE;
}

// RAY=?
int parameter_processor::param_ray()
{
    if (numval < 0 || numval > 6)
    {
        return bad_parameter();
    }
    RAY = numval;
    return CMDARG_3D_PARAM;
}

// BRIEF?
int parameter_processor::param_brief()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    BRIEF = yesnoval[0] != 0;
    return CMDARG_3D_PARAM;
}

// release
int parameter_processor::param_release()
{
    if (numval < 0)
    {
        return bad_parameter();
    }
    save_release = numval;
    return CMDARG_3D_PARAM;
}

// curdir=
int parameter_processor::param_cur_dir()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    checkcurdir = yesnoval[0] != 0;
    return CMDARG_NONE;
}

// virtual=
int parameter_processor::param_virtual()
{
    if (yesnoval[0] < 0)
    {
        return bad_parameter();
    }
    g_virtual_screens = yesnoval[0] != 0;
    return CMDARG_FRACTAL_PARAM;
}

// Some routines broken out of above so compiler doesn't run out of heap:

static void parse_textcolors(char const *value)
{
    int hexval;
    if (strcmp(value, "mono") == 0)
    {
        for (auto & elem : txtcolor)
            elem = BLACK*16+WHITE;
        /* C_HELP_CURLINK = C_PROMPT_INPUT = C_CHOICE_CURRENT = C_GENERAL_INPUT
                          = C_AUTHDIV1 = C_AUTHDIV2 = WHITE*16+BLACK; */
        txtcolor[28] = WHITE*16+BLACK;
        txtcolor[27] = txtcolor[28];
        txtcolor[20] = txtcolor[27];
        txtcolor[14] = txtcolor[20];
        txtcolor[13] = txtcolor[14];
        txtcolor[12] = txtcolor[13];
        txtcolor[6] = txtcolor[12];
        /* C_TITLE = C_HELP_HDG = C_HELP_LINK = C_PROMPT_HI = C_CHOICE_SP_KEYIN
                   = C_GENERAL_HI = C_DVID_HI = C_STOP_ERR
                   = C_STOP_INFO = BLACK*16+L_WHITE; */
        txtcolor[25] = BLACK*16+L_WHITE;
        txtcolor[24] = txtcolor[25];
        txtcolor[22] = txtcolor[24];
        txtcolor[17] = txtcolor[22];
        txtcolor[16] = txtcolor[17];
        txtcolor[11] = txtcolor[16];
        txtcolor[5] = txtcolor[11];
        txtcolor[2] = txtcolor[5];
        txtcolor[0] = txtcolor[2];
    }
    else
    {
        int k = 0;
        while (k < sizeof(txtcolor))
        {
            if (*value == 0)
                break;
            if (*value != '/')
            {
                sscanf(value, "%x", &hexval);
                int i = (hexval / 16) & 7;
                int j = hexval & 15;
                if (i == j || (i == 0 && j == 8)) // force contrast
                    j = 15;
                txtcolor[k] = (BYTE)(i * 16 + j);
                value = strchr(value, '/');
                if (value == nullptr)
                    break;
            }
            ++value;
            ++k;
        }
    }
}

static int parse_colors(char const *value)
{
    if (*value == '@')
    {
        if (merge_pathnames(MAP_name, &value[1], cmd_file::AT_CMD_LINE_SET_NAME) < 0)
            init_msg("", &value[1], cmd_file::AT_CMD_LINE_SET_NAME);
        if ((int)strlen(value) > FILE_MAX_PATH || ValidateLuts(MAP_name.c_str()))
            goto badcolor;
        if (display_3d != display_3d_modes::NONE)
        {
            mapset = true;
        }
        else
        {
            if (merge_pathnames(colorfile, &value[1], cmd_file::AT_CMD_LINE_SET_NAME) < 0)
                init_msg("", &value[1], cmd_file::AT_CMD_LINE_SET_NAME);
            colorstate = 2;
        }
    }
    else
    {
        int smooth = 0;
        int i = 0;
        while (*value)
        {
            if (i >= 256)
                goto badcolor;
            if (*value == '<')
            {
                if (i == 0 || smooth
                        || (smooth = atoi(value+1)) < 2
                        || (value = strchr(value, '>')) == nullptr)
                    goto badcolor;
                i += smooth;
                ++value;
            }
            else
            {
                for (int j = 0; j < 3; ++j)
                {
                    int k = *(value++);
                    if (k < '0')
                    {
                        goto badcolor;
                    }
                    else if (k <= '9')
                    {
                        k -= '0';
                    }
                    else if (k < 'A')
                    {
                        goto badcolor;
                    }
                    else if (k <= 'Z')
                    {
                        k -= ('A'-10);
                    }
                    else if (k < '_' || k > 'z')
                    {
                        goto badcolor;
                    }
                    else
                    {
                        k -= ('_'-36);
                    }
                    g_dac_box[i][j] = (BYTE)k;
                    if (smooth)
                    {
                        int spread = smooth + 1;
                        int start = i - spread;
                        int cnum = 0;
                        if ((k - (int)g_dac_box[start][j]) == 0)
                        {
                            while (++cnum < spread)
                                g_dac_box[start+cnum][j] = (BYTE)k;
                        }
                        else
                        {
                            while (++cnum < spread)
                                g_dac_box[start+cnum][j] =
                                        (BYTE)((cnum *g_dac_box[i][j]
                                                + (i-(start+cnum))*g_dac_box[start][j]
                                                + spread/2)
                                                / (BYTE) spread);
                        }
                    }
                }
                smooth = 0;
                ++i;
            }
        }
        if (smooth)
            goto badcolor;
        while (i < 256)
        { // zap unset entries
            g_dac_box[i][2] = 40;
            g_dac_box[i][1] = g_dac_box[i][2];
            g_dac_box[i][0] = g_dac_box[i][1];
            ++i;
        }
        colorstate = 1;
    }
    colors_preloaded = true;
    memcpy(old_dac_box, g_dac_box, 256*3);
    return CMDARG_NONE;
    badcolor:
    return -1;
}

static void argerror(char const *badarg)      // oops. couldn't decode this
{
    std::string spillover;
    if ((int) strlen(badarg) > 70)
    {
        spillover = std::string(&badarg[0], &badarg[70]);
        badarg = spillover.c_str();
    }
    std::string msg{"Oops. I couldn't understand the argument:\n  "};
    msg += badarg;

    if (first_init)       // this is 1st call to cmdfiles
    {
        msg += "\n"
                "\n"
                "(see the Startup Help screens or documentation for a complete\n"
                " argument list with descriptions)";
    }
    stopmsg(STOPMSG_NONE, msg.c_str());
    if (init_batch != batch_modes::NONE)
    {
        init_batch = batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE;
        goodbye();
    }
}

void set_3d_defaults()
{
    ROUGH     = 30;
    WATERLINE = 0;
    ZVIEWER   = 0;
    XSHIFT    = 0;
    YSHIFT    = 0;
    xtrans    = 0;
    ytrans    = 0;
    LIGHTAVG  = 0;
    Ambient   = 20;
    RANDOMIZE = 0;
    haze      = 0;
    back_color[0] = 51;
    back_color[1] = 153;
    back_color[2] = 200;
    if (SPHERE)
    {
        PHI1      =  180;
        PHI2      =  0;
        THETA1    =  -90;
        THETA2    =  90;
        RADIUS    =  100;
        FILLTYPE  = 2;
        XLIGHT    = 1;
        YLIGHT    = 1;
        ZLIGHT    = 1;
    }
    else
    {
        XROT      = 60;
        YROT      = 30;
        ZROT      = 0;
        XSCALE    = 90;
        YSCALE    = 90;
        FILLTYPE  = 0;
        XLIGHT    = 1;
        YLIGHT    = -1;
        ZLIGHT    = 1;
    }
}

// copy a big number from a string, up to slash
static int get_bf(bf_t bf, char const *curarg)
{
    char const *s;
    s = strchr(curarg, '/');
    if (s)
    {
        std::string buff(curarg, s);
        strtobf(bf, buff.c_str());
    }
    else
    {
        strtobf(bf, curarg);
    }
    return 0;
}

// Get length of current args
int get_curarg_len(char const *curarg)
{
    char const *s;
    s = strchr(curarg, '/');
    if (s)
    {
        return s - curarg;
    }
    else
    {
        return strlen(curarg);
    }
}

// Get max length of current args
int get_max_curarg_len(char const *floatvalstr[], int totparms)
{
    int tmp, max_str;
    max_str = 0;
    for (int i = 0; i < totparms; i++)
    {
        tmp = get_curarg_len(floatvalstr[i]);
        if (tmp > max_str)
            max_str = tmp;
    }
    return max_str;
}

// mode = 0 command line @filename
//        1 sstools.ini
//        2 <@> command after startup
//        3 command line @filename/setname
// this is like stopmsg() but can be used in cmdfiles()
// call with NULL for badfilename to get pause for driver_get_key()
int init_msg(char const *cmdstr, char const *badfilename, cmd_file mode)
{
    char const *modestr[4] =
            {"command line", "sstools.ini", "PAR file", "PAR file"};
    static int row = 1;

    if (init_batch == batch_modes::NORMAL)
    { // in batch mode
        if (badfilename)
            return -1;
    }
    char cmd[80];
    strncpy(cmd, cmdstr, 30);
    cmd[29] = 0;

    if (*cmd)
        strcat(cmd, "=");
    std::string msg;
    if (badfilename)
        // cppcheck-suppress constStatement
        msg = std::string{"Can't find "} + cmd + badfilename
                + ", please check " + modestr[static_cast<int>(mode)];
    if (first_init)
    {     // & cmdfiles hasn't finished 1st try
        if (row == 1 && badfilename)
        {
            driver_set_for_text();
            driver_put_string(0, 0, 15, "Fractint found the following problems when parsing commands: ");
        }
        if (badfilename)
            driver_put_string(row++, 0, 7, msg.c_str());
        else if (row > 1)
        {
            driver_put_string(++row, 0, 15, "Press Escape to abort, any other key to continue");
            driver_move_cursor(row+1, 0);
            dopause(2);  // defer getakeynohelp until after parsing
        }
    }
    else if (badfilename)
        stopmsg(STOPMSG_NONE, msg.c_str());
    return 0;
}

// defer pause until after parsing so we know if in batch mode
void dopause(int action)
{
    static unsigned char needpause = 0;
    switch (action)
    {
    case 0:
        if (init_batch == batch_modes::NONE)
        {
            if (needpause == 1)
                driver_get_key();
            else if (needpause == 2)
            if (getakeynohelp() == FIK_ESC)
                goodbye();
        }
        needpause = 0;
        break;
    case 1:
    case 2:
        needpause = (char)action;
        break;
    default:
        break;
    }
}

/*
   Crude function to detect a floating point number. Intended for
   use with arbitrary precision.
*/
static bool isabigfloat(char const *str)
{
    // [+|-]numbers][.]numbers[+|-][e|g]numbers
    bool result = true;
    char const *s = str;
    int numdot = 0;
    int nume = 0;
    int numsign = 0;
    while (*s != 0 && *s != '/' && *s != ' ')
    {
        if (*s == '-' || *s == '+')
            numsign++;
        else if (*s == '.')
            numdot++;
        else if (*s == 'e' || *s == 'E' || *s == 'g' || *s == 'G')
            nume++;
        else if (!isdigit(*s))
        {
            result = false;
            break;
        }
        s++;
    }
    if (numdot > 1 || numsign > 2 || nume > 1)
        result = false;
    return result;
}
