/* d_win32.c
 *
 * Routines for a Win32 GDI driver for fractint.
 */
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

#include "WinText.h"
#include "frame.h"
#include "plot.h"
#include "d_win32.h"
#include "ods.h"

extern HINSTANCE g_instance;

#define DI(name_) Win32BaseDriver *name_ = (Win32BaseDriver *) drv

/*
; New (Apr '90) mouse code by Pieter Branderhorst follows.
; The variable lookatmouse controls it all.  Callers of keypressed and
; getakey should set lookatmouse to:
;      0  ignore the mouse entirely
;     <0  only test for left button click; if it occurs return fake key
;           number 0-lookatmouse
;      1  return enter key for left button, arrow keys for mouse movement,
;           mouse sensitivity is suitable for graphics cursor
;      2  same as 1 but sensitivity is suitable for text cursor
;      3  specials for zoombox, left/right double-clicks generate fake
;           keys, mouse movement generates a variety of fake keys
;           depending on state of buttons
; Mouse movement is accumulated & saved across calls.  Eg if mouse has been
; moved up-right quickly, the next few calls to getakey might return:
;      right,right,up,right,up
; Minor jiggling of the mouse generates no keystroke, and is forgotten (not
; accumulated with additional movement) if no additional movement in the
; same direction occurs within a short interval.
; Movements on angles near horiz/vert are treated as horiz/vert; the nearness
; tolerated varies depending on mode.
; Any movement not picked up by calling routine within a short time of mouse
; stopping is forgotten.  (This does not apply to button pushes in modes<3.)
; Mouseread would be more accurate if interrupt-driven, but with the usage
; in fractint (tight getakey loops while mouse active) there's no need.

; translate table for mouse movement -> fake keys
mousefkey dw   1077,1075,1080,1072  ; right,left,down,up     just movement
        dw        0,   0,1081,1073  ;            ,pgdn,pgup  + left button
        dw    1144,1142,1147,1146  ; kpad+,kpad-,cdel,cins  + rt   button
        dw    1117,1119,1118,1132  ; ctl-end,home,pgdn,pgup + mid/multi
*/
int mousefkey[16] =
{
	FIK_RIGHT_ARROW,	FIK_LEFT_ARROW,	FIK_DOWN_ARROW,		FIK_UP_ARROW,
	0,					0,				FIK_PAGE_DOWN,		FIK_PAGE_UP,
	FIK_CTL_PLUS,		FIK_CTL_MINUS,	FIK_CTL_DEL,		FIK_CTL_INSERT,
	FIK_CTL_END,		FIK_CTL_HOME,	FIK_CTL_PAGE_DOWN,	FIK_CTL_PAGE_UP
};

int lookatmouse = LOOK_MOUSE_NONE;
static int previous_look_mouse = LOOK_MOUSE_NONE;
static int mousetime = 0;				/* time of last mouseread call */
static int mlbtimer = 0;				/* time of left button 1st click */
static int mrbtimer = 0;				/* time of right button 1st click */
static int mhtimer = 0;					/* time of last horiz move */
static int mvtimer = 0;					/* time of last vert  move */
static int mhmickeys = 0;				/* pending horiz movement */
static int mvmickeys = 0;				/* pending vert  movement */
static int mbstatus = 0;				/* status of mouse buttons: MOUSE_CLICK_{NONE, LEFT, RIGHT} */
static int mbclicks = 0;				/* had 1 click so far? &1 mlb, &2 mrb */
#define MOUSE_CLICK_NONE 0
#define MOUSE_CLICK_LEFT 1
#define MOUSE_CLICK_RIGHT 2
int mouse_x = 0;
int mouse_y = 0;

/* timed save variables, handled by readmouse: */
static int savechktime = 0;				/* time of last autosave check */
long savebase = 0;						/* base clock ticks */
long saveticks = 0;						/* save after this many ticks */
int finishrow = 0;						/* save when this row is finished */

/*
DclickTime    equ 9   ; ticks within which 2nd click must occur
JitterTime    equ 6   ; idle ticks before turfing unreported mickeys
TextHSens     equ 22  ; horizontal sensitivity in text mode
TextVSens     equ 44  ; vertical sensitivity in text mode
GraphSens     equ 5   ; sensitivity in graphics mode; gets lower @ higher res
ZoomSens      equ 20  ; sensitivity for zoom box sizing/rotation
TextVHLimit   equ 6   ; treat angles < 1:6  as straight
GraphVHLimit  equ 14  ; treat angles < 1:14 as straight
ZoomVHLimit   equ 1   ; treat angles < 1:1  as straight
JitterMickeys equ 3   ; mickeys to ignore before noticing motion
*/
#define DclickTime 9
#define JitterTime 6
#define TextHSens 22
#define TextVSens 44
#define GraphSens 5
#define ZoomSens 20
#define TextVHLimit 6
#define GraphVHLimit 14
#define ZoomVHLimit 1
#define JitterMickeys 3

int left_button_pressed(void)
{
	return g_frame.button_down[BUTTON_LEFT];
}

int left_button_released(void)
{
	return !g_frame.button_down[BUTTON_LEFT];
}

int right_button_pressed(void)
{
	return g_frame.button_down[BUTTON_RIGHT];
}

