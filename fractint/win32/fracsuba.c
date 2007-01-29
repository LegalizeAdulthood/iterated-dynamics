#include "port.h"
#include "prototyp.h"

int FManOWarfpFractal(void)
{
	return ManOWarfpFractal();
}

int FJuliafpFractal(void)
{
	return JuliafpFractal();
}

int FBarnsley1FPFractal(void)
{
	return Barnsley1FPFractal();
}

int FBarnsley2FPFractal(void)
{
	return Barnsley2FPFractal();
}

int FLambdaFPFractal(void)
{
	return LambdaFPFractal();
}


int asmlMODbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	lmagnitud = ltempsqrx + ltempsqry;
	if (lmagnitud >= llimit || lmagnitud < 0 || labs(lnew.x) > llimit2
		|| labs(lnew.y) > llimit2 || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlREALbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqrx >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlIMAGbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqry >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlORbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqrx >= llimit || ltempsqry >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlANDbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if ((ltempsqrx >= llimit && ltempsqry >= llimit) || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlMANHbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	magnitude = fabs(g_new.x) + fabs(g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	lold = lnew;
	return 0;
}

int asmlMANRbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	magnitude = fabs(g_new.x + g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lMODbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	lmagnitud = ltempsqrx + ltempsqry;
	if (lmagnitud >= llimit || lmagnitud < 0 || labs(lnew.x) > llimit2
		|| labs(lnew.y) > llimit2 || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lREALbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqrx >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lIMAGbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqry >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lORbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if (ltempsqrx >= llimit || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lANDbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	if ((ltempsqrx >= llimit && ltempsqry >= llimit) || overflow)
	{
		overflow = 0;
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lMANHbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	magnitude = fabs(g_new.x) + fabs(g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	lold = lnew;
	return 0;
}

int asm386lMANRbailout(void)
{
	ltempsqrx = lsqr(lnew.x);
	ltempsqry = lsqr(lnew.y);
	magnitude = fabs(g_new.x + g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	lold = lnew;
	return 0;
}

/*
asmfpMODbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2
        fst     tempsqry
        fld     qword ptr new			; nx ny2
        fmul    st,st                   ; nx2 ny2
        fst     tempsqrx
        fadd
        fst     magnitude
        fcomp   rqlim                   ; stack is empty
        fstsw   ax                      ; 287 and up only
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMODbailout endp
*/
int asmfpMODbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	magnitude = tempsqrx + tempsqry;
	if (magnitude > rqlim || magnitude < 0.0 || fabs(g_new.x) > rqlim2 ||
		fabs(g_new.y) > rqlim2 || overflow)
	{
		overflow = 0;
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpREALbailout proc near uses si di
        fld     qword ptr new
        fmul    st,st                   ; nx2 
        fst     tempsqrx
        fld     qword ptr new+8 ; ny nx2 
        fmul    st,st                   ; ny2 nx2 
        fst     tempsqry                ; ny2 nx2 
        fadd    st,st(1)                ; ny2+nx2 nx2 
        fstp    magnitude               ; nx2 
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpREALbailout endp
*/
int asmfpREALbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	if (tempsqrx >= rqlim || overflow)
	{
		overflow = 0;
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpIMAGbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2 
        fst     tempsqry
        fld     qword ptr new   ; nx ny2 
        fmul    st,st                   ; nx2 ny2 
        fst     tempsqrx                ; nx2 ny2 
        fadd    st,st(1)                ; nx2+ny2 ny2 
        fstp    magnitude               ; ny2 
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpIMAGbailout endp
*/
int asmfpIMAGbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	if (tempsqry >= rqlim || overflow)
	{
		overflow = 0;
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpORbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2 
        fst     tempsqry
        fld     qword ptr new   ; nx ny2 
        fmul    st,st                   ; nx2 ny2 
        fst     tempsqrx
        fld     st(1)                   ; ny2 nx2 ny2 
        fadd    st,st(1)                ; ny2+nx2 nx2 ny2 
        fstp    magnitude               ; nx2 ny2 
        fcomp   rqlim                   ; ny2 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailoutp
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailoutp:
        finit           ; cleans up stack 
bailout:
        mov     ax,1
        ret
asmfpORbailout endp
*/
int asmfpORbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	if (tempsqrx >= rqlim || tempsqry >= rqlim || overflow)
	{
		overflow = 0;
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpANDbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2 
        fst     tempsqry
        fld     qword ptr new   ; nx ny2 
        fmul    st,st                   ; nx2 ny2 
        fst     tempsqrx
        fld     st(1)                   ; ny2 nx2 ny2 
        fadd    st,st(1)                ; ny2+nx2 nx2 ny2 
        fstp    magnitude               ; nx2 ny2 
        fcomp   rqlim                   ; ny2 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jb      nobailoutp
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack 
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpANDbailout endp
*/
int asmfpANDbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	if ((tempsqrx >= rqlim && tempsqry >= rqlim) || overflow)
	{
		overflow = 0;
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpMANHbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ; ny2 ny 
        fst     tempsqry
        fld     qword ptr new   ; nx ny2 ny 
        fld     st
        fmul    st,st                   ; nx2 nx ny2 ny 
        fst     tempsqrx
        faddp   st(2),st                ; nx nx2+ny2 ny 
        fxch    st(1)                   ; nx2+ny2 nx ny 
        fstp    magnitude               ; nx ny 
        fabs
        fxch
        fabs
        fadd                            ; |nx|+|ny| 
        fmul    st,st                   ; (|nx|+|ny|)2 
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack 
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMANHbailout endp
*/
int asmfpMANHbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	magnitude = fabs(g_new.x) + fabs(g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	old = g_new;
	return 0;
}

/*
asmfpMANRbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ; ny2 ny 
        fst     tempsqry
        fld     qword ptr new           ; nx ny2 ny 
        fld     st
        fmul    st,st                   ; nx2 nx ny2 ny 
        fst     tempsqrx
        faddp   st(2),st                ; nx nx2+ny2 ny 
        fxch    st(1)                   ; nx2+ny2 nx ny 
        fstp    magnitude               ; nx ny 
        fadd                            ; nx+ny 
        fmul    st,st                   ; square, don't need abs
        fcomp   rqlim                   ; ** stack is empty 
        fstsw   ax                      ; ** 287 and up only 
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack 
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMANRbailout endp
*/
int asmfpMANRbailout(void)
{
	tempsqrx = sqr(g_new.x);
	tempsqry = sqr(g_new.y);
	magnitude = fabs(g_new.x + g_new.y);
	if (magnitude*magnitude >= rqlim)
	{
		return 1;
	}
	old = g_new;
	return 0;
}
