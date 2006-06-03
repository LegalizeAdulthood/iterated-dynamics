; bignuma.asm

; based on:
; bbignuma.asm - asm routines for bignumbers
; Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
; based pointer version

; See BIGLIB.TXT for further documentation.

; general programming notes for bases pointer version
; ALL big_t pointers must have a segment value equal to bignum_seg.
; single arg procedures, p(r), r = bx (or si when required)
; two arg procedures,    p(r,n), r=di, n=bx(or si when required)
; two arg procedures,    p(n1,n2), n1=bx(or si when required), n2=di
; three arg proc,        p(r,n1,n2), r=di, n1=si, n2=bx
; unless otherwise noted, such as full_mult, mult, full_square, square

.MODEL medium, c

include big.inc
include bigport.inc

.DATA

.CODE
.8086

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = 0
clear_bn   PROC USES di, r:bn_t

        mov     cx, bnlength
        mov     di, word ptr r
        mov     es, bignum_seg          ; load pointer in es:di

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        sub     ax, ax                  ; clear ax
        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     stosw                   ; clear r, word at a time
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        sub     eax, eax                ; clear eax
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     stosd                   ; clear r, dword at a time
ENDIF

bottom:
.8086
        mov     ax, word ptr r          ; return r in ax
        ret

clear_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = max positive value
max_bn   PROC USES di, r:bn_t

        mov     cx, bnlength
        mov     di, word ptr r
        mov     es, bignum_seg          ; load pointer in es:di

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ax, 0FFFFh              ; set ax to max value
        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     stosw                   ; max out r, word at a time
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     eax, 0FFFFFFFFh         ; set eax to max value
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     stosd                   ; max out r, dword at a time
ENDIF

bottom:
.8086
        ; when the above stos is finished, di points to the byte past the end
        mov     byte ptr es:[di-1], 7Fh       ; turn off the sign bit

        mov     ax, word ptr r              ; return r in ax
        ret

max_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n
copy_bn   PROC USES di si, r:bn_t, n:bn_t

        mov     ax, ds                  ; save ds for later
        mov     cx, bnlength
        mov     di, word ptr r
        mov     es, bignum_seg          ; load pointer in es:di
        mov     si, word ptr n

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load pointer in ds:si for movs

        shr     cx, 1                   ; 1 byte = 1/2 word
        rep     movsw                   ; copy word at a time
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load pointer in ds:si for movs

        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     movsd                   ; copy dword at a time
ENDIF

bottom:
.8086
        mov     ds, ax                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret

copy_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; n1 != n2 ?
; RETURNS: if n1 == n2 returns 0
;          if n1 > n2 returns a positive (steps left to go when mismatch occured)
;          if n1 < n2 returns a negative (steps left to go when mismatch occured)
cmp_bn   PROC USES di, n1:bn_t, n2:bn_t

        push    ds                      ; save DS
        mov     cx, bnlength
        mov     dx, cx                  ; save bnlength for later comparison
        mov     di, word ptr n2         ; load n2 pointer in di
        mov     bx, word ptr n1         ; load n1 pointer in bx

        add     bx, cx                  ; point to end of bignumbers
        add     di, cx                  ; where the msb is

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds
        shr     cx, 1                   ; byte = 1/2 word
top_loop_16:
        sub     bx, 2                   ; decrement to previous word
        sub     di, 2
        mov     ax, ds:[bx]             ; load n1
        cmp     ax, ds:[di]             ; compare to n2
        jne     not_match_16            ; don't match
        loop    top_loop_16
        jmp     match                   ; cx is zero
not_match_16:
        ; now determine which byte of the two did not match
        shl     cx, 1                   ; convert back to bytes
        cmp     ah, ds:[di+1]           ; compare to n2
        jne     bottom                  ; jump if ah doesn't match
        ; if ah does match, then mismatch was in al
        dec     cx                      ; decrement cx by 1 to show match
        cmp     al, ds:[di]             ; reset the flags for below
        jmp     bottom

ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds
        shr     cx, 2                   ; byte = 1/4 dword
top_loop_32:
        sub     bx, 4                   ; decrement to previous dword
        sub     di, 4
        mov     eax, ds:[bx]            ; load n1
        cmp     eax, ds:[di]            ; compare to n2
        jne     not_match_32            ; don't match
        loop    top_loop_32
        jmp     match                   ; cx is zero
not_match_32:
        ; now determine which byte of the four did not match
        shl     cx, 2                   ; convert back to bytes
        mov     ebx, eax
        shr     ebx, 16                 ; shift ebx_high to bx
        cmp     bh, ds:[di+3]           ; compare to n2
        jne     bottom                  ; jump if bh doesn't match
        dec     cx                      ; decrement cx by 1 to show match
        cmp     bl, ds:[di+2]           ; compare to n2
        jne     bottom                  ; jump if bl doesn't match
        dec     cx                      ; decrement cx by 1 to show match
        cmp     ah, ds:[di+1]           ; compare to n2
        jne     bottom                  ; jump if ah doesn't match
        ; if bh,bl,ah do match, then mismatch was in al
        dec     cx                      ; decrement cx by 1 to show match
        cmp     al, ds:[di]             ; reset the flags for below
        jmp     bottom

ENDIF

bottom:
.8086
; flags are still set from last cmp
; if cx == dx, then most significant part didn't match, use signed comparison
; else the decimals didn't match, use unsigned comparison
        lahf                            ; load results of last cmp
        cmp     cx, dx                  ; did they differ on very first cmp
        jne     not_first_step          ; no

        sahf                            ; yes
        jg      n1_bigger               ; signed comparison
        jmp     n2_bigger

not_first_step:
        sahf
        ja      n1_bigger               ; unsigned comparison

n2_bigger:
        neg     cx                      ; make it negative
n1_bigger:                              ; leave it positive
match:                                  ; leave it zero
        mov     ax, cx
        pop     ds                      ; restore DS
        ret

cmp_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r < 0 ?
; returns 1 if negative, 0 if positive or zero
is_bn_neg   PROC n:bn_t

        ; for a one-pass routine like this, don't bother with ds
        mov     bx, word ptr n
        mov     es, bignum_seg              ; load n pointer in es:bx

        add     bx, bnlength                ; find sign bit
        mov     al, es:[bx-1]               ; got it

        and     al, 80h                     ; check the sign bit
        rol     al, 1                       ; rotate sign big to bit 0
        sub     ah, ah                      ; clear upper ax
        ret

is_bn_neg   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; n != 0 ?
; RETURNS: if n != 0 returns 1
;          else returns 0
is_bn_not_zero   PROC n:bn_t

        mov     ax, ds                  ; save DS
        mov     cx, bnlength
        mov     bx, word ptr n

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load n pointer in ds:bx
        shr     cx, 1                   ; byte = 1/2 word
top_loop_16:
        cmp     word ptr ds:[bx], 0     ; compare to n to 0
        jnz     bottom                  ; not zero
        add     bx, 2                   ; increment to next word
        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load n pointer in ds:bx
        shr     cx, 2                   ; byte = 1/4 dword
top_loop_32:
        cmp     dword ptr ds:[bx], 0    ; compare to n to 0
        jnz     bottom                  ; not zero
        add     bx, 4                   ; increment to next dword
        loop    top_loop_32
        jmp     bottom
ENDIF

bottom:
.8086
        mov     ds, ax                  ; restore DS
        ; if cx is zero, then n was zero
        mov     ax, cx
        ret

is_bn_not_zero   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 + n2
add_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     si, WORD PTR n1
        mov     bx, WORD PTR n2


IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, ds:[si]             ; n1
        adc     ax, ds:[bx]             ; n1+n2
        mov     ds:[di], ax             ; r = n1+n2

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     si                      ; add  si, 2
        inc     si
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16

ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, ds:[si]            ; n1
        adc     eax, ds:[bx]            ; n1+n2
        mov     ds:[di], eax            ; r = n1+n2

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     si, 4
        add     bx, 4
        sahf                            ; restore carry flag

        loop    top_loop_32
ENDIF

bottom:
.8086
        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
add_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r += n
add_a_bn   PROC USES di, r:bn_t, n:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     bx, WORD PTR n

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, ds:[bx]             ; n
        adc     ds:[di], ax             ; r += n

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add  di, 2
        inc     bx

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, ds:[bx]            ; n
        adc     ds:[di], eax            ; r += n

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
        sahf                            ; restore carry flag

        loop    top_loop_32
ENDIF

bottom:
.8086
        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
add_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 - n2
sub_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     si, WORD PTR n1
        mov     bx, WORD PTR n2


IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, ds:[si]             ; n1
        sbb     ax, ds:[bx]             ; n1-n2
        mov     ds:[di], ax             ; r = n1-n2

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     si                      ; add  si, 2
        inc     si
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, ds:[si]            ; n1
        sbb     eax, ds:[bx]            ; n1-n2
        mov     ds:[di], eax            ; r = n1-n2

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     si, 4
        add     bx, 4
        sahf                            ; restore carry flag

        loop    top_loop_32
ENDIF

bottom:
.8086

        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
sub_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r -= n
sub_a_bn   PROC USES di, r:bn_t, n:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     bx, WORD PTR n

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc                             ; clear carry flag

top_loop_16:
        mov     ax, ds:[bx]             ; n
        sbb     ds:[di], ax             ; r -= n

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add  di, 2
        inc     bx

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 double word
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, ds:[bx]            ; n
        sbb     ds:[di], eax            ; r -= n

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
        sahf                            ; restore carry flag

        loop    top_loop_32

ENDIF

bottom:
.8086

        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
sub_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = -n
neg_bn   PROC USES di, r:bn_t, n:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     bx, WORD PTR n

IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word