int right_button_released(void)
{
	return !g_frame.button_down[BUTTON_RIGHT];
}

int button_states(void)
{
	return (g_frame.button_down[BUTTON_LEFT] ? MOUSE_CLICK_LEFT : 0) |
		(g_frame.button_down[BUTTON_RIGHT] ? MOUSE_CLICK_RIGHT : 0);
}

void get_mouse_motion(void)
{
	mouse_x = g_frame.start_x;
	mouse_y = g_frame.start_y;
}

/*
MOUSE_GET_POSITION_STATUS	equ 03h
	INT 33,3 - Get Mouse Position and Button Status
		AX = 03
	on return:
		CX = horizontal (X) position  (0..639)
		DX = vertical (Y) position  (0..199)
		BX = button status:

			|F-8|7|6|5|4|3|2|1|0|  Button Status
			|  | | | | | | | `---- left button (1 = pressed)
			|  | | | | | | `----- right button (1 = pressed)
			`------------------- unused

		- values returned in CX, DX are the same regardless of video mode
*/
void mouse_get_position_status(int *horiz, int *vert, int *status)
{
}

/*
MOUSE_GET_BUTTON_PRESS		equ 05h
	INT 33,5 - Get Mouse Button Press Information
		AX = 5
		BX = 0	left button
			1	right button
	on return:
		BX = count of button presses (0-32767), set to zero after call
		CX = horizontal position at last press
		DX = vertical position at last press
		AX = status:

			|F-8|7|6|5|4|3|2|1|0|  Button Status
			|  | | | | | | | `---- left button (1 = pressed)
			|  | | | | | | `----- right button (1 = pressed)
			`------------------- unused
*/
void mouse_get_button_press(int button, int *count, int *horiz, int *vert, int *status)
{
}

/*
MOUSE_GET_BUTTON_RELEASE	equ 06h
	INT 33,6 - Get Mouse Button Release Information
		AX = 6
		BX = 0	left button
			1	right button
	on return:
		BX = count of button releases (0-32767), set to zero after call
		CX = horizontal position at last release
		DX = vertical position at last release
		AX = status

			|F-8|7|6|5|4|3|2|1|0|  Button status
			|  | | | | | | | `---- left button (1 = pressed)
			|  | | | | | | `----- right button (1 = pressed)
			`------------------- unused
*/
void mouse_get_button_release(int button, int *count, int *horiz, int *vert, int *status)
{
}

/*
MOUSE_GET_MOTION_COUNTERS	equ 0Bh
	INT 33,B - Read Mouse Motion Counters
		AX = 0B
	on return:
		CX = horizontal mickey count (-32768 to 32767)
		DX = vertical mickey count (-32768 to 32767)

		- count values are 1/200 inch intervals (1/200 in. = 1 mickey)
*/
void mouse_get_motion_counters(int *horizontal_mickeys, int *vertical_mickeys)
{
}

