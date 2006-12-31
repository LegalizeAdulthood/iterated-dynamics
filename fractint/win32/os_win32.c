#include <assert.h>
#include <string.h>
#include <signal.h>
#include <sys/timeb.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <shlwapi.h>

#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "drivers.h"
#include "externs.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "wintext.h"

/* External declarations */
extern void check_samename(void);

HINSTANCE g_instance = NULL;

typedef enum
{
	FE_UNKNOWN = -1,
	FE_IMAGE_INFO,					/* TAB */
	FE_RESTART,						/* INSERT */
	FE_SELECT_VIDEO_MODE,			/* DELETE */
	FE_EXECUTE_COMMANDS,			/* @ */
	FE_COMMAND_SHELL,				/* d */
	FE_ORBITS_WINDOW,				/* o */
	FE_SELECT_FRACTAL_TYPE,			/* t */
	FE_TOGGLE_JULIA,				/* FIK_SPACE */
	FE_TOGGLE_INVERSE,				/* j */
	FE_PRIOR_IMAGE,					/* h */
	FE_REVERSE_HISTORY,				/* ^H */
	FE_BASIC_OPTIONS,				/* x */
	FE_EXTENDED_OPTIONS,			/* y */
	FE_TYPE_SPECIFIC_PARAMS,		/* z */
	FE_PASSES_OPTIONS,				/* p */
	FE_VIEW_WINDOW_OPTIONS,			/* v */
	FE_3D_PARAMS,					/* i */
	FE_BROWSE_PARAMS,				/* ^B */
	FE_EVOLVER_PARAMS,				/* ^E */
	FE_SOUND_PARAMS,				/* ^F */
	FE_SAVE_IMAGE,					/* s */
	FE_LOAD_IMAGE,					/* r */
	FE_3D_TRANSFORM,				/* 3 */
	FE_3D_OVERLAY,					/* # */
	FE_SAVE_CURRENT_PARAMS,			/* b */
	FE_PRINT_IMAGE,					/* ^P */
	FE_GIVE_COMMAND_STRING,			/* g */
	FE_QUIT,						/* ESC */
	FE_COLOR_CYCLING_MODE,			/* c */
	FE_ROTATE_PALETTE_DOWN,			/* - */
	FE_ROTATE_PALETTE_UP,			/* + */
	FE_EDIT_PALETTE,				/* e */
	FE_MAKE_STARFIELD,				/* a */
	FE_ANT_AUTOMATON,				/* ^A */
	FE_STEREOGRAM,					/* ^S */
	FE_VIDEO_F1,
	FE_VIDEO_F2,
	FE_VIDEO_F3,
	FE_VIDEO_F4,
	FE_VIDEO_F5,
	FE_VIDEO_F6,
	FE_VIDEO_F7,
	FE_VIDEO_F8,
	FE_VIDEO_F9,
	FE_VIDEO_F10,
	FE_VIDEO_F11,
	FE_VIDEO_F12,
	FE_VIDEO_AF1,
	FE_VIDEO_AF2,
	FE_VIDEO_AF3,
	FE_VIDEO_AF4,
	FE_VIDEO_AF5,
	FE_VIDEO_AF6,
	FE_VIDEO_AF7,
	FE_VIDEO_AF8,
	FE_VIDEO_AF9,
	FE_VIDEO_AF10,
	FE_VIDEO_AF11,
	FE_VIDEO_AF12,
	FE_VIDEO_CF1,
	FE_VIDEO_CF2,
	FE_VIDEO_CF3,
	FE_VIDEO_CF4,
	FE_VIDEO_CF5,
	FE_VIDEO_CF6,
	FE_VIDEO_CF7,
	FE_VIDEO_CF8,
	FE_VIDEO_CF9,
	FE_VIDEO_CF10,
	FE_VIDEO_CF11,
	FE_VIDEO_CF12
} fractint_event;

/* Global variables (yuck!) */
int g_and_color;
BYTE block[256] = { 0 };
int boxx[2304] = { 0 };
int boxy[1024] = { 0 };
int boxvalues[512] = { 0 };
int g_checked_vvs = 0;
int g_color_dark = 0;		/* darkest color in palette */
int g_color_bright = 0;		/* brightest color in palette */
int g_color_medium = 0;		/* nearest to medbright grey in palette
				   Zoom-Box values (2K x 2K screens max) */
