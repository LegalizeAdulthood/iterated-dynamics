;       Generic assembler routines that have very little at all
;       to do with fractals.
;
;       (NOTE:  The video routines have been moved over to VIDEO.ASM)
;
; ---- Overall Support
;
;       initasmvars()
;
; ---- Quick-copy to/from Extraseg support
;
;       toextra()
;       fromextra()
;       cmpextra()
;
; ---- far memory allocation support
;
;       farmemalloc()
;       farmemfree()
;       erasesegment()
;
; ---- General Turbo-C (artificial) support
;
;       disable()
;       enable()
;
; ---- 32-bit Multiply/Divide Routines (includes 16-bit emulation)
;
;       multiply()
;       divide()
;
; ---- Keyboard, audio (and, hidden inside them, Mouse) support
;
;       keypressed()
;       getakey()
;       buzzer() ; renamed to buzzerpcspkr()
;       delay()
;       tone()
;       snd()
;       nosnd()
;
; ---- Expanded Memory Support
;
;       emmquery()
;       emmgetfree()
;       emmallocate()
;       emmdeallocate()
;       emmgetpage()
;       emmclearpage()
;
; ---- Extended Memory Support
;
;       xmmquery()
;       xmmlongest()
;       xmmallocate()
;       xmmreallocate()
;       xmmdeallocate()
;       xmmmoveextended()
;
; ---- CPU, FPU Detectors
;
;       cputype()
;       fputype()
;


;                        required for compatibility if Turbo ASM
IFDEF ??version
        MASM51
        QUIRKS
ENDIF

        .MODEL  medium,c

        .8086

        ; these must NOT be in any segment!!
        ; this get's rid of TURBO-C fixup errors

        extrn   help:far                ; help code (in help.c)
        extrn   tab_display:far         ; TAB display (in fractint.c)
      ; extrn   restore_active_ovly:far
        extrn   edit_text_colors:far
        extrn   adapter_init:far        ; video adapter init (in video.asm)
        extrn   slideshw:far
        extrn   recordshw:far
        extrn   stopslideshow:far
        extrn   mute:far
        extrn   scrub_orbit:far

.DATA

; ************************ External variables *****************************

        extrn   soundflag:word          ; if 0, supress sounds
        extrn   debugflag:word          ; for debugging purposes only
        extrn   helpmode:word           ; help mode (AUTHORS is special)
        extrn   tabmode:word            ; tab key enabled?
        extrn   sxdots:word             ; horizontal pixels
        extrn   timedsave:word          ; triggers autosave
        extrn   calc_status:word        ; in calcfrac.c
        extrn   got_status:word         ; in calcfrac.c
        extrn   currow:word             ; in calcfrac.c
        extrn   slides:word             ; in cmdfiles.c
        extrn   show_orbit:word         ; in calcfrac.c
        extrn   taborhelp:word          ; in fracsubr.c

; ************************ Public variables *****************************

public          cpu                     ; used by 'calcmand'
public          fpu                     ; will be used by somebody someday
public          lookatmouse             ; used by 'calcfrac'
public          saveticks               ; set by fractint
public          savebase                ; set by fractint
public          finishrow               ; set by fractint
public          _dataseg_xx             ; used by TARGA, other Turbo Code

public          extraseg                ; extra 64K segment

public          overflow                ; Mul, Div overflow flag: 0 means none

;               arrays declared here, used elsewhere
;               arrays not used simultaneously are deliberately overlapped

public          keybuffer                       ; needed for ungetakey
public          prefix, suffix, dstack, decoderline ; for the Decoder
public          tstack                          ; for the prompting routines
public          strlocn, block                  ; used by the Encoder
public          boxx, boxy, boxvalues           ; zoom-box arrays
public          olddacbox                       ; temporary DAC saves
public          rlebuf                          ; Used ty the TARGA En/Decoder
public          paldata, stbuff                 ; 8514A arrays, (FR8514A.ASM)

; ************************* "Shared" array areas **************************

; Shared near arrays are discussed below. First some comments about "extraseg".
; It is a 96k far area permanently allocated during fractint runup.
; The first part is used for:
;   64k coordinate arrays during image calculation (initialized in fracsubr.c)
;   64k create gif save file, encoder.c
;   64k 3d transforms, line3d.c
;   64k credits screen, intro.c
;   22k video mode selection (for fractint.cfg, loadfdos, miscovl)
;    2k .frm .ifs .l .par entry selection and file selection (prompts.c)
;   ??k printing, printer.c
;   ??k fractint.doc creation, help.c, not important cause it saves/restores
;   64K arbitrary precision - critical variables at top, safe from encoder
;   10K cmdfiles.c input buffer
;   4K  miscovl.c PAR file buffer
;
; The high 32k is used for graphics image save during text mode; video.asm
; and realdos.c.

; Short forms used in subsequent comments:
;   name........duration of use......................modules....................
;   encoder     "s"aving an image                    encoder.c
;   decoder     "r"estoring an image                 decoder.c, gifview.c
;   zoom        zoom box is visible                  zoom.c, video.asm
;   vidswitch   temp during video mode setting       video.asm
;   8514a       8514a is in use (graphics only?)     fr8514a.asm
;   tgaview     restore of tga image                 tgaview.c
;   solidguess  image gen with "g", not to disk      calcfrac.c
;   btm         image gen with "b", not to disk      calcfrac.c
;   editpal     palette editor "heap"                editpal.c
;   cellular    cellular fractal type generation     misfrac.c
;   browse      browsing                             loadfile.c
;   lsystem     lsystem fractal type generation      lsys.c, lsysf.c, lsysa.asm, lsysaf.asm
; Several arrays used in cmdfiles.c and miscovl.c, but are saved and
; restored to extraseg so not critical.
;
; Note that decoder using an 8514a is worst case, uses all arrays at once.
; Keep db lengths even so that word alignment is preserved for speed.

block           label   byte            ; encoder(4k), loadfile(4k),
suffix          dw      2048 dup(0)     ; decoder(4k), vidswitch(256),
                                        ; savegraphics/restoregraphics(4k)

tstack          label   byte            ; prompts(4k), ifsload(4k),
                                        ; make_batch(4k)
olddacbox       label   byte            ; fractint(768), prompts(768)
dstack          dw      2048 dup(0)     ; decoder(4k), solidguess(4k), btm(2k)
                                        ;   zoom(2k), printer(2400)
                                        ;   loadfdos(2000), cellular(2025)

strlocn         label   word            ; encoder(10k), editpal(10k)
prefix          label   word            ; decoder(8k), solidguess(6k)
boxx            dw      2048 dup(0)     ; zoom(4k), tgaview(4k), prompts(9k)
                                        ;   make_batch(8k), parser(8k)
                                        ;   browse(?)
boxy            dw      2048 dup(0)     ; zoom(4k), cellular(2025), browse(?)
                                        ;   lsystem(1000)
boxvalues       label   byte            ; zoom(2k), browse(?)
decoderline     db      2050 dup(0)     ; decoder(2049), btm(2k)

rlebuf          label   byte            ; f16.c(258) .tga save/restore?
paldata         db      1024 dup(0)     ; 8514a(1k)
stbuff          db      415 dup(0)      ; 8514a(415)

; ************************ Internal variables *****************************

                align   2
cpu             dw      0               ; cpu type: 86, 186, 286, 386, 486, 586...
fpu             dw      0               ; fpu type: 0, 87, 287, 387
_dataseg_xx     dw      0               ; our "near" data segment

overflow        dw      0               ; overflow flag

kbd_type        db      0               ; type of keyboard
                align   2
keybuffer       dw      0               ; real small keyboard buffer

delayloop       dw      32              ; delay loop value
delaycount      dw      0               ; number of delay "loops" per ms.

extraseg        dw      0               ; extra 64K segment (allocated by init)

; ********************** Mouse Support Variables **************************

lookatmouse     dw      0               ; see notes at mouseread routine
prevlamouse     dw      0               ; previous lookatmouse value
mousetime       dw      0               ; time of last mouseread call
mlbtimer        dw      0               ; time of left button 1st click
mrbtimer        dw      0               ; time of right button 1st click
mhtimer         dw      0               ; time of last horiz move
mvtimer         dw      0               ; time of last vert  move
mhmickeys       dw      0               ; pending horiz movement
mvmickeys       dw      0               ; pending vert  movement
mbstatus        db      0               ; status of mouse buttons
mouse           db      0               ; == -1 if/when a mouse is found.
mbclicks        db      0               ; had 1 click so far? &1 mlb, &2 mrb

                align   2
; timed save variables, handled by readmouse:
savechktime     dw      0               ; time of last autosave check
savebase        dw      2 dup(0)        ; base clock ticks
saveticks       dw      2 dup(0)        ; save after this many ticks
finishrow       dw      0               ; save when this row is finished


.CODE

; *************** Function toextra(tooffset,fromaddr, fromcount) *********

toextra proc    uses es di si, tooffset:word, fromaddr:word, fromcount:word
        cld                             ; move forward
        mov     ax,extraseg             ; load ES == extra segment
        mov     es,ax                   ;  ..
        mov     di,tooffset             ; load to here
        mov     si,fromaddr             ; load from here
        mov     cx,fromcount            ; this many bytes
        rep     movsb                   ; do it.
        ret                             ; we done.
toextra endp


; *************** Function fromextra(fromoffset, toaddr, tocount) *********

fromextra proc  uses es di si, fromoffset:word, toaddr:word, tocount:word
        push    ds                      ; save DS for a tad
        pop     es                      ; restore it to ES
        cld                             ; move forward
        mov     si,fromoffset           ; load from here
        mov     di,toaddr               ; load to here
        mov     cx,tocount              ; this many bytes
        mov     ax,extraseg             ; load DS == extra segment
        mov     ds,ax                   ;  ..
        rep     movsb                   ; do it.
        push    es                      ; save ES again.
        pop     ds                      ; restore DS
        ret                             ; we done.
fromextra endp


; *************** Function cmpextra(cmpoffset,cmpaddr, cmpcount) *********

