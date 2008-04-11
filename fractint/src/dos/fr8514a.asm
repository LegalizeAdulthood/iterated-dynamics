

IFDEF ??version
        MASM51
        QUIRKS
ENDIF

        .MODEL  medium,c

        .8086



HOPEN   equ     8
HSMX    equ     9
HINT    equ     16
HLDPAL  equ     19
HBBW    equ     21
HBBR    equ     23
HBBCHN  equ     24
HBBC    equ     25
HQMODE  equ     29
HRECT   equ     32
HCLOSE  equ     34
HINIT   equ     48
HSYNC   equ     49
HSPAL   equ     57
HRPAL   equ     58


HLINE   equ     0
HSCOL   equ     7


.DATA

        extrn   sxdots:word, sydots:word  ; number of dots across and down
        extrn   dacbox:byte, daccount:word

afiptr          dd      0

xadj            dw      0
yadj            dw      0

extrn           paldata:byte            ; 1024-byte array (in GENERAL.ASM)

extrn           stbuff:byte             ; 415-byte array (in GENERAL.ASM)

linedata        db      0

hopendata       db      3, 0, 0, 0, 0
hclosedata      dw      2, 0
hinitdata       dw      2, 0
bbw             dw      10, 8, 0, 1, 0, 0
bbr             dw      12, 8, 0, 1, 0, 0, 0
smx             dw      2, 0
chn             dw      6
                dd      linedata
                dw      1
pal             dw      10, 0, 0, 256
                dd      paldata
hidata          dw      4, 0, 8000h
amode           dw      18, 9 dup(?)


hlinedata       dw      8, 0, 0, 0, 0
hscoldata       dw      4, 0, 0


.CODE


callafi proc    near

        push    ds              ; Pass the parameter pointer
        push    si

        shl     ax,1            ; form offset from entry no. required
        shl     ax,1
        mov     si,ax

        les     bx, afiptr      ; entry block address to es:bx
        call    dword ptr es:[bx][si]    ; call entry point

        ret                     ; return to caller

callafi endp


getafi  proc    near

        mov     ax,357fh        ; read interrupt vector 7f
        int     21h
        mov     ax,es
        or      ax,bx           ; is 7f vector null
        stc
        jz      getafiret

        mov     ax,0105h        ; get Interface address
        int     7fh             ; by software interrupt 7f

        jc      getafiret               ; Interface not OK if carry set

        mov     word ptr afiptr,dx      ; save afi pointer offset
        mov     word ptr afiptr+2,cx    ; save afi pointer segment

        clc                     ; clear carry flag

getafiret:
        ret                     ; return to caller

getafi endp


do85open proc   near

        push    ax
        mov     ax, HOPEN
        call    callafi

        mov     ax, offset stbuff       ;get the state segment
        add     ax, 15
        mov     cl, 4
        shr     ax, cl

        mov     bx, ds
        add     ax, bx

        mov     si, offset hinitdata
        mov     [si] + 2, ax

        pop     ax
        call    callafi

        clc
        ret

do85open        endp


open8514        proc    far

        call    load8514dacbox  ; load dacbox for 8514/A setup  JCO 4/6/92

        call    getafi          ;get adapter interface
        jc      afinotfound

        mov     bl, 0           ;if > 640 x 480 then 1024 x 768

        mov     ax, sxdots

        cmp     ax, 1024        ;if > 1024, don't use afi, JCO 4/4/92
        ja      afinotfound

        cmp     ax, 800 ; must be 1024
        ja      setupopen

        cmp     ax, 640 ; could be 800
        ja      afinotfound

        mov     ax, sydots
        cmp     ax, 480
        ja      setupopen

        inc     bl

setupopen:

        mov     si, offset hopendata    ;open the adapter
        mov     byte ptr [si + 2], 40h          ;zero the image but leave pallette
        mov     [si + 3], bl
        mov     ax, HINIT               ;initialize state

        call    do85open
        jc      afinotfound

        mov     si, offset amode        ;make sure on the size
        mov     ax, HQMODE              ;get the adapter mode
        call    callafi

        mov     ax, amode + 10          ;get the screen width
        cmp     ax, sxdots
        jae     xdotsok                 ;check for fit
        mov     sxdots, ax
xdotsok:
        sub     ax, sxdots              ;save centering factor
        shr     ax, 1
        mov     xadj, ax

        mov     ax, amode + 12          ;get the screen height
        cmp     ax, sydots
        jae     ydotsok
        mov     sydots, ax
ydotsok:
        sub     ax, sydots
        shr     ax, 1
        mov     yadj, ax
        clc
        ret

afinotfound:                            ; No 8514/A interface found
        stc                             ; flag bad mode
        ret                             ;  and bail out

open8514        endp

reopen8514      proc    far

        mov     si, offset hopendata    ;open the adapter
        mov     byte ptr [si + 2], 0C0h         ;zero the image but leave pallette
        mov     ax, HSYNC               ;initialize state
        call    do85open
        ret

reopen8514      endp


close8514       proc    far

        mov     si, offset hclosedata           ;turn off 8514a
        mov     ax, HCLOSE
        call    callafi

        ret

close8514       endp


fr85wdotnew     proc    near    uses si

        mov     byte ptr [hscoldata + 2], al

        add     cx, xadj
        add     dx, yadj

        mov     hlinedata + 2, cx
        mov     hlinedata + 4, dx
        inc     cx              ; increment x direction
        mov     hlinedata + 6, cx
        mov     hlinedata + 8, dx

; set the color
        mov     si, offset hscoldata
        mov     ax, HSCOL
        call    callafi

; plot the point
        mov     si, offset hlinedata

        mov     ax, HLINE
        call    callafi

        ret

fr85wdotnew     endp


fr85wdot        proc    far uses si

        mov     linedata, al

        mov     bbw + 4, 1              ;define the rectangle
;       mov     bbw + 6, 1
        add     cx, xadj
        add     dx, yadj

        mov     bbw + 8, cx
        mov     bbw + 10, dx
        mov     si, offset bbw
        mov     ax, HBBW
        call    callafi

        mov     si, offset chn
        mov     word ptr [si + 2], offset linedata
        mov     word ptr [si + 6], 1    ;send the data

        mov     ax, HBBCHN
        call    callafi

fr85wdotx:
        ret

fr85wdot        endp


fr85wbox        proc    far uses si

        sub     ax, cx
        inc     ax                      ; BDT patch 11/4/90
;       add     ax, xadj
        add     cx, xadj
        add     dx, yadj
        mov     chn + 2, si             ;point to data
        mov     chn + 6, ax
        mov     bbw + 4, ax             ;define the rectangle
;       mov     bbw + 6, 1              ;set in declaration
        mov     bbw + 8, cx
        mov     bbw + 10, dx

        mov     si, offset bbw
        mov     ax, HBBW
        call    callafi

        mov     si, offset chn
        mov     ax, HBBCHN
        call    callafi

        ret

fr85wbox        endp


fr85rdot        proc    far uses si

        mov     bbr + 4, 1              ;define the rectangle
;       mov     bbr + 6, 1              ;set in declaration
        add     cx, xadj
        add     dx, yadj
        mov     bbr + 10, cx
        mov     bbr + 12, dx
        mov     si, offset bbr
        mov     ax, HBBR
        call    callafi

        mov     si, offset chn
        mov     word ptr [si + 2], offset linedata
        mov     word ptr [si + 6], 1    ;send the data
        mov     ax, HBBCHN
        call    callafi

        mov     al, linedata

fr85rdotx:
        ret

fr85rdot        endp

fr85rbox        proc    far uses si

        sub     ax, cx
        inc     ax                      ; BDT patch 11/4/90
;       add     ax, xadj
        add     cx, xadj
        add     dx, yadj
        mov     chn + 2, di             ;point to data
        mov     chn + 6, ax
        mov     bbr + 4, ax             ;define the rectangle
;       mov     bbr + 6, 1              ;set in declaration
        mov     bbr + 10, cx
        mov     bbr + 12, dx

        mov     si, offset bbr
        mov     ax, HBBR
        call    callafi

        mov     si, offset chn
        mov     ax, HBBCHN
        call    callafi

        ret

fr85rbox        endp


w8514pal        proc    far

        mov     si, offset dacbox

        mov     cx, daccount    ;limit daccount to 128 to avoid fliker
        cmp     cx, 128
        jbe     countok

        mov     cx, 128
        mov     daccount, cx

countok:                                ;now build 8514 pallette
        mov     ax, 256                 ;from the data in dacbox
        mov     pal + 4, 0
        mov     di, offset paldata
        cld
cpallp:
        push    ax                      ;do daccount at a time
        mov     dx, di
        cmp     ax, cx
        jae     dopass
        mov     cx, ax
dopass:
        mov     pal + 6, cx             ;entries this time
        push    cx