int cpu, fpu;                        /* cpu, fpu flags */
unsigned char g_dac_box[256][3];
int g_dac_learn = 0;
int dacnorm = 0;
int g_dac_count = 0;
int g_disk_flag = 0;
int disktarga = 0;
int DivideOverflow = 0;
static char extrasegment[0x18000] = { 0 };
void *extraseg = &extrasegment[0];
int fake_lut = 0;
int finishrow = 0;
int fm_attack = 0;
int fm_decay = 0;
int fm_release = 0;
int fm_sustain = 0;
int fm_vol = 0;
int fm_wavetype = 0;
int g_good_mode = 0;
int g_got_real_dac = 0;
int hi_atten = 0;
int inside_help = 0;
int g_is_true_color = 0;
long linitx = 0;
long linity = 0;
int lookatmouse = 0;
BYTE olddacbox[256][3];
int overflow = 0;
int polyphony = 0;
int g_really_ega = 0;
char rlebuf[512] = { 0 };
int g_row_count = 0;
long savebase = 0;				/* base clock ticks */ 
long saveticks = 0;				/* save after this many ticks */ 
unsigned int strlocn[10*1024] = { 0 };
BYTE suffix[2048] = { 0 };
char supervga_list[] =
{
	'a', 'h', 'e', 'a', 'd', 'a',		//supervga_list   db      "aheada"
	0, 0,								//aheada  dw      0
	'a', 't', 'i', ' ', ' ', ' ',		//        db      "ati   "
	0, 0,								//ativga  dw      0
	'c', 'h', 'i', ' ', ' ', ' ',		//        db      "chi   "
	0, 0,								//chipstech dw    0
	'e', 'v', 'e', ' ', ' ', ' ',		//        db      "eve   "
	0, 0,								//everex  dw      0
	'g', 'e', 'n', ' ', ' ', ' ',		//        db      "gen   "
	0, 0,								//genoa   dw      0
	'n', 'c', 'r', ' ', ' ', ' ',		//        db      "ncr   "
	0, 0,								//ncr     dw      0
	'o', 'a', 'k', ' ', ' ', ' ',		//        db      "oak   "
	0, 0,								//oaktech dw      0
	'p', 'a', 'r', ' ', ' ', ' ',		//        db      "par   "
	0, 0,								//paradise dw     0
	't', 'r', 'i', ' ', ' ', ' ',		//        db      "tri   "
	0, 0,								//trident dw      0
	't', 's', 'e', 'n', 'g', '3',		//        db      "tseng3"
	0, 0,								//tseng   dw      0
	't', 's', 'e', 'n', 'g', '4',		//        db      "tseng4"
	0, 0,								//tseng4  dw      0
	'v', 'i', 'd', ' ', ' ', ' ',		//        db      "vid   "
	0, 0,								//video7  dw      0
	'a', 'h', 'e', 'a', 'd', 'b',		//        db      "aheadb"
	0, 0,								//aheadb  dw      0
	'v', 'e', 's', 'a', ' ', ' ',		//        db      "vesa  "
	0, 0,								//vesa    dw      0
	'c', 'i', 'r', 'r', 'u', 's',		//        db      "cirrus"
	0, 0,								//cirrus  dw      0
	't', '8', '9', '0', '0', ' ',		//        db      "t8900 "
	0, 0,								//t8900   dw      0
	'c', 'o', 'm', 'p', 'a', 'q',		//        db      "compaq"
	0, 0,								//compaq  dw      0
	'x', 'g', 'a', ' ', ' ', ' ',		//        db      "xga   "
	0, 0,								//xga     dw      0
	' ', ' ', ' ', ' ', ' ', ' ',		//        db      "      "        ; end-of-the-list
	0, 0								//        dw      0
};
int g_svga_type = 0;
int g_text_type = 0;
int g_text_cbase = 0;
int g_text_col = 0;
int g_text_rbase = 0;
int g_text_row = 0;
int g_text_safe = 0;
char tstack[4096] = { 0 };
int g_vesa_detect = 0;
int g_vesa_x_res = 0;
int g_vesa_y_res = 0;
int g_video_scroll = 0;
int g_video_start_x = 0;
int g_video_start_y = 0;
/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAXVIDEOMODES] =
{
	{
		"unused  mode             ", "                         ",
		0, 0, 0,
		0, 0, 0, 0, 0, 0
	}
};
VIDEOINFO vidtbl[MAXVIDEOMODES] = { 0 };
int g_vxdots = 0;

