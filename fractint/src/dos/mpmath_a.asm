TITLE mpmath_a.asm (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
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


.data

extrn cpu:WORD


MP STRUC
   Exp   DW    0
   Mant  DD    0
MP ENDS


PUBLIC MPOverflow
PUBLIC Ans
MPOverflow  DW        0

Ans         MP       <?>
Double      DQ        ?


.code

.8086

fg2MP086    PROC     x:DWORD, fg:WORD
   mov   ax, WORD PTR [x]
   mov   dx, WORD PTR [x+2]
   mov   cx, ax
   or    cx, dx
   jz    ExitFg2MP

   mov   cx, 1 SHL 14 + 30
   sub   cx, fg

   or    dx, dx
   jns   BitScanRight

   or    ch, 80h
   not   ax
   not   dx
   add   ax, 1
   adc   dx, 0

BitScanRight:
   shl   ax, 1
   rcl   dx, 1
   dec   cx
   or    dx, dx
   jns   BitScanRight

ExitFg2MP:
   mov   Ans.Exp, cx
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP086    ENDP



MPcmp086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
LOCAL Rev:WORD, Flag:WORD
   mov   Rev, 0
   mov   Flag, 0
   mov   ax, xExp
   mov   dx, WORD PTR [xMant]
   mov   si, WORD PTR [xMant+2]
   mov   bx, yExp
   mov   cx, WORD PTR [yMant]
   mov   di, WORD PTR [yMant+2]
   or    ax, ax
   jns   AtLeastOnePos

   or    bx, bx
   jns   AtLeastOnePos

   mov   Rev, 1
   and   ah, 7fh
   and   bh, 7fh

AtLeastOnePos:
   cmp   ax, bx
   jle   Cmp1

   mov   Flag, 1
   jmp   ChkRev

Cmp1:
   je    Cmp2

   mov   Flag, -1
   jmp   ChkRev

Cmp2:
   cmp   si, di
   jbe   Cmp3

   mov   Flag, 1
   jmp   ChkRev

Cmp3:
   je    Cmp4

   mov   Flag, -1
   jmp   ChkRev

Cmp4:
   cmp   dx, cx
   jbe   Cmp5

   mov   Flag, 1
   jmp   ChkRev

Cmp5:
   je    ChkRev

   mov   Flag, -1

ChkRev:
        or    Rev, 0
   jz    ExitCmp

   neg   Flag

ExitCmp:
   mov   ax, Flag
   ret
MPcmp086    ENDP






MPmul086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    WORD PTR [xMant+2], 0
   jz    ZeroAns
   or    WORD PTR [yMant+2], 0
   jz    ZeroAns

   mov   MPOverflow, 1

ZeroAns:
   xor   ax, ax
   xor   dx, dx
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   si, WORD PTR [xMant+2]
   mov   bx, WORD PTR [xMant]
   mov   di, WORD PTR [yMant+2]
   mov   cx, WORD PTR [yMant]

   mov   ax, si
   or    ax, bx
   jz    ZeroAns

   mov   ax, di
   or    ax, cx
   jz    ZeroAns

   mov   ax, cx
   mul   bx
   push  dx

   mov   ax, cx
   mul   si
   push  ax
   push  dx

   mov   ax, bx
   mul   di
   push  ax
   push  dx

   mov   ax, si
   mul   di
   pop   bx
   pop   cx
   pop   si
   pop   di

   add   ax, bx
   adc   dx, 0
   pop   bx
   add   di, bx
   adc   ax, 0
   adc   dx, 0
   add   di, cx
   adc   ax, si
   adc   dx, 0

   or    dx, dx
   js    StoreMant

   shl   di, 1
   rcl   ax, 1
   rcl   dx, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPmul086    ENDP



d2MP086     PROC     uses si di, x:QWORD
   mov   dx, WORD PTR [x+6]
   mov   ax, WORD PTR [x+4]
   mov   bx, WORD PTR [x+2]
   mov   cx, WORD PTR [x]
   mov   si, dx
   shl   si, 1
   or    si, bx
   or    si, ax
   or    si, dx
   or    si, cx
   jnz   NonZero

   xor   ax, ax
   xor   dx, dx
   jmp   StoreAns

NonZero:
   mov   si, dx
   shl   si, 1
   pushf
   mov   cl, 4
   shr   si, cl
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   mov   di, ax                           ; shl dx:ax:bx 12 bits
   mov   cl, 12
   shl   dx, cl
   shl   ax, cl
   mov   cl, 4
   shr   di, cl
   shr   bx, cl
   or    dx, di
   or    ax, bx
   stc
   rcr   dx, 1
   rcr   ax, 1

StoreAns:
   mov   Ans.Exp, si
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
d2MP086     ENDP



MP2d086     PROC     uses si di, xExp:WORD, xMant:DWORD
   sub   xExp, (1 SHL 14) - (1 SHL 10)
   jo    Overflow

   mov   bx, xExp
   and   bx, 0111100000000000b
   jz    InRangeOfDouble

Overflow:
   mov   MPOverflow, 1
   xor   ax, ax
   xor   dx, dx
   xor   bx, bx
   jmp   StoreAns

InRangeOfDouble:
   mov   si, xExp
   mov   ax, si
   mov   cl, 5
   shl   si, cl
   shl   ax, 1
   rcr   si, 1

   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]
   shl   ax, 1
   rcl   dx, 1

   mov   bx, ax
   mov   di, dx
   mov   cl, 12
   shr   dx, cl
   shr   ax, cl
   mov   cl, 4
   shl   bx, cl
   shl   di, cl
   or    ax, di
   or    dx, si

