;       FRACASM.ASM - Assembler subroutines for fractals.c

;                        required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.MODEL  medium,c

.8086

        extrn   floatbailout:word               ; this routine is in 'fractals.c'

.data

.code FRACTALS_TEXT

;  Fast fractal orbit calculation procs for Fractint.
;  By Chuck Ebbert   CIS: (76306,1226)
;
;       FManOWarfpFractal()
;       FJuliafpFractal()
;       FBarnsley1FPFractal()
;       FBarnsley2FPFractal()
;       FLambdaFPFractal()
;
;       asmfloatbailout()     -- bailout proc (NEAR proc used by above)
;               NOTE: asmfloatbailout() modifies SI and DI.
;
;  These will only run on machines with a 287 or a 387 (and a 486 of course.)
;
;  Some of the speed gains from this code are due to the storing of the NPU
;    status word directly to the AX register.  FRACTINT will use these
;        routines only when it finds a 287 or better coprocessor installed.

.286
.287

.data

        extrn new:qword, old:qword, tmp:qword
        extrn rqlim:qword, magnitude:qword, tempsqrx:qword, tempsqry:qword
        extrn cpu:word, fpu:word, floatparm:word

.code FRACTALS_TEXT

        ; px,py = floatparm->x,y
        ; ox,oy = oldx,oldy
        ; nx,ny = newx,newy
        ; nx2,ny2 = newxsquared,newysquared
        ; tx,ty = tempx, tempy (used in lambda)

FManOWarfpFractal       proc uses si di
                           ; From Art Matrix via Lee Skinner
        fld     tempsqrx                ; ox2
        fsub    tempsqry                ; ox2-oy2
        mov     bx,floatparm
        fadd    tmp                     ; ox2-oy2+tx
        fadd    qword ptr [bx]          ; newx
        fld     old                     ; oldx newx
        fmul    old+8                   ; oldx*oldy newx
        fadd    st,st                   ; oldx*oldy*2 newx
        fadd    tmp+8                   ; oldx*oldy*2+tmp.y newx
        fadd    qword ptr [bx+8]        ; newy newx
        fstp    qword ptr new+8 ; newx
        fstp    qword ptr new   ; stack is empty
        mov     si,offset old           ; tmp=old
        mov     di,offset tmp
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
;       call    near ptr asmfloatbailout
        call    word ptr [floatbailout]
        ret
FManOWarfpFractal       endp

FJuliafpFractal proc uses si di
        ; Julia/Mandelbrot floating-point orbit function.
        fld     tempsqrx
        fsub    tempsqry
        mov     bx,floatparm
        fadd    qword ptr [bx]          ;/* add floatparm->x */
        fld     qword ptr old           ;/* now get 2*x*y+floatparm->y */
        fmul    qword ptr old+8
        fadd    st,st
        fadd    qword ptr [bx+8]        ; add floatparm->y
        fstp    qword ptr new+8 ; newx
        fstp    qword ptr new   ; stack is empty
;       call    near ptr asmfloatbailout
        call    word ptr [floatbailout]
        ret
FJuliafpFractal                 endp

FBarnsley1FPFractal     proc uses si di
        ; From Fractals Everywhere by Michael Barnsley
        mov     bx,floatparm
        fld     qword ptr [bx]          ;/* STACK: px */
        fld     qword ptr [bx+8]        ;/* py px */
        fld     qword ptr old           ;/* ox py px */
        ftst                            ;/*** we will want this later */
        fstsw   ax                      ;/*** 287 or better only */
        fld     old+8                   ;/* oy ox py px */
        fld     st(1)                   ;/* ox oy ox py px */
        fmul    st,st(4)                ;/* ox*px oy ox py px */
        fld     st(1)                   ;/* oy ox*px oy ox py px */
        fmul    st,st(4)                ;/* oy*py ox*px oy ox py px */
        fsub                            ;/* (ox*px-oy*py) oy ox py px */
        fxch                            ;/* oy (oxpx-oypy) ox py px */
        fmul    st,st(4)                ;/* oy*px (oxpx-oypy) ox py px */
        fxch    st(2)                   ;/* ox (oxpx-oypy) oy*px py px */
        fmul    st,st(3)                ;/* ox*py (oxpx-oypy) oy*px py px */
        faddp   st(2),st                ;/* oxpx-oypy oypx+oxpy py px */
        sahf                            ;/*** use the saved status (finally) */
        jb      BFPM1add
        fsubrp  st(3),st                ;/* oypx+oxpy py nx */
        fsubr                           ;/* ny nx */
        jmp     short BFPM1cont