/* Global variables that should be phased out (old video mode stuff) */
int g_video_vram = 0;
int g_virtual_screens = 0;


/* Global functions
 *
 * These were either copied from a .c file under unix, or
 * they have assembly language equivalents that we provide
 * here in a slower C form for portability.
 */

/* keyboard_event
**
** Map a keypress into an event id.
*/
static fractint_event keyboard_event(int key)
{
	struct
	{
		int key;
		fractint_event event;
	}
	mapping[] =
	{
		FIK_CTL_A,	FE_ANT_AUTOMATON,
		FIK_CTL_B,	FE_BROWSE_PARAMS,
		FIK_CTL_E,	FE_EVOLVER_PARAMS,
		FIK_CTL_F,	FE_SOUND_PARAMS,
		FIK_BACKSPACE,	FE_REVERSE_HISTORY,
		FIK_TAB,		FE_IMAGE_INFO,
		FIK_CTL_P,	FE_PRINT_IMAGE,
		FIK_CTL_S,	FE_STEREOGRAM,
		FIK_ESC,		FE_QUIT,
		FIK_SPACE,	FE_TOGGLE_JULIA,
		FIK_INSERT,		FE_RESTART,
		DELETE,		FE_SELECT_VIDEO_MODE,
		'@',		FE_EXECUTE_COMMANDS,
		'#',		FE_3D_OVERLAY,
		'3',		FE_3D_TRANSFORM,
		'a',		FE_MAKE_STARFIELD,
		'b',		FE_SAVE_CURRENT_PARAMS,
		'c',		FE_COLOR_CYCLING_MODE,
		'd',		FE_COMMAND_SHELL,
		'e',		FE_EDIT_PALETTE,
		'g',		FE_GIVE_COMMAND_STRING,
		'h',		FE_PRIOR_IMAGE,
		'j',		FE_TOGGLE_INVERSE,
		'i',		FE_3D_PARAMS,
		'o',		FE_ORBITS_WINDOW,
		'p',		FE_PASSES_OPTIONS,
		'r',		FE_LOAD_IMAGE,
		's',		FE_SAVE_IMAGE,
		't',		FE_SELECT_FRACTAL_TYPE,
		'v',		FE_VIEW_WINDOW_OPTIONS,
		'x',		FE_BASIC_OPTIONS,
		'y',		FE_EXTENDED_OPTIONS,
		'z',		FE_TYPE_SPECIFIC_PARAMS,
		'-',		FE_ROTATE_PALETTE_DOWN,
		'+',		FE_ROTATE_PALETTE_UP
	};
	key = tolower(key);
	{
		int i;
		for (i = 0; i < NUM_OF(mapping); i++)
		{
			if (mapping[i].key == key)
			{
				return mapping[i].event;
			}
		}
	}

	return FE_UNKNOWN;
}

/* Return available stack space ... shouldn't be needed in Win32, should it? */
long stackavail()
{
	/* TODO */
	_ASSERTE(FALSE);
	return 8192;
}

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       z = divide(x, y, n);       z = x / y;
*/
long divide(long x, long y, int n)
{
    return (long) (((float) x) / ((float) y)*(float) (1 << n));
}

/*
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x, y, n)
;
*/

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 */
long multiply(long x, long y, int n)
{
    register long l;
    l = (long) (((float) x) * ((float) y)/(float) (1 << n));
    if (l == 0x7fffffff)
	{
		overflow = 1;
    }
    return l;
}

/*
; ****************** Function getakey() *****************************
; **************** Function keypressed() ****************************

;       'getakey()' gets a key from either a "normal" or an enhanced
;       keyboard.   Returns either the vanilla ASCII code for regular
;       keys, or 1000+(the scan code) for special keys (like F1, etc)
;       Use of this routine permits the Control-Up/Down arrow keys on
;       enhanced keyboards.
;
;       The concept for this routine was "borrowed" from the MSKermit
;       SCANCHEK utility
;
;       'keypressed()' returns a zero if no keypress is outstanding,
;       and the value that 'getakey()' will return if one is.  Note
;       that you must still call 'getakey()' to flush the character.
;       As a sidebar function, calls 'help()' if appropriate, or
;       'tab_display()' if appropriate.
;       Think of 'keypressed()' as a super-'kbhit()'.
*/
int keybuffer = 0;

