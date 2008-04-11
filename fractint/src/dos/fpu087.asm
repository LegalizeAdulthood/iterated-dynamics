TITLE fpu087.asm (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
SUBTTL All rights reserved.
;
;  Code may be used in any program provided the author is credited
;    either during program execution or in the documentation.  Source
;    code may be distributed only in combination with public domain or
;    shareware source code.  Source code may be modified provided the
;    copyright notice and this message is left unchanged and all
;    modifications are clearly documented.
;
;    I would appreciate a copy of any work which incorporates this code.
;
;    Mark C. Peterson
;    405-C Queen St., Suite #181
;    Southington, CT 06489
;    (203) 276-9721
;
;  References:
;     The VNR Concise Encyclopedia of Mathematics
;        by W. Gellert, H. Hustner, M. Hellwich, and H. Kastner
;        Published by Van Nostrand Reinhold Comp, 1975
;
;     80386/80286 Assembly Language Programming
;        by William H. Murray, III and Chris H. Pappas
;        Published by Osborne McGraw-Hill, 1986
;
;  History since Fractint 16.3:
;     CJLT changed e2x and Log086 algorithms for more speed
;     CJLT corrected SinCos086 for -ve angles in 2nd and 4th quadrants
;     CJLT speeded up SinCos086 for angles >45 degrees in any quadrant
;     (See comments containing the string `CJLT')
; 14 Aug 91 CJLT removed r16Mul - not called from anywhere
; 21 Aug 91 CJLT corrected Table[1] from 6 to b
;                improved bx factors in Log086 for more accuracy
;                corrected Exp086 overflow detection to 15 from 16 bits.
; 07 Sep 92 MP   corrected problem in FPUcplxlog
; 07 Sep 92 MP   added argument test for FPUcplxdiv
; 06 Nov 92 CAE  made some varibles public for PARSERA.ASM
; 07 Dec 92 CAE  sped up FPUsinhcosh function
;
; CJLT=      Chris J Lusby Taylor
;            32 Turnpike Road
;            Newbury, England (where's that?)
;        Contactable via Compuserve user Stan Chelchowski [100016,351]
;                     or Tel 011 44 635 33270 (home)
;
; CAE=   Chuck Ebbert  CompuServe [76306,1226]
;
;
;PROCs in this module:
;FPUcplxmul     PROC     x:word, y:word, z:word
;FPUcplxdiv     PROC     x:word, y:word, z:word
;FPUcplxlog     PROC     x:word, z:word
;FPUsinhcosh    PROC     x:word, sinh:word, cosh:word
;FPUsincos  PROC  x:word, sinx:word, cosx:word
;r16Mul     PROC    uses si di, x1:word, x2:word, y1:word, y2:word
;RegFloat2Fg     PROC    x1:word, x2:word, Fudge:word
;RegFg2Float     PROC   x1:word, x2:word, FudgeFact:byte
;ExpFudged      PROC    uses si, x_low:word, x_high:word, Fudge:word
;LogFudged      PROC    uses si di, x_low:word, x_high:word, Fudge:word
;LogFloat14     PROC     x1:word, x2:word
;RegSftFloat     PROC   x1:word, x2:word, Shift:byte
;RegDivFloat     PROC  uses si di, x1:word, x2:word, y1:word, y2:word
;SinCos086   PROC     uses si di, LoNum:WORD, HiNum:WORD, SinAddr:WORD, \
;_e2xLT   PROC          ;argument in dx.ax (bitshift=16 is hard coded here)
;Exp086    PROC     uses si di, LoNum:WORD, HiNum:WORD
;SinhCosh086    PROC     uses si di, LoNum:WORD, HiNum:WORD, SinhAddr:WORD, \
;Log086   PROC     uses si di, LoNum:WORD, HiNum:WORD, Fudge:WORD


IFDEF ??version
MASM51
QUIRKS
ENDIF

.model medium, c

extrn cos:far
extrn _Loaded387sincos:far
extrn compiled_by_turboc:word


.data

extrn cpu:WORD
extrn overflow:word
;extrn save_release:word
PUBLIC TrigLimit

; CAE 6Nov92 made these public for PARSERA.ASM */
PUBLIC _1_, _2_, PointFive, infinity

PiFg13         dw       6487h
InvPiFg33      dd       0a2f9836eh
InvPiFg16      dw       517ch
Ln2Fg16        dw       0b172h ;ln(2) * 2^16 . True value is b172.18
Ln2Fg15        dw       058b9h ;used by e2xLT. True value is 58b9.0C
TrigLimit      dd       0
;
;Table of 2^(n/16) for n=0 to 15. All entries fg15.
;Used by e2xLT
;
Table           dw      08000h,085abh,08b96h,091c4h
                dw      09838h,09ef5h,0a5ffh,0ad58h
                dw      0b505h,0bd09h,0c567h,0ce25h
                dw      0d745h,0e0cdh,0eac1h,0f525h
one            dw       ?
expSign        dw       ?
a              dw       ?
SinNeg         dw       ?       ;CJLT - not now needed by SinCos086, but by
CosNeg         dw       ?       ;       ArcTan086
Ans            dq       ?
fake_es        dw       ?         ; <BDT> Windows can't use ES for storage

TaylorTerm  MACRO
LOCAL Ratio
   add   Factorial, one
   jnc   SHORT Ratio

   rcr   Factorial, 1
   shr   Num, 1
   shr   one, 1

Ratio:
   mul   Num
   div   Factorial
ENDM



_4_         dq    4.0
_2_         dq    2.0
_1_         dq    1.0
PointFive   dq    0.5
temp        dq     ?
Sign        dw     ?

extrn fpu:word

infinity          dq    1.0E+300

.code


FPUcplxmul     PROC     x:word, y:word, z:word
   mov   bx, x
   fld   QWORD PTR [bx]       ; x.x
   fld   QWORD PTR [bx+8]     ; x.y, x.x
   mov   bx, y
   fld   QWORD PTR [bx]       ; y.x, x.y, x.x
   fld   QWORD PTR [bx+8]     ; y.y, y.x, x.y, x.x
   mov   bx, z
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st(3)            ; y.y*x.y, y.y. y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*x.y, y.y, y.x, x.y, x.x
   fmul  st, st(5)            ; y.x*x.x, y.y*x.y, y.y, y.x, x.y, x.x
   fsubr                      ; y.x*x.x - y.y*x.y, y.y, y.x, x.y, x.x
   fstp  QWORD PTR [bx]       ; y.y, y.x, x.y, x.x
   fmulp st(3), st            ; y.x, x.y, x.x*y.y
   fmul                       ; y.x*x.y, x.x*y.y
   fadd                       ; y.x*x.y + x.x*y.y
   fstp  QWORD PTR [bx+8]
   ret
FPUcplxmul     ENDP


FPUcplxdiv     PROC     x:word, y:word, z:word
LOCAL Status:WORD
   mov   bx, x
   fld   QWORD PTR [bx]       ; x.x
   fld   QWORD PTR [bx+8]     ; x.y, x.x
   mov   bx, y
   fld   QWORD PTR [bx]       ; y.x, x.y, x.x
   fld   QWORD PTR [bx+8]     ; y.y, y.x, x.y, x.x
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st               ; y.y*y.y, y.y, y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*y.y, y.y, y.x, x.y, x.x
   fmul  st, st               ; y.x*y.x, y.y*y.y, y.y, y.x, x.y, x.x
   fadd                       ; mod, y.y, y.x, x.y, x.x

   ftst                       ; test whether mod is (0,0)
   fstsw Status
   mov   ax, Status
   and   ah, 01000101b
   cmp   ah, 01000000b
   jne   NotZero

   fstp  st
   fstp  st
   fstp  st
   fstp  st
   fstp  st

   fld   infinity
   fld   st
   mov   bx, z
   fstp  QWORD PTR [bx]
   fstp  QWORD PTR [bx+8]
;   mov   ax,save_release
;   cmp   ax,1920
;   jle   ExitDiv       ; before 19.20 overflow wasn't set
   mov   overflow, 1
   jmp   ExitDiv

NotZero:
   fdiv  st(1), st            ; mod, y.y=y.y/mod, y.x, x.y, x.x
   fdivp st(2), st            ; y.y, y.x=y.x/mod, x.y, x.x
   mov   bx, z
   fld   st                   ; y.y, y.y, y.x, x.y, x.x
   fmul  st, st(3)            ; y.y*x.y, y.y. y.x, x.y, x.x
   fld   st(2)                ; y.x, y.y*x.y, y.y, y.x, x.y, x.x
   fmul  st, st(5)            ; y.x*x.x, y.y*x.y, y.y, y.x, x.y, x.x
   fadd                       ; y.x*x.x - y.y*x.y, y.y, y.x, x.y, x.x
   fstp  QWORD PTR [bx]       ; y.y, y.x, x.y, x.x
   fmulp st(3), st            ; y.x, x.y, x.x*y.y
   fmul                       ; y.x*x.y, x.x*y.y
   fsubr                      ; y.x*x.y + x.x*y.y
   fstp  QWORD PTR [bx+8]

ExitDiv:
   ret
FPUcplxdiv     ENDP



FPUcplxlog     PROC     x:word, z:word
LOCAL Status:word, ImagZero:WORD
   mov   bx, x

   mov   ax, WORD PTR [bx+8+6]
   mov   ImagZero, ax
   or    ax, WORD PTR [bx+6]
   jnz   NotBothZero

   fldz
   fldz
   jmp   StoreZX

NotBothZero:
   fld   QWORD PTR [bx+8]        ; x.y
   fld   QWORD PTR [bx]          ; x.x, x.y
   mov   bx, z
   fldln2                        ; ln2, x.x, x.y
   fdiv  _2_                     ; ln2/2, x.x, x.y
   fld   st(2)                   ; x.y, ln2/2, x.x, x.y
   fmul  st, st                  ; sqr(x.y), ln2/2, x.x, x.y
   fld   st(2)                   ; x.x, sqr(x.y), ln2/2, x.x, x.y
   fmul  st, st                  ; sqr(x.x), sqr(x.y), ln2/2, x.x, x.y
   fadd                          ; mod, ln2/2, x.x, x.y
   fyl2x                         ; z.x, x.x, x.y
   fxch  st(2)                   ; x.y, x.x, z.x
   fxch                          ; x.x, x.y, z.x
   cmp   fpu, 387
   jb    Restricted

   fpatan                        ; z.y, z.x
   jmp   StoreZX

Restricted:
   mov   bx, x
   mov   dh, BYTE PTR [bx+7]
   or    dh, dh
   jns   ChkYSign

   fchs                          ; |x.x|, x.y, z.x

ChkYSign:
   mov   dl, BYTE PTR [bx+8+7]
   or    dl, dl
   jns   ChkMagnitudes

   fxch                          ; x.y, |x.x|, z.x
   fchs                          ; |x.y|, |x.x|, z.x
   fxch                          ; |x.x|, |x.y|, z.x

ChkMagnitudes:
   fcom  st(1)                   ; x.x, x.y, z.x
   fstsw Status                  ; x.x, x.y, z.x
   test  Status, 4500h
   jz    XisGTY

   test  Status, 4000h
   jz    XneY

   fstp  st                      ; x.y, z.x
   fstp  st                      ; z.x
   fldpi                         ; Pi, z.x
   fdiv  _4_                     ; Pi/4, z.x
   jmp   ChkSignZ

XneY:
   fxch                          ; x.y, x.x, z.x
   fpatan                        ; Pi/2 - Angle, z.x
   fldpi                         ; Pi, Pi/2 - Angle, z.x
   fdiv  _2_                     ; Pi/2, Pi/2 - Angle, z.x
   fsubr                         ; Angle, z.x
   jmp   ChkSignZ

XisGTY:
   fpatan                        ; Pi-Angle or Angle+Pi, z.x

ChkSignZ:
   or    dh, dh
   js    NegX

   or    dl, dl
   jns   StoreZX

   fchs
   jmp   StoreZX

NegX:
   or    dl, dl
   js    QuadIII

   fldpi                        ; Pi, Pi-Angle, z.x
   fsubr                        ; Angle, z.x
   jmp   StoreZX

QuadIII:
   fldpi                        ; Pi, Angle+Pi, z.x
   fsub                         ; Angle,  z.x

StoreZX:
   mov   bx, z
   fstp  QWORD PTR [bx+8]       ; z.x
   fstp  QWORD PTR [bx]         ; <empty>
   ret
FPUcplxlog     ENDP

FPUsinhcosh    PROC     x:word, sinh:word, cosh:word
LOCAL Control:word
   fstcw Control
   push  Control                       ; Save control word on the stack
   or    Control, 0000110000000000b
   fldcw Control                       ; Set control to round towards zero

   mov   Sign, 0              ; Assume the sign is positive
   mov   bx, x

   fldln2                     ; ln(2)
   fdivr QWORD PTR [bx]       ; x/ln(2)

   cmp   BYTE PTR [bx+7], 0
   jns   DuplicateX

   fchs                       ; x = |x|

DuplicateX:
   fld   st                   ; x/ln(2), x/ln(2)
   frndint                    ; int = integer(|x|/ln(2)), x/ln(2)
   fxch                       ; x/ln(2), int
   fsub  st, st(1)            ; rem < 1.0, int
   fmul  PointFive            ; rem/2 < 0.5, int
      ; CAE 7Dec92 changed above from divide by 2 to multiply by .5
   f2xm1                      ; (2**rem/2)-1, int
   fadd  _1_                  ; 2**rem/2, int
   fmul  st, st               ; 2**rem, int
   fscale                     ; e**|x|, int
   fstp  st(1)                ; e**|x|

   cmp   BYTE PTR [bx+7], 0
   jns   ExitFexp

   fdivr _1_                  ; e**x

ExitFexp:
   fld   st                   ; e**x, e**x
   fdivr PointFive            ; e**-x/2, e**x
   fld   st                   ; e**-x/2, e**-x/2, e**x
   fxch  st(2)                ; e**x, e**-x/2, e**-x/2
   fmul  PointFive            ; e**x/2,  e**-x/2, e**-x/2
      ; CAE 7Dec92 changed above from divide by 2 to multiply by .5
   fadd  st(2), st            ; e**x/2,  e**-x/2, cosh(x)
   fsubr                      ; sinh(x), cosh(x)

   mov   bx, sinh             ; sinh, cosh
   fstp  QWORD PTR [bx]       ; cosh
   mov   bx, cosh
   fstp  QWORD PTR [bx]       ; <empty>

   pop   Control
   fldcw Control              ; Restore control word
   ret
FPUsinhcosh    ENDP


FPUsincos  PROC  x:word, sinx:word, cosx:word
LOCAL Status:word
   mov   bx, x
   fld   QWORD PTR [bx]       ; x

   cmp   fpu, 387
   jb    Use387FPUsincos

   call  _Loaded387sincos     ; cos(x), sin(x)
   mov   bx, cosx
   fstp  QWORD PTR [bx]       ; sin(x)
   mov   bx, sinx
   fstp  QWORD PTR [bx]       ; <empty>
   ret

Use387FPUsincos:

   sub   sp, 8                ; save 'x' on the CPU stack
   mov   bx, sp
   fstp  QWORD PTR [bx]       ; FPU stack:  <empty>

   call  cos

   add   sp, 8                ; take 'cos(x)' off the CPU stack
   mov   bx, ax
   cmp   compiled_by_turboc,0
   jne   turbo_c1

   fld   QWORD PTR [bx]       ; FPU stack:  cos(x)

turbo_c1:
   fld   st                   ; FPU stack:  cos(x), cos(x)
   fmul  st, st               ; cos(x)**2, cos(x)
   fsubr _1_                  ; sin(x)**2, cos(x)
   fsqrt                      ; +/-sin(x), cos(x)

   mov   bx, x
   fld   QWORD PTR [bx]       ; x, +/-sin(x), cos(x)
   fldpi                      ; Pi, x, +/-sin(x), cos(x)
   fadd  st, st               ; 2Pi, x, +/-sin(x), cos(x)
   fxch                       ; |x|, 2Pi, +/-sin(x), cos(x)
   fprem                      ; Angle, 2Pi, +/-sin(x), cos(x)
   fstp  st(1)                ; Angle, +/-sin(x), cos(x)
   fldpi                      ; Pi, Angle, +/-sin(x), cos(x)

   cmp   BYTE PTR [bx+7], 0
   jns   SignAlignedPi

   fchs                       ; -Pi, Angle, +/-sin(x), cos(x)

SignAlignedPi:
   fcompp                     ; +/-sin(x), cos(x)
   fstsw Status               ; +/-sin(x), cos(x)

   mov   ax, Status
   and   ah, 1
   jz    StoreSinCos          ; Angle <= Pi

   fchs                       ; sin(x), cos(x)

StoreSinCos:
   mov   bx, sinx
   fstp  QWORD PTR [bx]       ; cos(x)
   mov   bx, cosx
   fstp  QWORD PTR [bx]       ; <empty>
   ret
FPUsincos   ENDP


PUBLIC r16Mul
r16Mul     PROC    uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2

      xor   ax, ax
      shl   bx, 1
      jz    Exitr16Mult          ; Destination is zero

      rcl   ah, 1
      shl   cx, 1
      jnz   Chkr16Exp
      xor   bx, bx               ; Source is zero
      xor   si, si
      jmp   Exitr16Mult

   Chkr16Exp:
      rcl   al, 1
      xor   ah, al               ; Resulting sign in ah
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, 127              ; Determine resulting exponent
      add   bh, ch
      mov   al, bh
      mov   fake_es, ax          ; es has the resulting exponent and sign

      mov   ax, di
      mov   al, ah
      mov   ah, cl

      mov   dx, si
      mov   dl, dh
      mov   dh, bl

      mul   dx
      mov   cx, fake_es

      shl   ax, 1
      rcl   dx, 1
      jnc   Remr16MulOneBit      ; 'One' bit is the next bit over

      inc   cl                   ; 'One' bit removed with previous shift
      jmp   Afterr16MulNorm

   Remr16MulOneBit:
      shl   ax, 1
      rcl   dx, 1

   Afterr16MulNorm:
      mov   bl, dh               ; Perform remaining 8 bit shift
      mov   dh, dl
      mov   dl, ah
      mov   si, dx
      mov   bh, cl               ; Put in the exponent
      rcr   ch, 1                ; Get the sign
      rcr   bx, 1                ; Normalize the result
      rcr   si, 1
   Exitr16Mult:
      mov   ax, si
      mov   dx, bx
      ret
r16Mul      ENDP


PUBLIC RegFloat2Fg
RegFloat2Fg     PROC    x1:word, x2:word, Fudge:word
      mov   ax, WORD PTR x1
      mov   dx, WORD PTR x2
      mov   bx, ax
      or    bx, dx
      jz    ExitRegFloat2Fg

      xor   bx, bx
      mov   cx, bx

      shl   ax, 1
      rcl   dx, 1
      rcl   bx, 1                   ; bx contains the sign

      xchg  cl, dh                  ; cx contains the exponent

      stc                           ; Put in the One bit
      rcr   dl, 1
      rcr   ax, 1

      sub   cx, 127 + 23
      add   cx, Fudge
      jz    ChkFgSign
      jns   ShiftFgLeft

      neg   cx
   ShiftFgRight:
      shr   dx, 1
      rcr   ax, 1
      loop  ShiftFgRight
      jmp   ChkFgSign

   ShiftFgLeft:
      shl   ax, 1
      rcl   dx, 1
      loop  ShiftFgLeft

   ChkFgSign:
      or    bx, bx
      jz    ExitRegFloat2Fg

      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0

   ExitRegFloat2Fg:
      ret
RegFloat2Fg    ENDP

PUBLIC RegFg2Float
RegFg2Float     PROC   x1:word, x2:word, FudgeFact:byte
      mov   ax, x1
      mov   dx, x2

      mov   cx, ax
      or    cx, dx
      jz    ExitFudgedToRegFloat

      mov   ch, 127 + 32
      sub   ch, FudgeFact
      xor   cl, cl
      shl   ax, 1       ; Get the sign bit
      rcl   dx, 1
      jnc   FindOneBit

      inc   cl          ; Fudged < 0, convert to postive
      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0

   FindOneBit:
      shl   ax, 1
      rcl   dx, 1
      dec   ch
      jnc   FindOneBit
      dec   ch

      mov   al, ah
      mov   ah, dl
      mov   dl, dh
      mov   dh, ch

      shr   cl, 1       ; Put sign bit in
      rcr   dx, 1
      rcr   ax, 1

   ExitFudgedToRegFloat:
      ret
RegFg2Float      ENDP


PUBLIC ExpFudged
ExpFudged      PROC     uses si, x_low:word, x_high:word, Fudge:word
LOCAL exp:WORD
      xor   ax, ax
      mov   WORD PTR Ans, ax
      mov   WORD PTR Ans + 2, ax
      mov   ax, WORD PTR x_low
      mov   dx, WORD PTR x_high
      or    dx, dx
      js    NegativeExp

      div   Ln2Fg16
      mov   exp, ax
      or    dx, dx
      jz    Raiseexp

      mov   ax, dx
      mov   si, dx
      mov   bx, 1

   PosExpLoop:
      add   WORD PTR Ans, ax
      adc   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jnz   PosExpLoop

   Raiseexp:
      inc   WORD PTR Ans + 2
      mov   ax, WORD PTR Ans
      mov   dx, WORD PTR Ans + 2
      mov   cx, -16
      add   cx, Fudge
      add   cx, exp
      or    cx, cx
      jz    ExitExpFudged
      jns   LeftShift
      neg   cx

   RightShift:
      shr   dx, 1
      rcr   ax, 1
      loop  RightShift
      jmp   ExitExpFudged

   NegativeExp:
      not   ax
      not   dx
      add   ax, 1
      adc   dx, 0
      div   Ln2Fg16
      neg   ax
      mov   exp, ax

      or    dx, dx
      jz    Raiseexp

      mov   ax, dx
      mov   si, dx
      mov   bx, 1

   NegExpLoop:
      sub   WORD PTR Ans, ax
      sbb   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jz    Raiseexp

      add   WORD PTR Ans, ax
      adc   WORD PTR Ans + 2, 0
      inc   bx
      mul   si
      mov   ax, dx
      xor   dx, dx
      div   bx
      or    ax, ax
      jnz   NegExpLoop
      jmp   Raiseexp

   LeftShift:
      shl   ax, 1
      rcl   dx, 1
      loop  LeftShift

   ExitExpFudged:
      ret
ExpFudged      ENDP



PUBLIC   LogFudged
LogFudged      PROC     uses si di, x_low:word, x_high:word, Fudge:word
LOCAL exp:WORD
      xor   bx, bx
      mov   cx, 16
      sub   cx, Fudge
      mov   ax, x_low
      mov   dx, x_high

      or    dx, dx
      jz    ChkLowWord

   Incexp:
      shr   dx, 1
      jz    DetermineOper
      rcr   ax, 1
      inc   cx
      jmp   Incexp

   ChkLowWord:
      or    ax, ax
      jnz   Decexp
      jmp   ExitLogFudged

   Decexp:
      dec   cx                      ; Determine power of two
      shl   ax, 1
      jnc   Decexp

   DetermineOper:
      mov   exp, cx
      mov   si, ax                  ; si =: x + 1
      shr   si, 1
      stc
      rcr   si, 1
      mov   dx, ax
      xor   ax, ax
      shr   dx, 1
      rcr   ax, 1
      shr   dx, 1
      rcr   ax, 1                   ; dx:ax = x - 1
      div   si

      mov   bx, ax                  ; ax, Fudged 16, max of 0.3333333
      shl   ax, 1                   ; x = (x - 1) / (x + 1), Fudged 16
      mul   ax
      shl   ax, 1
      rcl   dx, 1
      mov   ax, dx                  ; dx:ax, Fudged 35, max = 0.1111111
      mov   si, ax                  ; si = (ax * ax), Fudged 19

      mov   ax, bx
   ; bx is the accumulator, First term is x
      mul   si                      ; dx:ax, Fudged 35, max of 0.037037
      mov   fake_es, dx             ; Save high word, Fudged (35 - 16) = 19
      mov   di, 0c000h              ; di, 3 Fudged 14
      div   di                      ; ax, Fudged (36 - 14) = 21
      or    ax, ax
      jz    Addexp

      mov   cl, 5
      shr   ax, cl
      add   bx, ax                  ; bx, max of 0.345679
   ; x = x + x**3/3

      mov   ax, fake_es             ; ax, Fudged 19
      mul   si                      ; dx:ax, Fudged 38, max of 0.004115
      mov   fake_es, dx             ; Save high word, Fudged (38 - 16) = 22
      mov   di, 0a000h              ; di, 5 Fudged 13
      div   di                      ; ax, Fudged (38 - 13) = 25
      or    ax, ax
      jz    Addexp

      mov   cl, 9
      shr   ax, cl
      add   bx, ax
   ; x = x + x**3/3 + x**5/5

      mov   ax, fake_es             ; ax, Fudged 22
      mul   si                      ; dx:ax, Fudged 41, max of 0.0004572
      mov   di, 0e000h              ; di, 7 Fudged 13
      div   di                      ; ax, Fudged (41 - 13) = 28
      mov   cl, 12
      shr   ax, cl
      add   bx, ax

   Addexp:
      shl   bx, 1                   ; bx *= 2, Fudged 16, max of 0.693147
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7)
      mov   cx, exp
      mov   ax, Ln2Fg16            ; Answer += exp * Ln2Fg16
      or    cx, cx
      js    SubFromAns

      mul   cx
      add   ax, bx
      adc   dx, 0
      jmp   ExitLogFudged

   SubFromAns:
      neg   cx
      mul   cx
      xor   cx, cx
      xchg  cx, dx
      xchg  bx, ax
      sub   ax, bx
      sbb   dx, cx

   ExitLogFudged:
   ; x = 2 * (x + x**3/3 + x**5/5 + x**7/7) + (exp * Ln2Fg16)
      ret
LogFudged      ENDP




PUBLIC LogFloat14
LogFloat14     PROC     x1:word, x2:word
      mov   ax, WORD PTR x1
      mov   dx, WORD PTR x2
      shl   ax, 1
      rcl   dx, 1
      xor   cx, cx
      xchg  cl, dh

      stc
                rcr   dl, 1
                rcr   ax, 1

      sub   cx, 127 + 23
      neg   cx
      push  cx
      push  dx
      push  ax
      call  LogFudged
      add   sp, 6
      ret
LogFloat14     ENDP


PUBLIC RegSftFloat
RegSftFloat     PROC   x1:word, x2:word, Shift:byte
      mov   ax, x1
      mov   dx, x2

      shl   dx, 1
      rcl   cl, 1

      add   dh, Shift

      shr   cl, 1
      rcr   dx, 1

      ret
RegSftFloat      ENDP




PUBLIC RegDivFloat
RegDivFloat     PROC  uses si di, x1:word, x2:word, y1:word, y2:word
      mov   si, x1
      mov   bx, x2
      mov   di, y1
      mov   cx, y2

      xor   ax, ax
      shl   bx, 1
      jnz   ChkOtherOp
      jmp   ExitRegDiv           ; Destination is zero

   ChkOtherOp:
      rcl   ah, 1
      shl   cx, 1
      jnz   ChkDivExp
      xor   bx, bx               ; Source is zero
      xor   si, si
      jmp   ExitRegDiv

   ChkDivExp:
      rcl   al, 1
      xor   ah, al               ; Resulting sign in ah
      stc                        ; Put 'one' bit back into number
      rcr   bl, 1
      stc
      rcr   cl, 1

      sub   ch, 127              ; Determine resulting exponent
      sub   bh, ch
      mov   al, bh
      mov   fake_es, ax          ; es has the resulting exponent and sign

      mov   ax, si               ; 8 bit shift, bx:si moved to dx:ax
      mov   dh, bl
      mov   dl, ah
      mov   ah, al
      xor   al, al

      mov   bh, cl               ; 8 bit shift, cx:di moved to bx:cx
      mov   cx, di
      mov   bl, ch
      mov   ch, cl
      xor   cl, cl

      shr   dx, 1
      rcr   ax, 1

      div   bx
      mov   si, dx               ; Save (and shift) remainder
      mov   dx, cx               ; Save the quess
      mov   cx, ax
      mul   dx                   ; Mult quess times low word
      xor   di, di
      sub   di, ax               ; Determine remainder
      sbb   si, dx
      mov   ax, di
      mov   dx, si
      jc    RemainderNeg

      xor   di, di
      jmp   GetNextDigit

   RemainderNeg:
      mov   di, 1                ; Flag digit as negative
      not   ax                   ; Convert remainder to positive
      not   dx
      add   ax, 1
      adc   dx, 0

   GetNextDigit:
      shr   dx, 1
      rcr   ax, 1
      div   bx
      xor   bx, bx
      shl   dx, 1
      rcl   ax, 1
      rcl   bl, 1                ; Save high bit

      mov   dx, cx               ; Retrieve first digit
      or    di, di
      jz    RemoveDivOneBit

      neg   ax                   ; Digit was negative
      neg   bx
      dec   dx

   RemoveDivOneBit:
      add   dx, bx
      mov   cx, fake_es
      shl   ax, 1
      rcl   dx, 1
      jc    AfterDivNorm

      dec   cl
      shl   ax, 1
      rcl   dx, 1

   AfterDivNorm:
      mov   bl, dh               ; Perform remaining 8 bit shift
      mov   dh, dl
      mov   dl, ah
      mov   si, dx
      mov   bh, cl               ; Put in the exponent
      shr   ch, 1                ; Get the sign
      rcr   bx, 1                ; Normalize the result
      rcr   si, 1

   ExitRegDiv:
      mov   ax, si
      mov   dx, bx
      ret
RegDivFloat      ENDP



Term        equ      <ax>
Num         equ      <bx>
Factorial   equ      <cx>
Sin         equ      <si>
Cos         equ      <di>
e           equ      <si>
Inve        equ      <di>

SinCos086   PROC     uses si di, LoNum:WORD, HiNum:WORD, SinAddr:WORD, \
                                CosAddr:WORD
   mov   ax, LoNum
   mov   dx, HiNum

   xor   cx, cx
;   mov   SinNeg, cx    ;CJLT - not needed now
;   mov   CosNeg, cx     ;CJLT - not needed now
   mov   a, cx          ;CJLT - Not needed by the original code, but it
                        ;       is now!
   or    dx, dx
   jns   AnglePositive

   not   ax
   not   dx
   add   ax, 1
   adc   dx, cx         ;conveniently zero
   mov   a,8            ;a now has 4 bits: Sign+quadrant+octant

AnglePositive:
   mov   si, ax
   mov   di, dx
   mul   WORD PTR InvPiFg33
   mov   bx, dx
   mov   ax, di
   mul   WORD PTR InvPiFg33
   add   bx, ax
   adc   cx, dx
   mov   ax, si
   mul   WORD PTR InvPiFg33 + 2
   add   bx, ax
   adc   cx, dx
   mov   ax, di
   mul   WORD PTR InvPiFg33 + 2
   add   ax, cx
   adc   dx, 0

   and   dx, 3  ;Remove multiples of 2 pi
   shl   dx, 1  ;move over to make space for octant number
;
;CJLT - new code to reduce angle to:  0 <= angle <= 45
;
   or    ax, ax
   jns   Lessthan45
   inc   dx     ;octant number
   not   ax     ;angle=90-angle if it was >45 degrees
Lessthan45:
   add   a,  dx ;remember octant and Sign in a
   mov   Num, ax ;ax=Term, now
;
; We do the Taylor series with all numbers scaled by pi/2
; so InvPiFg33 represents one. Truly.
;
   mov   Factorial, WORD PTR InvPiFg33 + 2
   mov   one, Factorial
   mov   Cos, Factorial          ; Cos = 1
   mov   Sin, Num                ; Sin = Num

LoopIntSinCos:
   TaylorTerm                    ; ax=Term = Num * (Num/2) * (Num/3) * . . .
   sub   Cos, Term               ; Cos = 1 - Num*(Num/2) + (Num**4)/4! - . . .
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm
   sub   Sin, Term               ; Sin = Num - Num*(Num/2)*(Num/3) + (Num**5)/5! - . . .
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm
   add   Cos, Term
   cmp   Term, WORD PTR TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm                    ; Term = Num * (x/2) * (x/3) * . . .
   add   Sin, Term
   cmp   Term, WORD PTR TrigLimit
   jnbe  LoopIntSinCos

ExitIntSinCos:
   xor   ax, ax
   mov   cx, ax
   cmp   Cos, WORD PTR InvPiFg33 + 2
   jb    CosDivide               ; Cos < 1.0

   inc   cx                      ; Cos == 1.0
   jmp   StoreCos

CosDivide:
   mov   dx, Cos
   div   WORD PTR InvPiFg33 + 2

StoreCos:
   mov   Cos, ax                 ; cx:Cos

   xor   ax, ax
   mov   bx, ax
   cmp   Sin, WORD PTR InvPiFg33 + 2
   jb    SinDivide               ; Sin < 1.0

   inc   bx                      ; Sin == 1.0
   jmp   StoreSin

SinDivide:
   mov   dx, Sin
   div   WORD PTR InvPiFg33 + 2

StoreSin:
   mov   Sin, ax                 ; bx:Sin
; CJLT again. New tests are needed to correct signs and exchange sin/cos values
   mov   ax, a
   inc   al     ;forces bit 1 to xor of previous bits 0 and 1
   test  al, 2
   jz    ChkNegCos

   xchg  cx, bx
   xchg  Sin, Cos
;   mov   ax, SinNeg    commented out by CJLT. This was a bug.
;   xchg  ax, CosNeg
;   mov   CosNeg, ax    and this was meant to be  mov  SinNeg,ax

ChkNegCos:
   inc   al     ;forces bit 2 to xor of original bits 1 and 2
   test  al, 4
   jz    ChkNegSin

   not   Cos    ;negate cos if quadrant 2 or 3
   not   cx
   add   Cos, 1
   adc   cx, 0

ChkNegSin:
   inc   al
   inc   al     ;forces bit 3 to xor of original bits 2 and 3
   test  al, 8
   jz    CorrectQuad

   not   Sin
   not   bx
   add   Sin, 1
   adc   bx, 0

CorrectQuad:

CosPolarized:
   mov   dx, bx
   mov   bx, CosAddr
   mov   WORD PTR [bx], Cos
   mov   WORD PTR [bx+2], cx

SinPolarized:
   mov   bx, SinAddr
   mov   WORD PTR [bx], Sin
   mov   WORD PTR [bx+2], dx
   ret
SinCos086      ENDP


PUBLIC ArcTan086
;
;Used to calculate logs of complex numbers, since log(R,theta)=log(R)+i.theta
; in polar coordinates.
;
;In C we need the prototype:
;long ArcTan086(long, long)

;The parameters are x and y, the returned value arctan(y/x) in the range 0..2pi.
ArcTan086     PROC  uses si di, x1:word, x2:word, y1:word, y2:word
      xor   ax, ax ;ax=0
      mov   si, x1 ;Lo
      mov   bx, x2 ;Hi
      or    bx, bx ;Sign set ?
      jns   xplus
      inc   ax
      not   si
      not   bx
      add   bx, 1
      adc   si, 0       ;We have abs(x) now
 xplus:
      mov   di, y1 ;Lo
      mov   cx, y2 ;Hi
      or    cx, cx ;Sign set?
      jns   yplus
      inc   ax
      inc   ax     ;Set bit 1 of ax
      shl   ax, 1  ; and shift it all left
      not  di
      not   cx
      add   di, 1
      adc   cx, 0  ;We have abs(y) now
yplus:
      cmp   bx, cx      ;y>x?
      jl    no_xchg
      jg    xchg_xy
      cmp   si, di      ;Hi words zero. What about lo words?
      jle   no_xchg
xchg_xy:                ;Exchange them
      inc   ax          ;Flag the exchange
      xchg  si, di
      xchg  bx, cx
no_xchg:
      mov   SinNeg, ax  ;Remember signs of x and y, and whether exchanged
      or    cx, cx      ;y<x now, but is y>=1.0 ?
      jz    ynorm       ;no, so continue
normy:                  ;yes, normalise by reducing y to 16 bits max.
      rcr   cx, 1       ; (We don't really lose any precision)
      rcr   di, 1
      clc
      rcr   bx, 1
      rcr   si, 1
      or    cx, cx
      jnz   normy
ynorm:

      ret
ArcTan086       ENDP

;
;There now follows Chris Lusby Taylor's novel (such modesty!) method
;of calculating exp(x). Originally, I was going to do it by decomposing x
;into a sum of terms of the form:
;
;  th            -i
; i   term=ln(1+2  )
;                                                                        -i
;If x>term[i] we subtract term[i] from x and multiply the answer by 1 + 2
;
;We can multiply by that factor by shifting and adding. Neat eh!
;
;Unfortunately, however, for 16 bit accuracy, that method turns out to be
;slower than what follows, which is also novel. Here, I first divide x not
;by ln(2) but ln(2)/16 so that we can look up an initial answer in a table of
;2^(n/16). This leaves the remainder so small that the polynomial Taylor series
;converges in just 1+x+x^2/2 which is calculated as 1+x(1+x/2).
;
_e2xLT   PROC           ;argument in dx.ax (bitshift=16 is hard coded here)
   or    dx, dx
   jns   CalcExpLT

   not   ax             ; if negative, calc exp(abs(x))
   not   dx
   add   ax, 1
   adc   dx, 0

CalcExpLT:
         shl   ax, 1
      rcl   dx, 1
      shl   ax, 1
      rcl   dx, 1
      shl   ax, 1
      rcl   dx, 1               ;x is now in fg19 form for accuracy
                                ; so, relative to fg16, we have:
   div   Ln2Fg15                ; 8x==(a * Ln(2)/2) + Rem
                                ;  x=(a * Ln(2)/16) + Rem/8
                                ;so exp(x)=exp((a * Ln(2)/16) + Rem/8)
                                ;         =exp(a/16 * Ln(2)) * exp(Rem/8)
                                ;         =2^(a/16) * exp(Rem)
                                ;a mod 16 will give us an initial estimate
   mov  cl, 4
   mov  di, ax                  ;Remember original
   shr  ax, cl
   mov   a, ax                  ;a/16 will give us a bit shift
   mov  ax, di
   and  ax, 0000fh              ;a mod 16
   shl  ax, 1                   ;used as an index into 2 byte per element Table
   mov  di, ax
   dec  cx                      ;3, please
   add   dx, 4                  ;to control rounding up/down
   shr   dx, cl                 ;Num=Rem/8 (convert back to fg16)
                                ;
   mov   ax, dx
   shr   ax, 1                  ;Num/2  fg 16
   inc   ax                     ;rounding control
   stc
   rcr   ax, 1                  ;1+Num/2   fg15
   mul   dx                     ;dx:ax=Num(1+Num/2) fg31, so dx alone is fg15
   shl   ax, 1                  ;more rounding control
   adc   dx, 8000H              ;dx=1+Num(1+Num/2) fg15
   mov   ax, Table[di]          ;Get table entry fg15
   mul   dx                     ;Only two multiplys used!  fg14, now (15+15-16)
   shl   ax, 1
   rcl   dx, 1                  ;fg15
   mov   e,  dx
   ret                          ; return e=e**x * (2**15), 1 < e**x < 2
_e2xLT   ENDP                   ;        a= bitshift needed



Exp086    PROC     uses si di, LoNum:WORD, HiNum:WORD
   mov   ax, LoNum
   mov   dx, HiNum
;   mov   overflow, 0

    call  _e2xLT        ;Use Chris Lusby Taylor's e2x

   cmp   a, 15          ;CJLT - was 16
   jae   Overflow

;   cmp   expSign, 0
;   jnz   NegNumber
   cmp   HiNum, 0       ;CJLT - we don't really need expSign
   jl   NegNumber

   mov   ax, e
   mov   dx, ax
   inc   a
   mov   cx, 16
   sub   cx, a
   shr   dx, cl
   mov   cx, a
   shl   ax, cl
   jmp   ExitExp086

Overflow:
   xor   ax, ax
   xor   dx, dx
   mov   overflow, 1
   jmp   ExitExp086

NegNumber:
   cmp   e, 8000h
   jne   DivideE

   mov   ax, e
   dec   a
   jmp   ShiftE

DivideE:
   xor   ax, ax
   mov   dx, ax
   stc
   rcr   dx, 1
   div   e

ShiftE:
   xor   dx, dx
   mov   cx, a
   shr   ax, cl

ExitExp086:
   ret
Exp086    ENDP



SinhCosh086    PROC     uses si di, LoNum:WORD, HiNum:WORD, SinhAddr:WORD, \
                                   CoshAddr:WORD
   mov   ax, LoNum
   mov   dx, HiNum

   call  _e2xLT         ;calculate exp(|x|) fg 15
                        ;answer is e*2^a
   cmp   e, 8000h       ;e==1 ?
   jne   InvertE        ; e > 1, so we can invert it.

   mov   dx, 1
   xor   ax, ax
   cmp   a, 0
   jne   Shiftone

   mov   e, ax
   mov   cx, ax
   jmp   ChkSinhSign

Shiftone:
   mov   cx, a
   shl   dx, cl
   dec   cx
   shr   e, cl
   shr   dx, 1
   shr   e, 1
   mov   cx, dx
   sub   ax, e
   sbb   dx, 0
   xchg  ax, e
   xchg  dx, cx
   jmp   ChkSinhSign

InvertE:
   xor   ax, ax               ; calc 1/e
   mov   dx, 8000h
   div   e

   mov   Inve, ax

ShiftE:
   mov   cx, a
   shr   Inve, cl
   inc   cl
   mov   dx, e
   shl   e, cl
   neg   cl
   add   cl, 16
   shr   dx, cl
   mov   cx, dx               ; cx:e == e**Exp

   mov   ax, e                ; dx:e == e**Exp
   add   ax, Inve
   adc   dx, 0
   shr   dx, 1
   rcr   ax, 1                ; cosh(Num) = (e**Exp + 1/e**Exp) / 2

   sub   e, Inve
   sbb   cx, 0
   sar   cx, 1
   rcr   e, 1

ChkSinhSign:
   or    HiNum, 0
   jns   StoreHyperbolics

   not   e
   not   cx
   add   e, 1
   adc   cx, 0

StoreHyperbolics:
   mov   bx, CoshAddr
   mov   WORD PTR [bx], ax
   mov   WORD PTR [bx+2], dx

   mov   bx, SinhAddr
   mov   WORD PTR [bx], e
   mov   WORD PTR [bx+2], cx

   ret
SinhCosh086    ENDP



Log086   PROC     uses si di, LoNum:WORD, HiNum:WORD, Fudge:WORD
LOCAL Exp:WORD   ; not used, Accum:WORD, LoAns:WORD, HiAns:WORD
;NOTE: CJLT's method does not need LoAns, HiAns, but he hasn't yet
;taken them out
      xor   bx, bx
      mov   cx, Fudge
      mov   ax, LoNum
      mov   dx, HiNum
;      mov   overflow, 0

      or    dx, dx
      js    Overflow
      jnz   ShiftUp

      or    ax, ax
      jnz   ShiftUp

   Overflow:
      mov   overflow, 1
      jmp   ExitLog086

   ShiftUp:
      inc   cx                      ; cx = Exp
      shl   ax, 1
      rcl   dx, 1
      or    dx, dx
      jns   ShiftUp             ;shift until dx in fg15 form

      neg   cx
      add   cx, 31
      mov   Exp, cx
;CJLT method starts here. We try to reduce x to make it very close to 1
;store LoAns in bx for now (fg 16; initially 0)
      mov  cl,2         ;shift count
redoterm2:
      cmp  dx, 0AAABH   ;x > 4/3 ?
      jb  doterm3
      mov  ax, dx
      shr  ax, cl
      sub  dx, ax       ;x:=x(1-1/4)
      add  bx, 49a5H    ;ln(4/3) fg 15
      jmp  redoterm2
 doterm3:
      inc  cl           ;count=3
redoterm3:
      cmp  dx, 9249H    ;x > 8/7 ?
      jb  doterm4
      mov  ax, dx
      shr  ax, cl
      sub  dx, ax       ;x:=x(1-1/8)
      add  bx, 222eH    ;ln(8/7)
      jmp  redoterm3
 doterm4:
      inc  cl           ;count=4
      cmp  dx, 8889H    ;x > 16/15 ?
      jb  skipterm4
      mov  ax, dx
      shr  ax, cl
      sub  dx, ax       ;x:=x(1-1/16)
      add  bx, 1085h    ;ln(16/15)
;No need to retry term4 as error is acceptably low if we ignore it
skipterm4:
;Now we have reduced x to the range 1 <=x <1.072
;
;Now we continue with the conventional series, but x is so small we
;can ignore all terms except the first!
;i.e.:
;ln(x)=2(x-1)/(x+1)
      sub   dx, 8000h           ; dx= x-1, fg 15
      mov   cx, dx
      stc
      rcr   cx, 1               ; cx = 1 + (x-1)/2   fg 15
                                ;   = 1+x            fg 14
      mov   ax,4000H            ;rounding control (Trust me)
      div   cx                  ;ax=ln(x)
      add   bx, ax              ;so add it to the rest of the Ans. No carry
   MultExp:
      mov   cx, Exp
      mov   ax, Ln2Fg16
      or    cx, cx
      js    SubFromAns

      mul   cx                      ; cx = Exp * Lg2Fg16, fg 16
      add   ax, bx              ;add bx part of answer
      adc   dx, 0
      jmp   ExitLog086

   SubFromAns:
      inc   bx          ;Somewhat artificial, but we need to add 1 somewhere
      neg   cx
      mul   cx
   not   ax
      not   dx
      add   ax, bx
      adc   dx, 0

   ExitLog086:
      ret
Log086   ENDP


END
