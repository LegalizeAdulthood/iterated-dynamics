;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Wesley Loewer's attempt at writing his own
; 80x87 assembler implementation of Lyapunov fractals
; based on lyapunov_cycles_in_c() in miscfrac.c
;
; Nicholas Wilt, April 1992, originally wrote an 80x87 assembler
; implementation of Lyapunov fractals, but I couldn't get his
; lyapunov_cycles() to work right with fractint.
; So I'm starting mostly from scratch with credits to Nicholas Wilt's
; code marked with NW.

.8086
.8087
.MODEL medium,c

; Increase the overflowcheck value at your own risk.
; Bigger value -> check less often -> faster run time, increased risk of overflow.
; I've had failures with 6 when using "set no87" as emulation can be less forgiving.
overflowcheck EQU 5

EXTRN   Population:QWORD,Rate:QWORD
EXTRN   colors:WORD, maxit:DWORD, lyaLength:WORD
EXTRN   lyaRxy:WORD, LogFlag:DWORD
EXTRN   fpu:WORD
EXTRN   filter_cycles:DWORD

.DATA
ALIGN 2
BigNum      DD      100000.0
half        DD      0.5                 ; for rounding to integers
e_10        DQ      22026.4657948       ; e^10
e_neg10     DQ      4.53999297625e-5    ; e^(-10)

.CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; BifurcLambda - not used in lyapunov.asm, but used in miscfrac.c
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC  BifurcLambda
BifurcLambda    PROC
;   Population = Rate * Population * (1 - Population);
;   return (fabs(Population) > BIG);
        push    bp              ; Establish stack frame
        mov     bp,sp           ;
        sub     sp,2            ; Local for 80x87 flags
        fld     Population      ; Get population
        fld     st              ; Population Population
        fld1                    ; 1 Population Population
        fsubr                   ; 1 - Population Population
        fmul                    ; Population * (1 - Population)
        fmul    Rate            ; Rate * Population * (1 - Population)
        fst     Population      ; Store in Population
        fabs                    ; Take absolute value
        fcomp   BigNum          ; Compare to 100000.0
        fstsw   [bp-2]          ; Return 1 if greater than 100000.0
        mov     ax,[bp-2]       ;
        sahf                    ;
        ja      Return1         ;
        xor     ax,ax           ;
        jmp     short Done      ;
Return1:
        mov     ax,1            ;
Done:   inc     sp              ; Deallocate locals
        inc     sp              ;
        pop     bp              ; Restore stack frame
        ret                     ; Return
BifurcLambda    ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; OneMinusExpMacro - calculates e^x
; parameter : x in st(0)
; returns   : e^x in st(0)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OneMinusExpMacro   MACRO
LOCAL not_387, positive_x, was_positive_x
LOCAL long_way_386, long_way_286
LOCAL positive_x_long, was_positive_x_long, done

; To take e^x, one can use 2^(x*log_2(e)), however on on 8087/287
; the valid domain for x is 0 <= x <= 0.5  while the 387/487 allows
; -1.0 <= x <= +1.0.

; I wrote the following 387+ specific code to take advantage of the
; improved domain in the f2xm1 command, but the 287- code seems to work
; about as fast.

        cmp     fpu,387                 ; is fpu >= 387 ?
        jl      not_387