/*
 * This is the low level key handling routine.
 * If block is set, we want to block before returning, since we are waiting
 * for a key press.
 * We also have to handle the slide file, etc.
 */

int getkeyint(int block)
{
    int ch;
    int curkey;
    if (keybuffer)
	{
		ch = keybuffer;
		keybuffer = 0;
		return ch;
    }
    curkey = driver_get_key(); //0);
    if (slides==1 && curkey == FIK_ESC)
	{
		stopslideshow();
		return 0;
    }

    if (curkey==0 && slides==1)
	{
		curkey = slideshw();
    }

    if (curkey==0 && block)
	{
		curkey = driver_get_key(); //1);
		if (slides==1 && curkey == FIK_ESC)
		{
			stopslideshow();
			return 0;
		}
    }

    if (curkey && slides==2)
	{
		recordshw(curkey);
    }

    return curkey;
}

/*
 * This routine returns a keypress
 */
int getakey(void)
{
    int ch;

    do
	{
		ch = getkeyint(1);
    }
	while (ch==0);
    return ch;
}

/*
 * This routine returns a key, ignoring F1
 */
int getakeynohelp(void)
{
	int ch;
	do
	{
		ch = driver_get_key();
	}
	while (ch == FIK_F1);
	return ch;
}

int keypressed(void)
{
	int ch = keybuffer;
	if (ch)
	{
		return ch;
	}
	ch = wintext_getkeypress(0);
	if (ch)
	{
		keybuffer = wintext_getkeypress(1);
		if (FIK_F1 == ch && helpmode)
		{
			if (!inside_help)
			{
				keybuffer = 0;
				inside_help = 1;
				help(0);
				inside_help = 0;
				ch = 0;
			}
		}
		else if (FIK_TAB == ch && tabmode)
		{
			keybuffer = 0;
			tab_display();
			ch = 0;
		}
	}

	return ch;
}

/* Wait for a key.
 * This should be used instead of:
 * while (!keypressed()) {}
 * If timeout=1, waitkeypressed will time out after .5 sec.
 */
int waitkeypressed(int timeout)
{
    while (!keybuffer)
	{
		keybuffer = getkeyint(1);
		if (timeout)
			break;
    }
    return driver_key_pressed();
}

/*
; *************** Function find_special_colors ********************

;       Find the darkest and brightest colors in palette, and a medium
;       color which is reasonably bright and reasonably grey.
*/
void
find_special_colors (void)
{
	int maxb = 0;
	int minb = 9999;
	int med = 0;
	int maxgun, mingun;
	int brt;
	int i;

	g_color_dark = 0;
	g_color_medium = 7;
	g_color_bright = 15;

	if (colors == 2)
	{
		g_color_medium = 1;
		g_color_bright = 1;
		return;
	}

	if (!(g_got_real_dac || fake_lut))
		return;

	for (i = 0; i < colors; i++)
	{
		brt = (int) g_dac_box[i][0] + (int) g_dac_box[i][1] + (int) g_dac_box[i][2];
		if (brt > maxb)
		{
			maxb = brt;
			g_color_bright = i;
		}
		if (brt < minb)
		{
			minb = brt;
			g_color_dark = i;
		}
		if (brt < 150 && brt > 80)
		{
			maxgun = mingun = (int) g_dac_box[i][0];
			if ((int) g_dac_box[i][1] > (int) g_dac_box[i][0])
			{
				maxgun = (int) g_dac_box[i][1];
			}
			else
			{
				mingun = (int) g_dac_box[i][1];
			}
			if ((int) g_dac_box[i][2] > maxgun)
			{
				maxgun = (int) g_dac_box[i][2];
			}
			if ((int) g_dac_box[i][2] < mingun)
			{
				mingun = (int) g_dac_box[i][2];
			}
			if (brt - (maxgun - mingun) / 2 > med)
			{
				g_color_medium = i;
				med = brt - (maxgun - mingun) / 2;
			}
		}
	}
}

static HANDLE s_find_context = INVALID_HANDLE_VALUE;
static char s_find_base[MAX_PATH] = { 0 };
static WIN32_FIND_DATA s_find_data = { 0 };

