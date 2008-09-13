TITLE fpu387.asm (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
SUBTTL All rights reserved.
;
;  Code may be used in any program provided the author is credited
;    either during program execution or in the documentation.  Source
;    code may be distributed only in combination with public domain or
;    shareware source code.  Source code may be modified provided the
;    copyright notice and this message is left unchanged and all
;    modifications are clearly documented.
;
;    I would appreciate a copy of any work which incorporates this code,
;    however this is optional.
;
;    Mark C. Peterson
;    405-C Queen St., Suite #181
;    Southington, CT 06489
;    (203) 276-9721
;
;  Note: Remark statements following floating point commands generally indicate
;     the FPU stack contents after the command is completed.
;
;  References:
;     80386/80286 Assembly Language Programming
;        by William H. Murray, III and Chris H. Pappas
;        Published by Osborne McGraw-Hill, 1986
;
;
;



IFDEF ??version
MASM51
QUIRKS
ENDIF

.model medium, c

.code

.386
.387

_Loaded387sincos   PROC
   fsincos
   fwait
   ret
_Loaded387sincos   ENDP


FPU387sin      PROC     x:word, sin:word
   mov   bx, x
   fld   QWORD PTR [bx]    ; x
   fsin                    ; sin
   mov   bx, sin
   fstp  QWORD PTR [bx]    ; <empty>
   ret
FPU387sin      ENDP


FPU387cos      PROC     x:word, cos:word
   mov   bx, x
   fld   QWORD PTR [bx]    ; x
   fcos                    ; cos
   mov   bx, cos
   fstp  QWORD PTR [bx]    ; <empty>
   ret
FPU387cos      ENDP


FPUaptan387    PROC     x:word, y:word, z:word
   mov   bx, y
   fld   QWORD PTR [bx]    ; y
   mov   bx, x
   fld   QWORD PTR [bx]    ; x, y
   fpatan                  ; ArtTan
   mov   bx, z
   fstp  QWORD PTR [bx]    ; <empty>
   ret
FPUaptan387    ENDP


FPUcplxexp387  PROC     x:word, z:word
   mov   bx, x
   fld   QWORD PTR [bx+8]  ; x.y
   fsincos                 ; cos, sin
   fldln2                  ; ln2, cos, sin
   fdivr QWORD PTR [bx]    ; x.x/ln2, cos, sin
   fld1                    ; 1, x.x/ln2, cos, sin
   fld   st(1)             ; x.x/ln2, 1, x.x/ln2, cos, sin
   fprem                   ; prem, 1, x.x/ln2, cos, sin
   f2xm1                   ; e**prem-1, 1, x.x/ln2, cos, sin
   fadd                    ; e**prem, x.x/ln2, cos, sin
   fscale                  ; e**x.x, x.x/ln2, cos, sin
   fstp  st(1)             ; e**x.x, cos, sin
   fmul  st(2), st         ; e**x.x, cos, z.y
   fmul                    ; z.x, z.y
   mov   bx, z
   fstp  QWORD PTR [bx]    ; z.y
   fstp  QWORD PTR [bx+8]  ; <empty>
   ret
FPUcplxexp387  ENDP


.data

extrn overflow:WORD, TrigLimit:DWORD

PiFg13         dw       6487h
InvPiFg17      dw       0a2f9h
InvPiFg33      dd       0a2f9836eh
InvPiFg65      dq       0a2f9836e4e44152ah
InvPiFg16      dw       517ch
Ln2Fg16        dw       0b172h
Ln2Fg32        dd       0b17217f7h
One            dd       ?
ExpSign        dd       ?
Exp            dd       ?
SinNeg         dd       ?
CosNeg         dd       ?


.code


TaylorTerm  MACRO
LOCAL Ratio
   add   Factorial, One
   jnc   SHORT Ratio

   rcr   Factorial, 1
   shr   Num, 1
   shr   One, 1

Ratio:
   mul   Num
   div   Factorial
ENDM



Term           equ      <eax>
HiTerm         equ      <edx>
Num            equ      <ebx>
Factorial      equ      <ecx>
Sin            equ      <esi>
Cos            equ      <edi>
e              equ      <esi>
Inve           equ      <edi>
LoPtr          equ      <DWORD PTR [bx]>
HiPtr          equ      <DWORD PTR [bx+2]>



_sincos386   PROC
   xor   Factorial, Factorial
   mov   SinNeg, Factorial
   mov   CosNeg, Factorial
   mov   Exp, Factorial
   or    HiTerm, HiTerm
   jns   AnglePositive

   not   Term
   not   HiTerm
   add   Term, 1
   adc   HiTerm, Factorial
   mov   SinNeg, 1

AnglePositive:
   mov   Sin, Term
   mov   Cos, HiTerm
   mul   DWORD PTR InvPiFg65
   mov   Num, HiTerm
   mov   Term, Cos
   mul   DWORD PTR InvPiFg65
   add   Num, Term
   adc   Factorial, HiTerm
   mov   Term, Sin
   mul   InvPiFg33
   add   Num, Term
   adc   Factorial, HiTerm
   mov   Term, Cos
   mul   InvPiFg33
   add   Term, Factorial
   adc   HiTerm, 0

   and   HiTerm, 3
   mov   Exp, HiTerm

   mov   Num, Term
   mov   Factorial, InvPiFg33
   mov   One, Factorial
   mov   Cos, Factorial          ; Cos = 1
   mov   Sin, Num                  ; Sin = Num

LoopIntSinCos:
   TaylorTerm                    ; Term = Num * (x/2) * (x/3) * (x/4) * . . .
   sub   Cos, Term               ; Cos = 1 - Num*(x/2) + (x**4)/4! - . . .
   cmp   Term, TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm
   sub   Sin, Term               ; Sin = Num - Num*(x/2)*(x/3) + (x**5)/5! - . . .
   cmp   Term, TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm
   add   Cos, Term
   cmp   Term, TrigLimit
   jbe   SHORT ExitIntSinCos

   TaylorTerm                    ; Term = Num * (x/2) * (x/3) * . . .
   add   Sin, Term
   cmp   Term, TrigLimit
   jnbe  LoopIntSinCos

ExitIntSinCos:
   xor   Term, Term
   mov   Factorial, Term
   cmp   Cos, InvPiFg33
   jb    CosDivide               ; Cos < 1.0

   inc   Factorial                      ; Cos == 1.0
   jmp   StoreCos

CosDivide:
   mov   HiTerm, Cos
   div   InvPiFg33

StoreCos:
   mov   Cos, Term                 ; Factorial:Cos

   xor   Term, Term
   mov   Num, Term
   cmp   Sin, InvPiFg33
   jb    SinDivide               ; Sin < 1.0

   inc   Num                      ; Sin == 1.0
   jmp   StoreSin

SinDivide:
   mov   HiTerm, Sin
   div   InvPiFg33

StoreSin:
   mov   Sin, Term                 ; Num:Sin

   test  Exp, 1
   jz    ChkNegCos

   xchg  Factorial, Num
   xchg  Sin, Cos
   mov   Term, SinNeg
   xchg  Term, CosNeg
   mov   CosNeg, Term

ChkNegCos:
   mov   Term, Exp
   shr   Term, 1
   mov   Term, 0
   rcr   Term, 1
   xor   Term, Exp
   jz    ChkNegSin

   xor   CosNeg, 1

ChkNegSin:
   test  Exp, 2
   jz    CorrectQuad

   xor   SinNeg, 1

CorrectQuad:
   ret
_sincos386     ENDP


SinCos386   PROC     LoNum:DWORD, HiNum:DWORD, SinAddr:DWORD, CosAddr:DWORD
   mov   Term, LoNum
   mov   HiTerm, HiNum

   call  _sincos386

   cmp   CosNeg, 1
   jne   CosPolarized

   not   Cos
   not   Factorial
   add   Cos, 1
   adc   Factorial, 0

CosPolarized:
   mov   HiTerm, Num
   mov   Num, CosAddr
   mov   LoPtr, Cos
   mov   HiPtr, Factorial

   cmp   SinNeg, 1
   jne   SinPolarized

   not   Sin
   not   HiTerm
   add   Sin, 1
   adc   HiTerm, 0

SinPolarized:
   mov   Num, SinAddr
   mov   LoPtr, Sin
   mov   HiPtr, HiTerm
   ret
SinCos386      ENDP



_e2y386   PROC                 ; eTerm =: Num * 2**16, 0 < Num < Ln2
   mov   ExpSign, 0
   or    HiTerm, HiTerm
   jns   CalcExp

   mov   ExpSign, 1
   not   Term
   not   HiTerm
   add   Term, 1
   adc   HiTerm, 0

CalcExp:
   div   Ln2Fg32
   mov   Exp, Term
   mov   Num, HiTerm

   xor   Factorial, Factorial
   stc
   rcr   Factorial, 1
   mov   One, Factorial
   mov   e, Num
   mov   Term, Num
   shr   Num, 1

Loop_e2y386:
   TaylorTerm
   add   e, Term                 ; e = 1 + Num + Num*x/2 + (x**3)/3! + . . .
   cmp   Term, TrigLimit
   jnbe  SHORT Loop_e2y386

ExitIntSinhCosh:
   stc
   rcr   e, 1
   ret                           ; return e**y * (2**32), 1 < e**y < 2
_e2y386   ENDP



Exp386    PROC     LoNum:DWORD, HiNum:DWORD
   mov   Term, LoNum
   mov   HiTerm, HiNum

   call  _e2y386

   cmp   Exp, 32
   jae   Overflow

   cmp   ExpSign, 0
   jnz   NegNumber

   mov   Factorial, Exp
   shld  HiTerm, Term, cl
   shl   Term, cl
   jmp   ExitExp386

Overflow:
   xor   Term, Term
   xor   HiTerm, HiTerm
   mov   overflow, 1
   jmp   ExitExp386

NegNumber:
   cmp   e, 80000000h
   jne   DivideE

   mov   Term, e
   dec   Exp
   jmp   ShiftE

DivideE:
   xor   Term, Term
   mov   HiTerm, Term
   stc
   rcr   HiTerm, 1
   div   e

ShiftE:
   xor   HiTerm, HiTerm
   mov   Factorial, Exp
   shr   Term, cl

ExitExp386:
   ret
Exp386    ENDP



SinhCosh386 PROC  LoNum:DWORD, HiNum:DWORD, SinhAddr:DWORD, CoshAddr:DWORD
   mov   Term, LoNum
   mov   HiTerm, HiNum

   call  _e2y386

   cmp   e, 80000000h
   jne   InvertE              ; e > 1

   mov   HiTerm, 1
   xor   Term, Term
   cmp   Exp, 0
   jne   ShiftOne

   mov   e, Term
   mov   Factorial, Term
   jmp   ChkSinhSign

ShiftOne:
   mov   Factorial, Exp
   shl   HiTerm, cl
   dec   Factorial
   shr   e, cl
   shr   HiTerm, 1
   shr   e, 1
   mov   Factorial, HiTerm
   sub   Term, e
   sbb   HiTerm, 0
   xchg  Term, e
   xchg  HiTerm, Factorial
   jmp   ChkSinhSign

InvertE:
   xor   Term, Term               ; calc 1/e
   mov   HiTerm, 80000000h
   div   e

   mov   Inve, Term

ShiftE:
   mov   Factorial, Exp
   shr   Inve, cl
   inc   cl
   shld  HiTerm, e, cl
   mov   Factorial, HiTerm      ; Factorial:e == e**Exp

   mov   Term, e                ; HiTerm:e == e**Exp
   add   Term, Inve
   adc   HiTerm, 0
   shr   HiTerm, 1
   rcr   Term, 1                ; cosh(Num) = (e**Exp + 1/e**Exp) / 2

   sub   e, Inve
   sbb   Factorial, 0
   sar   Factorial, 1
   rcr   e, 1

ChkSinhSign:
   or    HiNum, 0
   jns   StoreHyperbolics

   not   e
   not   Factorial
   add   e, 1
   adc   Factorial, 0

StoreHyperbolics:
   mov   Num, CoshAddr
   mov   LoPtr, Term
   mov   HiPtr, HiTerm

   mov   Num, SinhAddr
   mov   LoPtr, e
   mov   HiPtr, Factorial

   ret
SinhCosh386    ENDP



END

