   PAGE          ,132

;    Name: PARSERA.ASM
;  Author: Chuck Ebbert  CompuServe [76306,1226]
;                         internet: 76306.1226@compuserve.com

; Fast floating-point routines for Fractint.

;   (c) Copyright 1992-1998 Chuck Ebbert.  All rights reserved.

; This program is an assembler version of the C 'execution engine' part
;    of Mark Peterson's FRACTINT Formula Parser.  Many of the operator
;    functions were copied from Mark's code in the files FPU087.ASM
;    and FPU387.ASM.  The basic operator functions are assembler versions
;    of the code in PARSER.C.  Many 'combined' operator functions were
;    added to the program as well.

; As of 31 Decmember 1993 this is also an in-memory compiler.  The code
;    generator is in PARSERFP.C.  Define the variable COMPILER to
;    build the compiler, otherwise the interpreter will be built.
;    COMPILER must also be #defined in PARSERFP.C to build compiler.

; 5 Dec 1999 bug fix: removed loop unroll because the modified return
;    address was skipping over functions that needed to execute.

; 1 Dec 1998 speed improvements:
;    Functions Clr2, LT2, LodLT2, LTE2, LodLTE2, GT2, LodGT2,
;    LodGTE2, AndClr2, OrClr2, LodLTEAnd2 were modified to alter
;    their return address on the stack, forcing an exit from
;    the evaluation loop.  This allows the code to run
;    faster because it doesn't have to test for loop end.
;    The loop was also unrolled one time for further gains.

; ---------------------------------------------------------------------------

;    This code may be freely distributed and used in non-commercial
;    programs provided the author is credited either during program
;    execution or in the documentation, and this copyright notice
;    is left intact.  Sale of this code, or its use in any commercial
;    product requires permission from the author.  Nominal distribution
;    and handling fees may be charged by shareware and freeware
;    distributors.


;             Date      Init   Change description
;
;           5 Dec 1999   JCO   Removed loop unroll
;           1 Dec 1998   CAE   Speed improvements
;           5 Apr 1998   JCO   Added LoadRealPwr (0,0) domain check
;           6 Mar 1997   TIW   Added if/else support
;           7 Aug 1996   TIW   Added scrnpix constant
;           4 Aug 1996   TIW   Added whitesq variable support
;          30 Jun 1996   TIW   Added floor, ceil, trunc, and round functions
;           7 Mar 1995   TIW   Added PWR (0,0) domain check
;          21 Feb 1995   TIW   Shortened ATanh/ATan for MASM 6 compatibility
;          21 Feb 1995   CAE   Changes ATan and ATanh

;          15 Feb 1995   CAE   Added safety tests to macros.
;                              Changed fStkASin, etc. to work with compiler.
;                              Added fwait to Sto2 function for safety.

;           8 Feb 1995   CAE   Removed transparent3d code.
;                              Added inversion support (compiler untested.)

;           8 Jan 1995   JCO   Added fStkASin, fStkASinh, fStkACos, fStkACosh,
;                              fStkATan, fStkATanh, fStkSqrt, fStkCAbs.

;          31 Dec 1994   JCO   Made changes to keep code in line with C code.
;                              Not necessary, since code isn't called.  Will
;                              make it easier to make it run later.  Added
;                              old <- z to end of fform_per_pixel to match
;                              C code.

;          30 Dec 1993   CAE   Compiler is working
;                              Changed EXIT_OPER -> ret in 3 operator fns
;                              Added safety test for fn size in macros

;          12 Dec 1993   CAE   Compiler additions

;           4 Dec 1993   CAE   SinhCosh function accuracy improved
;                              Added LoadImagAdd/Sub/Mul

;          19 Nov 1993   CAE   Revised macros for compiler mode.

;          10 Nov 1993   CAE   Changed Exp function for more accuracy.

;          06 Nov 93     CAE   Added 'LodRealPwr', 'One', 'ORClr2', 'Sqr3'.
;                              Revised Pwr function to use regs vs. memory.
;                              Changed many functions to 'included' type.

;          31 Oct 93     CAE   Added 'Dbl' function.

;          09 Oct 1993   CAE   Changed SinhCosh to use wider range of 387.
;                              Most FNINITs changed to FINIT.
;                              Loop logic revised slightly.
;                              Separated code from parserfp.c's codeseg.
;                              Added fStkStoClr2, fStkZero and fStkIdent.
;                              New 'pseudo calctype' fn. fFormulaX added.

;          12 Jul 1993   CAE   Moved BadFormula to PARSER.C.


   .386                                ; this only works on a 386
   .387                                ;  with a 387

ifdef ??version
   masm51
   quirks
endif

ARGSZ              equ 16              ; size of complex arg
;;;ARGSZ              equ 32              ; size of hypercomplex arg
CPFX               equ 4               ; size of constarg prefix
CARG               equ CPFX+ARGSZ      ; size of constarg
LASTSQR            equ CARG*4+CPFX     ; offset of lastsqr from start of v
WHITESQ            equ CARG*9+CPFX     ; offset of whitesq from start of v
SCRNPIX            equ CARG*10+CPFX    ; offset of scrnpix from start of v
JCSZ               equ 10              ; size of jump_control structure
; ---------------------------------------------------------------------------
FRAME              MACRO regs          ; build a stack frame
      push         bp
      mov          bp, sp
   IRP             reg, <regs>
      push         reg
      ENDM
   ENDM

UNFRAME            MACRO regs          ; unframe before return
   IRP             reg, <regs>
      pop          reg
      ENDM
      pop          bp
   ENDM

; ---------------------------------------------------------------------------
; Pop a number of scalars from the FPU stack.
; Generate as many 'fcompp' instr.'s as possible.
; Then a 'fstp st(0)' if needed.
POP_STK            MACRO StkPop
   NumToPop        = StkPop SHR 1
   REPT            NumToPop
      fcompp
      ENDM
   NumToPop        = StkPop - ( NumToPop SHL 1 )
   REPT            NumToPop
      fstp         st(0)
      ENDM
   ENDM

; Uncomment the following line to enable compiler code generation.
;COMPILER           EQU 1

; ---------------------------------------------------------------------------
; Generate beginning code for operator fn.
BEGN_OPER          MACRO OperName
   ifndef          COMPILER
;; only align when no compiler
   align           4
   endif

;; always generate public and begin of proc (before fixups)
   public          _fStk&OperName
_fStk&OperName     proc near

   ifdef           COMPILER
;; generate the fixups for compiler
;; size of fn. | 8000h to mark it as an OPER instead of an INCL  CAE 27Dec93
      dw           Size_&OperName OR 8000h
;; near pointer to the start of actual code                      CAE 19Dec93
      dw           offset PARSERA_TEXT:Code_&OperName
;;    addr of fn to include (undefined if Incl_&OperName==255 below)
      dw           IAddr_&OperName
;; offset of x fixup or 255 if none
      db           XFixup_&OperName
;; offset of y fixup or 255 if none
      db           YFixup_&OperName
;; offset of included(called) fn or 255 if none
      db           Incl_&OperName

   endif

;; added label for code begin point                              CAE 25Nov93
Code_&OperName:

   ENDM

; ---------------------------------------------------------------------------
END_OPER           MACRO OperName
; Generate end of operator fn. code.

   ifndef          COMPILER
;; gen a return instr.
      ret
   else

;; gen a jump label
End_&OperName:

;; generate zero for fixups not generated during fn.

   ifndef          Incl_&OperName
;; No included operator. Generate 255 offset, 0 address.          CAE 19Nov93
Incl_&OperName     EQU 255
IAddr_&OperName    EQU 0
   endif

   ifndef          XFixup_&OperName
XFixup_&OperName   EQU 255
   endif

   ifndef          YFixup_&OperName
YFixup_&OperName   EQU 255
   endif

   endif

;; Always gen size of fn (subtract size of header here)
Size_&OperName     EQU $ - Code_&OperName
;; Make sure fn is of legal size                                  CAE 30DEC93
   .errnz          (Size_&OperName GT 127)

;; and end of procedure.
_fStk&OperName     endp
   ENDM

; ---------------------------------------------------------------------------
BEGN_INCL          MACRO OperName
;; Generate beginning code for 'included' operator fn.
;; No fixups allowed in one of these functions.

;; Safety test: generate an equate here so the INCL_OPER          CAE 15Feb95
;;    macro can test to see if this really is includable.
Is_Incl_&OperName  EQU 1

;; Don't bother with align in compiler mode.
   ifndef          COMPILER
   align           4
   endif

;; Generate public (incl fns. can be called directly) and begin of proc.
   public          _fStk&OperName
_fStk&OperName     proc near

   ifdef           COMPILER
;; Size of included fn.  changed to word                          CAE 27Dec93
      dw           Size_&OperName
   endif

;; added label for code begin point                               CAE 25Nov93
Code_&OperName:

   ENDM

; ---------------------------------------------------------------------------
; Generate end of 'included' operator fn. code.
END_INCL           MACRO OperName
   ifndef          COMPILER
;; generate return
      ret
   else

;; generate label for jump to end of fn.
End_&OperName:
   endif

;; always generate actual size of fn. (subtract hdr. size)
   Size_&OperName  EQU $ - Code_&OperName
;; Make sure fn is of legal size                                  CAE 30DEC93
   .errnz          (Size_&OperName GT 127)
;; always generate end-of-proc
_fStk&OperName     endp
   ENDM

; ---------------------------------------------------------------------------
; 'Include' a function inside another one
INCL_OPER          MACRO CallingOper,OperToIncl

;; Make sure the included fn was defined with the BEGN_INCL macro.
   ifndef          Is_Incl_&OperToIncl                         ;  CAE 15Feb95
   .error          "Included function was not defined with BEGN_INCL macro"
   endif

;; Gen equate for offset of include in outer fn.
;; Always generate this to prevent >1 include even when not       CAE 15FEB95
;;    building the compiler.
Incl_&CallingOper  EQU $ - Code_&CallingOper
   ifdef           COMPILER
;; Address of included fn.
IAddr_&CallingOper EQU _fStk&OperToIncl
;; Gen 1 1-byte placeholder for the included fn to make codegen easier
      db           0ffH
   else

;; Generate a call to the included fn.
      call         _fStk&OperToIncl
   endif
   ENDM

; ---------------------------------------------------------------------------
; Exit early from an operator function.
EXIT_OPER          MACRO FnToExit
   ifdef           COMPILER
;; jump to end of operator fn
      jmp          short End_&FnToExit
   else

;; return to caller
      ret
   endif
   ENDM

; ---------------------------------------------------------------------------
; Generate an FPU instruction and a fixup.
; AddrToFix is = X or Y
FIXUP              MACRO OperName, InstrToFix, Addr
   ifdef           COMPILER

;; Generate a fixup as an offset from start of fn.
;; Fixup is two bytes into the instruction, thus the '+ 2'.
;; This may not be true for all instructions.
   ifidni          <Addr>, <X>
XFixup_&OperName   EQU $ - Code_&OperName + 2
   else
;; assume fixup is for y
YFixup_&OperName   EQU $ - Code_&OperName + 2
   endif
;; Generate a load, store or whatever of any convenient value using DS.
      &InstrToFix  QWORD PTR ds:_fLastOp
   else

   ifidni          <Addr>, <X>
;; Gen load of X using SI.
      &InstrToFix  QWORD PTR [si]
   else
;; Assume fixup is for y, use SI+8.
      &InstrToFix  QWORD PTR [si+8]
   endif
   endif

   ENDM

; ---------------------------------------------------------------------------
; Align 4 if no compiler.
PARSALIGN          macro AlignFn
   ifndef          COMPILER
   align           4
   endif
   ENDM

; CAE added macros for common operations Feb 1995

GEN_SQR0           macro
;; square the stack top, don't save magnitude in lastsqr          CAE 15FEB95
      fld          st(0)               ; x x y
      fld          st(0)               ; x x x y
      fmul         st,st(3)            ; xy x x y
      fadd         st,st               ; 2xy x x y
      fxch         st(3)               ; y x x 2xy
      fadd         st(2),st            ; y x x+y 2xy
      fsubp        st(1),st            ; x-y x+y 2xy
      fmulp        st(1),st            ; xx-yy 2xy
      ENDM

