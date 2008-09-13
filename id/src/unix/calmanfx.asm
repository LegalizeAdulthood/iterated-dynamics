;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; calmanp5.asm - pentium floating point version of the calcmand.asm file
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; This code started from calmanfp.asm as a base.  This provided the code that
; takes care of the overhead that is needed to interface with the Fractint
; engine.  The initial pentium optimizations where provided by Daniele
; Paccaloni back in March of 1995.  For whatever reason, these optimizations
; didn't make it into version 19.6, which was released in May of 1997.  In
; July of 1997, Tim Wegner brought to my attention an article by Agner Fog
; Titled "Pentium Optimizations".  This article can currently be found at:

; http://www.azillionmonkeys.com/qed/p5opt.html

; It's a good article that claims to compare the Mandelbrot FPU code similar
; to what is in Fractint with his (and other's) pentium optimized code.  The
; only similarity I was able to find was they both calculate the Mandelbrot
; set.  Admittedly, the Fractint FPU Mandelbrot code was not optimized for a
; pentium.  So, taking the code segments provided by Agner Fog, Terje Mathisen,
; Thomas Jentzsch, and Damien Jones, I set out to optimize Fractint's FPU
; Mandelbrot code.  Unfortunately, it is not possible to just drop someone
; elses Mandelbrot code into Fractint.  I made good progress, but lost
; interest after several months.

; In April of 1998, Rees Acheson (author of MANDELB), contacted me about
; included his pentium optimized Mandelbrot code in the next release of
; Fractint.  This started a flurry of correspondence resulting in
; faster code in Fractint and faster code in MANDELB.  His code didn't
; drop right in, but his input and feedback are much appreciated.  The
; code in this file is largely due to his efforts.

; July 1998, Jonathan Osuch
;
; Updated 10 Oct 1998 by Chuck Ebbert (CAE) -- 5.17% speed gain on a P133
; Fixed keyboard/periodicity conflict JCO  10 DEC 1999
;

%include "xfract_a.inc"

; external functions
CEXTERN   keypressed    ;:FAR          ; this routine is in 'general.asm'
CEXTERN   getakey          ;:FAR             ; this routine is in 'general.asm'
CEXTERN   plot_orbit        ;:FAR          ; this routine is in 'fracsubr.c'
CEXTERN   scrub_orbit    ;:FAR         ; this routine is in 'fracsubr.c'

; external data
CEXTERN init      ;:QWORD                 ; declared as type complex
CEXTERN parm      ;:QWORD                 ; declared as type complex
CEXTERN new      ;:QWORD                  ; declared as type complex
CEXTERN maxit      ;:DWORD
CEXTERN inside      ;:DWORD
CEXTERN outside      ;:DWORD
CEXTERN rqlim      ;:QWORD               ; bailout (I never did figure out
                                ;   what "rqlim" stands for. -Wes)
CEXTERN coloriter      ;:DWORD
CEXTERN oldcoloriter      ;:DWORD
CEXTERN realcoloriter      ;:DWORD
CEXTERN periodicitycheck      ;:DWORD
CEXTERN reset_periodicity      ;:DWORD
CEXTERN closenuff      ;:QWORD
CEXTERN fractype      ;:DWORD             ; Mandelbrot or Julia
CEXTERN kbdcount      ;:DWORD            ; keyboard counter
CEXTERN dotmode      ;:DWORD
CEXTERN show_orbit      ;:DWORD           ; "show-orbit" flag
CEXTERN orbit_ptr      ;:DWORD            ; "orbit pointer" flag
CEXTERN magnitude      ;:QWORD           ; when using potential
CEXTERN nextsavedincr      ;:dword              ; for incrementing AND value
CEXTERN firstsavedand      ;:dword             ; AND value
CEXTERN bad_outside      ;:dword        ; old FPU code with bad      ;: real,imag,mult,summ
CEXTERN save_release      ;:dword
CEXTERN showdot      ;:DWORD
CEXTERN orbit_delay      ;:DWORD
CEXTERN atan_colors      ;:DWORD

JULIAFP  EQU 3                  ; from FRACTYPE.H
MANDELFP EQU 0
KEYPRESSDELAY equ 16383         ; 3FFFh

initx      EQU  init   ; just to make life easier
inity      EQU  init+DBLSZ
parmx   EQU  parm
parmy   EQU  parm+DBLSZ
newx     EQU new
newy     EQU new+8

section .data
;align 4
orbit_real              DQ  0.0
orbit_imag              DQ  0.0
round_down_half   DQ  0.5
tmp_dword               DD  0
inside_color            DD  0
periodicity_color       DD  7
%define savedincr  edi      ; space, but it doesn't hurt either
;;savedand_p5         DD 0    ;EQU     EDX

savedx_p5           DQ  0.0 ;:QWORD
savedy_p5           DQ  0.0 ;:QWORD
closenuff_p5        DQ  0.0 ;:QWORD

tmp_ten_byte_1   DT 0.0  ;:tbyte
tmp_ten_byte_2   DT 0.0  ;:tbyte
tmp_ten_byte_3   DT 0.0  ;:tbyte
tmp_ten_byte_4   DT 0.0  ;:tbyte
tmp_ten_byte_5   DT 0.0  ;:tbyte

Control  DW 0   ;:word

;calmanp5_text:
section .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This routine is called once per image.
; Put things here that won't change from one pixel to the next.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CGLOBAL calcmandfpasmstart_p5
calcmandfpasmstart_p5:

        sub     eax,eax
        mov     eax,[inside]
        cmp     eax,0                    ; if (inside color == maxiter)
        jnl     non_neg_inside
        mov     eax,[maxit]               ;   use maxit as inside_color

non_neg_inside:                         ; else
        mov     [inside_color],eax        ;   use inside as inside_color
        cmp     dword [periodicitycheck],0      ; if periodicitycheck < 0
        jnl     non_neg_periodicitycheck
        mov     eax,7                   ;   use color 7 (default white)
non_neg_periodicitycheck:               ; else
        mov     [periodicity_color],eax   ; use inside_color still in ax
        mov     dword [oldcoloriter],0          ; no periodicity checking on 1st pixel
        ret
;;_calcmandfpasmstart_p5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pentium floating point version of calcmandasm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CGLOBAL calcmandfpasm_p5
ALIGN 16
calcmandfpasm_p5:

; Register usage:  eax: ??????    ebx:oldcoloriter
;                  ecx:counter    edx:savedand_p5
;                  di:????   esi:0ffffh

     FRAME esi, edi

;    {Set up FPU stack for quick access while in the loop}
; initialization stuff
        sub     eax,eax                 ; clear eax
;        cmp     periodicitycheck,ax     ; periodicity checking?
        cmp     dword [periodicitycheck],0     ; periodicity checking?
        je      initoldcolor            ;  no, set oldcolor 0 to disable it
;        cmp     reset_periodicity,ax    ; periodicity reset?
        cmp     dword [reset_periodicity],0    ; periodicity reset?
        je      initparms               ;  no, inherit oldcolor from prior invocation
        mov     eax,[maxit]               ; yup.  reset oldcolor to maxit-250
        sub     eax,250                 ; (avoids slowness at high maxits)

initoldcolor:
        mov     [oldcoloriter],eax   ; reset oldcolor

initparms:
;        fld     qword [closenuff]
;        fstp    qword [closenuff_p5]
        fldz
;        sub     eax,eax                   ; clear ax for below
        mov     dword [orbit_ptr],0             ; clear orbits
        mov     edx,1           ; edx = savedand_p5 = 1
        fst     qword [savedx_p5]      ; savedx = 0.0
        fstp    qword [savedy_p5]      ; savedy = 0.0
;        mov     savedincr,dx
        mov     edi,edx          ; savedincr is in edi = 1
;        mov     savedincr,1             ; savedincr = 1
;        mov     edx,firstsavedand

        mov     esi,0FFFFh
        dec     dword [kbdcount]                ; decrement the keyboard counter
        jns      near nokey        ;  skip keyboard test if still positive
        mov     dword [kbdcount],10             ; stuff in a low kbd count
        cmp     dword [show_orbit],0            ; are we showing orbits?
        jne     quickkbd                ;  yup.  leave it that way.
        cmp     dword [orbit_delay],0           ; are we delaying orbits?
        je      slowkbd                 ;  nope.  change it.
        cmp     dword [showdot],0               ; are we showing the current pixel?
        jge     quickkbd                ;  yup.  leave it that way.
;this may need to be adjusted, I'm guessing at the "appropriate" values -Wes
slowkbd:
        mov     dword [kbdcount],5000           ; else, stuff an appropriate count val
        cmp     dword [dotmode],11              ; disk video?
        jne     quickkbd                ;  no, leave as is
        shr     dword [kbdcount],2              ; yes, reduce count

quickkbd:
        call    keypressed      ; has a key been pressed?
        cmp     ax,0                    ;  ...
        je      nokey                   ; nope.  proceed
        mov     dword [kbdcount],0              ; make sure it goes negative again
        cmp     ax,'o'                  ; orbit toggle hit?
        je      orbitkey                ;  yup.  show orbits
        cmp     ax,'O'                  ; orbit toggle hit?
        jne     keyhit                  ;  nope.  normal key.
orbitkey:
        call    getakey         ; read the key for real
        mov     eax,1                   ; reset orbittoggle = 1 - orbittoggle
        sub     eax,[show_orbit]           ;  ...
        mov     [show_orbit],eax           ;  ...
        jmp     short nokey             ; pretend no key was hit
keyhit:
        fninit
        mov     eax,-1                   ; return with -1
        mov     [coloriter],eax            ; set color to -1
        mov     edx,eax                  ; put results in ax,dx
;        shr     edx,16                  ; all 1's anyway, don't bother w/shift
        ret                             ; bail out!
nokey:

        mov     ecx,[maxit]               ; initialize counter
        mov     ebx,[oldcoloriter]

        cmp     dword [fractype],JULIAFP        ; julia or mandelbrot set?
        je      near dojulia_p5              ; julia set - go there

; Mandelbrot _p5 initialization of stack
        dec     ecx                     ;  always do one already
                                        ; the fpu stack is shown below
                                        ; st(0) ... st(7)

        fld     qword [initx]                   ; Cx
        fld     qword [inity]                   ; Cy Cx
        fld     qword [rqlim]                   ; b Cy Cx
        fld     st2                   ; Cx b Cy Cx
        fadd    qword [parmx]                   ; Px+Cx b Cy Cx
        fld     st0                   ; Px+Cx Px+Cx b Cy Cx
        fmul    st0,st0                   ; (Px+Cx)^2 Px+Cx b Cy Cx
        fld     st3                   ; Cy (Px+Cx)^2 Px+Cx b Cy Cx
        fadd    qword [parmy]                   ; Py+Cy (Px+Cx)^2 Px+Cx b Cy Cx
        jmp     bottom_of_dojulia_p5

align 16
DoKeyCheck:
        push    eax
        push    ecx
        push    ebx
        call    keypressed      ; has a key been pressed?
        pop     ebx
        pop     ecx
;        cmp     ax,0                    ;  ...
        or      eax,eax
        je      SkipKeyCheck            ; nope.  proceed
        pop     eax
        jmp     keyhit

ALIGN 16
save_new_old_value:
        fld     st2                   ; y y^2 x^2 y x b Cy Cx
        fstp    qword [savedy_p5]               ; y^2 x^2 y x b Cy Cx
        fld     st3                   ; x y^2 x^2 y x b Cy Cx
        fstp    qword [savedx_p5]               ; y^2 x^2 y x b Cy Cx
        dec     savedincr               ; time to lengthen the periodicity?
        jnz     near JustAfterFnstsw    ; if not 0, then skip
;        add     edx,edx            ; savedand = (savedand * 2) + 1
;        inc     edx                ; for longer periodicity
        lea     edx,[edx*2+1]
        mov     savedincr,[nextsavedincr]       ; and restart counter

;        test    cx,KEYPRESSDELAY       ; ecx holds the loop count
;        test    cx,0FFFFh
;        test    ecx,esi                ; put 0FFFFh into esi above
        test    cx,si
        jz      DoKeyCheck
        jmp     JustAfterFnstsw

SkipKeyCheck:
        pop     eax
        jmp     JustAfterFnstsw

ALIGN 16
do_check_p5_fast:
;        call    near ptr periodicity_check_p5  ; y x b Cy Cx
; REMEMBER, the cx counter is counting BACKWARDS from maxit to 0
                                        ; fpu stack is
                                        ; y2 x2 y x b Cy Cx
        fld     qword [savedx_p5]               ; savedx y2 x2 y x ...
        fsub    st0,st4             ; x-savedx y2 x2 y x ...
        fabs                            ; |x-savedx| y2 x2 y x ...
        fcomp   qword [closenuff]            ; y2 x2 y x ...
        push    ax                      ; push AX for later
        fnstsw  ax
        and     ah,41h                  ; if |x-savedx| > closenuff
        jz      near per_check_p5_ret_fast   ; we're done
        fld     qword [savedy_p5]               ; savedy y2 x2 y x ...
        fsub    st0,st3             ; y-savedy y2 x2 y x ...
        fabs                            ; |y-savedy| y2 x2 y x ...
        fcomp   qword [closenuff]            ; y2 x2 y x ...
        fnstsw  ax
        and     ah,41h                  ; if |y-savedy| > closenuff
        jz      near per_check_p5_ret_fast   ; we're done
                                       ; caught a cycle!!!
        pop     ax                     ; undo push
        fcompp                         ; pop off y2 and x2, leaving y x ...
        mov     eax,[maxit]
        mov     dword [oldcoloriter],-1        ; check periodicity immediately next time
        mov     [realcoloriter],eax      ; save unadjusted realcolor as maxit
        mov     eax,[periodicity_color]  ; set color
        jmp     overiteration_p5


dojulia_p5:
                                        ; Julia p5 initialization of stack
                                        ; note that init and parm are "reversed"
        fld     qword [parmx]                   ; Cx
        fld     qword [parmy]                   ; Cy Cx
        fld     qword [rqlim]                   ; b Cy Cx

        fld     qword [initx]                   ; x b Cy Cx
        fld     st0                      ; x x b Cy Cx
        fmul    st0,st0                   ; x^2 x b Cy Cx
        fld     qword [inity]                   ; y x^2 x b Cy Cx

bottom_of_dojulia_p5:
        fmul    st2,st0                ; y x^2 xy b Cy Cx
        fmul    st0,st0                   ; y^2 x^2 xy b Cy Cx

        fsubp  st1,st0                        ; x^2-y^2 xy b Cy Cx
        fadd    st0,st4                ; x^2-y^2+Cx xy b Cy Cx
        fxch                            ; xy x^2-y^2+Cx b Cy Cx

        fadd    st0,st0                   ; 2xy x^2-y^2+Cx b Cy Cx
        fadd    st0,st3                ; 2xy+Cy x^2-y^2+Cx b Cy Cx

; first iteration complete
; {FPU stack all set, we're ready for the start of the loop}
align 16
LoopStart:

; {While (Sqr(x) + Sqr(y) < b) and (Count < MaxIterations) do}
;    {square both numbers}
        fld     st1                   ;  {x, y, x, b, Cy, Cx}
        fmul    st0,st0             ;  {x^2, y, x, b, Cy, Cx}
        fld     st1                   ;  {y, x^2, y, x, b, Cy, Cx}
        fmul    st0,st0             ;  {y^2, x^2, y, x, b, Cy, Cx}

;    {add both squares and leave at top of stack ready for the compare}
        fld     st1                   ;  {x^2, y^2, x^2, y, x, b, Cy, Cx}
        fadd    st0,st1             ;  {(y^2)+(x^2), y^2, x^2, y, x, b, Cy, Cx}
;    {Check to see if (x^2)+(y^2) < b and discard (x^2)+(y^2)}
        fcomp   st5                   ;  {y^2, x^2, y, x, b, Cy, Cx}

        cmp     ecx,ebx    ; put oldcoloriter in ebx above
        jae     SkipTasks  ; don't check periodicity

        fnstsw  ax         ;Get the pending NPX info into AX

        test    ecx,edx         ; save on 0, check on anything else
        jnz     near do_check_p5_fast        ;  time to save a new "old" value
        jmp     save_new_old_value
;        jz      save_new_old_value
;        jmp     do_check_p5_fast        ;  time to save a new "old" value
align 16
per_check_p5_ret_fast:

        pop     ax              ;pop AX to continue with the FCOMP test
        jz      short JustAfterFnstsw ;test that got us here, & pairable
        jmp     short JustAfterFnstsw ;since we have done the FNSTSW,
                                ; Skip over next instruction
align 4
SkipTasks:

        fnstsw ax             ;  {Store the NPX status word in AX, no FWAIT}

JustAfterFnstsw:

;  {FPU stack again has all the required elements for terminating the loop }
;  {Continue with the FCOMP test.}
;  {The following does the same as SAHF; JA @LoopEnd; but in 3 fewer cycles}
        shr     ah,1            ; {Shift right, shifts low bit into carry flag }
        jnc     short overbailout_p5  ; {Jmp if not carry.  Do while waiting for FPU }

;  {Temp = Sqr(x) - Sqr(y) + Cx}  {Temp = Newx}
;    {Subtract y^2 from Cx ...}
        fsubr   st0,st6             ;  {Cx-y^2, x^2, y, x, b, Cy, Cx}

;  CAE changed this around for Pentium, 10 Oct 1998
;  exchange this pending result with y so there's no wait for fsubr to finish
        fxch    st2                   ; {y, x^2, Cx-y^2, x, b, Cy, Cx}

;  now compute x*y while the above fsubr is still running
        fmulp   st3,st0                ; {x^2, Cx-y^2, xy, b, Cy, Cx}

;    {... then add x^2 to Cx-y^2}
        faddp   st1,st0             ; {Newx, xy, b, Cy, Cx}

;    {Place the temp (Newx) in the x slot ready for next time in the
;     loop, while placing xy in ST(0) to use below.}
        fxch    st1                   ;  {xy, Newx, b, Cy, Cx}

; {y = (y * x * 2) + Cy   (Use old x, not temp)}
;    {multiply y * x was already done above so it was removed here -- CAE}


;    {Now multiply x*y by 2 (add ST to ST)}
        fadd    st0,st0                ;  {x*y*2, Newx, b, Cy, Cx}

;  compare was moved down so it would run concurrently with above add -- CAE
        cmp     dword [show_orbit],0            ; is show_orbit clear

;    {Finally, add Cy to x*y*2}
        fadd    st0,st3              ;  {Newy, Newx, b, Cy, Cx}

        jz      no_show_orbit_p5         ; if so then skip
        call    show_orbit_xy_p5  ; y x b Cy Cx
align 4
no_show_orbit_p5:

        dec     ecx
        jnz     LoopStart
;        jmp     LoopStart    ;  {FPU stack has required elements for next loop}
align 4
LoopEnd:                                ;  {Newy, Newx, b, Cy, Cx}

; reached maxit, inside
        mov     eax,[maxit]
        mov     dword [oldcoloriter],-1        ; check periodicity immediately next time
        mov     [realcoloriter],eax      ; save unadjusted realcolor
        mov     eax,[inside_color]
        jmp     short overiteration_p5
align 4
overbailout_p5:

        faddp   st1,st0                       ; x^2+y^2 y x b Cy Cx
        mov     eax,ecx
        fstp    qword [magnitude]               ; y x b Cy Cx
        sub     eax,10                  ; 10 more next time before checking

        jns     no_fix_underflow_p5
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
        sub     eax,eax                 ; don't check next time
no_fix_underflow_p5:
        mov     [oldcoloriter],eax        ; check when past this - 10 next time
        mov     eax,[maxit]
        sub     eax,ecx                 ; leave 'times through loop' in eax

; zero color fix
        jnz     zero_color_fix_p5
        inc     eax                     ; if (eax == 0 ) eax = 1
zero_color_fix_p5:
        mov     [realcoloriter],eax       ; save unadjusted realcolor
        sub     [kbdcount],eax             ; adjust the keyboard count

        cmp     dword [outside],-1              ; iter ? (most common case)
        je      overiteration_p5
        cmp     dword [outside],-2              ; outside <= -2 ?
        jle     to_special_outside_p5   ; yes, go do special outside options
;        sub     eax,eax                 ; clear top half of eax for next
        mov     eax,[outside]              ; use outside color
        jmp     short overiteration_p5

to_special_outside_p5:

        call    special_outside_p5
align 4
overiteration_p5:

        fstp    qword [newy]                    ; x b Cy Cx
        fstp    qword [newx]                    ; b Cy Cx


;    {Pop 3 used registers from FPU stack, discarding the values.
;       All we care about is ECX, the count.}

        fcompp
        fstp    st0
        mov     [coloriter],eax

        cmp     dword [orbit_ptr],0             ; any orbits to clear?
        je      calcmandfpasm_ret_p5    ; nope.
        call    scrub_orbit     ; clear out any old orbits
        mov     eax,[coloriter]           ; restore color
                                        ; speed not critical here in orbit land

calcmandfpasm_ret_p5:

;        mov     edx,eax       ;     {The low 16 bits already in AX}
;        shr     edx,16        ;     {Shift high 16 bits to low 16 bits position}

      UNFRAME esi, edi

        ret

;;; _calcmandfpasm_p5

ALIGN 16
show_orbit_xy_p5:             ;PROC NEAR USES ebx ecx edx esi edi
; USES is needed because in all likelyhood, plot_orbit surely
; uses these registers.  It's ok to have to push/pop's here in the
; orbits as speed is not crucial when showing orbits.

    FRAME ebx, ecx, edx, esi, edi
                                        ; fpu stack is either
                                        ; y x b Cx Cy (p5)
        fld     st1                   ;
                                        ; x y ...
                                        ; and needs to returned as
                                        ; y ...

        fstp    qword [orbit_real]              ; y ...
        fst      qword [orbit_imag]              ; y ...
        mov     eax,-1                   ; color for plot orbit
        push    eax                      ;       ...
; since the number fpu registers that plot_orbit() preserves is compiler
; dependant, it's best to fstp the entire stack into 10 byte memories
; and fld them back after plot_orbit() returns.
        fstp    tword [tmp_ten_byte_1]          ; store the stack in 80 bit form
        fstp    tword [tmp_ten_byte_2]
        fstp    tword [tmp_ten_byte_3]
        fstp    tword [tmp_ten_byte_4]
        fstp    tword [tmp_ten_byte_5]
;        fwait                           ; just to be safe
;        push    word ptr orbit_imag+6   ; co-ordinates for plot orbit
        push    dword [orbit_imag+4]   ;       ...
;        push    word ptr orbit_imag+2   ;       ...
        push    dword [orbit_imag]     ;       ...
;        push    word ptr orbit_real+6   ; co-ordinates for plot orbit
        push    dword [orbit_real+4]   ;       ...
;        push    word ptr orbit_real+2   ;       ...
        push    dword [orbit_real]     ;       ...
        call    plot_orbit      ; display the orbit
        add     sp,5*4                  ; clear out the parameters

        fld     tword [tmp_ten_byte_5]
        fld     tword [tmp_ten_byte_4]
        fld     tword [tmp_ten_byte_3]
        fld     tword [tmp_ten_byte_2]
        fld     tword [tmp_ten_byte_1]
;        fwait                           ; just to be safe
    UNFRAME ebx, ecx, edx, esi, edi
        ret
;;;   show_orbit_xy_p5   ENDP

ALIGN 16
special_outside_p5:       ; PROC NEAR
; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  Using
; "FSUB round_down_half" causes the values to be rounded down.
; Boo-Hiss if values are negative, change FPU control word to truncate values.
        fstcw [Control]
        push  word [Control]                       ; Save control word on the stack
        or    word [Control], 0000110000000000b
        fldcw [Control]                       ; Set control to round towards zero

        cmp     dword [outside],-2
        jne     short not_real

        fld     st1                  ; newx
        test    dword [bad_outside],1h
        jz      over_bad_real
        fsub    qword [round_down_half]
        jmp     over_good_real
over_bad_real:
        frndint
over_good_real:
        fistp   dword [tmp_dword]
        add     eax,7
        add     eax,[tmp_dword]
        jmp     check_color
not_real:
        cmp     dword [outside],-3
        jne     short not_imag
        fld     st0            ; newy
        test    dword [bad_outside],1h
        jz      short over_bad_imag
        fsub    qword [round_down_half]
        jmp     short over_good_imag
over_bad_imag:
        frndint
over_good_imag:
        fistp   dword [tmp_dword]
        add     eax,7
        add     eax,[tmp_dword]
        jmp     check_color
not_imag:
        cmp     dword [outside],-4
        jne     short not_mult
        push    ax              ; save current ax value
        fld     st0           ; newy
        ftst                    ; check to see if newy == 0
        fstsw   ax
        sahf
        pop     ax              ; retrieve ax (does not affect flags)
        jne     short non_zero_y
        fcomp   st0           ; pop it off the stack
;        ret                     ; if y==0, return with normal ax
        jmp     special_outside_ret
non_zero_y:
        fdivr   st0,st2         ; newx/newy
        mov     [tmp_dword],eax
        fimul   dword [tmp_dword]       ; (ax,dx)*newx/newy  (Use FIMUL instead of MUL
        test    dword [bad_outside],1h
        jz      short over_bad_mult
        fsub    qword [round_down_half] ; to make it match the C code.)
        jmp     short over_good_mult
over_bad_mult:
        frndint
over_good_mult:
        fistp   dword [tmp_dword]
;        fwait
        mov     eax,[tmp_dword]
        jmp     short check_color
not_mult:
        cmp     dword [outside],-5
        jne     short not_sum
        fld     st1           ; newx
        fadd    st0,st1     ; newx+newy
        test    dword [bad_outside],1h
        jz     short over_bad_summ
        fsub    qword [round_down_half]
        jmp     short over_good_summ
over_bad_summ:
        frndint
over_good_summ:
        fistp   dword [tmp_dword]
;        fwait
        add     eax,[tmp_dword]
        jmp     short check_color
not_sum:
        cmp     dword [outside],-6      ; currently always equal, but put here
        jne     short not_atan        ; for future outside types
        fld     st0           ; newy
        fld     st2           ; newx newy
        fpatan                  ; arctan(y/x)
        fimul   dword [atan_colors] ; atan_colors*atan
        fldpi                    ; pi atan_colors*atan
        fdivp   st1,st0             ; atan_colors*atan/pi
        fabs
        frndint
        fistp   dword [tmp_dword]
;        fwait
        mov     eax,[tmp_dword]

not_atan:
check_color:
        cmp     eax,[maxit]               ; use UNSIGNED comparison
        jbe     short check_release     ; color < 0 || color > maxit
        sub     eax,eax                 ; eax = 0
check_release:
        cmp     dword [save_release],1961
        jb      short special_outside_ret
        cmp     eax,0
        jne     special_outside_ret
        mov     eax,1                   ; eax = 1
special_outside_ret:
        pop   word [Control]
        fldcw [Control]              ; Restore control word
        ret
;;;   special_outside_p5 ENDP

;;;calmanp5_text  ENDS

;END

