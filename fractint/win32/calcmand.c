/* calcmand.c
 * This file contains routines to replace calcmand.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "externs.h"
#include "drivers.h"

#define FUDGE_FACTOR_BITS 29
#define FUDGE_FACTOR ((1L << FUDGE_FACTOR_BITS)-1)
#define FUDGE_MUL(x_, y_) MulDiv(x_, y_, FUDGE_FACTOR)

#define KEYPRESSDELAY 32767
#define ABS(x) ((x) < 0?-(x):(x))

extern int g_atan_colors;

static int inside_color;
static int periodicity_color;

static unsigned long savedmask = 0;

static int x = 0;
static int y = 0;
static int savedx = 0;
static int savedy = 0;
static int k = 0;
static int savedand = 0;
static int savedincr = 0;
static int period = 0;

long cdecl
calcmandasm(void)
{
	return integer_unsupported();
}

static long cdecl
calc_mand_floating_point(void)
{
	long cx;
	long savedand;
	int savedincr;
	long tmpfsd;
	long x, y, x2, y2, xy, Cx, Cy, savedx, savedy;

	g_old_color_iter = (g_periodicity_check == 0) ? 0 : (maxit - 250);
	tmpfsd = maxit - g_first_saved_and;
	if (g_old_color_iter > tmpfsd)
	{
		g_old_color_iter = tmpfsd;
	}

	savedx = 0;
	savedy = 0;
	g_orbit_index = 0;
	savedand = g_first_saved_and;
	savedincr = 1;             /* start checking the very first time */
	g_input_counter--;                /* Only check the keyboard sometimes */
	if (g_input_counter < 0)
	{
		int key;
		g_input_counter = 1000;
		key = driver_key_pressed();
		if (key)
		{
			if (key == 'o' || key == 'O')
			{
				driver_get_key();
				g_show_orbit = g_show_orbit ? FALSE : TRUE;
			}
			else
			{
				g_color_iter = -1;
				return -1;
			}
		}
	}

	cx = maxit;
	if (fractype != JULIA)
	{
		/* Mandelbrot_87 */
		Cx = linitx;
		Cy = linity;
		x = g_parameter_l.x + Cx;
		y = g_parameter_l.y + Cy;
	}
	else
	{
		/* dojulia_87 */
		Cx = g_parameter_l.x;
		Cy = g_parameter_l.y;
		x = linitx;
		y = linity;
		x2 = FUDGE_MUL(x, x);
		y2 = FUDGE_MUL(y, y);
		xy = FUDGE_MUL(x, y);
		x = x2-y2 + Cx;
		y = 2*xy + Cy;
	}
	x2 = FUDGE_MUL(x, x);
	y2 = FUDGE_MUL(y, y);
	xy = FUDGE_MUL(x, y);

	/* top_of_cs_loop_87 */
	while (--cx > 0)
	{
		x = x2-y2 + Cx;
		y = 2*xy + Cy;
		x2 = FUDGE_MUL(x, x);
		y2 = FUDGE_MUL(y, y);
		xy = FUDGE_MUL(x, y);
		g_magnitude = x2 + y2;

		if (g_magnitude >= g_magnitude_limit)
		{
			if (g_outside <= -2)
			{
				g_new_z_l.x = x;
				g_new_z_l.y = y;
			}
			g_old_color_iter = (cx - 10 > 0) ? cx - 10 : 0;
			g_real_color_iter = maxit - cx;
			g_color_iter = g_real_color_iter;

			if (g_color_iter == 0)
			{
				g_color_iter = 1;
			}
			g_input_counter -= g_real_color_iter;
			if (g_outside == -1)
			{
			}
			else if (g_outside > -2)
			{
				g_color_iter = g_outside;
			}
			else
			{
				/* special_outside */
				if (g_outside == REAL)
				{
					g_color_iter += g_new_z_l.x + 7;
				}
				else if (g_outside == IMAG)
				{
					g_color_iter += g_new_z_l.y + 7;
				}
				else if (g_outside == MULT && g_new_z_l.y != 0)
				{
					g_color_iter = FUDGE_MUL(g_color_iter, g_new_z_l.x) / g_new_z_l.y;
				}
				else if (g_outside == SUM)
				{
					g_color_iter += g_new_z_l.x + g_new_z_l.y;
				}
				else if (g_outside == ATAN)
				{
					g_color_iter = (long) fabs(atan2(g_new_z_l.y, g_new_z_l.x)*g_atan_colors/PI);
				}
				/* check_color */
				if ((g_color_iter <= 0 || g_color_iter > maxit) && g_outside != FMOD)
				{
					g_color_iter = (g_save_release < 1961) ? 0 : 1;
				}
			}

			goto pop_stack;
		}

		/* no_save_new_xy_87 */
		if (cx < g_old_color_iter)   /* check periodicity */
		{
			if (((maxit - cx) & savedand) == 0)
			{
				savedx = x;
				savedy = y;
				savedincr--;
				if (savedincr == 0)
				{
					savedand = (savedand << 1) + 1;
					savedincr = g_next_saved_incr;
				}
			}
			else if (ABS(savedx-x) < g_close_enough_l && ABS(savedy-y) < g_close_enough_l)
			{
				/* g_old_color_iter = 65535;  */
				g_old_color_iter = maxit;
				g_real_color_iter = maxit;
				g_input_counter = g_input_counter - (maxit - cx);
				g_color_iter = periodicity_color;
				goto pop_stack;
			}
		}
		/* no_periodicity_check_87 */
		if (g_show_orbit)
		{
			plot_orbit(x, y, -1);
		}
		/* no_show_orbit_87 */
	} /* while (--cx > 0) */

	/* reached maxit */
	/* g_old_color_iter = 65535;  */
	/* check periodicity immediately next time, remember we count down from maxit */
	g_old_color_iter = maxit;
	g_input_counter -= maxit;
	g_real_color_iter = maxit;

	g_color_iter = inside_color;