cpallp2:
        push    ds                      ;pallette format is r, b, g
        pop     es                      ;0 - 255 each

        lodsb                           ;red
        shl     al, 1
        shl     al, 1
        stosb
        lodsb                           ;green
        shl     al, 1
        shl     al, 1
        xchg    ah, al
        lodsb                           ;blue
        shl     al, 1
        shl     al, 1
        stosw
        mov     al, 0                   ;filler
        stosb
        loop    cpallp2

        push    si
        push    di
        push    dx

        mov     si, hidata              ;wait for flyback
        mov     ax, HINT
        call    callafi

        pop     dx
        mov     pal + 8, dx

        mov     si, offset pal          ;load this piece
        mov     ax, HLDPAL
        call    callafi

        pop     di
        pop     si
        pop     cx
        add     pal + 4, cx             ;increment the pallette index
        pop     ax
        sub     ax, cx
        jnz     cpallp

        ret

w8514pal        endp


;********************************************************************
;* 8514/A Hardware Interface Routines
;* Written by Aaron M. Williams for Fractint
;* This code may be used freely by anyone for anything and freely distributed.
;* All routines here are written for a V20 (80186) or better CPU.
;* All code has been at least partially optimized for a 486 (i.e. pipelining)
;* The macros were written by Roger Brown for Adex Corporation and have been
;* placed into the public domain by Adex corporation.
;*
;* Special support has been added for the Brooktree RAMDAC, which uses 8
;* bits for rgb values instead of 6 bits.  This RAMDAC is used only in the
;* 1280x1024 mode (unless programmed otherwise)
;*
;* Completed on 3/8/92

;* Revised by JCO on 4/12/92
; changed width to wdth and other minor fixes so it would assemble
;   using MASM 6.0
; took out duplicate variables, xadj, yadj, linedata
; took out .model C
; added VIDEO_TEXT to .code and made procedures near
; added TRANSY macro for 640x480x16 (512K), but not used
; w8514hwpal
;  Changed normal 8514/A routine to slow it down
; reopen8514hw
;  Renamed enableHIRES as reopen8514hw
;  Commented out old reopen8514hw
; open8514hw
;  Changed where board is reset so a hung board won't prevent detect of 8514/A
;  Added load8514dacbox routine to initialize colors
;  Added detection and setup of 512K 8514/A, use debug=8514 to test on 1 Meg
;  Changed foreground mix to FSS_FRGDCOL.  It's faster to use FRGD_COLOR for
;   dots and set/reset the mix for boxes.
;  Took out call to w8514hwpal, hangs machine if another video mode is not used
;   first, load8514dacbox takes care of loading initial colors
; fr85hwwdot, fr85hwrdot
;  Replaced with routines that use short stroke vectors, uses fewer port calls
; fr85hwwbox, fr85hwrbox
;  Replaced with routines that use vectors
; close8514hw
;  Made non-Adex 8514/A not use the enableVGA routine
; load8514dacbox
;  Added this routine to load dacbox

;* Added support for ATI ULTRA 800x600x256 and 1280x1024x16 modes JCO, 11/7/92

        .286    ; we use 286 code here for speed
                ; for in graphics, speed is everything

.DATA

; Defines

BIT_0_ON                EQU     0000000000000001b
BIT_1_ON                EQU     0000000000000010b
BIT_2_ON                EQU     0000000000000100b
BIT_3_ON                EQU     0000000000001000b
BIT_4_ON                EQU     0000000000010000b
BIT_5_ON                EQU     0000000000100000b
BIT_6_ON                EQU     0000000001000000b
BIT_7_ON                EQU     0000000010000000b
BIT_8_ON                EQU     0000000100000000b
BIT_9_ON                EQU     0000001000000000b
BIT_10_ON               EQU     0000010000000000b
BIT_11_ON               EQU     0000100000000000b
BIT_12_ON               EQU     0001000000000000b
BIT_13_ON               EQU     0010000000000000b
BIT_14_ON               EQU     0100000000000000b
BIT_15_ON               EQU     1000000000000000b

BIT_0_OFF               EQU     1111111111111110b
BIT_1_OFF               EQU     1111111111111101b
BIT_2_OFF               EQU     1111111111111011b
BIT_3_OFF               EQU     1111111111110111b
BIT_4_OFF               EQU     1111111111101111b
BIT_5_OFF               EQU     1111111111011111b
BIT_6_OFF               EQU     1111111110111111b
BIT_7_OFF               EQU     1111111101111111b
BIT_8_OFF               EQU     1111111011111111b
BIT_9_OFF               EQU     1111110111111111b
BIT_10_OFF              EQU     1111101111111111b
BIT_11_OFF              EQU     1111011111111111b
BIT_12_OFF              EQU     1110111111111111b
BIT_13_OFF              EQU     1101111111111111b
BIT_14_OFF              EQU     1011111111111111b
BIT_15_OFF              EQU     0111111111111111b

;==========================================
; Equates for use with Wait_Till_FIFO Macro
;==========================================
ONEEMPTY                = BIT_7_ON
TWOEMPTY                = BIT_6_ON
THREEEMPTY              = BIT_5_ON
FOUREMPTY               = BIT_4_ON
FIVEEMPTY               = BIT_3_ON
SIXEMPTY                = BIT_2_ON
SEVENEMPTY              = BIT_1_ON
EIGHTEMPTY              = BIT_0_ON

VSYNC_BIT_MASK          = BIT_0_ON
DEFAULT_MASK            = 00FFh                 ; Default to all 8 bits.
LOCK_FLAG               = BIT_15_ON             ; Flag to lock CRTC timing regs

GE_BUSY_MASK            = BIT_9_ON              ; Mask for GE_BUSY
GE_BUSY_SHFT            = 9

MAX_RES_MASK            = BIT_3_ON
MAX_RES_SHFT            = 3

PATTERN_POS_MASK        = (BIT_13_ON+BIT_12_ON+BIT_11_ON+BIT_10_ON+BIT_9_ON+BIT_8_ON)
PATTERN_POS_SHFT        = 8

SELECT_DPAGE_2          = BIT_1_ON
SELECT_DPAGE_1          = BIT_1_OFF

SELECT_VPAGE_2          = BIT_2_ON
SELECT_VPAGE_1          = BIT_2_OFF

LOWER_12_BITS           = 0FFFh                 ; Mask off upper 4 bits


;*****************************************************************************
;
; MACRO DEFINITIONS
;
;*****************************************************************************

;PURPOSE     : Wait until there is [number] locations available in the FIFO.

        Wait_Till_FIFO  MACRO   number
LOCAL   waitfifoloop

         mov    dx, GP_STAT
waitfifoloop:
         in     ax, dx
         test   ax, number
         jnz    waitfifoloop
ENDM


; Wait if Hardware is Busy
        Wait_If_HW_Busy MACRO
LOCAL   waithwloop
        mov     dx, GP_STAT
waithwloop:
        in      ax, dx
        test    ax, FSR_HWBUSY
        jnz     waithwloop
ENDM


; PURPOSE: Wait Till CPU data is Available (Used for Image Read/Write)

        Wait_Till_Data_Avail    MACRO
LOCAL   waitdataloop
        mov     dx, GP_STAT
waitdataloop:
        in      ax, dx
        test    ax, 0100h
        jz      waitdataloop
ENDM


; PURPOSE: Output a word to the specified i/o port

        Out_Port        MACRO   port, value

IFIDNI <ax>, <value>    ;; [ax] already loaded
ELSE
        mov     ax, value
ENDIF

IFIDNI  <dx>, <port>    ;; [dx] already loaded
ELSE
        mov     dx, port
ENDIF
        out     dx, ax
ENDM

; PURPOSE: Input a word from the specified i/o port

        In_Port MACRO   port
IFIDNI  <dx>, <port>    ;; [dx] already loaded
ELSE
        mov     dx, port
ENDIF
        in      ax, dx
ENDM


; PURPOSE: Output a byte to the specified i/o port

        Out_Port_Byte   MACRO   port, value
IFIDNI  <al>, <value>   ;; [al] already loaded
ELSE
        mov     al, value
ENDIF
IFIDNI  <dx>, <port>    ;; [dx] already loaded
ELSE
        mov     dx, port
ENDIF
        out     dx, al
ENDM


; PURPOSE: Input a byte from the specified i/o port

        In_Port_Byte    MACRO   port
        mov     dx, port                ;; output contents of ax
        in      al, dx                  ;; al = value from [port]
ENDM


; PURPOSE: Wait for Vsync to go low, then high,

                Wait_For_Vsync  MACRO
LOCAL   wait_low, wait_high
        mov     dx, SUBSYS_STAT
        mov     ax, RVBLNKFLAG
        out     dx, ax               ; Clear Vsync status bit