/*
; New (Apr '90) mouse code by Pieter Branderhorst follows.
; The variable lookatmouse controls it all.  Callers of keypressed and
; getakey should set lookatmouse to:
;      0  ignore the mouse entirely
;     <0  only test for left button click; if it occurs return fake key
;           number 0-lookatmouse
;      1  return enter key for left button, arrow keys for mouse movement,
;           mouse sensitivity is suitable for graphics cursor
;      2  same as 1 but sensitivity is suitable for text cursor
;      3  specials for zoombox, left/right double-clicks generate fake
;           keys, mouse movement generates a variety of fake keys
;           depending on state of buttons
; Mouse movement is accumulated & saved across calls.  Eg if mouse has been
; moved up-right quickly, the next few calls to getakey might return:
;      right,right,up,right,up
; Minor jiggling of the mouse generates no keystroke, and is forgotten (not
; accumulated with additional movement) if no additional movement in the
; same direction occurs within a short interval.
; Movements on angles near horiz/vert are treated as horiz/vert; the nearness
; tolerated varies depending on mode.
; Any movement not picked up by calling routine within a short time of mouse
; stopping is forgotten.  (This does not apply to button pushes in modes<3.)
; Mouseread would be more accurate if interrupt-driven, but with the usage
; in fractint (tight getakey loops while mouse active) there's no need.

; translate table for mouse movement -> fake keys
mousefkey dw   1077,1075,1080,1072  ; right,left,down,up     just movement
        dw        0,   0,1081,1073  ;            ,pgdn,pgup  + left button
        dw    1144,1142,1147,1146  ; kpad+,kpad-,cdel,cins  + rt   button
        dw    1117,1119,1118,1132  ; ctl-end,home,pgdn,pgup + mid/multi

DclickTime    equ 9   ; ticks within which 2nd click must occur
JitterTime    equ 6   ; idle ticks before turfing unreported mickeys
TextHSens     equ 22  ; horizontal sensitivity in text mode
TextVSens     equ 44  ; vertical sensitivity in text mode
GraphSens     equ 5   ; sensitivity in graphics mode; gets lower @ higher res
ZoomSens      equ 20  ; sensitivity for zoom box sizing/rotation
TextVHLimit   equ 6   ; treat angles < 1:6  as straight
GraphVHLimit  equ 14  ; treat angles < 1:14 as straight
ZoomVHLimit   equ 1   ; treat angles < 1:1  as straight
JitterMickeys equ 3   ; mickeys to ignore before noticing motion

BIOS_MOUSE					equ 33h
MOUSE_GET_POSITION_STATUS	equ 03h
MOUSE_GET_BUTTON_PRESS		equ 05h
MOUSE_GET_BUTTON_RELEASE	equ 06h
MOUSE_GET_MOTION_COUNTERS	equ 0Bh
MOUSE_LEFT					equ 0h
MOUSE_RIGHT					equ 1h

mouseread proc near USES bx cx dx
        local   moveaxis:word

        ; check if it is time to do an autosave
        cmp     saveticks,0     ; autosave timer running?
        je      mouse0          ;  nope
        sub     ax,ax           ; reset ES to BIOS data area
        mov     es,ax           ;  see notes at mouse1 in similar code
tickread:
        mov     ax,es:046ch     ; obtain the current timer value
        cmp     ax,savechktime  ; a new clock tick since last check?
        je      mouse0          ;  nope, save a dozen opcodes or so
        mov     dx,es:046eh     ; high word of ticker
        cmp     ax,es:046ch     ; did a tick get counted just as we looked?
        jne     tickread        ; yep, reread both words to be safe
        mov     savechktime,ax
        sub     ax,savebase     ; calculate ticks since timer started
        sbb     dx,savebase+2
        jns     tickcompare
        add     ax,0b0h         ; wrapped past midnight, add a day
        adc     dx,018h
tickcompare:
        cmp     dx,saveticks+2  ; check if past autosave time
        jb      mouse0
        ja      ticksavetime
        cmp     ax,saveticks
        jb      mouse0
ticksavetime:                   ; it is time to do a save
        mov     ax,finishrow
        cmp     ax,-1           ; waiting for the end of a row before save?
        jne     tickcheckrow    ;  yup, go check row
        cmp     calc_status,1   ; safety check, calc active?
        jne     tickdosave      ;  nope, don't check type of calc
        cmp     got_status,0    ; 1pass or 2pass?
        je      ticknoterow     ;  yup
        cmp     got_status,1    ; solid guessing?
        jne     tickdosave      ;  not 1pass, 2pass, ssg, so save immediately
ticknoterow:
        mov     ax,currow       ; note the current row
        mov     finishrow,ax    ;  ...
        jmp     short mouse0    ; and keep working for now
tickcheckrow:
        cmp     ax,currow       ; started a new row since timer went off?
        je      mouse0          ;  nope, don't do the save yet
tickdosave:
        mov     timedsave,1     ; tell mainline what's up
        mov     ax,9999         ; a dummy key value, never gets used
        jmp     mouseret

mouse0: ; now the mouse stuff
        cmp     mouse,-1
        jne     mouseidle       ; no mouse, that was easy
        mov     ax,lookatmouse
        cmp     ax,prevlamouse
        je      mouse1

        ; lookatmouse changed, reset everything
        mov     prevlamouse,ax
        mov     mbclicks,0
        mov     mbstatus,0
        mov     mhmickeys,0
        mov     mvmickeys,0
        ; note: don't use int 33 func 0 nor 21 to reset, they're SLOW
        mov     ax,MOUSE_GET_BUTTON_RELEASE          ; reset button counts by reading them
        mov     bx,MOUSE_LEFT
        int     BIOS_MOUSE
        mov     ax,MOUSE_GET_BUTTON_RELEASE
        mov     bx,MOUSE_RIGHT
        int     BIOS_MOUSE
        mov     ax,MOUSE_GET_BUTTON_PRESS
        mov     bx,MOUSE_LEFT
        int     BIOS_MOUSE
        mov     ax,MOUSE_GET_MOTION_COUNTERS          ; reset motion counters by reading
        int     BIOS_MOUSE
        mov     ax,lookatmouse

mouse1: or      ax,ax
        jz      mouseidle       ; check nothing when lookatmouse=0
        ; following code directly accesses bios tick counter; it would be
        ; better not to rely on addr (use int 1A instead) but old PCs don't
        ; have the required int, the addr is constant in bios to date, and
        ; fractint startup already counts on it, so:
        mov     ax,0            ; reset ES to BIOS data area
        mov     es,ax           ;  ...
        mov     dx,es:46ch      ; obtain the current timer value
        cmp     dx,mousetime
        ; if timer same as last call, skip int 33s:  reduces expense and gives
        ; caller a chance to read all pending stuff and paint something
        jne     mnewtick
        cmp     lookatmouse,0   ; interested in anything other than left button?
        jl      mouseidle       ; nope, done
        jmp     mouse5

mouseidle:
        clc                     ; tell caller no mouse activity this time
        ret

mnewtick: ; new tick, read buttons and motion
        mov     mousetime,dx    ; note current timer
        cmp     lookatmouse,3
        je      mouse2          ; skip button press if mode 3

        ; check press of left button
        mov     ax,MOUSE_GET_BUTTON_PRESS          ; get button press info
        mov     bx,MOUSE_LEFT            ; for left button
        int     BIOS_MOUSE
        or      bx,bx
        jnz     mleftb
        cmp     lookatmouse,0
        jl      mouseidle       ; exit if nothing but left button matters
        jmp     mouse3          ; not mode 3 so skip past button release stuff
mleftb: mov     ax,13
        cmp     lookatmouse,0
        jg      mouser          ; return fake key enter
        mov     ax,lookatmouse  ; return fake key 0-lookatmouse
        neg     ax
mouser: jmp     mouseret

mouse2: ; mode 3, check for double clicks
        mov     ax,MOUSE_GET_BUTTON_RELEASE          ; get button release info
        mov     bx,MOUSE_LEFT            ; left button
        int     BIOS_MOUSE
        mov     dx,mousetime
        cmp     bx,1            ; left button released?
        jl      msnolb          ; go check timer if not
        jg      mslbgo          ; double click
        test    mbclicks,1      ; had a 1st click already?
        jnz     mslbgo          ; yup, double click
        mov     mlbtimer,dx     ; note time of 1st click
        or      mbclicks,1
        jmp     short mousrb
mslbgo: and     mbclicks,0ffh-1
        mov     ax,13           ; fake key enter
        jmp     mouseret
msnolb: sub     dx,mlbtimer     ; clear 1st click if its been too long
        cmp     dx,DclickTime
        jb      mousrb
        and     mbclicks,0ffh-1 ; forget 1st click if any
        ; next all the same code for right button
mousrb: mov     ax,MOUSE_GET_BUTTON_RELEASE          ; get button release info
        mov     bx,MOUSE_RIGHT            ; right button
        int     BIOS_MOUSE
        ; now much the same as for left
        mov     dx,mousetime
        cmp     bx,1
        jl      msnorb
        jg      msrbgo
        test    mbclicks,2
        jnz     msrbgo
        mov     mrbtimer,dx
        or      mbclicks,2
        jmp     short mouse3
msrbgo: and     mbclicks,0ffh-2
        mov     ax,1010         ; fake key ctl-enter
        jmp     mouseret
msnorb: sub     dx,mrbtimer
        cmp     dx,DclickTime
        jb      mouse3
        and     mbclicks,0ffh-2

        ; get buttons state, if any changed reset mickey counters
mouse3: mov     ax,MOUSE_GET_MOTION_COUNTERS          ; get button status
        int     BIOS_MOUSE
        and     bl,7            ; just the button bits
        cmp     bl,mbstatus     ; any changed?
        je      mouse4
        mov     mbstatus,bl     ; yup, reset stuff
        mov     mhmickeys,0
        mov     mvmickeys,0
        mov     ax,MOUSE_GET_MOTION_COUNTERS
        int     BIOS_MOUSE             ; reset driver's mickeys by reading them

        ; get motion counters, forget any jiggle
mouse4: mov     ax,MOUSE_GET_MOTION_COUNTERS          ; get motion counters
        int     BIOS_MOUSE
        mov     bx,mousetime    ; just to have it in a register
        cmp     cx,0            ; horiz motion?
        jne     moushm          ; yup, go accum it
        mov     ax,bx
        sub     ax,mhtimer
        cmp     ax,JitterTime   ; timeout since last horiz motion?
        jb      mousev
        mov     mhmickeys,0
        jmp     short mousev
moushm: mov     mhtimer,bx      ; note time of latest motion
        add     mhmickeys,cx
        ; same as above for vertical movement:
mousev: cmp     dx,0            ; vert motion?
        jne     mousvm
        mov     ax,bx
        sub     ax,mvtimer
        cmp     ax,JitterTime
        jb      mouse5
        mov     mvmickeys,0
        jmp     short mouse5
mousvm: mov     mvtimer,bx
        add     mvmickeys,dx

        ; pick the axis with largest pending movement
mouse5: mov     bx,mhmickeys
        or      bx,bx
        jns     mchkv
        neg     bx              ; make it +ve
mchkv:  mov     cx,mvmickeys
        or      cx,cx
        jns     mchkmx
        neg     cx
mchkmx: mov     moveaxis,0      ; flag that we're going horiz
        cmp     bx,cx           ; horiz>=vert?
        jge     mangle
        xchg    bx,cx           ; nope, use vert
        mov     moveaxis,1      ; flag that we're going vert

        ; if moving nearly horiz/vert, make it exactly horiz/vert
mangle: mov     ax,TextVHLimit
        cmp     lookatmouse,2   ; slow (text) mode?
        je      mangl2
        mov     ax,GraphVHLimit
        cmp     lookatmouse,3   ; special mode?
        jne     mangl2
        cmp     mbstatus,0      ; yup, any buttons down?
        je      mangl2
        mov     ax,ZoomVHLimit  ; yup, special zoom functions
mangl2: mul     cx              ; smaller axis * limit
        cmp     ax,bx
        ja      mchkmv          ; min*ratio <= max?
        cmp     moveaxis,0      ; yup, clear the smaller movement axis
        jne     mzeroh
        mov     mvmickeys,0
        jmp     short mchkmv
mzeroh: mov     mhmickeys,0

        ; pick sensitivity to use
mchkmv: cmp     lookatmouse,2   ; slow (text) mode?
        je      mchkmt
        mov     dx,ZoomSens+JitterMickeys
        cmp     lookatmouse,3   ; special mode?
        jne     mchkmg
        cmp     mbstatus,0      ; yup, any buttons down?
        jne     mchkm2          ; yup, use zoomsens
mchkmg: mov     dx,GraphSens
        mov     cx,sxdots       ; reduce sensitivity for higher res
mchkg2: cmp     cx,400          ; horiz dots >= 400?
        jl      mchkg3
        shr     cx,1            ; horiz/2
        shr     dx,1
        inc     dx              ; sensitivity/2+1
        jmp     short mchkg2
mchkg3: add     dx,JitterMickeys
        jmp     short mchkm2
mchkmt: mov     dx,TextVSens+JitterMickeys
        cmp     moveaxis,0
        jne     mchkm2
        mov     dx,TextHSens+JitterMickeys ; slower on X axis than Y

        ; is largest movement past threshold?
mchkm2: cmp     bx,dx
        jge     mmove
        jmp     mouseidle       ; no movement past threshold, return nothing

        ; set bx for right/left/down/up, and reduce the pending mickeys
mmove:  sub     dx,JitterMickeys
        cmp     moveaxis,0
        jne     mmovev
        cmp     mhmickeys,0
        jl      mmovh2
        sub     mhmickeys,dx    ; horiz, right
        mov     bx,0
        jmp     short mmoveb
mmovh2: add     mhmickeys,dx    ; horiz, left
        mov     bx,2
        jmp     short mmoveb
mmovev: cmp     mvmickeys,0
        jl      mmovv2
        sub     mvmickeys,dx    ; vert, down
        mov     bx,4
        jmp     short mmoveb
mmovv2: add     mvmickeys,dx    ; vert, up
        mov     bx,6

        ; modify bx if a button is being held down
mmoveb: cmp     lookatmouse,3
        jne     mmovek          ; only modify in mode 3
        cmp     mbstatus,1
        jne     mmovb2
        add     bx,8            ; modify by left button
        jmp     short mmovek
mmovb2: cmp     mbstatus,2
        jne     mmovb3
        add     bx,16           ; modify by rb
        jmp     short mmovek
mmovb3: cmp     mbstatus,0
        je      mmovek
        add     bx,24           ; modify by middle or multiple

        ; finally, get the fake key number
mmovek: mov     ax,mousefkey[bx]

mouseret:
        stc
        ret
mouseread endp
*/
/*
	http://heim.ifi.uio.no/~stanisls/helppc/int_table.html
	http://heim.ifi.uio.no/~stanisls/helppc/int_10.html INT 10 - Video BIOS Services
	http://heim.ifi.uio.no/~stanisls/helppc/int_16.html INT 16 - Keyboard BIOS Services
	http://heim.ifi.uio.no/~stanisls/helppc/int_33.html INT 33 - Mouse Function Calls
*/
int mouseread(int ch)
{
	int ax, bx, cx, dx;
	int moveaxis = 0;
	int ticker;

	if (ch != 0)
	{
		return ch;
	}

	/* now check for automatic/periodic saving... */
	ticker = readticker();
	if (saveticks && (ticker != savechktime))
	{
		savechktime = ticker;
		ticker -= savebase;
		if (ticker > saveticks)
		{
			if (finishrow == 1)
			{
				if (calc_status != CALCSTAT_IN_PROGRESS)
				{
					if ((got_status != GOT_STATUS_12PASS) && (got_status != GOT_STATUS_GUESSING))
					{
						finishrow = currow;
					}
				}
			}
			else if (currow != finishrow)
			{
				timedsave = TRUE;
				return 9999;
			}
		}
	}

	/* no save needed, now check some mouse movement stuff */

	/* did lookatmouse change since we were last here? */
	if (lookatmouse != previous_look_mouse)
	{
		/* lookatmouse changed, reset everything */
		previous_look_mouse = lookatmouse;
		mbclicks = 0;
		mbstatus = MOUSE_CLICK_NONE;
		mhmickeys = 0;
		mvmickeys = 0;
	}

	/* are we doing any mouse snooping? */
	if (lookatmouse != LOOK_MOUSE_NONE)
	{
		if (readticker() != mousetime)
		{
			mousetime = readticker();
			if (LOOK_MOUSE_ZOOM_BOX == lookatmouse)
			{
				if (left_button_released())
				{
					if (mbclicks & MOUSE_CLICK_LEFT)
					{
						mbclicks = ~MOUSE_CLICK_LEFT;
						return FIK_ENTER;
					}

					mlbtimer = mousetime;
					mbclicks |= MOUSE_CLICK_LEFT;
					goto mouse_right_button;
				}
				else
				{
					goto mouse_no_left_button;
				}
			}
			if (left_button_pressed())
			{
				return (lookatmouse > LOOK_MOUSE_NONE) ? FIK_ENTER : -lookatmouse;
			}
			if (lookatmouse < 0)
			{
				return 0;
			}
			goto mouse3;
		}
		if (lookatmouse >= 0)
		{
			goto mouse5;
		}
	}
	return 0;

mouse_no_left_button:
	if (mousetime - mlbtimer > DclickTime)
	{
		mbclicks = ~MOUSE_CLICK_LEFT;
	}

mouse_right_button:
	if (right_button_pressed())
	{
		if (mbclicks & MOUSE_CLICK_RIGHT)
		{
			mbclicks &= ~MOUSE_CLICK_RIGHT;
			return FIK_CTL_ENTER;
		}
		mrbtimer = mousetime;
		mbclicks |= MOUSE_CLICK_RIGHT;
		goto mouse3;
	}

	if (mousetime - mrbtimer > DclickTime)
	{
		mbclicks |= ~MOUSE_CLICK_LEFT;
	}

mouse3:
	if (button_states() != mbstatus)
	{
		mbstatus = button_states();
		mhmickeys = 0;
		mvmickeys = 0;
	}
	get_mouse_motion();
	if (mouse_x > 0)
	{
		goto mouse_horizontal_motion;
	}
	if (mousetime - mhtimer > JitterTime)
	{
		goto mouse_vertical;
	}
	mhmickeys = 0;
	goto mouse_vertical;

mouse_horizontal_motion:
	mhmickeys += mouse_x;

mouse_vertical:
	if (mouse_y > 0)
	{
		goto mouse_vertical_motion;
	}
	if (mousetime - mvtimer > JitterTime)
	{
		goto mouse5;
	}
	mvmickeys = 0;
	goto mouse5;

mouse_vertical_motion:
	mvtimer = mousetime;
	mvmickeys += mouse_y;

mouse5:
	bx = (mhmickeys > 0) ? mhmickeys : -mhmickeys;
	cx = (mvmickeys > 0) ? mvmickeys : -mvmickeys;
	moveaxis = 0;
	if (bx < cx)
	{
		int tmp = bx;
		bx = cx;
		cx = tmp;
		moveaxis = 1;
	}

	if (LOOK_MOUSE_TEXT == lookatmouse)
	{
		ax = TextVHLimit;
	}
	else if (LOOK_MOUSE_ZOOM_BOX != lookatmouse)
	{
		ax = GraphVHLimit;
	}
	else if (mbstatus == MOUSE_CLICK_NONE)
	{
		ax = ZoomVHLimit;
	}
	ax *= cx;
	if (ax > bx)
	{
		goto mouse_check_move;
	}
	if (moveaxis != 0)
	{
		mhmickeys = 0;
	}
	else
	{
		mvmickeys = 0;
	}

mouse_check_move:
	if (LOOK_MOUSE_TEXT == lookatmouse)
	{
		goto mchkmt;
	}
	dx = ZoomSens + JitterMickeys;
	if (LOOK_MOUSE_ZOOM_BOX != lookatmouse)
	{
		goto mchkmg;
	}
	if (mbstatus != MOUSE_CLICK_NONE)
	{
		goto mchkm2;
	}

mchkmg:
	dx = GraphSens;
	cx = sxdots;

	while (!(cx < 400))
	{
		cx >>= 1;
		dx >>= 1;
		dx++;
	}

	dx += JitterMickeys;
	goto mchkm2;

mchkmt:
	dx = TextVSens + JitterMickeys;
	if (moveaxis == 0)
	{
		dx = TextHSens + JitterMickeys;
	}

mchkm2:
	if (!(bx >= dx))
	{
		return 0;
	}

	dx -= JitterMickeys;
	if (moveaxis != 0)
	{
		goto mmovev;
	}
	if (mhmickeys < 0)
	{
		goto mmovh2;
	}
	mhmickeys -= dx;
	bx = 0;
	goto mmoveb;

mmovh2:
	mhmickeys += dx;
	bx = 2;
	goto mmoveb;

mmovev:
	if (mvmickeys < 0)
	{
		goto mmovv2;
	}
	mvmickeys -= dx;
	bx = 4;
	goto mmoveb;

mmovv2:
	mvmickeys += dx;
	bx = 6;

mmoveb:
	if (LOOK_MOUSE_ZOOM_BOX == lookatmouse)
	{
		goto mmovek;
	}

	switch (mbstatus)
	{
	case MOUSE_CLICK_LEFT: bx += 8; break;
	case MOUSE_CLICK_RIGHT: bx += 16; break;
	case MOUSE_CLICK_NONE: break;
	default:
		bx += 24;
	}

mmovek:
	_ASSERTE(bx < NUM_OF(mousefkey));
	ax = mousefkey[bx];

	return ax;
}

