; the code was originally written for MASM 6.0b
; the following makes it compatible with various masm/tasm versions


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IFDEF @Version          ; microsoft

IF @Version ge 600      ; masm 6.0 or later
IFDEF BIG_NEAR
big_t TYPEDEF near ptr byte   ; near pointer to bigfloat array
ELSEIFDEF BIG_BASED
big_t TYPEDEF near ptr byte   ; based pointer to bigfloat array
ELSEIFDEF BIG_FAR
big_t TYPEDEF far ptr byte    ; far pointer to bigfloat array
ELSE
.err <BIG_NEAR, or BIG_BASED, or BIG_FAR must be defined>
ENDIF

bn_t equ <big_t>
bf_t equ <big_t>
clear_bf PROTO, n:bf_t
neg_a_bf PROTO, n:bf_t

ENDIF

IF @Version lt 600      ; masm 5.1 or qasm 2.01, 2.51

real10 equ <tbyte>
REAL10 equ <tbyte>
extern equ <extrn>
sword  equ <word>

IFDEF BIG_NEAR
big_t equ <near ptr byte>     ; near pointer to bigfloat array
ELSEIFDEF BIG_BASED
big_t equ <near ptr byte>     ; based pointer to bigfloat array
ELSEIFDEF BIG_FAR
big_t equ <far ptr byte>      ; far pointer to bigfloat array
ENDIF

bn_t equ <big_t>
bf_t equ <big_t>

extrn clear_bf:far
extrn neg_a_bf:far

; the invoke macro is designed to emulate the masm 6 invoke command
invoke macro prc,a,b,c,d,e,f,g      ; simple to add more args if necessary

  pushsize = 0
  IRP     x, <g,f,e,d,c,b,a> ;; push in reverse order, C style
     IFNB <x>
	argsize = size x
        REPT argsize/2
	   argsize  = argsize  - 2
	   push  word ptr x + argsize
	   pushsize = pushsize + 2
        ENDM
        IF argsize eq 1    ;; byte
	   mov   al, x  ;; this is exactly what masm 6 invoke does
	   push  ax
	   pushsize = pushsize + 2
        ENDIF
     ENDIF
  ENDM

  call      prc                  ;; call actual procedure
  add       sp, pushsize         ;; pop the stack pointer
ENDM

IF @Version eq 520      ; qasm 2.01, 2.51
IFDEF BIG32
.ERR <Quick Assembler does not support 386 instructions with BIG32>
ENDIF
ENDIF ; IF @Version eq 520

ENDIF ;IF @Version lt 600
ENDIF ;@Version ; microsoft

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IFDEF ??version      ; tasm
   ideal
TYPEDEF big_t far ptr byte
bn_t equ <big_t>
bf_t equ <big_t>

   masm
   masm51
   quirks
extern equ extrn
real10 equ tbyte
REAL10 equ tbyte
sword  equ word

invoke equ <call>

extrn clear_bf:far
extrn neg_a_bf:far
ENDIF ;??version      ; tasm
