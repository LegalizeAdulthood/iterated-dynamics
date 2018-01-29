#include <float.h>
#include <stdlib.h>

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fractals.h"

int asmlMODbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2
        || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlREALbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlIMAGbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_y >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlORbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_l_temp_sqr_y >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlANDbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if ((g_l_temp_sqr_x >= g_l_magnitude_limit && g_l_temp_sqr_y >= g_l_magnitude_limit) || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlMANHbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asmlMANRbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = fabs(g_new_z.x + g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lMODbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2
        || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lREALbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lIMAGbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_y >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lORbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lANDbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if ((g_l_temp_sqr_x >= g_l_magnitude_limit && g_l_temp_sqr_y >= g_l_magnitude_limit) || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lMANHbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int asm386lMANRbailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = fabs(g_new_z.x + g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

// asmfpMODbailout proc near uses si di
//         fld     qword ptr new+8
//         fmul    st,st                   ; ny2
//         fst     tempsqry
//         fld     qword ptr new           ; nx ny2
//         fmul    st,st                   ; nx2 ny2
//         fst     tempsqrx
//         fadd
//         fst     magnitude
//         fcomp   rqlim                   ; stack is empty
//         fstsw   ax                      ; 287 and up only
//         sahf
//         jae     bailout
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpMODbailout endp
//
int asmfpMODbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude > g_magnitude_limit
        || g_magnitude < 0.0
        || fabs(g_new_z.x) > g_magnitude_limit2
        || fabs(g_new_z.y) > g_magnitude_limit2
        || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpREALbailout proc near uses si di
//         fld     qword ptr new
//         fmul    st,st                   ; nx2
//         fst     tempsqrx
//         fld     qword ptr new+8 ; ny nx2
//         fmul    st,st                   ; ny2 nx2
//         fst     tempsqry                ; ny2 nx2
//         fadd    st,st(1)                ; ny2+nx2 nx2
//         fstp    magnitude               ; nx2
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpREALbailout endp
//
int asmfpREALbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    if (g_temp_sqr_x >= g_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpIMAGbailout proc near uses si di
//         fld     qword ptr new+8
//         fmul    st,st                   ; ny2
//         fst     tempsqry
//         fld     qword ptr new   ; nx ny2
//         fmul    st,st                   ; nx2 ny2
//         fst     tempsqrx                ; nx2 ny2
//         fadd    st,st(1)                ; nx2+ny2 ny2
//         fstp    magnitude               ; ny2
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpIMAGbailout endp
//
int asmfpIMAGbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    if (g_temp_sqr_y >= g_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpORbailout proc near uses si di
//         fld     qword ptr new+8
//         fmul    st,st                   ; ny2
//         fst     tempsqry
//         fld     qword ptr new   ; nx ny2
//         fmul    st,st                   ; nx2 ny2
//         fst     tempsqrx
//         fld     st(1)                   ; ny2 nx2 ny2
//         fadd    st,st(1)                ; ny2+nx2 nx2 ny2
//         fstp    magnitude               ; nx2 ny2
//         fcomp   rqlim                   ; ny2
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailoutp
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailoutp:
//         finit           ; cleans up stack
// bailout:
//         mov     ax,1
//         ret
// asmfpORbailout endp
//
int asmfpORbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    if (g_temp_sqr_x >= g_magnitude_limit || g_temp_sqr_y >= g_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpANDbailout proc near uses si di
//         fld     qword ptr new+8
//         fmul    st,st                   ; ny2
//         fst     tempsqry
//         fld     qword ptr new   ; nx ny2
//         fmul    st,st                   ; nx2 ny2
//         fst     tempsqrx
//         fld     st(1)                   ; ny2 nx2 ny2
//         fadd    st,st(1)                ; ny2+nx2 nx2 ny2
//         fstp    magnitude               ; nx2 ny2
//         fcomp   rqlim                   ; ny2
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jb      nobailoutp
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         jmp     short nobailout
// nobailoutp:
//         finit           ; cleans up stack
// nobailout:
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpANDbailout endp
//
int asmfpANDbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    if ((g_temp_sqr_x >= g_magnitude_limit && g_temp_sqr_y >= g_magnitude_limit) || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpMANHbailout proc near uses si di
//         fld     qword ptr new+8
//         fld     st
//         fmul    st,st                   ; ny2 ny
//         fst     tempsqry
//         fld     qword ptr new   ; nx ny2 ny
//         fld     st
//         fmul    st,st                   ; nx2 nx ny2 ny
//         fst     tempsqrx
//         faddp   st(2),st                ; nx nx2+ny2 ny
//         fxch    st(1)                   ; nx2+ny2 nx ny
//         fstp    magnitude               ; nx ny
//         fabs
//         fxch
//         fabs
//         fadd                            ; |nx|+|ny|
//         fmul    st,st                   ; (|nx|+|ny|)2
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         jmp     short nobailout
// nobailoutp:
//         finit           ; cleans up stack
// nobailout:
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpMANHbailout endp
//
int asmfpMANHbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = fabs(g_new_z.x) + fabs(g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// asmfpMANRbailout proc near uses si di
//         fld     qword ptr new+8
//         fld     st
//         fmul    st,st                   ; ny2 ny
//         fst     tempsqry
//         fld     qword ptr new           ; nx ny2 ny
//         fld     st
//         fmul    st,st                   ; nx2 nx ny2 ny
//         fst     tempsqrx
//         faddp   st(2),st                ; nx nx2+ny2 ny
//         fxch    st(1)                   ; nx2+ny2 nx ny
//         fstp    magnitude               ; nx ny
//         fadd                            ; nx+ny
//         fmul    st,st                   ; square, don't need abs
//         fcomp   rqlim                   ; ** stack is empty
//         fstsw   ax                      ; ** 287 and up only
//         sahf
//         jae     bailout
//         jmp     short nobailout
// nobailoutp:
//         finit           ; cleans up stack
// nobailout:
//         mov     si,offset new
//         mov     di,offset old
//         mov     ax,ds
//         mov     es,ax
//         mov     cx,8
//         rep     movsw
//         xor     ax,ax
//         ret
// bailout:
//         mov     ax,1
//         ret
// asmfpMANRbailout endp
//
int asmfpMANRbailout()
{
    // TODO: verify this code is correct
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = fabs(g_new_z.x + g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}
