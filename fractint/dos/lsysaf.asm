; LSYSAF.ASM: assembler support routines for optimized L-System code
; (floating-point version)
; Nicholas Wilt, 7/93.

.MODEL  MEDIUM,C

lsys_turtlestatef   STRUC
counter             DB      ?
angle               DB      ?
reverse             DB      ?
stackoflow          DB      ?
maxangle            DB      ?
dmaxangle           DB      ?
curcolor            DB      ?
dummy               DB      ?
ssize               DT      ?
realangle           DT      ?
xpos                DT      ?
ypos                DT      ?
xmin                DT      ?
ymin                DT      ?
xmax                DT      ?
ymax                DT      ?
aspect              DT      ?
num                 DD      ?
lsys_turtlestatef   ENDS

EXTRN   overflow:WORD
EXTRN   draw_line:FAR
EXTRN   FPUsincos:FAR
EXTRN   cpu:WORD

EXTRN   boxy:TBYTE

sins_f  equ     boxy
coss_f  equ     boxy + 500      ; 50 * 10 bytes

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

        PUBLIC  lsysf_doplus

lsysf_doplus    PROC    lsyscmd:ptr
        mov     bx,lsyscmd
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jnz     PlusIncAngle
        DECANGLE
        ret
PlusIncAngle:
        INCANGLE
        ret
lsysf_doplus    ENDP

        PUBLIC  lsysf_dominus

lsysf_dominus   PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.angle]
        cmp     [bx.reverse],0
        jnz     MinusDecAngle
        INCANGLE
        ret
MinusDecAngle:
        DECANGLE
        ret
lsysf_dominus   ENDP

        PUBLIC  lsysf_doplus_pow2

lsysf_doplus_pow2       PROC    lsyscmd:ptr
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
lsysf_doplus_pow2       ENDP

        PUBLIC  lsysf_dominus_pow2

lsysf_dominus_pow2       PROC   lsyscmd:ptr
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
lsysf_dominus_pow2       ENDP

        PUBLIC  lsysf_dopipe_pow2

lsysf_dopipe_pow2       PROC    lsyscmd:ptr
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
lsysf_dopipe_pow2       ENDP

        PUBLIC  lsysf_dobang

lsysf_dobang    PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        mov     al,[bx.reverse] ; reverse = ! reverse;
        dec     al              ; -1 if was 0; 0 if was 1
        neg     al              ; 1 if was 0; 0 if was 1
        mov     [bx.reverse],al ;
        ret
lsysf_dobang    ENDP

        PUBLIC  lsysf_doat

lsysf_doat      PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        fld     tbyte ptr [bx.num] ; Get N.
                                ; FPU stk: N xpos ypos size aspect
        fmulp   st(3),st        ; Multiply size by it.
        ret
lsysf_doat      ENDP

.386

LSYS_SINCOS     MACRO   OFFS
        LOCAL   UseSinCos, Done

        cmp     cpu,386         ; If CPU >= 386, we can use fsincos.
        jge     UseSinCos       ;

                                ; Otherwise we use FPUsincos.

        sub     sp,50+24        ; Save enough room for entire state
                                ; plus parameter to sin or cos
        fstp    tbyte ptr [bp-OFFS-10] ; Save state
        fstp    tbyte ptr [bp-OFFS-20] ;
        fstp    tbyte ptr [bp-OFFS-30] ;
        fstp    tbyte ptr [bp-OFFS-40] ;
        fld     st              ; realangle remains on FPU stack
        fstp    tbyte ptr [bp-OFFS-50] ;
        fstp    qword ptr [bp-OFFS-50-8] ; Store as parameter to FPUsincos

        lea     ax,[bp-OFFS-50-16]      ; Push pointer to cosine
        push    ax              ;
        lea     ax,[bp-OFFS-50-24]      ; Push pointer to sine
        push    ax              ;
        lea     ax,[bp-OFFS-50-8]       ; Push pointer to parameter
        push    ax              ;
        call    FPUsincos       ; Call it.

        add     sp,6            ; Restore stack

        fld     tbyte ptr [bp-OFFS-50] ; Restore state
        fld     tbyte ptr [bp-OFFS-40] ;
        fld     tbyte ptr [bp-OFFS-30] ;
        fld     tbyte ptr [bp-OFFS-20] ;
        fld     tbyte ptr [bp-OFFS-10] ;
        fld     qword ptr [bp-OFFS-50-24] ; Get sine
        fld     qword ptr [bp-OFFS-50-16] ; Get cosine

        add     sp,50+24        ; Restore stack

        jmp     short Done      ;
UseSinCos:
        fld     st(4)           ; Get angle
        fsincos                 ; c s xpos ypos size aspect realangle
Done:
        ENDM

        PUBLIC  lsysf_dosizedm