cmpextra proc   uses es di si, cmpoffset:word, cmpaddr:word, cmpcount:word
        cld                             ; move forward
        mov     ax,extraseg             ; load ES == extra segment
        mov     es,ax                   ;  ..
        mov     di,cmpoffset            ; load to here
        mov     si,cmpaddr              ; load from here
        mov     cx,cmpcount             ; this many bytes
        rep     cmpsb                   ; do it.
        jnz     cmpbad                  ; failed.
        sub     ax,ax                   ; 0 == true
        jmp     short cmpend
cmpbad:
        mov     ax,1                    ; 1 == false
cmpend:
        ret                             ; we done.
cmpextra        endp


; =======================================================
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x,y,n)
;
;       requires the presence of an external variable, 'cpu'.
;               'cpu' >= 386 if a >=386 is present.

.8086

.DATA

temp    dw      5 dup(0)                ; temporary 64-bit result goes here
sign    db      0                       ; sign flag goes here

.CODE

FRAME   MACRO regs
        push    bp
        mov     bp, sp
        IRP     reg, <regs>
          push  reg
          ENDM
        ENDM

UNFRAME MACRO regs
        IRP     reg, <regs>
          pop reg
          ENDM
        pop bp
        ENDM


multiply        proc    x:dword, y:dword, n:word

        cmp     cpu,386                 ; go-fast time?
        jb      slowmultiply            ; no.  yawn...

.386                                    ; 386-specific code starts here

        mov     eax,x                   ; load X into EAX
        imul    y                       ; do the multiply
        mov     cx,n                    ; set up the shift
        cmp     cx,32                   ; ugly klooge:  check for 32-bit shift
        jb      short fastm1            ;  < 32 bits:  no problem
        mov     eax,edx                 ;  >= 32 bits:  manual shift
        mov     edx,0                   ;  ...
        sub     cx,32                   ;  ...
fastm1: shrd    eax,edx,cl              ; shift down 'n' bits
        js      fastm3
        sar     edx,cl
        jne     overmf
        shld    edx,eax,16
        ret
fastm3: sar     edx,cl
        inc     edx
        jne     overmf
        shld    edx,eax,16
        ret
overmf:
        mov     ax,0ffffh               ; overflow value
        mov     dx,07fffh               ; overflow value
        mov     overflow,1              ; flag overflow
        ret

.8086                                   ; 386-specific code ends here

slowmultiply:                           ; (sigh)  time to do it the hard way...
        push    di
        push    si
        push    es

        mov     ax,0
        mov     temp+4,ax               ; first, zero out the (temporary)
        mov     temp+6,ax               ;  result
        mov     temp+8,ax

        les     bx,x                    ; move X to SI:BX
        mov     si,es                   ;  ...
        les     cx,y                    ; move Y to DI:CX
        mov     di,es                   ;  ...

        mov     sign,0                  ; clear out the sign flag
        cmp     si,0                    ; is X negative?
        jge     mults1                  ;  nope
        not     sign                    ;  yup.  flip signs
        not     bx                      ;   ...
        not     si                      ;   ...
        stc                             ;   ...
        adc     bx,ax                   ;   ...
        adc     si,ax                   ;   ...
mults1: cmp     di,0                    ; is DI:CX negative?
        jge     mults2                  ;  nope
        not     sign                    ;  yup.  flip signs
        not     cx                      ;   ...
        not     di                      ;   ...
        stc                             ;   ...
        adc     cx,ax                   ;   ...
        adc     di,ax                   ;   ...
mults2:

        mov     ax,bx                   ; perform BX x CX
        mul     cx                      ;  ...
        mov     temp,ax                 ;  results in lowest 32 bits
        mov     temp+2,dx               ;  ...

        mov     ax,bx                   ; perform BX x DI
        mul     di                      ;  ...
        add     temp+2,ax               ;  results in middle 32 bits
        adc     temp+4,dx               ;  ...
        jnc     mults3                  ;  carry bit set?
        inc     word ptr temp+6         ;  yup.  overflow
mults3:

        mov     ax,si                   ; perform SI * CX
        mul     cx                      ;  ...
        add     temp+2,ax               ;  results in middle 32 bits
        adc     temp+4,dx               ;  ...
        jnc     mults4                  ;  carry bit set?
        inc     word ptr temp+6         ;  yup.  overflow
mults4:

        mov     ax,si                   ; perform SI * DI
        mul     di                      ;  ...
        add     temp+4,ax               ; results in highest 32 bits
        adc     temp+6,dx               ;  ...

        mov     cx,n                    ; set up for the shift loop
        cmp     cx,24                   ; shifting by three bytes or more?
        jl      multc1                  ;  nope.  check for something else
        sub     cx,24                   ; quick-shift 24 bits
        mov     ax,temp+3               ; load up the registers
        mov     dx,temp+5               ;  ...
        mov     si,temp+7               ;  ...
        mov     bx,0                    ;  ...
        jmp     short multc4            ; branch to common code
multc1: cmp     cx,16                   ; shifting by two bytes or more?
        jl      multc2                  ;  nope.  check for something else
        sub     cx,16                   ; quick-shift 16 bits
        mov     ax,temp+2               ; load up the registers
        mov     dx,temp+4               ;  ...
        mov     si,temp+6               ;  ...
        mov     bx,0                    ;  ...
        jmp     short multc4            ; branch to common code
multc2: cmp     cx,8                    ; shifting by one byte or more?
        jl      multc3                  ;  nope.  check for something else
        sub     cx,8                    ; quick-shift 8 bits
        mov     ax,temp+1               ; load up the registers
        mov     dx,temp+3               ;  ...
        mov     si,temp+5               ;  ...
        mov     bx,temp+7               ;  ...
        jmp     short multc4            ; branch to common code
multc3: mov     ax,temp                 ; load up the regs
        mov     dx,temp+2               ;  ...
        mov     si,temp+4               ;  ...
        mov     bx,temp+6               ;  ...
multc4: cmp     cx,0                    ; done shifting?
        je      multc5                  ;  yup.  bail out

multloop:
        shr     bx,1                    ; shift down 1 bit, cascading
        rcr     si,1                    ;  ...
        rcr     dx,1                    ;  ...
        rcr     ax,1                    ;  ...
        loop    multloop                ; try the next bit, if any
multc5:
        cmp     si,0                    ; overflow time?
        jne     overm1                  ; yup.  Bail out.
        cmp     bx,0                    ; overflow time?
        jne     overm1                  ; yup.  Bail out.
        cmp     dx,0                    ; overflow time?
        jl      overm1                  ; yup.  Bail out.

        cmp     sign,0                  ; should we negate the result?
        je      mults5                  ;  nope.
        not     ax                      ;  yup.  flip signs.
        not     dx                      ;   ...
        mov     bx,0                    ;   ...
        stc                             ;   ...
        adc     ax,bx                   ;   ...
        adc     dx,bx                   ;   ...
mults5:
        jmp     multiplyreturn

overm1:
        mov     ax,0ffffh               ; overflow value
        mov     dx,07fffh               ; overflow value
        mov     overflow,1              ; flag overflow

multiplyreturn:                         ; that's all, folks!
        pop     es
        pop     si
        pop     di
        ret
multiply        endp


; =======================================================
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, divide();
;       int n;
;
;       z = divide(x,y,n);      /* z = x / y; */
;
;       requires the presence of an external variable, 'cpu'.
;               'cpu' >= 386 if a >=386 is present.


.8086

divide          proc    uses di si es, x:dword, y:dword, n:word

        cmp     cpu,386                 ; go-fast time?
        jb      slowdivide              ; no.  yawn...

.386                                    ; 386-specific code starts here

        mov     edx,x                   ; load X into EDX (shifts to EDX:EAX)
        mov     ebx,y                   ; load Y into EBX

        mov     sign,0                  ; clear out the sign flag
        cmp     edx,0                   ; is X negative?
        jge     short divides1          ;  nope
        not     sign                    ;  yup.  flip signs
        neg     edx                     ;   ...
divides1:
        cmp     ebx,0                   ; is Y negative?
        jg      short divides2          ;  nope
        jl      short setsign1
        jmp     overd1          ; don't divide by zero, set overflow
setsign1:
        not     sign                    ;  yup.  flip signs
        neg     ebx                     ;   ...
divides2:

        mov     eax,0                   ; clear out the low-order bits
        mov     cx,32                   ; set up the shift
        sub     cx,n                    ; (for large shift counts - faster)
fastd1: cmp     cx,0                    ; done shifting?
        je      fastd2                  ; yup.
        shr     edx,1                   ; shift one bit
        rcr     eax,1                   ;  ...
        loop    fastd1                  ; and try again
fastd2:
        cmp     edx,ebx                 ; umm, will the divide blow out?
        jae     overd1                  ;  yup.  better skip it.
        div     ebx                     ; do the divide
        cmp     eax,0                   ; did the sign flip?
        jl      overd1                  ;  then we overflowed
        cmp     sign,0                  ; is the sign reversed?
        je      short divides3          ;  nope
        neg     eax                     ; flip the sign
divides3:
        push    eax                     ; save the 64-bit result
        pop     ax                      ; low-order  16 bits
        pop     dx                      ; high-order 16 bits
        jmp     dividereturn            ; back to common code

.8086                                   ; 386-specific code ends here

slowdivide:                             ; (sigh)  time to do it the hard way...

        les     ax,x                    ; move X to DX:AX
        mov     dx,es                   ;  ...

        mov     sign,0                  ; clear out the sign flag
        cmp     dx,0                    ; is X negative?
        jge     divides4                ;  nope
        not     sign                    ;  yup.  flip signs
        not     ax                      ;   ...
        not     dx                      ;   ...
        stc                             ;   ...
        adc     ax,0                    ;   ...
        adc     dx,0                    ;   ...