pop_stack:
	if (g_orbit_index)
	{
		orbit_scrub();
	}

	return g_color_iter;
}

static long cdecl
calc_mand_assembly(void)
{
	__asm
	{
		sub		eax, eax					; clear eax
		mov		edx, eax					; clear edx
		cmp		g_periodicity_check, eax		;	periodicity	checking disabled?
		je		initoldcolor				;  yup,	set	oldcolor 0 to disable it
		cmp		g_reset_periodicity, eax		;	periodicity	reset?
		je		short initparms				; inherit oldcolor from	prior invocation
		mov		eax, dword ptr maxit		; yup.  reset oldcolor to maxit-250
		sub		eax, 250					;	(avoids	slowness at	high maxits)

initoldcolor:
		mov		dword ptr g_old_color_iter,	eax	; reset g_old_color_iter

initparms:
		mov		eax, dword ptr creal		; initialize	x == creal
		mov		dword ptr x, eax			;  ...

		mov		eax, dword ptr cimag		; initialize	y == cimag
		mov		dword ptr y, eax			;  ...

		mov		eax, dword ptr maxit		; setup k = maxit
		mov		edx, dword ptr maxit + 4
		add		eax, 1						;	(+ 1)
		adc		edx, 0
		mov		dword ptr k, eax			;  (decrementing	to 0 is	faster)
		mov		dword ptr k + 4, edx

		cmp		fractype, 1					; julia or	mandelbrot set?
		je		short dojulia				; julia	set	- go there

		sub		dword ptr k, 1				;	we know	the	first iteration	passed
		sbb		dword ptr k + 4, 0			;	we know	the	first iteration	passed
		mov		edx, dword ptr linitx + 4		; add x += linitx
		mov		eax, dword ptr linitx		;  ...
		add		dword ptr x, eax			;  ...
		adc		dword ptr x + 4, edx			;  ...
		mov		edx, dword ptr linity + 4		; add y += linity
		mov		eax, dword ptr linity		;  ...
		add		dword ptr y, eax			;  ...
		adc		dword ptr y + 4, edx			;  ...
		jmp		short doeither				; branch around	the	julia switch

dojulia:									; Julia	Set	initialization
											; "fudge" Mandelbrot start-up values
		mov		eax, dword ptr x			; switch	x with linitx
		mov		edx, dword ptr x + 4			;  ...
		mov		ebx, dword ptr linitx		;  ...
		mov		ecx, dword ptr linitx + 4		;  ...
		mov		dword ptr x, ebx			;  ...
		mov		dword ptr x + 4, ecx			;  ...
		mov		dword ptr linitx, eax		;  ...
		mov		dword ptr linitx + 4,	edx		;  ...

		mov		eax, dword ptr y			; switch	y with linity
		mov		edx, dword ptr y + 4			;  ...
		mov		ebx, dword ptr linity		;  ...
		mov		ecx, dword ptr linity + 4		;  ...
		mov		dword ptr y, ebx			;  ...
		mov		dword ptr y + 4, ecx			;  ...
		mov		dword ptr linity, eax		;  ...
		mov		dword ptr linity + 4,	edx		;  ...

doeither:									; common Mandelbrot, Julia set code
		mov		period,	0					; claim periodicity of	1
		mov		eax, dword ptr g_first_saved_and ; initial periodicity check
		mov		dword ptr savedand,	eax		; initial periodicity check
		mov		eax, dword ptr g_first_saved_and + 4 ; initial periodicity check
		mov		dword ptr savedand + 4, eax	; initial periodicity check
		mov		savedincr, 1				;	flag for incrementing periodicity
		mov		dword ptr savedx + 4,	0ffffh	;	impossible value of	"old" x
		mov		dword ptr savedy + 4,	0ffffh	;	impossible value of	"old" y
		mov		g_orbit_index, 0				; clear orbits

		dec		g_input_counter					; decrement	the	keyboard counter
		jns		nokey						;  skip	keyboard test if still positive
		mov		g_input_counter, 10				; stuff in	a low kbd count
		cmp		g_show_orbit,	0				; are we showing orbits?
		jne		quickkbd					;  yup.	 leave it that way.
		cmp		orbit_delay, 0				; are we delaying orbits?
		je		slowkbd						;  nope.  change it.
		cmp		g_show_dot, 0					; are we showing the current pixel?
		jge		quickkbd					;  yup.	 leave it that way.
slowkbd:
		mov		g_input_counter, 5000				; else, stuff an appropriate count	val
		cmp		cpu, 386					; ("appropriate" to the CPU)
		jae		short kbddiskadj			;  ...
		cmp		dword ptr delmin + 4,	8		;	is 16-bit math good	enough?
		ja		kbddiskadj					;  yes.	test less often
		mov		g_input_counter, 500				;	no.	 test more often
kbddiskadj:
		cmp		dotmode, 11					; disk	video?
		jne		quickkbd					;  no, leave as	is
		shr		g_input_counter, 1					; yes,	reduce count
		shr		g_input_counter, 1					; yes,	reduce count
quickkbd:
		call	driver_key_pressed			; has a	key	been pressed?
		cmp		eax, 0						;	 ...
		je		nokey						; nope.	 proceed
		mov		g_input_counter, 0					; make	sure it	goes negative again
		cmp		eax, 'o'					;	orbit toggle hit?
		je		orbitkey					;  yup.	 show orbits
		cmp		eax, 'O'					;	orbit toggle hit?
		jne		keyhit						;  nope.  normal key.
orbitkey:
		call	 driver_get_key				; read the key for real
		mov		eax, 1						;	reset orbittoggle =	1 -	orbittoggle
		sub		eax, g_show_orbit				;	 ...
		mov		g_show_orbit,	eax				;	 ...
		jmp		short nokey					; pretend no key was hit
keyhit:	mov		eax, -1						;	return with	-1
		mov		edx, eax
		mov		dword ptr g_color_iter, eax	; set coloriter to -1
		mov		dword ptr g_color_iter + 4, edx
		pop		esi
		pop		edi
		pop		ebp
		ret									; bail out!

nokey:
		cmp		g_show_orbit,	0				; is orbiting on?
		jne		no16bitcode					;  yup.	 slow down.
		cmp		cpu, 386					; are we on a 386?
		jae		short code386bit			;  YAY!! 386-class speed!
		cmp		dword ptr delmin + 4,	8		;	OK,	we're desperate.  16 bits OK?
		ja		yes16bitcode				;  YAY!	 16-bit	speed!
no16bitcode:
		call	code32bit					; BOO!! nap time.  Full 32 bit math
		cmp		ecx, -1
		je		keyhit						; key stroke, get us out of	here
		jmp		kloopend					;  bypass the 386-specific code.
yes16bitcode:
		call	code16bit					; invoke	the	16-bit version
		cmp		ecx, -1
		je		keyhit						; key stroke, get us out of	here
		jmp		kloopend					;  bypass the 386-specific code.

code386bit:
		cmp		delmin + 4, 8					; is 16-bit	math good enough?
		jbe		code386_32					; nope,	go do 32 bit stuff

		; 16 bit on	386, now we	are	really gonna move
		movsx	esi, dword ptr x + 4			;	use	SI for X
		movsx	edi, dword ptr y + 4			;	use	DI for Y
		push	ebp
		mov		ebp, -1
		shl		ebp, FUDGE_FACTOR_BITS-1
		mov		ecx, FUDGE_FACTOR_BITS-16

kloop386_16:   ; ecx=bitshift-16, ebp=overflow.mask

		mov		ebx, esi					; compute (x *	x)
		imul	ebx, ebx					;	...
		test	ebx, ebp					;
		jnz		short end386_16				;  (oops.  We done.)
		shr		ebx, cl						; get result down to 16 bits

		mov		edx, edi					; compute (y *	y)
		imul	edx, edx					;	...
		test	edx, ebp					; say,	did	we overflow? <V20-compat>
		jnz		short end386_16				;  (oops.  We done.)
		shr		edx, cl						; get result down to 16 bits

		mov		eax, ebx					; compute (x*x -	y*y) / fudge
		sub		ebx, edx					;  for the next iteration

		add		eax, edx					; compute (x*x +	y*y) / fudge

		cmp		eax, dword ptr g_magnitude_limit + 4			; while (xx + yy <	g_magnitude_limit)
		jae		short end386_16				;  ...

		imul	edi, esi					; compute (y *	x)
		shl		edi, 1						; ( * 2 / fudge)
		sar		edi, cl
		add		edi, dword ptr linity + 4		; (2*y*x) / fudge + linity
		movsx	edi, edi					;	save as	y

		add		ebx, dword ptr linitx + 4		; (from above) (x*x - y*y)/fudge	+ linitx
		movsx	esi, bx						; save	as x

		mov		eax, k						; rearranged for speed
		cmp		eax, g_old_color_iter
		jb		short chkpd386_16
nonmax386_16:
; miraculously,	k is always	loaded into	eax	at this	point
		test	eax, KEYPRESSDELAY
		jne		notakey1					; don't	test yet
		push	ecx
		call	driver_key_pressed			; has a	key	been pressed?
		pop		ecx
		cmp		eax, 0						;	 ...
		je		notakey1					; nope.	 proceed
		pop		ebp
		jmp		keyhit
notakey1:

		dec		k							; while	(k < maxit)
		jnz		short kloop386_16			; try, try again
end386_16:
		pop		ebp
		jmp		kloopend32					; we done

chkpd386_16:
		test	eax, savedand				; save	on 0, check	on anything	else
		jz		short chksv386_16			;  time	to save	a new "old"	value
		mov		ebx, esi					; load up x
		xor		ebx, dword ptr savedx + 4		; does X	match?
		cmp		ebx, dword ptr g_close_enough_l + 4	;  truncate	to appropriate precision
		ja		short nonmax386_16			;  nope.  forget it.
		mov		ebx, edi					; now test y
		xor		ebx, dword ptr savedy + 4		; does Y	match?
		cmp		ebx, dword ptr g_close_enough_l + 4	;  truncate	to appropriate precision
		ja		short nonmax386_16			;  nope.  forget it.
		mov		period,	1					; note	that we	have found periodicity
		mov		k, 0						; pretend maxit reached
		jmp		short end386_16
chksv386_16:
		mov		dword ptr savedx + 4,	esi		; save x
		mov		dword ptr savedy + 4,	edi		; save y
		dec		savedincr					; time to change the periodicity?
		jnz		short nonmax386_16			;  nope.
		shl		savedand, 1					; well	then, let's	try	this one!
		inc		savedand					;  (2**n +1)
		mov		eax, g_next_saved_incr			;	and	reset the increment	flag
		mov		savedincr, eax				;	and	reset the increment	flag
		jmp		short nonmax386_16

		; 32bit	on 386:
code386_32:
		mov		esi, x						; use ESI for X
		mov		edi, y						; use EDI for Y

;		This is	the	main processing	loop.  Here, every T-state counts...

kloop:										; for (k = 0; k	<= maxit; k++)

		mov		eax, esi					; compute (x *	x)
		imul	esi							;  ...
		shrd	eax, edx, FUDGE_FACTOR_BITS		;	( /	fudge)
		shr		edx, FUDGE_FACTOR_BITS-1			; (complete 64-bit	shift and check
		jne		short kloopend1				; bail out if too high
		mov		ebx, eax					; save	this for below

		mov		eax, edi					; compute (y *	y)
		imul	edi							;  ...
		shrd	eax, edx, FUDGE_FACTOR_BITS		;	( /	fudge)
		shr		edx, FUDGE_FACTOR_BITS-1			; (complete 64-bit	shift and check
		jne		short kloopend1				; bail out if too high

		mov		ecx, ebx					; compute (x*x	- y*y) / fudge
		sub		ebx, eax					;	for	the	next iteration

		add		ecx, eax					; compute (x*x	+ y*y) / fudge
		cmp		ecx, g_magnitude_limit						; while (lr < g_magnitude_limit)
		jae		short kloopend1				;  ...

		mov		eax, edi					; compute (y *	x)
		imul	esi							;  ...
		shrd	eax, edx, FUDGE_FACTOR_BITS-1		;	 ( * 2 / fudge)
		add		eax, linity					;	(above)	+ linity
		mov		edi, eax					;	save this as y

;		(from the earlier code)				; compute (x*x - y*y) /	fudge
		add		ebx, linitx					;		 + linitx
		mov		esi, ebx					; save	this as	x

		mov		eax, k						; rearranged for speed
		cmp		eax, g_old_color_iter
		jb		short chkperiod1
nonmax1:
		mov		eax, k
		test	eax, KEYPRESSDELAY
		jne		notakey2					; don't	test yet
		call	driver_key_pressed			; has a	key	been pressed?
		cmp		eax, 0						;	 ...
		je		notakey2					; nope.	 proceed
		jmp		keyhit
notakey2:

		dec		k							; while	(k < maxit)	(dec to	0 is faster)
		jnz		kloop						; while	(k < maxit)	...
kloopend1:
		jmp		short kloopend32			; we done.

chkperiod1:
		test	eax, savedand
		jz		short chksave1
		mov		eax, esi
		xor		eax, savedx
		cmp		eax, g_close_enough_l
		ja		short nonmax1
		mov		eax, edi
		xor		eax, savedy
		cmp		eax, g_close_enough_l
		ja		short nonmax1
		mov		period,	1					; note	that we	have found periodicity
		mov		k, 0						; pretend maxit reached
		jmp		short kloopend32			; we done.
chksave1:
		mov		eax, k
		mov		savedx,	esi
		mov		savedy,	edi
		dec		savedincr					; time to change the periodicity?
		jnz		short nonmax1				;  nope.
		shl		savedand, 1					; well	then, let's	try	this one!
		inc		savedand					;  (2**n +1)
		mov		eax, g_next_saved_incr			;	and	reset the increment	flag
		mov		savedincr, eax				;	and	reset the increment	flag
		jmp		short nonmax1

kloopend32:

		cmp		g_orbit_index, 0				; any orbits to clear?
		je		noorbit32					;  nope.
		call	orbit_scrub					; clear	out	any	old	orbits
noorbit32:

		mov		eax, k						; set old color
		sub		eax, 10						; minus 10, for safety
		mov		g_old_color_iter, eax			; and save	it as the "old"	color
		mov		eax, maxit					; compute color
		sub		eax, k						;	(first,	re-compute "k")
		sub		g_input_counter, eax				;	adjust the keyboard	count (use eax only)
		cmp		eax, 0						; convert any "outlier" region
		jg		short coloradjust1_32		;  (where abs(x) > 2 or	abs(y) > 2)
		mov		eax, 1						;	 to	look like we ran through
coloradjust1_32:							;	 at	least one loop.
		mov		g_real_color_iter, eax			; result before adjustments
		cmp		eax, maxit					; did we max out on iterations?
		jne		short notmax32				;	 nope.
		mov		g_old_color_iter, eax			; set "oldcolor"	to maximum
		cmp		g_inside,	0					; is "inside" >= 0?
		jl		wedone32					;  nope.  leave	it at "maxit"
		sub		eax, eax
		mov		eax, g_inside					;	reset max-out color	to default
		cmp		g_periodicity_check, 0			; show	periodicity	matches?
		jge		wedone32					;  nope.
		cmp		period,	0
		je		wedone32
		mov		eax, 7						;	use	color 7	(default white)
		jmp		short wedone32

notmax32:
		cmp		g_outside, 0					; is "outside"	>= 0?
		jl		wedone32					;	nope. leave	as realcolor
		sub		eax, eax
		mov		eax, g_outside				; reset to	"outside" color

wedone32:									;
		mov		g_color_iter, eax				; save	the	color result
		shld	edx, eax, 16				;	put	result in eax, edx
		shr		eax, 16
		pop		esi
		pop		edi
		pop		ebp
		ret									; and return with color


kloopend:
		cmp		g_orbit_index, 0				; any orbits to clear?
		je		noorbit2					;  nope.
		call	orbit_scrub					; clear	out	any	old	orbits
noorbit2:

		mov		eax, dword ptr k			; set old color
		mov		edx, dword ptr k + 4			; set old color
		sub		eax, 10						;	minus 10, for safety
		sbb		edx, 0
		mov		dword ptr g_old_color_iter,	eax	; and save it as	the	"old" color
		mov		dword ptr g_old_color_iter + 4, edx ; and save it as	the	"old" color
		mov		eax, dword ptr maxit		; compute color
		mov		edx, dword ptr maxit + 4		; compute color
		sub		eax, dword ptr k			;  (first, re-compute "k")
		sbb		edx, dword ptr k + 4			;  (first, re-compute "k")
		sub		g_input_counter, eax				;	adjust the keyboard	count
		cmp		edx, 0						;	convert	any	"outlier" region
		js		short kludge_for_julia		;  k can be	> maxit!!!
		ja		short coloradjust1			;  (where abs(x) > 2 or	abs(y) > 2)
		cmp		eax, 0
		ja		short coloradjust1			;  (where abs(x) > 2 or	abs(y) > 2)
kludge_for_julia:
		mov		eax, 1						;	  to look like we ran through
		sub		edx, edx
coloradjust1:								;	 at	least one loop.
		mov		dword ptr g_real_color_iter, eax	; result before adjustments
		mov		dword ptr g_real_color_iter + 4, edx	; result before adjustments
		cmp		edx, dword ptr maxit + 4			; did we max out on iterations?
		jne		short notmax					;  nope.
		cmp		eax, dword ptr maxit			; did we max out on iterations?
		jne		short notmax					;  nope.
		mov		dword ptr g_old_color_iter,	eax		; set "oldcolor" to maximum
		mov		dword ptr g_old_color_iter + 4, edx	; set "oldcolor" to maximum
		cmp		g_inside,	0					; is "inside" >= 0?
		jl		wedone						;  nope.  leave	it at "maxit"
		mov		eax, g_inside					;	reset max-out color	to default
		sub		edx, edx
		cmp		g_periodicity_check, 0			; show	periodicity	matches?
		jge		wedone						;  nope.
		mov		eax, 7						;	use	color 7	(default white)
		jmp		short wedone

notmax:
		cmp		g_outside, 0					; is "outside"	>= 0?
		jl		wedone						;	nope. leave	as realcolor
		mov		eax, g_outside				; reset to	"outside" color
		sub		edx, edx

wedone:										;
		mov		dword ptr g_color_iter, eax	; save the color	result
		mov		dword ptr g_color_iter + 4, edx	; save	the	color result
		pop		esi
		pop		edi
		pop		ebp
		ret									; and return with color

; ******************** Function	code16bit()	*****************************
;
;		Performs "short-cut" 16-bit	math where we can get away with	it.
; CJLT has modified	it,	mostly by preshifting x	and	y to fg30 from fg29
; or, since	we ignore the lower	16 bits, fg14 from fg13.
; If this shift	overflows we are outside x*x + y*y=2,	so have	escaped.
; Also,	he commented out several conditional jumps which he	calculated could
; never	be taken (e.g. mov eax,	esi	/ imul esi ; cannot overflow).

code16bit:
		mov		esi, dword ptr x + 4			; use SI	for	X fg13
		mov		edi, dword ptr y + 4			; use DI	for	Y fg13

start16bit:
		add		esi, esi					; CJLT-Convert to	fg14
		jno		not_end16bit1
		jmp		end16bit					; overflows if <-2 or >2
not_end16bit1:
		mov		eax, esi					; compute (x	* x)
		imul	esi							; Answer is fg14 + 14-16=fg12
		shl		eax, 1						;	 ...
		rcl		edx, 1						;	 ...
		jno		not_end16bit2
		jmp		end16bit					;  (oops.  overflow)
not_end16bit2:
		mov		ebx, edx					; save this for a tad

; ditto for y*y...

		add		edi, edi					; CJLT-Convert to	fg14
		jno		not_end16bit3
		jmp		end16bit					; overflows if <-2 or >2
not_end16bit3:
		mov		eax, edi					; compute (y	* y)
		imul	edi							;	...
		shl		eax, 1						;	 ...
		rcl		edx, 1						;	 ...
		jo		end16bit					;  (oops.  overflow)

		mov		ecx, ebx					; compute (x*x -	y*y) / fudge
		sub		ebx, edx					;  for the next iteration

		add		ecx, edx					; compute (x*x +	y*y) / fudge
		jo		end16bit					; bail out if too high

		cmp		ecx, dword ptr g_magnitude_limit + 4			; while (xx + yy <	g_magnitude_limit)
		jae		end16bit					;  ...
		sub		dword ptr k, 1				;	while (k < maxit)
		sbb		dword ptr k + 4, 0
		jnz		notdoneyet
		cmp		dword ptr k, 0
		jz		end16bit					;  we done.
notdoneyet:
		mov		eax, edi					; compute (y	* x) fg14 + 14=fg28
		imul	esi							;	...
		shl		eax, 1						;	 ...
		rcl		edx, 1						;	 ...
		shl		eax, 1						;	 shift two bits
		rcl		edx, 1						;	 cannot	overflow as	|x| <= 2,	|y| <= 2
		add		edx, dword ptr linity + 4		; (2*y*x) / fudge + linity
		jo		end16bit					; bail out if too high
		mov		edi, edx					; save as y

		add		ebx, dword ptr linitx + 4		; (from above) (x*x - y*y)/fudge	+ linitx
		jo		end16bit					; bail out if too high
		mov		esi, ebx					; save as x

		mov		edx, dword ptr g_old_color_iter + 4	; recall	the	old	color
		cmp		edx, dword ptr k + 4				; check it against this iter
		jb		short nonmax3					;  nope.  bypass periodicity check.
		mov		eax, dword ptr g_old_color_iter		; recall	the	old	color
		cmp		eax, dword ptr k				; check it against this iter
		jb		short nonmax3					;  nope.  bypass periodicity check.
		mov		dword ptr x + 4, esi				; save x	for	periodicity	check
		mov		dword ptr y + 4, edi				; save y	for	periodicity	check
		call	checkperiod						; check	for	periodicity
nonmax3:
		mov		eax, dword ptr k			; set up	to test	for	key	stroke
		test	eax, KEYPRESSDELAY
		jne		notakey3					; don't	test yet
		push	ecx
		push	ebx
		call	driver_key_pressed			; has a	key	been pressed?
		pop		ebx
		pop		ecx
		cmp		eax, 0						;	 ...
		je		notakey3					; nope.	 proceed
		mov		ecx, -1
		jmp		short end16bitgotkey		; ecx set, jump	to end
notakey3:
		jmp		start16bit					; try, try again.

end16bit:									; we done.
		xor		ecx, ecx					; no	key	so zero	ecx
end16bitgotkey:								; jump here	if key
		ret


;		The	following routine checks for periodic loops	(a known
;		method of decay	inside "Mandelbrot Lake", and an easy way to
;		bail out of	"lake" points quickly).	 For speed,	only the
;		high-order sixteen bits	of X and Y are checked for periodicity.
;		For	accuracy, this routine is only fired up	if the previous	pixel
;		was	in the lake	(which means that the FIRST	"wet" pixel	was
;		detected by	the	dull-normal	maximum	iteration process).

checkperiod:								; periodicity check
		mov		eax, dword ptr k			; set up to	test for save-time
		test	eax, dword ptr savedand		; save on 0,	check on anything else
		jnz		notimeyet					;  NOT time	to save	a new "old"	value
		mov		edx, dword ptr k + 4 ; set up	to test	for	save-time
		test	edx, dword ptr savedand + 4	; save on 0,	check on anything else
		jz		checksave					;  time	to save	a new "old"	value
notimeyet:
		mov		edx, dword ptr x + 4			; load up x
		xor		edx, dword ptr savedx + 4
		cmp		edx, dword ptr g_close_enough_l + 4
		ja		checkdone
		mov		eax, dword ptr x			; load up x
		xor		eax, dword ptr savedx
		cmp		eax, dword ptr g_close_enough_l
		ja		checkdone
		mov		edx, dword ptr y + 4			; load up y
		xor		edx, dword ptr savedy + 4
		cmp		edx, dword ptr g_close_enough_l + 4
		ja		checkdone
		mov		eax, dword ptr y			; load up y
		xor		eax, dword ptr savedy
		cmp		eax, dword ptr g_close_enough_l
		ja		checkdone
		mov		period,	1					; note	that we	have found periodicity
		mov		dword ptr k, 1				;	pretend	maxit reached
		mov		dword ptr k + 4, 0			;	pretend	maxit reached
checksave:
		mov		edx, dword ptr x + 4			; load up x
		mov		dword ptr savedx + 4,	edx		;  and save it
		mov		eax, dword ptr x			; load up x
		mov		dword ptr savedx, eax		;  and save it
		mov		edx, dword ptr y + 4			; load up y
		mov		dword ptr savedy + 4,	edx		;  and save it
		mov		eax, dword ptr y			; load up y
		mov		dword ptr savedy, eax		;  and save it
		dec		savedincr					; time to change the periodicity?
		jnz		checkdone					;  nope.
		shl		dword ptr savedand,	1		;	well then, let's try this one!
		rcl		dword ptr savedand + 4, 1		;	well then, let's try this one!
		add		dword ptr savedand,	1		;	 (2**n +1)
		adc		dword ptr savedand + 4, 0		;	 (2**n +1)
		mov		eax, g_next_saved_incr			;	and	reset the increment	flag
		mov		savedincr, eax				;	and	reset the increment	flag
checkdone:
		ret									; we done.


; ******************** Function	code32bit()	*****************************
;
;		Perform	the	32-bit logic required using	16-bit logic
;
;		New	twice as fast logic,
;		   Courtesy	of Bill	Townsend and Mike Gelvin (CIS:73337, 520)
;		Even newer,	faster still by	Chris Lusby	Taylor
;		 who noted that	we needn't square the low word if we first multiply
;		 by	4, since we	only need 29 places	after the point	and	this will
;		 give 30. (We divide answer	by two to give 29 bit shift	in answer)
;		Also, he removed all testing for special cases where a word	of operand
;		happens	to be 0, since testing 65536 times costs more than the saving
;		1 time out of 65536! (I	benchmarked	it.	Just removing the tests	speeds
;		us up by 3%.)
;
; Note that square returns DI, AX squared in	DX,	AX now.
; DI, AX is	first converted	to unsigned	fg31 form.
; (For its square to be	representable in fg29 (range -4.. + 3.999)
; DI:AX	must be	in the range 0.. + 1.999 which fits neatly into unsigned fg31.)
; This allows us to	ignore the part	of the answer corresponding	to AX*AX as	it
; is less than half	a least	significant	bit	of the final answer.
; I	thought	you'd like that.
;
; As we	prescaled DI:AX, we	need to	shift the answer 1 bit to the right	to
; end up in	fg29 form since	29=(29 + 2) + (29 + 2)-32-1
; However, the mid term	AX*DI is needed	twice, so the shifts cancel.
;
; Since	abs(x) and abs(y) in fg31 form will	be needed in calculating 2*X*Y
; we push them onto	the	stack for later	use.

; Note that	square does	nor	affect bl, esi,	ebp
; and leaves highword of argument in edi
; but destroys bh, ecx

code32bit:
;
; BL IS	USED FOR THE "NEGSWT" FLAG
;	NEGSWT IS ONE IF EITHER	"X"	OR "Y" ARE NEGATIVE, BUT NOT BOTH NEGATIVE
;

		push	ebp
		xor		bl,	bl						; NEGSWT STARTS OFF ZERO

;		iteration loop

nextit:	mov		eax, dword ptr y			; eax=low(y)
		mov		edi, dword ptr y + 4			; edi=high(y)

		; square	done1						; square	y and quit via done1 if	it overflows
		shl		eax, 1						; Multiply	by 2 to	convert	to fg30
		rcl		edi, 1						; If this overflows DI:AX was negative
		jnc		notneg
		not		eax							; so negate it
		not		edi							; ...
		add		eax, 1						;	...
		adc		edi, 0						;	...
		not		bl							; change negswt
notneg:	shl		eax, 1						; Multiply	by 2 again to give fg31
		rcl		edi, 1						; If this gives a carry then DI:AX	was	>= 2.0
											; If	its	high bit is	set	then DI:AX was >= 1.0
											; This is OK, but note that this	means that
											; DI:AX must	now	be treated as unsigned.
		jc		done1
		push	edi							; save	y or x (in fg31	form) on stack
		push	eax							; ...
		mul		edi							; GET MIDDLE PART -	2*A*B
		mov		bh,	ah						; Miraculously,	it needs no	shifting!
		mov		ecx, edx
		mov		eax, edi
		mul		eax							; SQUARE HIGH HWORD	- A*A
		shl		bh,	1						; See if we	round up
		adc		eax, 1						; Anyway, add 1 to	round up/down accurately
		adc		edx, 0
		shr		edx, 1						; This	needs shifting one bit
		rcr		eax, 1
		add		eax, ecx					; Add	in the 2*A*B term
		adc		edx, 0

		mov		esi, eax					; square	returns	results	in edx,	eax
		mov		ebp, edx					; save y*y in ebp, esi
		mov		eax, dword ptr x
		mov		edi, dword ptr x + 4

		; square	 done2						; square x	and	quit via done2 if it overflows
		shl		eax, 1						; Multiply	by 2 to	convert	to fg30
		rcl		edi, 1						; If this overflows DI:AX was negative
		jnc		notneg2
		not		eax							; so negate it
		not		edi							; ...
		add		eax, 1						;	...
		adc		edi, 0						;	...
		not		bl							; change negswt
notneg2: shl	 eax, 1						; Multiply by	2 again	to give	fg31
		rcl		edi, 1						; If this gives a carry then DI:AX	was	>= 2.0
											; If	its	high bit is	set	then DI:AX was >= 1.0
											;This is OK, but note that this	means that
											;DI:AX must	now	be treated as unsigned.
		jc		done2
		push	edi							; save	y or x (in fg31	form) on stack
		push	eax							; ...
		mul		edi							;GET MIDDLE PART -	2*A*B
		mov		bh,	ah						;Miraculously,	it needs no	shifting!
		mov		ecx, edx
		mov		eax, edi
		mul		eax							;SQUARE HIGH HWORD	- A*A
		shl		bh,	1						;See if we	round up
		adc		eax, 1						;Anyway, add 1 to	round up/down accurately
		adc		edx, 0
		shr		edx, 1						;This	needs shifting one bit
		rcr		eax, 1
		add		eax, ecx					;Add	in the 2*A*B term
		adc		edx, 0

		mov		ecx, eax					; Save low answer in	ecx.
		ADD		eax, esi					; calc y*y +	x*x
		mov		eax, ebp
		ADC		eax, edx					;  ...
		jno		nextxy						; overflow?
											;NOTE: The original	code tests against g_magnitude_limit
											;here, but as g_magnitude_limit=4<<29 this	is the same
											;as	testing	for	signed overflow
done4:	add		sp,	4						; discard saved value of |x| fg 31
done2:	add		sp,	4						; discard saved value of |y| fg 31
done1:	xor		ecx, ecx					; no	key	exit, zero ecx
done0:										; exit here	if key hit
		pop		ebp							; restore saved ebp
		ret

;---------------------------------------------------------------------------

nextxy:	sub		dword ptr k, 1				;	while (k < maxit)
		sbb		dword ptr k + 4, 0
		jnz		tryagain
		cmp		dword ptr k, 0
		jz		done4						;  we done.
tryagain:
		sub		ecx, esi					; subtract y*y from x*x
		sbb		edx, ebp					;  ...
		add		ecx, dword ptr linitx		; add "A"
		adc		edx, dword ptr linitx + 4		;  ...
		jo		done4						;CJLT-Must detect overflow here
											; but increment	loop count first
		mov		dword ptr x, ecx			; store new x = x*x-y*y + a
		mov		dword ptr x + 4, edx			;  ...

; now calculate	x*y
;
;More CJLT tricks here.	We use the pushed abs(x) and abs(y)	in fg31	form
;which,	when multiplied, give x*y in fg30, which, luckily, is the same as...
;2*x*y fg29.
;As	with squaring, we can ignore the product of	the	low	order words, and still
;be	more accurate than the original	algorithm.
;
		pop		ebp							;Xlow
		pop		edi							;Xhigh	(already there,	actually)
		pop		eax							;Ylow
		mul		edi							;Xhigh	* Ylow
		mov		bh,	ah						;Discard lowest 8 bits	of answer
		mov		ecx, edx
		pop		eax							;Yhigh
		mov		esi, eax					; copy it
		mul		ebp							;Xlow * Yhigh
		xor		ebp, ebp					;Clear answer
		add		bh,	ah
		adc		ecx, edx
		adc		ebp, 0
		mov		eax, esi					; Yhigh
		mul		edi							; Xhigh	* Yhigh
		shl		bh,	1						; round	up/down
		adc		eax, ecx					; Answer-low
		adc		edx, ebp					; Answer-high
											; NOTE: The answer is 0..3.9999 in fg29
		js		done1						; Overflow if high bit set
		or		bl,	bl						; ZERO	IF NONE	OR BOTH	X ,	Y NEG
		jz		signok						; ONE IF ONLY ONE OF X OR Y	IS NEG
		not		eax							; negate result
		not		edx							;	...
		add		eax, 1						;	 ...
		adc		edx, 0						;	 ...
		xor		bl,	bl						; Clear	negswt
signok:
		add		eax, dword ptr linity
		adc		edx, dword ptr linity + 4		; edx, eax =	2(X*Y) + B
		jo		done1
		mov		dword ptr y, eax			; save the new value	of y
		mov		dword ptr y + 4, edx			;  ...
		mov		edx, dword ptr g_old_color_iter + 4 ; recall	the	old	color
		cmp		edx, dword ptr k + 4			; check it against this iter
		jb		short chkkey4				;  nope.  bypass periodicity check.
		mov		eax, dword ptr g_old_color_iter	; recall	the	old	color
		cmp		eax, dword ptr k			; check it against this iter
		jb		short chkkey4				;  nope.  bypass periodicity check.
		call	checkperiod					; check	for	periodicity

chkkey4:
		mov		eax, dword ptr k			; set up	to test	for	key	stroke
		test	eax, KEYPRESSDELAY
		jne		notakey4					; don't	test yet
		push	ecx
		push	bx
		call	driver_key_pressed			; has a	key	been pressed?
		pop		bx
		pop		ecx
		cmp		eax, 0						;	 ...
		je		notakey4					; nope.	 proceed
		mov		ecx, -1
		jmp		done0						; ecx set, jump	to very	end
notakey4:

		cmp		g_show_orbit,	0				; orbiting	on?
		jne		horbit						;  yep.
		jmp		nextit						; go	around again

horbit:	push	bx							; save my flags
		mov		eax, -1						;	color for plot orbit
		push	eax							;	...
		push	dword ptr y + 4				; co-ordinates	for	plot orbit
		push	dword ptr y					;	...
		push	dword ptr x + 4				;	...
		push	dword ptr x					;	...
		call	plot_orbit_i					; display the orbit
		add		sp,	5*2						; clear out the parameters
		pop		bx							; restore flags
		jmp		nextit						; go around	again
	}

	return 0;
}