/* handle_special_keys
 *
 * First, do some slideshow processing.  Then handle F1 and TAB display.
 *
 * Because we want context sensitive help to work everywhere, with the
 * help to display indicated by a non-zero value in helpmode, we need
 * to trap the F1 key at a very low level.  The same is true of the
 * TAB display.
 *
 * What we do here is check for these keys and invoke their displays.
 * To avoid a recursive invoke of help(), a static is used to avoid
 * recursing on ourselves as help will invoke get key!
 */
static int
handle_special_keys(int ch)
{
	static int inside_help = 0;

	if (ch != 9999)
	{
		if (SLIDES_PLAY == g_slides)
		{
			if (ch == FIK_ESC)
			{
				stopslideshow();
				ch = 0;
			}
			else if (!ch)
			{
				ch = slideshw();
			}
		}
		else if ((SLIDES_RECORD == g_slides) && ch)
		{
			recordshw(ch);
		}
	}
	if (3000 == debugflag)
	{
		if ('~' == ch)
		{
			edit_text_colors();
			ch = 0;
		}
	}

	if (FIK_F1 == ch && helpmode && !inside_help)
	{
		inside_help = 1;
		help(0);
		inside_help = 0;
		ch = 0;
	}
	else if (FIK_TAB == ch && tabmode)
	{
		int old_tab = tabmode;
		tabmode = 0;
		tab_display();
		tabmode = old_tab;
		ch = 0;
	}

	return ch;
}

