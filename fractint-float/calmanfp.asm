;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; calmanfp.asm - floating point version of the calcmand.asm file
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The following code was adapted from a little program called "Mandelbrot
; Sets by Wesley Loewer" which had a very limited distribution (my
; Algebra II class).  It didn't have any of the fancy integer math, but it
; did run floating point stuff pretty fast.
;
; The code was originally optimized for a 287 ('cuz that's what I've got)
; and for a large maxit (ie: use of generous overhead outside the loop to get
; slightly faster code inside the loop), which is generally the case when
; Fractint chooses to use floating point math.  This code also has the
; advantage that once the initial parameters are loaded into the fpu
; register, no transfers of fp values to/from memory are needed except to
; check periodicity and to show orbits and the like.  Thus, values keep all
; the significant digits of the full 10 byte real number format internal to
; the fpu.  Intermediate results are not rounded to the normal IEEE 8 byte
; format (double) at any time.
;
; The non fpu specific stuff, such as periodicity checking and orbits,
; was adapted from CALCFRAC.C and CALCMAND.ASM.
;
; This file must be assembled with floating point emulation turned on.  I
; suppose there could be some compiler differences in the emulation
; libraries, but this code has been successfully tested with the MSQC 2.51
; and MSC 5.1 emulation libraries.
;
;                                               Wes Loewer
;
; and now for some REAL fractal calculations...
; (get it, real, floating point..., never mind)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;   1. Made maxit a dword variable. 1/18/94
;   2. Added p5 support, fixed bug with real, imag, mult, & summ and
;       negative numbers. 7/13/97
;   3. Fixed FPUatan bugs, made individual _p5, _287, and _87 per pixel
;       routines for speed.  Put check for FPU in setup code (frasetup.c).
;       7/20/97

;                        required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.8086
.8087

.MODEL medium,c

; external functions
EXTRN   keypressed:FAR          ; this routine is in 'general.asm'
EXTRN   getakey:FAR             ; this routine is in 'general.asm'
EXTRN   plot_orbit:FAR          ; this routine is in 'fracsubr.c'
EXTRN   scrub_orbit:FAR         ; this routine is in 'fracsubr.c'

; external data
EXTRN init:WORD                 ; declared as type complex
EXTRN parm:WORD                 ; declared as type complex
EXTRN new:WORD                  ; declared as type complex
EXTRN maxit:DWORD
EXTRN inside:WORD
EXTRN outside:WORD
EXTRN fpu:WORD                  ; fpu type: 87, 287, or 387
EXTRN cpu:WORD                  ; cpu type
EXTRN rqlim:QWORD               ; bailout (I never did figure out
                                ;   what "rqlim" stands for. -Wes)
EXTRN coloriter:DWORD
EXTRN oldcoloriter:DWORD
EXTRN realcoloriter:DWORD
EXTRN periodicitycheck:WORD
EXTRN reset_periodicity:WORD
EXTRN closenuff:QWORD
EXTRN fractype:WORD             ; Mandelbrot or Julia
EXTRN kbdcount:WORD            ; keyboard counter
EXTRN dotmode:WORD
EXTRN show_orbit:WORD           ; "show-orbit" flag
EXTRN orbit_ptr:WORD            ; "orbit pointer" flag
EXTRN potflag:WORD              ; potential flag
EXTRN magnitude:QWORD           ; when using potential
extrn   nextsavedincr:word              ; for incrementing AND value
extrn   firstsavedand:dword             ; AND value
extrn   bad_outside:word        ; old FPU code with bad: real,imag,mult,summ
extrn   save_release:word
extrn   showdot:word
extrn   orbit_delay:word
extrn   atan_colors:word

JULIAFP  EQU 3                  ; from FRACTYPE.H
MANDELFP EQU 0
GREEN    EQU 2                  ; near y-axis
YELLOW   EQU 6                  ; near x-axis
KEYPRESSDELAY equ 16383         ; 3FFFh

initx    EQU <qword ptr init>   ; just to make life easier
inity    EQU <qword ptr init+8>
parmx    EQU <qword ptr parm>
parmy    EQU <qword ptr parm+8>
newx     EQU <qword ptr new>
newy     EQU <qword ptr new+8>

; Apparently, these might be needed for TC++ overlays. I don't know if
; these are really needed here since I am not familiar with TC++. -Wes
FRAME   MACRO regs
        push    bp
        mov     bp, sp
        IRP     reg, <regs>
          push  reg
          ENDM
        ENDM

UNFRAME MACRO regs
        IRP     reg, <regs>
          pop reg
          ENDM
        pop bp
        ENDM


.DATA
        align   2
savedx                  DQ  ?
savedy                  DQ  ?
orbit_real              DQ  ?
orbit_imag              DQ  ?
_2_                     DQ  2.0
_4_                     DQ  4.0
close                   DD  0.01
round_down_half         DD  0.5
tmp_word                DW  ?
tmp_dword               DD  ?
inside_color            DD  ?
periodicity_color       DD  7
savedand                DD  ?   ; need 4 bytes now, not 2
;savedincr              DW  ?
;savedand                EQU     SI      ; this doesn't save much time or
savedincr               EQU     DI      ; space, but it doesn't hurt either

.CODE
.8086
.8087
EVEN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This routine is called once per image.
; Put things here that won't change from one pixel to the next.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC calcmandfpasmstart
calcmandfpasmstart   PROC
                                        ; not sure if needed here
        FRAME   <di,si>                 ; std frame, for TC++ overlays

        sub     dx,dx                   ; clear dx
        mov     ax,inside
        cmp     ax,0                    ; if (inside color == maxiter)
        jnl     non_neg_inside
        mov     ax,word ptr maxit       ;   use maxit as inside_color
        mov     dx,word ptr maxit+2     ;   use maxit as inside_color

non_neg_inside:                         ; else
        mov     word ptr inside_color,ax  ;   use inside as inside_color
        mov     word ptr inside_color+2,dx ;   use inside as inside_color

        cmp     periodicitycheck,0      ; if periodicitycheck < 0
        jnl     non_neg_periodicitycheck
        mov     ax,7                    ;   use color 7 (default white)
        sub     dx,dx                   ; clear dx
non_neg_periodicitycheck:               ; else
        mov     word ptr periodicity_color,ax ; use inside_color still in ax
        mov     word ptr periodicity_color+2,dx ; use inside_color still in dx
        mov     word ptr oldcoloriter,0 ; no periodicity checking on 1st pixel
        mov     word ptr oldcoloriter+2,0 ; no periodicity checking on 1st pixel
;        sub     ax,ax                   ; ax=0
;        sub     dx,dx
        UNFRAME <si,di>                 ; pop stack frame
        ret
calcmandfpasmstart       ENDP

.286
.287
EVEN
PUBLIC calcmandfpasm_287
calcmandfpasm_287  PROC
        FRAME   <di,si>                 ; std frame, for TC++ overlays
; initialization stuff
        sub     ax,ax                   ; clear ax
        mov     dx,ax                   ; dx=0
        cmp     periodicitycheck,ax     ; periodicity checking?
        je      initoldcolor            ;  no, set oldcolor 0 to disable it
;        cmp     inside,-59              ; zmag?
;        je      initoldcolor            ;  set oldcolor to 0
        cmp     reset_periodicity,ax    ; periodicity reset?
        je      short initparms         ;  no, inherit oldcolor from prior invocation
        mov     ax,word ptr maxit       ; yup.  reset oldcolor to maxit-250
        mov     dx,word ptr maxit+2
        sub     ax,250                  ; (avoids slowness at high maxits)
        sbb     dx,0                            ; (faster than conditional jump)
initoldcolor:
        mov     word ptr oldcoloriter,ax   ; reset oldcolor
        mov     word ptr oldcoloriter+2,dx ; reset oldcolor

initparms:
        sub     ax,ax                   ; clear ax
        mov     dx,ax                   ; clear dx
        mov     word ptr savedx,ax      ; savedx = 0.0
        mov     word ptr savedx+2,ax    ; needed since savedx is a QWORD
        mov     word ptr savedx+4,ax
        mov     word ptr savedx+6,ax
        mov     word ptr savedy,ax      ; savedy = 0.0
        mov     word ptr savedy+2,ax    ; needed since savedy is a QWORD
        mov     word ptr savedy+4,ax
        mov     word ptr savedy+6,ax
        mov     ax,word ptr firstsavedand+2 ; high part of savedand=0
        mov     word ptr savedand+2,ax ; high part of savedand=0
        mov     ax,word ptr firstsavedand    ; low part of savedand
        mov     word ptr savedand,ax    ; low part of savedand
        mov     savedincr,1             ; savedincr = 1
        mov     orbit_ptr,0             ; clear orbits
        dec     kbdcount                ; decrement the keyboard counter
        jns     short nokey        ;  skip keyboard test if still positive
        mov     kbdcount,10             ; stuff in a low kbd count
        cmp     show_orbit,0            ; are we showing orbits?
        jne     quickkbd                ;  yup.  leave it that way.
        cmp     orbit_delay,0           ; are we delaying orbits?
        je      slowkbd                 ;  nope.  change it.
        cmp     showdot,0               ; are we showing the current pixel?
        jge     quickkbd                ;  yup.  leave it that way.
;this may need to be adjusted, I'm guessing at the "appropriate" values -Wes
slowkbd:
        mov     kbdcount,3000           ; else, stuff an appropriate count val
                                        ; ("appropriate" to the FPU)
kbddiskadj:
        cmp     dotmode,11              ; disk video?
        jne     quickkbd                ;  no, leave as is
        shr     kbdcount,1              ; yes, reduce count
        shr     kbdcount,1              ; yes, reduce count

quickkbd:
        call    far ptr keypressed      ; has a key been pressed?
        cmp     ax,0                    ;  ...
        je      nokey                   ; nope.  proceed
        mov     kbdcount,0              ; make sure it goes negative again
        cmp     ax,'o'                  ; orbit toggle hit?
        je      orbitkey                ;  yup.  show orbits
        cmp     ax,'O'                  ; orbit toggle hit?
        jne     keyhit                  ;  nope.  normal key.
orbitkey:
        call    far ptr getakey         ; read the key for real
        mov     ax,1                    ; reset orbittoggle = 1 - orbittoggle
        sub     ax,show_orbit           ;  ...
        mov     show_orbit,ax           ;  ...
        jmp     short nokey             ; pretend no key was hit
keyhit:
        mov     ax,-1                   ; return with -1
        mov     dx,ax
        mov     word ptr coloriter,ax   ; set color to -1
        mov     word ptr coloriter+2,dx ; set color to -1
        UNFRAME <si,di>                 ; pop stack frame
        ret                             ; bail out!
nokey:

; OK, here's the heart of the floating point code.
; In my original program, the bailout value was loaded once per image and
; was left on the floating point stack after each pixel, and finally popped
; off the stack when the fractal was finished.  A lot of overhead for very
; little gain in my opinion, so I changed it so that it loads and unloads
; per pixel. -Wes

        fld     rqlim                   ; everything needs bailout first
        mov     cx,word ptr maxit+2     ; using cx and bx as loop counter
        mov     bx,word ptr maxit       ; using cx and bx as loop counter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _287 version (closely resembles original code)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.286
.287
start_287:      ; 287
        cmp     fractype,JULIAFP        ; julia or mandelbrot set?
        je      short dojulia_287       ; julia set - go there

; Mandelbrot _287 initialization of stack
        sub     bx,1                      ; always requires at least 1 iteration
        sbb     cx,0
                                        ; the fpu stack is shown below
                                        ; st(0) ... st(7)
                                        ; b (already on stack)
        fld     inity                   ; Cy b
        fld     initx                   ; Cx Cy b
        fld     st(1)                   ; Cy Cx Cy b
        fadd    parmy                   ; Py+Cy Cx Cy b
        fld1                            ; 1 Py+Cy Cx Cy b
        fld     st(1)                   ; Py+Cy 1 Py+Cy Cx Cy b
        fmul    st,st                   ; (Py+Cy)^2 1 Py+Cy Cx Cy b
        fld     st(3)                   ; Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
        fadd    parmx                   ; Px+Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
        fmul    st(3),st                ; Px+Cx (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
        fmul    st,st                   ; (Px+Cx)^2 (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
        ; which is the next                x^2 y^2 1 xy Cx Cy b
        jmp     short top_of_cx_loop_287 ; branch around the julia switch

dojulia_287:
                                        ; Julia 287 initialization of stack
                                        ; note that init and parm are "reversed"
                                        ; b (already on stack)
        fld     parmy                   ; Cy b
        fld     parmx                   ; Cx Cy b
        fld     inity                   ; y Cx Cy b
        fld1                            ; 1 y Cx Cy b
        fld     st(1)                   ; y 1 y Cx Cy b
        fmul    st,st                   ; y^2 1 y Cx Cy b
        fld     initx                   ; x y^2 1 y Cx Cy b
        fmul    st(3),st                ; x y^2 1 xy Cx Cy b
        fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b

top_of_cx_loop_287:                     ; x^2 y^2 1 xy Cx Cy b
        fsubr                           ; x^2-y^2 1 xy Cx Cy b
        fadd    st,st(3)                ; x^2-y^2+Cx 1 xy Cx Cy b
        fxch    st(2)                   ; xy 1 x^2-y^2+Cx Cx Cy b
; FSCALE is faster than FADD for 287
        fscale                          ; 2xy 1 x^2-y^2+Cx Cx Cy b
        fadd    st,st(4)                ; 2xy+Cy 1 x^2-y^2+Cx Cx Cy b
                                        ; now same as the new
                                        ; y 1 x Cx Cy b

        cmp     outside,-2              ; real, imag, mult, or sum ?
        jg      no_save_new_xy_287      ; if not, then skip this
        fld     st(2)                   ; x y 1 x Cx Cy b
        fstp    newx                    ; y 1 x Cx Cy b
        fst     newy                    ; y 1 x Cx Cy b
no_save_new_xy_287:

;        cmp     inside,-100                     ; epsilon cross ?
;        jne     end_epsilon_cross_287
;        call    near ptr epsilon_cross          ; y 1 x Cx Cy b
;        cmp     bx,0
;        jnz     end_epsilon_cross_287
;        cmp     cx,0
;        jnz   end_epsilon_cross_287             ; if cx=0, pop stack
;        jmp     pop_stack_287
;end_epsilon_cross_287:

        test    bx,KEYPRESSDELAY        ; bx holds the low word of the loop count
        jne     notakey3                ; don't test yet
        push    cx
        push    bx
        call    far ptr keypressed      ; has a key been pressed?
        pop     bx
        pop     cx
        cmp     ax,0                    ;  ...
        je      notakey3                        ; nope.  proceed
        jmp     keyhit
notakey3:

        cmp     cx,word ptr oldcoloriter+2      ; if cx > oldcolor
        ja      end_periodicity_check_287       ; don't check periodicity
        cmp     bx,word ptr oldcoloriter        ; if bx >= oldcolor
        jae     end_periodicity_check_287       ; don't check periodicity
        call    near ptr periodicity_check_287_387 ; y 1 x Cx Cy b
        cmp     bx,0
        jnz     end_periodicity_check_287
        cmp     cx,0
        jnz   end_periodicity_check_287                   ; if cx=0, pop stack
        jmp     pop_stack_287
end_periodicity_check_287:

        cmp     show_orbit,0            ; is show_orbit clear
        je      no_show_orbit_287       ; if so then skip
        call    near ptr show_orbit_xy  ; y 1 x Cx Cy b
no_show_orbit_287:
                                        ; y 1 x Cx Cy b
        fld     st(2)                   ; x y 1 x Cx Cy b
        fld     st(1)                   ; y x y 1 x Cx Cy b
        fmul    st(4),st                ; y x y 1 xy Cx Cy b
        fmulp   st(2),st                ; x y^2 1 xy Cx Cy b
        fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b
        fld     st                      ; x^2 x^2 y^2 1 xy Cx Cy b
        fadd    st,st(2)                ; x^2+y^2 x^2 y^2 1 xy Cx Cy b

        cmp     potflag,0               ; check for potential
        je      no_potflag_287
        fst     magnitude               ; if so, save magnitude
no_potflag_287:

        fcomp   st(7)                   ; x^2 y^2 1 xy Cx Cy b
        fstsw   ax
        sahf
        ja      over_bailout_287

;less than or equal to bailout
;       loop    top_of_cx_loop_287      ; x^2 y^2 1 xy Cx Cy b
        sub     bx,1
        sbb     cx,0
;        jnz     top_of_cx_loop_287
        jz      chk_bx
        jmp     top_of_cx_loop_287
chk_bx:
        cmp     bx,0
;        jnz     top_of_cx_loop_287
        jz      not_top
        jmp     top_of_cx_loop_287
not_top:

; reached maxit, inside
        mov     word ptr oldcoloriter,-1   ; check periodicity immediately next time
        mov     word ptr oldcoloriter+2,-1 ; check periodicity immediately next time
        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        sub     kbdcount,ax                 ; adjust the keyboard count
        mov     word ptr realcoloriter,ax   ; save unadjusted realcolor
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor
        mov     ax,word ptr inside_color
        mov     dx,word ptr inside_color+2

;        cmp     inside,-59              ; zmag ?
;        jne     no_zmag_287
;        fadd    st,st(1)                ; x^2+y^2 y^2 1 xy Cx Cy b
;        fimul   maxit                   ; maxit*|z^2| x^2 y^2 1 xy Cx Cy b

; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  The following
; line cause the positive value to be truncated.
;        fsub    round_down_half

;        fist    tmp_dword                ; tmp_word = |z^2|*maxit
;        fwait
;        mov     ax,word ptr tmp_dword
;        shr     dx,1                    ; |z^2|*maxit/2
;        rcr     ax,1
;        add     ax,1                    ; |z^2|*maxit/2+1
;        adc     dx,0

;no_zmag_287:

pop_stack_287:
        fninit

        mov     word ptr coloriter,ax
        mov     word ptr coloriter+2,dx

        cmp     orbit_ptr,0             ; any orbits to clear?
        je      calcmandfpasm_ret_287   ; nope.
        call    far ptr scrub_orbit     ; clear out any old orbits
        mov     ax,word ptr coloriter   ; restore color
        mov     dx,word ptr coloriter+2 ; restore color
                                        ; speed not critical here in orbit land

calcmandfpasm_ret_287:
        UNFRAME <si,di>                 ; pop stack frame
        fwait                           ; just to make sure
        ret

over_bailout_287:                       ; x^2 y^2 1 xy Cx Cy b
; outside
        mov     dx,cx
        mov     ax,bx
        sub     ax,10                   ; 10 more next time before checking
        sbb     dx,0
        jns     no_fix_underflow_287
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
        sub     ax,ax                   ; don't check next time
        mov     dx,ax
no_fix_underflow_287:
        mov     word ptr oldcoloriter,ax ; check when past this - 10 next time
        mov     word ptr oldcoloriter+2,dx ; check when past this - 10 next time
        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        sub     ax,bx                   ; leave 'times through loop' in ax
        sbb     dx,cx                   ; and dx

; zero color fix
        jnz     zero_color_fix_287
        cmp     ax,0
        jnz     zero_color_fix_287
        inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_287:
        mov     word ptr realcoloriter,ax   ; save unadjusted realcolor
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor
        sub     kbdcount,ax                 ; adjust the keyboard count

        cmp     outside,-1              ; iter ? (most common case)
        je      pop_stack_287
        cmp     outside,-2              ; outside <= -2 ?
        jle     special_outside_287     ; yes, go do special outside options
        mov     ax,outside              ; use outside color
        sub     dx,dx
        jmp     short pop_stack_287
special_outside_287:
        call    near ptr special_outside
        jmp     short pop_stack_287

calcmandfpasm_287  ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Since periodicity checking is used most of the time, I decided to
; separate the periodicity_check routines into a _287_387 version
; and an _87 version to achieve a slight increase in speed.  The
; epsilon_cross, show_orbit_xy, and special_outside routines are less
; frequently used and therefore have been implemented as single routines
; usable by the 8087 and up. -Wes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.286
.287
EVEN
periodicity_check_287_387   PROC    NEAR
; REMEMBER, the cx counter is counting BACKWARDS from maxit to 0
                                        ; fpu stack is either
                                        ; y x Cx Cy b (387)
                                        ; y 1 x Cx Cy b (287/emul)
        cmp     fpu,387
        jb      pc_load_x
        fld     st(1)                   ; if 387
        jmp     short pc_end_load_x
pc_load_x:
        fld     st(2)                   ; if 287/emul
pc_end_load_x:
                                        ; x y ...
        test    cx,word ptr savedand+2  ; save on 0, check on anything else
        jnz     do_check_287_387        ;  time to save a new "old" value
        test    bx,word ptr savedand    ; save on 0, check on anything else
        jnz     do_check_287_387        ;  time to save a new "old" value

; save last value                       ; fpu stack is
        fstp    savedx                  ; x y ...
        fst     savedy                  ; y ...
        dec     savedincr               ; time to lengthen the periodicity?
        jnz     per_check_287_387_ret   ; if not 0, then skip
        shl     word ptr savedand,1     ; savedand = (savedand << 1) + 1
        rcl     word ptr savedand+2,1   ; savedand = (savedand << 1) + 1
        add     word ptr savedand,1     ; for longer periodicity
        adc     word ptr savedand+2,0   ; for longer periodicity
        mov     savedincr,nextsavedincr      ; and restart counter
;        mov     ax,nextsavedincr      ; and restart counter
;        mov     savedincr,ax      ; and restart counter
        ret                             ; y ...

do_check_287_387:                       ; fpu stack is
                                        ; x y ...
        fsub    savedx                  ; x-savedx y ...
        fabs                            ; |x-savedx| y ...
        fcomp   closenuff               ; y ...
        fstsw   ax
        sahf
        ja      per_check_287_387_ret
        fld     st                      ; y y ...
        fsub    savedy                  ; y-savedy y ...
        fabs                            ; |y-savedy| y ...
        fcomp   closenuff               ; y ...
        fstsw   ax
        sahf
        ja      per_check_287_387_ret
                                        ; caught a cycle!!!
        mov     word ptr oldcoloriter,-1    ; check periodicity immediately next time
        mov     word ptr oldcoloriter+2,-1    ; check periodicity immediately next time

        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        mov     word ptr realcoloriter,ax ; save unadjusted realcolor as maxit
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor as maxit
        sub     dx,cx                   ; subtract half c
        sbb     ax,bx                   ; subtract half c
        sub     kbdcount,ax             ; adjust the keyboard count
        mov     ax,word ptr periodicity_color    ; set color
        mov     dx,word ptr periodicity_color+2    ; set color
        sub     cx,cx                   ; flag to exit cx loop immediately
        mov     bx,cx

per_check_287_387_ret:
        ret
periodicity_check_287_387   ENDP

.8086
.8087

EVEN
PUBLIC calcmandfpasm_87
calcmandfpasm_87  PROC
        FRAME   <di,si>                 ; std frame, for TC++ overlays
; initialization stuff
        sub     ax,ax                   ; clear ax
        mov     dx,ax                   ; dx=0
        cmp     periodicitycheck,ax     ; periodicity checking?
        je      initoldcolor            ;  no, set oldcolor 0 to disable it
        cmp     inside,-59              ; zmag?
        je      initoldcolor            ;  set oldcolor to 0
        cmp     reset_periodicity,ax    ; periodicity reset?
        je      short initparms         ;  no, inherit oldcolor from prior invocation
        mov     ax,word ptr maxit       ; yup.  reset oldcolor to maxit-250
        mov     dx,word ptr maxit+2
        sub     ax,250                  ; (avoids slowness at high maxits)
        sbb     dx,0                            ; (faster than conditional jump)
initoldcolor:
        mov     word ptr oldcoloriter,ax   ; reset oldcolor
        mov     word ptr oldcoloriter+2,dx ; reset oldcolor

initparms:
        sub     ax,ax                   ; clear ax
        mov     dx,ax                   ; clear dx
        mov     word ptr savedx,ax      ; savedx = 0.0
        mov     word ptr savedx+2,ax    ; needed since savedx is a QWORD
        mov     word ptr savedx+4,ax
        mov     word ptr savedx+6,ax
        mov     word ptr savedy,ax      ; savedy = 0.0
        mov     word ptr savedy+2,ax    ; needed since savedy is a QWORD
        mov     word ptr savedy+4,ax
        mov     word ptr savedy+6,ax
        mov     ax,word ptr firstsavedand+2 ; high part of savedand=0
        mov     word ptr savedand+2,ax ; high part of savedand=0
        mov     ax,word ptr firstsavedand    ; low part of savedand
        mov     word ptr savedand,ax    ; low part of savedand
        mov     savedincr,1             ; savedincr = 1
        mov     orbit_ptr,0             ; clear orbits
        dec     kbdcount                ; decrement the keyboard counter
        jns     short nokey        ;  skip keyboard test if still positive
        mov     kbdcount,10             ; stuff in a low kbd count
        cmp     show_orbit,0            ; are we showing orbits?
        jne     quickkbd                ;  yup.  leave it that way.
;this may need to be adjusted, I'm guessing at the "appropriate" values -Wes
        mov     kbdcount,1000           ; else, stuff an appropriate count val
                                        ; ("appropriate" to the FPU)
kbddiskadj:
        cmp     dotmode,11              ; disk video?
        jne     quickkbd                ;  no, leave as is
        shr     kbdcount,1              ; yes, reduce count
        shr     kbdcount,1              ; yes, reduce count

quickkbd:
        call    far ptr keypressed      ; has a key been pressed?
        cmp     ax,0                    ;  ...
        je      nokey                   ; nope.  proceed
        mov     kbdcount,0              ; make sure it goes negative again
        cmp     ax,'o'                  ; orbit toggle hit?
        je      orbitkey                ;  yup.  show orbits
        cmp     ax,'O'                  ; orbit toggle hit?
        jne     keyhit                  ;  nope.  normal key.
orbitkey:
        call    far ptr getakey         ; read the key for real
        mov     ax,1                    ; reset orbittoggle = 1 - orbittoggle
        sub     ax,show_orbit           ;  ...
        mov     show_orbit,ax           ;  ...
        jmp     short nokey             ; pretend no key was hit
keyhit:
        mov     ax,-1                   ; return with -1
        mov     dx,ax
        mov     word ptr coloriter,ax   ; set color to -1
        mov     word ptr coloriter+2,dx ; set color to -1
        UNFRAME <si,di>                 ; pop stack frame
        ret                             ; bail out!
nokey:

; OK, here's the heart of the floating point code.
; In my original program, the bailout value was loaded once per image and
; was left on the floating point stack after each pixel, and finally popped
; off the stack when the fractal was finished.  A lot of overhead for very
; little gain in my opinion, so I changed it so that it loads and unloads
; per pixel. -Wes

        fld     rqlim                   ; everything needs bailout first
        mov     cx,word ptr maxit+2     ; using cx and bx as loop counter
        mov     bx,word ptr maxit       ; using cx and bx as loop counter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _87 code is just like 287 code except that it must use
;       fstsw   tmp_word
;       fwait
;       mov     ax,tmp_word
; instead of
;       fstsw   ax
;
.8086
.8087
;start_87:
; let emulation fall through to the 87 code here
; as it seems not emulate correctly on an 8088/86 otherwise
        cmp     fractype,JULIAFP        ; julia or mandelbrot set?
        je      short dojulia_87        ; julia set - go there

; Mandelbrot _87 initialization of stack
        sub     bx,1                      ; always requires at least 1 iteration
        sbb     cx,0
                                        ; the fpu stack is shown below
                                        ; st(0) ... st(7)
                                        ; b (already on stack)
        fld     inity                   ; Cy b
        fld     initx                   ; Cx Cy b
        fld     st(1)                   ; Cy Cx Cy b
        fadd    parmy                   ; Py+Cy Cx Cy b
        fld1                            ; 1 Py+Cy Cx Cy b
        fld     st(1)                   ; Py+Cy 1 Py+Cy Cx Cy b
        fmul    st,st                   ; (Py+Cy)^2 1 Py+Cy Cx Cy b
        fld     st(3)                   ; Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
        fadd    parmx                   ; Px+Cx (Py+Cy)^2 1 Py+Cy Cx Cy b
        fmul    st(3),st                ; Px+Cx (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
        fmul    st,st                   ; (Px+Cx)^2 (Py+Cy)^2 1 (Py+Cy)(Px+Cx) Cx Cy b
        ; which is the next               x^2 y^2 1 xy Cx Cy b
        jmp     short top_of_cx_loop_87 ; branch around the julia switch

dojulia_87:
                                        ; Julia 87 initialization of stack
                                        ; note that init and parm are "reversed"
                                        ; b (already on stack)
        fld     parmy                   ; Cy b
        fld     parmx                   ; Cx Cy b
        fld     inity                   ; y Cx Cy b
        fld1                            ; 1 y Cx Cy b
        fld     st(1)                   ; y 1 y Cx Cy b
        fmul    st,st                   ; y^2 1 y Cx Cy b
        fld     initx                   ; x y^2 1 y Cx Cy b
        fmul    st(3),st                ; x y^2 1 xy Cx Cy b
        fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b

top_of_cx_loop_87:                      ; x^2 y^2 1 xy Cx Cy b
        fsubr                           ; x^2-y^2 1 xy Cx Cy b
        fadd    st,st(3)                ; x^2-y^2+Cx 1 xy Cx Cy b
        fxch    st(2)                   ; xy 1 x^2-y^2+Cx Cx Cy b
; FSCALE is faster than FADD for 87
        fscale                          ; 2xy 1 x^2-y^2+Cx Cx Cy b
        fadd    st,st(4)                ; 2xy+Cy 1 x^2-y^2+Cx Cx Cy b
                                        ; now same as the new
                                        ; y 1 x Cx Cy b

        cmp     outside,-2              ; real, imag, mult, or sum ?
        jg      no_save_new_xy_87       ; if not, then skip this
        fld     st(2)                   ; x y 1 x Cx Cy b
        fstp    newx                    ; y 1 x Cx Cy b
        fst     newy                    ; y 1 x Cx Cy b
no_save_new_xy_87:

;        cmp     inside,-100                     ; epsilon cross ?
;        jne     end_epsilon_cross_87
;        call    near ptr epsilon_cross          ; y 1 x Cx Cy b
;        cmp     bx,0
;        jnz     end_epsilon_cross_87
;        cmp     cx,0
;        jnz    end_epsilon_cross_87                   ; if cx=0, pop stack
;        jmp     pop_stack_6_87                  ; with a long jump
;end_epsilon_cross_87:

        test    bx,KEYPRESSDELAY        ; bx holds the low word of the loop count
        jne     notakey4                ; don't test yet
        push    cx
        push    bx
        call    far ptr keypressed      ; has a key been pressed?
        pop     bx
        pop     cx
        cmp     ax,0                    ;  ...
        je      notakey4                        ; nope.  proceed
        jmp     keyhit
notakey4:

        cmp     cx,word ptr oldcoloriter+2      ; if cx > oldcolor
        ja      no_periodicity_check_87         ; don't check periodicity
        cmp     bx,word ptr oldcoloriter        ; if bx >= oldcolor
        jae     no_periodicity_check_87         ; don't check periodicity
        call    near ptr periodicity_check_87   ; y 1 x Cx Cy b
        cmp     bx,0
        jnz     no_periodicity_check_87
        cmp     cx,0
        jnz   no_periodicity_check_87           ; if cx=0, pop stack
        jmp     pop_stack_6_87
no_periodicity_check_87:

        cmp     show_orbit,0            ; is show_orbit clear
        je      no_show_orbit_87        ; if so then skip
        call    near ptr show_orbit_xy  ; y 1 x Cx Cy b
no_show_orbit_87:

                                        ; y 1 x Cx Cy b
        fld     st(2)                   ; x y 1 x Cx Cy b
        fld     st(1)                   ; y x y 1 x Cx Cy b
        fmul    st(4),st                ; y x y 1 xy Cx Cy b
        fmulp   st(2),st                ; x y^2 1 xy Cx Cy b
        fmul    st,st                   ; x^2 y^2 1 xy Cx Cy b
        fld     st                      ; x^2 x^2 y^2 1 xy Cx Cy b
        fadd    st,st(2)                ; x^2+y^2 x^2 y^2 1 xy Cx Cy b

        cmp     potflag,0               ; check for potential
        je      no_potflag_87
        fst     magnitude               ; if so, save magnitude
no_potflag_87:

        fcomp   st(7)                   ; x^2 y^2 1 xy Cx Cy b
        fstsw   tmp_word
        fwait
        mov     ax,tmp_word
        sahf
        ja      over_bailout_87

;less than or equal to bailout
;       loop    top_of_cx_loop_87       ; x^2 y^2 1 xy Cx Cy b
        sub     bx,1
        sbb     cx,0
;        jnz     top_of_cx_loop_87
        jz      not_top1
        jmp     top_of_cx_loop_87
not_top1:
        cmp     bx,0
;        jnz     top_of_cx_loop_87
        jz      not_top2
        jmp     top_of_cx_loop_87
not_top2:

; reached maxit
        mov     word ptr oldcoloriter,-1   ; check periodicity immediately next time
        mov     word ptr oldcoloriter+2,-1 ; check periodicity immediately next time
        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        sub     kbdcount,ax                 ; adjust the keyboard count
        mov     word ptr realcoloriter,ax   ; save unadjusted realcolor
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor
        mov     ax,word ptr inside_color
        mov     dx,word ptr inside_color+2

;        cmp     inside,-59              ; zmag ?
;        jne     no_zmag_87
;        fadd    st,st(1)                ; x^2+y^2 y^2 1 xy Cx Cy b
;        fimul   maxit                   ; maxit*|z^2| x^2 y^2 1 xy Cx Cy b

; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  The following
; line cause the positive value to be truncated.
;        fsub    round_down_half

;        fist    tmp_dword                ; tmp_word = |z^2|*maxit
;        fwait
;        mov     ax,word ptr tmp_dword
;        mov     dx,word ptr tmp_dword+2
;        shr     dx,1                    ; |z^2|*maxit/2
;        rcr     ax,1
;        add     ax,1                    ; |z^2|*maxit/2+1
;        adc     dx,0

;no_zmag_87:

pop_stack_7_87:
; The idea here is just to clear the floating point stack.  There was a
; problem using FNINIT with the emulation library.  It didn't seem to
; properly clear the emulated stack, resulting in "stack overflow"
; messages.  Therefore, if emulation is being used, then FSTP's are used
; instead.

        cmp     fpu,0                   ; are we using emulation?
        jne     no_emulation            ; if not, then jump
        fstp    st
; you could just jump over this next check, but its faster to just check again
pop_stack_6_87:
        cmp     fpu,0                   ; are we using emulation?
        jne     no_emulation            ; if not, then jump
        fstp    st
        fstp    st
        fstp    st
        fstp    st
        fstp    st
        fstp    st
        jmp     short end_pop_stack_87
no_emulation:                           ; if no emulation, then
        fninit                          ;   use the faster FNINIT
end_pop_stack_87:

        mov     word ptr coloriter,ax
        mov     word ptr coloriter+2,dx

        cmp     orbit_ptr,0             ; any orbits to clear?
        je      calcmandfpasm_ret_87    ; nope.
        call    far ptr scrub_orbit     ; clear out any old orbits
        mov     ax,word ptr coloriter   ; restore color
        mov     dx,word ptr coloriter+2 ; restore color
                                        ; speed not critical here in orbit land

calcmandfpasm_ret_87:
        UNFRAME <si,di>                 ; pop stack frame
        fwait                           ; just to make sure
        ret

over_bailout_87:                        ; x^2 y^2 1 xy Cx Cy b
; outside
        mov     dx,cx
        mov     ax,bx
        sub     ax,10                   ; 10 more next time before checking
        sbb     dx,0
        jns     no_fix_underflow_87
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
        sub     ax,ax                   ; don't check next time
        mov     dx,ax
no_fix_underflow_87:
        mov     word ptr oldcoloriter,ax ; check when past this - 10 next time
        mov     word ptr oldcoloriter+2,dx ; check when past this - 10 next time
        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        sub     ax,bx                   ; leave 'times through loop' in ax
        sbb     dx,cx                   ; and dx

; zero color fix
        jnz     zero_color_fix_87
        cmp     ax,0
        jnz     zero_color_fix_87
        inc     ax                      ; if (ax == 0 ) ax = 1
zero_color_fix_87:
        mov     word ptr realcoloriter,ax   ; save unadjusted realcolor
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor
        sub     kbdcount,ax                 ; adjust the keyboard count

        cmp     outside,-1              ; iter ? (most common case)
        jne     dont_pop
;        je      pop_stack_7_87
        jmp     pop_stack_7_87
dont_pop:
        cmp     outside,-2              ; outside <= -2 ?
        jle     special_outside_87      ; yes, go do special outside options
        mov     ax,outside              ; use outside color
        sub     dx,dx
        jmp     pop_stack_7_87
special_outside_87:
        call    near ptr special_outside
        jmp     pop_stack_7_87

calcmandfpasm_87  ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
EVEN
periodicity_check_87    PROC    NEAR
; just like periodicity_check_287_387 except for the use of
;       fstsw tmp_word
; instead of
;       fstsw ax

; REMEMBER, the cx counter is counting BACKWARDS from maxit to 0
        fld     st(2)                   ; x y ...
        test    cx,word ptr savedand+2  ; save on 0, check on anything else
        jnz     do_check_87             ;  time to save a new "old" value
        test    bx,word ptr savedand    ; save on 0, check on anything else
        jnz     do_check_87             ;  time to save a new "old" value

; save last value                       ; fpu stack is
                                        ; x y ...
        fstp    savedx                  ; y ...
        fst     savedy                  ; y ...
        dec     savedincr               ; time to lengthen the periodicity?
        jnz     per_check_87_ret        ; if not 0, then skip
        shl     word ptr savedand,1     ; savedand = (savedand << 1) + 1
        rcl     word ptr savedand+2,1   ; savedand = (savedand << 1) + 1
        add     word ptr savedand,1     ; for longer periodicity
        adc     word ptr savedand+2,0   ; for longer periodicity
        mov     savedincr,nextsavedincr        ; and restart counter
;        mov     ax,nextsavedincr        ; and restart counter
;        mov     savedincr,ax             ; and restart counter
        ret                             ; y ...

do_check_87:                            ; fpu stack is
                                        ; x y ...
        fsub    savedx                  ; x-savedx y ...
        fabs                            ; |x-savedx| y ...
        fcomp   closenuff               ; y ...
        fstsw   tmp_word
        fwait
        mov     ax,tmp_word
        sahf
        ja      per_check_87_ret
        fld     st                      ; y y ...
        fsub    savedy                  ; y-savedy y ...
        fabs                            ; |y-savedy| y ...
        fcomp   closenuff               ; y ...
        fstsw   tmp_word
        fwait
        mov     ax,tmp_word
        sahf
        ja      per_check_87_ret
                                        ; caught a cycle!!!
        mov     word ptr oldcoloriter,-1    ; check periodicity immediately next time
        mov     word ptr oldcoloriter+2,-1  ; check periodicity immediately next time

        mov     ax,word ptr maxit
        mov     dx,word ptr maxit+2
        mov     word ptr realcoloriter,ax ; save unadjusted realcolor as maxit
        mov     word ptr realcoloriter+2,dx ; save unadjusted realcolor as maxit
        sub     dx,cx                   ; subtract half c
        sbb     ax,bx                   ; subtract half c
        sub     kbdcount,ax             ; adjust the keyboard count
        mov     ax,word ptr periodicity_color ; set color
        mov     dx,word ptr periodicity_color+2 ; set color
        sub     cx,cx                   ; flag to exit cx loop immediately
        mov     bx,cx

per_check_87_ret:
        ret
periodicity_check_87       ENDP

.8086
.8087
EVEN
show_orbit_xy   PROC NEAR USES bx cx dx si di
IFDEF @Version        ; MASM
IF @Version lt 600
        local   tmp_ten_byte_0:tbyte    ; stupid klooge for MASM 5.1 LOCAL bug
ENDIF
ENDIF
        local   tmp_ten_byte_1:tbyte
        local   tmp_ten_byte_2:tbyte
        local   tmp_ten_byte_3:tbyte
        local   tmp_ten_byte_4:tbyte
        local   tmp_ten_byte_5:tbyte
        local   tmp_ten_byte_6:tbyte
; USES is needed because in all likelyhood, plot_orbit surely
; uses these registers.  It's ok to have to push/pop's here in the
; orbits as speed is not crucial when showing orbits.

                                        ; fpu stack is either
                                        ; y x Cx Cy b (387)
                                        ; y 1 x Cx Cy b (287/87/emul)
        cmp     fpu,387
        jb      so_load_x
        fld     st(1)                   ; if 387
        jmp     short so_end_load_x
so_load_x:
        fld     st(2)                   ; if 287/87/emul
so_end_load_x:
                                        ; x y ...
                                        ; and needs to returned as
                                        ; y ...

        fstp    orbit_real              ; y ...
        fst     orbit_imag              ; y ...
        mov     ax,-1                   ; color for plot orbit
        push    ax                      ;       ...
; since the number fpu registers that plot_orbit() preserves is compiler
; dependant, it's best to fstp the entire stack into 10 byte memories
; and fld them back after plot_orbit() returns.
        fstp    tmp_ten_byte_1          ; store the stack in 80 bit form
        fstp    tmp_ten_byte_2
        fstp    tmp_ten_byte_3
        fstp    tmp_ten_byte_4
        fstp    tmp_ten_byte_5
        cmp     fpu,287                 ; with 287/87/emul the stack is 6 high
        jg      no_store_6              ; with 387 it is only 5 high
        fstp    tmp_ten_byte_6
no_store_6:
        fwait                           ; just to be safe
        push    word ptr orbit_imag+6   ; co-ordinates for plot orbit
        push    word ptr orbit_imag+4   ;       ...
        push    word ptr orbit_imag+2   ;       ...
        push    word ptr orbit_imag     ;       ...
        push    word ptr orbit_real+6   ; co-ordinates for plot orbit
        push    word ptr orbit_real+4   ;       ...
        push    word ptr orbit_real+2   ;       ...
        push    word ptr orbit_real     ;       ...
        call    far ptr plot_orbit      ; display the orbit
        add     sp,9*2                  ; clear out the parameters

        cmp     fpu,287
        jg      no_load_6
        fld     tmp_ten_byte_6          ; load them back in reverse order
no_load_6:
        fld     tmp_ten_byte_5
        fld     tmp_ten_byte_4
        fld     tmp_ten_byte_3
        fld     tmp_ten_byte_2
        fld     tmp_ten_byte_1
        fwait                           ; just to be safe
        ret
show_orbit_xy   ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.8086
.8087
EVEN
special_outside PROC NEAR
; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  Using
; "FSUB round_down_half" causes the values to be rounded down.
LOCAL Control:word
IFDEF @Version        ; MASM
IF @Version lt 600
        local   tmp_ten_byte_0:tbyte    ; stupid klooge for MASM 5.1 LOCAL bug
ENDIF
ENDIF
        local   tmp_ten_byte_1:tbyte
        fstcw Control
        push  Control                       ; Save control word on the stack
        or    Control, 0000110000000000b
        fldcw Control                       ; Set control to round towards zero

        cmp     outside,-2
        jne     not_real
        fld     newx
        test    bad_outside,1h
        jz      over_bad_real
        fsub    round_down_half
        jmp     over_good_real
over_bad_real:
        frndint
over_good_real:
        fistp   tmp_dword
        add     ax,7
        adc     dx,0
        fwait
        add     ax,word ptr tmp_dword
        adc     dx,word ptr tmp_dword+2
        jmp     check_color
not_real:
        cmp     outside,-3
        jne     not_imag
        fld     newy
        test    bad_outside,1h
        jz      short over_bad_imag
        fsub    round_down_half
        jmp     short over_good_imag
over_bad_imag:
        frndint
over_good_imag:
        fistp   tmp_dword
        add     ax,7
        adc     dx,0
        fwait
        add     ax,word ptr tmp_dword
        adc     dx,word ptr tmp_dword+2
        jmp     check_color
not_imag:
        cmp     outside,-4
        jne     not_mult
        fld     newy
        ftst                    ; check to see if newy == 0
        fstsw   tmp_word
        push    ax              ; save current ax value
        fwait
        mov     ax,tmp_word
        sahf
        pop     ax              ; retrieve ax (does not affect flags)
        jne     non_zero_y
        jmp     special_outside_ret
;        ret                     ; if y==0, return with normal ax
non_zero_y:
        fdivr   newx            ; newx/newy
        mov     word ptr tmp_dword,ax
        mov     word ptr tmp_dword+2,dx
        fimul   tmp_dword       ; (ax,dx)*newx/newy  (Use FIMUL instead of MUL
        test    bad_outside,1h
        jz      short over_bad_mult
        fsub    round_down_half ; to make it match the C code.)
        jmp     short over_good_mult
over_bad_mult:
        frndint
over_good_mult:
        fistp   tmp_dword
        fwait
        mov     ax,word ptr tmp_dword
        mov     dx,word ptr tmp_dword+2
        jmp     short check_color
not_mult:
        cmp     outside,-5
        jne     not_sum
        fld     newx
        fadd    newy            ; newx+newy
        test    bad_outside,1h
        jz     short over_bad_summ
        fsub    round_down_half
        jmp     short over_good_summ
over_bad_summ:
        frndint
over_good_summ:
        fistp   tmp_dword
        fwait
        add     ax,word ptr tmp_dword
        adc     dx,word ptr tmp_dword+2
        jmp     short check_color
not_sum:
        cmp     outside,-6      ; currently always equal, but put here
        jne     not_atan        ; for future outside types
; FPUatan needs 2 spots on the FPU stack, only 1 is available.
        fstp    tmp_ten_byte_1   ; store the top of stack in 80 bit form

        call    near ptr FPUatan ; return with atan on FPU stack
        fimul   atan_colors     ; atan_colors*Angle
        fldpi                   ; pi atan_colors*Angle
        fdiv                    ; atan_colors*Angle/pi
        fabs
        frndint
        fistp   tmp_dword

        fld     tmp_ten_byte_1  ; restore the top of stack

        fwait
        mov     ax,word ptr tmp_dword
        mov     dx,word ptr tmp_dword+2

not_atan:
check_color:
        cmp     dx,word ptr maxit+2     ; use UNSIGNED comparison
        jb      check_release           ; color < 0 || color > maxit
        cmp     ax,word ptr maxit       ; use UNSIGNED comparison
        jbe     check_release           ; color < 0 || color > maxit
        sub     ax,ax                   ; ax = 0
        mov     dx,ax                   ; dx = 0
check_release:
        cmp     save_release,1961
        jb      special_outside_ret
        cmp     dx,0
        jne     special_outside_ret
        cmp     ax,0
        jne     special_outside_ret
        mov     ax,1                    ; ax = 1
        mov     dx,0                    ; dx = 0
special_outside_ret:
        pop   Control
        fldcw Control              ; Restore control word
        ret
special_outside ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.8086   ;just to be sure
.8087
EVEN
FPUatan PROC NEAR USES ax dx
; This is derived from FPUcplxlog in fpu087.asm
; The arctan is returned on the FPU stack
LOCAL Status:word

        mov     ax, word ptr newy+6
        or      ax, word ptr newx+6
        jnz     NotBothZero

        fldz
        jmp     atandone

NotBothZero:
        fld     newy            ; newy
        fld     newx            ; newx newy

        mov     dh, BYTE PTR newx+7
        or      dh, dh
        jns     ChkYSign

        fchs                    ; |newx| newy

ChkYSign:
        mov     dl, BYTE PTR newy+7
        or      dl, dl
        jns     ChkMagnitudes

        fxch                    ; newy |newx|
        fchs                    ; |newy| |newx|
        fxch                    ; |newx| |newy|

ChkMagnitudes:
        fcom    st(1)           ; |newx| |newy|
        fstsw   Status
        fwait
        test    Status, 4500h
        jz      XisGTY

        test    Status, 4000h
        jz      short XneY

; newx = newy and atan = pi/4
        fstp    st              ; newy
        fstp    st              ; empty
        fldpi                   ; pi
        fdiv    _4_             ; pi/4

        or      dh, dh
        js      NegX_pi4

        or      dl, dl
        jns     short atandone  ; x+ y+

        fchs
        jmp     short atandone  ; x+ y-

NegX_pi4:
        fld     st(0)           ; pi/4 pi/4
        fadd    st,st           ; pi/2 pi/4
        fadd                    ; 3pi/4

        or      dl, dl
        jns     short atandone  ; x- y+

        fchs
        jmp     short atandone  ; x- y-

XneY: ; y > x
        fxch                    ; newy newx
        fpatan                  ; pi/2 - Angle
        fldpi                   ; pi, pi/2 - Angle
        fdiv    _2_             ; pi/2, pi/2 - Angle
        fsubr                   ; Angle

        or      dh, dh
        js      NegX

        or      dl, dl
        jns     short atandone  ; x+ y+

        fchs
        jmp     short atandone  ; x+ y-

XisGTY: ; x > y
        fpatan                  ; pi-Angle or Angle+pi

        or      dh, dh
        js      NegX

        or      dl, dl
        jns     short atandone  ; x+ y+

        fchs
        jmp     short atandone  ; x+ y-

NegX:
        fldpi                   ; pi, Angle
        fsubr                   ; pi - Angle

        or      dl, dl
        jns     short atandone  ; x- y+

        fchs
;        jmp     short atandone  ; x- y-

atandone:
        ret                     ; Leave result on FPU stack and return

FPUatan ENDP

END

