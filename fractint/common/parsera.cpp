#include <cassert>

#include "port.h"
#include "prototyp.h"

#include "parsera.h"

#define FN(name_) void fStk##name_() { assert(0 && "Called " #name_); }

FN(Abs)
FN(ACos)
FN(ACosh)
FN(Add)
FN(AND)
FN(ASin)
FN(ASinh)
FN(ATan)
FN(ATanh)
FN(CAbs)
FN(Ceil)
FN(Clr1)
FN(Conj)
FN(Cos)
FN(Cosh)
FN(CosXX)
FN(CoTan)
FN(CoTanh)
FN(Div)
FN(EndInit)
FN(EQ)
FN(Exp)
FN(Flip)
FN(Floor)
FN(GT)
FN(GTE)
FN(Ident)
FN(Imag)
FN(Jump)
FN(JumpLabel)
FN(JumpOnFalse)
FN(JumpOnTrue)
FN(Lod)
FN(Log)
FN(LT)
FN(LTE)
FN(Mod)
FN(Mul)
FN(NE)
FN(Neg)
FN(One)
FN(OR)
FN(Pwr)
FN(Real)
FN(Recip)
FN(Round)
FN(Sin)
FN(Sinh)
FN(Sqr)
FN(Sqrt)
FN(Sto)
FN(Sub)
FN(Tan)
FN(Tanh)
FN(Trunc)
FN(Zero)
FN(ANDClr2)
FN(Clr2)
FN(Dbl)
FN(GT2)
FN(ImagFlip)
FN(LodAdd)
FN(LodConj)
FN(LodDbl)
FN(LodDup)
FN(LodEQ)
FN(LodGT)
FN(LodGT2)
FN(LodGTE)
FN(LodGTE2)
FN(LodImag)
FN(LodImagAbs)
FN(LodImagAdd)
FN(LodImagFlip)
FN(LodImagMul)
FN(LodImagSub)
FN(LodLT)
FN(LodLT2)
FN(LodLTE)
FN(LodLTE2)
FN(LodLTEAnd2)
FN(LodLTEMul)
FN(LodLTMul)
FN(LodMod2)
FN(LodMul)
FN(LodNE)
FN(LodReal)
FN(LodRealAbs)
FN(LodRealAdd)
FN(LodRealC)
FN(LodRealFlip)
FN(LodRealMul)
FN(LodRealPwr)
FN(LodRealSub)
FN(LodSqr)
FN(LodSqr2)
FN(LodSub)
FN(LodSubMod)
FN(LT2)
FN(LTE2)
FN(Mod2)
FN(ORClr2)
FN(PLodAdd)
FN(PLodSub)
FN(Pull2)
FN(Push2)
FN(Push2a)
FN(Push4)
FN(Real2)
FN(RealFlip)
FN(Sqr0)
FN(Sqr3)
FN(Sto2)
FN(StoClr1)
FN(StoClr2)
FN(StoDbl)
FN(StoDup)
FN(StoMod2)
FN(StoSqr)
FN(StoSqr0)

#undef FN

int formula_per_pixel_fp()
{
	assert(0 && "formula_per_pixel_fp called.");
	return 0;
}

/*
; --------------------------------------------------------------------------
;       orbitcalc function follows
; --------------------------------------------------------------------------
	public          _fFormula
	align           16
_fFormula          proc far
	push         di                  ; don't build a frame here
	mov          di, offset DGROUP:_s ; reset this for stk overflow area
	mov          bx, _InitOpPtr       ; bx -> one before first token
	mov          ax, WORD PTR _InitJumpIndex
	mov          WORD PTR _jump_index, ax
	mov          ax, ds               ; save ds in ax
	lds          cx, _fLastOp         ; ds:cx -> last token
	mov          es, ax               ; es -> DGROUP
	assume          es:DGROUP, ds:nothing
	push         si

	; ;; ;align           8
inner_loop:                            ; new loop             CAE 1 Dec 1998
	mov          si, WORD PTR [bx + 2]
	call         WORD PTR [bx]
;      mov          si, WORD PTR [bx + 6]  ; now set si to operand pointer
;      call         WORD PTR [bx + 4]     ; ...and jump to operator fn
;      add          bx, 8     ; JCO removed loop unroll, 12/5/99
	add          bx, 4
	jmp          short inner_loop

	; ;; ;align           8
past_loop:
	; NOTE: AX was set by the last operator fn called.
	mov          si, _PtrToZ          ; ds:si -> z
	mov          di, offset DGROUP:_new ; es:di -> new
	mov          cx, 4                ; get ready to move 4 dwords
	rep          movsd               ; new = z
	mov          bx, es               ; put seg dgroup in bx
	pop          si
	pop          di                  ; restore si, di
	mov          ds, bx               ; restore ds from bx before return
	assume          ds:DGROUP, es:nothing
	ret                              ; return AX unmodified
_fFormula          endp
*/
int formula_fp()
{
	assert(0 && "formula_fp called.");
	return 0;
}

/*
; --------------------------------------------------------------------------
; called once per image
; --------------------------------------------------------------------------
	public          _Img_Setup
	align           2
	; Changed to FAR, FRAME/UNFRAME added by CAE 09OCT93
_Img_Setup         proc far
	FRAME        <si, di>
	les          si, _pfls            ; es:si = &g_function_load_store_pointers[0]

	mov          di, _LastOp          ; load index of lastop

	dec          di                  ; flastop now points at last operator
	; above added by CAE 09OCT93 because of loop logic changes

	shl          di, 2                ; convert to offset
	mov          bx, offset DGROUP:_fLastOp ; set bx for store
	add          di, si               ; di = offset lastop
	mov          WORD PTR [bx], di    ; save value of flastop
	mov          ax, es               ; es has segment value
	mov          WORD PTR [bx + 2], ax  ; save seg for easy reload
	mov          ax, word ptr _v      ; build a ptr to Z
	add          ax, 3*CARG + CPFX
	mov          _PtrToZ, ax          ; and save it
	UNFRAME      <di, si>
	ret
_Img_Setup         endp
*/
void image_setup()
{
	assert(0 && "Img_Setup called.");
}