int fr_findfirst(char *path)       /* Find 1st file (or subdir) meeting path/filespec */
{
	if (s_find_context != INVALID_HANDLE_VALUE)
	{
		BOOL result = FindClose(s_find_context);
		_ASSERTE(result);
	}
	SetLastError(0);
	s_find_context = FindFirstFile(path, &s_find_data);
	if (INVALID_HANDLE_VALUE == s_find_context)
	{
		return GetLastError() || -1;
	}

	strncpy(s_find_base, path, NUM_OF(s_find_base));
	{
		char *whack = strrchr(s_find_base, '\\');
		if (whack != NULL)
		{
			whack[1] = 0;
		}
	}
	_snprintf(DTA.path, NUM_OF(DTA.path), "%s%s", s_find_base, s_find_data.cFileName);

	return 0;
}

int fr_findnext()
{
	BOOL result = FALSE;
	_ASSERTE(INVALID_HANDLE_VALUE != s_find_context);
	result = FindNextFile(s_find_context, &s_find_data);
	if (result != 0)
	{
		DWORD code = GetLastError();
		_ASSERTE(ERROR_NO_MORE_FILES == code);
		return code || -1;
	}

	_snprintf(DTA.path, NUM_OF(DTA.path), "%s%s", s_find_base, s_find_data.cFileName);

	return 0;
}

int get_sound_params(void)
{
	/* TODO */
	_ASSERTE(FALSE);
	return(0);
}

/*
; long readticker() returns current bios ticker value
*/
long readticker(void)
{
	/* TODO */
	_ASSERTE(FALSE);
	return 0;
}

void windows_pump_messages(BOOL nowait)
{
	while (1)
	{
		MSG msg;
		int status;
		
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == 0)
		{
			if (nowait)
			{
				return;
			}
		}

		status = GetMessage(&msg, NULL, 0, 0);
		if (status == -1)
		{
			/* error */
			OutputDebugString("!windows_pump_messages status == -1, error\n");
			return;
		}
		else if (status > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (status == 0)
		{
			OutputDebugString("!windows_pump_messages status == 0\n");
		}
	}
}

/*
; ***************** Function delay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
*/
void delay(int delaytime)
{
	wintext_look_for_activity(FALSE);
	Sleep(delaytime);
}

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spindac(int dir, int inc)
{
	int i, top;
	unsigned char tmp[3];
	unsigned char *dacbot;
	int len;
	if (colors < 16)
		return;
	if (g_is_true_color && truemode)
		return;
	if (dir != 0 && rotate_lo < colors && rotate_lo < rotate_hi)
	{
		top = rotate_hi > colors ? colors - 1 : rotate_hi;
		dacbot = (unsigned char *) g_dac_box + 3 * rotate_lo;
		len = (top - rotate_lo) * 3 * sizeof (unsigned char);
		if (dir > 0)
		{
			for (i = 0; i < inc; i++)
			{
				bcopy(dacbot, tmp, 3 * sizeof(unsigned char));
				bcopy(dacbot + 3 * sizeof(unsigned char), dacbot, len);
				bcopy(tmp, dacbot + len, 3 * sizeof(unsigned char));
			}
		}
		else
		{
			for (i = 0; i < inc; i++)
			{
				bcopy(dacbot + len, tmp, 3 * sizeof(unsigned char));
				bcopy(dacbot, dacbot + 3 * sizeof(unsigned char), len);
				bcopy(tmp, dacbot, 3 * sizeof(unsigned char));
			}
		}
	}
	//writevideopalette();
	delay(colors - g_dac_count - 1);
}

//; ************* function scroll_relative(bycol, byrow) ***********************
//
//; scroll_relative ------------------------------------------------------------
//; * relative screen center scrolling, arguments passed are signed deltas
//; ------------------------------------------------------------16-08-2002-ChCh-
//
//scroll_relative proc    bycol: word, byrow: word
//        cmp     g_video_scroll,0        ; is the scrolling on?
//        jne     okletsmove              ;  ok, lets move
//        jmp     staystill               ;  no, stay still
//okletsmove:
//        mov     cx,g_video_start_x         ; where we already are..
//        mov     dx,g_video_start_y
//        add     cx,video_cofs_x         ; find the screen center
//        add     dx,video_cofs_y
//        add     cx,bycol                ; add the relative shift
//        add     dx,byrow
//        call    VESAscroll              ; replace this later with a variable
//staystill:
//        ret
//scroll_relative endp

