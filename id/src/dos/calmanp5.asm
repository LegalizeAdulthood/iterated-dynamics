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



;                        required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF

.MODEL medium,c

.486

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
EXTRN magnitude:QWORD           ; when using potential
extrn   nextsavedincr:word              ; for incrementing AND value
extrn   firstsavedand:dword             ; AND value
extrn   bad_outside:word        ; old FPU code with bad: real,imag,mult,summ
extrn   save_release:word
extrn   showdot:WORD
extrn   orbit_delay:WORD
extrn   atan_colors:word

JULIAFP  EQU 6                  ; from FRACTYPE.H
MANDELFP EQU 4
KEYPRESSDELAY equ 16383         ; 3FFFh

initx    EQU <qword ptr init>   ; just to make life easier
inity    EQU <qword ptr init+8>
parmx    EQU <qword ptr parm>
parmy    EQU <qword ptr parm+8>
newx     EQU <qword ptr new>
newy     EQU <qword ptr new+8>

.DATA
EVEN
orbit_real              DQ  ?
orbit_imag              DQ  ?
round_down_half         DD  0.5
tmp_dword               DD  ?
inside_color            DD  ?
periodicity_color       DD  7
savedincr               EQU     DI      ; space, but it doesn't hurt either
savedand_p5             EQU     EDX

calmanp5_text SEGMENT PARA PUBLIC USE16 'CODE'
ASSUME cs:calmanp5_text
ALIGN 16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This routine is called once per image.
; Put things here that won't change from one pixel to the next.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC calcmandfpasmstart_p5
calcmandfpasmstart_p5   PROC

        sub     eax,eax
        mov     ax,inside
        cmp     ax,0                    ; if (inside color == maxiter)
        jnl     non_neg_inside
        mov     eax,maxit               ;   use maxit as inside_color

non_neg_inside:                         ; else
        mov     inside_color,eax        ;   use inside as inside_color

        cmp     periodicitycheck,0      ; if periodicitycheck < 0
        jnl     non_neg_periodicitycheck
        mov     eax,7                   ;   use color 7 (default white)
non_neg_periodicitycheck:               ; else
        mov     periodicity_color,eax   ; use inside_color still in ax
        mov     oldcoloriter,0          ; no periodicity checking on 1st pixel
        ret
calcmandfpasmstart_p5       ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pentium floating point version of calcmandasm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC calcmandfpasm_p5
calcmandfpasm_p5  PROC
ALIGN 16
LOCAL  savedx_p5:QWORD, savedy_p5:QWORD
LOCAL  closenuff_p5:QWORD

; Register usage:  eax: ??????    ebx:oldcoloriter
;                  ecx:counter    edx:savedand_p5
;                  di:savedincr   esi:0ffffh
ALIGN 16
;    {Set up FPU stack for quick access while in the loop}
; initialization stuff
        sub     eax,eax                 ; clear eax
;        cmp     periodicitycheck,ax     ; periodicity checking?
        cmp     periodicitycheck,0     ; periodicity checking?
        je      initoldcolor            ;  no, set oldcolor 0 to disable it
;        cmp     reset_periodicity,ax    ; periodicity reset?
        cmp     reset_periodicity,0    ; periodicity reset?
        je      initparms               ;  no, inherit oldcolor from prior invocation
        mov     eax,maxit               ; yup.  reset oldcolor to maxit-250
        sub     eax,250                 ; (avoids slowness at high maxits)

initoldcolor:
        mov     oldcoloriter,eax   ; reset oldcolor

initparms:
        fld     closenuff
        fstp    closenuff_p5
        fldz
;        sub     eax,eax                   ; clear ax for below
        mov     orbit_ptr,0             ; clear orbits
        mov     savedand_p5,1           ; edx = savedand_p5 = 1
        fst     savedx_p5      ; savedx = 0.0
        fstp    savedy_p5      ; savedy = 0.0
;        mov     savedincr,dx
        mov     edi,savedand_p5          ; savedincr is in edi = 1