wait_low:
;        in      ax, dx
;        test    ax, VSYNC_BIT_MASK
;        jnz     wait_low               ; causes problems with ATI ********

wait_high:
        in      ax, dx
        test    ax, VSYNC_BIT_MASK
        jz      wait_high            ; Loop until beginning of Vysnc (blank)
ENDM


; PURPOSE: Enter Western Digital Enhanced Mode.

        Enter_WD_Enhanced_Mode  MACRO

                        mov     dx, WD_ESCAPE_REG
                        in      al, dx
ENDM


; PURPOSE: Write pixel data in [ax] to PIX_TRANS port [dx]
; ENTRY  : [dx] = PIX_TRANS, data in [ax]

        Write_A_Pixel   MACRO
        Out_Port        dx, ax
ENDM


; PURPOSE: Resets MULTIFUNC_CNTL register to FCOL & MIX

        Reset_MULTIFUNC_CNTL    MACRO
        Out_Port        MULTIFUNC_CNTL, 0A000h
ENDM

;
; TRANSY
;
; Translate y value for the case of 4 bpp and 640x480
; The y value is assumed to be in ax.
; The result is left in ax.
; result = (y & 1) | ((y >> 1) << 2)
; by Jonathan Osuch, 2/15/92
; Not needed by Graphics Ultra, others might need it
;
TRANSY   macro
         push  bx
         mov   bx, ax
         and   bx, 1
         shr   ax, 1
         shl   ax, 1
         shl   ax, 1
         or    ax, bx
         pop   bx
         endm
;


;========================================
; Return Value Definitions
;========================================
TRUE    = 1
FALSE   = 0

;*****************************************************************************
;
;               VESA STANDARD 8514/A REGISTER MNEMONICS
;
;*****************************************************************************

;=============================
; 8514/A READBACK REGISTER SET
;=============================
SETUP_ID1               equ 00100h  ; Setup Mode Identification
SETUP_ID2               equ 00101h  ; Setup Mode Identification
DISP_STAT               equ 002E8h  ; Display Status
WD_ESCAPE_REG           equ 028E9h  ; WD Escape Functions
SUBSYS_STAT             equ 042E8h  ; Subsystem Status
WD_ENHANCED_MODE_REG    equ 096E8h  ; Enter WD Enhanced Mode
GP_STAT                 equ 09AE8h  ; Graphics Processor Status
FSR_HWBUSY              equ 00200h  ; Bit Set if Hardware Busy

;==========================
; 8514/A WRITE REGISTER SET
;==========================
SETUP_OPT               equ 00102h  ; Setup Mode Option Select
H_TOTAL                 equ 002E8h  ; Horizontal Total
DAC_MASK                equ 002EAh  ; DAC Mask
DAC_R_INDEX             equ 002EBh  ; DAC Read Index
DAC_W_INDEX             equ 002ECh  ; DAC Write Index
DAC_DATA                equ 002EDh  ; DAC Data
H_DISP                  equ 006E8h  ; Horizontal Displayed
H_SYNC_STRT             equ 00AE8h  ; Horizontal Sync Start
H_SYNC_WID              equ 00EE8h  ; Horizontal Sync Width
V_TOTAL                 equ 012E8h  ; Vertical Total
V_DISP                  equ 016E8h  ; Vertical Displayed
V_SYNC_STRT             equ 01AE8h  ; Vertical Sync Start
V_SYNC_WID              equ 01EE8h  ; Vertical Sync Width
DISP_CNTL               equ 022E8h  ; Display Control
SUBSYS_CNTL             equ 042E8h  ; Subsystem Control
ICR_GERESET             equ 09000h  ; reset mask
ICR_NORMAL              equ 08000h  ; normal mask
ROM_PAGE_SEL            equ 046E8h  ; ROM Page Select
ADVFUNC_CNTL            equ 04AE8h  ; Advanced Function Control
MODE_VGA                equ 00010b  ;
MODE_768                equ 00111b  ;
MODE_480                equ 00011b  ;
CUR_Y                   equ 082E8h  ; Current Y Position
CUR_X                   equ 086E8h  ; Current X Position
DESTY_AXSTP             equ 08AE8h  ; Destination Y Position /
                                    ; Axial Step Constant
DESTX_DIASTP            equ 08EE8h  ; Destination X Position /
                                    ; Axial Step Constant
ERR_TERM                equ 092E8h  ; Error Term
MAJ_AXIS_PCNT           equ 096E8h  ; Major Axis Pixel Count
CMD                     equ 09AE8h  ; Command
SHORT_STROKE            equ 09EE8h  ; Short Stroke Vector Trnsf
BKGD_COLOR              equ 0A2E8h  ; Background Color
FRGD_COLOR              equ 0A6E8h  ; Foreground Color
WRT_MASK                equ 0AAE8h  ; Write Mask
RD_MASK                 equ 0AEE8h  ; Read Mask
COLOR_CMP               equ 0B2E8h  ; Color Compare
BKGD_MIX                equ 0B6E8h  ; Background Mix
FRGD_MIX                equ 0BAE8h  ; Foreground Mix
MULTIFUNC_CNTL          equ 0BEE8h  ; Multi-Function Control
PIX_TRANS               equ 0E2E8h    ; Pixel Data Transfer

MIN_AXIS_PCNT           equ  0000h   ; Minor Axis Pixel Count

T_SCISSORS              equ   1000h   ; Top Scissors
L_SCISSORS              equ   2000h   ; Left Scissors
B_SCISSORS              equ   3000h   ; Bottom Scissors
R_SCISSORS              equ   4000h   ; Right Scissors

MEM_CNTL                equ   5000h   ; Memory Control
PATTERN_L               equ   8000h   ; Fixed Pattern - Low
PATTERN_H               equ   9000h   ; Fixed Pattern - High
PIX_CNTL                equ  0A000h   ; Pixel Control

; Display Status bit field
HORTOG                  equ     0004h   ;
VBLANK                  equ     0002h   ;
SENSE                   equ     0001h

; Horizontal Sync Width Bit Field
HSYNCPOL_NEG            equ     0020h   ;       negative polarity
HSYNCPOL_POS            equ     0000h   ;       positive polarity

; Vertical Sync Width Bit Field
VSYNCPOL_NEG            equ     0020h   ;       negative polarity
VSYNCPOL_POS            equ     0000h   ;       positive polarity

; Display control  bit field
DISPEN_NC               equ   0000h     ; no change
DISPEN_DISAB    equ   0040h     ; disable display, syncs, and refresh
DISPEN_ENAB             equ   0020h     ; enable display, syncs, and refresh
INTERLACE               equ   0010h     ; interlace enable bit
DBLSCAN         equ   0008h     ; double scan bit
MEMCFG_2                equ   0000h     ; 2 CAS configuration
MEMCFG_4                equ   0002h     ;
MEMCFG_6                equ   0004h     ;
MEMCFG_8                equ   0006h     ;
ODDBANKENAB             equ   0001h     ; Use alternate odd/even banks for
                                        ; each line

; Subsystem status register bits
_8PLANE                 equ   0080h   ; 8 planes of memory installed
MONITORID_MASK          equ   0070h   ; Monitor ID mask
MONITORID_8503          equ   0050h   ;
MONITORID_8507          equ   0010h   ;
MONITORID_8512          equ   0060h   ;
MONITORID_8513          equ   0060h   ;
MONITORID_8514          equ   0020h   ;
MONITORID_NONE          equ   0070h   ;
GPIDLE                  equ   0008h   ; Processor idle bit, command queue empty
INVALIDIO               equ   0004h   ; Set when command written to full queue
                                      ; or the Pixel Data Transfer register was
                                      ; read when no data was available.  This
                                      ; bit must be cleared prior to any other
                                      ; operation with RINVALIDIO bit
PICKFLAG                equ   0002h   ; This bit is set when a write inside the
                                      ; clipping rectangle is about to be made.
                                      ; You can clear it with RPICKFLAG
VBLNKFLAG               equ   0001h   ; This bit is set at the start of the
                                      ; vertical blanking period.  It can only
                                      ; be cleared by setting RVBLNKFLG

; Subsystem Control Register bit field
GPCTRL_NC               equ     0000h   ;       no change
GPCTRL_ENAB             equ     4000h   ;       enable 8514
GPCTRL_RESET            equ     8000h   ;       reset 8514/A and disable
                                        ;       also flushes command queue
CHPTEST_NC              equ     0000h   ;       no change
CHPTEST_NORMAL          equ     1000h   ;       Enables synchronization between
                                        ;       chips
CHPTEST_ENAB            equ     2000h   ;       Disables synchronization.  Use
                                        ;       only as a diagnostic procedure
IGPIDLE                 equ     0800h   ;       Enable GPIDLE interrupt.
                                        ;       Usually this is IRQ9 (SW IRQ2)