top_loop_16:
        mov     ax, ds:[bx]
        neg     ax
        mov     ds:[di], ax
        jc      short no_more_carry_16  ; notice the "reverse" logic here

        add     di, 2                   ; increment by word size
        add     bx, 2

        loop    top_loop_16
        jmp     short bottom

no_more_carry_16:
        add     di, 2
        add     bx, 2
        loop    top_loop_no_more_carry_16   ; jump down
        jmp     short bottom

top_loop_no_more_carry_16:
        mov     ax, ds:[bx]
        not     ax
        mov     ds:[di], ax

        add     di, 2
        add     bx, 2

        loop    top_loop_no_more_carry_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 dword

top_loop_32:
        mov     eax, ds:[bx]
        neg     eax
        mov     ds:[di], eax
        jc      short no_more_carry_32   ; notice the "reverse" logic here

        add     di, 4                   ; increment by double word size
        add     bx, 4

        loop    top_loop_32
        jmp     short bottom

no_more_carry_32:
        add     di, 4                   ; increment by double word size
        add     bx, 4
        loop    top_loop_no_more_carry_32   ; jump down
        jmp     short bottom

top_loop_no_more_carry_32:
        mov     eax, ds:[bx]
        not     eax
        mov     ds:[di], eax

        add     di, 4                   ; increment by double word size
        add     bx, 4

        loop    top_loop_no_more_carry_32
ENDIF

bottom:
.8086

        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
neg_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= -1
neg_a_bn   PROC r:bn_t

        mov     ax, ds                  ; save ds
        mov     cx, bnlength
        mov     bx, WORD PTR r

IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds
        shr     cx, 1                   ; byte = 1/2 word

top_loop_16:
        neg     word ptr ds:[bx]
        jc      short no_more_carry_16  ; notice the "reverse" logic here

        add     bx, 2

        loop    top_loop_16
        jmp     short bottom

no_more_carry_16:
        add     bx, 2
        loop    top_loop_no_more_carry_16   ; jump down
        jmp     short bottom

top_loop_no_more_carry_16:
        not     word ptr ds:[bx]

        add     bx, 2

        loop    top_loop_no_more_carry_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds
        shr     cx, 2                   ; byte = 1/4 dword

top_loop_32:
        neg     dword ptr ds:[bx]
        jc      short no_more_carry_32   ; notice the "reverse" logic here

        add     bx, 4

        loop    top_loop_32
        jmp     short bottom

no_more_carry_32:
        add     bx, 4
        loop    top_loop_no_more_carry_32   ; jump down
        jmp     short bottom

top_loop_no_more_carry_32:
        not     dword ptr ds:[bx]

        add     bx, 4

        loop    top_loop_no_more_carry_32
ENDIF

bottom:
.8086
        mov     ds, ax                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
neg_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = 2*n
double_bn   PROC USES di, r:bn_t, n:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     bx, WORD PTR n

IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc

top_loop_16:
        mov     ax, ds:[bx]
        rcl     ax, 1                   ; rotate with carry left
        mov     ds:[di], ax

                                        ; inc does not change carry flag
        inc     di                      ; add  di, 2
        inc     di
        inc     bx                      ; add bx, 2
        inc     bx

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 dword
        clc                             ; clear carry flag

top_loop_32:
        mov     eax, ds:[bx]
        rcl     eax, 1                  ; rotate with carry left
        mov     ds:[di], eax

        lahf                            ; save carry flag
        add     di, 4                   ; increment by double word size
        add     bx, 4
        sahf                            ; restore carry flag

        loop    top_loop_32

ENDIF
bottom:
.8086

        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
double_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= 2
double_a_bn   PROC r:bn_t

        mov     ax, ds                  ; save ds
        mov     cx, bnlength
        mov     bx, WORD PTR r

IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word
        clc

top_loop_16:
        rcl     word ptr ds:[bx], 1     ; rotate with carry left

                                        ; inc does not change carry flag
        inc     bx                      ; add  bx, 2
        inc     bx

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 dword
        clc                             ; clear carry flag

top_loop_32:
        rcl     dword ptr ds:[bx], 1    ; rotate with carry left

        inc     bx                      ; add bx, 4 but keep carry flag
        inc     bx
        inc     bx
        inc     bx

        loop    top_loop_32
ENDIF

bottom:
.8086

        mov     ds, ax                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
double_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n/2
half_bn   PROC USES di, r:bn_t, n:bn_t

        mov     dx, ds                  ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     bx, WORD PTR n

        add     di, cx                  ; start with msb
        add     bx, cx

IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word

        ; handle the first step with sar, the rest with rcr
        sub     di, 2
        sub     bx, 2

        mov     ax, ds:[bx]
        sar     ax, 1                   ; shift arithmetic right
        mov     ds:[di], ax

        loop    top_loop_16
        jmp     short bottom


top_loop_16:
                                        ; inc does not change carry flag
        dec     di                      ; sub  di, 2
        dec     di
        dec     bx                      ; sub bx, 2
        dec     bx

        mov     ax, ds:[bx]
        rcr     ax, 1                   ; rotate with carry right
        mov     ds:[di], ax

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 dword

        sub     di, 4                   ; decrement by double word size
        sub     bx, 4

        mov     eax, ds:[bx]
        sar     eax, 1                  ; shift arithmetic right
        mov     ds:[di], eax

        loop    top_loop_32
        jmp     short bottom

top_loop_32:
        lahf                            ; save carry flag
        sub     di, 4                   ; decrement by double word size
        sub     bx, 4
        sahf                            ; restore carry flag

        mov     eax, ds:[bx]
        rcr     eax, 1                  ; rotate with carry right
        mov     ds:[di], eax

        loop    top_loop_32
ENDIF

bottom:
.8086

        mov     ds, dx                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
half_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r /= 2
half_a_bn   PROC r:bn_t

        mov     ax, ds                  ; save ds
        mov     cx, bnlength
        mov     bx, WORD PTR r

        add     bx, cx                  ; start with msb


IFDEF BIG16AND32
        cmp     cpu, 386
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        shr     cx, 1                   ; byte = 1/2 word

        ; handle the first step with sar, the rest with rcr
        sub     bx, 2

        sar     word ptr ds:[bx], 1     ; shift arithmetic right

        loop    top_loop_16
        jmp     short bottom


top_loop_16:
                                        ; inc does not change carry flag
        dec     bx                      ; sub bx, 2
        dec     bx

        rcr     word ptr ds:[bx], 1     ; rotate with carry right

        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     short bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        shr     cx, 2                   ; byte = 1/4 dword
        sub     bx, 4                   ; decrement by double word size
        sar     dword ptr ds:[bx], 1    ; shift arithmetic right

        loop    top_loop_32
        jmp     short bottom

top_loop_32:
        dec     bx                      ; sub bx, 4 but keep carry flag
        dec     bx
        dec     bx
        dec     bx

        rcr     dword ptr ds:[bx], 1       ; rotate with carry right

        loop    top_loop_32
ENDIF

bottom:
.8086

        mov     ds, ax                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
half_a_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 * n2
; Note: r will be a double wide result, 2*bnlength
;       n1 and n2 can be the same pointer
; SIDE-EFFECTS: n1 and n2 are changed to their absolute values
;
unsafe_full_mult_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t
LOCAL sign1:byte, sign2:byte, samevar:byte, \
      i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      n1p: near ptr byte, n2p: near ptr byte

        push    ds                          ; save ds
        mov     es, bignum_seg              ; load es for when ds is a pain

; Test to see if n1 and n2 are the same variable.  It would be better to
; use square_bn(), but it could happen.

        mov     samevar, 0                  ; assume they are not the same
        mov     bx, word ptr n1
        cmp     bx, word ptr n2             ; compare offset
        jne     end_samevar_check           ; not the same
        mov     samevar, 1                  ; they are the same
end_samevar_check:

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

                                            ; check for sign bits
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign1, al
        jz      already_pos1
        invoke  neg_a_bn, n1
already_pos1:

        cmp     samevar, 1                  ; if it's the same variable
        je      already_pos2                ; then skip this second check
        mov     bx, word ptr n2
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign2, al
        jz      already_pos2
        invoke  neg_a_bn, n2
already_pos2:

; in the following loops, the following pointers are used
;   n1p, n2p = points to the part of n1, n2 being used
;   di = points to part of doublebignumber r used in outer loop
;   si = points to part of doublebignumber r used in inner loop
;   bx = points to part of doublebignumber r for carry flag loop
; Also, since r is used more than n1p or n2p, abandon the convention of
; using ES for r.  Using DS will save a few clock cycles.

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
;        jae     use_32_bit              ; use faster 32 bit code if possible
        jb      wont_use_32bit
        jmp     use_32_bit              ; use faster 32 bit code if possible
wont_use_32bit:
ENDIF

IFDEF BIG16
        ; set variables
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; byte = 1/2 word
        mov     steps, dx               ; save in steps
        mov     i, dx
        shl     dx, 1                   ; double steps

        ; clear r
        sub     ax, ax                  ; clear ax
        mov     cx, dx                  ; size of doublebignumber (r) in words
        mov     di, word ptr r          ; load r in es:di for stos
        rep     stosw                   ; initialize r to 0

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx

        ; prepare segments and offsets for loops
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here
        mov     ds, bignum_seg          ; load ds
        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax
        ; use ds for all pointers