GEN_SQRT           macro               ;                           CAE 15Feb95
   ; can use a max of 2 regs
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      fpatan                           ; atan x y
      fdiv         __2_                ; theta=atan/2 x y
      fsincos                          ; cos sin x y
      fxch         st(3)               ; y sin x cos
      fmul         st,st(0)            ; yy sin x cos
      fxch         st(2)               ; x sin yy cos
      fmul         st,st(0)            ; xx sin yy cos
      faddp        st(2),st            ; sin xx+yy cos
      fxch         st(2)               ; cos xx+yy sin
      fxch                             ; xx+yy cos sin
      fsqrt                            ; sqrt(xx+yy) cos sin
      fsqrt                            ; mag=sqrt(sqrt(xx+yy)) cos sin
      fmul         st(2),st            ; mag cos mag*sin
      fmulp        st(1),st            ; mag*cos mag*sin
      ENDM

; ---------------------------------------------------------------------------
; CAE 1 Dec 1998 added macro

ALTER_RET_ADDR     macro
;;;      mov          WORD PTR [sp],offset past_loop
      pop          bx
      push         offset past_loop
      ENDM
; ---------------------------------------------------------------------------
; external functions
   extrn           _invertz2:far

; ---------------------------------------------------------------------------
_DATA              segment word public use16 'DATA'
   extrn           _invert:WORD
   extrn           _maxit:DWORD
   extrn           _inside:WORD
   extrn           _outside:WORD
   extrn           _coloriter:DWORD
   extrn           _kbdcount:WORD      ; keyboard counter
   extrn           _dotmode:WORD
   extrn           __1_:QWORD, _PointFive:QWORD, __2_:QWORD, _infinity:QWORD
   extrn           _LastOp:WORD, _LastInitOp:WORD
   extrn           _InitOpPtr:WORD, _InitStoPtr:WORD, _InitLodPtr:WORD
   extrn           _s:WORD
   extrn           _dy1:DWORD, _dx1:DWORD, _dy0:DWORD, _dx0:DWORD
   extrn           _new:WORD, _old:WORD
   extrn           _overflow:WORD
   extrn           _save_release:WORD
   extrn           _col:WORD, _row:WORD
   extrn           _Arg1:WORD, _Arg2:WORD
   extrn           _pfls:DWORD, _v:DWORD
   extrn           _ldcheck:WORD
   extrn           _jump_index:WORD, _InitJumpIndex:WORD
   extrn           _jump_control:DWORD
   extrn           _delxx:TBYTE
   extrn           _delxx2:TBYTE
   extrn           _delyy:TBYTE
   extrn           _delyy2:TBYTE
   extrn           _xxmin:QWORD
   extrn           _yymax:QWORD
   extrn           _use_grid:WORD
_DATA               ends

; ---------------------------------------------------------------------------

_BSS               segment word public use16 'BSS'
_fLastOp           label DWORD         ; save seg, offset of lastop here
      dd           ?
_PtrToZ            label WORD          ; offset of z
      dw           ?
_BSS               ends

DGROUP             group _DATA,_BSS

; ---------------------------------------------------------------------------
; Operator Functions follow.
; ---------------------------------------------------------------------------

; NOTE: None of these operator functions may change any registers but
;       ax and si.  The exceptions are those functions that update
;       the current values of the 'status' regs as needed.

;  On entry to these functions:
;   FPU stack is used as the evaluation stack.
;         The FPU stack can overflow into memory.  Accuracy is not lost but
;         calculations are slower.
;   es -> DGROUP
;   ds -> parser data
;   cx -> lastop
;   edx == orbit counter (in fFormulaX)
;   di -> stack overflow area, used by push and pull functions and as
;         a temporary storage area
;   bx -> current operator, operand pair
;    [bx] = operator function address, i.e. addr. of current '_fStkXXX'
;    [bx+2] = operand pointer or zero if no operand
;   si = operand pointer (loaded from [bx+2] before call of operator fn.)

; New rules Feb 1993:
;  1. No EXIT_OPER before an INCL_OPER
;     (no jumps can be made past an included function.)
;  2. No included fn may include another, or have any fixups.
;  3. Only one included fn. allowed per 'normal' fn.

; --------------------------------------------------------------------------
   ;  Put this code in PARSERA_TEXT, not PARSERFP_TEXT           CAE 09OCT93
PARSERA_TEXT     segment para public use16 'CODE'
   ;  Non-standard segment register setup.
   assume         es:DGROUP, ds:nothing, cs:PARSERA_TEXT

; --------------------------------------------------------------------------
; Included functions must be before any fns that include them.
; --------------------------------------------------------------------------
   BEGN_INCL       Log                 ; Log
   ; From FPU387.ASM
   ; Log is called by Pwr and is also called directly.
      ftst
      fstsw        ax
      sahf
      jnz          short NotBothZero
      fxch                             ; y x
      ftst
      fstsw        ax
      sahf
      fxch                             ; x y
      jnz          short NotBothZero
      POP_STK      2                   ; clear two numbers
      fldz
      fldz
      mov          ax, 1               ; domain error (1 in ax)
      EXIT_OPER    Log                 ; return (0,0)
   PARSALIGN
NotBothZero:
      xor          ax,ax               ; no domain error (0 in ax)
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      fpatan                           ; z.y x y
      fxch         st(2)               ; y x z.y
      fmul         st,st(0)            ; yy x z.y
      fxch                             ; x yy z.y
      fmul         st,st(0)            ; xx yy z.y
      fadd                             ; mod z.y
      fldln2                           ; ln2, mod, z.y
      fmul         _PointFive          ; ln2/2, mod, z.y
      fxch                             ; mod, ln2/2, z.y
      fyl2x                            ; z.x, z.y
   END_INCL        Log
; --------------------------------------------------------------------------
   BEGN_INCL       SinhCosh            ; Included fn, Sinh, Cosh of st
   ; From FPU087.ASM with mods to use less registers & for 387.
   ; Mod for 387-only after Fractint v18.                        CAE 09OCT93
   ; NOTE: Full 80-bit accuracy is *NOT* maintained in this function!
   ;       Only 1 additional register can be used here.
   ; Changed fn so that rounding errors are less.                CAE 04DEC93
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      fldln2                           ; ln(2) x
      fdivp        st(1),st            ; x/ln(2), start the fdivr instr.
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      or           ax,0000110000000000b
      mov          _Arg2,ax
      fld          st                  ; x/ln(2), x/ln(2)
      fldcw        _Arg2               ; Now set control to round toward zero
   ; Chop toward zero rounding applies now                        CAE 4DEC93
      frndint                          ; int = integer(x/ln(2)), x/ln(2)
      pop          ax                  ; restore old CW to AX
      mov          _Arg2,ax            ; ...then move it to Arg2
      fldcw        _Arg2               ; Restore control word from Arg2
   ; Normal rounding is in effect again                           CAE 4DEC93
      fxch                             ; x/ln(2), int
      fsub         st,st(1)            ; -1 < rem < 1.0, int
      f2xm1                            ; 2**rem-1, int
      fadd         __1_                ; 2**rem, int
      fscale                           ; e**x, int
      fstp         st(1)               ; e**x
      fld          st                  ; e**x, e**x
      fmul         _PointFive          ; e^x/2 e^x
      fstp         QWORD PTR es:[di]   ; e^x  use overflow stk for temp here
      fdivr        _PointFive          ; e**-x/2
      fld          st                  ; e**-x/2, e**-x/2
      fadd         QWORD PTR es:[di]   ; coshx, e**-x/2
      fxch                             ; e^-x/2, coshx
      fsubr        QWORD PTR es:[di]   ; sinhx, coshx (fsubr pending)
   END_INCL        SinhCosh
; --------------------------------------------------------------------------
   BEGN_INCL       Ident               ; Ident                   CAE 09OCT93
   END_INCL        Ident
; --------------------------------------------------------------------------
   BEGN_INCL       Sqr3                ; Sqr3                    CAE 06NOV93
      fmul         st,st(0)            ; Magnitude/sqr of a real# on st
   END_INCL        Sqr3                ; x^2 0 ...
; --------------------------------------------------------------------------
   BEGN_INCL       Conj                ; Complex conjugate
      fxch                             ; y x ...
      fchs                             ; -y x ...
      fxch                             ; x -y ...
   END_INCL        Conj
; --------------------------------------------------------------------------
   BEGN_INCL       Conj2               ; Complex conjugate (uses a reg)
      fldz                             ; 0 x y ...               CAE 20Nov93
      fsubrp       st(2),st            ; x -y ...
   END_INCL        Conj2
; --------------------------------------------------------------------------
   BEGN_INCL       Real                ; Real
      fstp         st(1)               ; x ...
      fldz                             ; 0 x ...
      fxch                             ; x 0 ...
   END_INCL        Real
; --------------------------------------------------------------------------
   BEGN_INCL       RealFlip            ; Real, flip combined.
      fstp         st(1)               ; y=x ...
      fldz                             ; x=0 y ...
   END_INCL        RealFlip
; --------------------------------------------------------------------------
   BEGN_INCL       Add                 ; Add
      faddp        st(2),st            ; Arg2->d.x += Arg1->d.x;
      faddp        st(2),st            ; Arg2->d.y += Arg1->d.y;
   END_INCL        Add
; --------------------------------------------------------------------------
   BEGN_INCL       Sub                 ; Subtract
      fsubp        st(2),st            ; Arg2->d.x -= Arg1->d.x;
      fsubp        st(2),st            ; Arg2->d.y -= Arg1->d.y;
   END_INCL        Sub
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealAdd          ; Load, Real, Add combined
      FIXUP        LodRealAdd, fadd, X ; Add x-value from memory
   END_OPER        LodRealAdd
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealSub          ; Load, Real, Subtract combined
      FIXUP        LodRealSub, fsub, X ; (fsub qword ptr X)
   END_OPER        LodRealSub
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagAdd          ; Load, Imag, Add combined CAE 4DEC93
      FIXUP        LodImagAdd, fadd, Y ; Add x-value from memory
   END_OPER        LodImagAdd
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagSub          ; Load, Imag, Sub combined CAE 4DEC93
      FIXUP        LodImagSub, fsub, Y ; (fsub qword ptr X)
   END_OPER        LodImagSub
; --------------------------------------------------------------------------
   BEGN_INCL       Real2               ; Real value (fast version)
      fldz                             ; 0 x y ... (uses a reg)
      fstp         st(2)               ; x 0 ...
   END_INCL        Real2
; --------------------------------------------------------------------------
   BEGN_OPER       Lod                 ; Load
      FIXUP        Lod, fld, Y         ; y ...
      FIXUP        Lod, fld, X         ; x y ...
   END_OPER        Lod
; --------------------------------------------------------------------------
   BEGN_INCL       Clr1                ; Clear stack
      finit                            ; changed from fninit     CAE 09OCT93
   END_INCL        Clr1
; --------------------------------------------------------------------------
   BEGN_INCL       Imag                ; Imaginary value
      POP_STK      1                   ; y
      fldz                             ; 0 y
      fxch                             ; x=y 0
   END_INCL        Imag
; --------------------------------------------------------------------------
   BEGN_INCL       ImagFlip            ; Imaginary value, flip combined
      POP_STK      1                   ; y ...
      fldz                             ; x=0 y ...
   END_INCL        ImagFlip
; --------------------------------------------------------------------------
   BEGN_INCL Abs                       ; Absolute value
      fxch
      fabs
      fxch
      fabs
   END_INCL Abs
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealMul          ; Load, Real, Multiply
      FIXUP        LodRealMul, fld, X  ; y.x x.x x.y
      fmul         st(2),st            ; y.x x.x z.y
      fmul                             ; z.x z.y
   END_OPER        LodRealMul
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagMul          ; Load, Imag, Multiply     CAE 4DEC93
      FIXUP        LodImagMul, fld, Y  ; y.y x.x x.y
      fmul         st(2),st            ; y.y x.x z.y
      fmul                             ; z.x z.y
   END_OPER        LodImagMul