divides4:

        mov     cx,32                   ; get ready to shift the bits
        sub     cx,n                    ; (shift down rather than up)
        mov     byte ptr temp+4,cl      ;  ...

        mov     cx,0                    ;  clear out low bits of DX:AX:CX:BX
        mov     bx,0                    ;  ...

        cmp     byte ptr temp+4,16      ; >= 16 bits to shift?
        jl      dividex0                ;  nope
        mov     bx,cx                   ;  yup.  Take a short-cut
        mov     cx,ax                   ;   ...
        mov     ax,dx                   ;   ...
        mov     dx,0                    ;   ...
        sub     byte ptr temp+4,16      ;   ...
dividex0:
        cmp     byte ptr temp+4,8       ; >= 8 bits to shift?
        jl      dividex1                ;  nope
        mov     bl,bh                   ;  yup.  Take a short-cut
        mov     bh,cl                   ;   ...
        mov     cl,ch                   ;   ...
        mov     ch,al                   ;   ...
        mov     al,ah                   ;   ...
        mov     ah,dl                   ;   ...
        mov     dl,dh                   ;   ...
        mov     dh,0                    ;   ...
        sub     byte ptr temp+4,8       ;   ...
dividex1:
        cmp     byte ptr temp+4,0       ; are we done yet?
        je      dividex2                ;  yup
        shr     dx,1                    ; shift all 64 bits
        rcr     ax,1                    ;  ...
        rcr     cx,1                    ;  ...
        rcr     bx,1                    ;  ...
        dec     byte ptr temp+4         ; decrement the shift counter
        jmp     short dividex1          ;  and try again
dividex2:

        les     di,y                    ; move Y to SI:DI
        mov     si,es                   ;  ...

        cmp     si,0                    ; is Y negative?
        jg      divides5                ;  nope
        jl      short setsign2
        cmp     di,0                    ; high part is zero, check low part
        je      overd1                  ; don't divide by zero, set overflow
        jmp     short divides5
setsign2:
        not     sign                    ;  yup.  flip signs
        not     di                      ;   ...
        not     si                      ;   ...
        stc                             ;   ...
        adc     di,0                    ;   ...
        adc     si,0                    ;   ...
divides5:

        mov     byte ptr temp+4,33      ; main loop counter
        mov     temp,0                  ; results in temp
        mov     word ptr temp+2,0       ;  ...

dividel1:
        shl     temp,1                  ; shift the result up 1
        rcl     word ptr temp+2,1       ;  ...
        cmp     dx,si                   ; is DX:AX >= Y?
        jb      dividel3                ;  nope
        ja      dividel2                ;  yup
        cmp     ax,di                   ;  maybe
        jb      dividel3                ;  nope
dividel2:
        cmp     byte ptr temp+4,32      ; overflow city?
        jge     overd1                  ;  yup.
        sub     ax,di                   ; subtract Y
        sbb     dx,si                   ;  ...
        inc     temp                    ; add 1 to the result
        adc     word ptr temp+2,0       ;  ...
dividel3:
        shl     bx,1                    ; shift all 64 bits
        rcl     cx,1                    ;  ...
        rcl     ax,1                    ;  ...
        rcl     dx,1                    ;  ...
        dec     byte ptr temp+4         ; time to quit?
        jnz     dividel1                ;  nope.  try again.

        mov     ax,temp                 ; copy the result to DX:AX
        mov     dx,word ptr temp+2      ;  ...
        cmp     sign,0                  ; should we negate the result?
        je      divides6                ;  nope.
        not     ax                      ;  yup.  flip signs.
        not     dx                      ;   ...
        mov     bx,0                    ;   ...
        stc                             ;   ...
        adc     ax,0                    ;   ...
        adc     dx,0                    ;   ...
divides6:
        jmp     short dividereturn

overd1:
        mov     ax,0ffffh               ; overflow value
        mov     dx,07fffh               ; overflow value
        mov     overflow,1              ; flag overflow

dividereturn:                           ; that's all, folks!
        ret
divide          endp


; ****************** Function getakey() *****************************
; **************** Function keypressed() ****************************

;       'getakey()' gets a key from either a "normal" or an enhanced
;       keyboard.   Returns either the vanilla ASCII code for regular
;       keys, or 1000+(the scan code) for special keys (like F1, etc)
;       Use of this routine permits the Control-Up/Down arrow keys on
;       enhanced keyboards.
;
;       The concept for this routine was "borrowed" from the MSKermit
;       SCANCHEK utility
;
;       'keypressed()' returns a zero if no keypress is outstanding,
;       and the value that 'getakey()' will return if one is.  Note
;       that you must still call 'getakey()' to flush the character.
;       As a sidebar function, calls 'help()' if appropriate, or
;       'tab_display()' if appropriate.
;       Think of 'keypressed()' as a super-'kbhit()'.

keypressed  proc
        call    far ptr getkeynowait            ; check for key
        jc      keypressed1                     ;  got a key
        sub     ax,ax                           ; fast no-key return
        ret
keypressed1:
        FRAME   <di,si,es>                      ; std frame, for TC++ overlays
        mov     keybuffer,ax                    ; remember it for next time
        cmp     ax,1059                         ; help called?
        jne     keypressed2                     ; no help asked for.
        cmp     helpmode,0                      ; help enabled?
        jl      keypressedx                     ;  nope.
        mov     keybuffer,0                     ; say no key hit
        xor     ax,ax
        push    ax
        cmp     show_orbit,0
        je      noscrub1
        call    far ptr scrub_orbit             ; clean up the orbits
        mov     taborhelp,1
noscrub1:
        call    far ptr help                    ; help!
        pop     ax
;       call    far ptr restore_active_ovly     ; help might've clobbered ovly
        sub     ax,ax
        jmp     short keypressedx
keypressed2:
        cmp     ax,9                            ; TAB key hit?
        je      keypressed3                     ;  yup.  TAB display.
        cmp     ax,1148                         ; CTL_TAB key hit?
        je      keypressed3                     ;  yup.  TAB display.
        jmp     keypressedx                     ;  nope.  no TAB display.
keypressed3:
        cmp     tabmode,0                       ; tab enabled?
        je      keypressedx                     ;  nope
        mov     keybuffer,0                     ; say no key hit
        cmp     show_orbit,0
        je      noscrub2
        call    far ptr scrub_orbit             ; clean up the orbits
        mov     taborhelp,1
noscrub2:
        call    far ptr tab_display             ; show the TAB status
;       call    far ptr restore_active_ovly     ; tab might've clobbered ovly
        sub     ax,ax
keypressedx:
        UNFRAME <es,si,di>                      ; pop frame
        ret
keypressed      endp

getakeynohelp proc
gknhloop:
        call    far ptr getakey                 ; get keystroke
        cmp     ax,1059                         ; help key?
        je      gknhloop                        ;  ignore help, none available
        ret
getakeynohelp endp

getakey proc
        mov     bx,soundflag
        and     bx,7                            ; is the sound on?
        jz      getakeyloop                     ; ok, sound is off
        cmp     bx,1                            ; if = 1, just using buzzer
        je      getakeyloop
        call    far ptr mute                    ; turn off sound
getakeyloop:
        call    far ptr getkeynowait            ; check for keystroke
        jnc     getakeyloop                     ;  no key, loop till we get one
        ret
getakey endp

getkeynowait proc
        FRAME   <di,si,es>                      ; std frame, for TC++ overlays

; The following fixes the "midnight bug".  If two midnights are crossed
; before a call to a dos date call (such as clock()), the system date
; will be wrong and therefore the calculation time will be wrong.
        mov     ah, 2Ah    ; Get Date function
        int     21h        ; don't do anything with the results, just call it

getkeyn0:
        cmp     keybuffer,0                     ; got a key buffered?
        je      getkeynobuf                     ;  nope
        mov     ax,keybuffer                    ; key was buffered here
        mov     keybuffer,0                     ; clear buffer
        jmp     getkeyyup                       ; exit with the key
getkeynobuf:
        call    mouseread                       ; mouse activity or savetime?
        jc      getkeyn4                        ;  yup, ax holds the phoney key
        mov     ah,kbd_type                     ; get the keyboard type
        or      ah,1                            ; check if a key is ready
        int     16h                             ; now check a key
        jnz     gotkeyn                         ;  got one
        cmp     slides,1                        ; slideshow playback active?
        jne     getkeynope                      ;  nope, return no key
        call    far ptr slideshw                ; check next playback keystroke
        cmp     ax,0                            ; got one?
        jne     getkeyn5                        ;  yup, use it
getkeynope:
        clc                                     ; return no key
        UNFRAME <es,si,di>                      ; pop frame
        ret
gotkeyn:                                        ; got a real keyboard keystroke
        mov     ah,kbd_type                     ; get the keyboard type
        int     16h                             ; now get a key
        cmp     al,0e0h                         ; check: Enhanced Keyboard key?
        jne     short getkeyn1                  ; nope.  proceed
        cmp     ah,0                            ; part 2 of Enhanced Key check
        je      short getkeyn1                  ; failed.  normal key.
        mov     al,0                            ; Turn enhanced key "normal"
        jmp     short getkeyn2                  ; jump to common code
getkeyn1:
        cmp     ah,0e0h                         ; check again:  Enhanced Key?
        jne     short getkeyn2                  ;  nope.  proceed.
        mov     ah,al                           ; Turn Enhanced key "normal"
        mov     al,0                            ;  ...
getkeyn2:
        cmp     al,0                            ; Function Key?
        jne     short getkeyn3                  ;  nope.  proceed.
        mov     al,ah                           ; klooge into ASCII Key
        mov     ah,0                            ; clobber the scan code
        add     ax,1000                         ;  + 1000
        jmp     short getkeyn4                  ; go to common return
getkeyn3:
        mov     ah,0                            ; clobber the scan code
getkeyn4:                                       ; got real key (not playback)
        cmp     ax,9999                         ; savetime from mousread?
        je      getkeyn6                        ;  yup, do it and don't record
        cmp     slides,1                        ; slideshow playback active?
        jne     getkeyn5                        ;  nope
        cmp     ax,1bh                          ; escape?
        jne     getkeyn0                        ;  nope, ignore the key
        call    far ptr stopslideshow           ; terminate playback
        jmp     short getkeyn0                  ; go check for another key