StoreAns:
   mov   WORD PTR Double+6, dx
   mov   WORD PTR Double+4, ax
   mov   WORD PTR Double+2, bx
   xor   bx, bx
   mov   WORD PTR Double, bx

   lea   ax, Double
   mov   dx, ds
   ret
MP2d086     ENDP




MPadd086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, xExp
   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]

   mov   di, yExp

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  dx, WORD PTR [yMant+2]
   xchg  ax, WORD PTR [yMant]

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jl    ChkSixteen
   jmp   StoreAns

ChkSixteen:
   cmp   cx, 16
   jl    SixteenBitShift

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SameMag

SixteenBitShift:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SameMag:
   add   ax, WORD PTR [yMant]
   adc   dx, WORD PTR [yMant+2]
   jc    ShiftCarry
   jmp   StoreAns

ShiftCarry:
   rcr   dx, 1
   rcr   ax, 1
   add   si, 1
   jo    Overflow
   jmp   StoreAns

Overflow:
   mov   MPOverflow, 1

ZeroAns:
   xor   si, si
   xor   ax, ax
   xor   dx, dx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   dx, WORD PTR [yMant+2]
   jg    SubtractNumbers
   jne   SwapNumbers

   cmp   ax, WORD PTR [yMant]
   jg    SubtractNumbers
   je    ZeroAns

SwapNumbers:
   xor   si, 8000h
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   cmp   cx, 16
   jl    SixteenBitShift2

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SubtractNumbers

SixteenBitShift2:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SubtractNumbers:
   sub   ax, WORD PTR [yMant]
   sbb   dx, WORD PTR [yMant+2]

BitScanRight:
   or    dx, dx
   js    StoreAns

   shl   ax, 1
   rcl   dx, 1
   sub   si, 1
   jno   BitScanRight
   jmp   Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPadd086    ENDP



MPdiv086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   sub   ax, bx
   jno   NoOverflow

Overflow:
   mov   MPOverflow, 1

