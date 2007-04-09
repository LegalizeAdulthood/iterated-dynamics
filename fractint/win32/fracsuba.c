#include "port.h"
#include "prototyp.h"

int FManOWarfpFractal(void)
{
	return man_o_war_orbit_fp();
}

int FJuliafpFractal(void)
{
	return julia_orbit_fp();
}

int FBarnsley1FPFractal(void)
{
	return barnsley1_orbit_fp();
}

int FBarnsley2FPFractal(void)
{
	return barnsley2_orbit_fp();
}

int FLambdaFPFractal(void)
{
	return lambda_orbit_fp();
}


int asmlMODbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.x) > g_limit2_l
		|| labs(g_new_z_l.y) > g_limit2_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlREALbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlIMAGbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_y_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlORbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_temp_sqr_y_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlANDbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if ((g_temp_sqr_x_l >= g_limit_l && g_temp_sqr_y_l >= g_limit_l) || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlMANHbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asmlMANRbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x + g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lMODbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
	if (g_magnitude_l >= g_limit_l || g_magnitude_l < 0 || labs(g_new_z_l.x) > g_limit2_l
		|| labs(g_new_z_l.y) > g_limit2_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lREALbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lIMAGbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_y_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lORbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if (g_temp_sqr_x_l >= g_limit_l || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lANDbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	if ((g_temp_sqr_x_l >= g_limit_l && g_temp_sqr_y_l >= g_limit_l) || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lMANHbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

int asm386lMANRbailout(void)
{
	g_temp_sqr_x_l = lsqr(g_new_z_l.x);
	g_temp_sqr_y_l = lsqr(g_new_z_l.y);
	g_magnitude = fabs(g_new_z.x + g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
}

/*
asmfpMODbailout proc near uses si di
		fld     qword ptr new + 8
		fmul    st, st                   ; ny2
		fst     g_temp_sqr_y
		fld     qword ptr new			; nx ny2
		fmul    st, st                   ; nx2 ny2
		fst     g_temp_sqr_x
		fadd
		fst     g_magnitude
		fcomp   g_rq_limit                   ; stack is empty
		fstsw   ax                      ; 287 and up only
		sahf
		jae     bailout
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpMODbailout endp
*/
int asmfpMODbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
	if (g_magnitude > g_rq_limit || g_magnitude < 0.0 || fabs(g_new_z.x) > g_rq_limit2 ||
		fabs(g_new_z.y) > g_rq_limit2 || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpREALbailout proc near uses si di
		fld     qword ptr new
		fmul    st, st                   ; nx2
		fst     g_temp_sqr_x
		fld     qword ptr new + 8 ; ny nx2
		fmul    st, st                   ; ny2 nx2
		fst     g_temp_sqr_y                ; ny2 nx2
		fadd    st, st(1)                ; ny2 + nx2 nx2
		fstp    g_magnitude               ; nx2
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpREALbailout endp
*/
int asmfpREALbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	if (g_temp_sqr_x >= g_rq_limit || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpIMAGbailout proc near uses si di
		fld     qword ptr new + 8
		fmul    st, st                   ; ny2
		fst     g_temp_sqr_y
		fld     qword ptr new   ; nx ny2
		fmul    st, st                   ; nx2 ny2
		fst     g_temp_sqr_x                ; nx2 ny2
		fadd    st, st(1)                ; nx2 + ny2 ny2
		fstp    g_magnitude               ; ny2
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpIMAGbailout endp
*/
int asmfpIMAGbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	if (g_temp_sqr_y >= g_rq_limit || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpORbailout proc near uses si di
		fld     qword ptr new + 8
		fmul    st, st                   ; ny2
		fst     g_temp_sqr_y
		fld     qword ptr new   ; nx ny2
		fmul    st, st                   ; nx2 ny2
		fst     g_temp_sqr_x
		fld     st(1)                   ; ny2 nx2 ny2
		fadd    st, st(1)                ; ny2 + nx2 nx2 ny2
		fstp    g_magnitude               ; nx2 ny2
		fcomp   g_rq_limit                   ; ny2
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailoutp
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailoutp:
		finit           ; cleans up stack
bailout:
		mov     ax, 1
		ret
asmfpORbailout endp
*/
int asmfpORbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	if (g_temp_sqr_x >= g_rq_limit || g_temp_sqr_y >= g_rq_limit || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpANDbailout proc near uses si di
		fld     qword ptr new + 8
		fmul    st, st                   ; ny2
		fst     g_temp_sqr_y
		fld     qword ptr new   ; nx ny2
		fmul    st, st                   ; nx2 ny2
		fst     g_temp_sqr_x
		fld     st(1)                   ; ny2 nx2 ny2
		fadd    st, st(1)                ; ny2 + nx2 nx2 ny2
		fstp    g_magnitude               ; nx2 ny2
		fcomp   g_rq_limit                   ; ny2
		fstsw   ax                      ; ** 287 and up only
		sahf
		jb      nobailoutp
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		jmp     short nobailout
nobailoutp:
		finit           ; cleans up stack
nobailout:
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpANDbailout endp
*/
int asmfpANDbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	if ((g_temp_sqr_x >= g_rq_limit && g_temp_sqr_y >= g_rq_limit) || g_overflow)
	{
		g_overflow = 0;
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpMANHbailout proc near uses si di
		fld     qword ptr new + 8
		fld     st
		fmul    st, st                   ; ny2 ny
		fst     g_temp_sqr_y
		fld     qword ptr new   ; nx ny2 ny
		fld     st
		fmul    st, st                   ; nx2 nx ny2 ny
		fst     g_temp_sqr_x
		faddp   st(2), st                ; nx nx2 + ny2 ny
		fxch    st(1)                   ; nx2 + ny2 nx ny
		fstp    g_magnitude               ; nx ny
		fabs
		fxch
		fabs
		fadd                            ; |nx| + |ny|
		fmul    st, st                   ; (|nx| + |ny|)2
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		jmp     short nobailout
nobailoutp:
		finit           ; cleans up stack
nobailout:
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpMANHbailout endp
*/
int asmfpMANHbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

/*
asmfpMANRbailout proc near uses si di
		fld     qword ptr new + 8
		fld     st
		fmul    st, st                   ; ny2 ny
		fst     g_temp_sqr_y
		fld     qword ptr new           ; nx ny2 ny
		fld     st
		fmul    st, st                   ; nx2 nx ny2 ny
		fst     g_temp_sqr_x
		faddp   st(2), st                ; nx nx2 + ny2 ny
		fxch    st(1)                   ; nx2 + ny2 nx ny
		fstp    g_magnitude               ; nx ny
		fadd                            ; nx + ny
		fmul    st, st                   ; square, don't need abs
		fcomp   g_rq_limit                   ; ** stack is empty
		fstsw   ax                      ; ** 287 and up only
		sahf
		jae     bailout
		jmp     short nobailout
nobailoutp:
		finit           ; cleans up stack
nobailout:
		mov     si, offset new
		mov     di, offset old
		mov     ax, ds
		mov     es, ax
		mov     cx, 8
		rep     movsw
		xor     ax, ax
		ret
bailout:
		mov     ax, 1
		ret
asmfpMANRbailout endp
*/
int asmfpMANRbailout(void)
{
	/* TODO: verify this code is correct */
	g_temp_sqr_x = sqr(g_new_z.x);
	g_temp_sqr_y = sqr(g_new_z.y);
	g_magnitude = fabs(g_new_z.x + g_new_z.y);
	if (g_magnitude*g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}