void scroll_relative(int bycol, int byrow)
{
	if (g_video_scroll)
	{
		// blt pixels around :-)
	}
}

/*
; adapter_detect:
;       This routine performs a few quick checks on the type of
;       video adapter installed.
;       It sets variable g_text_safe,
;       and fills in a few bank-switching routines.
*/
void
adapter_detect(void)
{
	static int done_detect = 0;

	if (done_detect)
		return;
	done_detect = 1;
	g_text_safe = 2;
}

void erasesegment(int segaddress, int segvalue)
{
	_ASSERTE(FALSE);
}

void put_a_char(int ch)
{
	/* TODO */
	_ASSERTE(FALSE);
}

/*
; ********* Function gettruecolor(xdot, ydot, &red, &green, &blue) **************
;       Return the color on the screen at the (xdot, ydot) point
*/
void gettruecolor(int xdot, int ydot, int *red, int *green, int *blue)
{
	/* TODO */
	_ASSERTE(FALSE);
	*red = 0;
	*green = 0;
	*blue = 0;
}

/*
; **************** Function home()  ********************************

;       Home the cursor (called before printfs)
*/
void home(void)
{
	driver_move_cursor(0, 0);
	g_text_row = 0;
	g_text_col = 0;
}

/*
; ****************** Function initasmvars() *****************************
*/
void initasmvars(void)
{
	if (cpu != 0)
	{
		return;
	}
	overflow = 0;

	/* set cpu type */
	cpu = 1;

	/* set fpu type */
	/* not needed, set fpu in sstools.ini */
}

int isadirectory(char *s)
{
	return PathFileExists(s);
}

/*
; ******* Function puttruecolor(xdot, ydot, red, green, blue) *************
;       write the color on the screen at the (xdot, ydot) point
*/
void puttruecolor(int xdot, int ydot, int red, int green, int blue)
{
	/* TODO */
	_ASSERTE(FALSE);
}

/* tenths of millisecond timewr routine */
/* static struct timeval tv_start; */

void restart_uclock(void)
{
	/* TODO */
}


/*
**  usec_clock()
**
**  An analog of the clock() function, usec_clock() returns a number of
**  type uclock_t (defined in UCLOCK.H) which represents the number of
**  microseconds past midnight. Analogous to CLK_TCK is UCLK_TCK, the
**  number which a usec_clock() reading must be divided by to yield
**  a number of seconds.
*/
typedef unsigned long uclock_t;
uclock_t usec_clock(void)
{
   uclock_t result = 0;
   /* TODO */
   _ASSERTE(FALSE);

   return result; 
}

void swapnormread(void)
{
	/* TODO */
	_ASSERTE(FALSE);
}

void swapnormwrite(void)
{
	/* TODO */
	_ASSERTE(FALSE);
}

/*
; ************* function scroll_center(tocol, torow) *************************

; scroll_center --------------------------------------------------------------
; * this is meant to be an universal scrolling redirection routine
;   (if scrolling will be coded for the other modes too, the VESAscroll
;   call should be replaced by a preset variable (like in proc newbank))
; * arguments passed are the coords of the screen center because
;   there is no universal way to determine physical screen resolution
; ------------------------------------------------------------12-08-2002-ChCh-
*/
void scroll_center(int tocol, int torow)
{
	/* TODO */
	_ASSERTE(FALSE);
}

void showfreemem(void)
{
	/* TODO */
	_ASSERTE(FALSE);
}

long fr_farfree(void)
{
	/* TODO */
	return 0x8FFFFL;
}

unsigned long GetDiskSpace(void)
{
	/* TODO */
	return 0x7FFFFFFF;
}

void windows_shell_to_dos(void)
{
	STARTUPINFO si =
	{
		sizeof(si)
	};
	PROCESS_INFORMATION pi = { 0 };
	char *comspec = getenv("COMSPEC");

	if (NULL == comspec)
	{
		comspec = "cmd.exe";
	}
	if (CreateProcess(NULL, comspec, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		DWORD status = WaitForSingleObject(pi.hProcess, 1000);
		while (WAIT_TIMEOUT == status)
		{
			wintext_look_for_activity(0);
			status = WaitForSingleObject(pi.hProcess, 1000); 
		}
		CloseHandle(pi.hProcess);
	}
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdLine, int show)
{
	g_instance = instance;
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
	return main(__argc, __argv);
}