getkeyn5:
        cmp     slides,2                        ; slideshow record mode?
        jne     getkeyn6                        ;  nope
        push    ax
        call    far ptr recordshw               ; record the key
        pop     ax
getkeyn6:
        cmp     debugflag,3000                  ; color play enabled?
        jne     getkeyyup                       ;  nope
        cmp     ax,'~'                          ; color play requested?
        jne     getkeyyup                       ;  nope
        call    far ptr edit_text_colors        ; play
;       call    far ptr restore_active_ovly     ; might've clobbered ovly
        jmp     getkeyn0                        ; done playing, back around
getkeyyup:
        stc                                     ; indicate we have a key
        UNFRAME <es,si,di>                      ; pop frame
        ret
getkeynowait endp

; ****************** Function buzzerpcspkr(int buzzertype) *******************
;
;       Sound a tone based on the value of the parameter
;
;       0 = normal completion of task
;       1 = interrupted task
;       2 = error contition

;       "buzzer()" codes:  strings of two-word pairs
;               (frequency in cycles/sec, delay in milliseconds)
;               frequency == 0 means no sound
;               delay     == 0 means end-of-tune
buzzer0         dw      1047,100        ; "normal" completion
                dw      1109,100
                dw      1175,100
                dw      0,0
buzzer1         dw      2093,100        ; "interrupted" completion
                dw      1976,100
                dw      1857,100
                dw      0,0
buzzer2         dw      40,500          ; "error" condition (razzberry)
                dw      0,0

; ***********************************************************************

buzzerpcspkr  proc    uses si, buzzertype:word
        test    soundflag,08h           ; is the speaker sound supressed?
        jz      buzzerreturn            ;  yup.  bail out.
        mov     si, offset buzzer0      ; normal completion frequency
        cmp     buzzertype,0            ; normal completion?
        je      buzzerdoit              ; do it
        mov     si,offset buzzer1       ; interrupted task frequency
        cmp     buzzertype,1            ; interrupted task?
        je      buzzerdoit              ; do it
        mov     si,offset buzzer2       ; error condition frequency
buzzerdoit:
        mov     ax,cs:0[si]             ; get the (next) frequency
        mov     bx,cs:2[si]             ; get the (next) delay
        add     si,4                    ; get ready for the next tone
        cmp     bx,0                    ; are we done?
        je      buzzerreturn            ;  yup.
        push    bx                      ; put delay time on the stack
        push    ax                      ; put tone value on the stack
        call    far ptr tone            ; do it
        pop     ax                      ; restore stack
        pop     bx                      ; restore stack
        jmp     short buzzerdoit        ; get the next tone
buzzerreturn:
        ret                             ; we done
buzzerpcspkr  endp

; ***************** Function delay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
;
; ************************************************************************

delayamillisecond       proc    near    ; internal delay-a-millisecond code
        mov     bx,delaycount           ; set up to burn another millisecond
delayamill1:
        mov     cx,delayloop            ; start up the counter
delayamill2:                            ;
        loop    delayamill2             ; burn up some time
        dec     bx                      ; have we burned up a millisecond?
        jnz     delayamill1             ;  nope.  try again.
        ret                             ; we done
delayamillisecond       endp

delay   proc    uses es, delaytime:word ; delay loop (arg in milliseconds)
        mov     ax,delaytime            ; get the number of milliseconds
        cmp     ax,0                    ; any delay time at all?
        je      delayreturn             ;  nope.
delayloop1:
        call    delayamillisecond       ; burn up a millisecond of time
        dec     ax                      ; have we burned up enough m-seconds?
        jnz     delayloop1              ;  nope.  try again.
delayreturn:
        ret                             ; we done.
delay   endp

; ************** Function tone(int frequency,int delaytime) **************
;
;       buzzes the speaker with this frequency for this amount of time
;
; ************************************************************************

tone    proc    uses es, tonefrequency:word, tonedelay:word
        mov     al,0b6h                 ; latch to channel 2
        out     43h,al                  ;  ...
        cmp     tonefrequency,12h       ; was there a frequency?
        jbe     tonebypass              ;  nope.  delay only
        mov     bx,tonefrequency        ; get the frequency value
        mov     ax,0                    ; ugly klooge: convert this to the
        mov     dx,12h                  ; divisor the 8253 wants to see
        div     bx                      ;  ...
        out     42h,al                  ; send the low value
        mov     al,ah                   ; then the high value
        out     42h,al                  ;  ...
        in      al,61h                  ; get the current 8255 bits
        or      al,3                    ; turn bits 0 and 1 on
        out     61h,al                  ;  ...
tonebypass:
        mov     ax,tonedelay            ; get the delay value
        push    ax                      ; set the parameter
        call    far ptr delay           ; and force a delay
        pop     ax                      ; restore the parameter

        in      al,61h                  ; get the current 8255 bits
        and     al,11111100b            ; turn bits 0 and 1 off
        out     61h,al

        ret                             ; we done
tone    endp

; ************** Function snd(int hertz) and nosnd() **************
;
;       turn the speaker on with this frequency (snd) or off (nosnd)
;
; *****************************************************************

snd     proc    hertz:word              ;Sound the speaker
        cmp     hertz, 20
        jl      hertzbad
        cmp     hertz, 5000
        jg      hertzbad
        mov     ax,0                    ;Convert hertz
        mov     dx, 12h                 ;for use by routine
        div     hertz
        mov     bx, ax
        mov     al,10110110b            ;Put magic number
        out     43h, al                 ;into timer2
        mov     ax, bx                  ;Pitch into AX
        out     42h, al                 ;LSB into timer2
        mov     al, ah                  ;MSB to AL then
        out     42h, al                 ;to timer2
        in      al, 61h                 ;read I/O port B into AL
        or      al,3                    ;turn on bits 0 and 1
        out     61h,al                  ;to turn on speaker
hertzbad:
        ret
snd             endp

nosnd           proc                            ;Turn off speaker
        in      al, 61h                 ;Read I/O port B into AL
        and     al, 11111100b           ;mask lower two bits
        out     61h, al                 ;to turn off speaker
        ret
nosnd   endp


; ****************** Function initasmvars() *****************************

initasmvars     proc    uses es si di

         cmp    cpu,0                   ; have we been called yet:
         je     initasmvarsgo           ;  nope.  proceed.
         jmp    initreturn              ;  yup.  no need to be here.