IINVALIDIO              equ     0400h   ;       Enable invalid I/O interrupt
                                        ;       Interrupt when subsystem status
                                        ;       register INVALIDIO bit set
IPICKFLAG               equ     0200h   ;       Interrupts the system when
                                        ;       PICKFLAG in the subsystem
                                        ;       status register goes high
IVBLNKFLAG              equ     0100h   ;       Interrupts the system when
                                        ;       VBLNKFLAG in Subsystem Status
                                        ;       goes high
RGPIDLE                 equ     0008h   ;       Resets GPIDLE bit in subsystem
                                        ;       status register
RINVALIDIO              equ     0004h   ;       Resets INVALIDIO bit in
                                        ;       Subsystem Status Register
RPICKFLAG               equ     0002h   ;       Resets PICKFLAG in Subsystem
                                        ;       Status Register
RVBLNKFLAG              equ     0001h   ;       Resets VBLNKFLAG in Subsystem
                                        ;       Status Register

; Current X, Y and Destination X, Y mask
COORD_MASK              equ     07FFh   ;       coordinate mask (2047)

; Advanced Function Control Register bit field
CLKSEL                  equ     0004h   ; 1 = 44.9 MHz clock, 0 = 25.175 MHz
DISSABPASSTHRU          equ     0001h   ; 0 = VGA pass through, 1 = 8514/A

; Graphics Processor Status Register
GPBUSY                  equ     0200h   ; 1 when processor is busy in command
                                        ; and in data transfer
DATARDY                 equ     0100h   ; 0 = no data ready to be read
                                        ; 1 = data ready for reading.
                                        ; used for Pixel Data Transfer reads

; Command Register
CMD_NOP                 equ     0000h   ; do nothing
CMD_LINE                equ     2000h   ; Draw a line according to LINETYPE bit
                                        ; when LINETYPE = 1, bits 567 specify
                                        ; direction of vector with length
                                        ; stored in Major Axis Pixel Count
CMD_RECT                equ     4000h   ; Fast-Fill Rectangle accordign to
                                        ; PLANEMODE.  Can read as well as write
                                        ; according to PCDATA and WRTDATA
CMD_RECTV1              equ     6000h   ; Draws a rectangle vertically in
                                        ; columns starting at the upper left
                                        ; and working down
CMD_RECTV2              equ     8000h   ; Like CMD_RECT1, except accesses 4
                                        ; pixels at a time horizontally rather
                                        ; than 1
CMD_LINEAF              equ     0A000h  ; Draw line for area fill.  Only draws
                                        ; one pixel for each scan line crossed.
CMD_BITBLT              equ     0C000h  ; Copy rectangle on display and to/from
                                        ; PC memory through Pixel Data Transfer
                                        ; register.
CMD_OP_MSK              equ     0E000h  ; command mask
BYTSEQ                  equ     01000h  ; Selects byte ordering for pixel data
                                        ; transfer and short-stroke vector
                                        ; transfer registers only.  0 = high
                                        ; byte first, low byte second, 1 =
                                        ; low byte first, high byte second.
_16BIT                  equ     00200h  ; Affects Pixel Data Transfer and Short
                                        ; Stroke Vector Transfer registers.
                                        ; 0 = 8-bit access, 1 = 16-bit access
PCDATA                  equ     00100h  ; 0 = drawing operations use 8514/A
                                        ;     based data
                                        ; 1 = drawing operations wait for
                                        ;     data to be written or read from
                                        ;     the Pixel Data Transfer register
                                        ;     before proceeding to the next
                                        ;     pixel.  Direction of transfer
                                        ;     is based on WRTDATA
INC_Y                   equ     00080h  ; Determines y direction of lines
                                        ; during line drawing when LINETYPE is
                                        ; cleared.
                                        ; 0 = UP, 1 = DOWN
YMAJAXIS                equ     00040h  ; Determines major axis when LINETYPE
                                        ; is 0.
                                        ; 0 = X is major axis, 1 = Y is major
                                        ; axis.
INC_X                   equ     00020h  ; Determines direction of X when drawing
                                        ; lines when LINETYPE = 0
                                        ; 0 = right to left (negative X dir)
                                        ; 1 = left to right (positive X dir)
DRAW                    equ     00010h  ; 0 = move only, no pixels drawn
                                        ; 1 = draw
LINETYPE                equ     00008h  ; Selects line drawing algorithm
                                        ; 0 = normal Bresenham line drawing
                                        ; 1 = vector drawing CMD_NOP = short
                                        ;       stroke, CMD_LINE = long line
LASTPIX                 equ     00004h  ; 0 = last pixel for lines and vectors
                                        ;       drawn
                                        ; 1 = last pixel not drawn
PLANAR                  equ     00002h  ; Access is Pixel at a time or Planar
                                        ; 0 = Pixel, 1 = Planar
WRTDATA                 equ     00001h  ; 0 = read operation, 1 = write
                                        ; used for Pixel Data Transfer

; Short Stroke vector transfer register
; can also be used for command register when LINETYPE=1 and CMD_LINE
VECDIR_000              equ     0000h
VECDIR_045              equ     0020h
VECDIR_090              equ     0040h
VECDIR_135              equ     0060h
VECDIR_180              equ     0080h
VECDIR_225              equ     00A0h
VECDIR_270              equ     00C0h
VECDIR_315              equ     00E0h
SSVDRAW                 equ     0010h   ; 0 = move position, 1 = draw vector
                                        ; and move

; Background MIX register
BSS_BKGDCOL             equ     0000h   ; use background color
BSS_FRGDCOL             equ     0020h   ; use foreground color
BSS_PCDATA              equ     0040h   ; PC data (via Pixel Data Transfer reg)
BSS_BITBLT              equ     0060h   ; All-Plane Copy

; Foreground MIX register
FSS_BKGDCOL             equ     0000h   ; use background color
FSS_FRGDCOL             equ     0020h   ; use foreground color
FSS_PCDATA              equ     0040h   ; PC data (via Pixel Data Transfer reg)
FSS_BITBLT              equ     0060h   ; All-Plane Copy

; Mixing applications
MIX_MASK                equ     001Fh   ; mask for mixing values

MIX_NOT_DST             equ     0000h   ; NOT Dst
MIX_0                   equ     0001h   ; All bits cleared
MIX_1                   equ     0002h   ; All bits set

MIX_DST                 equ     0003h   ; Dst
MIX_LEAVE_ALONE         equ     0003h   ; Do nothing

MIX_NOT_SRC             equ     0004h   ; NOT Src

MIX_SRC_XOR_DST         equ     0005h   ; Src XOR Dst
MIX_XOR                 equ     0005h   ;

MIX_NOT__SRC_XOR_DST    equ     0006h   ; NOT (Src XOR Dst)
MIX_XNOR                equ     0006h   ;

MIX_SRC                 equ     0007h   ; Src
MIX_REPLACE             equ     0007h   ;
MIX_PAINT               equ     0007h   ;

MIX_NOT_SRC_OR_NOT_DST  equ     0008h   ; Not Src OR NOT Dst
MIX_NAND                equ     0008h   ;

MIX_NOT_SRC_OR_DST      equ     0009h   ; NOT Src OR Dst
MIX_SRC_OR_NOT_DST      equ     000Ah   ; Src OR NOT Dst

MIX_SRC_OR_DST          equ     000Bh   ; Src OR Dst
MIX_OR                  equ     000Bh   ;

MIX_SRC_AND_DST         equ     000Ch   ; Src AND Dst
MIX_AND                 equ     000Ch

MIX_SRC_AND_NOT_DST     equ     000Dh   ; Src AND NOT Dst
MIX_NOT_SRC_AND_DST     equ     000Eh   ; NOT Src AND Dst

MIX_NOT_SRC_AND_NOT_DST equ     000Fh   ; NOT Src AND NOT Dst
MIX_NOR                 equ     000Fh   ; Src NOR Dst

MIX_MIN                 equ     0010h   ; MINIMUM (Src, Dst)
MIX_DST_MINUS_SRC       equ     0011h   ; Dst - Src (with underflow)
MIX_SRC_MINUS_DST       equ     0012h   ; Src - Dst (with underflow)
MIX_PLUS                equ     0013h   ; Src + Dst (with overflow)
MIX_MAX                 equ     0014h   ; MAXIMUM (Src, Dst)
MIX_HALF__DST_MINUS_SRC equ     0015h   ; (Dst - Src) / 2 (with underflow)
MIX_HALF__SRC_MINUS_DST equ     0016h   ; (Src - Dst) / 2 (with underflow)
MIX_AVERAGE             equ     0017h   ; (Src + Dst) / 2 (with overflow)
MIX_DST_MINUS_SRC_SAT   equ     0018h   ; (Dst - Src) (with saturate)
MIX_SRC_MINUS_DST_SAT   equ     001Ah   ; (Src - Dst) (with saturate)
MIX_PLUS_SAT            equ     001Bh   ; (Src + Dst) (with saturate)
MIX_HALF__DST_MINUS_SRC_SAT     equ     001Ch   ; (Dst - Src) / 2 (with sat)
MIX_HALF__SRC_MINUS_DST_SAT     equ     001Eh   ; (Src - Dst) / 2 (with sat)
MIX_AVERAGE_SAT         equ     001Fh   ; (Src + Dst) / 2 (with saturate)

