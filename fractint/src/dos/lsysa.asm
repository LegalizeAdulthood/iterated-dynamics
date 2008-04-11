; LSYSA.ASM: assembler support routines for optimized L-System code
; Nicholas Wilt, 11/13/91.
; Updated to work with integer/FP code, 7/93.

ifdef ??version
   masm51
   quirks
endif

.MODEL  MEDIUM,C

lsys_turtlestatei   STRUC
counter             DB      ?
angle               DB      ?
reverse             DB      ?
stackoflow          DB      ?
maxangle            DB      ?
dmaxangle           DB      ?
curcolor            DB      ?
dummy               DB      ?
ssize               DD      ?
realangle           DD      ?
xpos                DD      ?
ypos                DD      ?
xmin                DD      ?
ymin                DD      ?
xmax                DD      ?
ymax                DD      ?
aspect              DD      ?
num                 DD      ?
lsys_turtlestatei   ENDS

EXTRN   overflow:WORD
EXTRN   boxy:DWORD

sins    equ     boxy
coss    equ     boxy + 200      ; 50 * 4 bytes

.CODE

DECANGLE MACRO
        LOCAL   @1
        dec     al
        jge     @1
        mov     al,[bx.dmaxangle]
@1:     mov     [bx.angle],al
        ENDM

INCANGLE MACRO
        LOCAL   @1
        inc     al
        cmp     al,[bx.maxangle]
        jne     @1
        xor     ax,ax
@1:     mov     [bx.angle],al
        ENDM

        PUBLIC  lsysi_doplus

lsysi_doplus    PROC    lsyscmd:ptr
        mov     bx,lsyscmd
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jnz     PlusIncAngle
        DECANGLE
        ret
PlusIncAngle:
        INCANGLE
        ret
lsysi_doplus    ENDP

        PUBLIC  lsysi_dominus

lsysi_dominus   PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jnz     MinusDecAngle
        INCANGLE
        ret
MinusDecAngle:
        DECANGLE
        ret
lsysi_dominus   ENDP

        PUBLIC  lsysi_doplus_pow2

lsysi_doplus_pow2       PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jnz     Plus2IncAngle
        dec     al
        and     al,[bx.dmaxangle]
        mov     [bx.angle],al
        ret
Plus2IncAngle:
        inc     al
        and     al,[bx.dmaxangle]
        mov     [bx.angle],al
        ret
lsysi_doplus_pow2       ENDP

        PUBLIC  lsysi_dominus_pow2

lsysi_dominus_pow2       PROC   lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jz      Minus2IncAngle
        dec     al
        and     al,[bx.dmaxangle]
        mov     [bx.angle],al
        ret
Minus2IncAngle:
        inc     al
        and     al,[bx.dmaxangle]
        mov     [bx.angle],al
        ret
lsysi_dominus_pow2       ENDP

        PUBLIC  lsysi_dopipe_pow2

lsysi_dopipe_pow2       PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        xor     ax,ax
        mov     al,[bx.maxangle]
        shr     ax,1
        xor     dx,dx
        mov     dl,[bx.angle]
        add     ax,dx
        and     al,[bx.dmaxangle]
        mov     [bx.angle],al
        ret
lsysi_dopipe_pow2       ENDP

        PUBLIC  lsysi_dobang

lsysi_dobang    PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.reverse] ; reverse = ! reverse;
        dec     al              ; -1 if was 0; 0 if was 1
        neg     al              ; 1 if was 0; 0 if was 1
        mov     [bx.reverse],al ;
        ret
lsysi_dobang    ENDP

; Some 386-specific leaf functions go here.

.386

        PUBLIC  lsysi_doslash_386

lsysi_doslash_386       PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     eax,[bx.num]
        cmp     [bx.reverse],0
        jnz     DoSlashDec
        add     [bx.realangle],eax
        ret
DoSlashDec:
        sub     [bx.realangle],eax
        ret
lsysi_doslash_386       ENDP

        PUBLIC  lsysi_dobslash_386