static void flush_output(void)
{
	static time_t start = 0;
	static long ticks_per_second = 0;
	static long last = 0;
	static long frames_per_second = 10;

	if (!ticks_per_second)
	{
		if (!start)
		{
			time(&start);
			last = readticker();
		}
		else
		{
			time_t now = time(NULL);
			long now_ticks = readticker();
			if (now > start)
			{
				ticks_per_second = (now_ticks - last)/((long) (now - start));
			}
		}
	}
	else
	{
		long now = readticker();
		if ((now - last)*frames_per_second > ticks_per_second)
		{
			driver_flush();
			frame_pump_messages(FALSE);
			last = now;
		}
	}
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* win32_terminate --
*
*	Cleanup windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Cleans up.
*
*----------------------------------------------------------------------
*/
void
win32_terminate(Driver *drv)
{
	DI(di);
	ODS("win32_terminate");

	/* plot_terminate(&di->plot); */
	wintext_destroy(&di->wintext);
	{
		int i;
		for (i = 0; i < NUM_OF(di->saved_screens); i++)
		{
			if (NULL != di->saved_screens[i])
			{
				free(di->saved_screens[i]);
				di->saved_screens[i] = NULL;
			}
		}
	}
}

/*----------------------------------------------------------------------
*
* win32_init --
*
*	Initialize the windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Initializes windows.
*
*----------------------------------------------------------------------
*/
int
win32_init(Driver *drv, int *argc, char **argv)
{
	LPCSTR title = "FractInt for Windows";
	DI(di);

	ODS("win32_init");
	frame_init(g_instance, title);
	if (!wintext_initialize(&di->wintext, g_instance, NULL, "Text"))
	{
		return FALSE;
	}

	return TRUE;
}

/* win32_key_pressed
 *
 * Return 0 if no key has been pressed, or the FIK value if it has.
 * driver_get_key() must still be called to eat the key; this routine
 * only peeks ahead.
 *
 * When a keystroke has been found by the underlying wintext_xxx
 * message pump, stash it in the one key buffer for later use by
 * get_key.
 */
int
win32_key_pressed(Driver *drv)
{
	DI(di);
	int ch = di->key_buffer;

	if (ch)
	{
		return ch;
	}
	flush_output();
	ch = handle_special_keys(frame_get_key_press(0));
	_ASSERTE(di->key_buffer == 0);
	di->key_buffer = ch;

	return ch;
}

/* win32_unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void win32_unget_key(Driver *drv, int key)
{
	DI(di);
	_ASSERTE(0 == di->key_buffer);
	di->key_buffer = key;
}

/* win32_get_key
 *
 * Get a keystroke, blocking if necessary.  First, check the key buffer
 * and if that's empty ask the wintext window to pump a keystroke for us.
 * If we get it, pass it off to handle tab and help displays.  If those
 * displays ate the key, then get another one.
 */
int
win32_get_key(Driver *drv)
{
	DI(di);
	int ch;
	
	do
	{
		if (di->key_buffer)
		{
			ch = di->key_buffer;
			di->key_buffer = 0;
		}
		else
		{
			ch = handle_special_keys(frame_get_key_press(1));
		}
	}
	while (ch == 0);

	return ch;
}

/*
*----------------------------------------------------------------------
*
* shell_to_dos --
*
*	Exit to a unix shell.
*
* Results:
*	None.
*
* Side effects:
*	Goes to shell
*
*----------------------------------------------------------------------
*/
void
win32_shell(Driver *drv)
{
	DI(di);
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
			frame_pump_messages(0);
			status = WaitForSingleObject(pi.hProcess, 1000); 
		}
		CloseHandle(pi.hProcess);
	}
}