top_outer_loop_16:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     bx, n1p
        mov     ax, ds:[bx]
        mov     bx, n2p
        mul     word ptr ds:[bx]

        mov     bx, si
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     n2p, 2                  ; increase by word size
        add     si, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     n1p, 2                  ; increase by word size
        add     di, 2
        mov     si, di                  ; start with si=di

        dec     doublesteps             ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax


        dec     i
        ja      top_outer_loop_16

        ; result is now r, a double wide bignumber
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        ; set variables
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; byte = 1/4 dword
        mov     steps, dx               ; save in steps
        mov     i, dx
        shl     dx, 1                   ; double steps

        ; clear r
        sub     eax, eax                ; clear eax
        mov     cx, dx                  ; size of doublebignumber in dwords
        mov     di, word ptr r          ; load r in es:di for stos
        rep     stosd                   ; initialize r to 0

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx

        ; prepare segments and offsets for loops
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here
        mov     ds, bignum_seg          ; load ds
        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax

top_outer_loop_32:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     bx, n1p
        mov     eax, ds:[bx]
        mov     bx, n2p
        mul     dword ptr ds:[bx]

        mov     bx, si
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     n2p, 4                  ; increase by dword size
        add     si, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     n1p, 4                  ; increase by dword size
        add     di, 4
        mov     si, di                  ; start with si=di

        dec     doublesteps             ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax


        dec     i
        ja      top_outer_loop_32

        ; result is now r, a double wide bignumber
ENDIF

bottom:
.8086

        pop     ds                      ; restore ds
        cmp     samevar, 1              ; were the variable the same ones?
        je      pos_answer              ; if yes, then jump

        mov     al, sign1               ; is result + or - ?
        cmp     al, sign2               ; sign(n1) == sign(n2) ?
        je      pos_answer              ; yes
        shl     bnlength, 1             ; temporarily double bnlength
                                        ; for double wide bignumber
        invoke  neg_a_bn, r             ; does not affect ES
        shr     bnlength, 1             ; restore bnlength
pos_answer:

        mov     ax, word ptr r          ; return r in ax
        ret
unsafe_full_mult_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n1 * n2 calculating only the top rlength bytes
; Note: r will be of length rlength
;       2*bnlength <= rlength < bnlength
;       n1 and n2 can be the same pointer
; SIDE-EFFECTS: n1 and n2 are changed to their absolute values
;
unsafe_mult_bn   PROC USES di si, r:bn_t, n1:bn_t, n2:bn_t
LOCAL sign1:byte, sign2:byte, samevar:byte, \
      i:word, j:word, steps:word, doublesteps:word, \
      carry_steps:word, skips:word, \
      n1p: ptr byte, n2p: ptr byte

        push    ds                          ; save ds
        mov     es, bignum_seg              ; load es for when ds is a pain

; Test to see if n1 and n2 are the same variable.  It would be better to
; use square_bn(), but it could happen.

        mov     samevar, 0                  ; assume they are not the same
        mov     bx, word ptr n1
        cmp     bx, word ptr n2             ; compare offset
        jne     end_samevar_check           ; not the same
        mov     samevar, 1                  ; they are the same
end_samevar_check:

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

                                            ; check for sign bits
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign1, al
        jz      already_pos1
        invoke  neg_a_bn, n1
already_pos1:

        cmp     samevar, 1                  ; if it's the same variable
        je      already_pos2                ; then skip this second check
        mov     bx, word ptr n2
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign2, al
        jz      already_pos2
        invoke  neg_a_bn, n2
already_pos2:

        ; adjust n2 pointer for partial precision
        mov     ax, bnlength
        shl     ax, 1                   ; 2*bnlength
        sub     ax, rlength             ; 2*bnlength-rlength
        add     word ptr n2, ax         ; n2 = n2+2*bnlength-rlength


; in the following loops, the following pointers are used
;   n1p, n2p = points to the part of n1, n2 being used
;   di = points to part of doublebignumber used in outer loop
;   si = points to part of doublebignumber used in inner loop
;   bx = points to part of doublebignumber for carry flag loop
; Also, since r is used more than n1p or n2p, abandon the convention of
; using ES for r.  Using DS will save a few clock cycles.

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
;        jae     use_32_bit              ; use faster 32 bit code if possible
        jb      cant_use_32bit
        jmp     use_32_bit              ; use faster 32 bit code if possible
cant_use_32bit:
ENDIF

IFDEF BIG16
        ; clear r
        sub     ax, ax                  ; clear ax
        mov     cx, rlength             ; size of r in bytes
        shr     cx, 1                   ; byte = 1/2 word
        mov     di, word ptr r          ; load r in es:di for stos
        rep     stosw                   ; initialize r to 0

        ; set variables
        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     steps, ax               ; save in steps

        mov     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     i, ax

        sub     ax, steps
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, rlength             ; set steps for first loop
        shr     ax, 1                   ; byte = 1/2 word
        sub     ax, 2                   ; only rlength/2-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        ; prepare segments and offsets for loops
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here
        mov     ds, bignum_seg          ; load ds
        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax
        ; use ds for all pointers


top_outer_loop_16:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     bx, n1p
        mov     ax, ds:[bx]
        mov     bx, n2p
        mul     word ptr ds:[bx]

        mov     bx, si
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     n2p, 2                  ; increase by word size
        add     si, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     n1p, 2                  ; increase by word size

        cmp     skips, 0
        je      type2_shifts_16
        sub     word ptr n2, 2          ; shift n2 back a word
        inc     steps                   ; one more step this time
        ; leave di and doublesteps where they are
        dec     skips                   ; keep track of how many times we've done this
        jmp     shifts_bottom_16
type2_shifts_16:
        add     di, 2                   ; shift di forward a word
        dec     doublesteps             ; reduce the carry steps needed
shifts_bottom_16:
        mov     si, di                  ; start with si=di
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_16

        ; result is in r
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386

        ; clear r
        sub     eax, eax                ; clear eax
        mov     cx, rlength             ; size of r in bytes
        shr     cx, 2                   ; byte = 1/4 dword
        mov     di, word ptr r          ; load r in es:di for stos
        rep     stosd                   ; initialize r to 0

        ; set variables
        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     steps, ax               ; save in steps

        mov     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     i, ax

        sub     ax, steps
        mov     skips, ax               ; how long to skip over pointer shifts

        mov     ax, rlength             ; set steps for first loop
        shr     ax, 2                   ; byte = 1/4 dword
        sub     ax, 2                   ; only rlength/4-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        ; prepare segments and offsets for loops
        mov     di, word ptr r
        mov     si, di                  ; both si and di are used here
        mov     ds, bignum_seg          ; load ds
        mov     ax, word ptr n1         ; load pointers
        mov     n1p, ax


top_outer_loop_32:
        mov     ax, word ptr n2         ; set n2p pointer
        mov     n2p, ax
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     bx, n1p
        mov     eax, ds:[bx]
        mov     bx, n2p
        mul     dword ptr ds:[bx]

        mov     bx, si
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of r
top_carry_loop_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     n2p, 4                  ; increase by dword size
        add     si, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     n1p, 4                  ; increase by dword size

        cmp     skips, 0
        je      type2_shifts_32
        sub     word ptr n2, 4          ; shift n2 back a dword
        inc     steps                   ; one more step this time
        ; leave di and doublesteps where they are
        dec     skips                   ; keep track of how many times we've done this
        jmp     shifts_bottom_32
type2_shifts_32:
        add     di, 4                   ; shift di forward a dword
        dec     doublesteps             ; reduce the carry steps needed
shifts_bottom_32:
        mov     si, di                  ; start with si=di
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_32

        ; result is in r
ENDIF

bottom:
.8086
        pop     ds                      ; restore ds
        cmp     samevar, 1              ; were the variable the same ones?
        je      pos_answer              ; if yes, then jump

        mov     al, sign1               ; is result + or - ?
        cmp     al, sign2               ; sign(n1) == sign(n2) ?
        je      pos_answer              ; yes
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; set bnlength = rlength
        invoke  neg_a_bn, r             ; does not affect ES
        pop     bnlength                ; restore bnlength
pos_answer:

        mov     ax, word ptr r          ; return r in ax
        ret
unsafe_mult_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n^2
;   because of the symetry involved, n^2 is much faster than n*n
;   for a bignumber of length l
;      n*n takes l^2 multiplications
;      n^2 takes (l^2+l)/2 multiplications
;          which is about 1/2 n*n as l gets large
;  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
;
; Note: r will be a double wide result, 2*bnlength
; SIDE-EFFECTS: n is changed to its absolute value
;
unsafe_full_square_bn   PROC USES di si, r:bn_t, n:bn_t
LOCAL i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      save_ds:word, \
      rp1: ptr byte, rp2: ptr byte

        mov     save_ds, ds                 ; save ds
        mov     es, bignum_seg              ; load es for when ds is a pain

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

                                            ; check for sign bit
        mov     bx, word ptr n
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:

; in the following loops, the following pointers are used
;   n1p(di), n2p(si) = points to the parts of n being used (es)
;   rp1 = points to part of doublebignumber used in outer loop  (ds)
;   rp2 = points to part of doublebignumber used in inner loop  (ds)
;   bx  = points to part of doublebignumber for carry flag loop (ds)

        mov     cx, bnlength            ; size of doublebignumber in words

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
;        jae     use_32_bit              ; use faster 32 bit code if possible
        jb      dont_use_32bit
        jmp     use_32_bit              ; use faster 32 bit code if possible
dont_use_32bit:
ENDIF

IFDEF BIG16
        ; clear r
        sub     ax, ax                  ; clear ax
        ; 2{twice the size}*bnlength/2{bytes per word}
        mov     di, word ptr r          ; load r pointer in es:di for stos
        rep     stosw                   ; initialize r to 0

        ; initialize vars
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; byte = 1/2 word
        dec     dx                      ; don't need to do last one
        mov     i, dx                   ; loop counter
        mov     steps, dx               ; save in steps
        shl     dx, 1                   ; double steps
        sub     dx, 1                   ; only 2*s-1 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx

        ; initialize pointers
        mov     di, word ptr n
        mov     ax, word ptr r
        mov     ds, bignum_seg          ; load ds
        add     ax, 2                   ; start with second word
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1

        cmp     i, 0                    ; if bignumberlength is 2
        je      skip_middle_terms_16