ZeroAns:
   xor   ax, ax
   xor   dx, dx
   mov   Ans.Exp, dx
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]
   or    dx, dx
   jz    ZeroAns

   mov   cx, WORD PTR [yMant+2]
   mov   bx, WORD PTR [yMant]
   or    cx, cx
   jz    Overflow

   cmp   dx, cx
   jl    Divide

   shr   dx, 1
   rcr   ax, 1
   add   Ans.Exp, 1
   jo    Overflow

Divide:
   div   cx
   mov   si, dx
   mov   dx, bx
   mov   bx, ax
   mul   dx
   xor   di, di
   cmp   dx, si
   jnc   RemReallyNeg

   xchg  ax, di
   xchg  dx, si
   sub   ax, di
   sbb   dx, si

   shr   dx, 1
   rcr   ax, 1
   div   cx
   mov   dx, bx
   shl   ax, 1
   adc   dx, 0
   jmp   StoreMant

RemReallyNeg:
   sub   ax, di
   sbb   dx, si

   shr   dx, 1
   rcr   ax, 1
   div   cx
   mov   dx, bx
   mov   bx, ax
   xor   ax, ax
   xor   cx, cx
   shl   bx, 1
   rcl   cx, 1
   sub   ax, bx
   sbb   dx, cx
   jno   StoreMant

   shl   ax, 1
   rcl   dx, 1
   dec   Ans.Exp

StoreMant:
   mov   WORD PTR Ans.Mant, ax
   mov   WORD PTR Ans.Mant+2, dx
   lea   ax, Ans
   mov   dx, ds
   ret
MPdiv086    ENDP


MPcmp386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, 0
   mov   di, si
.386
   mov   ax, xExp
   mov   edx, xMant
   mov   bx, yExp
   mov   ecx, yMant
   or    ax, ax
   jns   AtLeastOnePos

   or    bx, bx
   jns   AtLeastOnePos

   mov   si, 1
   and   ah, 7fh
   and   bh, 7fh

AtLeastOnePos:
   cmp   ax, bx
   jle   Cmp1

   mov   di, 1
   jmp   ChkRev

Cmp1:
   je    Cmp2

   mov   di, -1
   jmp   ChkRev

Cmp2:
   cmp   edx, ecx
   jbe   Cmp3

   mov   di, 1
   jmp   ChkRev

Cmp3:
   je    ChkRev

   mov   di, -1

ChkRev:
        or    si, si
   jz    ExitCmp

   neg   di

ExitCmp:
   mov   ax, di
.8086
   ret
MPcmp386    ENDP



d2MP386     PROC     uses si di, x:QWORD
   mov   si, WORD PTR [x+6]
.386
   mov   edx, DWORD PTR [x+4]
   mov   eax, DWORD PTR [x]

   mov   ebx, edx
   shl   ebx, 1
   or    ebx, eax
   jnz   NonZero

   xor   si, si
   xor   edx, edx
   jmp   StoreAns

NonZero:
   shl   si, 1
   pushf
   shr   si, 4
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   shld  edx, eax, 12
   stc
   rcr   edx, 1

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, edx

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
d2MP386     ENDP




MP2d386     PROC     uses si di, xExp:WORD, xMant:DWORD
   sub   xExp, (1 SHL 14) - (1 SHL 10)
.386
   jo    Overflow

   mov   bx, xExp
   and   bx, 0111100000000000b
   jz    InRangeOfDouble

Overflow:
   mov   MPOverflow, 1
   xor   eax, eax
   xor   edx, edx
   jmp   StoreAns

InRangeOfDouble:
   mov   bx, xExp
   mov   ax, bx
   shl   bx, 5
   shl   ax, 1
   rcr   bx, 1
   shr   bx, 4

   mov   edx, xMant
   shl   edx, 1
   xor   eax, eax
   shrd  eax, edx, 12
   shrd  edx, ebx, 12

StoreAns:
   mov   DWORD PTR Double+4, edx
   mov   DWORD PTR Double, eax

   lea   ax, Double
   mov   dx, ds