void
win32_hide_text_cursor(Driver *drv)
{
	DI(di);
	if (TRUE == di->cursor_shown)
	{
		di->cursor_shown = FALSE;
		wintext_hide_cursor(&di->wintext);
	}
	ODS("win32_hide_text_cursor");
}

/* win32_set_video_mode
*/
void
win32_set_video_mode(Driver *drv, VIDEOINFO *mode)
{
	extern void set_normal_dot(void);
	extern void set_normal_line(void);
	DI(di);

	/* initially, set the virtual line to be the scan line length */
	g_vxdots = sxdots;
	g_is_true_color = 0;				/* assume not truecolor */
	g_vesa_x_res = 0;					/* reset indicators used for */
	g_vesa_y_res = 0;					/* virtual screen limits estimation */
	g_ok_to_print = FALSE;
	g_good_mode = 1;
	if (dotmode !=0)
	{
		g_and_color = colors-1;
		boxcount = 0;
		g_dac_learn = 1;
		g_dac_count = cyclelimit;
		g_got_real_dac = TRUE;			/* we are "VGA" */

		driver_read_palette();
	}

	driver_resize();

	if (g_disk_flag)
	{
		enddisk();
	}

	set_normal_dot();
	set_normal_line();

	driver_set_for_graphics();
	driver_set_clear();
}