; Memory control register
BUFSWP                  equ     0010h   ; pseudo 8-plane on 4-plane board
VRTCFG_2                equ     0000h   ; vertical memory configuration
VRTCFG_4                equ     0004h
VRTCFG_6                equ     0008h
VRTCFG_8                equ     000Ch
HORCFG_4                equ     0000h   ; Horizontal memory configuration
HORCFG_5                equ     0001h
HORCFG_8                equ     0002h
HORCFG_10               equ     0003h

; Pixel Control Register
MIXSEL_FRGDMIX          equ     0000h   ; use foreground mix for all drawing
                                        ; operations
MIXSEL_PATT             equ     0040h   ; use fixed pattern to decide which
                                        ; mix setting to use on a pixel
MIXSEL_EXPPC            equ     0080h   ; PC Data Expansion.  Use data from
                                        ; Pixel Transfer Register
MIXSEL_EXPBLT           equ     00C0h   ; Bits in source plane determine
                                        ; foreground or background MIX
                                        ; 0 = bkgd, 1 = frgd
COLCMPOP_F              equ     0000h   ; FALSE
COLCMPOP_T              equ     0008h   ; TRUE
COLCMPOP_GE             equ     0010h   ; Dst >= CC
COLCMPOP_LT             equ     0018h   ; Dst < CC
COLCMPOP_NE             equ     0020h   ; Dst != CC
COLCMPOP_EQ             equ     0028h   ; Dst == CC
COLCMPOP_LE             equ     0030h   ; Dst <= CC
COLCMPOP_GT             equ     0038h   ; Dst > CC
PLANEMODE               equ     0004h   ; Enables plane mode for area fill and
                                        ; single plane expansion

; The following code was written largely by Aaron Williams
; and largely mucked up by Jonathan Osuch

.DATA
        TEMP_SIZE       =        12
        NUM_ENTRIES     =       256

__temp_palette          DB      TEMP_SIZE DUP (?)

Gra_mode_ctl_sh         dw      ?
WD_enhance_mode_sh      dw      0

extrn   sxdots:word, sydots:word  ; number of dots across and down
extrn   dacbox:byte

extrn           daccount:word           ; count of entries in DAC table

extrn           cpu:word                ; CPU type 88, 186, etc.
extrn           debugflag:word  ; for debugging purposes

wdth            dw      0       ; JCO 4/11/92
height  dw      0

bppstatus       dw      0       ; temporary status for bpp      ; JCO 4/11/92
bpp4x640        db      0       ; flag for 4 bpp and 640x480    ; JCO 4/11/92

adexboard       db      0       ; set to 1 when ADEX board
currentmode     dw      0       ; points to current mode table
ati_enhance_mode        dw      0       ; 1 for 800x600, 11h for 1280x1024
loadset dd      0C0000064h      ; entries to bios jump table
setmode dd      0C0000068h      ; modified later if rom is moved
ati_temp        dw      0

; 8514/A initialization tables written by Aaron Williams
mode640 dw      2381h   ; Western Digital Enhanced Mode Register
                dw      0003h   ; advanced function control
                dw      5006h   ; Multifunction control
                dw      0063h   ; Horizontal total
                dw      004Fh   ; Horizontal displayed
                dw      0052h   ; Horizontal sync start
                dw      002Ch   ; Horizontal sync width
                dw      0418h   ; Vertical total
                dw      03BBh   ; Vertical displayed
                dw      03D2h   ; Vertical sync start
                dw      0022h   ; Vertical sync width
                dw      0023h   ; Display control


mode1024        dw      2501h   ; Western Digital Enhanced Mode Register
                                ; I will later add options for 70hz mode,
                                ; interlaced mode, etc.  This is used only
                                ; for Adex or compatible boards
                                ; for 70 hz, change to 2581h
                dw      0007h   ; advanced function control
                dw      5006h   ; Multifunction control
                dw      00A2h   ; Horizontal total
                dw      007Fh   ; Horizontal displayed
                dw      0083h   ; Horizontal sync start
                dw      0016h   ; Horizontal sync width
                dw      0660h   ; Vertical total
                dw      05FBh   ; Vertical displayed
                dw      0600h   ; Vertical sync start
                dw      0008h   ; Vertical sync width
disp1024        dw      0023h   ; Display control

; The 1280 mode is supported only on Adex boards.  If anyone has any info on
; other boards capable of this mode, I'd like to add support.
mode1280        dw      2589h   ; WD enhanced mode register
                dw      0007h   ; advanced function control
                dw      5006h   ; Multifunction control
                dw      0069h   ; Horizontal total
                dw      004Fh   ; Horizontal displayed
                dw      0053h   ; Horizontal sync start
                dw      0009h   ; Horizontal sync width
                dw      0874h   ; Vertical total
                dw      07FFh   ; Vertical displayed
                dw      0806h   ; Vertical sync start
                dw      0003h   ; Vertical sync width
                dw      0023h   ; Display control

; 4bpp mode added by JCO 4/5/92, 1024x4 same as 1024x8
mode640x4       dw      0000h   ; Western Digital Enhanced Mode Register ????
                dw      0003h   ; advanced function control
                dw      5002h   ; Multifunction control ; This may need to be 5000h
                dw      0063h   ; Horizontal total
                dw      004Fh   ; Horizontal displayed
                dw      0052h   ; Horizontal sync start
                dw      002Ch   ; Horizontal sync width
                dw      0830h   ; Vertical total
                dw      0779h   ; Vertical displayed
                dw      07A8h   ; Vertical sync start
                dw      0022h   ; Vertical sync width
                dw      0021h   ; Display control               ; This may need to be 0020h


.CODE

; This routine updates the 8514/A palette
; For modes with resolutions > 1024x768, a different DAC must be used.
; The ADEX board uses a high-speed Brooktree DAC which uses 24 bits per
; color instead of the usual 18 bits.
; The data is written out in 3 parts during vertical retrace to prevent snow.
; Normal 8514/A routine modified to slow down the spin, JCO 4/3/92
w8514hwpal        proc    far

        mov     si, offset dacbox
        cld

        ; dac_w_index
        mov     dx, DAC_W_INDEX
        mov     al, 0           ;start at beginning of 8514a palette
        out     dx, al;

        cmp     wdth, 1024
        jbe     writedac
        cmp     adexboard, 1
        je      wbrooktree


writedac:               ; rewritten to slow down the spin,  JCO 4/11/92
        mov     cx, daccount
        mov     bx, 0           ;use bx to hold index into the dac

        mov     ax, 256
cpallp:
        push    ax
        cmp     ax, cx
        jae     dopass
        mov     cx, ax
dopass:
        push    cx

; wait for first vertical blank
        mov     dx, DISP_STAT
chkvblnk1:              ;loop til vertical blank
        in      ax, dx          ;read status register
        test    ax, VBLANK
        jz      chkvblnk1               ;set to 1 during vertical blank

; wait for screen to display
chkvblnk2:              ;loop while screen displayed
        in      ax, dx          ;read status register
        test    ax, VBLANK
        jnz     chkvblnk2               ;set to 0 during screen display

; wait for next vertical blank, make sure we didn't miss it
chkvblnk3:              ;loop til vertical blank
        in      ax, dx          ;read status register
        test    ax, VBLANK
        jz      chkvblnk3               ;set to 1 during vertical blank

; move the palette in dacbox
        mov     dx, DAC_DATA

cpall2:
        outsb           ;put red into 8514/a palette
        outsb           ;put green into 8514/a palette
        outsb           ;put blue into 8514/a palette

        loop    cpall2

        pop     cx
        add     bx, cx
        mov     dx, DAC_W_INDEX ;load next piece of palette
        mov     ax, bx
        out     dx, al

        pop     ax
        sub     ax, cx
        jnz     cpallp

        sti
        ret

wbrooktree:                     ; we go here for updating the Brooktree
        mov     cx, 256         ; output first 1/3 of data
        cli
        Wait_For_Vsync          ; wait for vertical retrace
        mov     dx, 02EDh
pall1:                          ; the brooktree uses 8 bits instead of 6
        lodsb
        shl     al, 2
        out     dx, al
        loop    pall1
        sti

        mov     cx, 256         ; output second 1/3 of data
        cli
        Wait_For_Vsync          ; wait for vertical retrace
        mov     dx, 02EDh