;.386   ; a 387 or better is present so we might as well use .386/7 here
;.387   ; so "fstsw ax" can be used.  Note that the same can be accomplished
.286    ; with .286/7 which is recognized by more more assemblers.
.287    ; (ie: MS QuickAssembler doesn't recognize .386/7)

; |x*log_2(e)| must be <= 1.0 in order to work with 386 or greater.
; It usually is, so it's worth taking these extra steps to check.
        fldl2e                          ; log_2(e) x
        fmul                            ; x*log_2(e)
;begining of short_way code
        fld1                            ; 1 x*log_2(e)
        fld     st(1)                   ; x*log_2(e) 1 x*log_2(e)
        fabs                            ; |x*log_2(e)| 1 x*log_2(e)
        fcompp                          ; x*log_2(e)
        fstsw   ax
        sahf
        ja      long_way_386            ; do it the long way
        f2xm1                           ; e^x-1=2^(x*log_2(e))-1
        fchs                            ; 1-e^x which is what we wanted anyway
        jmp     done                    ; done here.

long_way_386:
; mostly taken from NW's code
        fld     st                      ; x x
        frndint                         ; i x
        fsub    st(1),st                ; i x-i
        fxch                            ; x-i i
        f2xm1                           ; e^(x-i)-1 i
        fld1                            ; 1 e^(x-i)-1 i
        fadd                            ; e^(x-i) i
        fscale                          ; e^x i
        fstp    st(1)                   ; e^x
        fld1                            ; 1 e^x
        fsubr                           ; 1-e^x
        jmp     done

not_387:
.8086
.8087
; |x*log_2(e)| must be <= 0.5 in order to work with 286 or less.
; It usually is, so it's worth taking these extra steps to check.
        fldl2e                          ; log_2(e) x
        fmul                            ; x*log_2(e)

;begining of short_way code
        fld     st                      ; x*log_2(e) x*log_2(e)
        fabs                            ; |x*log_2(e)| x*log_2(e)
        fcomp   half                    ; x*log_2(e)
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        ja      long_way_286            ; do it the long way

; 286 or less requires x>=0, if x is negative, use e^(-x) = 1/e^x
        ftst                            ; x
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        jae     positive_x
        fabs                            ; -x
        f2xm1                           ; e^(-x)-1=2^(-x*log_2(e))-1
        fld1                            ; 1 e^(-x)-1
        fadd    st,st(1)                ; e^(-x) e^(-x)-1
        fdiv                            ; 1-e^x=(e^(-x)-1)/e^(-x)
        jmp     SHORT done              ; done here.

positive_x:
        f2xm1                           ; e^x-1=2^(x*log_2(e))-1
        fchs                            ; 1-e^x which is what we wanted anyway
        jmp     SHORT done              ; done here.

long_way_286:
; mostly taken from NW's code
        fld     st                      ; x x
        frndint                         ; i x
        fsub    st(1),st                ; i x-i
        fxch                            ; x-i i
; x-i could be pos or neg
; 286 or less requires x>=0, if negative, use e^(-x) = 1/e^x
        ftst
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        jae     positive_x_long_way
        fabs                            ; i-x
        f2xm1                           ; e^(i-x)-1 i
        fld1                            ; 1 e^(i-x)-1 i
        fadd    st(1),st                ; 1 e^(i-x) i
        fdivr                           ; e^(x-i) i
        fscale                          ; e^x i
        fstp    st(1)                   ; e^x
        fld1                            ; 1 e^x
        fsubr                           ; 1-e^x
        jmp     SHORT done              ; done here.

positive_x_long_way:                    ; x-i
        f2xm1                           ; e^(x-i)-1 i
        fld1                            ; 1 e^(x-i)-1 i
        fadd                            ; e^(x-i) i
        fscale                          ; e^x i
        fstp    st(1)                   ; e^x
        fld1                            ; 1 e^x
        fsubr                           ; 1-e^x
done:
ENDM    ; OneMinusExpMacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; modeled from lyapunov_cycles_in_c() in miscfrac.c
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;int lyapunov_cycles_in_c(int filter_cycles, double a, double b) {
;    int color, count, i, lnadjust;
;    double lyap, total, temp;

PUBLIC lyapunov_cycles
lyapunov_cycles  PROC  USES si di, a:QWORD, b:QWORD
    LOCAL color:WORD, i:WORD, lnadjust:WORD
    LOCAL halfmax:DWORD, status_word:WORD
    LOCAL total:QWORD


;    for (i=0; i<filter_cycles; i++) {
;        for (count=0; count<lyaLength; count++) {
;            Rate = lyaRxy[count] ? a : b;
;            if (curfractalspecific->orbitcalc()) {
;                overflow = TRUE;
;                goto jumpout;
;                }
;            }
;        }

; Popalation is left on the stack during most of this procedure
        fld     Population              ; Population
        mov     bx,1                    ; using bx for overflowcheck counter
        mov     ax,word ptr filter_cycles
        mov     dx,word ptr filter_cycles+2
        add     ax,1
        adc     dx,0                    ; add 1 because dec at beginning
        mov     si, ax                  ; using si for low part of i
        mov     i, dx
filter_cycles_top:
        dec     si
        jne     hop_skip_and_jump       ; si > 0 :
        cmp     i,0
        je      filter_cycles_bottom
        dec     i
hop_skip_and_jump:
        mov     cx, lyaLength           ; using cx for count
        mov     di,OFFSET lyaRxy        ; use di as lyaRxy[] index
        ; no need to compare cx with lyaLength at top of loop
        ; since lyaLength is guaranteed to be > 0

filter_cycles_count_top:
        cmp     WORD PTR [di],0         ; lyaRxy[count] ?
        jz      filter_cycles_use_b
        fld     a                       ; if lyaRxy[count]==true,  use a
        jmp     SHORT filter_cycles_used_a
filter_cycles_use_b:
        fld     b                       ; if lyaRxy[count]==false, use b
filter_cycles_used_a:
        ; leave Rate on stack for use in BifurcLambdaMacro
        ; BifurcLambdaMacro               ; returns in flag register

        fld     st(1)                   ; Population Rate Population
        fmul    st,st                   ; Population^2 Rate Population
        fsubp   st(2),st                ; Rate Population-Population^2
                                        ;      =Population*(1-Population)
        fmul                            ; Rate*Population*(1-Population)
        dec     bx                      ; decrement overflowcheck counter
        jnz     filter_cycles_skip_overflow_check ; can we skip check?
        fld     st                      ; NewPopulation NewPopulation
        fabs                            ; Take absolute value
        fcomp   BigNum                  ; Compare to 100000.0
        fstsw   status_word             ;
        fwait
        mov     bx,overflowcheck        ; reset overflowcheck counter
        mov     ax,status_word          ;
        sahf                            ;  NewPopulation

;        ja      overflowed_with_Pop
        jna     filter_cycles_skip_overflow_check
        jmp     overflowed_with_Pop
filter_cycles_skip_overflow_check:
        add     di,2                    ; di += sizeof(*lyaRxy)
        loop    filter_cycles_count_top ; if --cx > 0 then loop

        jmp     filter_cycles_top       ; let's do it again

filter_cycles_bottom:


;    total = 1.0;
;    lnadjust = 0;

        fld1
        fstp    total
        mov     lnadjust,0

;    for (i=0; i < maxit/2; i++) {
;        for (count = 0; count < lyaLength; count++) {
;            Rate = lyaRxy[count] ? a : b;
;            if (curfractalspecific->orbitcalc()) {
;                overflow = TRUE;
;                goto jumpout;
;                }
;            temp = fabs(Rate-2.0*Rate*Population);
;                if ((total *= (temp))==0) {
;                overflow = TRUE;
;                goto jumpout;
;                }
;            }
;        while (total > 22026.4657948) {
;            total *= 0.0000453999297625;
;            lnadjust += 10;
;            }
;        while (total < 0.0000453999297625) {
;            total *= 22026.4657948;
;            lnadjust -= 10;
;            }
;        }

        ; don't forget Population is still on stack
        mov     ax,word ptr maxit       ; calculate halfmax
        mov     dx,word ptr maxit+2     ; calculate halfmax
        shr     dx,1
        rcr     ax,1
        mov     word ptr halfmax,ax
        mov     word ptr halfmax+2,dx
        add     ax,1
        adc     dx,0                   ; add 1 because dec at beginning
        mov     i,dx                   ; count down from halfmax
        mov     si,ax                  ; using si for low part of i
halfmax_top:
        dec     si
        jne     init_halfmax_count     ; si > 0 ?
        cmp     i,0
        je      step_to_halfmax_bottom
        dec     i
        jmp     short init_halfmax_count      ; yes, continue on with loop
step_to_halfmax_bottom:
        jmp     halfmax_bottom          ; if not, end loop
init_halfmax_count:
        mov     cx, lyaLength           ; using cx for count
        mov     di,OFFSET lyaRxy        ; use di as lyaRxy[] index
        ; no need to compare cx with lyaLength at top of loop
        ; since lyaLength is guaranteed to be > 0

halfmax_count_top:
        cmp     WORD PTR [di],0         ; lyaRxy[count] ?
        jz      halfmax_use_b
        fld     a                       ; if lyaRxy[count]==true,  use a
        jmp     SHORT halfmax_used_a
halfmax_use_b:
        fld     b                       ; if lyaRxy[count]==false, use b
halfmax_used_a:
        ; save Rate, but leave it on stack for use in BifurcLambdaMacro
        fst     Rate                    ; save for not_overflowed use
        ;BifurcLambdaMacro               ; returns in flag register

        fld     st(1)                   ; Population Rate Population
        fmul    st,st                   ; Population^2 Rate Population
        fsubp   st(2),st                ; Rate Population-Population^2
                                        ;      =Population*(1-Population)
        fmul                            ; Rate*Population*(1-Population)
        dec     bx                      ; decrement overflowcheck counter
        jnz     not_overflowed          ; can we skip overflow check?
        fld     st                      ; NewPopulation NewPopulation
        fabs                            ; Take absolute value
        fcomp   BigNum                  ; Compare to 100000.0
        fstsw   status_word             ;
        fwait
        mov     bx,overflowcheck        ; reset overflowcheck counter
        mov     ax,status_word          ;
        sahf                            ;

        jbe     not_overflowed

        ; putting overflowed_with_Pop: here saves 2 long jumps in inner loops
overflowed_with_Pop:
        fstp    Population              ; save Population and clear stack
        jmp     color_zero              ;

not_overflowed:                         ; Population
        fld     st                      ; Population Population
        ; slightly faster _not_ to fld Rate here
        fmul    Rate                    ; Rate*Population Population
        fadd    st,st                   ; 2.0*Rate*Population Population
        fsubr   Rate                    ; Rate-2.0*Rate*Population Population
        fabs                            ; fabs(Rate-2.0*Rate*Population) Population
        fmul    total                   ; total*fabs(Rate-2.0*Rate*Population) Population
        ftst                            ; compare to 0
        fstp    total                   ; save the new total
        fstsw   status_word             ; Population
        fwait
        mov     ax,status_word
        sahf
        jz      overflowed_with_Pop     ; if total==0, then treat as overflow

        add     di,2                    ; di += sizeof(*lyaRxy)
        loop    halfmax_count_top       ; if --cx > 0 then loop

        fld     total                   ; total Population
too_big_top:
        fcom    e_10                    ; total Population
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        jna     too_big_bottom
        fmul    e_neg10                 ; total*e_neg10 Population
        add     lnadjust,10
        jmp     SHORT too_big_top
too_big_bottom:

too_small_top:                          ; total Population
        fcom    e_neg10                 ; total Population
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        jnb     too_small_bottom
        fmul    e_10                    ; total*e_10 Population
        sub     lnadjust,10
        jmp     SHORT too_small_top
too_small_bottom:

        fstp    total                   ; save as total
        jmp     halfmax_top             ; let's do it again

halfmax_bottom:
; better make another check, just to be sure
                                        ; NewPopulation
        cmp     bx,overflowcheck        ; was overflow just checked?
        jl      last_overflowcheck      ; if not, better check one last time
        fstp    Population              ; save Population and clear stack
        jmp     short jumpout           ; skip overflowcheck

last_overflowcheck:
        fst     Population              ; save new Population
        fabs                            ; |NewPopulation|
        fcomp   BigNum                  ; Compare to 100000.0
        fstsw   status_word             ;
        fwait
        mov     ax,status_word
        sahf
        ja      color_zero              ; overflowed

jumpout:

;    if (overflow || total <= 0 || (temp = log(total) + lnadjust) > 0)
;        color = 0;
;    else {
;        if (LogFlag)
;            lyap = -temp/((double) lyaLength*i);
;        else
;            lyap = 1 - exp(temp/((double) lyaLength*i));
;        color = 1 + (int)(lyap * (colors-1));
;        }
;    return color;
;}

; no use testing for the overflow variable as you
; cannot get here and have overflow be true
        fld     total                   ; total
        ftst                            ; is total <= 0 ?
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        ja      total_not_neg           ; if not, continue
        fstp    st                      ; pop total from stack
        jmp     SHORT color_zero        ; if so, set color to 0

total_not_neg:                          ; total is still on stack
        fldln2                          ; ln(2) total
        fxch                            ; total ln(2)
        fyl2x                           ; ln(total)=ln(2)*log_2(total)
        fiadd   lnadjust                ; ln(total)+lnadjust
        ftst                            ; compare against 0
        fstsw   status_word
        fwait
        mov     ax,status_word
        sahf
        jbe     not_positive
        fstp    st                      ; pop temp from stack

color_zero:
        xor     ax,ax                   ; return color 0
        jmp     thats_all

not_positive:                           ; temp is still on stack
        fild    lyaLength               ; lyaLength temp
        fimul   halfmax                 ; lyaLength*halfmax temp
        fdiv                            ; temp/(lyaLength*halfmax)
        cmp     word ptr LogFlag,0      ; is LogFlag set?
        jz      LogFlag_not_set         ; if !LogFlag goto LogFlag_not_set:
        cmp     word ptr LogFlag+2,0      ; is LogFlag set?
        jz      LogFlag_not_set         ; if !LogFlag goto LogFlag_not_set:
        fchs                            ; -temp/(lyaLength*halfmax)
        jmp     calc_color

LogFlag_not_set:                        ; temp/(lyaLength*halfmax)
        OneMinusExpMacro                ; 1-exp(temp/(lyaLength*halfmax))
calc_color:
        ; what is now left on the stack is what the C code calls lyap
                                        ; lyap
        mov     ax,colors               ; colors
        dec     ax                      ; colors-1
        mov     i,ax                    ; temporary storage
        fimul   i                       ; lyap*(colors-1)
        ; the "half" makes ASM round like C does
        fsub    half                    ; sub 0.5 for rounding purposes
        fistp   i                       ; temporary = (int)(lyap*(colors-1))
        fwait                           ; one moment please...
        mov     ax,i                    ; ax = temporary
        inc     ax                      ; ax = 1 + (int)(lyap * (colors-1));

thats_all:
        mov     color, ax
        ret
lyapunov_cycles  endp

END
