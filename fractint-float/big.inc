; big.inc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The following allows the programmer to customize routines.
; They can be selected here or on the assembler command line.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; different pointer models: near, based, far
; huge pointers is not supported in assembly, only in C
; uncomment only ONE of these
;BIG_NEAR  equ <>
BIG_BASED equ <>
;BIG_FAR   equ <>

; different cpu usage:
; uncomment at least one of these
; uncomment both if cpu to be determined at run time
BIG16 equ <>       ; <= 286
BIG32 equ <>       ; >= 386

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The rest is determined by the assembler
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF BIG_NEAR
IFNDEF BIG_BASED
IFNDEF BIG_FAR
.err <select only one of the following: BIG_NEAR, BIG_BASED, or BIG_FAR>
ENDIF
ENDIF
ENDIF

IFDEF BIG16
IFDEF BIG32
BIG16AND32 equ 1
ENDIF
ENDIF

IFNDEF BIG16
IFNDEF BIG32
.err <Either BIG16 or BIG32 must be defined>
ENDIF
ENDIF

IF @DataSize gt 0
.err <Only NEAR data models can be used (SMALL or MEDIUM)>
ENDIF

;external variables

extern bnlength:word, rlength:word, bflength:word, intlength:word, fpu:word
IFDEF BIG16AND32
extern cpu:word
ENDIF
IFNDEF BIG_FAR  ; defined for BIG_BASED and BIG_NEAR
extern bignum_seg:word
ENDIF