pall2:
        lodsb
        shl     al, 2
        out     dx, al
        loop    pall2
        sti

        mov     cx, 256         ; output third 1/3 of data
        cli
        Wait_For_Vsync          ; wait for vertical retrace
        mov     dx, 02EDh
pall3:
        lodsb
        shl     al, 2
        out     dx, al
        loop    pall3

        sti
        ret
w8514hwpal      endp


; reopen8514hw turns off VGA pass through and enables the 8514/A display
reopen8514hw    PROC    far
        cmp     adexboard, 0
        je      enableati

        mov     dx, WD_ESCAPE_REG
        in      al, dx

        mov     si, currentmode
        mov     dx, WD_ENHANCED_MODE_REG
        outsw
        jmp     enablegeneric

enableati:
        cmp     ati_enhance_mode, 0
        je      enablegeneric
        mov     ax, ati_enhance_mode    ; load mode into shadow set 1 (lores)
        call    dword ptr [loadset]
        mov     ax, 1   ; set lores mode
        call    dword ptr [setmode]
        ret

enablegeneric:
        mov     ax, [Gra_mode_ctl_sh]   ; Read shadow register.
        or      ax, BIT_0_ON            ; Set for HIRES mode
        mov     [Gra_mode_ctl_sh], ax   ; Update shadow register.
        Out_Port        ADVFUNC_CNTL, ax
        ret
reopen8514hw    ENDP


; open8514hw initializes the 8514/A for drawing graphics.  It test for the
; existence of an 8514/A first.
; CY set on error
open8514hw      proc    far
        ; Test for the existence of an 8514/A card by writing to and reading
        ; from the Error term register.
        xor     al, al
        mov     adexboard, al

        call    load8514dacbox  ; load dacbox for 8514/A setup  JCO 4/6/92

        ; Assume 8514/A present and reset it.  Otherwise a locked up board
        ;  would not appear as an 8514/A.  JCO 4/3/92
        ; reset 8514/A subsystem
        mov     dx, SUBSYS_CNTL
        mov     ax, GPCTRL_RESET+CHPTEST_NORMAL ; Reset + Normal
        out     dx, ax
        mov     ax, GPCTRL_ENAB+CHPTEST_NORMAL  ; Enable + Normal
        out     dx, ax

        mov     dx, ERR_TERM
        mov     ax, 5A5Ah       ; output our test value
        out     dx, ax
        jmp     $+2             ; add slight delay
        jmp     $+2
        in      ax, dx

        cmp     ax, 5A5Ah
        je      Found_8514      ; jump if ok

        stc     ; set error if not found
        ret

Found_8514:
        ; we need at least a 186 or better for rep outsw and stuff for speed.
        ; We won't support the 8086/8088, since it's *very* unlikely that
        ; anyone with an 8088 based machine would invest in an 8514/A
        ;
        cmp     [cpu], 88
        jne     GoodCPU

        stc
        ret

GoodCPU:                                ; JCO 5/8/92
        mov     bx, sxdots              ; uncommented this section and made check
        cmp     bx, 640         ; for interlaced monitor vs non-interlaced
        jbe     monitor_ok              ; any old monitor should work non-interlaced

        mov     dx, SUBSYS_STAT
        in      ax,dx
        and     ax, MONITORID_MASK
        cmp     ax, MONITORID_8514      ; do we need to interlace?
        jz      setinterlaced           ; yes, jump
        cmp     ax, MONITORID_8507      ; do we need to interlace?
        jz      setinterlaced           ; yes, jump
        jmp     monitor_ok                      ; use default non-interlaced mode

setinterlaced:
        or      disp1024, INTERLACE     ; set interlace bit, JCO 5/8/92

monitor_ok:

        mov     bpp4x640, 0             ;clear flag for y value translation
        mov     dx, SUBSYS_STAT
        in      ax, dx

;************** debug 4bpp
        cmp     debugflag, 8514
        jne     notest
        test    ax, _8PLANE             ;if not set, 4bpp anyway, don't change
        jnz     notest
        xor     ax, _8PLANE             ;clear the 8bpp bit to test 4bpp
notest:
;**************

        mov     bppstatus, ax           ;save the status for a while
        test    ax, _8PLANE     ;is it 8 bits per pixel?
        jnz     plane8  ;yes, 1024K video memory

; no, only 512K video memory, 4 bits per pixel

        mov     ax, sxdots      ; AX contains H resolution

        ; test if 640x480x4bpp
        mov     bpp4x640, 1             ;set flag for y value translation
        mov     si, offset mode640x4    ; SI = offset of register data
        mov     bx, 640         ; BX = X resolution
        mov     cx, 480         ; CX = Y resolution
        mov     wdth, bx        ; store display width
        mov     height, cx      ; store display height
        cmp     ax, 640         ; jump if this resolution is correct
        jbe     setupopen

        ; test if 1024x768x4bpp
        mov     bpp4x640, 0             ;clear flag for y value translation
        mov     si, offset mode1024
        mov     bx, 1024
        mov     cx, 768
        mov     wdth, bx
        mov     height, cx
        cmp     ax, 1024
        jbe     setupopen
        stc                     ; oops, to high a resolution
        ret

plane8:
        mov     ax, sxdots      ; AX contains H resolution

        ; test if 640x480
        mov     si, offset mode640      ; SI = offset of register data
        mov     bx, 640         ; BX = X resolution
        mov     cx, 480         ; CX = Y resolution
        mov     wdth, bx        ; store display width
        mov     height, cx      ; store display height
        cmp     ax, 640         ; jump if this resolution is correct
        jbe     setupopen

        ; test if 800x600 (special ati ultra mode)
        ; si does not need to be set, everything is in eeprom
        mov     bx, 800
        mov     cx, 600
        mov     wdth, bx
        mov     height, cx
        cmp     ax, 800
        jbe     setupopen

        ; test if 1024x768
        mov     si, offset mode1024
        mov     bx, 1024
        mov     cx, 768
        mov     wdth, bx
        mov     height, cx
        cmp     ax, 1024
        jbe     setupopen

        ; must be 1280x1024
        mov     si, offset mode1280
        mov     bx, 1280
        mov     cx, 1024
        mov     wdth, bx
        mov     height, cx

setupopen:
        ; test for Western Digital Chipset
        mov     currentmode, si
        mov     dx, WD_ESCAPE_REG
        in      al, dx

        mov     ax, 6AAAh
        mov     dx, MAJ_AXIS_PCNT
        out     dx, ax

        mov     dx, WD_ESCAPE_REG       ; enable enhanced mode for ADEX board
        in      al, dx

        mov     dx, MAJ_AXIS_PCNT ; if port 96E8 is between 3F00h and 2A00h we
        in      ax, dx            ; have a WD board, else IBM/Other
        cmp     ax, 2A00h
        jb      ati_ultra
        cmp     ax, 3F00h
        ja      ati_ultra

        ; We must have a Western Digital chip set.
        ; May not be Adex.
        ; Future test to implement will be to write a pixel to X,Y location
        ; with X > 1024 and read it back to check for enough memory for 1280
        ; mode.
        mov     al, 1
        mov     adexboard, al   ; set adex board
        mov     dx, WD_ESCAPE_REG       ; program WD
        in      al, dx

        mov     dx, WD_ENHANCED_MODE_REG
        outsw                                   ; output the Western Digital
                                                ; enhanced mode register

        mov     [WD_enhance_mode_sh], ax        ; keep a copy of it
        jmp     openOK

ibm_8514_step:
        jmp     ibm_8514

ati_ultra:
        mov     ati_enhance_mode, 0     ; make sure enhanced mode is clear
; check for ATI
        mov     dx, 52EEh               ; ROM_ADDR_1 register
        in      ax, dx
        mov     ati_temp, ax    ; temporary save
        mov     ax, 5555h
        out     dx, ax
        Wait_If_HW_Busy         ; make sure HW is not busy
        mov     dx, 52EEh               ; ROM_ADDR_1 register
        in      ax, dx
        cmp     ax, 5555h
        jne     ibm_8514_step   ; nope must be real 8514a

        mov     ax, 2A2Ah
        out     dx, ax
        Wait_If_HW_Busy         ; make sure HW is not busy
        mov     dx, 52EEh               ; ROM_ADDR_1 register
        in      ax, dx
        cmp     ax, 2A2Ah
        jne     ibm_8514_step   ; nope must be real 8514a

        mov     ax, ati_temp
        out     dx, ax  ; restore ROM_ADDR_1 register

        and     ati_temp, 007Fh ; calculate the ROM base address
        mov     ax, 80h         ; (ROM_ADDR_1 & 0x7F)*0x80 + 0xC000
        mul     ati_temp
        add     ax, 0C000h
        mov     word ptr loadset+2, ax
        mov     word ptr setmode+2, ax

        mov     es, ax
        mov     ax, es:4Ch      ; get ati bios revision
        cmp     al, 1h
        jl      ibm_8514        ; revision level too low, can't do it
        cmp     ah, 3h
        jl      ibm_8514        ; revision level too low, can't do it