; --------------------------------------------------------------------------
   BEGN_INCL       Neg                 ; Negative
      fxch
      fchs                             ; Arg1->d.y = -Arg1->d.y;
      fxch
      fchs
   END_INCL        Neg
; --------------------------------------------------------------------------
   BEGN_OPER       EndInit             ; End of initialization expr.
   ifndef          COMPILER            ; this instr not needed   CAE 30DEC93
      mov          _LastInitOp,bx      ; LastInitOp=OpPtr
   endif
      mov          ax, _jump_index     ; InitJumpIndex=jump_index TIW 06Mar97
      mov          _InitJumpIndex,ax
      finit                            ; changed from fninit     CAE 09OCT93
   END_OPER        EndInit
; --------------------------------------------------------------------------
   BEGN_OPER       StoClr1             ; Store, clear FPU
      FIXUP        StoClr1, fstp, X    ; y ...
      FIXUP        StoClr1, fst, Y     ; y ...
      finit                            ; use finit, not fninit
   END_OPER        StoClr1
; --------------------------------------------------------------------------
   BEGN_OPER       StoClr2             ; Store, clear FPU        CAE 09OCT93
      FIXUP        StoClr2, fstp, X    ; y
      FIXUP        StoClr2, fstp, Y    ; <empty> (store pending)
   END_OPER        StoClr2
; --------------------------------------------------------------------------
   BEGN_OPER       Sto                 ; Store, leave on ST
   ; Revised to do store first, then exchange.                   CAE 10NOV93
      FIXUP        Sto, fst, X
      fxch                             ; y x ...
      FIXUP        Sto, fst, Y
      fxch                             ; x y ...
   END_OPER        Sto
; --------------------------------------------------------------------------
   BEGN_OPER       Sto2                ; Store, leave on ST (uses a reg)
      fld          st(1)               ; y x y
      FIXUP        Sto2, fstp, Y       ; x y
      FIXUP        Sto2, fst, X
      fwait                            ; CAE added fwait for safety 15Feb95
   END_OPER        Sto2
; --------------------------------------------------------------------------
   BEGN_OPER       LodReal             ; Load a real
      fldz                             ; 0 ...
      FIXUP        LodReal, fld, X     ; x 0 ...
   END_OPER        LodReal
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealC            ; Load real const
      fldz                             ; y=0 ...
      FIXUP        LodRealC, fld, X    ; x 0 ...
   END_OPER        LodRealC
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealFlip         ; Load real, flip
      FIXUP        LodRealFlip, fld, X ; y=x ...
      fldz                             ; x=0 y ...
   END_OPER        LodRealFlip
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealAbs          ; Load real, abs
      fldz                             ; 0 ...
      FIXUP        LodRealAbs, fld, X  ; x 0 ...
      fabs                             ; x=abs(x) 0 ...
   END_OPER        LodRealAbs
; --------------------------------------------------------------------------
   BEGN_INCL       Flip                ; Exchange real, imag
      fxch                             ; x=y y=x ...
   END_INCL        Flip
; --------------------------------------------------------------------------
   BEGN_OPER       LodImag             ; Load, imaginary
      fldz                             ; 0 ...
      FIXUP        LodImag, fld, Y     ; x=y 0
   END_OPER        LodImag
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagFlip         ; Load, imaginary, flip
      FIXUP        LodImagFlip, fld, Y ; y ...
      fldz                             ; 0 y ...
   END_OPER        LodImagFlip
; --------------------------------------------------------------------------
   BEGN_OPER       LodImagAbs          ; Load, imaginary, absolute value
      fldz                             ; 0 ...
      FIXUP        LodImagAbs, fld, Y  ; x=y 0 ...
      fabs                             ; x=abs(y) 0 ...
   END_OPER        LodImagAbs
; --------------------------------------------------------------------------
   BEGN_OPER       LodConj             ; Load, conjugate
      FIXUP        LodConj, fld, Y     ; y ...
      fchs                             ; y=-y ...
      FIXUP        LodConj, fld, X     ; x y ...
   END_OPER        LodConj
; --------------------------------------------------------------------------
   BEGN_OPER       LodAdd              ; Load, Add (uses a reg)
      FIXUP        LodAdd, fadd, X
      FIXUP        LodAdd, fld, Y
      faddp        st(2),st
   END_OPER        LodAdd
; --------------------------------------------------------------------------
   BEGN_OPER       LodSub              ; Load, Subtract (uses a reg)
      FIXUP        LodSub, fsub, X
      FIXUP        LodSub, fld, Y
      fsubp        st(2),st
   END_OPER        LodSub
; --------------------------------------------------------------------------
   BEGN_OPER       StoDup              ; Store, duplicate top operand
      FIXUP        StoDup, fst, X      ; x y
      fld          st(1)               ; y x y
      FIXUP        StoDup, fst, Y      ; y x y
      fld          st(1)               ; x y x y
   END_OPER        StoDup
; --------------------------------------------------------------------------
   BEGN_OPER       StoDbl              ; Store, double (uses a reg)
      FIXUP        StoDbl, fst, X      ; x y (store x)
      fadd         st,st               ; 2x y
      fld          st(1)               ; y 2x y
      FIXUP        StoDbl, fst, Y      ; y 2x y (store y)
      faddp        st(2),st            ; 2x 2y
   END_OPER        StoDbl
; --------------------------------------------------------------------------
   BEGN_INCL       Zero                ; Zero                    CAE 09OCT93
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      fldz                             ; 0 0 ...
   END_INCL        Zero
; --------------------------------------------------------------------------
   BEGN_INCL       One                 ; One                     CAE 06NOV93
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      fld1                             ; 1 0 ...
   END_INCL        One
; --------------------------------------------------------------------------
   BEGN_OPER       LodSubMod           ; Load, Subtract, Mod
      FIXUP        LodSubMod, fsub, X  ; x.x-y.x  x.y  ...
      fmul         st,st               ; sqr(x.x-y.x) x.y ...
      fldz                             ; 0 sqrx x.y ...
      fxch         st(2)               ; x.y sqrx 0 ...
      FIXUP        LodSubMod, fsub, Y  ; x.y-y.y sqrx 0 ...
      fmul         st,st               ; sqry sqrx 0 ...
      fadd                             ; mod 0
   END_OPER        LodSubMod
; --------------------------------------------------------------------------
   BEGN_INCL       Sqr                 ; Square, save magnitude in LastSqr
      fld          st(0)               ; x x y
      fmul         st(1),st            ; x x*x y
      fmul         st,st(2)            ; xy xx y
      mov          si, WORD PTR _v     ; si -> variables
      fadd         st,st(0)            ; 2xy xx y
      fxch         st(2)               ; y xx 2xy
      fmul         st,st(0)            ; yy xx 2xy
      fld          st(1)               ; xx yy xx 2xy
      fadd         st,st(1)            ; xx+yy yy xx 2xy
      fstp         QWORD PTR [si+LASTSQR] ; yy xx 2xy
      fsubp        st(1),st            ; xx-yy 2xy
   END_INCL        Sqr
; --------------------------------------------------------------------------
   BEGN_INCL       Sqr0                ; Square, don't save magnitude
       GEN_SQR0
   END_INCL        Sqr0
; --------------------------------------------------------------------------
   BEGN_INCL       Mul                 ; Multiply
   ; From FPU087.ASM
      fld          st(1)               ; y.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.y*x.y, y.x. y.y, x.x, x.y
      fld          st(1)               ; y.x, y.y*x.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.x*x.x,y.y*x.y,y.x y.y,x.x,x.y
      fsubr                            ; newx=y.x*x.x-y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x, y.x, y.y, newx, x.y
      fmulp        st(2),st            ; y.x, y.y*x.x, newx, x.y
      fmulp        st(3),st            ; y.y*x.x, newx, y.x*x.y
      faddp        st(2),st            ; newx newy = y.x*x.y + x.x*y.y
   END_INCL        Mul
; --------------------------------------------------------------------------
   BEGN_OPER       LodMul              ; Load, Multiply
   ; This is just load followed by multiply but it saves a fn. call
   ;    and also allows optimizer enhancements.
      FIXUP        LodMul, fld, Y      ; y.y x.x x.y
      FIXUP        LodMul, fld, X      ; y.x y.y x.x x.y
      fld          st(1)               ; y.y, y.x, y.y, x.x, x.y
      fmul         st,st(4)            ; y.y*x.y, y.x. y.y, x.x, x.y
      fld          st(1)               ; y.x, y.y*x.y, y.x, y.y, x.x, x.y
      fmul         st, st(4)           ; y.x*x.x, y.y*x.y, y.x, y.y, x.x, x.y
      fsubr                            ; newx=y.x*x.x-y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x, y.x, y.y, newx, x.y
      fmulp        st(2), st           ; y.x, y.y*x.x, newx, x.y
      fmulp        st(3), st           ; y.y*x.x, newx, y.x*x.y
      faddp        st(2), st           ; newx newy = y.x*x.y + x.x*y.y
   END_OPER        LodMul
; --------------------------------------------------------------------------
   BEGN_INCL       Div                 ; Divide
   ; From FPU087.ASM with speedups
      fld          st(1)               ; y.y,y.x,y.y,x.x,x.y
      fmul         st,st               ; y.y*y.y,y.x,y.y,x.x,x.y
      fld          st(1)               ; y.x,y.y*y.y,y.x,y.y,x.x,x.y
      fmul         st,st               ; y.x*y.x,y.y*y.y,y.x,y.y,x.x,x.y
      fadd                             ; mod,y.x,y.y,x.x,x.y
      ftst
      fstsw        ax
      sahf
      jz           short DivNotOk
                                       ; can't do this divide until now
      fdiv         st(1),st            ; mod,y.x=y.x/mod,y.y,x.x,x.y
      fdivp        st(2),st            ; y.x,y.y=y.y/mod,x.x,x.y
      fld          st(1)               ; y.y,y.x,y.y,x.x,x.y
      fmul         st,st(4)            ; y.y*x.y,y.x,y.y,x.x,x.y
      fld          st(1)               ; y.x,y.y*x.y,y.x,y.y,x.x,x.y
      fmul         st,st(4)            ; y.x*x.x,y.y*x.y,y.x,y.y,x.x,x.y
      fadd                             ; y.x*x.x+y.y*x.y,y.x,y.y,x.x,x.y
      fxch         st(3)               ; x.x,y.x,y.y,newx,x.y
      fmulp        st(2),st            ; y.x,y.y*x.x,newx,x.y
      fmulp        st(3),st            ; x.x*y.y,newx,y.x*x.y
      fsubp        st(2),st            ; newx,newy
      EXIT_OPER    Div
DivNotOk:
      POP_STK      5                   ; clear 5 from stack (!)
      fld          _infinity           ; return a very large number
      fld          st(0)
      mov          ax,_save_release
      cmp          ax,1920
      jle          oldwayD
      mov          _overflow, 1
oldwayD:
   END_INCL        Div
; --------------------------------------------------------------------------
   BEGN_INCL       Recip               ; Reciprocal
   ; From FPU087.ASM
      fld          st(1)               ; y, x, y
      fmul         st,st               ; y*y, x, y
      fld          st(1)               ; x, y*y, x, y
      fmul         st,st               ; x*x, y*y, x, y
      fadd                             ; mod, x, y
      ftst
      fstsw        ax
      sahf
      jz           short RecipNotOk
      fdiv         st(1),st            ; mod, newx=x/mod, y
      fchs                             ; -mod newx y
      fdivp        st(2),st            ; newx, newy=y/-mod
      EXIT_OPER    Recip
RecipNotOk:
      POP_STK      3                   ; clear three from stack
      fld          _infinity           ; return a very large number
      fld          st(0)
      mov          ax,_save_release
      cmp          ax,1920
      jle          oldwayR
      mov          _overflow, 1
oldwayR:
   END_INCL        Recip
