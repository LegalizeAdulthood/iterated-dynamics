        TITLE   Hercules Graphics Routines for FRACTINT

;                        required for compatibility if Turbo ASM
IFDEF ??version
        MASM51
        QUIRKS
ENDIF

        .MODEL  medium,c

        .8086
.data

HGCBase         equ     0B000h          ;segment for HGC regen buffer page 0

herc_index      equ     03B4h
herc_cntrl      equ     03B8h
herc_status     equ     03BAh
herc_config     equ     03BFh


; Hercules control/configuration register settings

scrn_on equ     08h
grph    equ     02h
text    equ     20h
enable  equ     03h

.code

;
;
; inithgc -  Initialize the HGC in graphics mode.
;           Code mostly from the Hercules Graphics Card Owner's Manual.
;
inithgc PROC USES DI SI

        mov     al,enable               ; enable mode changes
        mov     dx,herc_config          ; same as HGC FULL command
        out     dx,al

        mov     al,grph                 ; set graphic mode
        lea     si,gtable               ; address of graphic parameters
        mov     bx,0
        mov     cx,4000h
        call    setmd                   ; call set mode common processing

        ret

inithgc ENDP

;
; termhgc -  Restore the Hercules Graphics Card to text mode.
;           Code mostly from the Hercules Graphics Card Owner's Manual.
;
termhgc PROC USES DI SI

        mov     al,text                 ; set text mode
        lea     si,ttable               ; get address of text parameters
        mov     bx,720h
        mov     cx,2000
        call    setmd

        ret

termhgc ENDP

;
; setmd - sets mode to graphic or text depending on al
;         si = address of parameter table
;         cx = number of words to be cleared
;         bx = blank value
;
;         from Hercules Graphics Card Owner's Manual
;
setmd   PROC NEAR
;
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx

;    change mode, but without screen on
        mov     dx,herc_cntrl           ; get address of control register
        out     dx,al                   ; al has the mode byte

;    initialize the 6845
        mov     ax,ds
        mov     es,ax                   ; also point es:si to parameter table

        mov     dx,herc_index           ; get index register address
        mov     cx,12                   ; 12 parameters to be output
        xor     ah,ah                   ; starting from register 0

parms:  mov     al,ah                   ; first output register number
        out     dx,al

        inc     dx                      ; get data register address
        lodsb                           ; get next byte from param. table
        out     dx,al                   ; output parameter data

        inc     ah                      ; increment register number
        dec     dx                      ; restore index register address
        loop    parms                   ; go do another one

;    now go clear the buffer
        pop     cx                      ; get number of words to clear
        mov     ax,HGCBase              ; get address off video buffer
        cld                             ; set auto increment
        mov     es,ax                   ; set segment for string move
        xor     di,di                   ; start at offset 0
        pop     ax                      ; get blank value
        rep     stosw                   ; repeat store string

;   turn screen on with page 0 active
        mov     dx,herc_cntrl           ; get control register address
        pop     ax                      ; get the mode byte
        add     al,scrn_on              ; set the screen-on bit
        out     dx,al

        mov     sp,bp
        pop     bp
        ret

setmd   ENDP
;
; writehgc (x, y, c)  - write a dot at x, y in color color
;  x = x coordinate
;  y = y coordinate
;  color = color
;
writehgc        PROC USES DI SI, x, y, color

        cmp     y,348                   ; Clip for hardware boundaries
        jge     WtDot030
        cmp     x,720
        jge     WtDot030

        lea     bx,HGCRegen             ;set up offset of regen scan line table
        mov     ax,HGCBase              ;segment for regen buffer
        mov     es,ax

; calculate byte address of dot in regen buffer
        mov     si,y            ;get y coordinate
        shl     si,1            ;mult by 2 to get offset in Scan Line Table
        mov     si,[bx][si]     ;get address of start of scan line from table
        mov     ax,x            ;get x coordinate
        mov     cl,3
        shr     ax,cl           ;divide by 8 to get byte offset
        add     si,ax           ;es:si has address of byte with the bit
; build the bit mask for the specific dot in the byte
        mov     cx,x            ;get x coordinate
        and     cx,0007h        ;get bit number within the byte
        mov     al,80h          ;prepare bit mask
        shr     al,cl           ;al has bit mask
; either turn on the bit or turn it off, depending on the color
        cmp     word ptr color,0        ;turn off bit?
        je      WtDot020        ;yes -- branch
; turn on the bit
        or      byte ptr es:[si],al
        jmp     short WtDot030
WtDot020:
; turn off the bit
        xor     al,0FFh
        and     byte ptr es:[si],al
WtDot030:
        ret