lsysi_dobslash_386       PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     eax,[bx.num]
        cmp     [bx.reverse],0
        jz      DoBSlashDec
        add     [bx.realangle],eax
        ret
DoBSlashDec:
        sub     [bx.realangle],eax
        ret
lsysi_dobslash_386       ENDP

        PUBLIC  lsysi_doat_386

lsysi_doat_386  PROC    lsyscmd:ptr ; not used, N:DWORD
        mov     bx,lsyscmd      ; Get pointer
        mov     eax,[bx.ssize]  ; Get size
        imul    [bx.num]        ; Mul by n
        shrd    eax,edx,19
        mov     [bx.ssize],eax  ; Save back
        ret
lsysi_doat_386  ENDP

        PUBLIC  lsysi_dosizegf_386

lsysi_dosizegf_386      PROC    USES SI,lsyscmd:ptr
        mov     si,lsyscmd      ;
        mov     ecx,[si.ssize]  ; Get size; we'll need it twice
   movzx ebx,[si.angle]
ifndef ??version
        mov     eax,coss[ebx*4] ; eax <- coss[angle]
else
   ; TASM-only (this is slower but it should work)
   shl   bx,2
   add   bx,offset DGROUP:coss
        mov     eax,[ebx]       ; eax <- coss[angle]
endif
        imul    ecx             ; Mul by size
        shrd    eax,edx,29      ; eax <- multiply(size, coss[angle], 29)
        add     eax,[si.xpos]   ;
        jno     nooverfl        ;   check for overflow
        mov     overflow,1      ;    oops - flag the user later
nooverfl:
        cmp     eax,[si.xmax]   ; If xpos <= [si.xmax,
        jle     GF1             ;   jump
        mov     [si.xmax],eax   ;
GF1:    cmp     eax,[si.xmin]   ; If xpos >= [si.xmin
        jge     GF2             ;   jump
        mov     [si.xmin],eax   ;
GF2:    mov     [si.xpos],eax   ; Save xpos
ifndef ??version
        mov     eax,sins[ebx*4] ; eax <- sins[angle]
else     ; sins table is 200 bytes before coss
        mov     eax,[ebx-200]   ; eax <- sins[angle]
endif
        imul    ecx             ;
        shrd    eax,edx,29      ;
        add     eax,[si.ypos]   ;
        cmp     eax,[si.ymax]   ; If ypos <= [si.ymax,
        jle     GF3             ;   jump
        mov     [si.ymax],eax   ;
GF3:    cmp     eax,[si.ymin]   ; If ypos >= [si.ymin
        jge     GF4             ;   jump
        mov     [si.ymin],eax   ;
GF4:    mov     [si.ypos],eax   ;
        ret
lsysi_dosizegf_386      ENDP

        PUBLIC  lsysi_dodrawg_386

lsysi_dodrawg_386       PROC    USES SI,lsyscmd:ptr
        mov     si,lsyscmd      ; Get pointer to structure
        mov     ecx,[si.ssize]  ; Because we need it twice
   movzx ebx,[si.angle]
ifndef ??version
        mov     eax,coss[ebx*4] ; eax <- coss[angle]
else
   ; TASM-only (this is slow but it should work)
   shl   bx,2
   add   bx,offset DGROUP:coss
        mov     eax,[ebx]       ; eax <- coss[angle]
endif
        imul    ecx             ;
        shrd    eax,edx,29      ;
        add     [si.xpos],eax   ; xpos += size*coss[angle] >> 29
ifndef ??version
        mov     eax,sins[ebx*4] ; eax <- sins[angle]
else     ; sins table is 200 bytes before coss
        mov     eax,[ebx-200]   ; eax <- sins[angle]
endif
        imul    ecx             ; ypos += size*sins[angle] >> 29
        shrd    eax,edx,29      ;
        add     [si.ypos],eax   ;
        ret                     ;
lsysi_dodrawg_386       ENDP

        END