; --------------------------------------------------------------------------
   BEGN_OPER       StoSqr              ; Sto, Square, save magnitude
      fld          st(0)               ; x x y
      FIXUP        StoSqr, fst, X      ;   "   (store x)
      fmul         st(1),st            ; x x*x y
      fmul         st,st(2)            ; xy xx y
      fadd         st,st(0)            ; 2xy xx y
      fxch         st(2)               ; y xx 2xy
      FIXUP        StoSqr, fst, Y      ;    "     (store y)
      fmul         st,st(0)            ; yy xx 2xy
   ; It is now safe to overlay si here
      mov          si, WORD PTR _v     ; si -> variables
      fld          st(1)               ; xx yy xx 2xy
      fadd         st,st(1)            ; xx+yy yy xx 2xy
      fstp         QWORD PTR [si+LASTSQR] ; yy xx 2xy
      fsubp        st(1),st            ; xx-yy 2xy
   END_OPER        StoSqr
; --------------------------------------------------------------------------
   BEGN_OPER       StoSqr0             ; Sto, Square, don't save magnitude
      fld          st(0)               ; x x y
      FIXUP        StoSqr0, fst, X     ; store x
      fld          st(0)               ; x x x y
      fmul         st,st(3)            ; xy x x y
      fadd         st,st               ; 2xy x x y
      fxch         st(3)               ; y x x 2xy
      FIXUP        StoSqr0, fst, Y     ; store y
      fadd         st(2),st            ; y x x+y 2xy
      fsubp        st(1),st            ; x-y x+y 2xy
      fmulp        st(1),st            ; xx-yy 2xy
   END_OPER        StoSqr0
; --------------------------------------------------------------------------
   BEGN_INCL       Mod2                ; Modulus (uses a reg)
      fmul         st,st               ; xx y
      fldz                             ; 0 xx y
      fxch         st(2)               ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_INCL        Mod2
; --------------------------------------------------------------------------
   BEGN_OPER       LodMod2             ; Load, Modulus (uses a reg)
      fldz                             ; 0 ...
      FIXUP        LodMod2, fld, X     ; x 0 ...
      fmul         st,st               ; xx 0
      FIXUP        LodMod2, fld, Y     ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_OPER        LodMod2
; --------------------------------------------------------------------------
   BEGN_OPER       StoMod2             ; Store, Modulus (uses a reg)
      FIXUP        StoMod2, fst, X     ; x y
      fmul         st,st               ; xx y
      fldz                             ; 0 xx y
      fxch         st(2)               ; y xx 0
      FIXUP        StoMod2, fst, Y     ; y xx 0
      fmul         st,st               ; yy xx 0
      fadd                             ; mod 0
   END_OPER        StoMod2
; --------------------------------------------------------------------------
   BEGN_OPER       Clr2                ; Test ST, clear FPU
      ftst
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit                           ; fstsw will complete first
      and          ah,01000000b        ; return 1 if zf=1
      shr          ax,14               ; AX will be returned by fFormula()
   END_OPER        Clr2
; --------------------------------------------------------------------------
   BEGN_OPER       PLodAdd             ; Load, Add (uses no regs)
      fxch                             ; y x
      FIXUP        PLodAdd, fadd, Y    ; add y from memory
      fxch                             ; x y
      FIXUP        PLodAdd, fadd, X    ; add x, overlap execution
   END_OPER        PLodAdd
; --------------------------------------------------------------------------
   BEGN_OPER       PLodSub             ; Load, Subtract (uses no regs)
      fxch
      FIXUP        PLodSub, fsub, Y    ; sub y from memory
      fxch                             ; x y
      FIXUP        PLodSub, fsub, X    ; sub x, overlap execution
   END_OPER        PLodSub
; --------------------------------------------------------------------------
   BEGN_OPER       LodDup              ; Load, duplicate
      FIXUP        LodDup, fld, Y      ; y ...
      FIXUP        LodDup, fld, X      ; x y ...
      fld          st(1)               ; y x y ...
      fld          st(1)               ; x y x y ...
   END_OPER        LodDup
; --------------------------------------------------------------------------
   BEGN_OPER       LodSqr              ; Load, square (no save lastsqr)
      FIXUP        LodSqr, fld, Y      ; y ...
      fld          st(0)               ; y y ...
      fadd         st(1),st            ; y 2y ...
      fld          st(0)               ; y y 2y
      FIXUP        LodSqr, fld, X      ; x y y 2y ...
      fmul         st(3),st            ; x y y 2xy ...
      fadd         st(2),st            ; x y X+y 2xy ...
      fsubrp       st(1),st            ; x-y x+y 2xy ...
      fmul                             ; xx-yy 2xy ...
   END_OPER        LodSqr
; --------------------------------------------------------------------------
   BEGN_OPER       LodSqr2             ; Load, square (save lastsqr)
      FIXUP        LodSqr2, fld, Y     ; y ...
      fld          st(0)               ; y y ...
      fadd         st(1),st            ; y 2y ...
      fmul         st,st(0)            ; yy 2y ...
      FIXUP        LodSqr2, fld, X     ; x yy 2y ...
      fmul         st(2),st            ; x yy 2xy ...
      mov          si,WORD PTR _v      ; put address of v in si
      fmul         st,st(0)            ; xx yy 2xy ...
      fld          st(0)               ; xx xx yy 2xy
      fadd         st,st(2)            ; mod xx yy 2xy
      fstp         QWORD PTR [si+LASTSQR] ; xx yy 2xy ... (save lastsqr)
      fsubrp       st(1),st            ; xx-yy 2xy ...
   END_OPER        LodSqr2
; --------------------------------------------------------------------------
   BEGN_OPER       LodDbl              ; Load, double
      FIXUP        LodDbl, fld, Y      ; load y
      fadd         st,st(0)            ; double it
      FIXUP        LodDbl, fld, X      ; same for x
      fadd         st,st(0)
   END_OPER        LodDbl
; --------------------------------------------------------------------------
   BEGN_INCL       Dbl                 ; Double                  CAE 31OCT93
      fxch                             ; y x ...
      fadd         st,st(0)            ; 2y x ...
      fxch                             ; x 2y ...
      fadd         st,st(0)            ; 2x 2y ...
   END_INCL        Dbl
; --------------------------------------------------------------------------
   BEGN_INCL       Mod                 ; Modulus (uses no regs)
      fmul         st,st               ; x*x y
      fxch                             ; y x*x
      fmul         st,st               ; y*y x*x
      fadd                             ; mod
      fldz                             ; 0 mod
      fxch                             ; mod 0
   END_INCL        Mod
; --------------------------------------------------------------------------
; The following code was 'discovered' by experimentation.  The Intel manuals
;   really don't help much in writing this kind of code.
; --------------------------------------------------------------------------
   BEGN_INCL       Push2               ; Push stack down from 8 to 6
      fdecstp                          ; roll the stack
      fdecstp                          ; ...
      fstp         tbyte PTR es:[di]   ; store x on overflow stack
      fstp         tbyte PTR es:[di+10] ; and y (ten bytes each)
      add          di,20               ; adjust di
   END_INCL        Push2
; --------------------------------------------------------------------------
   BEGN_INCL       Pull2               ; Pull stack up from 2 to 4
      fld          tbyte PTR es:[di-10] ; oldy x y
      sub          di,20               ; adjust di now
      fxch         st(2)               ; y x oldy
      fld          tbyte PTR es:[di]   ; oldx y x oldy
      fxch         st(2)               ; x y oldx oldy
   END_INCL        Pull2
; --------------------------------------------------------------------------
   BEGN_INCL       Push4               ; Push stack down from 8 to 4
      fdecstp                          ; roll the stack four times
      fdecstp
      fdecstp
      fdecstp
      fstp         tbyte PTR es:[di+20] ; save the bottom four numbers
      fstp         tbyte PTR es:[di+30] ; save full precision on overflow
      fstp         tbyte PTR es:[di]
      fstp         tbyte PTR es:[di+10]
      add          di,40                ; adjust di
   END_INCL        Push4
; --------------------------------------------------------------------------
   BEGN_INCL       Push2a              ; Push stack down from 6 to 4
      fdecstp                          ; roll the stack 4 times
      fdecstp
      fdecstp
      fdecstp
      fstp         tbyte PTR es:[di]   ; save only two numbers
      fstp         tbyte PTR es:[di+10]
      add          di, 20
      fincstp                          ; roll back 2 times
      fincstp
   END_INCL        Push2a
; --------------------------------------------------------------------------
; End of stack overflow/underflow code.
; --------------------------------------------------------------------------
   BEGN_INCL       Exp                ; Exponent
   ; From FPU387.ASM with mods to use less registers.
   ; Modified to preserve 80-bit accuracy.                      CAE 10NOV93
      fldln2                           ; ln2 x y
      fdivp        st(1),st            ; x/ln2 y
      fstp         TBYTE PTR es:[di]   ; y
      fsincos                          ; cosy, siny
      fld1                             ; 1 cos sin
      fld          TBYTE PTR es:[di]   ; x/ln2 1 cos sin
      fprem                            ; prem, 1, cos, sin
      f2xm1                            ; e**prem-1, 1, cos, sin
      fadd                             ; e**prem, cos, sin
      fld          TBYTE PTR es:[di]   ; x.x/ln2, e**prem, cos, sin
      fxch                             ; e**prem, x.x/ln2, cos, sin
      fscale                           ; e**x.x, x.x/ln2, cos, sin
      fstp         st(1)               ; e**x.x, cos, sin
      fmul         st(2),st            ; e**x.x, cos, z.y
      fmul                             ; z.x, z.y
   END_INCL        Exp
; --------------------------------------------------------------------------
   BEGN_OPER       Pwr                 ; Power
   ; First exchange the top two complex numbers.
      fxch         st(2)               ; x.x y.y y.x x.y
      fxch                             ; y.y x.x y.x x.y
      fxch         st(3)               ; x.y x.x y.x y.y
      fxch                             ; x.x x.y y.x y.y
   ; Now take the log of the # on st.
      INCL_OPER    Pwr, Log            ; l.x l.y y.x y.y
      cmp          ax,1                ; log domain error?
      jne          short domainok      ; nope
      test         _ldcheck, 1         ; user wants old pwr?
      jnz          short domainok      ; yup
      POP_STK      4                   ; clear stack completely
      fldz                             ; 0
      fldz                             ; 0 0
      EXIT_OPER    Pwr                 ; return (0,0)
   PARSALIGN
domainok:
   ; Inline multiply function from FPU087.ASM instead of include.
      fld          st(1)               ; y.y y.x y.y x.x x.y
      fmul         st,st(4)            ; y.y*x.y y.x y.y x.x x.y
      fld          st(1)               ; y.x y.y*x.y y.x y.y x.x x.y
      fmul         st,st(4)            ; y.x*x.x y.y*x.y y.x y.y x.x x.y
      fsubr                            ; newx=y.x*x.x-y.y*x.y y.x y.y x.x x.y
      fxch         st(3)               ; x.x y.x y.y newx x.y
      fmulp        st(2),st            ; y.x y.y*x.x newx x.y
      fmulp        st(3),st            ; y.y*x.x newx y.x*x.y
      faddp        st(2),st            ; newx newy=y.x*x.y+x.x*y.y
   ; Exp function from FPU387.ASM.  4 regs are free here.
   ; Modified to use the regs instead of memory.                 CAE 06NOV93
      fldln2                           ; ln2 x y
      fdiv                             ; x/ln2 y
      fxch                             ; y x/ln2
      fsincos                          ; cosy, siny, x/ln2
      fxch                             ; sin, cos, x/ln2
      fxch         st(2)               ; x/ln2, cos, sin
      fld1                             ; 1, x/ln2, cos, sin
      fld          st(1)               ; x/ln2, 1, x/ln2, cos, sin
      fprem                            ; prem, 1, x/ln2, cos, sin
      f2xm1                            ; e**prem-1, 1, x/ln2, cos, sin
      fadd                             ; e**prem, x/ln2, cos, sin
      fscale                           ; e**x.x, x.x/ln2, cos, sin
      fstp         st(1)               ; e**x.x, cos, sin
      fmul         st(2),st            ; e**x.x, cos, z.y
      fmul                             ; z.x, z.y
   END_OPER        Pwr