writehgc        ENDP
;
; readhgc (x,y) - read a dot at x,y
;  x = x coordinate
;  y = y coordinate
;
;  dot value is to be returned in AX
;
readhgc PROC USES DI SI, x,y

        lea     bx,HGCRegen             ;set up offset of regen scan line table
        mov     ax,HGCBase              ;segment for regen buffer
        mov     es,ax

; calculate byte address of dot in regen buffer
        mov     si,y            ;get y coordinate
        shl     si,1            ;mult by 2 to get offset in Scan Line Table
        mov     si,[bx][si]     ;get address of start of scan line from table
        mov     ax,x            ;get x coordinate
        mov     cl,3
        shr     ax,cl           ;divide by 8 to get byte offset
        add     si,ax           ;es:si has address of byte with the bit

; build the bit mask for the specific dot in the byte
        mov     cx,x            ;get x coordinate
        and     cx,0007h        ;get bit number within the byte
        mov     al,80h          ;prepare bit mask
        shr     al,cl           ;al has bit mask

; pick up the bit from the regen buffer
        test    byte ptr es:[si],al
        jz      readhgc020      ;branch if bit is zero
        mov     ax,1            ;else return foreground bit value
        jmp     short readhgc030
readhgc020:
        xor     ax,ax           ;bit is zero
readhgc030:
        ret

readhgc ENDP


.data

gtable  db      35h,2dh,2eh,07h
        db      5bh,02h,57h,57h
        db      02h,03h,00h,00h

ttable  db      61h,50h,52h,0fh
        db      19h,06h,19h,19h
        db      02h,0dh,0bh,0ch

                ;offsets into HGC regen buffer for each scan line
HGCRegen        dw      0,8192,16384,24576,90,8282,16474,24666
                dw      180,8372,16564,24756,270,8462,16654,24846
                dw      360,8552,16744,24936,450,8642,16834,25026
                dw      540,8732,16924,25116,630,8822,17014,25206
                dw      720,8912,17104,25296,810,9002,17194,25386
                dw      900,9092,17284,25476,990,9182,17374,25566
                dw      1080,9272,17464,25656,1170,9362,17554,25746
                dw      1260,9452,17644,25836,1350,9542,17734,25926
                dw      1440,9632,17824,26016,1530,9722,17914,26106
                dw      1620,9812,18004,26196,1710,9902,18094,26286
                dw      1800,9992,18184,26376,1890,10082,18274,26466
                dw      1980,10172,18364,26556,2070,10262,18454,26646
                dw      2160,10352,18544,26736,2250,10442,18634,26826
                dw      2340,10532,18724,26916,2430,10622,18814,27006
                dw      2520,10712,18904,27096,2610,10802,18994,27186
                dw      2700,10892,19084,27276,2790,10982,19174,27366
                dw      2880,11072,19264,27456,2970,11162,19354,27546
                dw      3060,11252,19444,27636,3150,11342,19534,27726
                dw      3240,11432,19624,27816,3330,11522,19714,27906
                dw      3420,11612,19804,27996,3510,11702,19894,28086
                dw      3600,11792,19984,28176,3690,11882,20074,28266
                dw      3780,11972,20164,28356,3870,12062,20254,28446
                dw      3960,12152,20344,28536,4050,12242,20434,28626
                dw      4140,12332,20524,28716,4230,12422,20614,28806
                dw      4320,12512,20704,28896,4410,12602,20794,28986
                dw      4500,12692,20884,29076,4590,12782,20974,29166
                dw      4680,12872,21064,29256,4770,12962,21154,29346
                dw      4860,13052,21244,29436,4950,13142,21334,29526
                dw      5040,13232,21424,29616,5130,13322,21514,29706
                dw      5220,13412,21604,29796,5310,13502,21694,29886
                dw      5400,13592,21784,29976,5490,13682,21874,30066
                dw      5580,13772,21964,30156,5670,13862,22054,30246
                dw      5760,13952,22144,30336,5850,14042,22234,30426
                dw      5940,14132,22324,30516,6030,14222,22414,30606
                dw      6120,14312,22504,30696,6210,14402,22594,30786
                dw      6300,14492,22684,30876,6390,14582,22774,30966
                dw      6480,14672,22864,31056,6570,14762,22954,31146
                dw      6660,14852,23044,31236,6750,14942,23134,31326
                dw      6840,15032,23224,31416,6930,15122,23314,31506
                dw      7020,15212,23404,31596,7110,15302,23494,31686
                dw      7200,15392,23584,31776,7290,15482,23674,31866
                dw      7380,15572,23764,31956,7470,15662,23854,32046
                dw      7560,15752,23944,32136,7650,15842,24034,32226
                dw      7740,15932,24124,32316

        END