;        mov     savedincr,1             ; savedincr = 1
;        mov     edx,firstsavedand

        mov     esi,0FFFFh
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
        mov     kbdcount,5000           ; else, stuff an appropriate count val
        cmp     dotmode,11              ; disk video?
        jne     quickkbd                ;  no, leave as is
        shr     kbdcount,2              ; yes, reduce count

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
        mov     eax,1                   ; reset orbittoggle = 1 - orbittoggle
        sub     ax,show_orbit           ;  ...
        mov     show_orbit,ax           ;  ...
        jmp     short nokey             ; pretend no key was hit
keyhit:
        fninit
        mov     eax,-1                   ; return with -1
        mov     coloriter,eax            ; set color to -1
        mov     edx,eax                  ; put results in ax,dx
;        shr     edx,16                  ; all 1's anyway, don't bother w/shift
        ret                             ; bail out!
nokey:

        mov     ecx,maxit               ; initialize counter
        mov     ebx,oldcoloriter

        cmp     fractype,JULIAFP        ; julia or mandelbrot set?
        je      dojulia_p5              ; julia set - go there

; Mandelbrot _p5 initialization of stack
        dec     ecx                     ;  always do one already
                                        ; the fpu stack is shown below
                                        ; st(0) ... st(7)

        fld     initx                   ; Cx
        fld     inity                   ; Cy Cx
        fld     rqlim                   ; b Cy Cx
        fld     st(2)                   ; Cx b Cy Cx
        fadd    parmx                   ; Px+Cx b Cy Cx
        fld     st(0)                   ; Px+Cx Px+Cx b Cy Cx
        fmul    st,st                   ; (Px+Cx)^2 Px+Cx b Cy Cx
        fld     st(3)                   ; Cy (Px+Cx)^2 Px+Cx b Cy Cx
        fadd    parmy                   ; Py+Cy (Px+Cx)^2 Px+Cx b Cy Cx
        jmp     bottom_of_dojulia_p5

EVEN
DoKeyCheck:
        push    eax
        push    ecx
        push    ebx
        call    far ptr keypressed      ; has a key been pressed?
        pop     ebx
        pop     ecx
;        cmp     ax,0                    ;  ...
        or      ax,ax
        je      SkipKeyCheck            ; nope.  proceed
        pop     eax
        jmp     keyhit

ALIGN 16
save_new_old_value:
        fld     st(2)                   ; y y^2 x^2 y x b Cy Cx
        fstp    savedy_p5               ; y^2 x^2 y x b Cy Cx
        fld     st(3)                   ; x y^2 x^2 y x b Cy Cx
        fstp    savedx_p5               ; y^2 x^2 y x b Cy Cx
        dec     savedincr               ; time to lengthen the periodicity?
        jnz     JustAfterFnstsw    ; if not 0, then skip
;        add     edx,edx            ; savedand = (savedand * 2) + 1
;        inc     edx                ; for longer periodicity
        lea     savedand_p5,[savedand_p5*2+1]
        mov     savedincr,nextsavedincr       ; and restart counter

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
        fld     savedx_p5               ; savedx y2 x2 y x ...
        fsub    st(0),st(4)             ; x-savedx y2 x2 y x ...
        fabs                            ; |x-savedx| y2 x2 y x ...
        fcomp   closenuff_p5            ; y2 x2 y x ...
        push    ax                      ; push AX for later
        fnstsw  ax
        and     ah,41h                  ; if |x-savedx| > closenuff
        jz      per_check_p5_ret_fast   ; we're done
        fld     savedy_p5               ; savedy y2 x2 y x ...
        fsub    st(0),st(3)             ; y-savedy y2 x2 y x ...
        fabs                            ; |y-savedy| y2 x2 y x ...
        fcomp   closenuff_p5            ; y2 x2 y x ...
        fnstsw  ax
        and     ah,41h                  ; if |y-savedy| > closenuff
        jz      per_check_p5_ret_fast   ; we're done
                                       ; caught a cycle!!!
        pop     ax                     ; undo push
        fcompp                         ; pop off y2 and x2, leaving y x ...
        mov     eax,maxit
        mov     oldcoloriter,-1        ; check periodicity immediately next time
        mov     realcoloriter,eax      ; save unadjusted realcolor as maxit
        mov     eax,periodicity_color  ; set color
        jmp     overiteration_p5