top_outer_loop_16:
        mov     si, di                  ; set n2p pointer
        add     si, 2                   ; to 1 word beyond n1p(di)
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     ax, ds:[di]
        mul     word ptr ds:[si]

        mov     bx, rp2
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     si, 2                   ; increase by word size
        add     rp2, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     di, 2                   ; increase by word size
        add     rp1, 4                  ; increase by 2*word size
        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     steps                   ; use one less step
        dec     i
        ja      top_outer_loop_16

        ; All the middle terms have been multiplied.  Now double it.
        mov     ds, save_ds             ; restore ds to get bnlength
        shl     bnlength, 1             ; r is a double wide bignumber
        invoke  double_a_bn, r          ; doesn't change es
        shr     bnlength, 1             ; restore r

skip_middle_terms_16:                   ; ds is not necessarily restored here

; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used (es)
;   rp1(si) = points to part of doublebignumber used in outer loop  (ds)
;   bx      = points to part of doublebignumber for carry flag loop (ds)

        mov     di, word ptr n          ; load n1p pointer in di

        mov     ds, save_ds             ; restore ds to get bnlength
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 1                   ; 1 bytes = 1/2 word
        mov     i, dx                   ; loop counter
        shl     dx, 1                   ; double steps

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     si, word ptr r          ; set rp1
        mov     ds, bignum_seg          ; load ds


top_outer_loop_squares_16:

        mov     ax, ds:[di]
        mul     ax                      ; square it

        mov     bx, si
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_squares_16 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_squares_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_squares_16

no_more_carry_squares_16:
        add     di, 2                   ; increase by word size
        add     si, 4                   ; increase by 2*word size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_16


        ; result is in r, a double wide bignumber
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        ; clear r
        sub     eax, eax                ; clear eax
        ; 2{twice the size}*bnlength/4{bytes per word}
        shr     cx, 1                   ; size of doublebignumber in dwords
        mov     di, word ptr r          ; load r pointer in es:di for stos
        rep     stosd                   ; initialize r to 0

        ; initialize vars
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; byte = 1/4 dword
        dec     dx                      ; don't need to do last one
        mov     i, dx                   ; loop counter
        mov     steps, dx               ; save in steps
        shl     dx, 1                   ; double steps
        sub     dx, 1                   ; only 2*s-1 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx

        ; initialize pointers
        mov     di, word ptr n          ; load n1p pointer
        mov     ax, word ptr r
        mov     ds, bignum_seg          ; load ds

        add     ax, 4                   ; start with second dword
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1

        cmp     i, 0                    ; if bignumberlength is 4
        je      skip_middle_terms_32

top_outer_loop_32:
        mov     si, di                  ; set n2p pointer
        add     si, 4                   ; to 1 dword beyond n1p(di)
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     eax, ds:[di]
        mul     dword ptr ds:[si]

        mov     bx, rp2
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     si, 4                   ; increase by dword size
        add     rp2, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     di, 4                   ; increase by dword size
        add     rp1, 8                  ; increase by 2*dword size
        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     steps                   ; use one less step
        dec     i
        ja      top_outer_loop_32

        ; All the middle terms have been multiplied.  Now double it.
        mov     ds, save_ds             ; restore ds to get bnlength
        shl     bnlength, 1             ; r is a double wide bignumber
        invoke  double_a_bn, r
        shr     bnlength, 1             ; restore r

skip_middle_terms_32:                   ; ds is not necessarily restored here

; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used (es)
;   rp1(si) = points to part of doublebignumber used in outer loop (ds)
;   bx = points to part of doublebignumber for carry flag loop     (ds)

        mov     di, word ptr n          ; load n1p pointer in ds:di

        mov     ds, save_ds             ; restore ds to get bnlength
        mov     dx, bnlength            ; set outer loop counter
        shr     dx, 2                   ; 1 bytes = 1/4 dword
        mov     i, dx                   ; loop counter
        shl     dx, 1                   ; double steps

        sub     dx, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, dx
        mov     carry_steps, dx
        mov     si, word ptr r          ; set rp1
        mov     ds, bignum_seg          ; load ds

top_outer_loop_squares_32:

        mov     eax, ds:[di]
        mul     eax                     ; square it

        mov     bx, si
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_squares_32 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_squares_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_squares_32

no_more_carry_squares_32:
        add     di, 4                   ; increase by dword size
        add     si, 8                   ; increase by 2*dword size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_32


        ; result is in r, a double wide bignumber
ENDIF

bottom:
.8086

; since it is a square, the result has to already be positive

        mov     ds, save_ds             ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
unsafe_full_square_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n^2
;   because of the symetry involved, n^2 is much faster than n*n
;   for a bignumber of length l
;      n*n takes l^2 multiplications
;      n^2 takes (l^2+l)/2 multiplications
;          which is about 1/2 n*n as l gets large
;  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
;
; Note: r will be of length rlength
;       2*bnlength >= rlength > bnlength
; SIDE-EFFECTS: n is changed to its absolute value
;
unsafe_square_bn   PROC USES di si, r:bn_t, n:bn_t
LOCAL i:word, j:word, steps:word, doublesteps:word, carry_steps:word, \
      skips:word, rodd:word, \
      save_ds:word, \
      n3p: ptr byte, \
      rp1: ptr byte, rp2: ptr byte

; This whole procedure would be a great deal simpler if we could assume that
; rlength < 2*bnlength (that is, not =).  Therefore, we will take the
; easy way out and call full_square_bn() if it is.
        mov     ax, rlength
        shr     ax, 1                   ; 1/2 * rlength
        cmp     ax, bnlength            ; 1/2 * rlength == bnlength?
        jne     not_full_square
        invoke  unsafe_full_square_bn, r, n
        ; dx:ax is still loaded with return value
        jmp     quit_proc               ; we're outa here
not_full_square:

        mov     save_ds, ds
        mov     es, bignum_seg              ; load es for when ds is a pain

; By forcing the bignumber to be positive and keeping track of the sign
; bits separately, quite a few multiplies are saved.

                                            ; check for sign bit
        mov     bx, word ptr n              ; load n1 pointer in es:bx
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:

; in the following loops, the following pointers are used
;   n1p(di), n2p(si) = points to the parts of n being used (es)
;   rp1 = points to part of doublebignumber used in outer loop  (ds)
;   rp2 = points to part of doublebignumber used in inner loop  (ds)
;   bx  = points to part of doublebignumber for carry flag loop (ds)

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
;        jae     use_32_bit              ; use faster 32 bit code if possible
        jb      skip_use_32bit
        jmp     use_32_bit              ; use faster 32 bit code if possible
skip_use_32bit:
ENDIF

IFDEF BIG16
        ; clear r
        sub     ax, ax                  ; clear ax
        mov     cx, rlength             ; size of rlength in bytes
        shr     cx, 1                   ; byte = 1/2 word
        mov     di, word ptr r          ; load r pointer in es:di for stos
        rep     stosw                   ; initialize r to 0


        ; initialize vars

        ; determine whether r is on an odd or even word in the number
        ; (even if rlength==2*bnlength, dec r alternates odd/even)
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        shr     ax, 1                   ; 1 byte = 1/2 word
        and     ax, 0001h               ; check the odd sign bit
        mov     rodd, ax

        mov     ax, bnlength            ; set outer loop counter
        shr     ax, 1                   ; byte = 1/2 word
        dec     ax                      ; don't need to do last one
        mov     i, ax                   ; loop counter

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 1                   ; byte = 1/2 word
        mov     steps, ax               ; save in steps

        mov     dx, bnlength
        shr     dx, 1                   ; bnlength/2
        add     ax, dx                  ; steps+bnlength/2
        sub     ax, 2                   ; steps+bnlength/2-2
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ax, i
        sub     ax, steps
        shr     ax, 1                   ; for both words and dwords
        mov     skips, ax               ; how long to skip over pointer shifts

        ; initialize pointers
        mov     di, word ptr n
        mov     si, di
        mov     ax, bnlength
        shr     ax, 1                   ; 1 byte = 1/2 word
        sub     ax, steps
        shl     ax, 1                   ; 1 byte = 1/2 word
        add     si, ax                  ; n2p = n1p + 2*(bnlength/2 - steps)
        mov     n3p, si                 ; save for later use
        mov     ax, word ptr r
        mov     ds, bignum_seg          ; load ds
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1

        cmp     i, 0                    ; if bignumberlength is 2
;        je      skip_middle_terms_16
        jne     top_outer_loop_16
        jmp     skip_middle_terms_16

top_outer_loop_16:
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_16:
        mov     ax, ds:[di]
        mul     word ptr ds:[si]

        mov     bx, rp2
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_16        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_16

no_more_carry_16:
        add     si, 2                   ; increase by word size
        add     rp2, 2
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_16

        add     di, 2                   ; increase by word size

        mov     ax, rodd                ; whether r is on an odd or even word

        cmp     skips, 0
        jle     type2_shifts_16
        sub     n3p, 2                  ; point to previous word
        mov     si, n3p
        inc     steps                   ; one more step this time
        ; leave rp1 and doublesteps where they are
        dec     skips
        jmp     shifts_bottom_16
type2_shifts_16:    ; only gets executed once
        jl      type3_shifts_16
        sub     steps, ax               ; steps -= (0 or 1)
        inc     ax                      ; ax = 1 or 2 now
        sub     doublesteps, ax         ; decrease double steps by 1 or 2
        shl     ax, 1                   ; 1 byte = 1/2 word
        add     rp1, ax                 ; add 1 or 2 words
        mov     si, di
        add     si, 2                   ; si = di + word
        dec     skips                   ; make skips negative
        jmp     shifts_bottom_16