initasmvarsgo:
        mov     ax,ds                   ; save the data segment
        mov     _dataseg_xx,ax          ;  for the C code

        mov     overflow,0              ; indicate no overflows so far

        mov     dx,1                    ; ask for 96K of far space
        mov     ax,8000h                ;  ...
        push    dx                      ;  ...
        push    ax                      ;  ...
        call    far ptr farmemalloc     ; use the assembler routine to do it
        pop     ax                      ; restore the stack
        pop     ax                      ;  ...
        mov     extraseg,dx             ; save the results here.

        call    adapter_init            ; call the video adapter init

                                       ; first see if a mouse is installed

        push    es                      ; (no, first check to ensure that
        mov     ax,0                    ; int 33h doesn't point to 0:0)
        mov     es,ax                   ; ...
        mov     ax,es:0cch              ; ...
        pop     es                      ; ...
        cmp     ax,0                    ; does int 33h have a non-zero value?
        je      noint33                 ;  nope.  then there's no mouse.

         xor    ax,ax                  ; function for mouse check
         int    33h                    ; call mouse driver
noint33:
         mov    mouse,al               ; al holds info about mouse

                                       ; now get the information about the kbd
         push   es                     ; save ES for a tad
         mov    ax,40h                 ; reload ES with BIOS data seg
         mov    es,ax                  ;  ...
         mov    ah,es:96h              ; get the keyboard byte
         pop    es                     ; restore ES
         and    ah,10h                 ; isolate the Enhanced KBD bit
         mov    kbd_type,ah            ; and save it

        call    far ptr cputype         ; what kind of CPU do we have here?
        cmp     ax,0                    ; protected mode of some sort?
        jge     positive                ;  nope.  proceed.
        neg     ax                      ;  yup.  flip the sign.
positive:
        mov     cpu,ax                  ; save the cpu type.
itsa386:
        cmp     debugflag,8088          ; say, should we pretend it's an 8088?
        jne     nodebug                 ;  nope.
        mov     cpu,86                  ; yup.  use 16-bit emulation.
nodebug:
        call far ptr fputype            ; what kind of an FPU do we have?
        mov     fpu,ax                  ;  save the results

        push    es                      ; save ES for a tad
        mov     ax,0                    ; reset ES to BIOS data area
        mov     es,ax                   ;  ...
        mov     dx,es:46ch              ; obtain the current timer value
        cmp     cpu,386                 ; are we on a 386 or above?
        jb      delaystartuploop        ;  nope.  don't adjust anything
        mov     delayloop, 256          ;  yup.  slow down the timer loop
delaystartuploop:
        cmp     dx,es:46ch              ; has the timer value changed?
        je      delaystartuploop        ;  nope.  check again.
        mov     dx,es:46ch              ; obtain the current timer value again
        mov     ax,0                    ; clear the delay counter
        mov     delaycount,55           ; 55 millisecs = 1/18.2 secs
delaytestloop:
        call    delayamillisecond       ; burn up a (fake) millisecond
        inc     ax                      ; indicate another loop has passed
        cmp     dx,es:46ch              ; has the timer value changed?
        je      delaytestloop           ; nope.  burn up some more time.
        mov     delaycount,ax           ; save the results here
        pop     es                      ; restore ES again

initreturn:
         ret                           ; return to caller
initasmvars endp


; New (Apr '90) mouse code by Pieter Branderhorst follows.
; The variable lookatmouse controls it all.  Callers of keypressed and
; getakey should set lookatmouse to:
;      0  ignore the mouse entirely
;     <0  only test for left button click; if it occurs return fake key
;           number 0-lookatmouse
;      1  return enter key for left button, arrow keys for mouse movement,
;           mouse sensitivity is suitable for graphics cursor
;      2  same as 1 but sensitivity is suitable for text cursor
;      3  specials for zoombox, left/right double-clicks generate fake
;           keys, mouse movement generates a variety of fake keys
;           depending on state of buttons
; Mouse movement is accumulated & saved across calls.  Eg if mouse has been
; moved up-right quickly, the next few calls to getakey might return:
;      right,right,up,right,up
; Minor jiggling of the mouse generates no keystroke, and is forgotten (not
; accumulated with additional movement) if no additional movement in the
; same direction occurs within a short interval.
; Movements on angles near horiz/vert are treated as horiz/vert; the nearness
; tolerated varies depending on mode.
; Any movement not picked up by calling routine within a short time of mouse
; stopping is forgotten.  (This does not apply to button pushes in modes<3.)
; Mouseread would be more accurate if interrupt-driven, but with the usage
; in fractint (tight getakey loops while mouse active) there's no need.

; translate table for mouse movement -> fake keys
mousefkey dw   1077,1075,1080,1072  ; right,left,down,up     just movement
        dw        0,   0,1081,1073  ;            ,pgdn,pgup  + left button
        dw    1144,1142,1147,1146  ; kpad+,kpad-,cdel,cins  + rt   button
        dw    1117,1119,1118,1132  ; ctl-end,home,pgdn,pgup + mid/multi

DclickTime    equ 9   ; ticks within which 2nd click must occur
JitterTime    equ 6   ; idle ticks before turfing unreported mickeys
TextHSens     equ 22  ; horizontal sensitivity in text mode
TextVSens     equ 44  ; vertical sensitivity in text mode
GraphSens     equ 5   ; sensitivity in graphics mode; gets lower @ higher res
ZoomSens      equ 20  ; sensitivity for zoom box sizing/rotation
TextVHLimit   equ 6   ; treat angles < 1:6  as straight
GraphVHLimit  equ 14  ; treat angles < 1:14 as straight
ZoomVHLimit   equ 1   ; treat angles < 1:1  as straight
JitterMickeys equ 3   ; mickeys to ignore before noticing motion

mouseread proc near USES bx cx dx
        local   moveaxis:word

        ; check if it is time to do an autosave
        cmp     saveticks,0     ; autosave timer running?
        je      mouse0          ;  nope
        sub     ax,ax           ; reset ES to BIOS data area
        mov     es,ax           ;  see notes at mouse1 in similar code
tickread:
        mov     ax,es:046ch     ; obtain the current timer value
        cmp     ax,savechktime  ; a new clock tick since last check?
        je      mouse0          ;  nope, save a dozen opcodes or so
        mov     dx,es:046eh     ; high word of ticker
        cmp     ax,es:046ch     ; did a tick get counted just as we looked?
        jne     tickread        ; yep, reread both words to be safe
        mov     savechktime,ax
        sub     ax,savebase     ; calculate ticks since timer started
        sbb     dx,savebase+2
        jns     tickcompare
        add     ax,0b0h         ; wrapped past midnight, add a day
        adc     dx,018h
tickcompare:
        cmp     dx,saveticks+2  ; check if past autosave time
        jb      mouse0
        ja      ticksavetime
        cmp     ax,saveticks
        jb      mouse0
ticksavetime:                   ; it is time to do a save
        mov     ax,finishrow
        cmp     ax,-1           ; waiting for the end of a row before save?
        jne     tickcheckrow    ;  yup, go check row
        cmp     calc_status,1   ; safety check, calc active?
        jne     tickdosave      ;  nope, don't check type of calc
        cmp     got_status,0    ; 1pass or 2pass?
        je      ticknoterow     ;  yup
        cmp     got_status,1    ; solid guessing?
        jne     tickdosave      ;  not 1pass, 2pass, ssg, so save immediately
ticknoterow:
        mov     ax,currow       ; note the current row
        mov     finishrow,ax    ;  ...
        jmp     short mouse0    ; and keep working for now
tickcheckrow:
        cmp     ax,currow       ; started a new row since timer went off?
        je      mouse0          ;  nope, don't do the save yet
tickdosave:
        mov     timedsave,1     ; tell mainline what's up
        mov     ax,9999         ; a dummy key value, never gets used
        jmp     mouseret

mouse0: ; now the mouse stuff
        cmp     mouse,-1
        jne     mouseidle       ; no mouse, that was easy
        mov     ax,lookatmouse
        cmp     ax,prevlamouse
        je      mouse1

        ; lookatmouse changed, reset everything
        mov     prevlamouse,ax
        mov     mbclicks,0
        mov     mbstatus,0
        mov     mhmickeys,0
        mov     mvmickeys,0
        ; note: don't use int 33 func 0 nor 21 to reset, they're SLOW
        mov     ax,06h          ; reset button counts by reading them
        mov     bx,0
        int     33h
        mov     ax,06h
        mov     bx,1
        int     33h
        mov     ax,05h
        mov     bx,0
        int     33h
        mov     ax,0Bh          ; reset motion counters by reading
        int     33h
        mov     ax,lookatmouse

mouse1: or      ax,ax
        jz      mouseidle       ; check nothing when lookatmouse=0
        ; following code directly accesses bios tick counter; it would be
        ; better not to rely on addr (use int 1A instead) but old PCs don't
        ; have the required int, the addr is constant in bios to date, and
        ; fractint startup already counts on it, so:
        mov     ax,0            ; reset ES to BIOS data area
        mov     es,ax           ;  ...
        mov     dx,es:46ch      ; obtain the current timer value
        cmp     dx,mousetime
        ; if timer same as last call, skip int 33s:  reduces expense and gives
        ; caller a chance to read all pending stuff and paint something
        jne     mnewtick
        cmp     lookatmouse,0   ; interested in anything other than left button?
        jl      mouseidle       ; nope, done
        jmp     mouse5

mouseidle:
        clc                     ; tell caller no mouse activity this time
        ret

mnewtick: ; new tick, read buttons and motion
        mov     mousetime,dx    ; note current timer
        cmp     lookatmouse,3
        je      mouse2          ; skip button press if mode 3

        ; check press of left button
        mov     ax,05h          ; get button press info
        mov     bx,0            ; for left button
        int     33h
        or      bx,bx
        jnz     mleftb
        cmp     lookatmouse,0
        jl      mouseidle       ; exit if nothing but left button matters
        jmp     mouse3          ; not mode 3 so skip past button release stuff
mleftb: mov     ax,13
        cmp     lookatmouse,0
        jg      mouser          ; return fake key enter
        mov     ax,lookatmouse  ; return fake key 0-lookatmouse
        neg     ax
mouser: jmp     mouseret

mouse2: ; mode 3, check for double clicks
        mov     ax,06h          ; get button release info
        mov     bx,0            ; left button
        int     33h
        mov     dx,mousetime
        cmp     bx,1            ; left button released?
        jl      msnolb          ; go check timer if not
        jg      mslbgo          ; double click
        test    mbclicks,1      ; had a 1st click already?
        jnz     mslbgo          ; yup, double click
        mov     mlbtimer,dx     ; note time of 1st click
        or      mbclicks,1
        jmp     short mousrb
mslbgo: and     mbclicks,0ffh-1
        mov     ax,13           ; fake key enter
        jmp     mouseret
msnolb: sub     dx,mlbtimer     ; clear 1st click if its been too long
        cmp     dx,DclickTime
        jb      mousrb
        and     mbclicks,0ffh-1 ; forget 1st click if any
        ; next all the same code for right button
mousrb: mov     ax,06h          ; get button release info
        mov     bx,1            ; right button
        int     33h
        ; now much the same as for left
        mov     dx,mousetime
        cmp     bx,1
        jl      msnorb
        jg      msrbgo
        test    mbclicks,2
        jnz     msrbgo
        mov     mrbtimer,dx
        or      mbclicks,2
        jmp     short mouse3
msrbgo: and     mbclicks,0ffh-2
        mov     ax,1010         ; fake key ctl-enter
        jmp     mouseret
msnorb: sub     dx,mrbtimer
        cmp     dx,DclickTime
        jb      mouse3
        and     mbclicks,0ffh-2

        ; get buttons state, if any changed reset mickey counters
mouse3: mov     ax,03h          ; get button status
        int     33h
        and     bl,7            ; just the button bits
        cmp     bl,mbstatus     ; any changed?
        je      mouse4
        mov     mbstatus,bl     ; yup, reset stuff
        mov     mhmickeys,0
        mov     mvmickeys,0
        mov     ax,0Bh
        int     33h             ; reset driver's mickeys by reading them

        ; get motion counters, forget any jiggle
mouse4: mov     ax,0Bh          ; get motion counters
        int     33h
        mov     bx,mousetime    ; just to have it in a register
        cmp     cx,0            ; horiz motion?
        jne     moushm          ; yup, go accum it
        mov     ax,bx
        sub     ax,mhtimer
        cmp     ax,JitterTime   ; timeout since last horiz motion?
        jb      mousev
        mov     mhmickeys,0
        jmp     short mousev
moushm: mov     mhtimer,bx      ; note time of latest motion
        add     mhmickeys,cx
        ; same as above for vertical movement:
mousev: cmp     dx,0            ; vert motion?
        jne     mousvm
        mov     ax,bx
        sub     ax,mvtimer
        cmp     ax,JitterTime
        jb      mouse5
        mov     mvmickeys,0
        jmp     short mouse5
mousvm: mov     mvtimer,bx
        add     mvmickeys,dx

        ; pick the axis with largest pending movement
mouse5: mov     bx,mhmickeys
        or      bx,bx
        jns     mchkv
        neg     bx              ; make it +ve
mchkv:  mov     cx,mvmickeys
        or      cx,cx
        jns     mchkmx
        neg     cx
mchkmx: mov     moveaxis,0      ; flag that we're going horiz
        cmp     bx,cx           ; horiz>=vert?
        jge     mangle
        xchg    bx,cx           ; nope, use vert
        mov     moveaxis,1      ; flag that we're going vert

        ; if moving nearly horiz/vert, make it exactly horiz/vert
mangle: mov     ax,TextVHLimit
        cmp     lookatmouse,2   ; slow (text) mode?
        je      mangl2
        mov     ax,GraphVHLimit
        cmp     lookatmouse,3   ; special mode?
        jne     mangl2
        cmp     mbstatus,0      ; yup, any buttons down?
        je      mangl2
        mov     ax,ZoomVHLimit  ; yup, special zoom functions
mangl2: mul     cx              ; smaller axis * limit
        cmp     ax,bx
        ja      mchkmv          ; min*ratio <= max?
        cmp     moveaxis,0      ; yup, clear the smaller movement axis
        jne     mzeroh
        mov     mvmickeys,0
        jmp     short mchkmv
mzeroh: mov     mhmickeys,0

        ; pick sensitivity to use
mchkmv: cmp     lookatmouse,2   ; slow (text) mode?
        je      mchkmt
        mov     dx,ZoomSens+JitterMickeys
        cmp     lookatmouse,3   ; special mode?
        jne     mchkmg
        cmp     mbstatus,0      ; yup, any buttons down?
        jne     mchkm2          ; yup, use zoomsens
mchkmg: mov     dx,GraphSens
        mov     cx,sxdots       ; reduce sensitivity for higher res
mchkg2: cmp     cx,400          ; horiz dots >= 400?
        jl      mchkg3
        shr     cx,1            ; horiz/2
        shr     dx,1
        inc     dx              ; sensitivity/2+1
        jmp     short mchkg2
mchkg3: add     dx,JitterMickeys
        jmp     short mchkm2
mchkmt: mov     dx,TextVSens+JitterMickeys
        cmp     moveaxis,0
        jne     mchkm2
        mov     dx,TextHSens+JitterMickeys ; slower on X axis than Y

        ; is largest movement past threshold?
mchkm2: cmp     bx,dx
        jge     mmove
        jmp     mouseidle       ; no movement past threshold, return nothing

        ; set bx for right/left/down/up, and reduce the pending mickeys
mmove:  sub     dx,JitterMickeys
        cmp     moveaxis,0
        jne     mmovev
        cmp     mhmickeys,0
        jl      mmovh2
        sub     mhmickeys,dx    ; horiz, right
        mov     bx,0
        jmp     short mmoveb
mmovh2: add     mhmickeys,dx    ; horiz, left
        mov     bx,2
        jmp     short mmoveb
mmovev: cmp     mvmickeys,0
        jl      mmovv2
        sub     mvmickeys,dx    ; vert, down
        mov     bx,4
        jmp     short mmoveb
mmovv2: add     mvmickeys,dx    ; vert, up
        mov     bx,6

        ; modify bx if a button is being held down
mmoveb: cmp     lookatmouse,3
        jne     mmovek          ; only modify in mode 3
        cmp     mbstatus,1
        jne     mmovb2
        add     bx,8            ; modify by left button
        jmp     short mmovek
mmovb2: cmp     mbstatus,2
        jne     mmovb3
        add     bx,16           ; modify by rb
        jmp     short mmovek
mmovb3: cmp     mbstatus,0
        je      mmovek
        add     bx,24           ; modify by middle or multiple

        ; finally, get the fake key number
mmovek: mov     ax,mousefkey[bx]

mouseret:
        stc
        ret
mouseread endp


; long readticker() returns current bios ticker value

readticker proc uses es
        sub     ax,ax           ; reset ES to BIOS data area
        mov     es,ax           ;  see notes at mouse1 in similar code
tickread:
        mov     ax,es:046ch     ; obtain the current timer value
        mov     dx,es:046eh     ; high word of ticker
        cmp     ax,es:046ch     ; did a tick get counted just as we looked?
        jne     tickread        ; yep, reread both words to be safe
        ret
readticker endp


;===============================================================
;
; CPUTYPE.ASM : C-callable functions cputype() and ndptype() adapted
; by Lee Daniel Crocker from code appearing in the late PC Tech Journal,
; August 1987 and November 1987.  PC Tech Journal was a Ziff-Davis
; Publication.  Code herein is copyrighted and used with permission.
;
; The function cputype() returns an integer value based on what kind
; of CPU it found, as follows:
;
;       Value   CPU Type
;       =====   ========
;       86      8086, 8088, V20, or V30
;       186     80186 or 80188
;       286     80286
;       386     80386 or 80386sx
;       486     80486 or 80486sx
;       586     pentium
;       -286    80286 in protected mode
;       -386    80386 or 80386sx in protected or 32-bit address mode
;       -486    80486 or 80486sx in protected or 32-bit address mode
;       -586    pentium in protected or 32-bit address mode
;
; The function ndptype() returns an integer based on the type of NDP
; it found, as follows:
;
;       Value   NDP Type
;       =====   ========
;       0       No NDP found
;       87      8087
;       287     80287
;       387     80387
;       487     80487
;       587     80587
;
; No provisions are made for the Weitek FPA chips.  For the 80486 and above,
; if an FPU is present it is CPU + 1.
;
; Neither function takes any arguments or affects any external storage,
; so there should be no memory-model dependencies.

.286P
.code

cputype proc
        push    bp

        push    sp                      ; 86/186 will push SP-2;
        pop     ax                      ; 286/386 will push SP.
        cmp     ax, sp
        jz      not86                   ; If equal, SP was pushed
        mov     ax, 186
        mov     cl, 32                  ;   186 uses count mod 32 = 0;
        shl     ax, cl                  ;   86 shifts 32 so ax = 0
;        jnz     exit                    ; Non-zero: no shift, so 186
        jz      notexit
        jmp     exit                    ; Non-zero: no shift, so 186
notexit:
        mov     ax, 86                  ; Zero: shifted out all bits
        jmp     exit
not86:
        pushf                           ; Test 16 or 32 operand size:
        mov     ax, sp                  ;   Pushed 2 or 4 bytes of flags?
        popf
        inc     ax
        inc     ax
        cmp     ax, sp                  ;   Did pushf change SP by 2?
        jnz     is32bit                 ;   If not, then 4 bytes of flags
is16bit:
        sub     sp, 6                   ; Is it 286 or 386 in 16-bit mode?
        mov     bp, sp                  ; Allocate stack space for GDT pointer
        sgdt    fword ptr [bp]
        add     sp, 4                   ; Discard 2 words of GDT pointer
        pop     ax                      ; Get third word
        inc     ah                      ; 286 stores -1, 386 stores 0 or 1
        jnz     is32bit
is286:
        mov     ax, 286
        jmp     testprot                ; Check for protected mode
is32bit:
.386
        pushfd                          ; save extended flags
        mov     eax,040000h
        push    eax                     ; push 40000h onto stack
        popfd                           ; pop extended flags
        pushfd                          ; push extended flags
        pop     eax                     ; put in eax
        popfd                           ; clean the stack
        and     eax,040000h             ; is bit 18 set?
        jne     tst486                  ; yes, it's a 486
is386:
        mov     ax, 386
        jmp     short testprot          ; Check for protected mode
tst486:
        pushfd                          ; push flags to save them
        pushfd                          ; push flags to look at
        pop     eax                     ; get eflags
        mov     ebx, eax                ; save for later
        xor     eax, 200000h            ; toggle bit 21
        push    eax
        popfd                           ; load modified eflags to CPU
        pushfd                          ; push eflags to look at
        pop     eax                     ; get current eflags
        popfd                           ; restore original flags
        xor     eax, ebx                ; check if bit changed 
        jnz     is586                   ; changed, it's a Pentium
is486:
        mov     ax, 486
        jmp     short testprot          ; Check for protected mode
is586:
        push    ecx                ; CPUID changes eax to edx
        push    edx
        mov     eax, 1             ; get family info function
;                                  ; get the CPUID information
        db      0Fh, 0A2h          ;  into eax, ebx, ecx, edx
        and     eax, 0F00h         ; find family info
        shr     eax, 8             ; move to al
        pop     edx
        pop     ecx
        cmp     ax,4               ; 4 = 80486, some 486's have CPUID
        je      is486
        cmp     ax,5               ; 5 = pentium
        ja      is686
        mov     ax,586
        jmp     testprot
is686:
        cmp     ax,6               ; 6 = pentium pro
        ja      is786
        mov     ax,686
        jmp     testprot
is786:
        cmp     ax,7
        ja      is886
        mov     ax,786             ; 7 = ??????
        jmp     testprot
is886:
        mov     ax,886             ; 8 = ??????

.286P
testprot:
        smsw    cx                      ; Protected?  Machine status -> CX
        ror     cx,1                    ; Protection bit -> carry flag
        jnc     exit                    ; Real mode if no carry
        neg     ax                      ; Protected:  return neg value
exit:
        pop     bp
        ret
cputype endp

.data

control dw      0                       ; Temp storage for 8087 control
                                        ;   and status registers
.code

fputype proc
        push    bp

        fninit                          ; Defaults to 64-bit mantissa
        mov     byte ptr control+1, 0
        fnstcw  control                 ; Store control word over 0
;       dw      3ed9h                   ; (klooge to avoid the MASM \e switch)
;       dw      offset control          ; ((equates to the above 'fnstcw' cmd))
        mov     ah, byte ptr control+1  ; Test contents of byte written
        cmp     ah, 03h                 ; Test for 64-bit precision flags
        je      gotone                  ; Got one!  Now let's find which
        xor     ax, ax
        jmp     fexit             ; No NDP found
gotone:
        and     control, not 0080h      ; IEM = 0 (interrupts on)
        fldcw   control
        fdisi                           ; Disable ints; 287/387 will ignore
        fstcw   control
        test    control, 0080h
        jz      not87                   ; Got 287/387; keep testing
        mov     ax, 87
        jmp     short freset
not87:
        finit
        fld1
        fldz
        fdiv                            ; Divide 1/0 to create infinity
        fld     st
        fchs                            ; Push -infinity on stack
        fcompp                          ; Compare +-infinity
        fstsw   control
        mov     ax, control
        sahf
        jnz     got387                  ; 387 will compare correctly
        mov     ax, 287
        jmp     short freset
got387:                                 ; add 1 to cpu # if fpu and >= 386
        cmp     cpu, 386
        jg      got487
        mov     ax, 387
        jmp     short freset
got487:
        cmp     cpu, 486
        jg      got587
        mov     ax, 487
        jmp     short freset
got587:
        cmp     cpu, 586
        jg      got687
        mov     ax, 587
        jmp     short freset
got687:
        cmp     cpu, 686
        jg      got787
        mov     ax, 687
        jmp     short freset
got787:
        cmp     cpu, 786
        jg      got887
        mov     ax, 787
        jmp     short freset
got887:
        mov     ax, 887
freset:
        fninit                          ; in case tests have had strange
        finit                           ; side-effects, reset
fexit:
        pop     bp
        ret
fputype endp

; ************************* Far Segment RAM Support **************************
;
;
;       farptr = (char far *)farmemalloc(long bytestoalloc);
;       (void)farmemfree(farptr);
;
;       alternatives to Microsoft/TurboC routines
;
;
.8086

farmemalloc     proc    uses es, bytestoallocate:dword
        les     bx,bytestoallocate      ; get the # of bytes into DX:BX
        mov     dx,es                   ;  ...
        add     bx,15                   ; round up to next paragraph boundary
        adc     dx,0                    ;  ...
        shr     dx,1                    ; convert to paragraphs
        rcr     bx,1                    ;  ...
        shr     dx,1                    ;  ...
        rcr     bx,1                    ;  ...
        shr     dx,1                    ;  ...
        rcr     bx,1                    ;  ...
        shr     dx,1                    ;  ...
        rcr     bx,1                    ;  ...
        cmp     dx,0                    ; ensure that we don't want > 1MB
        jne     farmemallocfailed       ;  bail out if we do
        mov     ah,48h                  ; invoke DOS to allocate memory
        int     21h                     ;  ...
        jc      farmemallocfailed       ; bail out on failure
        mov     dx,ax                   ; set up DX:AX as far address
        mov     ax,0                    ;  ...
        jmp     short farmemallocreturn ; and return
farmemallocfailed:
        mov     ax,0                    ; (load up with a failed response)
        mov     dx,0                    ;  ...
farmemallocreturn:
        ret                             ; we done.
farmemalloc     endp

farmemfree      proc    uses es, farptr:dword
        les     ax,farptr               ; get the segment into ES
        mov     ah,49h                  ; invoke DOS to free the segment
        int     21h                     ;  ...
        ret
farmemfree      endp

erasesegment    proc    uses es di si, segaddress:word, segvalue:word
        mov     ax,segaddress           ; load up the segment address
        mov     es,ax                   ;  ...
        mov     di,0                    ; start at the beginning
        mov     ax,segvalue             ; use this value
        mov     cx,8000h                ; over the entire segment
        rep     stosw                   ; do it
        ret                             ; we done
erasesegment    endp


farread proc uses ds, handle:word, buf:dword, len:word
        mov     ah, 03Fh
        mov     bx, [handle]
        mov     cx, [len]
        lds     dx, [buf]
        int     21h
        jnc     farreaddone
        mov     ax, -1
farreaddone:
        ret
farread endp


farwrite proc uses ds, handle:word, buf:dword, len:word
        mov     ah, 040h
        mov     bx, [handle]
        mov     cx, [len]
        lds     dx, [buf]
        int     21h
        jnc     farwritedone
        mov     ax, -1
farwritedone:
        ret
farwrite endp


; Convert segment:offset to equiv pointer with minimum possible offset
normalize proc p: dword
;       mov     ax, [word ptr p]
;       mov     dx, [word ptr p+2]
        les     ax, p
        mov     dx, es
        mov     bx, ax
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        and     ax, 0Fh
        add     dx, bx
        ret
normalize endp


; *************** Far string/memory functions *********
;       far_strlen ( char far *);
;       far_strcpy ( char far *, char far *);
;       far_strcmp ( char far *, char far *);
;       far_stricmp( char far *, char far *);
;       far_strnicmp(char far *, char far *, int);
;       far_strcat ( char far *, char far *);
;       far_memset ( char far *, int,        long);
;       far_memcpy ( char far *, char far *, int);
;       far_memcmp ( char far *, char far *, int);
;       far_memicmp( char far *, char far *, int);

;       xxxfar_routines are called internally with:
;               ds:si pointing to the source
;               es:di pointing to the destination
;               cx    containing a byte count
;               al    contining  a character (set) value
;               (and they destroy registers willy-nilly)

xxxfar_memlen   proc    near    ; return string length - INCLUDING the 0
        mov     ax,0
        mov     cx,65535
        repne   scasb
        sub     cx,65535
        neg     cx
        ret
xxxfar_memlen   endp

xxxfar_memcmp   proc    near    ; compare two strings - length in CX
        mov     ax,0
        rep     cmpsb
        jz      wedone
        mov     ax,1
wedone: ret
xxxfar_memcmp   endp

xxxfar_memicmp  proc    near    ; compare two caseless strings - length in CX
        mov     ax,0
        cmp     cx,0
        je      wedone
        dec     si
        dec     di
loop1:  inc     si
        inc     di
        mov     al,es:[di]
        mov     ah,ds:[si]
        cmp     al,ah
        je      loop2
        cmp     al,'A'
        jb      lower1
        cmp     al,'Z'
        ja      lower1
        add     al,20h
lower1: cmp     ah,'A'
        jb      lower2
        cmp     ah,'Z'
        ja      lower2
        add     ah,20h
lower2: cmp     al,ah
        jne     uneql
loop2:  loop    loop1
        mov     ax,0
        jmp     short wedone
uneql:  mov     ax,1
wedone: ret
xxxfar_memicmp  endp


far_strlen proc uses ds es di si, fromaddr:dword
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        mov     ax,cx                   ; return len
        dec     ax                      ; don't count null
        ret                             ; we done.
far_strlen endp

far_strnicmp proc       uses ds es di si, toaddr:dword, fromaddr:dword, len:word
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        cmp     cx,len                  ; source less than or equal to len?
        jle     cxbigger                ; yup - use cx
        mov     cx,len                  ; nope - use len
cxbigger:
        les     di,toaddr               ; get the dest string
        lds     si,fromaddr             ; get the source string
        call    xxxfar_memicmp          ; compare them
        ret                             ; we done.
far_strnicmp endp

far_strcpy proc uses ds es di si, toaddr:dword, fromaddr:dword
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        les     di,toaddr               ; now move to here
        lds     si,fromaddr             ; from here
        rep     movsb                   ; move them
        ret                             ; we done.
far_strcpy endp

far_strcmp proc uses ds es di si, toaddr:dword, fromaddr:dword
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        les     di,toaddr               ; now compare to here
        lds     si,fromaddr             ; compare here
        call    xxxfar_memcmp           ; compare them
        ret                             ; we done.
far_strcmp endp

far_stricmp proc        uses ds es di si, toaddr:dword, fromaddr:dword
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        les     di,toaddr               ; get the dest string
        lds     si,fromaddr             ; get the source string
        call    xxxfar_memicmp          ; compare them
        ret                             ; we done.
far_stricmp endp

far_strcat proc uses ds es di si, toaddr:dword, fromaddr:dword
        les     di,fromaddr             ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        push    cx                      ; save it
        les     di,toaddr               ; point to start-of-string
        call    xxxfar_memlen           ; find the string length
        les     di,toaddr               ; now move to here
        add     di,cx                   ; but start at the end of string
        dec     di                      ; (less the EOS zero)
        lds     si,fromaddr             ; from here
        pop     cx                      ; get the string length
        rep     movsb                   ; move them
        ret                             ; we done.
far_strcat endp

far_memset proc uses es di, toaddr:dword, fromvalue:word, slength:word
        mov     ax,fromvalue            ; get the value to store
        mov     cx,slength              ; get the store length
        les     di,toaddr               ; now move to here
        rep     stosb                   ; store them
        ret                             ; we done.
far_memset endp

far_memcpy proc uses ds es di si, toaddr:dword, fromaddr:dword, slength:word
        mov     cx,slength              ; get the move length
        les     di,toaddr               ; now move to here
        lds     si,fromaddr             ; from here
        rep     movsb                   ; move them
        ret                             ; we done.
far_memcpy endp

far_memcmp proc uses ds es di si, toaddr:dword, fromaddr:dword, slength:word
        mov     cx,slength              ; get the compare length
        les     di,toaddr               ; now compare to here
        lds     si,fromaddr             ; compare here
        call    xxxfar_memcmp           ; compare them
        ret                             ; we done.
far_memcmp endp

far_memicmp proc uses ds es di si, toaddr:dword, fromaddr:dword, slength:word
        mov     cx,slength              ; get the compare length
        les     di,toaddr               ; get the dest string
        lds     si,fromaddr             ; get the source string
        call    xxxfar_memicmp          ; compare them
        ret                             ; we done.
far_memicmp endp

disable proc                            ; disable interrupts
        cli
        ret
disable endp

enable  proc                            ; re-enable interrupts
        sti
        ret
enable  endp

; *************** Expanded Memory Manager Support Routines ******************
;               for use with LIM 3.2 or 4.0 Expanded Memory
;
;       farptr = emmquery()     ; Query presence of EMM and initialize EMM code
;                               ; returns EMM FAR Address, or 0 if no EMM
;       freepages = emmgetfree(); Returns the number of pages (1 page = 16K)
;                               ; not already allocated for something else
;       handle = emmallocate(pages)     ; allocate EMM pages (1 page = 16K)
;                               ; returns handle # if OK, or else 0
;       emmdeallocate(handle)   ; return EMM pages to system - MUST BE CALLED
;                               ; or allocated EMM memory fills up
;       emmgetpage(page, handle); get an EMM page (actually, links the EMM
;                               ; page to the EMM Segment ADDR, saving any
;                               ; prior page in the process)
;       emmclearpage(page, handle) ; performs an 'emmgetpage()' and then clears
;                               ; it out (quickly) to zeroes with a 'REP STOSW'

.8086

.DATA

emm_name        db      'EMMXXXX0',0    ; device driver for EMM
emm_segment     dw      0               ; EMM page frame segment
emm_zeroflag    db      0               ; klooge flag for handle==0

.CODE

emmquery        proc
        mov     ah,3dh                  ; function 3dh = open file
        mov     al,0                    ;  read only
        mov     dx,offset emm_name      ; DS:DX = address of name of EMM
        int     21h                     ; open it
        jc      emmqueryfailed          ;  oops.  no EMM.

        mov     bx,ax                   ; BX = handle for EMM
        mov     ah,44h                  ; function 44h = IOCTL
        mov     al,7                    ; get outo. status
        mov     cx,0                    ; CX = # of bytes to read
        int     21h                     ; do it.
        push    ax                      ; save the IOCTL handle.

        mov     ah,3eh                  ; function 3eH = close
        int     21h                     ; BX still contains handle
        jc      emmqueryfailed          ; huh?  close FAILED?
        pop     ax                      ; restore AX for the status query

        or      al,al                   ; was the status 0?
        jz      emmqueryfailed          ; well then, it wasn't EMM!

        mov     ah,40h                  ; query EMM: hardware ok?
        int     67h                     ; EMM call
        cmp     ah,0                    ; is it ok?
        jne     emmqueryfailed          ; if not, fail

        mov     ah,41h                  ; query EMM: Get Page Frame Segment
        int     67h                     ; EMM call
        cmp     ah,0                    ; is it ok?
        jne     emmqueryfailed          ; if not, fail
        mov     emm_segment,bx          ; save page frame segment
        mov     dx,bx                   ; return page frame address
        mov     ax,0                    ;  ...
        jmp     short   emmqueryreturn  ; we done.

emmqueryfailed:
        mov     ax,0                    ; return 0 (no EMM found)
        mov     dx,0                    ;  ...
emmqueryreturn:
        ret                             ; we done.
emmquery        endp

emmgetfree      proc                    ; get # of free EMM pages
        mov     ah,42h                  ; EMM call: get total and free pages
        int     67h                     ; EMM call
        cmp     ah,0                    ; did we suceed?
        jne     emmgetfreefailed        ;  nope.  return 0 free pages
        mov     ax,bx                   ; else return # of free pages
        jmp     emmgetfreereturn        ; we done.
emmgetfreefailed:
        mov     ax,0                    ; failure mode
emmgetfreereturn:
        ret                             ; we done
emmgetfree      endp

emmallocate     proc    pages:word      ; allocate EMM pages
        mov     bx,pages                ; BX = # of 16K pages
        mov     ah,43h                  ; ask for the memory
        int     67h                     ; EMM call
;        mov     emm_zeroflag,0          ; clear the klooge flag
        cmp     ah,0                    ; did the call work?
        jne     emmallocatebad          ;  nope.
        mov     ax,dx                   ; yup.  save the handle here
        cmp     ax,0                    ; was the handle a zero?
        jne     emmallocatereturn       ;  yup.  no kloogy fixes
        mov     emm_zeroflag,1          ; oops.  set an internal flag
        mov     ax,1234                 ; and make up a dummy handle.
        jmp     short   emmallocatereturn ; and return
emmallocatebad:
        mov     ax,0                    ; indicate no handle
emmallocatereturn:
        ret                             ; we done.
emmallocate     endp

emmdeallocate   proc    emm_handle:word ; De-allocate EMM memory
emmdeallocatestart:
        mov     dx,emm_handle           ; get the EMM handle
        cmp     dx,1234                 ; was it our special klooge value?
        jne     emmdeallocatecontinue   ;  nope.  proceed.
        cmp     emm_zeroflag,1          ; was it really a zero handle?
        jne     emmdeallocatecontinue   ;  nope.  proceed.
        mov     dx,0                    ; yup.  use zero instead.
        mov     emm_zeroflag,0          ; clear the klooge flag
emmdeallocatecontinue:
        mov     ah,45h                  ; EMM function: deallocate
        int     67h                     ; EMM call
        cmp     ah,0                    ; did it work?
        jne     emmdeallocatestart      ; well then, try it again!
emmdeallocatereturn:
        ret                             ; we done
emmdeallocate   endp

emmgetpage      proc    pagenum:word, emm_handle:word   ; get EMM page
        mov     bx,pagenum              ; BX = page numper
        mov     dx,emm_handle           ; DX = EMM handle
        cmp     dx,1234                 ; was it our special klooge value?
        jne     emmgetpagecontinue      ;  nope.  proceed.
        cmp     emm_zeroflag,1          ; was it really a zero handle?
        jne     emmgetpagecontinue      ;  nope.  proceed.
        mov     dx,0                    ; yup.  use zero instead.
emmgetpagecontinue:
        mov     ah,44h                  ; EMM call: get page
        mov     al,0                    ; get it into page 0
        int     67h                     ; EMM call
        ret                             ; we done
emmgetpage      endp

emmclearpage    proc    pagenum:word, emm_handle:word   ; clear EMM page
        mov     bx,pagenum              ; BX = page numper
        mov     dx,emm_handle           ; DX = EMM handle
        cmp     dx,1234                 ; was it our special klooge value?
        jne     emmclearpagecontinue    ;  nope.  proceed.
        cmp     emm_zeroflag,1          ; was it really a zero handle?
        jne     emmclearpagecontinue    ;  nope.  proceed.
        mov     dx,0                    ; yup.  use zero instead.
emmclearpagecontinue:
        mov     ah,44h                  ; EMM call: get page
        mov     al,0                    ; get it into page 0
        int     67h                     ; EMM call
        mov     ax,emm_segment          ; get EMM segment into ES
        push    es                      ;  ...
        mov     es,ax                   ;  ...
        mov     di,0                    ; start at offset 0
        mov     cx,8192                 ; for 16K (in words)
        mov     ax,0                    ; clear out EMM segment to zeroes
        rep     stosw                   ; clear the page
        pop     es                      ; restore ES
        ret                             ; we done
emmclearpage    endp

; *************** Extended Memory Manager Support Routines ******************
;                 for use XMS 2.0 and later Extended Memory
;
;       xmmquery()              ; Query presence of XMM and initialize XMM code
;                               ; returns 0 if no XMM
;       xmmlongest()            ; return size of largest available
;                               ; XMM block in Kbytes (or zero if none)
;       xmmfree()               ; return amount of available
;                               ; XMM blocks in Kbytes (or zero if none)
;       handle = xmmallocate(Kbytes)    ; allocate XMM block in Kbytes
;                                       ; returns handle # if OK, or else 0
;       xmmreallocate(handle, Kbytes)   ; change size of handle's block
;                               ; to size of Kbytes.  Returns 0 if failed
;       xmmdeallocate(handle)   ; return XMM block to system - MUST BE CALLED
;                               ; or allocated XMM memory is not released.
;       xmmmoveextended(&MoveStruct)    ; Moves a block of memory to or
;                               ; from extended memory.  Returns 1 if OK
;                               ; else returns 0.
; The structure format for use with xmmoveextended is:
;
;       ASM                      |              C
;--------------------------------+----------------------------------------
; XMM_Move      struc            |      struct XMM_Move
;                                |      {
;   Length        dd ?           |          unsigned long   Length;
;   SourceHandle  dw ?           |          unsigned int    SourceHandle;
;   SourceOffset  dd ?           |          unsigned long   SourceOffset;
;   DestHandle    dw ?           |          unsigned int    DestHandle;
;   DestOffset    dd ?           |          unsigned long   DestOffset;
; XMM_Move      ends             |      };
;                                |
;
; Please refer to XMS spec version 2.0 for further information.

.data
xmscontrol      dd dword ptr (0) ; Address of driver's control function

.code
xmmquery        proc
        mov     ax,4300h                ; Is an XMS driver installed?
        int     2fh
        cmp     al, 80h                 ; Did it succeed?
        jne     xmmqueryfailed          ; No
        mov     ax,4310h                ; Get control function address
        int     2fh
        mov     word ptr [xmscontrol], bx   ; Put address in xmscontrol
        mov     word ptr [xmscontrol+2], es ; ...
        mov     ah,00h                  ; Get version number
        call [xmscontrol]
        cmp     ax,0200h                ; Is 2.00 or higher?
        jge  xmmquerydone               ; Yes
xmmqueryfailed:
        mov     ax,0                    ; return failure
        mov     dx,0
xmmquerydone:
        ret
xmmquery        endp


xmmlongest      proc                    ; query length of largest avail block
        mov     ah, 08h                 ;
        call    [xmscontrol]
        mov     dx, 0
        ret
xmmlongest      endp

xmmfree      proc                       ; query total avail extended memory
        mov     ah, 08h                 ;
        call    [xmscontrol]
        mov     ax, dx
        mov     dx, 0
        ret
xmmfree      endp

xmmallocate     proc    ksize:word
        mov     ah,09h                  ; Allocate extended memory block
        mov     dx,ksize                ; size of block in Kbytes
        call    [xmscontrol]
        cmp     ax,0001h                ; did it succeed?
        jne     xmmallocatefail         ; nope
        mov     ax,dx                   ; Put handle here
        jmp     short xmmallocatedone
xmmallocatefail:
        mov     ax, 0                   ; Indicate failure;
xmmallocatedone:
        ret
xmmallocate     endp


xmmreallocate   proc    handle:word, newsize:word
        mov     ah, 0Fh                 ; Change size of extended mem block
        mov     dx, handle              ; handle of block to reallocate
        mov     bx, newsize             ; new size for block
        call    [xmscontrol]
        cmp     ax, 0001h               ; one indicates success
        je      xmmreallocdone
        mov     ax, 0                   ; we return zero for failure
xmmreallocdone:
        ret
xmmreallocate   endp


xmmdeallocate   proc    xmm_handle:word
        mov     ah,0ah                  ; Deallocate extended memory block
        mov     dx, xmm_handle          ; Give it handle
        call    [xmscontrol]
        ret
xmmdeallocate   endp

xmmmoveextended proc uses si, MoveStruct:word

        ; Call the XMS MoveExtended function.
        mov     ah,0Bh
        mov     si,MoveStruct                   ; the move structure.
        call    [xmscontrol]                    ;

; The call to xmscontrol returns a 1 in AX if successful, 0 otherwise.

        ret

xmmmoveextended endp

	END