; --------------------------------------------------------------------------
   BEGN_OPER       LodRealPwr          ; lod, real, power         CAE 6NOV93
   ; First take the log of the # on st.
      INCL_OPER    LodRealPwr, Log     ; l.x l.y
      cmp          ax,1                ; log domain error?
      jne          short domainok2     ; nope
      cmp          _ldcheck, 1         ; user wants old lodrealpwr?
      je           short domainok2     ; yup
      POP_STK      2                   ; clear stack completely
      fldz                             ; 0
      fldz                             ; 0 0
      EXIT_OPER    LodRealPwr          ; return (0,0)
   PARSALIGN
domainok2:
   ; Inline multiply by a real.
      FIXUP        LodRealPwr, fld, X  ; y.x, x.x, x.y
      fmul         st(2),st            ; y.x, x.x, z.y
      fmulp        st(1),st            ; z.x z.y
   ; Exp function from FPU387.ASM.  4 regs are free here, so use them.
      fldln2                           ; ln2 x y
      fdiv                             ; x/ln2 y
      fxch                             ; y x/ln2
      fsincos                          ; cosy, siny, x/ln2
      fxch                             ; sin, cos, x/ln2
      fxch         st(2)               ; x/ln2, cos, sin
      fld1                             ; 1, x/ln2, cos, sin
      fld          st(1)               ; x/ln2, 1, x/ln2, cos, sin
      fprem                            ; prem, 1, x/ln2, cos, sin
      f2xm1                            ; e**prem-1, 1, x/ln2, cos, sin
      fadd                             ; e**prem, x/ln2, cos, sin
      fscale                           ; e**x.x, x.x/ln2, cos, sin
      fstp         st(1)               ; e**x.x, cos, sin
      fmul         st(2),st            ; e**x.x, cos, z.y
      fmul                             ; z.x, z.y
   END_OPER        LodRealPwr
; --------------------------------------------------------------------------
   BEGN_OPER       Cosh                ; Cosh
      INCL_OPER    Cosh, SinhCosh      ; sinhx coshx y
      fxch         st(2)               ; y coshx sinhx
      fsincos                          ; cosy siny coshx sinhx
      fmulp        st(2),st            ; siny x=cosy*coshx sinhx
      fmulp        st(2),st            ; x y=sinhx*siny
   END_OPER        Cosh
; --------------------------------------------------------------------------
   BEGN_OPER       Sinh                ; Sinh
      INCL_OPER    Sinh, SinhCosh      ; sinhx coshx y
      fxch         st(2)               ; y coshx sinhx
      fsincos                          ; cosy siny coshx sinhx
      fmulp        st(3),st            ; siny coshx x=sinhx*cosy
      fmulp        st(1),st            ; y=coshx*siny x
      fxch                             ; x y
   END_OPER        Sinh
; --------------------------------------------------------------------------
   BEGN_OPER       Sin                 ; Sin
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    Sin, SinhCosh       ; sinhy coshy sinx cosx
      fmulp        st(3),st            ; coshy sinx y=cosx*sinhy
      fmulp        st(1),st            ; x=sinx*coshy y
   END_OPER        Sin
; --------------------------------------------------------------------------
   BEGN_OPER       Cos                 ; Cos
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    Cos, SinhCosh       ; sinhy coshy sinx cosx
      fchs                             ; -sinhy coshy sinx cosx
      fmulp        st(2),st            ; coshy y=-sinhy*sinx cosx
      fmulp        st(2),st            ; y x=cosx*coshy
      fxch                             ; x y
   END_OPER        Cos
; --------------------------------------------------------------------------
   BEGN_OPER       CosXX               ; CosXX
      fsincos                          ; cosx sinx y
      fxch         st(2)               ; y sinx cosx
      INCL_OPER    CosXX, SinhCosh     ; sinhy coshy sinx cosx
      ; note missing fchs here
      fmulp        st(2),st            ; coshy y=sinhy*sinx cosx
      fmulp        st(2),st            ; y x=cosx*coshy
      fxch                             ; x y
   END_OPER        CosXX
; --------------------------------------------------------------------------
   BEGN_OPER       Tan                 ; Tan
      fadd         st,st               ; 2x y
      fsincos                          ; cos2x sin2x y
      fxch         st(2)               ; y sin2x cos2x
      fadd         st,st               ; 2y sin2x cos2x
      INCL_OPER    Tan, SinhCosh       ; sinh2y cosh2y sin2x cos2x
      fxch                             ; cosh2y sinh2y sin2x cos2x
      faddp        st(3),st            ; sinhy sinx denom=cos2x+cosh2y
      fld          st(2)               ; denom sinh2y sin2x denom
      fdivp        st(2),st            ; sinh2y x=sin2x/denom denom
      fdivrp       st(2),st            ; x y=sinh2y/denom
   END_OPER        Tan
; --------------------------------------------------------------------------
   BEGN_OPER       CoTan               ; CoTan
      fadd         st,st               ; 2x y
      fsincos                          ; cos2x sin2x y
      fxch         st(2)               ; y sin2x cos2x
      fadd         st,st               ; 2y sin2x cos2x
      INCL_OPER    CoTan, SinhCosh     ; sinh2y cosh2y sin2x cos2x
      fxch                             ; cosh2y sinh2y sin2x cos2x
      fsubrp       st(3),st            ; sinh2y sin2x denom=cosh2y-cos2x
      fld          st(2)               ; denom sinh2y sin2x denom
      fdivp        st(2),st            ; sinh2y x=sin2x/denom denom
      fchs                             ; -sinh2y x denom
      fdivrp       st(2),st            ; x y=-sinh2y/denom
   END_OPER        CoTan
; --------------------------------------------------------------------------
   BEGN_OPER       Tanh                ; Tanh
      fadd         st,st               ; 2x y
      INCL_OPER    Tanh, SinhCosh      ; sinh2x cosh2x y
      fxch         st(2)               ; y cosh2x sinh2x
      fadd         st,st               ; 2y cosh2x sinh2x
      fsincos                          ; cos2y sin2y cosh2x sinh2x
      faddp        st(2),st            ; sin2y denom=cos2y+cosh2x sinh2x
      fxch                             ; denom sin2y sinh2x
      fdiv         st(1),st            ; denom y=sin2y/denom sinh2x
      fdivp        st(2),st            ; y x=sinh2x/denom
      fxch                             ; x y
   END_OPER        Tanh
; --------------------------------------------------------------------------
   BEGN_OPER       CoTanh              ; CoTanh
      fadd         st,st               ; 2x y
      INCL_OPER    CoTanh, SinhCosh    ; sinh2x cosh2x y
      fxch         st(2)               ; y cosh2x sinh2x
      fadd         st,st               ; 2y cosh2x sinh2x
      fsincos                          ; cos2y sin2y cosh2x sinh2x
      fsubp        st(2),st            ; sin2y denom=cosh2x-cos2y sinh2x
      fchs                             ; -sin2y denom sinh2x
      fxch                             ; denom -sin2y sinh2x
      fdiv         st(1),st            ; denom y=-sin2y/denom sinh2x
      fdivp        st(2),st            ; y x=sinh2x/denom
      fxch                             ; x y
   END_OPER CoTanh
; --------------------------------------------------------------------------
; JCO added Sqrt .. CAbs for version 19.
; CAE updated them 15Feb94 to work with compiler mode.
; --------------------------------------------------------------------------
   BEGN_OPER       Sqrt                ; Sqrt
      GEN_SQRT
   END_OPER Sqrt
; --------------------------------------------------------------------------
   BEGN_OPER       ASin                ; ArcSin
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      GEN_SQR0                         ; tz1.x tz1.y x y
      fxch         st(1)               ; tz1.y tz1.x x y
      fchs                             ; -tz1.y tz1.x x y
      fxch         st(1)               ; tz1.x -tz1.y x y
      fsubr        __1_                ; 1-tz1.x -tz1.y x y
      GEN_SQRT                         ; tz1.x tz1.y x y
      fsubrp       st(3),st            ; tz1.y x tz1.x-y
      fadd                             ; tz1.y+x tz1.x-y
      fxch         st(1)               ; tz1.x-y tz1.y+x
      INCL_OPER    ASin, Log           ; l.x l.y
      fchs                             ; -l.x l.y
      fxch         st(1)               ; l.y -l.x ;; rz = (-i)*l
   END_OPER ASin
; --------------------------------------------------------------------------
   BEGN_OPER       ACos                ; ArcCos
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      GEN_SQR0                         ; tz1.x tz1.y x y
      fsub         __1_                ; tz1.x-1 tz1.y x y
      GEN_SQRT                         ; tz.x tz.y x y
      faddp        st(2),st            ; tz.y tz.x+x y
      faddp        st(2),st            ; tz.x+x tz.y+y
      INCL_OPER    ACos, Log           ; l.x l.y
      fchs                             ; -l.x l.y
      fxch         st(1)               ; l.y -l.x ;; rz = (-i)*l
   END_OPER ACos
; --------------------------------------------------------------------------
   BEGN_OPER       ASinh               ; ArcSinh
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      GEN_SQR0                         ; tz1.x tz1.y x y
      fadd         __1_                ; tz1.x+1 tz1.y x y
      GEN_SQRT                         ; tz.x tz.y x y
      faddp        st(2),st            ; tz.y tz.x+x y
      faddp        st(2),st            ; tz.x+x tz.y+y
      INCL_OPER    ASinh, Log          ; l.x l.y
   END_OPER ASinh
; --------------------------------------------------------------------------
   BEGN_OPER       ACosh               ; ArcCosh
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      GEN_SQR0                         ; tz1.x tz1.y x y
      fsub         __1_                ; tz1.x+1 tz1.y x y
      GEN_SQRT                         ; tz.x tz.y x y
      faddp        st(2),st            ; tz.y tz.x+x y
      faddp        st(2),st            ; tz.x+x tz.y+y
      INCL_OPER    ACosh, Log          ; l.x l.y
   END_OPER ACosh
; --------------------------------------------------------------------------
   BEGN_OPER       ATanh               ; ArcTanh
      fld          st(1)               ; y x y
      fchs                             ; -y x y
      fld          st(1)               ; x -y x y
      fld1                             ; 1 x -y x y
      fadd         st(3),st            ; 1 x -y 1+x y
      fsubr                            ; 1-x -y 1+x y
      INCL_OPER    ATanh, Div          ; d.x d.y
   ; From FPU387.ASM
      ftst
      fstsw        ax
      sahf
      jnz          short ATanh_NotBothZero
      fxch                             ; y x
      ftst
      fstsw        ax
      sahf
      fxch                             ; x y
      jnz          short ATanh_NotBothZero
      POP_STK      2                   ; clear two numbers
      fldz
      fldz
      jmp          SHORT End_Log_ATanh ; return (0,0)
   PARSALIGN
ATanh_NotBothZero:
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      fpatan                           ; z.y x y
      fxch         st(2)               ; y x z.y
      fmul         st,st(0)            ; yy x z.y
      fxch                             ; x yy z.y
      fmul         st,st(0)            ; xx yy z.y
      fadd                             ; mod z.y
      fldln2                           ; ln2, mod, z.y
      fmul         _PointFive          ; ln2/2, mod, z.y
      fxch                             ; mod, ln2/2, z.y
      fyl2x                            ; z.x, z.y
End_Log_ATanh:
      fld          _PointFive          ; .5 l.x l.y
      fmul         st(1),st            ; .5 l.x/2 l.y
      fmulp        st(2),st            ; l.x/2 l.y/2
   END_OPER ATanh
; --------------------------------------------------------------------------
   BEGN_OPER       ATan                ; ArcTan
      fxch                             ; y x
      fld          st(1)               ; x y x
      fchs                             ; -x y x
      fxch         st(2)               ; x y -x
      fld          st(1)               ; y x y -x
      fld1                             ; 1 y x y -x
      fadd         st(3),st            ; 1 y x 1+y -x
      fsubr                            ; 1-y x 1+y -x
      INCL_OPER    ATan, Div           ; d.x d.y
   ; CAE put log fn inline 15Feb95
      ftst
      fstsw        ax
      sahf
      jnz          short ATan_NotBothZero
      fxch                             ; y x
      ftst
      fstsw        ax
      sahf
      fxch                             ; x y
      jnz          short ATan_NotBothZero
      POP_STK      2                   ; clear two numbers
      fldz
      fldz
      jmp          short End_Log_ATan  ; return (0,0)
   PARSALIGN