; everything appears okay

; get resolution, could be 800x600 or 1280x1024
        mov     ax, sxdots      ; AX contains H resolution
        cmp     ax, 640
        jbe     ibm_8514        ; too low for 800x600
        cmp     ax, 800
        jbe     set_800x600     ; must be 800x600
        cmp     ax, 1024
        jbe     ibm_8514        ; too low for 1280x1024
        cmp     ax, 1280
        jbe     set_1280x1024   ; must be 1280x1024
        jmp     ibm_8514        ; too high

set_800x600:
        mov     ax, 1   ; load 800x600 into shadow set 1 (lores)
        call    dword ptr [loadset]
        jc      ibm_8514        ; didn't work, forget it
        mov     ax, 1   ; set lores mode
        call    dword ptr [setmode]
        jc      ibm_8514        ; didn't work, forget it

        mov     ati_enhance_mode, 1     ; save mode
        jmp     setplane8

set_1280x1024:
        mov     ax, 11h ; load 1280x1024 into shadow set 1 (lores)
        call    dword ptr [loadset]
        jc      ibm_8514        ; didn't work, forget it
        mov     ax, 1   ; set lores mode
        call    dword ptr [setmode]
        jc      ibm_8514        ; didn't work, forget it

        mov     ati_enhance_mode, 11h   ; save mode
        jmp     setplane4       ; 4 bits/pixel

ibm_8514:
        lodsw   ; ignore WD enhanced mode register
        cmp     wdth, 1024      ; make sure the resolution isn't too high
        jbe     openOK

        stc                     ; a *real* 8514/A cannot run higher than
        ret                     ; 1024x768, so quit

openOK:
        Wait_If_HW_Busy         ; make sure HW is not busy
        mov     dx, ADVFUNC_CNTL

        lodsw                                   ; get Multifunction Control
        mov     Gra_mode_ctl_sh, ax             ; keep a copy of it
        out     dx, ax

        mov     dx, MULTIFUNC_CNTL              ; program multifunction control
        outsw

        mov     dx, H_TOTAL                     ; program HTOTAL
        outsw

        mov     dx, H_DISP                      ; program HDISPLAYED
        outsw

        mov     dx, H_SYNC_STRT                 ; set start of HSYNC
        outsw

        mov     dx, H_SYNC_WID                  ; set width of HSYNC signal
        outsw

        mov     dx, V_TOTAL                     ; set vertical total
        outsw

        mov     dx, V_DISP                      ; set vertical resolution
        outsw

        mov     dx, V_SYNC_STRT                 ; set start of V Sync
        outsw

        mov     dx, V_SYNC_WID                  ; set width of V Sync signal
        outsw

        mov     dx, DISP_CNTL                   ; set the display control
        outsw

        mov     ax, bppstatus           ;get status again, push/pop doesn't work!
        test    ax, _8PLANE
        jnz     setplane8

; enable 4 bits per pixel
setplane4:
        mov     ax, 0Fh
        mov     dx, DAC_MASK
        out     dx, al
        mov     ax, 0FF0Fh
        mov     dx, WRT_MASK
        out     dx, ax
        jmp     doneset

setplane8:
; enable 8 bits per pixel
        mov     ax, 0FFh
        mov     dx, DAC_MASK
        out     dx, al
        mov     ax, 0FFFFh
        mov     dx, WRT_MASK
        out     dx, ax

doneset:
; Complete environment set up 3/20/92 JCO
        Out_Port        BKGD_MIX, <BSS_BKGDCOL or MIX_REPLACE>  ;set mixes
; FRGD_MIX set below
; WRT_MASK set above
        Out_Port        MULTIFUNC_CNTL, <PIX_CNTL or 0h>        ; clear lower bits
; 3/20/92 JCO

        ; set clipping
        Wait_Till_FIFO  FOUREMPTY       ; wait for room in queue
        mov     dx, MULTIFUNC_CNTL
        mov     ax, T_SCISSORS
        out     dx, ax                  ; set top clip to 0
        mov     ax, cx
        dec     ax
        or      ax, B_SCISSORS
        out     dx, ax                  ; set bottom clip to maxy

        mov     ax, L_SCISSORS
        out     dx, ax                  ; set left clip to 0

        mov     ax, bx
        dec     ax
        or      ax, R_SCISSORS          ; set right clip to maxx
        out     dx, ax

        ; clear screen
        Wait_Till_FIFO  SIXEMPTY       ; wait for room in FIFO
        mov     dx, FRGD_MIX
        mov     ax, 0021h       ; zero memory ?? why 21h and not 01h? JCO
        out     dx, ax

        xor     ax, ax
        mov     dx, CUR_X       ; set start of rectangle to 0,0
        out     dx, ax

        mov     dx, CUR_Y
        out     dx, ax

        mov     ax, wdth
        mov     dx, MAJ_AXIS_PCNT
        dec     ax
        out     dx, ax          ; set width of rectangle to draw

        mov     ax, height
        mov     dx, MULTIFUNC_CNTL
        dec     ax
        out     dx, ax          ; set height of rectangle to draw

;       Reset_MULTIFUNC_CNTL    ; done above
        mov     dx, CMD
        mov     ax, 42F3h               ; issue rect draw command
        out     dx, ax          ; draw the rectangle

;       Wait_Till_FIFO  ONEEMPTY        ; set write mask  ** done above
;       mov     dx, WRT_MASK            ; to include all
;       mov     ax, 00FFh                       ; bits
;       out     dx, ax

        Wait_Till_FIFO  THREEEMPTY      ; set foreground to default
        mov     dx, FRGD_MIX
;       mov     ax, FSS_PCDATA + MIX_SRC
        mov     ax, FSS_FRGDCOL + MIX_SRC       ;slightly faster to not use pcdata
                                                        ;to draw the dots, JCO 4/12/92
        out     dx, ax

        xor     ax, ax
        mov     dx, CUR_X               ; set X,Y back to 0,0
        out     dx, ax
        mov     dx, CUR_Y
        out     dx, ax

        ; enable palette
        Wait_If_HW_Busy         ; wait until HW is done

        cmp     adexboard, 1    ; if adex board, we must assume a brooktree
        jne     notadexdac      ; dac at all resolutions > 1024x768
        cmp     wdth, 1024      ;
        ja      brooktree       ; jmp if using special Brooktree RAMDAC

notadexdac:
;       mov     dx, 2EAh        ; set palette mask ** done above
;       mov     al, 0FFh
;       out     dx, al
        jmp     paletteinitdone

brooktree:                      ; brooktree DAC requires special
        mov     dx, 2ECh        ; configuration.  This DAC is required
        mov     al, 04h         ; for 1280x1024 resolution, and it is not
        out     dx, al          ; totally compatible with the standard RAMDAC
        mov     dx, 2EAh
        mov     al, 0FFh        ; This code enables the DAC for proper operation
        out     dx, al
        mov     dx, 2ECh
        mov     al, 05h
        out     dx, al
        mov     dx, 2EAh
        mov     al, 0h
        out     dx, al
        mov     dx, 2ECh
        mov     al, 06h
        out     dx, al
        mov     dx, 2EAh
        mov     al, 040h
        out     dx, al

paletteinitdone:
;       push    bx      ; This works if another video mode is used first. JCO 4/9/92
;       push    cx      ; Using load8514dacbox, above, sets the 8514/A palette.
;       call    w8514hwpal              ; set 8514 palette
;       pop     cx
;       pop     bx

        mov     ax, bx
        sub     ax, sxdots                      ;save centering factor
        shr     ax, 1
        mov     xadj, ax

        mov     ax, cx
        sub     ax, sydots
        shr     ax, 1
        mov     yadj, ax
        clc                     ; no errors
        ret
open8514hw      endp


;reopen8514hw   proc    near
; Return to 8514/A after VGA
;       call    enableHIRES
;       ret
;reopen8514hw   endp


fr85hwwdot      proc    far
; draws a pixel at cx,dx of color al
        mov     bx, dx          ; temporary save of dx (y position)

        push    ax              ; need to save ax register (color)
        Wait_Till_FIFO FIVEEMPTY

        pop     ax
        mov     dx, FRGD_COLOR
        out     dx, ax

        add     cx, xadj
        mov     ax, cx
        mov     dx, CUR_X
        out     dx, ax                  ; set x position

        mov     ax, bx                  ; put y position into ax
        add     ax, yadj