BFPM1add:
        faddp   st(3),st                ;/* oypx+oxpy py nx */
        fadd                            ;/* ny nx */
BFPM1cont:
        fstp    qword ptr new+8 ; newx
        fstp    qword ptr new   ; stack is empty
;       call    near ptr asmfloatbailout
        call    word ptr [floatbailout]
        ret
FBarnsley1FPFractal endp

FBarnsley2FPFractal proc uses si di
        ; Also from Fractals Everywhere
        mov     bx,floatparm
        fld     qword ptr [bx]          ;/* STACK: px */
        fld     qword ptr [bx+8]        ;/* py px */
        fld     qword ptr old           ;/* ox py px */
        fld     qword ptr old+8         ;/* oy ox py px */
        fld     st(1)                   ;/* ox oy ox py px */
        fmul    st,st(4)                ;/* ox*px oy ox py px */
        fld     st(1)                   ;/* oy ox*px oy ox py px */
        fmul    st,st(4)                ;/* oy*py ox*px oy ox py px */
        fsub                            ;/* (ox*px-oy*py) oy ox py px */
        fxch    st(2)                   ;/* ox oy (oxpx-oypy) py px */
        fmul    st,st(3)                ;/* ox*py oy (oxpx-oypy) py px */
        fxch                            ;/* oy ox*py (oxpx-oypy) py px */
        fmul    st,st(4)                ;/* oy*px ox*py (oxpx-oypy) py px */
        fadd                            ;/* oypx+oxpy oxpx-oypy py px */
        ftst
        fstsw   ax                      ;/* 287 or better only */
        sahf
        jb      BFPM2add
        fsubrp  st(2),st                ;/* oxpx-oypy ny px */
        fsubrp  st(2),st                ;/* ny nx */
        jmp     short BFPM2cont
BFPM2add:
        faddp   st(2),st                ;/* oxpx-oypy ny px */
        faddp   st(2),st                ;/* ny nx */
BFPM2cont:
        fstp    qword ptr new+8 ; newx
        fstp    qword ptr new   ; stack is empty
;       call    near ptr asmfloatbailout
        call    word ptr [floatbailout]
        ret
FBarnsley2FPFractal endp

FLambdaFPFractal proc uses si di
        ; tempsqrx and tempsqry can be used -- the C code doesn't use them!
        fld     tempsqry                ;oy2
        fsub    tempsqrx                ;oy2-ox2
        fld     old                     ;ox oy2-ox2
        fadd    st(1),st                ;ox tx=ox+oy2-ox2
        fadd    st,st                   ;2ox tx
        fld1                            ;1 2ox tx
        fsubr                           ;(1-2ox) tx
        fmul    old+8                   ;ty=oy(1-2ox) tx
        fld     st(1)                   ;tx ty tx -- duplicate these now
        fld     st(1)                   ;ty tx ty tx
        mov     bx,floatparm
        fld     qword ptr [bx+8]        ;py ty tx ty tx -- load y first
        fld     qword ptr [bx]          ;px py ty tx ty tx
        fmul    st(5),st                ;px py ty tx ty pxtx
        fmulp   st(4),st                ;py ty tx pxty pxtx
        fmul    st(2),st                ;py ty pytx pxty pxtx
        fmul                            ;pyty pytx pxty pxtx
        fsubp   st(3),st                ;pytx pxty nx=pxtx-pyty
        fadd                            ;ny=pxty+pytx nx
        fstp    qword ptr new+8 ; newx
        fstp    qword ptr new   ; stack is empty
;       call    near ptr asmfloatbailout
        call    word ptr [floatbailout]
        ret
FLambdaFPFractal endp