dojulia_p5:

                                        ; Julia p5 initialization of stack
                                        ; note that init and parm are "reversed"
        fld     parmx                   ; Cx
        fld     parmy                   ; Cy Cx
        fld     rqlim                   ; b Cy Cx

        fld     initx                   ; x b Cy Cx
        fld     st                      ; x x b Cy Cx
        fmul    st,st                   ; x^2 x b Cy Cx
        fld     inity                   ; y x^2 x b Cy Cx

bottom_of_dojulia_p5:
        fmul    st(2),st                ; y x^2 xy b Cy Cx
        fmul    st,st                   ; y^2 x^2 xy b Cy Cx

        fsub                            ; x^2-y^2 xy b Cy Cx
        fadd    st,st(4)                ; x^2-y^2+Cx xy b Cy Cx
        fxch                            ; xy x^2-y^2+Cx b Cy Cx

        fadd    st,st                   ; 2xy x^2-y^2+Cx b Cy Cx
        fadd    st,st(3)                ; 2xy+Cy x^2-y^2+Cx b Cy Cx

; first iteration complete
; {FPU stack all set, we're ready for the start of the loop}
EVEN
LoopStart:

; {While (Sqr(x) + Sqr(y) < b) and (Count < MaxIterations) do}
;    {square both numbers}
        fld     st(1)                   ;  {x, y, x, b, Cy, Cx}
        fmul    st(0),st(0)             ;  {x^2, y, x, b, Cy, Cx}
        fld     st(1)                   ;  {y, x^2, y, x, b, Cy, Cx}
        fmul    st(0),st(0)             ;  {y^2, x^2, y, x, b, Cy, Cx}

;    {add both squares and leave at top of stack ready for the compare}
        fld     st(1)                   ;  {x^2, y^2, x^2, y, x, b, Cy, Cx}
        fadd    st(0),st(1)             ;  {(y^2)+(x^2), y^2, x^2, y, x, b, Cy, Cx}
;    {Check to see if (x^2)+(y^2) < b and discard (x^2)+(y^2)}
        fcomp   st(5)                   ;  {y^2, x^2, y, x, b, Cy, Cx}

        cmp     ecx,ebx    ; put oldcoloriter in ebx above
        jae     SkipTasks  ; don't check periodicity

        fnstsw  ax         ;Get the pending NPX info into AX

        test    ecx,savedand_p5         ; save on 0, check on anything else
        jnz     do_check_p5_fast        ;  time to save a new "old" value
        jmp     save_new_old_value
;        jz      save_new_old_value
;        jmp     do_check_p5_fast        ;  time to save a new "old" value
EVEN
per_check_p5_ret_fast:

        pop     ax              ;pop AX to continue with the FCOMP test
        jz      short JustAfterFnstsw ;test that got us here, & pairable
        jmp     short JustAfterFnstsw ;since we have done the FNSTSW,
                                ; Skip over next instruction     
EVEN
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
        fsubr   st(0),st(6)             ;  {Cx-y^2, x^2, y, x, b, Cy, Cx}

;  CAE changed this around for Pentium, 10 Oct 1998
;  exchange this pending result with y so there's no wait for fsubr to finish
        fxch    st(2)                   ; {y, x^2, Cx-y^2, x, b, Cy, Cx}

;  now compute x*y while the above fsubr is still running
        fmulp   st(3),st                ; {x^2, Cx-y^2, xy, b, Cy, Cx}

;    {... then add x^2 to Cx-y^2}
        faddp   st(1),st(0)             ; {Newx, xy, b, Cy, Cx}

;    {Place the temp (Newx) in the x slot ready for next time in the
;     loop, while placing xy in ST(0) to use below.}
        fxch    st(1)                   ;  {xy, Newx, b, Cy, Cx}

; {y = (y * x * 2) + Cy   (Use old x, not temp)}
;    {multiply y * x was already done above so it was removed here -- CAE}


;    {Now multiply x*y by 2 (add ST to ST)}
        fadd    st,st(0)                ;  {x*y*2, Newx, b, Cy, Cx}

;  compare was moved down so it would run concurrently with above add -- CAE
        cmp     show_orbit,0            ; is show_orbit clear

;    {Finally, add Cy to x*y*2}
        fadd    st(0),st(3)              ;  {Newy, Newx, b, Cy, Cx}

        jz      no_show_orbit_p5         ; if so then skip
        call    near ptr show_orbit_xy_p5  ; y x b Cy Cx
EVEN
no_show_orbit_p5:

        dec     ecx
        jnz     LoopStart
;        jmp     LoopStart    ;  {FPU stack has required elements for next loop}
EVEN
LoopEnd:                                ;  {Newy, Newx, b, Cy, Cx}

; reached maxit, inside
        mov     eax,maxit
        mov     oldcoloriter,-1        ; check periodicity immediately next time
        mov     realcoloriter,eax      ; save unadjusted realcolor
        mov     eax,inside_color
        jmp     short overiteration_p5
EVEN
overbailout_p5:

        fadd                            ; x^2+y^2 y x b Cy Cx
        mov     eax,ecx
        fstp    magnitude               ; y x b Cy Cx
        sub     eax,10                  ; 10 more next time before checking

        jns     no_fix_underflow_p5
; if the number of iterations was within 10 of maxit, then subtracting
; 10 would underflow and cause periodicity checking to start right
; away.  Catching a period doesn't occur as often in the pixels at
; the edge of the set anyway.
        sub     eax,eax                 ; don't check next time
no_fix_underflow_p5:
        mov     oldcoloriter,eax        ; check when past this - 10 next time
        mov     eax,maxit
        sub     eax,ecx                 ; leave 'times through loop' in eax

; zero color fix
        jnz     zero_color_fix_p5
        inc     eax                     ; if (eax == 0 ) eax = 1
zero_color_fix_p5:
        mov     realcoloriter,eax       ; save unadjusted realcolor
        sub     kbdcount,ax             ; adjust the keyboard count

        cmp     outside,-1              ; iter ? (most common case)
        je      overiteration_p5
        cmp     outside,-2              ; outside <= -2 ?
        jle     to_special_outside_p5   ; yes, go do special outside options
        sub     eax,eax                 ; clear top half of eax for next
        mov     ax,outside              ; use outside color
        jmp     short overiteration_p5

to_special_outside_p5:

        call    near ptr special_outside_p5
EVEN
overiteration_p5:

        fstp    newy                    ; x b Cy Cx
        fstp    newx                    ; b Cy Cx


;    {Pop 3 used registers from FPU stack, discarding the values.
;       All we care about is ECX, the count.}

        fcompp
        fstp    st
        mov     coloriter,eax

        cmp     orbit_ptr,0             ; any orbits to clear?
        je      calcmandfpasm_ret_p5    ; nope.
        call    far ptr scrub_orbit     ; clear out any old orbits
        mov     eax,coloriter           ; restore color
                                        ; speed not critical here in orbit land

calcmandfpasm_ret_p5:

        mov     edx,eax       ;     {The low 16 bits already in AX}
        shr     edx,16        ;     {Shift high 16 bits to low 16 bits position}

        ret

calcmandfpasm_p5   ENDP

ALIGN 16
show_orbit_xy_p5   PROC NEAR USES ebx ecx edx esi edi
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
; USES is needed because in all likelyhood, plot_orbit surely
; uses these registers.  It's ok to have to push/pop's here in the
; orbits as speed is not crucial when showing orbits.

                                        ; fpu stack is either
                                        ; y x b Cx Cy (p5)
        fld     st(1)                   ;
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
        fwait                           ; just to be safe
;        push    word ptr orbit_imag+6   ; co-ordinates for plot orbit
        push    dword ptr orbit_imag+4   ;       ...
;        push    word ptr orbit_imag+2   ;       ...
        push    dword ptr orbit_imag     ;       ...
;        push    word ptr orbit_real+6   ; co-ordinates for plot orbit
        push    dword ptr orbit_real+4   ;       ...
;        push    word ptr orbit_real+2   ;       ...
        push    dword ptr orbit_real     ;       ...
        call    far ptr plot_orbit      ; display the orbit
        add     sp,9*2                  ; clear out the parameters

        fld     tmp_ten_byte_5
        fld     tmp_ten_byte_4
        fld     tmp_ten_byte_3
        fld     tmp_ten_byte_2
        fld     tmp_ten_byte_1
        fwait                           ; just to be safe
        ret
show_orbit_xy_p5   ENDP

ALIGN 16
special_outside_p5 PROC NEAR
; When type casting floating point variables to integers in C, the decimal
; is truncated.  When using FIST in asm, the value is rounded.  Using
; "FSUB round_down_half" causes the values to be rounded down.
; Boo-Hiss if values are negative, change FPU control word to truncate values.
LOCAL Control:word
        fstcw Control
        push  Control                       ; Save control word on the stack
        or    Control, 0000110000000000b
        fldcw Control                       ; Set control to round towards zero

        cmp     outside,-2
        jne     short not_real

        fld     st(1)                  ; newx
        test    bad_outside,1h
        jz      over_bad_real
        fsub    round_down_half
        jmp     over_good_real
over_bad_real:
        frndint
over_good_real:
        fistp   tmp_dword
        add     eax,7
        add     eax,tmp_dword
        jmp     check_color
not_real:
        cmp     outside,-3
        jne     short not_imag
        fld     st(0)            ; newy
        test    bad_outside,1h
        jz      short over_bad_imag
        fsub    round_down_half
        jmp     short over_good_imag
over_bad_imag:
        frndint
over_good_imag:
        fistp   tmp_dword
        add     eax,7
        add     eax,tmp_dword
        jmp     check_color
not_imag:
        cmp     outside,-4
        jne     short not_mult
        push    ax              ; save current ax value
        fld     st(0)           ; newy
        ftst                    ; check to see if newy == 0
        fstsw   ax
        sahf
        pop     ax              ; retrieve ax (does not affect flags)
        jne     short non_zero_y
        fcomp   st(0)           ; pop it off the stack
;        ret                     ; if y==0, return with normal ax
        jmp     special_outside_ret
non_zero_y:
        fdivr   st(0),st(2)         ; newx/newy
        mov     tmp_dword,eax
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
        mov     eax,tmp_dword
        jmp     short check_color
not_mult:
        cmp     outside,-5
        jne     short not_sum
        fld     st(1)           ; newx
        fadd    st(0),st(1)     ; newx+newy
        test    bad_outside,1h
        jz     short over_bad_summ
        fsub    round_down_half
        jmp     short over_good_summ
over_bad_summ:
        frndint
over_good_summ:
        fistp   tmp_dword
        fwait
        add     eax,tmp_dword
        jmp     short check_color
not_sum:
        cmp     outside,-6      ; currently always equal, but put here
        jne     short not_atan        ; for future outside types
        fld     st(0)           ; newy
        fld     st(2)           ; newx newy
        fpatan                  ; arctan(y/x)
        fimul   atan_colors     ; atan_colors*atan
        fldpi                   ; pi atan_colors*atan
        fdiv                    ; atan_colors*atan/pi
        fabs
        frndint
        fistp   tmp_dword
        fwait
        mov     eax,tmp_dword

not_atan:
check_color:
        cmp     eax,maxit               ; use UNSIGNED comparison
        jbe     short check_release     ; color < 0 || color > maxit
        sub     eax,eax                 ; eax = 0
check_release:
        cmp     save_release,1961
        jb      short special_outside_ret
        cmp     eax,0
        jne     special_outside_ret
        mov     eax,1                   ; eax = 1
special_outside_ret:
        pop   Control
        fldcw Control              ; Restore control word
        ret
special_outside_p5 ENDP

calmanp5_text  ENDS

END