.8086
   ret
MP2d386     ENDP



MPmul386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    WORD PTR [xMant+2], 0
   jz    ZeroAns
   or    WORD PTR [yMant+2], 0
   jz    ZeroAns

   mov   MPOverflow, 1

ZeroAns:
   xor   edx, edx
   mov   Ans.Exp, dx
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   eax, xMant
   mov   edx, yMant
   or    eax, eax
   jz    ZeroAns

   or    edx, edx
   jz    ZeroAns

   mul   edx

   or    edx, edx
   js    StoreMant

   shld  edx, eax, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   Ans.Mant, edx
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPmul386    ENDP



MPadd386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   si, xExp
.386
   mov   eax, xMant

   mov   di, yExp
   mov   edx, yMant

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  eax, edx

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SameMag:
   add   eax, edx
   jnc   StoreAns

   rcr   eax, 1
   add   si, 1
   jno   StoreAns

Overflow:
   mov   MPOverflow, 1

ZeroAns:
   xor   si, si
   xor   edx, edx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   eax, edx
   jg    SubtractNumbers
   je    ZeroAns

   xor   si, 8000h
   xchg  eax, edx
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  eax, edx
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SubtractNumbers:
   sub   eax, edx
   bsr   ecx, eax
   neg   cx
   add   cx, 31
   shl   eax, cl
   sub   si, cx
   jo    Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, eax

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPadd386    ENDP





MPdiv386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   sub   ax, bx
   jno   NoOverflow

Overflow:
   mov   MPOverflow, 1

ZeroAns:
   xor   eax, eax
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   xor   eax, eax
   mov   edx, xMant
   mov   ecx, yMant
   or    edx, edx
   jz    ZeroAns

   or    ecx, ecx
   jz    Overflow

   cmp   edx, ecx
   jl    Divide

   shr   edx, 1
   rcr   eax, 1
   add   Ans.Exp, 1
   jo    Overflow

Divide:
   div   ecx

StoreMant:
   mov   Ans.Mant, eax
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPdiv386    ENDP


fg2MP386    PROC     x:DWORD, fg:WORD
   mov   bx, 1 SHL 14 + 30
   sub   bx, fg
.386
   mov   edx, x

   or    edx, edx
   jnz   ChkNegMP

   mov   bx, dx
   jmp   StoreAns

ChkNegMP:
   jns   BitScanRight

   or    bh, 80h
   neg   edx

BitScanRight:
   bsr   ecx, edx
   neg   cx
   add   cx, 31
   sub   bx, cx
   shl   edx, cl

StoreAns:
   mov   Ans.Exp, bx
   mov   Ans.Mant, edx
.8086
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP386    ENDP



PUBLIC MPcmp, MPmul, MPadd, MPdiv, MP2d, d2MP, fg2MP

fg2MP:
   cmp   cpu, 386
   jae   Use386fg2MP
   jmp   fg2MP086

Use386fg2MP:
   jmp   fg2MP386

MPcmp:
   cmp   cpu, 386
   jae   Use386cmp
   jmp   MPcmp086

Use386cmp:
   jmp   MPcmp386

MPmul:
   cmp   cpu, 386
   jae   Use386mul
   jmp   MPmul086

Use386mul:
   jmp   MPmul386

d2MP:
   cmp   cpu, 386
   jae   Use386d2MP
   jmp   d2MP086

Use386d2MP:
   jmp   d2MP386

MPadd:
   cmp   cpu, 386
   jae   Use386add
   jmp   MPadd086

Use386add:
   jmp   MPadd386

MPdiv:
   cmp   cpu, 386
   jae   Use386div
   jmp   MPdiv086

Use386div:
   jmp   MPdiv386

MP2d:
   cmp   cpu, 386
   jae   Use386MP2d
   jmp   MP2d086

Use386MP2d:
   jmp   MP2d386


END