; The following is no longer used.
asmfloatbailout proc near
        ; called with new.y and new.x on stack and clears the stack
        ; destroys SI and DI: caller must save them
        fst     qword ptr new+8
        fmul    st,st                   ;/* ny2 nx */
        fst     tempsqry
        fxch                            ;/* nx ny2 */
        fst     qword ptr new
        fmul    st,st                   ;/* nx2 ny2 */
        fst     tempsqrx
        fadd
        fst     magnitude
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
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
asmfloatbailout endp
; The preceeding is no longer used.

asmfpMODbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ;/* ny2 */
        fst     tempsqry
        fld     qword ptr new   ;/* nx ny2 */
        fmul    st,st                   ;/* nx2 ny2 */
        fst     tempsqrx
        fadd
        fst     magnitude
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
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

asmfpREALbailout proc near uses si di
        fld     qword ptr new
        fmul    st,st                   ;/* nx2 */
        fst     tempsqrx
        fld     qword ptr new+8 ;/* ny nx2 */
        fmul    st,st                   ;/* ny2 nx2 */
        fst     tempsqry                ;/* ny2 nx2 */
        fadd    st,st(1)                ;/* ny2+nx2 nx2 */
        fstp    magnitude               ;/* nx2 */
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
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

asmfpIMAGbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ;/* ny2 */
        fst     tempsqry
        fld     qword ptr new   ;/* nx ny2 */
        fmul    st,st                   ;/* nx2 ny2 */
        fst     tempsqrx                ;/* nx2 ny2 */
        fadd    st,st(1)                ;/* nx2+ny2 ny2 */
        fstp    magnitude               ;/* ny2 */
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
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

asmfpORbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ;/* ny2 */
        fst     tempsqry
        fld     qword ptr new   ;/* nx ny2 */
        fmul    st,st                   ;/* nx2 ny2 */
        fst     tempsqrx
        fld     st(1)                   ;/* ny2 nx2 ny2 */
        fadd    st,st(1)                ;/* ny2+nx2 nx2 ny2 */
        fstp    magnitude               ;/* nx2 ny2 */
        fcomp   rqlim                   ;/* ny2 */
        fstsw   ax                      ;/*** 287 and up only */
        sahf
        jae     bailoutp
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
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
        finit           ;/* cleans up stack */
bailout:
        mov     ax,1
        ret
asmfpORbailout endp

asmfpANDbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ;/* ny2 */
        fst     tempsqry
        fld     qword ptr new   ;/* nx ny2 */
        fmul    st,st                   ;/* nx2 ny2 */
        fst     tempsqrx
        fld     st(1)                   ;/* ny2 nx2 ny2 */
        fadd    st,st(1)                ;/* ny2+nx2 nx2 ny2 */
        fstp    magnitude               ;/* nx2 ny2 */
        fcomp   rqlim                   ;/* ny2 */
        fstsw   ax                      ;/*** 287 and up only */
        sahf
        jb      nobailoutp
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ;/* cleans up stack */
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

asmfpMANHbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ;/* ny2 ny */
        fst     tempsqry
        fld     qword ptr new   ;/* nx ny2 ny */
        fld     st
        fmul    st,st                   ;/* nx2 nx ny2 ny */
        fst     tempsqrx
        faddp   st(2),st                ;/* nx nx2+ny2 ny */
        fxch    st(1)                   ;/* nx2+ny2 nx ny */
        fstp    magnitude               ;/* nx ny */
        fabs
        fxch
        fabs
        fadd                            ;/* |nx|+|ny| */
        fmul    st,st                   ;/* (|nx|+|ny|)2 */
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ;/* cleans up stack */
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

asmfpMANRbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ;/* ny2 ny */
        fst     tempsqry
        fld     qword ptr new           ;/* nx ny2 ny */
        fld     st
        fmul    st,st                   ;/* nx2 nx ny2 ny */
        fst     tempsqrx
        faddp   st(2),st                ;/* nx nx2+ny2 ny */
        fxch    st(1)                   ;/* nx2+ny2 nx ny */
        fstp    magnitude               ;/* nx ny */
        fadd                            ;/* nx+ny */
        fmul    st,st                   ; square, don't need abs
        fcomp   rqlim                   ;/*** stack is empty */
        fstsw   ax                      ;/*** 287 and up only */
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ;/* cleans up stack */
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

.8086
.8087

        end