ATan_NotBothZero:
      fld          st(1)               ; y x y
      fld          st(1)               ; x y x y
      fpatan                           ; z.y x y
      fxch         st(2)               ; y x z.y
      fmul         st,st(0)            ; yy x z.y
      fxch                             ; x yy z.y
      fmul         st,st(0)            ; xx yy z.y
      fadd                             ; mod z.y
      fldln2                           ; ln2, mod, z.y
      fmul         _PointFive          ; ln2/2, mod, z.y
      fxch                             ; mod, ln2/2, z.y
      fyl2x                            ; z.x, z.y
End_Log_ATan:
      fld          _PointFive          ; .5 l.x l.y
      fmul         st(1),st            ; .5 z.y=l.x/2 l.y
      fmulp        st(2),st            ; z.y l.y/2
      fxch                             ; l.y/2 z.y
      fchs                             ; z.x=-l.y/2 z.y
   END_OPER ATan
; --------------------------------------------------------------------------
   BEGN_OPER       CAbs                ; Complex Absolute Value
      fmul         st,st               ; x*x y
      fxch                             ; y x*x
      fmul         st,st               ; y*y x*x
      fadd                             ; y*y+x*x
      fsqrt                            ; mag=sqrt(yy+xx)
      fldz                             ; 0 mag
      fxch                             ; mag 0
   END_OPER CAbs
; --------------------------------------------------------------------------
; End of new functions.                                          CAE 15Feb95
; --------------------------------------------------------------------------
   BEGN_OPER       Floor               ; Complex floor
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      and          ax,1111001111111111b
      or           ax,0000010000000000b
      mov          _Arg2,ax
      fldcw        _Arg2               ; Now set control to round toward -inf
   ; Chop toward negative infinity applies now
      frndint                          ; floor(x) y
      fxch                             ; y floor(x)
      frndint                          ; floor(y) floor(x)
      fxch                             ; floor(x) floor(y)
      pop          ax                  ; restore old CW to AX
      mov          _Arg2,ax            ; ...then move it to Arg2
      fldcw        _Arg2               ; Restore control word from Arg2
   ; Normal rounding is in effect again
   END_OPER        Floor
; --------------------------------------------------------------------------
   BEGN_OPER       Ceil                ; Complex ceiling
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      and          ax,1111001111111111b
      or           ax,0000100000000000b
      mov          _Arg2,ax
      fldcw        _Arg2               ; Now set control to round toward +inf
   ; Chop toward positive infinity applies now
      frndint                          ; ceil(x) y
      fxch                             ; y ceil(x)
      frndint                          ; ceil(y) ceil(x)
      fxch                             ; ceil(x) ceil(y)
      pop          ax                  ; restore old CW to AX
      mov          _Arg2,ax            ; ...then move it to Arg2
      fldcw        _Arg2               ; Restore control word from Arg2
   ; Normal rounding is in effect again
   END_OPER        Ceil
; --------------------------------------------------------------------------
   BEGN_OPER       Trunc               ; Complex truncation
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      or           ax,0000110000000000b
      mov          _Arg2,ax
      fldcw        _Arg2               ; Now set control to round toward zero
   ; Chop toward zero rounding applies now
      frndint                          ; trunc(x) y
      fxch                             ; y trunc(x)
      frndint                          ; trunc(y) trunc(x)
      fxch                             ; trunc(x) trunc(y)
      pop          ax                  ; restore old CW to AX
      mov          _Arg2,ax            ; ...then move it to Arg2
      fldcw        _Arg2               ; Restore control word from Arg2
   ; Normal rounding is in effect again
   END_OPER        Trunc
; --------------------------------------------------------------------------
   BEGN_OPER       Round               ; Complex round to nearest
      fstcw        _Arg2               ; use arg2 to hold CW
      fwait
      mov          ax,_Arg2            ; Now do some integer instr.'s
      push         ax                  ; Save control word on stack
      and          ax,1111001111111111b
      or           ax,0000010000000000b
      mov          _Arg2,ax
      fldcw        _Arg2               ; Now set control to round toward -inf
   ; Round toward negative infinity applies now
      fadd         _PointFive          ; x+.5  y
      frndint                          ; round(x) y
      fxch                             ; y round(x)
      fadd         _PointFive          ; y+.5 round(x)
      frndint                          ; round(y) round(x)
      fxch                             ; round(x) round(y)
      pop          ax                  ; restore old CW to AX
      mov          _Arg2,ax            ; ...then move it to Arg2
      fldcw        _Arg2               ; Restore control word from Arg2
   ; Normal rounding is in effect again
   END_OPER        Round
; --------------------------------------------------------------------------
; End of new functions.                                          TIW 30Jun96
; --------------------------------------------------------------------------
   BEGN_INCL       Jump                ;
      mov          ax,JCSZ             ; ax = sizeof(jump control struct)
      imul         _jump_index         ; address of jump_control[jump_index]
      push         es
      les          bx, _jump_control
      add          bx,ax
      mov          ax,WORD PTR es:[bx+8]; jump_index = DestJumpIndex 
      mov          bx,WORD PTR es:[bx+2]; bx = JumpOpPtr
      pop          es
      mov          _jump_index,ax
      add          bx, WORD PTR _pfls  ; 
   END_INCL        Jump                ;
; --------------------------------------------------------------------------
   BEGN_OPER       JumpOnTrue          ;
      ftst                             ; test Arg1.x
      fstsw        ax
      sahf
      jz           short NotTrue       ; if(Arg1.x != 0)     
      INCL_OPER    JumpOnTrue, Jump    ; call Jump
      jmp          short EndJumpOnTrue
NotTrue:
      add          _jump_index, 1      ; else jump_index++ 
EndJumpOnTrue:
   END_OPER        JumpOnTrue          ;
; --------------------------------------------------------------------------
   BEGN_OPER       JumpOnFalse         ;
      ftst                             ; test Arg1.x
      fstsw        ax
      sahf
      jnz          short True          ; if(Arg1.x == 0)
      INCL_OPER    JumpOnFalse, Jump
      jmp          short EndJumpOnFalse
True:
      add          _jump_index, 1      ; else jump_index++ 
EndJumpOnFalse:
   END_OPER        JumpOnFalse         ;
; --------------------------------------------------------------------------
   BEGN_OPER       JumpLabel           ;
      add          _jump_index, 1      ; jump_index++
   END_OPER        JumpLabel           ;
; --------------------------------------------------------------------------
; End of new functions.                                          TIW 09Mar97
; --------------------------------------------------------------------------
   BEGN_OPER       LT                  ; <
   ; Arg2->d.x = (double)(Arg2->d.x < Arg1->d.x);
      fcomp        st(2)               ; y.y, x.x, x.y, comp arg1 to arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jbe          short LTfalse       ; jump if arg1 <= arg2
      fld1                             ; 1 0 (return arg2 < arg1)
      EXIT_OPER    LT
LTfalse:
      fldz                             ; 0 0
   END_OPER        LT
; --------------------------------------------------------------------------
   BEGN_INCL       LT2                 ; LT, set AX, clear FPU
   ; returns !(Arg2->d.x < Arg1->d.x) in ax
      fcom         st(2)               ; compare arg1, arg2
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit
      sahf
      setbe        al                  ; return (Arg1 <= Arg2) in AX
      xor          ah,ah
   END_INCL        LT2
; --------------------------------------------------------------------------
   BEGN_OPER       LodLT               ; load, LT
   ; return (1,0) on stack if arg2 < arg1
      FIXUP        LodLT, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      jae          short LodLTfalse    ; jump when arg2 >= arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodLT
LodLTfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodLT
; --------------------------------------------------------------------------
   BEGN_OPER       LodLT2              ; Lod, LT, set AX, clear FPU
   ; returns !(Arg2->d.x < Arg1->d.x) in ax
      FIXUP        LodLT2, fcom, X     ; compare arg2, arg1
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit                           ; clear fpu
      sahf
      setae        al                  ; set al when arg2 >= arg1
      xor          ah,ah               ; clear ah
   END_OPER        LodLT2              ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTMul            ; Lod, LT, Multiply (needs 4 on stack)
   ; for '<expr> * ( <expr> < <var> )'
   ; return number on stack if arg2 < arg1
      FIXUP        LodLTMul, fcomp, X  ; comp Arg2 to Arg1, pop st
      fstsw        ax                  ; save status
      POP_STK      1                   ; clear 1 from stack
      sahf
      jae          short LodLTMulfalse ; jump if arg2 >= arg1
      EXIT_OPER    LodLTMul            ; return value on st
   PARSALIGN
LodLTMulfalse:
      POP_STK      2                   ; return (0,0)
      fldz
      fldz
   END_OPER        LodLTMul
; --------------------------------------------------------------------------
   BEGN_INCL       GT                  ; >
   ; Arg2->d.x = (double)(Arg2->d.x > Arg1->d.x);
      fcomp        st(2)               ; compare arg1, arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jae          short GTfalse       ; jump if Arg1 >= Arg2
      fld1                             ; 1 0, return arg2 > arg1
      EXIT_OPER    GT
GTfalse:
      fldz                             ; 0 0
   END_INCL        GT
; --------------------------------------------------------------------------
   BEGN_INCL       GT2                 ; GT, set AX, clear FPU
   ; returns !(Arg2->d.x > Arg1->d.x) in ax
      fcom         st(2)               ; compare arg1, arg2
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit
      sahf
      setae        al                  ; return (Arg1 >= Arg2) in AX
      xor          ah,ah
   END_INCL        GT2
; --------------------------------------------------------------------------
   BEGN_OPER       LodGT               ; load, GT
   ; return (1,0) on stack if arg2 > arg1
      FIXUP        LodGT, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      jbe          short LodGTfalse    ; jump when arg2 <= arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodGT
LodGTfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodGT
; --------------------------------------------------------------------------
   BEGN_OPER       LodGT2              ; Lod, GT, set AX, clear FPU
   ; returns !(Arg2->d.x > Arg1->d.x) in AX
      FIXUP        LodGT2, fcom, X     ; compare arg2, arg1
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit                           ; clear fpu
      sahf
      setbe        al                  ; set al when arg2 <= arg1
      xor          ah,ah               ; clear ah
   END_OPER        LodGT2              ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_INCL       LTE                 ; <=
   ; Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
      fcomp        st(2)               ; y x y, comp Arg1 to Arg2
      fstsw        ax                  ; save status now
      POP_STK      3
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      sahf
      jb           short LTEfalse      ; jump if arg1 > arg2
      fld1                             ; 1 0, ret arg2 <= arg1
      EXIT_OPER    LTE
LTEfalse:
      fldz                             ; 0 0
   END_INCL        LTE
; --------------------------------------------------------------------------
   BEGN_INCL       LTE2                ; LTE, test ST, clear
   ; return !(Arg2->d.x <= Arg1->d.x) in AX
      fcom         st(2)               ; comp Arg1 to Arg2
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit                           ; clear stack
      and          ah,1                ; mask cf
      shr          ax,8                ; ax=1 when arg1 < arg1
   END_INCL        LTE2                ; return (Arg1 < Arg2),
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTE              ; load, LTE
   ; return (1,0) on stack if arg2 <= arg1
      FIXUP        LodLTE, fcomp, X    ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      sahf
      fldz                             ; 0 ...
      ja           short LodLTEfalse   ; jump when arg2 > arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodLTE
LodLTEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodLTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTE2             ; Load, LTE, test ST, clear
   ; return !(Arg2->d.x <= Arg1->d.x) in AX
      FIXUP        LodLTE2, fcom, X    ; comp Arg2 to Arg1
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit
      sahf
      seta         al
      xor          ah,ah               ; ax=1 for expr. false
   END_OPER        LodLTE2             ; return (Arg2 > Arg1)
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTEMul           ; Lod, LTE, Multiply (needs 4 on stk)
   ; for '<expr> * ( <expr> <= <var> )'
   ; return number on stack if arg2 <= arg1
      FIXUP        LodLTEMul, fcomp, X ; comp Arg2 to Arg1, pop st
      fstsw        ax                  ; save status
      POP_STK      1                   ; clear 1 from stack
      sahf
      ja           short LodLTEMulfalse ; jump if arg2 > arg1
      EXIT_OPER    LodLTEMul           ; return value on st
   PARSALIGN