;******* not needed by Graphics Ultra, may be needed by others.
;******* would only be needed in 640x480x16 mode, 512K video memory
;       cmp     bpp4x640, 1
;       jne     wdotnorm
;       TRANSY  ;flag set, translate y value
;wdotnorm:

        mov     dx, CUR_Y
        out     dx, ax                  ; set y position

        ; next, set up command register
        Out_Port        CMD, <CMD_NOP or LINETYPE or WRTDATA>
        Out_Port        SHORT_STROKE, <VECDIR_000 or SSVDRAW or 0> ; plot 1 pixel

        ret

fr85hwwdot      endp


fr85hwwbox      proc    far uses si
; copies a line of data from ds:si to the display from cx,dx to ax,dx

        sub     ax, cx  ; delta is now in ax, 8514/a uses deltas
        mov     bx, ax  ; temporary save

        push    dx              ; need to save dx register (y value)
        Wait_Till_FIFO  SIXEMPTY
        pop     ax

        add     ax, yadj

;******* not needed by Graphics Ultra, may be needed by others.
;******* would only be needed in 640x480x16 mode, 512K video memory
;       cmp     bpp4x640, 1
;       jne     wboxnorm
;       TRANSY  ;flag set, translate y value
;wboxnorm:

        mov     dx, CUR_Y
        out     dx, ax  ; y position

        add     cx, xadj
        mov     ax, cx
        mov     dx, CUR_X
        out     dx, ax  ; x position

        mov     ax, bx
        mov     dx, MAJ_AXIS_PCNT
        out     dx, ax  ; line length delta

        Out_Port FRGD_MIX, <FSS_PCDATA or MIX_SRC>      ;set mix

        mov     cx, bx  ; number of pixels delta
        inc     cx              ; number of bytes to move
        shr     cx, 1           ; number of words to move
        jnc     evn             ; carry set if odd number of bytes
        inc     cx              ; write last byte
evn:

        Out_Port CMD, <CMD_LINE or LINETYPE or _16BIT or BYTSEQ or PCDATA \
                     or DRAW or WRTDATA or VECDIR_000>
        cld
        mov     dx, PIX_TRANS

        rep     outsw

        Out_Port FRGD_MIX, <FSS_FRGDCOL or MIX_SRC>     ;reset mix

        ret

fr85hwwbox      endp


fr85hwrdot      proc    far
; Reads a single pixel (x,y = cx,dx).  Color returned in ax

        push    dx              ; need to save dx register (y value)
        Wait_Till_FIFO  FOUREMPTY
        pop     ax              ; put y position into ax
        add     ax, yadj

;******* not needed by Graphics Ultra, may be needed by others.
;******* would only be needed in 640x480x16 mode, 512K video memory
;       cmp     bpp4x640, 1
;       jne     rdotnorm
;       TRANSY  ;flag set, translate y value
;rdotnorm:

        mov     dx, CUR_Y
        out     dx, ax                  ; set y position

        mov     ax, cx                  ; put x position into ax
        add     ax, xadj
        mov     dx, CUR_X
        out     dx, ax                  ; set x position

        ; next, set up command register
        Out_Port        CMD, <CMD_NOP or LINETYPE or PCDATA>
        Out_Port        SHORT_STROKE, <VECDIR_000 or SSVDRAW or 0> ; move to pixel

        Wait_Till_Data_Avail            ; make sure data is available to read
        In_Port PIX_TRANS

        ret

fr85hwrdot      endp

fr85hwrbox      proc    far uses es
; copies a line of data from cx,dx to ax,dx to es:di
        mov     bx, ds  ;set up string write
        mov     es, bx

        sub     ax, cx  ; delta is now in ax, 8514/a uses deltas
        mov     bx, ax  ; temporary save

        push    dx              ; need to save dx register
        Wait_Till_FIFO  FOUREMPTY
        pop     ax              ; put y value in ax

        add     ax, yadj

;******* not needed by Graphics Ultra, may be needed by others.
;******* would only be needed in 640x480x16 mode, 512K video memory
;       cmp     bpp4x640, 1
;       jne     rboxnorm
;       TRANSY  ;flag set, translate y value
;rboxnorm:

        mov     dx, CUR_Y
        out     dx, ax  ; y position

        add     cx, xadj
        mov     ax, cx
        mov     dx, CUR_X
        out     dx, ax  ; x position

        mov     ax, bx
        mov     dx, MAJ_AXIS_PCNT
        out     dx, ax  ; line length delta

        mov     cx, bx  ; number of pixels delta
        inc     cx              ; number of bytes to move
        shr     cx, 1           ; number of words to move
        jnc     revn            ; carry set if odd number of bytes
        inc     cx              ; read last byte
revn:

        Out_Port CMD, <CMD_LINE or LINETYPE or _16BIT or BYTSEQ or PCDATA \
                     or DRAW or VECDIR_000>
        cld             ; di is already set when routine is entered
        Wait_Till_Data_Avail    ; wait until data is available
        mov     dx, PIX_TRANS

        rep     insw

        ret

fr85hwrbox      endp


; enableVGA causes the VGA pass-through to be enabled.
; This was mostly written by Roger Brown, and optimized and updated by
; Aaron Williams
enableVGA       PROC    USES   ds es si di
        ; disable DAC mask
        mov    al, 0
        Out_Port_Byte  DAC_MASK, al

        Wait_For_Vsync

        ; use VGA lock up palette for Hi Res mode 8514/a display

        xor     bx, bx          ; Initial palette entry

read_palette_loop:
        mov     di, OFFSET __temp_palette       ; Buffer address

        mov     al, bl          ; [1] VGA 3C8h register is byte
                                ; register, use byte output only
        mov     dx, 3c8h        ; VGA_DAC_W reg.
        out     dx, al

        mov     dx, 3c7h        ; set VGA DAC read index
        out     dx, al          ; any write change mode

        mov     cx, TEMP_SIZE   ; Number of bytes to read each pass
        mov     dx, 3c9h        ; VGA_DAC_DATA_REG
        mov     ax, ds          ; Segment of _temp_palette[]
        mov     es, ax
        cld
        rep     insb            ; Read in the palette


        ; BX->start_index, CX->count, DS:DI->DWORD

        Out_Port_Byte  DAC_W_INDEX, bl
        cli
        cld
        mov     si, OFFSET __temp_palette       ; Buffer address
        mov     dx, DAC_DATA
        mov     cx, (TEMP_SIZE )     ; Number of entries to write each pass
        rep outsb                       ; update the DAC data
        sti

        mov     cx, (TEMP_SIZE / 3)
        add     bx, cx
        cmp     bx, NUM_ENTRIES
        jl      read_palette_loop

        ; reading vga DAC mask and writing to 8514 DAC mask 02EA

        mov     dx, 3c6h
        in      al, dx
        Out_Port_Byte  DAC_MASK, al

        ; enable vga pass-through mode

        mov     ax, Gra_mode_ctl_sh     ; Read shadow register.
                                        ; [2] No FIFO checking needed for
                                        ; 4AE8h register.
        and     ax, BIT_0_OFF           ; Set for VGA pass-through mode.
        mov     Gra_mode_ctl_sh, ax
        Out_Port        ADVFUNC_CNTL, ax;

        ret
enableVGA       ENDP


close8514hw     proc    far

        ; Re-enables VGA pass-through.

        cmp     adexboard, 0
        je      close_notAdex   ; Don't call enableVGA if not Adex, JCO 4/4/92
        call    enableVGA               ; This seems to transfer the dac from
                                        ;  the VGA to the 8514/A and then enable
                                        ;  VGA mode!!!

        mov     si, offset mode1024
        mov     dx, WD_ESCAPE_REG       ; program WD
        in      al, dx

        mov     dx, WD_ENHANCED_MODE_REG
        outsw

close_notAdex:
        cmp     ati_enhance_mode, 0
        je      close_notati
        mov     ax, 0   ; load defaults into shadow sets
        call    dword ptr [loadset]
        mov     ax, 0   ; set VGA passthrough
        call    dword ptr [setmode]

close_notati:
        mov     dx, ADVFUNC_CNTL
        mov     ax, 6
        out     dx, ax                  ; enable VGA

        ret
close8514hw     endp


; This routine sets 320x200x256 VGA mode and then loads dacbox, JCO 4/11/92
load8514dacbox proc near uses es

        mov     ax,13h                  ; switch to 320x200x256 mode
        int     10h
        push    ds                      ;  ...
        pop     es                      ;  ...
        mov     ax,1017h                ; get the old DAC values
        mov     bx,0                    ;  (assuming, of course, they exist)
        mov     cx,256          ;  ...
        mov     dx,offset dacbox        ;  ...
        int     10h                     ; do it.
        ret

load8514dacbox  endp

END