type3_shifts_16:
        dec     steps
        sub     doublesteps, 2
        add     rp1, 4                  ; + two words
        mov     si, di
        add     si, 2                   ; si = di + word
shifts_bottom_16:

        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
;        ja      top_outer_loop_16
        jna     not_top_outer_loop_16
        jmp     top_outer_loop_16
not_top_outer_loop_16:
        ; All the middle terms have been multiplied.  Now double it.
        mov     ds, save_ds             ; restore ds to get bnlength
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; r is of length rlength
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_16:
; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used (es)
;   rp1(si) = points to part of doublebignumber used in outer loop (ds)
;   bx = points to part of doublebignumber for carry flag loop     (ds)

        ; be careful, the next dozen or so lines are confusing!

        ; determine whether r is on an odd or even word in the number
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        mov     dx, ax                  ; save this for a moment
        and     ax, 0002h               ; check the odd sign bit

        mov     si, word ptr r          ; load r pointer in ds:si
        add     si, ax                  ; depending on odd or even byte

        shr     dx, 1                   ; assumes word size
        inc     dx
        and     dx, 0FFFEh              ; ~2+1, turn off last bit, mult of 2
        mov     di, word ptr n          ; load n1p pointer in di
                                        ; es is still set from before
        add     di, dx

        mov     ax, bnlength
        sub     ax, dx
        shr     ax, 1                   ; 1 byte = 1/2 word
        mov     i, ax

        shl     ax, 1                   ; double steps
        sub     ax, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ds, bignum_seg          ; load ds

top_outer_loop_squares_16:

        mov     ax, ds:[di]
        mul     ax                      ; square it

        mov     bx, si
        add     bx, 2                   ; increase by size of word
        add     ds:[bx-2], ax           ; add low word
        adc     ds:[bx], dx             ; add high word
        jnc     no_more_carry_squares_16 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_16
        add     bx, 2                   ; move pointer to next word

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_16:
        add     word ptr ds:[bx], 1     ; use add, not inc
        jnc     no_more_carry_squares_16
        add     bx, 2                   ; increase by size of word
        loop    top_carry_loop_squares_16

no_more_carry_squares_16:
        add     di, 2                   ; increase by word size
        add     si, 4                   ; increase by 2*word size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_16


        ; result is in r
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        ; clear r
        sub     eax, eax                ; clear eax
        mov     cx, rlength             ; size of rlength in bytes
        shr     cx, 2                   ; byte = 1/4 dword
        mov     di, word ptr r          ; load r pointer in es:di for stos
        rep     stosd                   ; initialize r to 0

        ; initialize vars

        ; determine whether r is on an odd or even dword in the number
        ; (even if rlength==2*bnlength, dec r alternates odd/even)
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        shr     ax, 2                   ; 1 byte = 1/4 dword
        and     ax, 0001h               ; check the odd sign bit
        mov     rodd, ax

        mov     ax, bnlength            ; set outer loop counter
        shr     ax, 2                   ; byte = 1/4 dword
        dec     ax                      ; don't need to do last one
        mov     i, ax                   ; loop counter

        mov     ax, rlength             ; set steps for first loop
        sub     ax, bnlength
        shr     ax, 2                   ; byte = 1/4 dword
        mov     steps, ax               ; save in steps

        mov     dx, bnlength
        shr     dx, 2                   ; bnlength/4
        add     ax, dx                  ; steps+bnlength/4
        sub     ax, 2                   ; steps+bnlength/4-2
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ax, i
        sub     ax, steps
        shr     ax, 1                   ; for both words and dwords
        mov     skips, ax               ; how long to skip over pointer shifts

        ; initialize pointers
        mov     di, word ptr n          ; load n1p pointer
        mov     si, di
        mov     ax, bnlength
        shr     ax, 2                   ; 1 byte = 1/4 dword
        sub     ax, steps
        shl     ax, 2                   ; 1 byte = 1/4 dword
        add     si, ax                  ; n2p = n1p + bnlength/4 - steps
        mov     n3p, si                 ; save for later use
        mov     ax, word ptr r
        mov     ds, bignum_seg          ; load ds
        mov     rp1, ax
        mov     rp2, ax                 ; start with rp2=rp1

        cmp     i, 0                    ; if bignumberlength is 2
        je      skip_middle_terms_32

top_outer_loop_32:
        mov     ax, steps               ; set inner loop counter
        mov     j, ax

top_inner_loop_32:
        mov     eax, ds:[di]
        mul     dword ptr ds:[si]

        mov     bx, rp2
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_32        ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_32

no_more_carry_32:
        add     si, 4                   ; increase by dword size
        add     rp2, 4
        dec     carry_steps             ; use one less step
        dec     j
        ja      top_inner_loop_32

        add     di, 4                   ; increase by dword size

        mov     ax, rodd                ; whether r is on an odd or even dword

        cmp     skips, 0
        jle     type2_shifts_32
        sub     n3p, 4                  ; point to previous dword
        mov     si, n3p
        inc     steps                   ; one more step this time
        ; leave rp1 and doublesteps where they are
        dec     skips
        jmp     shifts_bottom_32
type2_shifts_32:    ; only gets executed once
        jl      type3_shifts_32
        sub     steps, ax               ; steps -= (0 or 1)
        inc     ax                      ; ax = 1 or 2 now
        sub     doublesteps, ax         ; decrease double steps by 1 or 2
        shl     ax, 2                   ; 1 byte = 1/4 dword
        add     rp1, ax                 ; add 1 or 2 dwords
        mov     si, di
        add     si, 4                   ; si = di + dword
        dec     skips                   ; make skips negative
        jmp     shifts_bottom_32
type3_shifts_32:
        dec     steps
        sub     doublesteps, 2
        add     rp1, 8                  ; + two dwords
        mov     si, di
        add     si, 4                   ; si = di + dword
shifts_bottom_32:

        mov     ax, rp1
        mov     rp2, ax                 ; start with rp2=rp1

        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_32

        ; All the middle terms have been multiplied.  Now double it.
        mov     ds, save_ds             ; restore ds to get bnlength
        push    bnlength                ; save bnlength
        mov     ax, rlength
        mov     bnlength, ax            ; r is of length rlength
        invoke  double_a_bn, r
        pop     bnlength

skip_middle_terms_32:
; Now go back and add in the squared terms.
; In the following loops, the following pointers are used
;   n1p(di) = points to the parts of n being used (es)
;   rp1(si) = points to part of doublebignumber used in outer loop (ds)
;   bx = points to part of doublebignumber for carry flag loop     (ds)

        ; be careful, the next dozen or so lines are confusing!

        ; determine whether r is on an odd or even word in the number
        mov     ax, bnlength
        shl     ax, 1                   ; double wide width
        sub     ax, rlength             ; 2*bnlength-rlength
        mov     dx, ax                  ; save this for a moment
        and     ax, 0004h               ; check the odd sign bit

        mov     si, word ptr r          ; load r pointer in ds:si
        add     si, ax                  ; depending on odd or even byte

        shr     dx, 2                   ; assumes dword size
        inc     dx
        and     dx, 0FFFEh              ; ~2+1, turn off last bit, mult of 2
        shl     dx, 1
        mov     di, word ptr n          ; load n1p pointer in di
                                        ; es is still set from before
        add     di, dx

        mov     ax, bnlength
        sub     ax, dx
        shr     ax, 2                   ; 1 byte = 1/4 dword
        mov     i, ax

        shl     ax, 1                   ; double steps
        sub     ax, 2                   ; only 2*s-2 steps are really needed
        mov     doublesteps, ax
        mov     carry_steps, ax

        mov     ds, bignum_seg          ; load ds

top_outer_loop_squares_32:

        mov     eax, ds:[di]
        mul     eax                     ; square it

        mov     bx, si
        add     bx, 4                   ; increase by size of dword
        add     ds:[bx-4], eax          ; add low dword
        adc     ds:[bx], edx            ; add high dword
        jnc     no_more_carry_squares_32 ; carry loop not necessary

        mov     cx, carry_steps         ; how many till end of double big number
        jcxz    no_more_carry_squares_32
        add     bx, 4                   ; move pointer to next dword

        ; loop until no more carry or until end of double big number
top_carry_loop_squares_32:
        add     dword ptr ds:[bx], 1    ; use add, not inc
        jnc     no_more_carry_squares_32
        add     bx, 4                   ; increase by size of dword
        loop    top_carry_loop_squares_32

no_more_carry_squares_32:
        add     di, 4                   ; increase by dword size
        add     si, 8                   ; increase by 2*dword size

        sub     doublesteps,2           ; reduce the carry steps needed
        mov     ax, doublesteps
        mov     carry_steps, ax

        dec     i
        ja      top_outer_loop_squares_32


        ; result is in r
ENDIF

bottom:
.8086

; since it is a square, the result has to already be positive

        mov     ds, save_ds             ; restore ds
        mov     ax, word ptr r          ; return r in ax

quit_proc:
        ret
unsafe_square_bn   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n * u  where u is an unsigned integer
mult_bn_int   PROC USES di si, r:bn_t, n:bn_t, u:word
LOCAL   lu:dword  ; long unsigned integer in 32 bit math

        push    ds                      ; save ds
        mov     cx, bnlength
        mov     di, WORD PTR r
        mov     si, WORD PTR n


IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     use_32_bit              ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r

        shr     cx, 1                   ; byte = 1/2 word
        sub     bx, bx                  ; use bx for temp holding carried word