LodLTEMulfalse:
      POP_STK      2                   ; return (0,0)
      fldz
      fldz
   END_OPER        LodLTEMul
; --------------------------------------------------------------------------
   BEGN_OPER       LodLTEAnd2          ; Load, LTE, AND, test ST, clear
   ; this is for 'expression && (expression <= value)'
   ; stack has {arg2.x arg2.y logical.x junk} on entry (arg1 in memory)
   ; Arg2->d.x = (double)(Arg2->d.x <= Arg1->d.x);
      FIXUP        LodLTEAnd2, fcom, X ; comp Arg2 to Arg1
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      sahf
      fxch         st(2)               ; logical.x arg2.y arg2.x junk ...
      ja           LTEA2RFalse         ; right side is false, Arg2 > Arg1
      ftst                             ; now see if left side of expr is true
      fstsw        ax
      sahf
      fninit                           ; clear fpu
      jz           LTEA2LFalse         ; jump if left side of && is false
      xor          ax,ax               ; return zero in ax for expr true
      ret                              ; changed EXIT_OPER->ret  CAE 30DEC93
LTEA2RFalse:
      fninit
LTEA2LFalse:
      mov          ax,1                ; return ax=1 for condition false
   END_OPER        LodLTEAnd2
; --------------------------------------------------------------------------
   BEGN_INCL       GTE                 ; >=
   ; Arg2->d.x = (double)(Arg2->d.x >= Arg1->d.x);
      fcomp        st(2)               ; y x y (compare arg1,arg2)
      fstsw        ax
      POP_STK      3                   ; clear 3 from stk
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      ja           short GTEfalse      ; jmp if arg1 > arg2
      fld1                             ; 1 0 (return arg2 >= arg1 on stack)
      EXIT_OPER    GTE
GTEfalse:
      fldz                             ; 0 0
   END_INCL        GTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodGTE              ; load, GTE
   ; return (1,0) on stack if arg2 >= arg1
      FIXUP        LodGTE, fcomp, X    ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      jb           short LodGTEfalse   ; jump when arg2 < arg1
      fld1                             ; 1 0 ...
      EXIT_OPER    LodGTE
LodGTEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodGTE
; --------------------------------------------------------------------------
   BEGN_OPER       LodGTE2             ; Lod, GTE, set AX, clear FPU
   ; return !(Arg2->d.x >= Arg1->d.x) in AX
      FIXUP        LodGTE2, fcom, X    ; compare arg2, arg1
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      fninit                           ; clear fpu
      and          ah,1                ; mask cf
      shr          ax,8                ; shift it (AX = 1 when arg2 < arg1)
   END_OPER        LodGTE2             ; ret 0 in ax for true, 1 for false
; --------------------------------------------------------------------------
   BEGN_INCL       EQ                  ; ==
   ; Arg2->d.x = (double)(Arg2->d.x == Arg1->d.x);
      fcomp        st(2)               ; compare arg1, arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz                             ; 0 (Arg2->d.y = 0.0;)
      jne          short EQfalse       ; jmp if arg1 != arg2
      fld1                             ; 1 0 (ret arg2 == arg1)
      EXIT_OPER    EQ
EQfalse:
      fldz
   END_INCL        EQ
; --------------------------------------------------------------------------
   BEGN_OPER       LodEQ               ; load, EQ
   ; return (1,0) on stack if arg2 == arg1
      FIXUP        LodEQ, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      jne          short LodEQfalse    ; jump when arg2 != arg1
      fld1                             ; 1 0 ... (return arg2 == arg1)
      EXIT_OPER    LodEQ
LodEQfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodEQ
; --------------------------------------------------------------------------
   BEGN_INCL       NE                  ; !=
   ; Arg2->d.x = (double)(Arg2->d.x != Arg1->d.x);
      fcomp        st(2)               ; compare arg1,arg2
      fstsw        ax
      POP_STK      3
      sahf
      fldz
      je           short NEfalse       ; jmp if arg1 == arg2
      fld1                             ; ret arg2 != arg1
      EXIT_OPER    NE
NEfalse:
      fldz
   END_INCL        NE
; --------------------------------------------------------------------------
   BEGN_OPER       LodNE               ; load, NE
   ; return (1,0) on stack if arg2 != arg1
      FIXUP        LodNE, fcomp, X     ; compare arg2 to arg1, pop st
      fstsw        ax                  ; y ...
      POP_STK      1                   ; ...
      fldz                             ; 0 ...
      sahf
      je           short LodNEfalse    ; jump when arg2 == arg1
   ; CAE changed above 'jne' to 'je'                              9 MAR 1993
      fld1                             ; 1 0 ...
      EXIT_OPER    LodNE
LodNEfalse:
      fldz                             ; 0 0 ...
   END_OPER        LodNE
; --------------------------------------------------------------------------
   BEGN_INCL       OR                  ; Or
   ; Arg2->d.x = (double)(Arg2->d.x || Arg1->d.x);
      ftst                             ; a1.x a1.y a2.x a2.y ...
      fstsw        ax
      sahf
      POP_STK      2                   ; a2.x a2.y ...
      jnz          short Arg1True
      ftst
      fstsw        ax
      sahf
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      jz           short NoneTrue
      fld1                             ; 1 0 ...
      EXIT_OPER    OR
   PARSALIGN
Arg1True:
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      fld1                             ; 1 0 ...
      EXIT_OPER    OR
NoneTrue:                              ; 0 ...
      fldz                             ; 0 0 ...
   END_INCL        OR
; --------------------------------------------------------------------------
   BEGN_INCL       AND                 ; And
   ; Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
      ftst                             ; a1.x a1.y a2.x a2.y ...
      fstsw        ax
      sahf
      POP_STK      2                   ; a2.x a2.y ...
      jz           short Arg1False
      ftst
      fstsw        ax
      sahf
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
      jz           short Arg2False
      fld1                             ; 1 0 ...
      EXIT_OPER    AND
   PARSALIGN
Arg1False:
      POP_STK      2                   ; ...
      fldz                             ; 0 ...
Arg2False:
      fldz                             ; 0 0 ...
   END_INCL        AND
; --------------------------------------------------------------------------
   BEGN_INCL       ANDClr2             ; And, test ST, clear FPU
   ; for bailouts using <condition> && <condition>
   ;  Arg2->d.x = (double)(Arg2->d.x && Arg1->d.x);
   ;  Returns !(Arg1 && Arg2) in ax
      ftst                             ; y.x y.y x.x x.y
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      sahf
      jz           short Arg1False2
      fxch         st(2)               ; x.x y.y y.x x.y
      ftst
      fstsw        ax
      sahf
      fninit
      jz           short Arg2False2
BothTrue2:
      xor          ax,ax
      ret                              ; changed EXIT_OPER->ret  CAE 30DEC93
Arg1False2:
      fninit
Arg2False2:
      mov          ax,1
   END_INCL        ANDClr2
; --------------------------------------------------------------------------
   BEGN_INCL       ORClr2           ; Or, test ST, clear FPU      CAE 6NOV93
   ; for bailouts using <condition> || <condition>
   ;  Arg2->d.x = (double)(Arg2->d.x || Arg1->d.x);
   ;  Returns !(Arg1 || Arg2) in ax
      ftst                             ; y.x y.y x.x x.y
      fstsw        ax
                                       ;                      CAE 1 Dec 1998
      ALTER_RET_ADDR                   ; change return address on stack

      sahf
      jnz          short ORArg1True
      fxch         st(2)               ; x.x y.y y.x x.y
      ftst
      fstsw        ax
      sahf
      fninit
      jnz          short ORArg2True
ORNeitherTrue:
      mov          ax,1
      ret                              ; changed EXIT_OPER->ret  CAE 30DEC93
ORArg1True:
      fninit
ORArg2True:
      xor          ax,ax
   END_INCL        ORClr2

; --------------------------------------------------------------------------
   assume          ds:DGROUP, es:nothing
; --------------------------------------------------------------------------

   ifndef          COMPILER

; --------------------------------------------------------------------------
; called once per image
; --------------------------------------------------------------------------
   public          _Img_Setup
   align           2
   ; Changed to FAR, FRAME/UNFRAME added by CAE 09OCT93
_Img_Setup         proc far
      FRAME        <si,di>
      les          si,_pfls            ; es:si = &pfls[0]

      mov          di,_LastOp          ; load index of lastop

      dec          di                  ; flastop now points at last operator
      ; above added by CAE 09OCT93 because of loop logic changes

      shl          di,2                ; convert to offset
      mov          bx,offset DGROUP:_fLastOp ; set bx for store
      add          di,si               ; di = offset lastop
      mov          WORD PTR [bx],di    ; save value of flastop
      mov          ax,es               ; es has segment value
      mov          WORD PTR [bx+2],ax  ; save seg for easy reload
      mov          ax,word ptr _v      ; build a ptr to Z
      add          ax,3*CARG+CPFX
      mov          _PtrToZ,ax          ; and save it
      UNFRAME      <di,si>
      ret
_Img_Setup         endp
; --------------------------------------------------------------------------
;  Hybrid orbitcalc/per-pixel routine (tested, but not implemented.)
;
;  To implement, stick the following code in calcfrac.c around line 788,
;     just before the line that says "while (++coloriter < maxit)".
; --------------------------------------------------------------------------
;  if (curfractalspecific->orbitcalc == fFormula  /* 387 parser  */
;        && periodicitycheck == 0
;        && !show_orbit
;        && inside >= -5
;        && attractors == 0
;        && !distest ){
;     fFormulaX();  /* orbit till done  */
;  } else
; --------------------------------------------------------------------------
   public          _fFormulaX          ;                         CAE 09OCT93
   align           16
_fFormulaX         proc far
      push         si
      push         di
      mov          ax,WORD PTR _InitJumpIndex
      mov          WORD PTR _jump_index,ax
      mov          edx,_maxit          ; edx holds coloriter during loop
      mov          _coloriter,edx      ; set coloriter to maxit
      mov          ax,ds               ; save ds in ax
      lds          cx,_fLastOp         ; ds:cx -> one past last token
      mov          es,ax               ; es -> DGROUP
   assume          es:DGROUP, ds:nothing ; swap es, ds before any fn. calls
      jmp          short skipfirst     ; skip bailout test first time
   align           16
outer_loop:
      or           ax,ax               ; did bailout occur?
      jnz          short doneloop      ; yes, exit
skipfirst:
      dec          edx                 ; ++coloriter
      jle          short doneloop      ; yes, exit because of maxiter
      mov          bx,_InitOpPtr       ; bx -> one before first token
      mov          di,offset DGROUP:_s ; reset stk overflow ptr
   align           16
inner_loop2:
      cmp          bx,cx               ; time to quit yet?
      jae          short outer_loop    ; yes, bx points to last function
      add          bx,4                ; point to next pointer pair
      push         offset PARSERA_TEXT:inner_loop2 ; do this first
      mov          si,WORD PTR [bx+2]  ; set si to operand pointer
      jmp          WORD PTR [bx]       ; jmp to operator fn
   align           16
doneloop:
   ; NOTE: edx must be preserved here.
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_new ; es:di -> new
      mov          cx,4
      rep          movsd               ; new = z
      mov          ax,es
      pop          di
      pop          si
      mov          ds,ax               ; restore ds before return
   assume          ds:DGROUP, es:nothing
      sub          _coloriter,edx      ; now put new coloriter back from edx
      ret
_fFormulaX         endp
; --------------------------------------------------------------------------
;       orbitcalc function follows
; --------------------------------------------------------------------------
   public          _fFormula
   align           16
_fFormula          proc far
      push         di                  ; don't build a frame here
      mov          di,offset DGROUP:_s ; reset this for stk overflow area
      mov          bx,_InitOpPtr       ; bx -> one before first token
      mov          ax,WORD PTR _InitJumpIndex
      mov          WORD PTR _jump_index,ax
      mov          ax,ds               ; save ds in ax
      lds          cx,_fLastOp         ; ds:cx -> last token
      mov          es,ax               ; es -> DGROUP
   assume          es:DGROUP, ds:nothing
      push         si

   ;;;;align           8