lsysf_dosizedm  PROC    lsyscmd:ptr
        LSYS_SINCOS 0           ; Get sine and cosine of angle.
        mov     bx,lsyscmd      ; Get pointer to structure
        fmul    st,st(5)        ; c*aspect s xpos ypos size aspect
        fmul    st,st(4)        ; c*size*aspect s xpos ypos size aspect
        faddp   st(2),st        ; s xpos ypos size aspect
        fmul    st,st(3)        ; s*size xpos ypos size aspect
        faddp   st(2),st        ; xpos ypos size aspect

        push    ax              ; Allocate a local for FP status word

        fld     [bx.xmin]       ; Compare xpos to xmin
        fcomp                   ;
        fstsw   [bp-2]          ; Store status word
        mov     ax,[bp-2]       ;
        sahf                    ;
        jb      SizeDM1         ; Jump if ST > xmin
        fld     st              ;
        fstp    [bx.xmin]       ;
        jmp     short SizeDM2   ;
SizeDM1:
        fld     [bx.xmax]       ; Compare to xmax
        fcomp                   ;
        fstsw   [bp-2]          ; Store status word
        mov     ax,[bp-2]       ;
        sahf                    ;
        ja      SizeDM2         ; Jump if ST < xmax
        fld     st              ;
        fstp    [bx.xmax]       ;
SizeDM2:
        fxch                    ; Swap xpos and ypos
        fld     [bx.ymin]       ; Compare ypos to ymin
        fcomp                   ;
        fstsw   [bp-2]          ; Store status word
        mov     ax,[bp-2]       ;
        sahf                    ;
        jb      SizeDM3         ; Jump if ST > ymin
        fld     st              ;
        fstp    [bx.ymin]       ;
        jmp     short SizeDM4   ;
SizeDM3:
        fld     [bx.ymax]       ; Compare ypos to ymax
        fcomp                   ;
        fstsw   [bp-2]          ;
        mov     ax,[bp-2]       ;
        sahf                    ;
        ja      SizeDM4         ;
        fld     st              ;
        fstp    [bx.ymax]       ;
SizeDM4:
        fxch                    ; Swap xpos and ypos back
        pop     ax              ; Deallocate local for FPU status word

        ret                     ; Done.
lsysf_dosizedm  ENDP

        PUBLIC  lsysf_dosizegf

lsysf_dosizegf  PROC    USES SI,lsyscmd:ptr
        mov     si,lsyscmd      ;
        xor     bx,bx           ; BX <- angle * sizeof(long double)
        mov     bl,[si.angle]   ;
        shl     bx,1            ;
        mov     ax,bx           ;
        shl     ax,1            ;
        shl     ax,1            ;
        add     bx,ax           ;
        fld     st(2)           ; size xpos ypos size aspect
        fld     coss_f[bx]      ; size*cmd->coss[cmd->angle]
        fmul                    ;
        faddp   st(1),st        ; xpos ypos size aspect
        fld     st(2)           ; size xpos ypos size aspect
        fld     sins_f[bx]      ; size*cmd->sins[cmd->angle]
        fmul                    ;
        faddp   st(2),st        ; xpos ypos size aspect

        push    ax              ; Allocate a local for FPU status word

        fld     [si.xmin]       ; Compare xpos to xmin
        fcomp                   ;
        fstsw   [bp-4]          ; Store status word
        mov     ax,[bp-4]       ;
        sahf                    ;
        jb      SizeGF1         ; Jump if ST > xmin
        fld     st              ;
        fstp    [si.xmin]       ;
        jmp     short SizeGF2   ;
SizeGF1:
        fld     [si.xmax]       ; Compare to xmax
        fcomp                   ;
        fstsw   [bp-4]          ; Store status word
        mov     ax,[bp-4]       ;
        sahf                    ;
        ja      SizeGF2         ; Jump if ST < xmax
        fld     st              ;
        fstp    [si.xmax]       ;
SizeGF2:
        fxch                    ; Swap xpos and ypos
        fld     [si.ymin]       ; Compare ypos to ymin
        fcomp                   ;
        fstsw   [bp-4]          ; Store status word
        mov     ax,[bp-4]       ;
        sahf                    ;
        jb      SizeGF3         ; Jump if ST > ymin
        fld     st              ;
        fstp    [si.ymin]       ;
        jmp     short SizeGF4   ;
SizeGF3:
        fld     [si.ymax]       ; Compare ypos to ymax
        fcomp                   ;
        fstsw   [bp-4]          ;
        mov     ax,[bp-4]       ;
        sahf                    ;
        ja      SizeGF4         ;
        fld     st              ;
        fstp    [si.ymax]       ;
SizeGF4:
        fxch                    ; Swap xpos and ypos back
        pop     ax              ; Deallocate local for FPU status word
        ret
lsysf_dosizegf  ENDP

        PUBLIC  lsysf_dodrawg

lsysf_dodrawg   PROC    USES SI,lsyscmd:ptr
        mov     si,lsyscmd      ; Get pointer to structure
        xor     bx,bx           ; Get angle offset
        mov     bl,[si.angle]   ;
        mov     al,10           ;
        mul     bl              ;
        xchg    ax,bx           ;
        fld     coss_f[bx]      ; xpos += cmd->size * cmd->coss[cmd->angle]
        fmul    st,st(3)        ;
        fadd                    ;
        fld     sins_f[bx]      ; ypos += cmd->size * cmd->sins[cmd->angle]
        fmul    st,st(3)        ;
        faddp   st(2),st        ;
        ret                     ;