top_loop_16:
        mov     ax, ds:[si]             ; load next word from n
        mul     u                       ; n * u
        add     ax, bx                  ; add last carried upper word
        adc     dx, 0                   ; inc the carried word if carry flag set
        mov     bx, dx                  ; save high word in bx
        mov     ds:[di], ax             ; save low word

        add     di, 2                   ; next word in r
        add     si, 2                   ; next word in n
        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r

        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; use ebx for temp holding carried dword

        sub     eax, eax                ; clear upper eax
        mov     ax, u                   ; convert u (unsigned int)
        mov     lu, eax                 ;   to lu (long unsigned int)

top_loop_32:
        mov     eax, ds:[si]            ; load next dword from n
        mul     lu                      ; n * lu
        add     eax, ebx                ; add last carried upper dword
        adc     edx, 0                  ; inc the carried dword if carry flag set
        mov     ebx, edx                ; save high dword in ebx
        mov     ds:[di], eax            ; save low dword

        add     di, 4                   ; next dword in r
        add     si, 4                   ; next dword in n
        loop    top_loop_32
ENDIF

bottom:
.8086

        pop     ds
        mov     ax, word ptr r          ; return r in ax
        ret
mult_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r *= u  where u is an unsigned integer
mult_a_bn_int   PROC USES di si, r:bn_t, u:word

        push    ds                      ; save ds
        mov     cx, bnlength            ; set outer loop counter
        mov     si, WORD PTR r


IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     use_32_bit              ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds
        ; no need to clear r
        shr     cx, 1                   ; byte = 1/2 word
        sub     bx, bx                  ; use bx for temp holding carried word
        mov     di, u                   ; save u in di

top_loop_16:
        mov     ax, ds:[si]             ; load next word from r
        mul     di                      ; r * u
        add     ax, bx                  ; add last carried upper word
        adc     dx, 0                   ; inc the carried word if carry flag set
        mov     bx, dx                  ; save high word in bx
        mov     ds:[si], ax             ; save low word

        add     si, 2                   ; next word in r
        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds
        ; no need to clear r
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; use ebx for temp holding carried dword
        sub     edi, edi                ; clear upper edi
        mov     di, u                   ; save u in lower di

top_loop_32:
        mov     eax, ds:[si]            ; load next dword from r
        mul     edi                     ; r * u
        add     eax, ebx                ; add last carried upper dword
        adc     edx, 0                  ; inc the carried dword if carry flag set
        mov     ebx, edx                ; save high dword in ebx
        mov     ds:[si], eax            ; save low dword

        add     si, 4                   ; next dword in r
        loop    top_loop_32
ENDIF

bottom:
.8086

        pop     ds                      ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret
mult_a_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n / u  where u is an unsigned integer
unsafe_div_bn_int   PROC USES di si, r:bn_t, n:bn_t, u:word
LOCAL  sign:byte

        push    ds
                                            ; check for sign bits
        mov     bx, WORD PTR n
        mov     es, bignum_seg              ; load n pointer es:bx
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign, al
        jz      already_pos
        invoke  neg_a_bn, n
already_pos:

        mov     cx, bnlength                ; set outer loop counter
        mov     di, word ptr r
        mov     si, word ptr n              ; load pointers ds:si
        ; past most significant portion of the number
        add     si, cx
        add     di, cx

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     use_32_bit              ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 1                   ; byte = 1/2 word
        mov     bx, u

        ; need to start with most significant portion of the number
        sub     si, 2                   ; most sig word
        sub     di, 2                   ; most sig word

        sub     dx, dx                  ; clear dx register
                                        ; for first time through loop
top_loop_16:
        mov     ax, ds:[si]             ; load next word from n
        div     bx
        mov     ds:[di], ax             ; store low word
                                        ; leave remainder in dx

        sub     si, 2                   ; next word in n
        sub     di, 2                   ; next word in r
        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; clear upper word or ebx
        mov     bx, u

        ; need to start with most significant portion of the number
        sub     si, 4                   ; most sig dword
        sub     di, 4                   ; most sig dword

        sub     edx, edx                ; clear edx register
                                        ; for first time through loop
top_loop_32:
        mov     eax, ds:[si]            ; load next dword from n
        div     ebx
        mov     ds:[di], eax            ; store low dword
                                        ; leave remainder in edx

        sub     si, 4                   ; next dword in n
        sub     di, 4                   ; next dword in r
        loop    top_loop_32
ENDIF

bottom:
.8086

        pop     ds                      ; restore ds

        cmp     sign, 0                 ; is result + or - ?
        je      pos_answer              ; yes
        invoke  neg_a_bn, r             ; does not affect ES
pos_answer:

        mov     ax, word ptr r          ; return r in ax
        ret
unsafe_div_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r /= u  where u is an unsigned integer
div_a_bn_int   PROC USES si, r:bn_t, u:word
LOCAL  sign:byte

        push    ds

        mov     bx, WORD PTR r
        mov     es, bignum_seg              ; load r pointer es:bx
        add     bx, bnlength
        mov     al, es:[bx-1]
        and     al, 80h                     ; check the sign bit
        mov     sign, al
        jz      already_pos
        invoke  neg_a_bn, r
already_pos:

        mov     cx, bnlength            ; set outer loop counter
        mov     si, WORD PTR r
        ; past most significant portion of the number
        add     si, cx


IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     use_32_bit              ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 1                   ; byte = 1/2 word
        mov     bx, u

        ; need to start with most significant portion of the number
        sub     si, 2                   ; most sig word

        sub     dx, dx                  ; clear dx register
                                        ; for first time through loop
top_loop_16:
        mov     ax, ds:[si]             ; load next word from r
        div     bx
        mov     ds:[si], ax             ; store low word
                                        ; leave remainder in dx

        sub     si, 2                   ; next word in r
        loop    top_loop_16
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load ds

        ; no need to clear r here, values get mov'ed, not add'ed
        shr     cx, 2                   ; byte = 1/4 dword
        sub     ebx, ebx                ; clear upper word or ebx
        mov     bx, u

        ; need to start with most significant portion of the number
        sub     si, 4                   ; most sig dword

        sub     edx, edx                ; clear edx register
                                        ; for first time through loop
top_loop_32:
        mov     eax, ds:[si]            ; load next dword from r
        div     ebx
        mov     ds:[si], eax            ; store low dword
                                        ; leave remainder in edx

        sub     si, 4                   ; next dword in r
        loop    top_loop_32
ENDIF

bottom:
.8086
        pop     ds                      ; restore ds

        cmp     sign, 0                 ; is result + or - ?
        je      pos_answer              ; yes
        invoke  neg_a_bn, r             ; does not affect ES
pos_answer:

        mov     ax, word ptr r          ; return r in ax
        ret
div_a_bn_int   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; bf_t routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = 0    (just like clear_bn() but loads bflength+2 instead of bnlength)
clear_bf   PROC USES di, r:bf_t

        mov     cx, bflength
        mov     di, word ptr r
        mov     es, bignum_seg          ; load pointer in es:di

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        sub     ax, ax                  ; clear ax
        shr     cx, 1                   ; 1 byte = 1/2 word
        inc     cx                      ; plus the exponent
        rep     stosw                   ; clear r, word at a time
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        sub     eax, eax                ; clear eax
        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     stosd                   ; clear r, dword at a time
        stosw                           ; plus the exponent
ENDIF

bottom:
.8086
        mov     ax, word ptr r          ; return r in ax
        ret

clear_bf   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; r = n
copy_bf   PROC USES di si, r:bf_t, n:bf_t

        mov     ax, ds                  ; save ds for later
        mov     cx, bflength
        add     cx, 2
        mov     di, word ptr r
        mov     es, bignum_seg          ; load pointer in es:di
        mov     si, word ptr n

IFDEF BIG16AND32
        cmp     cpu, 386                ; check cpu
        jae     short use_32_bit        ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
        mov     ds, bignum_seg          ; load pointer in ds:si for movs

        shr     cx, 1                   ; 1 byte = 1/2 word
        inc     cx                      ; plus the exponent
        rep     movsw                   ; copy word at a time
ENDIF

IFDEF BIG16AND32
        jmp     bottom
ENDIF

IFDEF BIG32
use_32_bit:
.386
        mov     ds, bignum_seg          ; load pointer in ds:si for movs

        shr     cx, 2                   ; 1 byte = 1/4 word
        rep     movsd                   ; copy dword at a time
        movsw                           ; plus the exponent
ENDIF

bottom:
.8086
        mov     ds, ax                  ; restore ds
        mov     ax, word ptr r          ; return r in ax
        ret

copy_bf   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL bftofloat(bf_t n);
; converts a bf number to a 10 byte real
;
bftofloat   PROC USES di si, n:bf_t
   LOCAL value[11]:BYTE   ; 11=10+1

      mov      ax, ds                  ; save ds

      mov      cx, 9                   ; need up to 9 bytes
      cmp      bflength, 10            ; but no more than bflength-1
      jae      movebytes_set
      mov      cx, bflength            ; bflength is less than 10
      dec      cx                      ; cx=movebytes=bflength-1, 1 byte padding
movebytes_set:

IFDEF BIG16AND32
      cmp     cpu, 386              ; check cpu
;      jae     use_32_bit            ; use faster 32 bit code if possible
      jb      over_use_32bit
      jmp     use_32_bit            ; use faster 32 bit code if possible
over_use_32bit:
ENDIF