inner_loop:                            ; new loop             CAE 1 Dec 1998
      mov          si,WORD PTR [bx+2]
      call         WORD PTR [bx]
;      mov          si,WORD PTR [bx+6]  ; now set si to operand pointer
;      call         WORD PTR [bx+4]     ; ...and jump to operator fn
;      add          bx,8     ; JCO removed loop unroll, 12/5/99
      add          bx,4
      jmp          short inner_loop

   ;;;;align           8
past_loop:
   ; NOTE: AX was set by the last operator fn called.
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_new ; es:di -> new
      mov          cx,4                ; get ready to move 4 dwords
      rep          movsd               ; new = z
      mov          bx,es               ; put seg dgroup in bx
      pop          si
      pop          di                  ; restore si, di
      mov          ds,bx               ; restore ds from bx before return
   assume          ds:DGROUP, es:nothing
      ret                              ; return AX unmodified
_fFormula          endp
; --------------------------------------------------------------------------
   public          _fform_per_pixel    ; called once per pixel
   align           4
_fform_per_pixel   proc far
      FRAME        <si, di>
   ;    if((row+col)&1)
      mov          ax,_row             ; ax = row
      add          ax,_col             ; ax = row+col
      and          ax,1                ; ax = (row+col)&1
      les          bx,_v               ; load pointer to constants
      cmp          ax,0                ; zero?
      je           checker_is_0
   ;      v[9].a.d.x = 1.0;            ; not zero, set whitesq.x=1.0
      fld1                             ; constant 1.0 to ST
      fstp         QWORD PTR es:[bx+WHITESQ]  ; copy ST to whitesq.x
      jmp          checker_is_1
checker_is_0:                          ; is zero, set whitesq to (0,0)
   ;      v[9].a.d.y = 0.0;
      fldz                             ; load constant zero to ST
      fstp         QWORD PTR es:[bx+WHITESQ]  ; copy ST to whitesq.x
checker_is_1:
      fldz
      fstp         QWORD PTR es:[bx+WHITESQ+8]
   ;    v[10].a.d.x = (double)col;
      fild         _col                ; ST  = col
      fstp         QWORD PTR es:[bx+SCRNPIX] ; scrnpix.x = col
   ;    v[10].a.d.y = (double)row;
      fild         _row                ; ST  = row
      fstp         QWORD PTR es:[bx+SCRNPIX+8] ; scrnpix.y = row
      mov          _jump_index,0        ;jump_index = 0
      cmp          _invert,0            ; inversion support added
      je           skip_invert          ;                        CAE 08FEB95
      mov          si,offset DGROUP:_old
      push         si
      call         far ptr _invertz2
      add          sp,2
      ; now copy old to v[0].a.d
      les          di,_v                ; ds:si already points to old
      add          di,CPFX              ; make es:di point to v[0].a.d
      mov          cx,4
      rep          movsd
      jmp          after_load
skip_invert:
      cmp          _use_grid,0          ; inversion support added
      je           skip_grid  
   ;   v[0].a.d.x = dx0[col]+dShiftx;
      mov          ax,_col
      shl          ax,3
      les          bx,_dx0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_row
      shl          ax,3
      les          bx,_dx1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX]
   ;  v[0].a.d.y = dy0[row]+dShifty;
      mov          ax,_row
      shl          ax,3
      les          bx,_dy0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_col
      shl          ax,3
      les          bx,_dy1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX+8]
      jmp          after_load
skip_grid:
   ;  v[0].a.d.x = (double)(xxmin + col*delxx + row*delxx2); 
      fild         WORD PTR _row
      fld          TBYTE PTR _delxx2
      fmulp        st(1),st(0)
      fild         WORD PTR _col
      fld          TBYTE PTR _delxx
      fmulp        st(1),st(0)
      faddp        st(1),st(0)
      fadd         QWORD PTR _xxmin
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX]
      fwait
   ;  v[0].a.d.y = (double)(yymax - row*delyy - col*delyy2); */
      fild         WORD PTR _row
      fld          TBYTE PTR _delyy
      fmulp        st(1),st(0)
      fsubr        QWORD PTR _yymax
      fild         WORD PTR _col
      fld          TBYTE PTR _delyy2
      fmulp        st(1),st(0)
      fsubp        st(1),st(0)
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX+8]
after_load:
      mov          di,offset DGROUP:_s ; di points to stack overflow area
      mov          ax,ds
      mov          bx,WORD PTR _pfls   ; bx -> pfls
      lds          cx,_fLastOp         ; cx = offset &f[LastOp],load ds
      mov          es,ax
   assume          es:DGROUP, ds:nothing
      cmp          _LastInitOp,0
      je           short skip_initloop ; no operators to do here
      mov          _LastInitOp,cx      ; lastinitop=lastop
      jmp          short pixel_loop
   align           16
pixel_loop:
      mov          si,WORD PTR [bx+2]  ; get address of load or store
      call         WORD PTR [bx]       ; (*opptr)()
      add          bx,4                ; ++opptr
      cmp          bx,_LastInitOp
      jb           short pixel_loop
skip_initloop:
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_old ; es:di -> old
      mov          cx,4                ; get ready to move 4 dwords
      rep          movsd               ; old = z
      mov          ax,es
      mov          ds,ax
   assume          ds:DGROUP, es:nothing ; for the rest of the program
;
;                                      ; subtract removed     CAE 1 Dec 1998
      mov          _InitOpPtr, bx      ; InitOptPtr = OpPtr;
      UNFRAME      <di, si>
      xor          ax,ax
      ret
_fform_per_pixel   endp
; --------------------------------------------------------------------------

   else  ; Compiler

; --------------------------------------------------------------------------
; . . . and now for the real fun!
; --------------------------------------------------------------------------
   public          _Img_Setup
   align           2
_Img_Setup         proc far
      mov          ax,word ptr _v      ; build a ptr to Z
      add          ax,3*CARG+CPFX
      mov          _PtrToZ,ax          ; and save it
      ret
_Img_Setup         endp
; --------------------------------------------------------------------------
;  Hybrid orbitcalc/per-pixel routine.
; --------------------------------------------------------------------------
   public          _fFormulaX
   align           16
_fFormulaX         proc far
      push         si
      push         di
      mov          edx,_maxit          ; edx holds coloriter during loop
      mov          _coloriter,edx      ; set coloriter to maxit
      mov          ax,ds               ; save ds in ax
      mov          cx,word ptr _pfls+2 ; just get the seg part
      mov          es,ax               ; es -> DGROUP
      mov          ds,cx               ; ds -> parser data
   assume          es:DGROUP, ds:nothing
      jmp          short skipfirst     ; skip bailout test first time
   align           16
outer_loop:
      or           ax,ax               ; did bailout occur?
      jnz          short doneloop      ; yes, exit
skipfirst:
      dec          edx                 ; ++coloriter, was maxiter reached?
      jle          short doneloop      ; yes, exit because of maxiter
      push         offset PARSERA_TEXT:outer_loop
      mov          di,offset DGROUP:_s ; reset this for stk overflow area
      jmp          _compiled_fn_2      ; call the compiled code
doneloop:
   ; NOTE: edx must be preserved here.
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_new ; es:di -> new
      mov          cx,4
      rep          movsd               ; new = z
      mov          ax,es
      pop          di
      pop          si
      mov          ds,ax               ; restore ds before return
   assume          ds:DGROUP, es:nothing
      sub          _coloriter,edx      ; now put new coloriter back from edx
      ret
_fFormulaX         endp
; --------------------------------------------------------------------------
;       orbitcalc function follows
; --------------------------------------------------------------------------
   public          _fFormula
   align           16
_fFormula          proc far
      push         di                  ; don't build a frame here
      mov          di,offset DGROUP:_s ; reset this for stk overflow area
      mov          ax,ds               ; save ds in ax
      mov          cx,WORD PTR _pfls+2 ; just load seg value
      mov          es,ax               ; es -> DGROUP
      mov          ds,cx               ; ds -> parser data
   assume          es:DGROUP, ds:nothing
      push         si                  ; compiled_fn modifies si
      call         _compiled_fn_2      ; call the compiled code
   ; NOTE: AX was set by the compiled code and must be preserved here.
      mov          si,_PtrToZ          ; ds:si -> z
      mov          di,offset DGROUP:_new ; es:di -> new
      mov          cx,4                ; get ready to move 4 dwords
      rep          movsd               ; new = z
      mov          bx,es               ; put seg dgroup in bx
      pop          si
      pop          di                  ; restore si, di
      mov          ds,bx               ; restore ds from bx before return
   assume          ds:DGROUP, es:nothing
      ret                              ; return AX unmodified
_fFormula          endp
; --------------------------------------------------------------------------
   public          _fform_per_pixel    ; called once per pixel
   align           4
_fform_per_pixel   proc far
      FRAME        <si, di>
      cmp          _invert,0            ; inversion support added
      je           skip_invert          ;                        CAE 08FEB95
      mov          si,offset DGROUP:_old
      push         si
      call         far ptr _invertz2
      add          sp,2
      ; now copy old to v[0].a.d
      les          di,_v                ; ds:si already points to old
      add          di,CPFX              ; make es:di point to v[0].a.d
      mov          cx,4
      rep          movsd
      jmp          after_load
skip_invert:
      cmp          _use_grid,0          ; inversion support added
      je           skip_grid  
   ;   v[0].a.d.x = dx0[col]+dShiftx;
      mov          ax,_col
      shl          ax,3
      les          bx,_dx0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_row
      shl          ax,3
      les          bx,_dx1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX]
   ;  v[0].a.d.y = dy0[row]+dShifty;
      mov          ax,_row
      shl          ax,3
      les          bx,_dy0
      add          bx,ax
      fld          QWORD PTR es:[bx]
      mov          ax,_col
      shl          ax,3
      les          bx,_dy1
      add          bx,ax
      fadd         QWORD PTR es:[bx]
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX+8]
      jmp          after_load
skip_grid:
   ;  v[0].a.d.x = (double)(xxmin + col*delxx + row*delxx2); 
      fild         WORD PTR _row
      fld          TBYTE PTR _delxx2
      fmulp        st(1),st(0)
      fild         WORD PTR _col
      fld          TBYTE PTR _delxx
      fmulp        st(1),st(0)
      faddp        st(1),st(0)
      fadd         QWORD PTR _xxmin
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX]
      fwait     
   ;  v[0].a.d.y = (double)(yymax - row*delyy - col*delyy2); */
      fild         WORD PTR _row
      fld          TBYTE PTR _delyy
      fmulp        st(1),st(0)
      fsubr        QWORD PTR _yymax
      fild         WORD PTR _col
      fld          TBYTE PTR _delyy2
      fmulp        st(1),st(0)
      fsubp        st(1),st(0)
      les          bx,_v
      fstp         QWORD PTR es:[bx+CPFX+8] 
after_load:
      mov          di,offset DGROUP:_s ; di points to stack overflow area
      mov          ax,ds
      mov          cx,word ptr _pfls+2 ; just to load ds
      mov          es,ax               ; es -> DGROUP
      mov          ds,cx               ; ds -> parser data
   assume          es:DGROUP, ds:nothing
      call         _compiled_fn_1      ; call compiled code
      mov          ax,es
      mov          ds,ax
   assume          ds:DGROUP, es:nothing ; for the rest of the program
      UNFRAME      <di, si>
      xor          ax,ax
      ret
_fform_per_pixel   endp

   align           16
   public          _compiled_fn_1
_compiled_fn_1     proc near
      retn                             ; compiled code will be put here
      db           1023 DUP (?)
_compiled_fn_1     endp

   align           16
   public          _compiled_fn_2
_compiled_fn_2     proc near
      retn                             ; ...and here
      db           1023 DUP (?)
_compiled_fn_2     endp
; --------------------------------------------------------------------------

   endif  ; COMPILER

; --------------------------------------------------------------------------


PARSERA_TEXT      ends
   end