lsysf_dodrawg   ENDP

        PUBLIC  lsysf_dodrawd

lsysf_dodrawd   PROC    lsyscmd:PTR
        mov     bx,lsyscmd      ; Get pointer to structure
        xor     ax,ax           ; Push last parm to draw_line
        mov     al,[bx.curcolor]
        push    ax              ;
        sub     sp,8            ; Allocate the rest of draw_line's parms
        fist    word ptr [bp-10] ; Store xpos
        fxch                    ; Store ypos
        fist    word ptr [bp-8] ;
        fxch                    ;

        LSYS_SINCOS 10          ; c s xpos ypos size aspect

        fmul    st,st(5)        ; c*aspect s xpos ypos size aspect
        fmul    st,st(4)        ; size*c*aspect s xpos ypos size aspect
        faddp   st(2),st        ; s xpos ypos size aspect
        fmul    st,st(3)        ; s*size xpos ypos size aspect
        faddp   st(2),st        ; xpos ypos size aspect

        fist    word ptr [bp-6] ; Store xpos
        fxch                    ; Store ypos
        fist    word ptr [bp-4] ;
        fxch                    ;

        call    far ptr draw_line       ;
        add     sp,10           ; Remove parameters
        ret                     ;
lsysf_dodrawd   ENDP

        PUBLIC  lsysf_dodrawm

lsysf_dodrawm   PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer to structure, to quiet warning
        LSYS_SINCOS 0           ;
        fmul    st,st(5)        ; c*aspect s xpos ypos size aspect
        fmul    st,st(4)        ; size*c*aspect s xpos ypos size aspect
        faddp   st(2),st        ; s xpos ypos size aspect
        fmul    st,st(3)        ; s*size xpos ypos size aspect
        faddp   st(2),st        ; xpos ypos size aspect
        ret                     ;
lsysf_dodrawm   ENDP

        PUBLIC  lsysf_dodrawf

lsysf_dodrawf   PROC    USES SI, lsyscmd:ptr
        mov     si,lsyscmd              ; Get pointer to structure
        xor     ax,ax                   ; Push curcolor for draw_line
        mov     al,[si.curcolor]        ;
        push    ax                      ;
        sub     sp,8                    ; Allocate the rest of the draw_line call
        fist    word ptr [bp-12]        ; Store xpos in draw_line parms
        fxch                            ; Store ypos in draw_line parms
        fist    word ptr [bp-10]        ;
        fxch                            ;

        xor     bx,bx                   ; BX <- offset into cos/sin arrays
        mov     bl,[si.angle]           ;
        mov     al,10                   ;
        mul     bl                      ;
        xchg    ax,bx                   ;

        fld     st(2)           ; xpos += size*cmd->coss[cmd->angle]
        fld     coss_f[bx]      ;
        fmul
        fadd                    ;
        fist    word ptr [bp-8] ; Store new xpos in draw_line parms

        fld     st(2)           ; ypos += size*cmd->sins[cmd->angle]
        fld     sins_f[bx]      ;
        fmul                    ;
        faddp   st(2),st        ;
        fxch                    ; Store new ypos in draw_line parms
        fist    word ptr [bp-6] ;
        fxch                    ;
        call    far ptr draw_line       ; Call the line-drawing routine
        add     sp,10           ; Deallocate the stuff we pushed
        ret                     ;
lsysf_dodrawf   ENDP

        PUBLIC  lsysf_doslash

lsysf_doslash   PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        fld     tbyte ptr [bx.num]
        cmp     [bx.reverse],0
        jnz     DoSlashDec
        faddp   st(5),st
        ret
DoSlashDec:
        fsubp   st(5),st
        ret
lsysf_doslash   ENDP

        PUBLIC  lsysf_dobslash

lsysf_dobslash   PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer
        fld     tbyte ptr [bx.num]
        cmp     [bx.reverse],0
        jz      DoBSlashDec
        faddp   st(5),st
        ret
DoBSlashDec:
        fsubp   st(5),st
        ret
lsysf_dobslash   ENDP

        PUBLIC  lsys_prepfpu

lsys_prepfpu    PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer to structure
        fld     [bx.realangle]  ; Load: xpos ypos size aspect realangle
        fld     [bx.aspect]     ;
        fld     [bx.ssize]      ;
        fld     [bx.ypos]       ;
        fld     [bx.xpos]       ;
        ret                     ; Return.
lsys_prepfpu    ENDP

        PUBLIC  lsys_donefpu

lsys_donefpu    PROC    lsyscmd:ptr
        mov     bx,lsyscmd      ; Get pointer to structure
        fstp    [bx.xpos]       ; Save: xpos ypos size aspect realangle
        fstp    [bx.ypos]       ;
        fstp    [bx.ssize]      ;
        fstp    [bx.aspect]     ;
        fstp    [bx.realangle]  ;
        ret
lsys_donefpu    ENDP

        END