IFDEF BIG16
; 16 bit code
      ; clear value
      mov      word ptr value[0], 0
      mov      word ptr value[2], 0
      mov      word ptr value[4], 0
      mov      word ptr value[6], 0
      mov      word ptr value[8], 0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      dx, ss               ; move ss to es for movs
      mov      es, dx               ; ie: move ss:value+9-cx to es:di
      mov      bx, bflength
      dec      bx
      sub      bx, cx               ; cx holds movebytes
      mov      si, word ptr n
      mov      ds, bignum_seg       ; move n to ds:si for movs
      add      si, bx               ; n+bflength-1-movebytes
      rep movsb
      mov      bl, ds:[si]          ; save sign byte, si now points to it
      inc      si                   ; point to exponent
      mov      dx, ds:[si]          ; use dx as exponent
      mov      cl, 3                ; put exponent (dx) in base 2
      shl      dx, cl               ; 256^n = 2^(8n)

      ; adjust for negative values
      and      bl, 10000000b           ; isolate sign bit
      jz       not_neg_16
      neg      word ptr value[0]       ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      word ptr value[2]
      adc      word ptr value[2], 0
      not      word ptr value[4]
      adc      word ptr value[4], 0
      not      word ptr value[6]
      adc      word ptr value[6], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
not_neg_16:

      cmp      byte ptr value[8], 0          ; test for 0
      jnz      top_shift_16
      fldz
      jmp      return

      ; Shift until most signifcant bit is set.
top_shift_16:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom_shift_16
      dec      dx                      ; decrement exponent
      shl      word ptr value[0], 1    ; shift left the 9 byte number
      rcl      word ptr value[2], 1
      rcl      word ptr value[4], 1
      rcl      word ptr value[6], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_16
bottom_shift_16:

      ; round last byte
      cmp      byte ptr value[0], 80h  ;
;      jb       bottom                  ; no rounding necessary
      jnb      not_bottom1
      jmp      bottom                  ; no rounding necessary
not_bottom1:
      add      word ptr value[1], 1
      adc      word ptr value[3], 0
      adc      word ptr value[5], 0
      adc      word ptr value[7], 0
;      jnc      bottom
      jc       not_bottom2
      jmp      bottom
not_bottom2:
      ; to get to here, the pattern was rounded from +FFFF...
      ; to +10000... with the 1 getting moved to the carry bit
ENDIF

IFDEF BIG16AND32
      jmp      rounded_past_end
ENDIF

IFDEF BIG32
use_32_bit:
.386
      ; clear value
      mov      dword ptr value[0], 0
      mov      dword ptr value[4], 0
      mov      word ptr value[8],  0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      dx, ss               ; move ss to es for movs
      mov      es, dx               ; ie: move ss:value+9-cx to es:di
      mov      bx, bflength
      dec      bx
      sub      bx, cx               ; cx holds movebytes
      mov      si, word ptr n
      mov      ds, bignum_seg       ; move n to ds:si for movs
      add      si, bx               ; n+bflength-1-movebytes
      rep movsb
      mov      bl, ds:[si]          ; save sign byte, si now points to it
      inc      si                   ; point to exponent
      mov      dx, ds:[si]          ; use dx as exponent
      shl      dx, 3                ; 256^n = 2^(8n)

      ; adjust for negative values
      and      bl, 10000000b           ; determine sign
      jz       not_neg_32
      neg      dword ptr value[0]      ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      dword ptr value[4]
      adc      dword ptr value[4], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
not_neg_32:

      cmp      byte ptr value[8], 0          ; test for 0
      jnz      top_shift_32
      fldz
      jmp      return

      ; Shift until most signifcant bit is set.
top_shift_32:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom_shift_32
      dec      dx                      ; decrement exponent
      shl      dword ptr value[0], 1   ; shift left the 9 byte number
      rcl      dword ptr value[4], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_32
bottom_shift_32:

      ; round last byte
      cmp      byte ptr value[0], 80h  ;
      jb       bottom                  ; no rounding necessary
      add      dword ptr value[1], 1
      adc      dword ptr value[5], 0
      jnc      bottom

      ; to get to here, the pattern was rounded from +FFFF...
      ; to +10000... with the 1 getting moved to the carry bit
ENDIF

rounded_past_end:
.8086 ; used in 16 it code as well
      mov      byte ptr value[8], 10000000b
      inc      dx                      ; adjust the exponent

bottom:
      ; adjust exponent
      add      dx, 3FFFh+7             ; unbiased -> biased, + adjusted
      or       dh, bl                  ; set sign bit if set
      mov      word ptr value[9], dx

      ; unlike float and double, long double is returned on fpu stack
      fld      real10 ptr value[1]    ; load return value
return:
      mov      ds, ax                  ; restore ds
      ret

bftofloat   endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL floattobf(bf_t n, LDBL f);
; converts a 10 byte real to a bf number
;
floattobf   PROC USES di si, n:bf_t, f:REAL10
   LOCAL value[9]:BYTE   ; 9=8+1
; I figured out a way to do this with no local variables,
; but it's not worth the extra overhead.

      invoke   clear_bf, n

      ; check to see if f is 0
      cmp      byte ptr f[7], 0        ; f[7] can only be 0 if f is 0
;      jz       return                  ; if f is 0, bailout now
      jnz      over_return
      jmp      return                  ; if f is 0, bailout now
over_return:

      mov      cx, 9                   ; need up to 9 bytes
      cmp      bflength, 10            ; but no more than bflength-1
      jae      movebytes_set
      mov      cx, bflength            ; bflength is less than 10
      dec      cx                      ; movebytes = bflength-1, 1 byte padding
movebytes_set:

IFDEF BIG16AND32
      cmp     cpu, 386              ; check cpu
      jae     use_32_bit            ; use faster 32 bit code if possible
ENDIF

IFDEF BIG16
; 16 bit code
      ; copy bytes from f's mantissa to value
      mov      byte ptr value[0], 0    ; clear least sig byte
      mov      ax, word ptr f[0]
      mov      word ptr value[1], ax
      mov      ax, word ptr f[2]
      mov      word ptr value[3], ax
      mov      ax, word ptr f[4]
      mov      word ptr value[5], ax
      mov      ax, word ptr f[6]
      mov      word ptr value[7], ax

      ; get exponent in dx
      mov      dx, word ptr f[8]       ; location of exponent
      and      dx, 7FFFh               ; remove sign bit
      sub      dx, 3FFFh+7             ; biased -> unbiased, + adjust

      ; Shift down until exponent is a mult of 8 (2^8n=256n)
top_shift_16:
      test     dx, 111b                ; expon mod 8
      jz       bottom
      inc      dx                      ; increment exponent
      shr      word ptr value[7], 1    ; shift right the 9 byte number
      rcr      word ptr value[5], 1
      rcr      word ptr value[3], 1
      rcr      word ptr value[1], 1
      rcr      byte ptr value[0], 1    ; notice this last one is byte ptr
      jmp      top_shift_16
ENDIF

IFDEF BIG32
use_32_bit:
.386
      ; copy bytes from f's mantissa to value
      mov      byte ptr value[0], 0    ; clear least sig byte
      mov      eax, dword ptr f[0]
      mov      dword ptr value[1], eax
      mov      eax, dword ptr f[4]
      mov      dword ptr value[5], eax

      ; get exponent in dx
      mov      dx, word ptr f[8]       ; location of exponent
      and      dx, 7FFFh               ; remove sign bit
      sub      dx, 3FFFh+7             ; biased -> unbiased, + adjust

      ; Shift down until exponent is a mult of 8 (2^8n=256n)
top_shift_32:
      test     dx, 111b                ; expon mod 8
      jz       bottom
      inc      dx                      ; increment exponent
      shr      dword ptr value[5], 1   ; shift right the 9 byte number
      rcr      dword ptr value[1], 1
      rcr      byte ptr value[0], 1    ; notice this last one is byte ptr
      jmp      top_shift_32
ENDIF

bottom:
.8086
      ; Don't bother rounding last byte as it would only make a difference
      ; when bflength < 9, and then only on the last bit.

      ; move data into place, from value to n
      lea      si, value+9
      sub      si, cx               ; cx holds movebytes
      mov      ax, ds               ; save ds
      mov      bx, ss               ; copy ss to ds for movs
      mov      ds, bx               ; ds:si
      mov      di, word ptr n
      mov      es, bignum_seg       ; move n to es:di for movs
      add      di, bflength
      dec      di
      sub      di, cx               ; cx holds movebytes
      rep movsb
      inc      di
      mov      cl, 3
      sar      dx, cl               ; divide expon by 8, 256^n=2^8n
      mov      word ptr es:[di], dx ; store exponent
      mov      ds, ax               ; restore ds

      ; get sign
      test     byte ptr f[9], 10000000b           ; test sign bit
      jz       not_negative
      invoke   neg_a_bf, n
not_negative:
return:
      mov      ax, word ptr n
      mov      dx, word ptr n+2        ; return r in dx:ax
      ret
floattobf   endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL bntofloat(bf_t n);
; converts a bn number to a 10 byte real
; (the most speed critical of these to/from float routines)
bntofloat   PROC USES di si, n:bn_t
   LOCAL value[11]:BYTE   ; 11=10+1

      ; determine the most significant byte, not 0 or FF
      mov      si, word ptr n
      mov      es, bignum_seg
      dec      si
      add      si, bnlength            ; n+bnlength-1
      mov      bl, es:[si]             ; top byte
      mov      cx, bnlength            ; initialize cx with full bnlength
      cmp      bl, 0                   ; test top byte against 0
      je       determine_sig_bytes
      cmp      bl, 0FFh                ; test top byte against -1
      jne      sig_bytes_determined

determine_sig_bytes:
      dec      cx                      ; now bnlength-1
top_sig_byte:
      dec      si                      ; previous byte
      cmp      es:[si], bl             ; does it have the right stuff?
      jne      sig_bytes_determined    ; (ie: does it match top byte?)
      loop     top_sig_byte            ; decrement cx and repeat

; At this point, it must be 0 with no sig figs at all
; or -1/(256^bnlength), one bit away from being zero.
      cmp      bl, 0                   ; was it zero?
      jnz      not_zero                ; no, it was a very small negative
                                       ; yes
      fldz                             ; return zero
      jmp      return