void
win32_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
	DI(di);
	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	{
		int abs_row = g_text_rbase + g_text_row;
		int abs_col = g_text_cbase + g_text_col;
		_ASSERTE(abs_row >= 0 && abs_row < WINTEXT_MAX_ROW);
		_ASSERTE(abs_col >= 0 && abs_col < WINTEXT_MAX_COL);
		wintext_putstring(&di->wintext, abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
	}
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
void
win32_scroll_up(Driver *drv, int top, int bot)
{
	DI(di);

	wintext_scroll_up(&di->wintext, top, bot);
}

void
win32_move_cursor(Driver *drv, int row, int col)
{
	DI(di);

	if (row != -1)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (col != -1)
	{
		di->cursor_col = col;
		g_text_col = col;
	}
	row = di->cursor_row;
	col = di->cursor_col;
	wintext_cursor(&di->wintext, g_text_cbase + col, g_text_rbase + row, 1);
	di->cursor_shown = TRUE;
}

void
win32_set_attr(Driver *drv, int row, int col, int attr, int count)
{
	DI(di);

	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	wintext_set_attr(&di->wintext, g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
void
win32_stack_screen(Driver *drv)
{
	DI(di);

	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{
		/* already have some stacked */
		int i = di->screen_count - 1;

		_ASSERTE(i < WIN32_MAXSCREENS);
		if (i >= WIN32_MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		di->saved_screens[i] = wintext_screen_get(&di->wintext);
		driver_set_clear();
	}
	else
	{
		driver_set_for_text();
		driver_set_clear();
	}
}

void
win32_unstack_screen(Driver *drv)
{
	DI(di);

	_ASSERTE(di->screen_count >= 0);
	g_text_row = di->saved_cursor[di->screen_count] / 80;
	g_text_col = di->saved_cursor[di->screen_count] % 80;
	if (--di->screen_count >= 0)
	{
		/* unstack */
		wintext_screen_set(&di->wintext, di->saved_screens[di->screen_count]);
		free(di->saved_screens[di->screen_count]);
		di->saved_screens[di->screen_count] = NULL;
		win32_move_cursor(drv, -1, -1);
	}
	else
	{
		driver_set_for_graphics();
	}
}

void
win32_discard_screen(Driver *drv)
{
	DI(di);

	if (--di->screen_count >= 0)
	{
		/* unstack */
		if (di->saved_screens[di->screen_count])
		{
			free(di->saved_screens[di->screen_count]);
			di->saved_screens[di->screen_count] = NULL;
		}
	}
	else
	{
		driver_set_for_graphics();
	}
}

int
win32_init_fm(Driver *drv)
{
	ODS("win32_init_fm");
	return 0;
}

void
win32_buzzer(Driver *drv, int kind)
{
	ODS1("win32_buzzer %d", kind);
	MessageBeep(MB_OK);
}

int
win32_sound_on(Driver *drv, int freq)
{
	ODS1("win32_sound_on %d", freq);
	return 0;
}

void
win32_sound_off(Driver *drv)
{
	ODS("win32_sound_off");
}

void
win32_mute(Driver *drv)
{
	ODS("win32_mute");
}

int
win32_diskp(Driver *drv)
{
	return 0;
}

int
win32_key_cursor(Driver *drv, int row, int col)
{
	DI(di);
	int result;

	ODS2("win32_key_cursor %d,%d", row, col);
	if (-1 != row)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (-1 != col)
	{
		di->cursor_col = col;
		g_text_col = col;
	}

	col = di->cursor_col;
	row = di->cursor_row;

	if (win32_key_pressed(drv))
	{
		result = win32_get_key(drv);
	}
	else
	{
		di->cursor_shown = TRUE;
		wintext_cursor(&di->wintext, col, row, 1);
		result = win32_get_key(drv);
		win32_hide_text_cursor(drv);
		di->cursor_shown = FALSE;
	}

	return result;
}

int
win32_wait_key_pressed(Driver *drv, int timeout)
{
	int count = 10;
	while (!driver_key_pressed())
	{
		Sleep(25);
		if (timeout && (--count == 0))
		{
			break;
		}
	}

	return driver_key_pressed();
}

int
win32_get_char_attr(Driver *drv)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, g_text_row, g_text_col);
}

void
win32_put_char_attr(Driver *drv, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, g_text_row, g_text_col, char_attr);
}

int
win32_get_char_attr_rowcol(Driver *drv, int row, int col)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, row, col);
}

void
win32_put_char_attr_rowcol(Driver *drv, int row, int col, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, row, col, char_attr);
}

void
win32_delay(Driver *drv, int ms)
{
	DI(di);

	frame_pump_messages(FALSE);
	if (ms >= 0)
	{
		Sleep(ms);
	}
}

void
win32_get_truecolor(Driver *drv, int x, int y, int *r, int *g, int *b, int *a)
{
	_ASSERTE(0 && "win32_get_truecolor called.");
}

void
win32_put_truecolor(Driver *drv, int x, int y, int r, int g, int b, int a)
{
	_ASSERTE(0 && "win32_put_truecolor called.");
}

void
win32_set_keyboard_timeout(Driver *drv, int ms)
{
	frame_set_keyboard_timeout(ms);
}