not_zero:
      mov      ax, intlength
      sub      ax, bnlength
      mov      cl, 3
      shl      ax, cl                  ; 256^n=2^8n, now more like movebits
      add      ax, 3FFFh+0             ; bias, no adjustment necessary
      or       ah, 10000000b           ; turn on sign flag
      mov      word ptr value[9], ax   ; store exponent
      mov      word ptr value[7], 8000h ; store mantissa of 1 in most sig bit
      ; clear rest of value that is actually used
      mov      word ptr value[1], 0
      mov      word ptr value[3], 0
      mov      word ptr value[5], 0

      fld      real10 ptr value[1]
      jmp      return

sig_bytes_determined:
      mov      dx, cx               ; save in dx for later
      cmp      cx, 9-1              ; no more than cx bytes
      jb       set_movebytes
      mov      cx, 9-1              ; up to 8 bytes
set_movebytes:                      ; cx now holds movebytes
                                    ; si still points to most non-0 sig byte
      sub      si, cx               ; si now points to first byte to be moved
      inc      cx                   ; can be up to 9

IFDEF BIG16AND32
      cmp     cpu, 386              ; check cpu
;      jae     use_32_bit            ; use faster 32 bit code if possible
      jb      not_use_32_bit
      jmp     use_32_bit            ; use faster 32 bit code if possible
not_use_32_bit:
ENDIF

IFDEF BIG16
; 16 bit code
      ; clear value
      mov      word ptr value[0], 0
      mov      word ptr value[2], 0
      mov      word ptr value[4], 0
      mov      word ptr value[6], 0
      mov      word ptr value[8], 0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value  ; es:si still holds first move byte of n
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      ax, ss               ; move ss to es
      mov      es, ax               ; value[9] is in es:di
      mov      ax, ds               ; save ds
      mov      ds, bignum_seg       ; first move byte of n is now in ds:si
      rep movsb
      mov      ds, ax               ; restore ds

      ; adjust for negative values
      xor      ax, ax                  ; use ax as a flag
      ; get sign flag                  ; top byte is still in bl
      and      bl, 10000000b           ; isolate the sign bit
      jz       not_neg_16
      neg      word ptr value[0]       ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      word ptr value[2]
      adc      word ptr value[2], 0
      not      word ptr value[4]
      adc      word ptr value[4], 0
      not      word ptr value[6]
      adc      word ptr value[6], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
      jnc      not_neg_16              ; normal
      mov      byte ptr value[8], 10000000b    ;n was FFFF...0000...
      inc      ax                      ; set ax to 1 to flag this special case

not_neg_16:
      sub      dx, bnlength            ; adjust exponent
      add      dx, intlength           ; adjust exponent
      mov      cl, 3
      shl      dx, cl                  ; 256^n=2^8n
      add      dx, ax                  ; see special case above
      ; Shift until most signifcant bit is set.
top_shift_16:
      test     byte ptr value[8], 10000000b  ; test msb
;      jnz      bottom
      jz       over_bottom
      jmp      bottom
over_bottom:
      dec      dx                      ; decrement exponent
      shl      word ptr value[0], 1    ; shift left the 9 byte number
      rcl      word ptr value[2], 1
      rcl      word ptr value[4], 1
      rcl      word ptr value[6], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_16

; don't bother rounding, not really needed while speed is.
ENDIF

IFDEF BIG32
use_32_bit:
.386
      ; clear value
      mov      dword ptr value[0], 0
      mov      dword ptr value[4], 0
      mov      word ptr value[8],  0
      mov      byte ptr value[10], 0

      ; copy bytes from n to value  ; es:si still holds first move byte of n
      lea      di, value+9
      sub      di, cx               ; cx holds movebytes
      mov      ax, ss               ; move ss to es
      mov      es, ax               ; value[9] is in es:di
      mov      ax, ds               ; save ds
      mov      ds, bignum_seg       ; first move byte of n is now in ds:si
      rep movsb
      mov      ds, ax               ; restore ds

      ; adjust for negative values
      xor      ax, ax                  ; use ax as a flag
      ; get sign flag                  ; top byte is still in bl
      and      bl, 10000000b           ; determine sign
      jz       not_neg_32
      neg      dword ptr value[0]      ; take the negative of the 9 byte number
      cmc                              ; toggle carry flag
      not      dword ptr value[4]
      adc      dword ptr value[4], 0
      not      byte ptr value[8]       ; notice this last one is byte ptr
      adc      byte ptr value[8], 0
      jnc      not_neg_32              ; normal
      mov      byte ptr value[8], 10000000b    ;n was FFFF...0000...
      inc      ax                      ; set ax to 1 to flag this special case

not_neg_32:
      sub      dx, bnlength            ; adjust exponent
      add      dx, intlength           ; adjust exponent
      shl      dx, 3                   ; 256^n=2^8n
      add      dx, ax                  ; see special case above
      ; Shift until most signifcant bit is set.
top_shift_32:
      test     byte ptr value[8], 10000000b  ; test msb
      jnz      bottom
      dec      dx                      ; decrement exponent
      shl      dword ptr value[0], 1   ; shift left the 9 byte number
      rcl      dword ptr value[4], 1
      rcl      byte ptr value[8], 1    ; notice this last one is byte ptr
      jmp      top_shift_32

; don't bother rounding, not really needed while speed is.
ENDIF

bottom:
.8086
      ; adjust exponent
      add      dx, 3FFFh+7-8           ; unbiased -> biased, + adjusted
      or       dh, bl                  ; set sign bit if set
      mov      word ptr value[9], dx

      ; unlike float and double, long double is returned on fpu stack
      fld      real10 ptr value[1]    ; load return value
return:
      ret

bntofloat   endp

;
; LDBL floattobn(bf_t n, LDBL f) is in BIGNUM.C
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; These last two functions do not use bignum type numbers, but take
; long doubles as arguments.  These routines are called by the C code.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL extract_256(LDBL f, int *exp_ptr)
;
; extracts the mantissa and exponant of f
; finds m and n such that 1<=|m|<256 and f = m*256^n
; n is stored in *exp_ptr and m is returned, sort of like frexp()

extract_256   PROC f:real10, exp_ptr: ptr sword
local  expon:sword, exf:real10, tmp_word:word

        fld     f               ; f
        ftst                    ; test for zero
        fstsw   tmp_word
        fwait
        mov     ax,tmp_word
        sahf
        jnz     not_zero        ; proceed

        mov     bx, exp_ptr
        mov     word ptr [bx], 0    ; save = in *exp_ptr
        jmp     bottom          ; f, which is zero, is already on stack

not_zero:

; since a key fpu operation, fxtract, is not emulated by the MS floating
; point library, separate code is included under use_emul:
        cmp     fpu, 0
        je      use_emul

                                ; f is already on stack
        fxtract                 ; mant exp, where f=mant*2^exp
        fxch                    ; exp mant
        fistp   expon           ; mant
        fwait
        mov     ax, expon
        mov     dx, ax          ; make copy for later use

        cmp     ax, 0           ;
        jge     pos_exp         ; jump if exp >= 0

                                ; exp is neg, adjust exp
        add     ax, 8           ; exp+8

pos_exp:
; adjust mantissa
        and     ax, 7           ; ax mod 8
        jz      adjust_exponent ; don't bother with zero adjustments
        mov     expon, ax       ; use expon as a temp var
        fild    expon           ; exp mant

        fxch                    ; mant exp
        fscale                  ; mant*2^exp exp
        fstp    st(1)           ; mant*2^exp (store in 1 and pop)

adjust_exponent:
        mov     cl, 3
        sar     dx, cl          ; exp / 8
        mov     bx, exp_ptr
        mov     [bx], dx        ; save in *exp_ptr

        fwait
        jmp     bottom


use_emul:
; emulate above code by direct manipulation of 80 bit floating point format
                                    ; f is already on stack
        fstp    exf

        mov     ax, word ptr exf+8  ; get word with the exponent in it
        mov     dx, ax              ; make copy for later use

        and     dx, 8000h           ; keep just the sign bit
        or      dx, 3FFFh           ; 1<=f<2

        and     ax, 7FFFh           ; throw away the sign bit
        sub     ax, 3FFFh           ; unbiased -> biased
        mov     bx, ax
        cmp     bx, 0
        jge     pos_exp_emul
        add     bx, 8               ; adjust negative exponent
pos_exp_emul:
        and     bx, 7               ; bx mod 8
        add     dx, bx
        mov     word ptr exf+8, dx  ; put back word with the exponent in it

        mov     cl, 3
        sar     ax, cl              ; div by 8,  2^(8n) = 256^n
        mov     bx, exp_ptr
        mov     [bx], ax            ; save in *exp_ptr

        fld     exf                 ; for return value

bottom:
        ; unlike float and double, long double is returned on fpu stack
        ret
extract_256   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LDBL scale_256( LDBL f, int n );
; calculates and returns the value of f*256^n
; sort of like ldexp()
;
; n must be in the range -2^12 <= n < 2^12 (2^12=4096),
; which should not be a problem

scale_256   PROC f:real10, n: sword

        cmp     n, 0
        jne     non_zero
        fld     f
        jmp     bottom          ; don't bother with scales of zero

non_zero:
        mov     cl, 3
        shl     n, cl           ; 8n
        fild    n               ; 8n
        fld     f               ; f 8n
; the fscale range limits for 8087/287 processors won't be a problem here
        fscale                  ; new_f=f*2^(8n)=f*256^n  8n
        fstp    st(1)           ; new_f

bottom:
        ; unlike float and double, long double is returned on fpu stack
        ret
scale_256   ENDP

END
