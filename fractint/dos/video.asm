;       Generic assembler routines having to do with video adapter
;
; ---- Video Routines
;
;       setvideomode()
;       setvideotext()
;       getcolor()
;       putcolor_a()
;       gettruecolor()
;       puttruecolor()
;       rgb_to_dac()
;       dac_to_rgb()
;       out_line()
;       drawbox()
;       home()
;       movecursor()
;       keycursor()
;       putstring()
;       setattr()
;       scrollup()
;       scrolldown()
;       putstr()
;       loaddac()
;       spindac()
;       adapter_init
;       adapter_detect
;       setnullvideo
;       scroll_center()
;       scroll_relative()
;       scroll_state()
;
; ---- Help (Video) Support
;
;       setfortext()
;       setforgraphics()
;       setclear()
;       findfont()
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

        extrn   startvideo:far          ; start your-own-video routine
        extrn   readvideo:far           ; read  your-own-video routine
        extrn   writevideo:far          ; write your-own-video routine
        extrn   endvideo:far            ; end   your-own-video routine
        extrn   readvideopalette:far    ; read-your-own-palette routine
        extrn   writevideopalette:far   ; write-your-own-palette routine

        extrn   startdisk:far           ; start disk-video routine
        extrn   readdisk:far            ; read  disk-video routine
        extrn   writedisk:far           ; write disk-video routine
        extrn   enddisk:far             ; end   disk-video routine

        extrn   buzzer:far              ; nyaah, nyaah message

        extrn   getakey:far             ; for keycursor routine
        extrn   keypressed:far          ;  ...

; TARGA 28 May 80 - j mclain
        extrn   StartTGA  :far          ; start TARGA
        extrn   ReadTGA   :far          ; read  TARGA
        extrn   WriteTGA  :far          ; write TARGA
        extrn   EndTGA    :far          ; end   TARGA
        extrn   ReopenTGA :far          ; restart TARGA

; TARGA+ Mark Peterson 2-12-91
        extrn   MatchTPlusMode:far
        extrn   CheckForTPlus:far
        extrn   WriteTPlusBankedPixel:far
        extrn   ReadTPlusBankedPixel:far
        extrn   TPlusLUT:far

; 8514/A routines               ; changed 8514/A routines to near JCO 4/11/92
;   And back to far, JCO 4/13/97
        extrn   open8514  :far     ; start 8514a
        extrn   reopen8514:far     ; restart 8514a
        extrn   close8514 :far     ; stop 8514a
        extrn   fr85wdot  :far     ; 8514a write dot
        extrn   fr85wbox  :far     ; 8514a write box
        extrn   fr85rdot  :far     ; 8514a read dot
        extrn   fr85rbox  :far     ; 8514a read box
        extrn   w8514pal  :far     ; 8514a pallete update

; HW Compatible 8514/A routines    ; AW, made near JCO 4/11/92
;   And back to far, JCO 4/13/97
        extrn   open8514hw  :far      ; start 8514a
        extrn   reopen8514hw:far      ; restart 8514a
        extrn   close8514hw :far      ; stop 8514a
        extrn   fr85hwwdot  :far      ; 8514a write dot
        extrn   fr85hwwbox  :far      ; 8514a write box
        extrn   fr85hwrdot  :far      ; 8514a read dot
        extrn   fr85hwrbox  :far      ; 8514a read box
        extrn   w8514hwpal  :far      ; 8514a pallete update

; Hercules Routines
        extrn   inithgc   :far      ; Initialize Hercules card graphics mode
        extrn   termhgc   :far      ; Terminate Hercules card graphics mode
        extrn   writehgc  :far      ; Hercules write dot
        extrn   readhgc   :far      ; Hercules read dot

; setforgraphics/setfortext textsafe=save
        extrn   savegraphics    :far
        extrn   restoregraphics :far

.DATA

; ************************ External variables *****************************

        extrn   oktoprint: word         ; flag: == 1 if printf() will work
        extrn   videoentry:byte         ; video table entry flag
        extrn   dotmode: word           ; video mode (see the comments
                                        ; in front of the internal video
                                        ; table for legal dot modes)
        extrn   textsafe2: word         ; textsafe over-ride from videotable

        extrn   sxdots:word,sydots:word ; physical screen number of dots
        extrn   sxoffs:word,syoffs:word ; logical screen top left
        extrn   colors:word             ; colors
        extrn   cyclelimit:word         ; limiting factor for DAC-cycler
        extrn   debugflag:word          ; for debugging purposes only

        extrn   boxcount:word           ; (previous) box pt counter: 0 if none.

        extrn   cpu:word                ; CPU type (86, 186, 286, or 386)
        extrn   extraseg:word           ; location of the EXTRA segment

        extrn   suffix:word             ; (safe place during video-mode switches)

        extrn   swaplength:word         ; savegraphics/restoregraphics stuff
        extrn   swapoffset:dword        ; ...
        extrn   swapvidbuf:dword        ; ...
        extrn   swaptotlen:dword        ; ...

        extrn   rotate_lo:word, rotate_hi:word

        extrn   bios_palette:word
        extrn   paldata:byte
        extrn   realcoloriter:dword
        extrn   coloriter:dword
        extrn   truemode:word

        extrn   xdots:word
        extrn   ydots:word
        extrn   colors:word
        extrn   NonInterlaced:word
        extrn   PixelZoom:word
        extrn   MaxColorRes:word
        extrn   TPlusFlag:WORD          ; TARGA+ Mark Peterson 2-12-91
        extrn   ai_8514:byte            ;flag for 8514a afi JCO 4/11/92

; ************************ Public variables *****************************

public          andcolor                ; used by 'calcmand'
public          videotable
public          loadPalette             ; flag for loading VGA/TARGA palette from disk
public          dacbox                  ; GIF saves use this
public          daclearn, daccount      ; Rotate may want to use this
public          rowcount                ; row-counter for decoder and out_line
public          gotrealdac              ; loaddac worked, really got a dac
public          reallyega               ; "really an EGA" (faking a VGA) flag
public          diskflag                ; disk video active flag
public          video_type              ; video adapter type
public          svga_type               ; SuperVGA video adapter type
public          mode7text               ; for egamono and hgc
public          textaddr                ; text segment
public          textsafe                ; setfortext/setforgraphics logic
public          goodmode                ; video mode ok?
public          text_type               ; current mode's type of text
public          textrow                 ; current row in text mode
public          textcol                 ; current column in text mode
public          textrbase               ; textrow is relative to this
public          textcbase               ; textcol is relative to this

public          color_dark              ; darkest color in palette
public          color_bright            ; brightest color in palette
public          color_medium            ; nearest to medbright grey in palette

public          swapsetup               ; for savegraphics/restoregraphics

public          TPlusInstalled

public          vesa_detect             ; set to 0 to disable VESA-detection
public          vesa_xres               ; real screen width
public          vesa_yres               ; real screen height

public          vxdots                  ; virtual scan line length
public          video_scroll            ; is-scrolling-on? flag
public          video_startx            ; scrolled horizontaly this far
public          video_starty            ; scrolled verticaly this far
public          video_vram              ; VRAM size
public          virtual                 ; enable/disable virtual screen mode
public          chkd_vvs                ; we've run VESAvirtscan once

public          istruecolor             ; 1 if VESA truecolor mode, 0 otherwise

;               arrays declared here, used elsewhere
;               arrays not used simultaneously are deliberately overlapped

; ************************ Internal variables *****************************

vxdots          dw      0               ; virtual scan line length (bytes)
video_scroll    dw      0               ; is-scrolling-on? flag
video_vram      dw      0               ; VRAM size
virtual         dw      1               ; enable/disable virtual screen mode

video_startx    dw      0               ; scrolled horizontaly this far
video_starty    dw      0               ; scrolled verticaly this far
video_cofs_x    dw      0               ; half of the physical screen width
video_cofs_y    dw      0               ; half of the physical screen height
video_slim_x    dw      0               ; left-col limit for scrolling right
video_slim_y    dw      0               ; top-line limit for scrolling down
scroll_savex    dw      0               ; for restoring screen center position
scroll_savey    dw      0               ; when unstacking the scrolled screen
wait_retrace    dw      0               ; wait for retrace (80h) or not (00h)?

goodmode        dw      0               ; if non-zero, OK to read/write pixels
dotwrite        dw      0               ; write-a-dot routine:  mode-specific
dotread         dw      0               ; read-a-dot routine:   mode-specific
linewrite       dw      0               ; write-a-line routine: mode-specific
lineread        dw      0               ; read-a-line routine: mode-specific
swapsetup       dd      0               ; setfortext/graphics setup routine
andcolor        dw      0               ; "and" value used for color selection
color           db      0               ; the color to set a pixel
videoflag       db      0               ; special "your-own-video" flag
tgaflag         db      0               ; TARGA 28 May 89 - j mclain
loadPalette     db      0               ; TARGA/VGA load palette from disk

f85flag         db      0               ;flag for 8514a

HGCflag         db      0               ;flag for Hercules Graphics Adapter

TPlusInstalled  dw      0

xga_pos_base    dw      0               ; MCA Pos Base value
xga_cardid      dw      0               ; MCA Card ID value
xga_reg_base    dw      -1              ; XGA IO Reg Base (-1 means dunno yet)
xga_1mb         dd      0               ; XGA 1MB aperture address
xga_4mb         dd      0               ; XGA 4MB aperture address
xga_result      dw      0               ; XGA_detect result code
xga_isinmode    dw      0               ; XGA is in this mode right now
xga_iscolors    dw      0               ; XGA using this many colors (0=64K)
xga_clearvideo  db      0               ; set to 80h to prevent video-clearing
xga_loaddac     db      0               ; set to 1 to load 'dacbox' on modesw
xga_xdots       dw      0               ; pixels per scan line

                align   2
tmpbufptr       dd      0
color_dark      dw      0               ; darkest color in palette
color_bright    dw      0               ; brightest color in palette
color_medium    dw      0               ; nearest to medbright grey in palette
;                                       ; Zoom-Box values (2K x 2K screens max)
reallyega       dw      0               ; 1 if its an EGA posing as a VGA
gotrealdac      dw      0               ; 1 if loaddac has a dacbox
diskflag        dw      0               ; special "disk-video" flag
palettega       db      17 dup(0)       ; EGA palette registers go here
dacnorm         db      0               ; 0 if "normal" DAC update
daclearn        dw      0               ; 0 if "learning" DAC speed
daccount        dw      0               ; DAC registers to update in 1 pass
dacbox          db      773 dup(0)      ; DAC goes here
;;saved_dacreg  dw      0ffffh,0,0,0    ; saved DAC register goes here

orvideo         db      0               ; "or" value for setvideo
                align   2
rowcount        dw      0               ; row-counter for decoder and out_line

videomem        dw      0a000h          ; VGA videomemory
videoax         dw      0               ; graphics mode values: ax
videobx         dw      0               ; graphics mode values: bx
videocx         dw      0               ; graphics mode values: cx
videodx         dw      0               ; graphics mode values: dx

video_type      dw      0               ; actual video adapter type:
                                        ;   0  = type not yet determined
                                        ;   1  = Hercules
                                        ;   2  = CGA (assumed if nothing else)
                                        ;   3  = EGA
                                        ;   4  = MCGA
                                        ;   5  = VGA
                                        ;   6  = VESA (not yet checked)
                                        ;  11  = 8514/A (not yet checked)
                                        ;  12  = TIGA   (not yet checked)
                                        ;  13  = TARGA  (not yet checked)
svga_type       dw      0               ;  (forced) SVGA type
                                        ;   1 = ahead "A" type
                                        ;   2 = ATI
                                        ;   3 = C&T
                                        ;   4 = Everex
                                        ;   5 = Genoa
                                        ;   6 = Ncr
                                        ;   7 = Oak-Tech
                                        ;   8 = Paradise
                                        ;   9 = Trident
                                        ;  10 = Tseng 3000
                                        ;  11 = Tseng 4000
                                        ;  12 = Video-7
                                        ;  13 = ahead "B" type
                                        ;  14 = "null" type (for testing only)
mode7text       dw      0               ; nonzero for egamono and hgc
textaddr        dw      0b800h          ; b800 for mode 3, b000 for mode 7
textsafe        dw      0               ; 0 = default, runup chgs to 1
                                        ; 1 = yes
                                        ; 2 = no, use 640x200
                                        ; 3 = bios, yes plus use int 10h-1Ch
                                        ; 4 = save, save entire image
text_type       dw      0               ; current mode's type of text:
                                        ;   0  = real text, mode 3 (or 7)
                                        ;   1  = 640x200x2, mode 6
                                        ;   2  = some other mode, graphics
video_entries   dw      0               ; offset into video_entries table
video_bankadr   dw      0               ; offset  of  video_banking routine
video_bankseg   dw      0               ; segment of  video_banking routine

textrow         dw      0               ; for putstring(-1,...)
textcol         dw      0               ; for putstring(..,-1,...)
textrbase       dw      0               ; textrow is relative to this
textcbase       dw      0               ; textcol is relative to this
cursortyp       dw      0

tandyseg        dw      ?               ;Tandy 1000 video segment address
tandyofs        dw      ?               ;Tandy 1000 Offset into video buffer
tandyscan       dw      ?               ;Tandy 1000 scan line address pointer


; ******************* "Tweaked" VGA mode variables ************************

                                                ; 704 x 528 mode
x704y528        db      704/8                   ; number of screen columns
                db      528/16                  ; number of screen rows
                db       68h, 57h, 58h, 8Bh     ; CRTC Registers
                db       59h, 86h, 3EH,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 3Dh
                db       19h, 8Bh, 0Fh, 2Ch
                db        0h, 18h, 38h,0E3h
                db      0FFh
                                                ; 720 x 540 mode
x720y540        db      720/8                   ; number of screen columns
                db      540/16                  ; number of screen rows
                db       6Ah, 59h, 5Ah, 8Dh     ; CRTC Registers
                db       5Eh, 8Bh, 4AH,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 49h
                db       24h, 86h, 1Bh, 2Dh
                db        0h, 24h, 44h,0E3h
                db      0FFh
                                                ; 736 x 552 mode
x736y552        db      736/8                   ; number of screen columns
                db      552/16                  ; number of screen rows
                db       6Ch, 5Bh, 5Ch, 8Fh     ; CRTC Registers
                db       5Fh, 8Ch, 56H,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 55h
                db       2Bh, 8Dh, 27h, 2Eh
                db        0h, 30h, 50h,0E3h
                db      0FFh
                                                ; 752 x 564 mode
x752y564        db      752/8                   ; number of screen columns
                db      564/16                  ; number of screen rows
                db       6Eh, 5Dh, 5Eh, 91h     ; CRTC Registers
                db       62h, 8Fh, 62H,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 61h
                db       37h, 89h, 33h, 2Fh
                db        0h, 3Ch, 5Ch,0E3h
                db      0FFh
                                                ; 768 x 576 mode
x768y576        db      768/8                   ; number of screen columns
                db      576/16                  ; number of screen rows
                db       70h, 5Fh, 60h, 93h     ; CRTC Registers
                db       66h, 93h, 6EH,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 6Dh
                db       43h, 85h, 3Fh, 30h
                db        0h, 48h, 68h,0E3h
                db      0FFh
                                                ; 784 x 588 mode
x784y588        db      784/8                   ; number of screen columns
                db      588/16                  ; number of screen rows
                db       72h, 61h, 62h, 95h     ; CRTC Registers
                db       69h, 96h, 7AH,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 79h
                db       4Fh, 81h, 4Bh, 31h
                db        0h, 54h, 74h,0E3h
                db      0FFh
                                                ; 800 x 600 mode
x800y600        db      800/8                   ; number of screen columns
                db      600/16                  ; number of screen rows
                db       74h, 63h, 64h, 97h     ; CRTC Registers
                db       68h, 95h, 86H,0F0h
                db        0h, 60h,  0h,  0h
                db        0h,  0h,  2h, 85h
                db       5Bh, 8Dh, 57h, 32h
                db        0h, 60h, 80h,0E3h
                db      0FFh

x360y480        db      360/8                   ; number of screen columns
                db      480/16                  ; number of screen rows
                db       6bh, 59h, 5ah, 8eh     ; CRTC Registers
                db       5eh, 8ah, 0DH,03Eh
                db        0h, 40h, 00h,  0h
                db        0h,  0h,  0h, 31h
                db      0EAh, 0ACh, 0DFh, 2Dh
                db        0h,0E7h, 06h,0E3h
                db      0FFh

x320y480        db      320/8                   ; number of screen columns
                db      480/16                  ; number of screen rows
                db       5fh, 4fh, 50h, 82h     ; CRTC Registers
                db       54h, 80h, 0DH,03Eh
                db        0h, 40h, 00h,  0h
                db        0h,  0h,  0h,  0h
                db      0EAh, 0AEh, 0DFh, 28h
                db        0h,0E7h, 006h,0E3h
                db      0FFh

; mode x from Michael Abrash
x320y240        db      320/8                   ; number of screen columns
                db      240/16                  ; number of screen rows
                db      05fh, 04fh, 050h, 082h
                db      054h, 080h, 0dh, 03eh
                db      00h, 041h, 00h, 00h
                db      00h, 00h, 00h, 00h
                db      0eah, 0ach, 0dfh, 028h
                db      00h, 0e7h, 06h, 0e3h
                db       0ffh

x320y400        db      320/8                   ; number of screen columns
                db      400/16                  ; number of screen rows
                db       5fh, 4fh, 50h, 82h     ; CRTC Registers
                db       54h, 80h,0bfh, 1fh
                db       00h, 40h, 00h, 00h
                db       00h, 00h, 00h, 00h
                db       9ch, 8eh, 8fh, 28h
                db       00h, 96h,0b9h,0E3h
                db      0FFh

x640y400        db      640/8                   ; number of screen columns
                db      400/16                  ; number of screen rows
                db       5eh, 4fh, 50h, 01h     ; CRTC Registers
                db       54h, 9fh,0c0h, 1fh
                db       00h, 40h, 00h, 00h
                db       00h, 00h, 00h, 00h
                db       9ch,08eh, 8fh, 28h
                db       00h, 95h,0bch,0c3h
                db       0ffh
;for VGA
x400y600        db      400/8
                db      600/16
                db      74h,63h,64h,97h
                db      68h,95h,86h,0F0h
                db      00h,60h,00h,00h
                db      00h,00h,00h,31h
                db      5Bh,8Dh,57h,32h
                db      0h,60h,80h,0E3h
                db      0FFh
;for VGA
x376y564        db      376/8
                db      564/16
                db      6eh,5dh,5eh,91h
                db      62h,8fh,62h,0F0h
                db      00h,60h,00h,00h
                db      00h,00h,00h,31h
                db      37h,89h,33h,2fh
                db      0h,3ch,5ch,0E3h
                db      0FFh
;for VGA
x400y564        db      400/8
                db      564/16
                db      74h,63h,64h,97h
                db      68h,95h,62h,0F0h
                db      00h,60h,00h,00h
                db      00h,00h,00h,31h
                db      37h,89h,33h,32h
                db      0h,3ch,5ch,0E3h
                db      0FFh

testati         db      832/8
                db      612/16
                db      7dh,65h,68h,9fh
                db      69h,92h,44h,1Fh
                db      00h,00h,00h,00h
                db      00h,00h,00h,00h
                db      34h,86h,37h,34h
                db      0fh,34h,40h,0E7h
                db      0FFh

                align   2

tweaks          dw      offset x704y528         ; tweak table
                dw      offset x704y528
                dw      offset x720y540
                dw      offset x736y552
                dw      offset x752y564
                dw      offset x768y576
                dw      offset x784y588
                dw      offset x800y600
                dw      offset x360y480
                dw      offset x320y400
                dw      offset x640y400         ; Tseng Super VGA
                dw      offset x400y600         ; new tweak (VGA)
                dw      offset x376y564         ; new tweak (VGA)
                dw      offset x400y564         ; new tweak (VGA)
                dw      offset x720y540         ; ATI Tweak
                dw      offset x736y552         ; ATI Tweak
                dw      offset x752y564         ; ATI Tweak
                dw      offset testati          ; ATI 832x816 (works!)
                dw      offset x320y480
                dw      offset x320y240

tweakflag       dw      0                       ; tweak mode active flag
tweaktype       dw      0                       ; 8 or 9 (320x400 or 360x480)

bios_vidsave    dw      0                       ; for setfortext/graphics
setting_text    dw      0                       ; flag for setting text mode
chkd_vvs        dw      0                       ; we checked VESAvirtscan

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


;                       Video Table Entries
;
;       The Video Table has been moved to a FARDATA segment to relieve
;       some of the pressure on the poor little overloaded 64K DATA segment.

.code

video_requirements      dw      0               ; minimal video_type req'd
        dw      1, 3, 4, 5, 5, 5, 5, 5, 1, 1    ; dotmodes  1 - 10
        dw      1, 5, 2, 1, 5, 5, 5, 5, 1, 5    ; dotmodes 11 - 20
        dw      5, 5, 5, 5, 5, 5, 5, 5, 5, 5    ; dotmodes 21 - 30

videotable      label   byte    ; video table actually starts on the NEXT byte

;       Feel free to add your favorite video adapter to FRACTINT.CFG.
;       The entries hard coded here are repeated from fractint.cfg in case
;       it gets lost/destroyed, so a user can still have some modes.

;       Currently available Video Modes are (use the BIOS as a last resort)
;               1) use the BIOS (INT 10H, AH=12/13, AL=color) ((SLOW))
;               2) pretend it's a (perhaps super-res) EGA/VGA
;               3) pretend it's an MCGA
;               4) SuperVGA 256-Color mode using the Tseng Labs Chipset
;               5) SuperVGA 256-Color mode using the Paradise Chipset
;               6) SuperVGA 256-Color mode using the Video-7 Chipset
;               7) Non-Standard IBM VGA 360 x 480 x 256-Color mode
;               8) SuperVGA 1024x768x16 mode for the Everex Chipset
;               9) TARGA video modes
;               10) HERCULES video mode
;               11) Non-Video [disk or RAM] "video"
;               12) 8514/A video modes
;               13) CGA 320x200x4-color and 640x200x2-color modes
;               14) Tandy 1000 video modes
;               15) SuperVGA 256-Color mode using the Trident Chipset
;               16) SuperVGA 256-Color mode using the Chips & Tech Chipset
;               17) SuperVGA 256-Color mode using the ATI VGA Wonder Chipset
;               18) SuperVGA 256-Color mode using the Everex Chipset
;               19) Roll-Your-Own video, as defined in YOURVID.C
;               20) SuperVGA 1024x768x16 mode for the ATI VGA Wonder Chipset
;               21) SuperVGA 1024x768x16 mode for the Tseng Labs Chipset
;               22) SuperVGA 1024x768x16 mode for the Trident Chipset
;               23) SuperVGA 1024x768x16 mode for the Video 7 Chipset
;               24) SuperVGA 1024x768x16 mode for the Paradise Chipset
;               25) SuperVGA 1024x768x16 mode for the Chips & Tech Chipset
;               26) SuperVGA 1024x768x16 mode for the Everex Chipset
;               27) SuperVGA Auto-Detect mode
;               28) VESA modes
;               29) True Color Auto-Detect

;       (Several entries have been commented out - they should/did work,
;       but are handled by alternative entries.  Where multiple SuperVGA
;       entries are covered by a single SuperVGA Autodetect mode, the
;       individual modes have been commented out.  Where a SuperVGA
;       Autodetect mode covers only one brand of adapter, the Autodetect
;       mode has been commented out to avoid confusion.)

;               |--Adapter/Mode-Name------|-------Comments-----------|

;               |------INT 10H------|Dot-|--Resolution---|
;           |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|

        db      "IBM 16-Color EGA         ",0,"Standard EGA hi-res mode ",0
        dw 1060,  10h,   0,   0,   0,   2, 640, 350,  16
        db      "IBM 256-Color VGA/MCGA   ",0,"Quick and LOTS of colors ",0
        dw 1061,  13h,   0,   0,   0,   3, 320, 200, 256
        db      "IBM 16-Color VGA         ",0,"Nice high resolution     ",0
        dw 1062,  12h,   0,   0,   0,   2, 640, 480,  16
        db      "IBM 4-Color CGA          ",0,"(Ugh - Yuck - Bleah)     ",0
        dw 1063,   4h,   0,   0,   0,  13, 320, 200,   4
        db      "IBM Hi-Rez B&W CGA       ",0,"('Hi-Rez' Ugh - Yuck)    ",0
        dw 1064,   6h,   0,   0,   0,  13, 640, 200,   2
        db      "IBM B&W EGA              ",0,"(Monochrome EGA)         ",0
        dw 1065,  0fh,   0,   0,   0,   2, 640, 350,   2
        db      "IBM B&W VGA              ",0,"(Monochrome VGA)         ",0
        dw 1066,  11h,   0,   0,   0,   2, 640, 480,   2
        db      "IBM Low-Rez EGA          ",0,"Quick but chunky         ",0
        dw 1067,  0dh,   0,   0,   0,   2, 320, 200,  16
        db      "IBM VGA (non-std)        ",0,"Register Compatibles ONLY",0
        dw 1068,   0h,   0,   0,   9,   7, 320, 400, 256
        db      "IBM VGA (non-std)        ",0,"Register Compatibles ONLY",0
        dw 1084,   0h,   0,   0,   8,   7, 360, 480, 256
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1085,    0,   0,   0,   0,  27, 800, 600,  16
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1086,    0,   0,   0,   0,  27,1024, 768,  16
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1087,    0,   0,   0,   0,  27, 640, 400, 256
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1088,    0,   0,   0,   0,  27, 640, 480, 256
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1089,    0,   0,   0,   0,  27, 800, 600, 256
        db      "SuperVGA/VESA Autodetect ",0,"Works with most SuperVGA ",0
        dw 1090,    0,   0,   0,   0,  27,1024, 768, 256
        db      "VESA Standard interface  ",0,"OK: Andy Fu - Chips&Tech ",0
        dw 1091,4f02h,106h,   0,   0,  28,1280,1024,  16
        db      "VESA Standard interface  ",0,"OK: Andy Fu - Chips&Tech ",0
        dw 1092,4f02h,107h,   0,   0,  28,1280,1024, 256
        db      "8514/A Low  Res          ",0,"HW/AI (AI Reqs HDILOAD)  ",0
        dw 1093,   3h,   0,   0,   1,  12, 640, 480, 256
        db      "8514/A High Res          ",0,"HW/AI (AI Reqs HDILOAD)  ",0
        dw 1094,   3h,   0,   0,   1,  12,1024, 768, 256
        db      "8514/A Low  W/Border     ",0,"HW/AI (AI Reqs HDILOAD)  ",0
        dw 1095,   3h,   0,   0,   1,  12, 632, 474, 256
        db      "8514/A High W/Border     ",0,"HW/AI (AI Reqs HDILOAD)  ",0
        dw 1096,   3h,   0,   0,   1,  12,1016, 762, 256
        db      "IBM Med-Rez EGA          ",0,"(Silly but it's there!)  ",0
        dw 1097,  0eh,   0,   0,   0,   2, 640, 200,  16
        db      "IBM VGA (non-std)        ",0,"Register Compatibles ONLY",0
        dw 1098,   0h,   0,   0,  18,   7, 320, 480, 256
        db      "Hercules Graphics        ",0,"OK: Timothy Wegner       ",0
        dw 1099,   8h,   0,   0,   0,  10, 720, 348,   2
        db      "Tandy 1000               ",0,"OK: Joseph Albrecht      ",0
        dw 1100,   9h,   0,   0,   0,  14, 320, 200,  16
        db      "Pdise/AST/COMPAQ VGA     ",0,"OK: Phil Wilson          ",0
        dw 1101,  59h,   0,   0,   0,   1, 800, 600,   2
        db      140     dup(0)  ; 2 unused slots here default table
        db      "Disk/RAM 'Video'         ",0,"Full-Page L-Jet @  75DPI ",0
        dw 1104,   3h,   0,   0,   0,  11, 800, 600,   2
        db      "Disk/RAM 'Video'         ",0,"Full-Page L-Jet @ 150DPI ",0
        dw 1105,   3h,   0,   0,   0,  11,1600,1200,   2
        db      "Disk/RAM 'Video'         ",0,"Full-Page Epson @ 120DPI ",0
        dw 1106,   3h,   0,   0,   0,  11, 768, 960,   2
        db      "Disk/RAM 'Video'         ",0,"Full-Page Paintjet 90DPI ",0
        dw 1107,   3h,   0,   0,   0,  11, 960, 720, 256
        db      "Disk/RAM 'Video'         ",0,"For Background Fractals  ",0
        dw 1108,   3h,   0,   0,   0,  11, 800, 600, 256
        db      "Disk/RAM 'Video'         ",0,"For Background Fractals  ",0
        dw 1109,   3h,   0,   0,   0,  11,2048,2048, 256
        db      280     dup(0)  ; 4 unused slots here default table
        db       70     dup(0)  ; 1 slot reserved for unassigned current mode


bios_savebuf db 256 dup(0)  ; enough for 4 blocks (64 bytes/block)

.code

;               XGA Graphics mode setup values
;               (the first two entries in each line
;               indicate where the table values are to be stored)
;
;               1024x768x256 vvv
;               1024x768x16  -----vvvv
;               640x480x256  -----------vvvv
;               640x480x65536 ----------------vvvv
;               800x600x16   -----------------------vvvv
;               800x600x256  -----------------------------vvvv
;               800x600x65536 ----------------------------------vvvv

xga_twidth dw   9                                       ; width of these tables

xga_requir dw      0,    0,  0dh,  05h,  01h,  09h,  01h,  01h,  09h    ; adapter requirements
xga_colors dw      0,    0,  256,   16,  256,    0,   16,  256,    0    ; 0 means 64K colors
xga_swidth dw      0,    0, 1024,  512,  640, 1280,  400,  800, 1600    ; bytes / scan line

xga_val db      004h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; interrupt enable
        db      005h, 000h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh    ; interrupt status
        db      000h, 000h, 004h, 004h, 004h, 004h, 004h, 004h, 004h    ; operating mode
        db      00ah, 064h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; palette mask
        db      001h, 000h, 001h, 001h, 001h, 001h, 001h, 001h, 001h    ; vid mem aper cntl
        db      008h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; vid mem aper indx
        db      006h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; virt mem ctl
        db      009h, 000h, 003h, 002h, 003h, 004h, 002h, 003h, 004h    ; mem access mode
        db      00ah, 050h, 001h, 001h, 001h, 001h, 001h, 001h, 001h    ; disp mode 1
        db      00ah, 050h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; disp mode 1
        db      00ah, 010h, 09dh, 09dh, 063h, 063h, 088h, 088h, 088h    ; horiz tot lo.
        db      00ah, 011h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; horiz tot hi.
        db      00ah, 012h, 07fh, 07fh, 04fh, 04fh, 063h, 063h, 063h    ; hor disp end lo
        db      00ah, 013h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; hor disp end hi
        db      00ah, 014h, 080h, 080h, 050h, 050h, 064h, 064h, 064h    ; hor blank start lo
        db      00ah, 015h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; hor blank start hi
        db      00ah, 016h, 09ch, 09ch, 062h, 062h, 087h, 087h, 087h    ; hor blank end lo
        db      00ah, 017h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; hor blank end hi
        db      00ah, 018h, 087h, 087h, 055h, 055h, 06ah, 06ah, 06ah    ; hor sync start lo
        db      00ah, 019h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; hor sync start hi
        db      00ah, 01ah, 09ch, 09ch, 061h, 061h, 084h, 084h, 084h    ; hor sync end lo
        db      00ah, 01bh, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; hor sync end hi
        db      00ah, 01ch, 040h, 040h, 000h, 000h, 000h, 000h, 000h    ; hor sync pos
        db      00ah, 01eh, 004h, 004h, 000h, 000h, 000h, 000h, 000h    ; hor sync pos
        db      00ah, 020h, 030h, 030h, 00ch, 00ch, 086h, 086h, 086h    ; vert tot lo
        db      00ah, 021h, 003h, 003h, 002h, 002h, 002h, 002h, 002h    ; vert tot hi
        db      00ah, 022h, 0ffh, 0ffh, 0dfh, 0dfh, 057h, 057h, 057h    ; vert disp end lo
        db      00ah, 023h, 002h, 002h, 001h, 001h, 002h, 002h, 002h    ; vert disp end hi
        db      00ah, 024h, 000h, 000h, 0e0h, 0e0h, 058h, 058h, 058h    ; vert blank start lo
        db      00ah, 025h, 003h, 003h, 001h, 001h, 002h, 002h, 002h    ; vert blank start hi
        db      00ah, 026h, 02fh, 02fh, 00bh, 00bh, 085h, 085h, 085h    ; vert blank end lo
        db      00ah, 027h, 003h, 003h, 002h, 002h, 002h, 002h, 002h    ; vert blank end hi
        db      00ah, 028h, 000h, 000h, 0eah, 0eah, 058h, 058h, 058h    ; vert sync start lo
        db      00ah, 029h, 003h, 003h, 001h, 001h, 002h, 002h, 002h    ; vert sync start hi
        db      00ah, 02ah, 008h, 008h, 0ech, 0ech, 06eh, 06eh, 06eh    ; vert sync end
        db      00ah, 02ch, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh    ; vert line comp lo
        db      00ah, 02dh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh    ; vert line comp hi
        db      00ah, 036h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; sprite cntl
        db      00ah, 040h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; start addr lo
        db      00ah, 041h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; start addr me
        db      00ah, 042h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; start addr hi
        db      00ah, 043h, 080h, 040h, 050h, 0a0h, 032h, 064h, 0c8h    ; pixel map width lo
        db      00ah, 044h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; pixel map width hi
        db      00ah, 054h, 00dh, 00dh, 000h, 000h, 001h, 001h, 001h    ; clock sel
        db      00ah, 051h, 003h, 002h, 003h, 004h, 002h, 003h, 004h    ; display mode 2
        db      00ah, 070h, 000h, 000h, 000h, 000h, 080h, 080h, 080h    ; ext clock sel
        db      00ah, 050h, 00fh, 00fh, 0c7h, 0c7h, 007h, 007h, 007h    ; display mode 1
        db      00ah, 055h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; Border Color
        db      00ah, 060h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; Sprite Pal Lo
        db      00ah, 061h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; Sprite Pal hi
        db      00ah, 062h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; Sprite Pre Lo
        db      00ah, 063h, 000h, 000h, 000h, 000h, 000h, 000h, 000h    ; Sprite Pre hi
        db      00ah, 064h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh    ; Palette Mask
        db      0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh    ; end of the list


xga_newbank     proc                    ; XGA-specific bank-switching routine
        cmp     xga_isinmode,2          ; are we in an XGA-specific mode?
        jl      return                  ;  nope.  bail out.
        mov     curbk,ax                ; save the new current bank value
        mov     dx,xga_reg_base         ; Select Page
        add     dx,08h
        out     dx,al                   ; assumes bank number is in al
return: ret
xga_newbank     endp

xga_16linewrite proc    near            ; 16-color Line Write
        mov     bx,ax                   ; calculate the # of columns
        sub     bx,cx
        mov     ax,xga_xdots            ; this many dots / line
        mul     dx                      ; times this many lines - ans in dx:ax
        push    cx                      ; save the X-value for a tad
        shr     cx,1                    ; and adjust for two bits per pixel
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; answer in dx:ax - dl=bank, ax=offset
        mov     di,ax                   ; save offset in DI
        pop     cx                      ; restore the X-value
        mov     ax,dx                   ; xga_newbank expects bank in al
new_bank:
        call    far ptr xga_newbank
same_bank:
        mov     ah,es:[di]              ; grab the old byte value
        mov     al,[si]                 ; and the new color value
        and     al,0fh                  ; isolate the bits we want
        test    cx,1                    ; odd pixel address?
        jnz     xga_sk1                 ;  yup
        and     ah,0f0h                 ; isolate the low-order
        jmp     short   xga_sk2
xga_sk1:and     ah,0fh                  ; isolate the high-order
        shl     al,1
        shl     al,1
        shl     al,1
        shl     al,1
xga_sk2:or      al,ah                   ; combine the two nibbles
        mov     es:[di],al              ; write the dot
        inc     si                      ; increment the source addr
        dec     bx                      ; more to go?
        jz      done                    ; nope
        inc     cx                      ; next pixel
        test    cx,1                    ; odd pixel?
        jnz     same_bank               ;  yup
        inc     di                      ; increment the destination
        cmp     di,0                    ; segment wrap?
        jnz     same_bank               ;  nope
        mov     ax,curbk                ; update the bank cvalue
        inc     ax
        jmp     new_bank
done:   ret
xga_16linewrite         endp

xga_super16addr proc near               ; can be put in-line but shared by
                                        ; read and write routines
        clc                             ; clear carry flag
        push    ax                      ; save this for a tad
        mov     ax,xga_xdots            ; this many dots / line
        mul     dx                      ; times this many lines - ans in dx:ax
        push    cx                      ; save the X-value for a tad
        shr     cx,1                    ; and adjust for two bits per pixel
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; answer in dx:ax - dl=bank, ax=offset
        pop     cx                      ; restore the X-value
        mov     bx,ax                   ; save this in BX
        cmp     dx,curbk                ; see if bank changed
        je      same_bank               ; jump if old bank ok
        mov     ax,dx                   ; xga_newbank expects bank in al
        call    far ptr xga_newbank
same_bank:
        pop     ax                      ; restore AX
        ret
xga_super16addr endp

xga_16write     proc near               ; XGA 256 colors write-a-dot
        call    xga_super16addr         ; calculate address and switch banks
        mov     ah,es:[bx]              ; grab the old byte value
        and     al,0fh                  ; isolate the bits we want
        test    cx,1                    ; odd pixel address?
        jnz     xga_sk1                 ;  yup
        and     ah,0f0h                 ; isolate the low-order
        jmp     short   xga_sk2
xga_sk1:and     ah,0fh                  ; isolate the high-order
        shl     al,1
        shl     al,1
        shl     al,1
        shl     al,1
xga_sk2:or      al,ah                   ; combine the two nibbles
        mov     es:[bx],al              ; write the dot
        ret                             ; we done.
xga_16write     endp

xga_16read      proc near               ; XGA 256 colors read-a-dot
        call    xga_super16addr         ; calculate address and switch banks
        mov     al,es:[bx]              ; read the dot
        test    cx,1                    ; odd number of pixels?
        jz      xga_sk1                 ;  nope
        shr     ax,1                    ; adjust for odd pixel count
        shr     ax,1
        shr     ax,1
        shr     ax,1
xga_sk1:and     ax,0fh                  ; isolate the byte value
        ret                             ; we done.
xga_16read      endp

xga_clear       proc    uses es si di   ; clear the XGA memory
        cmp     xga_clearvideo,0        ; should we really do this?
        jne     return                  ;  nope.  skip it.
        mov     bx,xga_result           ; find out how much memory we have
        and     bx,08h                  ;  in 64K pages
        add     bx,08h
        mov     ax,0a000h               ; set up to clear 0a0000-0affff
        push    ax
        pop     es
xloop:  mov     ax,bx                   ; initialize the bank addr
        call    xga_newbank
        mov     ax,0
        mov     cx,16384                ; clear out 32K
        mov     di,0
        rep     stosw
        mov     cx,16384                ; clear out 32K
        rep     stosw
        dec     bx                      ; another page?
        cmp     bx,0
        jge     xloop
return: ret
xga_clear       endp

xga_setpalette  proc    uses es si di, palette:word     ; set the XGA palette
        cmp     xga_isinmode,2          ; are we in an XGA graphics mode?
        jl      return                  ;  nope

        mov     dx,xga_reg_base         ; wait for a retrace
        add     dx,5
        mov     al,1                    ; clear the start-of-blanking
        out     dx,al
bloop:  in      al,dx
        test    al,01h                  ; blanking started?
        jz      bloop                   ;  nope - try again

        mov     dx,xga_reg_base         ; set up for a palette load
        add     dx,0ah
        mov     ax,0064h                ; make invisible
        out     dx,ax
        mov     ax,0055h                ; border color
        out     dx,ax
        mov     ax,0066h                ; palette mode
        out     dx,ax
        mov     ax,0060h                ; start at palette 0
        out     dx,ax
        mov     ax,0061h
        out     dx,ax

        mov     si,palette
        mov     cx,768
        mov     ax,065h                 ; palette update
        out     dx,al
        inc     dx                      ; palette data
.186
        rep     outsb
.8086
        dec     dx

        mov     ax,0ff64h               ; make visible
        out     dx,ax

return: ret
xga_setpalette  endp

xga_detect      proc    uses es di si

        cmp     xga_reg_base,-2         ; has the XGA detector already failed?
        jne     xga_sk1                 ; ne = not yet
        jmp     xga_notfound            ; e = yes, fail again
xga_sk1:cmp     xga_reg_base,-1         ; have we already found the XGA?
        je      xga_loc                 ; e = no
        jmp     xga_found               ; yes, process it

xga_loc:push    bp                      ; save around int 10H calls
        mov     ax,1f00h                ; XGA-2 detect:
        int     10h                     ;  get DMQS length
        pop     bp                      ; restore BP
        cmp     al,1fh                  ; did this work?
        jne     xga_man                 ;  nope - try the older, manual approach
        cmp     bx,768                  ; room for the results?
        ja      xga_man                 ;  no?!?  try the older approach
        mov     ax,1f01h                ; get DMQS info
        push    ds                      ;  into here
        pop     es                      ;  ...
        mov     di, offset dacbox       ;  ...
        int     10h                     ;  ...
        cmp     al,1fh                  ; safety first
        jne     xga_man                 ; ?? try the older approach
        mov     bx, word ptr dacbox+09h ; get the register base
        mov     xga_reg_base,bx         ; save the results
        mov     xga_result,1            ; say we found an adapter
        cmp     byte ptr dacbox+15h,4   ; do we have 1MB of adapter RAM?
        jb      @F                      ;  nope
        or      xga_result,8h           ;  yup - say so.
@@:     mov     bx, word ptr dacbox+13h ; get the composite monitor ID
        and     bx,0f00h                ; high-rez-monitor?
        cmp     bx,0f00h                ;  ..
        je      @F                      ;  nope
        or      xga_result,4            ;  yup
@@:     jmp     xga_found               ; say we found the adapter

xga_man:mov     ah,35h                  ; DOS get interrupt vector
        mov     al,15h                  ; Int 15h
        int     21h                     ; returns vector in es:bx
        mov     ax,es                   ; segment part
        or      ax,ax                   ; undefined vector?
        jnz     xga_sk2                 ; nz = no, OK so far
        jmp     xga_notfound            ; z = yes - not an MCA machine
xga_sk2:mov     dx,-1                   ; start with an invalid POS address
        mov     ax,0c400h               ; look for POS base address
        int     15h                     ;  (Microchannel machines only)
        jnc     xga_sk3                 ; nc = success
        jmp     xga_notfound            ; error - not an MC machine
xga_sk3:mov     xga_pos_base,dx         ; save pos_base_address
        xor     cx,cx                   ; check all MCA slots & motherboard
        cmp     dx,-1                   ; do we have a good POS?
        jne     xga_lp1                 ; ne = yes, proceed with MCA checks
        jmp     xga_notfound            ; no, fail
xga_lp1:cli                             ; no interrupts, please
        cmp     cx,0                    ; treat the motherboard differently?
        jne     xga_sk4                 ; ne = yes
        mov     al,0dfh                 ; enable the motherboard for setup
        mov     dx,94h
        out     dx,al
        jmp     short xga_sk5
xga_sk4:mov     ax,0c401h               ; enable an MCA slot for setup
        mov     bx,cx                   ;  this slot
        int     15h
xga_sk5:mov     dx,xga_pos_base         ; get pos record for the slot ID
        in      ax,dx
        mov     xga_cardid,ax
        add     dx,2                    ; compute IO Res Base
        in      al,dx                   ;  get POS data byte1
        mov     byte ptr xga_1mb,al     ;   save it temporarily
        inc     dx                      ; switch to byte 2
        in      al,dx                   ;  get POS data
        mov     byte ptr xga_1mb+1,al   ;   save it temporarily
        inc     dx                      ; switch to byte 3
        in      al,dx                   ;  get POS data
        mov     byte ptr xga_1mb+2,al   ;   save it temporarily
        inc     dx                      ; switch to byte 4
        in      al,dx                   ;  get POS data
        mov     byte ptr xga_1mb+3,al   ;   save it temporarily
        cmp     cx,0                    ; treat the motherboard differently
        jne     xga_sk6                 ; ne = yes
        mov     al,0ffh                 ; enable the motherboard for normal
        out     094h,al
        jmp     short xga_sk7
xga_sk6:mov     ax,0c402h               ; enable the MCA slot for normal
        mov     bx,cx                   ;  this slot
        int     15h
xga_sk7:sti                             ; interrupts on again

        mov     ax,xga_cardid           ; is an XGA adapter on this slot?
        cmp     ax,08fd8h
        jae     xga_sk8                 ; ae = yes
        jmp     xga_lp2                 ; try another slot
xga_sk8:cmp     ax,08fdbh               ; still within range?
        jbe     xga_sk9                 ; be = yes
        jmp     xga_lp2                 ; no, try another slot

xga_sk9:mov     al,byte ptr xga_1mb     ;  restore POS data byte 1
        and     ax,0eh                  ;  muck about with it to get reg base
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,2100h
        mov     xga_reg_base,ax
        mov     dx,xga_reg_base         ; is there a monitor on this slot?
        add     dx,0ah
        mov     al,052h
        out     dx,al
        mov     dx,xga_reg_base
        add     dx,0bh
        in      al,dx
        and     al,0fh
        cmp     al,00h                  ; illegal value, returned under Win 3.0
        je      xga_lp2
        cmp     al,0fh
        jne     xga_isthere             ; ne = yes

xga_lp2:inc     cx                      ; try another adapter?
        cmp     cx,9                    ; done all slots?
        ja      xga_ska                 ; a = yes
        jmp     xga_lp1                 ; no, try another slot

xga_ska:jmp     xga_notfound            ; forget it - no XGA here

xga_isthere:
        and     ax,06h                  ; strip off the low & high bit
        xor     ax,05h                  ; reverse the 3rd & low bits
        mov     xga_result,ax           ; save the result flag

        mov     dx,xga_reg_base         ; is this XGA in VGA mode?
        in      al,dx
        test    al,1
        jnz     xga_skb                 ; nz = yes - single-monitor setup
        or      xga_result,10h          ;  dual-monitor setup
xga_skb:

        mov     ah,byte ptr xga_1mb+2   ; retrieve POS data byte 3
        and     ax,0fe00h               ; eliminate the low-order bits
        mov     bl,byte ptr xga_1mb     ; retrieve POS data byte 1
        and     bx,0eh                  ; strip it down to the IODA
        mov     cx,5                    ; shift it up 5 bits
        shl     bx,cl
        or      ax,bx                   ; compute the 4MB aperture value
        mov     word ptr xga_4mb+2,ax   ; save the result

        mov     al, byte ptr xga_1mb+3  ; retrieve POS data byte 4
        and     ax,0fh                  ; select the 1MB aperture bits
        mov     cx,4                    ; shift it up 4 bits
        shl     ax,cl
        mov     word ptr xga_1mb+2,ax   ; save the result
        mov     ax,0
        mov     word ptr xga_1mb,ax

        mov     dx,xga_reg_base         ; Interrupt Disable
        add     dx,4
        xor     al,al
        out     dx,al

        mov     dx,xga_reg_base         ; Switch to Extended Mode
;;      add     dx,00h
        mov     al,4
        out     dx,al

        mov     dx,xga_reg_base         ; Aperture Control
        add     dx,01h
        mov     al,1
        out     dx,al

        mov     dx,xga_reg_base         ; disable Palette Mask
        add     dx,0ah
        mov     ax,0064h
        out     dx,ax

        mov     xga_isinmode,2          ; pretend we're already in graphics
        mov     al,12                   ; select page 12
        call    xga_newbank

        push    es                      ; see if this page has any memory
        mov     ax,0a000h
        push    ax
        pop     es
        mov     ah,000a5h
        mov     es:0,al
        mov     es:1,ah
        cmp     es:0,al
        jne     xga_512
        add     xga_result,8            ; 1MB RAM found
xga_512:pop     es

        mov     al,0                    ; select page 0
        call    xga_newbank
        mov     xga_isinmode,0          ; replace the "in-graphics" flag

        mov     dx,xga_reg_base         ; Palette Mask
        add     dx,0ah
        mov     ax,0ff64h
        out     dx,ax

        test    xga_result,10h          ; dual monitor setup?
        jnz     xga_found               ;  yup - don't restore as a VGA

        mov     dx,xga_reg_base         ; Switch to VGA Mode
;;      add     dx,00h
        mov     al,1
        out     dx,al

        mov     dx,03c3h                ; Enable VGA Address Code
        mov     al,1
        out     dx,al

        jmp     short   xga_found

xga_notfound:
        mov     xga_reg_base,-2         ; set failure flag
xga_found:
        mov     ax,xga_result           ; return the result
        ret

xga_detect      endp

xga_mode        proc    uses es di si, mode:word

        mov     curbk,-1                ; preload impossible bank number

        call    xga_detect              ; is an XGA adapter present?
        cmp     ax,0
        jne     whichmode
        jmp     nope                    ;  nope
whichmode:
        mov     bx,mode
        cmp     bx,xga_twidth           ; mode number out of range?
        jb      whichmode0
        jmp     nope                    ;  yup - fail right now.
whichmode0:
        cmp     mode,0                  ; 80-col VGA text mode?
        jne     whichmode1
        jmp     mode_0                  ;  yup
whichmode1:
        cmp     mode,1                  ; 132-col VGA text mode?
        jne     whichmode2
        jmp     nope                    ;  Fractint doesn't use this routine
whichmode2:
        mov     bx,mode                 ; locate the table entries
        add     bx,mode
        mov     dx,xga_requir[bx]       ; does our setup support this mode?
        and     al,dl
        cmp     al,dl
        je      whichmode3
        jmp     nope                    ;  nope
whichmode3:

        mov     ax,13h                  ; switch to 320x200x256 mode
        call    maybeor                 ; maybe or AL or (for Video-7s) BL
        int     10h
        push    ds                      ; reset ES==DS
        pop     es
        mov     ax,1017h                ; get the DAC values
        mov     bx,0
        mov     cx,256
        mov     dx,offset paldata       ; a safe place when switching XGA modes
        int     10h
        cmp     xga_loaddac,0           ; save the palette?
        je      paskip                  ;  (yes, if we want to fake 'loaddac')
        mov     si, offset paldata
        mov     di, offset dacbox
        mov     cx,768/2
        rep     movsw
        mov     xga_loaddac,0           ; reset the toggle for next time
paskip:
        mov     bx,769                  ; adjust the palette
paloop: dec     bx
        mov     ah,paldata[bx]

        shl     ah,1
        shl     ah,1
        mov     paldata[bx],ah
        cmp     bx,0
        jne     paloop

        mov     dx,xga_reg_base         ; Palette Mask
        add     dx,0ah
        mov     ax,00064h               ; (Disable the XGA palette)
        out     dx,ax

        mov     dx,xga_swidth[bx]       ; collect and save the scan-line length
        mov     xga_xdots,dx
        mov     dx,xga_colors[bx]       ; how many colors do we have?
        mov     xga_iscolors,dx         ; save this

mode_3: mov     dx,03c3h                ; Enable VGA Address Code
        mov     al,1
        out     dx,al

        mov     si,offset xga_val       ; point to start of values table
        mov     bx,mode                 ; use mode as an offset
model1: mov     dx,xga_reg_base         ; get the base pointer
        mov     ah,0                    ; get the increment
        mov     al,cs:0[si]
        cmp     al,0ffh                 ; end of the table?
        je      model2                  ;  yup
        add     dx,ax
        cmp     al,0ah                  ; check for access type
        je      modsk2
        mov     al,cs:0[si+bx]          ; get the value and OUT it
        out     dx,al
        jmp     short modsk3
modsk2: mov     al,cs:1[si]             ; get the value and OUT it
        mov     ah,cs:0[si+bx]
        out     dx,ax
modsk3: add     si,xga_twidth           ; try another table entry
        jmp     short model1
model2:

        mov     xga_isinmode,2          ; pretend we're already in graphics
        call    xga_clear               ; clear out the memory
        mov     curbk,-1                ; reset the bank counter

        mov     dx,xga_reg_base         ; set up for final loads
        add     dx,0ah
        mov     bx,mode                 ; how many colors do we have?
        add     bx,mode
        cmp     xga_colors[bx],0        ; "true color" mode?
        jne     modsk4                  ; nope - skip the funny palette load

        mov     ax,0064h                ; make invisible
        out     dx,ax
        mov     ax,8055h                ; border color
        out     dx,ax
        mov     ax,0066h                ; palette mode
        out     dx,ax
        mov     ax,0060h                ; start at palette 0
        out     dx,ax
        mov     ax,0061h                ; ""
        out     dx,ax

        mov     cx,0                    ; ready to update the palette
        mov     al,065h                 ; palette update
        out     dx,al
        inc     dx                      ; palette data
model3: mov     al,0                    ; zero out the...
        out     dx,al                   ;  red value
        out     dx,al                   ;  and the green value
        mov     al,cl                   ; klooge up the blue value
        and     al,1fh                  ; convert to 1,2,...1f
        shl     al,1                    ; convert to 2,4,...3e
        shl     al,1                    ; convert to 4,8,...7c
        shl     al,1                    ; convert to 8,16,..fd
        out     dx,al                   ; blue value
        inc     cx                      ; another palette value to go?
        cmp     cx,128
        jb      model3
        dec     dx                      ; back to normal

        mov     ax,0ff64h               ; make the palette visible
        out     dx,ax
        jmp     ok

modsk4: mov     bx, offset paldata      ; reset the palette
        push    bx
        mov     xga_isinmode,2
        call    xga_setpalette
        pop     bx
        jmp     ok

mode_0:                                 ; Set 80 column mode
        mov     dx,xga_reg_base         ; Aperture Control
        add     dx,01h
        xor     al,al                   ; (disable the XGA 64K aperture)
        out     dx,al

        mov     dx,xga_reg_base         ; Interrupt Disable
        add     dx,4
        xor     al,al
        out     dx,al

        mov     dx,xga_reg_base         ; Clear Interrupts
        add     dx,5
        mov     al,0ffh
        out     dx,al

        test    xga_result,10h          ; dual monitor setup?
        jz      mode_0a
        jmp     nope                    ;  yup - don't restore as a VGA
mode_0a:

        mov     dx,xga_reg_base         ; Palette Mask
        add     dx,0ah
        mov     ax,0ff64h               ; (Enable the XGA palette)
        out     dx,ax

        mov     dx,xga_reg_base         ; Enable VFB, Prepare for Reset
        add     dx,0ah
        mov     ax,1550h
        out     dx,ax

        mov     dx,xga_reg_base         ; Enable VFB, reset CRTC
        add     dx,0ah
        mov     ax,1450h
        out     dx,ax

        mov     dx,xga_reg_base         ; Normal Scale Factors
        add     dx,0ah
        mov     ax,0051h
        out     dx,ax

        mov     dx,xga_reg_base         ; Select VGA Oscillator
        add     dx,0ah
        mov     ax,0454h
        out     dx,ax

        mov     dx,xga_reg_base         ; Ext Oscillator (VGA)
        add     dx,0ah
        mov     ax,7f70h
        out     dx,ax

        mov     dx,xga_reg_base         ; Ensure no Vsynch Interrupts
        add     dx,0ah
        mov     ax,202ah
        out     dx,ax

        mov     dx,xga_reg_base         ; Switch to VGA Mode
;;      add     dx,00h
        mov     al,1
        out     dx,al

        mov     dx,03c3h                ; Enable VGA Address Code
        mov     al,1
        out     dx,al

        mov     ax,1202h                ; select 400 scan lines
        mov     bl,30h
        int     10h

        mov     ax,0+3                  ; set video mode 3
        or      al,xga_clearvideo       ; (might supress video-clearing)
        cmp     xga_clearvideo,0        ; clear the video option set?
        je      mode_0b
        mov     ax,08eh                 ; ugly klooge: VGA graphics, no clear
mode_0b:
        int     10h

        jmp     ok                      ; we're done

nope:
        mov     xga_isinmode,0
        mov     ax,0                    ; return failure
        ret
ok:
        mov     ax,mode                 ; remember the mode we're in
        mov     xga_isinmode,ax
        mov     ax,1                    ; return OK
        ret
xga_mode        endp


; **************** internal Read/Write-a-dot routines ***********************
;
;       These Routines all assume the following register values:
;
;               AL = The Color (returned on reads, sent on writes)
;               CX = The X-Location of the Pixel
;               DX = The Y-Location of the Pixel

nullwrite       proc    near            ; "do-nothing" write
        ret
nullwrite       endp

nullread        proc    near            ; "do-nothing" read
        mov     ax,0                    ; "return" black pixels
        ret
nullread        endp

normalwrite     proc    near            ; generic write-a-dot routine
        mov     ah,12                   ; write the dot (al == color)
        mov     bx,0                    ; this page
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret                             ; we done.
normalwrite     endp

normalread      proc    near            ; generic read-a-dot routine
        mov     ah,13                   ; read the dot (al == color)
        mov     bx,0                    ; this page
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret                             ; we done.
normalread      endp

mcgawrite       proc    near            ; MCGA 320*200, 246 colors
        xchg    dh,dl                   ; bx := 256*y
        mov     bx,cx                   ; bx := x
        add     bx,dx                   ; bx := 256*y + x
        shr     dx,1
        shr     dx,1                    ; dx := 64*y
        add     bx,dx                   ; bx := 320*y + x
        mov     es:[bx],al              ; write the dot
        ret                             ; we done.
mcgawrite       endp

mcgaread        proc    near            ; MCGA 320*200, 246 colors
        xchg    dh,dl                   ; dx := 256*y
        mov     bx,cx                   ; bx := x
        add     bx,dx                   ; bx := 256*y + x
        shr     dx,1
        shr     dx,1                    ; dx := 64*y
        add     bx,dx                   ; bx := 320*y + x
        mov     al,es:[bx]              ; retrieve the previous value
        ret                             ; we done.
mcgaread        endp

;       These routines are for bit-plane 16 color modes, including bank
;       switched superVGA varieties such as the Tseng 1024x768x16 mode.
;               Tim Wegner
;
vgawrite        proc    near            ; bank-switched EGA/VGA write mode 0
        mov     bh,al                   ; save the color value for a bit
        mov     ax,vxdots               ; this many dots / line
        mul     dx                      ; times this many lines
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; DX:AX now holds the pixel count
        mov     cx,ax                   ; save this for the bit mask
        and     cx,7                    ; bit-mask shift calculation
        xor     cl,7                    ;  ...
        mov     si,ax                   ; set up for the address shift
        shr     dx,1                    ; (ugly) 32-bit shift-by-3 logic
        rcr     si,1                    ;  ((works on ANY 80x6 processor))
        shr     dx,1                    ;  ...
        rcr     si,1                    ;  ...
        shr     dx,1                    ;  ...
        rcr     si,1                    ;  ...

        cmp     dx,curbk                ; see if bank changed
        je      vgasame_bank            ; jump if old bank ok
        mov     ax,dx                   ; newbank expects bank in al
        call    far ptr newbank         ; switch banks
vgasame_bank:

        mov     dx,03ceh                ; graphics controller address
        mov     ax,0108h                ; set up controller bit mask register
        shl     ah,cl                   ;  ...
        out     dx,ax                   ;  ...
        mov     ah,bh                   ; set set/reset registers
        mov     al,0                    ;  ...
        out     dx,ax                   ;  ...
        mov     ax,0f01h                ; enable set/reset registers
        out     dx,ax                   ;  ...
        or      es:[si],al              ; update all bit planes
        ret                             ; we done.
vgawrite        endp

vgaread proc    near            ; bank-switched EGA/VGA read mode 0
        mov     ax,vxdots               ; this many dots / line
        mul     dx                      ; times this many lines
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; DX:AX now holds the pixel count
        mov     cx,ax                   ; save this for the bit mask
        and     cx,7                    ; bit-mask shift calculation
        xor     cl,7                    ;  ...
        mov     si,ax                   ; set up for the address shift
        shr     dx,1                    ; (ugly) 32-bit shift-by-3 logic
        rcr     si,1                    ;  ((works on ANY 80x6 processor))
        shr     dx,1                    ;  ...
        rcr     si,1                    ;  ...
        shr     dx,1                    ;  ...
        rcr     si,1                    ;  ...

        cmp     dx,curbk                ; see if bank changed
        je      vgasame_bank            ; jump if old bank ok
        mov     ax,dx                   ; newbank expects bank in al
        call    far ptr newbank         ; switch banks
vgasame_bank:

        mov     ch,01h                  ; bit mask to shift
        shl     ch,cl                   ;  ...
        mov     bx,0                    ; initialize bits-read value (none)
        mov     dx,03ceh                ; graphics controller address
        mov     ax,0304h                ; set up controller address register
vgareadloop:
        out     dx,ax                   ; do it
        mov     bh,es:[si]              ; retrieve the old value
        and     bh,ch                   ; mask one bit
        neg     bh                      ; set bit 7 correctly
        rol     bx,1                    ; rotate the bit into bl
        dec     ah                      ; go for another bit?
        jge     vgareadloop             ;  sure, why not.
        mov     al,bl                   ; returned pixel value
        ret                             ; we done.
vgaread endp

outax8bit       proc    near            ; convert OUT DX,AX to
        push    ax                      ; several OUT DX,ALs
        out     dx,al                   ; (leaving registers intact)
        inc     dx
        mov     al,ah
        out     dx,al
        dec     dx
        pop     ax
        ret
outax8bit       endp


; --------------------------------------------------------------------------

; new Tandy and CGA code from Joseph Albrecht

                align   2
;Scan line address table for 16K video memory
scan16k         dw      0000h,2000h,0050h,2050h,00A0h,20A0h,00F0h,20F0h
                dw      0140h,2140h,0190h,2190h,01E0h,21E0h,0230h,2230h
                dw      0280h,2280h,02D0h,22D0h,0320h,2320h,0370h,2370h
                dw      03C0h,23C0h,0410h,2410h,0460h,2460h,04B0h,24B0h
                dw      0500h,2500h,0550h,2550h,05A0h,25A0h,05F0h,25F0h
                dw      0640h,2640h,0690h,2690h,06E0h,26E0h,0730h,2730h
                dw      0780h,2780h,07D0h,27D0h,0820h,2820h,0870h,2870h
                dw      08C0h,28C0h,0910h,2910h,0960h,2960h,09B0h,29B0h
                dw      0A00h,2A00h,0A50h,2A50h,0AA0h,2AA0h,0AF0h,2AF0h
                dw      0B40h,2B40h,0B90h,2B90h,0BE0h,2BE0h,0C30h,2C30h
                dw      0C80h,2C80h,0CD0h,2CD0h,0D20h,2D20h,0D70h,2D70h
                dw      0DC0h,2DC0h,0E10h,2E10h,0E60h,2E60h,0EB0h,2EB0h
                dw      0F00h,2F00h,0F50h,2F50h,0FA0h,2FA0h,0FF0h,2FF0h
                dw      1040h,3040h,1090h,3090h,10E0h,30E0h,1130h,3130h
                dw      1180h,3180h,11D0h,31D0h,1220h,3220h,1270h,3270h
                dw      12C0h,32C0h,1310h,3310h,1360h,3360h,13B0h,33B0h
                dw      1400h,3400h,1450h,3450h,14A0h,34A0h,14F0h,34F0h
                dw      1540h,3540h,1590h,3590h,15E0h,35E0h,1630h,3630h
                dw      1680h,3680h,16D0h,36D0h,1720h,3720h,1770h,3770h
                dw      17C0h,37C0h,1810h,3810h,1860h,3860h,18B0h,38B0h
                dw      1900h,3900h,1950h,3950h,19A0h,39A0h,19F0h,39F0h
                dw      1A40h,3A40h,1A90h,3A90h,1AE0h,3AE0h,1B30h,3B30h
                dw      1B80h,3B80h,1BD0h,3BD0h,1C20h,3C20h,1C70h,3C70h
                dw      1CC0h,3CC0h,1D10h,3D10h,1D60h,3D60h,1DB0h,3DB0h
                dw      1E00h,3E00h,1E50h,3E50h,1EA0h,3EA0h,1EF0h,3EF0h

;Scan line address table for 32K video memory
scan32k         dw      0000h,2000h,4000h,6000h,00a0h,20a0h,40a0h,60a0h
                dw      0140h,2140h,4140h,6140h,01e0h,21e0h,41e0h,61e0h
                dw      0280h,2280h,4280h,6280h,0320h,2320h,4320h,6320h
                dw      03c0h,23c0h,43c0h,63c0h,0460h,2460h,4460h,6460h
                dw      0500h,2500h,4500h,6500h,05a0h,25a0h,45a0h,65a0h
                dw      0640h,2640h,4640h,6640h,06e0h,26e0h,46e0h,66e0h
                dw      0780h,2780h,4780h,6780h,0820h,2820h,4820h,6820h
                dw      08c0h,28c0h,48c0h,68c0h,0960h,2960h,4960h,6960h
                dw      0a00h,2a00h,4a00h,6a00h,0aa0h,2aa0h,4aa0h,6aa0h
                dw      0b40h,2b40h,4b40h,6b40h,0be0h,2be0h,4be0h,6be0h
                dw      0c80h,2c80h,4c80h,6c80h,0d20h,2d20h,4d20h,6d20h
                dw      0dc0h,2dc0h,4dc0h,6dc0h,0e60h,2e60h,4e60h,6e60h
                dw      0f00h,2f00h,4f00h,6f00h,0fa0h,2fa0h,4fa0h,6fa0h
                dw      1040h,3040h,5040h,7040h,10e0h,30e0h,50e0h,70e0h
                dw      1180h,3180h,5180h,7180h,1220h,3220h,5220h,7220h
                dw      12c0h,32c0h,52c0h,72c0h,1360h,3360h,5360h,7360h
                dw      1400h,3400h,5400h,7400h,14a0h,34a0h,54a0h,74a0h
                dw      1540h,3540h,5540h,7540h,15e0h,35e0h,55e0h,75e0h
                dw      1680h,3680h,5680h,7680h,1720h,3720h,5720h,7720h
                dw      17c0h,37c0h,57c0h,77c0h,1860h,3860h,5860h,7860h
                dw      1900h,3900h,5900h,7900h,19a0h,39a0h,59a0h,79a0h
                dw      1a40h,3a40h,5a40h,7a40h,1ae0h,3ae0h,5ae0h,7ae0h
                dw      1b80h,3b80h,5b80h,7b80h,1c20h,3c20h,5c20h,7c20h
                dw      1cc0h,3cc0h,5cc0h,7cc0h,1d60h,3d60h,5d60h,7d60h
                dw      1e00h,3e00h,5e00h,7e00h,1ea0h,3ea0h,5ea0h,7ea0h

;Scan line address table for 64K video memory
scan64k         dw      00000h,00140h,00280h,003c0h,00500h,00640h,00780h,008c0h
                dw      00a00h,00b40h,00c80h,00dc0h,00f00h,01040h,01180h,012c0h
                dw      01400h,01540h,01680h,017c0h,01900h,01a40h,01b80h,01cc0h
                dw      01e00h,01f40h,02080h,021c0h,02300h,02440h,02580h,026c0h
                dw      02800h,02940h,02a80h,02bc0h,02d00h,02e40h,02f80h,030c0h
                dw      03200h,03340h,03480h,035c0h,03700h,03840h,03980h,03ac0h
                dw      03c00h,03d40h,03e80h,03fc0h,04100h,04240h,04380h,044c0h
                dw      04600h,04740h,04880h,049c0h,04b00h,04c40h,04d80h,04ec0h
                dw      05000h,05140h,05280h,053c0h,05500h,05640h,05780h,058c0h
                dw      05a00h,05b40h,05c80h,05dc0h,05f00h,06040h,06180h,062c0h
                dw      06400h,06540h,06680h,067c0h,06900h,06a40h,06b80h,06cc0h
                dw      06e00h,06f40h,07080h,071c0h,07300h,07440h,07580h,076c0h
                dw      07800h,07940h,07a80h,07bc0h,07d00h,07e40h,07f80h,080c0h
                dw      08200h,08340h,08480h,085c0h,08700h,08840h,08980h,08ac0h
                dw      08c00h,08d40h,08e80h,08fc0h,09100h,09240h,09380h,094c0h
                dw      09600h,09740h,09880h,099c0h,09b00h,09c40h,09d80h,09ec0h
                dw      0a000h,0a140h,0a280h,0a3c0h,0a500h,0a640h,0a780h,0a8c0h
                dw      0aa00h,0ab40h,0ac80h,0adc0h,0af00h,0b040h,0b180h,0b2c0h
                dw      0b400h,0b540h,0b680h,0b7c0h,0b900h,0ba40h,0bb80h,0bcc0h
                dw      0be00h,0bf40h,0c080h,0c1c0h,0c300h,0c440h,0c580h,0c6c0h
                dw      0c800h,0c940h,0ca80h,0cbc0h,0cd00h,0ce40h,0cf80h,0d0c0h
                dw      0d200h,0d340h,0d480h,0d5c0h,0d700h,0d840h,0d980h,0dac0h
                dw      0dc00h,0dd40h,0de80h,0dfc0h,0e100h,0e240h,0e380h,0e4c0h
                dw      0e600h,0e740h,0e880h,0e9c0h,0eb00h,0ec40h,0ed80h,0eec0h
                dw      0f000h,0f140h,0f280h,0f3c0h,0f500h,0f640h,0f780h,0f8c0h

;
; ***** CGA Video Routines *****
;

;plot a point on 320x200x4 color graphics screen
plotcga4        proc    near

        mov     bx,0b800h               ;point es at video buffer
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan16k[bx]       ; ..
        mov     ah,cl                   ;save low byte of column
        shr     cx,1                    ;get column offset
        shr     cx,1                    ; ..
        add     bx,cx                   ;add column offset to address
        not     ah                      ;get shift count
        and     ah,3                    ; ..
        shl     ah,1                    ; ..
        mov     cl,ah                   ; ..
        and     al,3                    ;mask off unwanted bits
        rol     al,cl                   ;get or mask
        mov     ah,0fch                 ;get and mask
        rol     ah,cl                   ; ..
        mov     cl,es:[bx]              ;plot the point
        and     cl,ah                   ; ..
        or      cl,al                   ; ..
        mov     es:[bx],cl              ; ..
        ret

plotcga4        endp

;return a point from 320x200x4 color graphics screen
getcga4         proc    near

        mov     bx,0b800h               ;point es at video buffer
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan16k[bx]       ; ..
        mov     ax,cx                   ;save column
        shr     ax,1                    ;get column offset
        shr     ax,1                    ; ..
        add     bx,ax                   ;add column offset to address
        not     cl                      ;get shift count
        and     cl,3                    ; ..
        shl     cl,1                    ; ..
        mov     al,es:[bx]              ;return the point
        ror     al,cl                   ; ..
        and     ax,3                    ; ..
        ret

getcga4         endp

;plot a point on 640x200x2 color graphics screen
plotcga2        proc    near

        mov     bx,0b800h               ;point es at video buffer
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan16k[bx]       ; ..
        mov     ah,cl                   ;save low order byte of column
        shr     cx,1                    ;get column offset
        shr     cx,1                    ; ..
        shr     cx,1                    ; ..
        add     bx,cx                   ;add column offset to address
        not     ah                      ;get shift count
        and     ah,7                    ; ..
        mov     cl,ah                   ; ..
        and     al,1                    ;mask off unwanted bits
        rol     al,cl                   ;get or mask
        mov     ah,0feh                 ;get and mask
        rol     ah,cl                   ; ..
        mov     cl,es:[bx]              ;plot the point
        and     cl,ah                   ; ..
        or      cl,al                   ; ..
        mov     es:[bx],cl              ; ..
        ret

plotcga2        endp

;return a point from 640x200x2 color graphics screen
getcga2         proc    near

        mov     bx,0b800h               ;point es at video buffer
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan16k[bx]       ; ..
        mov     ax,cx                   ;save column
        shr     ax,1                    ;get column offset
        shr     ax,1                    ; ..
        shr     ax,1                    ; ..
        add     bx,ax                   ;add column offset to address
        not     cl                      ;get shift count
        and     cl,7                    ; ..
        mov     al,es:[bx]              ;return the point
        ror     al,cl                   ; ..
        and     ax,1                    ; ..
        ret

getcga2         endp

;
; ***** Tandy 1000 Video Routines *****
;

;plot a point on tandy 1000 640x200x4 color graphics screen
plottandy4      proc    near

        mov     bx,0b800h               ;point es at video segment
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan32k[bx]       ; ..
        mov     dx,cx                   ;save column
        shr     dx,1                    ;get column offset
        shr     dx,1                    ; ..
        and     dl,0feh                 ; ..
        add     bx,dx                   ;add column offset to address
        not     cl                      ;get shift count
        and     cl,7                    ; ..
        mov     ah,al                   ;get or mask
        shr     ah,1                    ; ..
        and     ax,101h                 ; ..
        rol     ax,cl                   ; ..
        mov     dx,0fefeh               ;get and mask
        rol     dx,cl                   ; ..
        mov     cx,es:[bx]              ;plot the point
        and     cx,dx                   ; ..
        or      cx,ax                   ; ..
        mov     es:[bx],cx              ; ..
        ret

plottandy4      endp

;return a point from tandy 1000 640x200x4 color graphics screen
gettandy4       proc    near

        mov     bx,0b800h               ;point es at video segment
        mov     es,bx                   ; ..
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        mov     bx,cs:scan32k[bx]       ; ..
        mov     ax,cx                   ;save column
        shr     ax,1                    ;get column offset
        shr     ax,1                    ; ..
        and     al,0feh                 ; ..
        add     bx,ax                   ;add column offset to address
        not     cl                      ;get shift count
        and     cl,7                    ; ..
        mov     ax,es:[bx]              ;return the point
        ror     ax,cl                   ; ..
        and     ax,101h                 ; ..
        rol     ah,1                    ; ..
        or      al,ah                   ; ..
        and     ax,3                    ; ..
        ret

gettandy4       endp

;plot a point on tandy 1000 16 color graphics screen
plottandy16     proc    near

        mov     es,tandyseg             ;point es at video buffer
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        add     bx,tandyscan            ; ..
        mov     bx,cs:[bx]              ; ..
        mov     ah,0f0h                 ;set mask for odd pixel
        mov     dx,cx                   ;save x
        shr     dx,1                    ;get column offset
        jc      plottandy16a            ;check for odd/even pixel
        not     ah                      ;set mask for even pixel
        mov     cl,4                    ;move color to proper pobxtion
        shl     al,cl                   ; ..
plottandy16a:
        add     bx,dx                   ;add column offset to address
        mov     dl,es:[bx]              ;plot the point
        and     dl,ah                   ; ..
        or      dl,al                   ; ..
        mov     es:[bx],dl              ; ..
        ret

plottandy16     endp

;return a point from tandy 1000 16 color graphics mode
gettandy16      proc    near

        mov     es,tandyseg             ;point es at video buffer
        mov     bx,dx                   ;get scan line address
        shl     bx,1                    ; ..
        add     bx,tandyscan            ; ..
        mov     bx,cs:[bx]              ; ..
        mov     dx,cx                   ;save x
        shr     cx,1                    ;add column offset to address
        add     bx,cx                   ; ..
        mov     al,es:[bx]              ;get pixel
        test    dl,1                    ;check for odd/even pixel
        jnz     gettandy16a             ; ..
        mov     cl,4                    ;move color to proper position
        shr     al,cl                   ; ..
gettandy16a:
        and     ax,0fh                  ;mask off unwanted bits
        ret

gettandy16      endp

;setup tandy 1000 640x200x16 color mode
tandysetup      proc    near
        mov     dx,03d4h                ; write to this address
        mov     ax,07100h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,05001h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,05a02h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,00e03h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,0ff04h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,00605h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,0c806h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,0e207h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,00009h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,0000ch               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,01810h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     ax,04612h               ; write to this register and value
        call    tandyport               ;  do it.
        mov     dx,03d8h                ; new port
        mov     al,01bh                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03d9h                ; new port
        mov     al,000h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03ddh                ; new port
        mov     al,000h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dfh                ; new port
        mov     al,024h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dah                ; new port
        mov     al,001h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03deh                ; new port
        mov     al,00fh                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dah                ; new port
        mov     al,002h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03deh                ; new port
        mov     al,000h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dah                ; new port
        mov     al,003h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03deh                ; new port
        mov     al,010h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dah                ; new port
        mov     al,005h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03deh                ; new port
        mov     al,001h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03dah                ; new port
        mov     al,008h                 ;  and value
        out     dx,al                   ;  do it.
        mov     dx,03deh                ; new port
        mov     al,002h                 ;  and value
        out     dx,al                   ;  do it.

;set color palette registers to default state
        mov     cx,16                   ;reset colors 0-15 to default state
        xor     bh,bh                   ;set initial color
        xor     bl,bl                   ;set initial paletter register
        mov     di,2                    ;reset border color
        call    settandypal             ; ..
tandysetup1:
        mov     di,16                   ;port offset for palette registers
        call    settandypal             ;set palette register
        inc     bl                      ;bump up to next palette register
        inc     bh                      ;bump up to next color
        loop    tandysetup1             ;do remaining palette registers

        cmp     orvideo,0               ; are we supposed to clear RAM?
        jne     tandysetup2             ;  (nope)
        push    es                      ; save ES for a tad
        mov     ax,0a000h               ; clear the memory
        mov     es,ax                   ;  ...
        cld                             ; string ops forward
        mov     cx,07d00h               ; this many words
        mov     di,0                    ; starting here
        mov     ax,0                    ; clear out to zero
        rep     stosw                   ; do it.
        pop     es                      ; restore ES
tandysetup2:
        mov     oktoprint,0             ; no printing in this mode
        ret

tandysetup      endp

;write data to 6845 registers
tandyport       proc    near

        out     dx,al                   ;write 6845 reg number to port 3d4
        mov     al,ah                   ;write 6845 reg data to port 3d5
        inc     dx                      ; ..
        out     dx,al                   ; ..
        dec     dx                      ;point back to port 3d4
        ret

tandyport       endp

;subroutine to set a Tandy 1000 palette register
settandypal     proc    near
        mov     dx,3dah                 ;address & status register
        cli                             ;disable interrupts
settandypal2:
        in      al,dx                   ;get status register
        and     al,8                    ;look for bit 3
        jz      settandypal2            ;wait for vertical retrace
        mov     al,bl                   ;get palette number
        cbw                             ; ..
        add     ax,di                   ;add offset for palette register
        out     dx,al                   ;set palette
        mov     al,bh                   ;get color to store
        mov     dx,3deh                 ;palette data register
        out     dx,al                   ;set palette color
        mov     dx,3dah                 ;address & status register
        xor     ax,ax                   ;al = 0 to reset address register
        out     dx,al                   ;reset it
        sti                             ;re-enable interrupts
        ret

settandypal     endp

; -----------------------------------------------------------------------------


;       The 360x480 mode draws heavily on Michael Abrash's article in
;       the January/February 1989 "Programmer's Journal" and files uploaded
;       to Compuserv's PICS forum by Dr. Lawrence Gozum - integrated here
;       by Timothy Wegner

; Michael Abrash equates. Not all used, but I'll leave for reference.

VGA_SEGMENT       EQU   0A000h
SC_INDEX          EQU   3C4h     ;Sequence Controller Index register
GC_INDEX          EQU   3CEh     ;Graphics Controller Index register
CRTC_INDEX        EQU   3D4h     ;CRT Controller Index register
MAP_MASK          EQU   2        ;Map Mask register index in SC
MEMORY_MODE       EQU   4        ;Memory Mode register in SC
MAX_SCAN_LINE     EQU   9        ;Maximum Scan Line reg index in CRTC
                                 ;Use 9 for 2 pages of 320x400
;MAX_SCAN_LINE    EQU   1        ;Use 1 for 4 pages of 320x200
START_ADD_HIGH    EQU   0Ch      ;Start Address High reg index in CRTC
UNDERLINE         EQU   14h      ;Underline Location reg index in CRTC
MODE_CONTROL      EQU   17h      ;Mode Control reg index in CRTC
READ_MAP          EQU   4        ;Read Mask register index in SC
GRAPHICS_MODE     EQU   5        ;Graphics Mode register index in SC
MISC              EQU   6        ;Miscellaneous register index in SC
WORD_OUTS_OK      EQU   1        ;set to 0 to assemble for computers
                                 ;that can't handle word outs to indexed
                                 ;VGA registers
;
;Macro to output a word value to a port
;
OUT_WORD MACRO
IF WORD_OUTS_OK
         OUT      DX,AX
ELSE
         OUT      DX,AL
         INC      DX
         XCHG     AH,AL
         OUT      DX,AL
         DEC      DX
         XCHG     AH,AL
ENDIF
         ENDM

;Macro to ouput a constant value to an indexed VGA register
CONSTANT_TO_INDEXED_REGISTER     MACRO  ADDRESS,INDEX,VALUE
         MOV      DX,ADDRESS
         MOV      AX,(VALUE SHL 8)+INDEX
         OUT_WORD
         ENDM

tweak256read    proc near uses si       ; Tweaked-VGA ...x256 color mode

  mov     ax,vxdots
;;  shr   ax,1
;;  shr   ax,1                   ; now ax = vxdots/4
  mul     dx                     ;Point to start of desired row
  push    cx                     ;Save X coordinate for later
  shr     cx,1                   ;There are 4 pixels at each address
  shr     cx,1                   ;so divide X by 4
  add     ax,cx                  ;Point to pixels address
  mov     si,ax
  pop     ax                     ;Retrieve X coordinate
  and     al,3                   ;Get the plane number of the pixel
  mov     ah,al
  mov     al,READ_MAP
  mov     dx,GC_INDEX
  OUT_WORD                       ;Set to write to the proper plane for the
                                 ;pixel
  xor     ax,ax
  lods    byte ptr es:[si]       ;Read the pixel
  ret

tweak256read    endp

tweak256write   proc near uses di       ; Tweaked-VGA ...x256 color mode
  mov     bl,al                 ; color
  mov     ax,vxdots
;;  shr   ax, 1
;;  shr   ax, 1                  ; now ax = vxdots/4
  mul     dx                    ;Point to start of desired row
  push    cx                    ;Save X coordinate for later
  shr     cx,1                  ;There are 4 pixels at each address
  shr     cx,1                  ;so divide X by 4
  add     ax,cx                 ;Point to pixels address
  mov     di,ax
  pop     cx                    ;Retrieve X coordinate
  and     cl,3                  ;Get the plane number of the pixel
  mov     ah,1
  shl     ah,cl                 ;Set the bit corresponding to the plane
                                ;the pixel is in
  mov     al,MAP_MASK
  mov     dx,SC_INDEX
  OUT_WORD                       ;Set to write to the proper plane for the
                                 ;pixel
  mov     es:[di],bl             ;Draw the pixel

  ret
tweak256write   endp

;       The following ATI 1024x768x16 mode is courtesy of Mark Peterson

ati1024read     proc near               ; ATI 1024x768x16 read
        call    ati1024addr             ; calculate the address
        mov     al,es:[bx]              ; get the byte the pixel is in

        cmp     xga_isinmode,0          ; say, is this really XGA-style I/O?
        je      notxga                  ;  nope
        test    cl,1                    ; is X odd?
        jnz     atireadhigh             ;  Yup.  Use the high bits
        jmp     short atireadlow        ;  else use the low bits
notxga:

        test    cl,1                    ; is X odd?
        jz      atireadhigh             ;  Nope.  Use the high bits
atireadlow:
        and     ax,0fh                  ; zero out the high-order bits
        ret
atireadhigh:
        and     ax,0f0h                 ; zero out the low-order bits
        mov     cl,4                    ; shift the results
        shr     al,cl                   ;  ...
        ret
ati1024read     endp

ati1024write    proc near               ; ATI 1024x768x16 write
        call    ati1024addr             ; calculate the address
        mov     dl,es:[bx]              ; get the byte the pixel is in
        and     al,00fh                 ; zero out the high-order color bits

        cmp     xga_isinmode,0          ; say, is this really XGA-style I/O?
        je      notxga                  ;  nope
        test    cl,1                    ; is X odd?
        jnz     atiwritehigh            ;  Yup.  Use the high bits
        jmp     short atiwritelow       ;  else use the low bits
notxga:

        test    cl,1                    ; is X odd?
        jz      atiwritehigh            ;  Nope.  Use the high bits
atiwritelow:
        and     dl,0f0h                 ; zero out the low-order video bits
        or      dl,al                   ; add the two together
        mov     es:[bx],dl              ; and write the results
        ret
atiwritehigh:
        mov     cl,4                    ; shift the color bits
        shl     al,cl                   ;  ...
        and     dl,0fh                  ; zero out the high-order video bits
        or      dl,al                   ; add the two together
        mov     es:[bx],dl              ; and write the results
        ret
ati1024write    endp

ati1024addr     proc    near            ; modification of TIW's Super256addr
        clc                             ; clear carry flag
        push    ax                      ; save this for a tad
        mov     ax,vxdots               ; this many dots / line
        mul     dx                      ; times this many lines - ans in dx:ax
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; answer in dx:ax
        shr     dx,1                    ; shift the answer right one bit
        rcr     ax,1                    ;  .. in the 32-bit DX:AX combo
        mov     bx,ax                   ; save this in BX
        cmp     dx,curbk                ; see if bank changed
        je      atisame_bank            ; jump if old bank ok
        mov     ax,dx                   ; newbank expects bank in al
        call    far ptr newbank
atisame_bank:
        pop     ax                      ; restore AX
        ret
ati1024addr     endp

;
;	VESA true-color routines

; ************** Function dac_to_rgb() *******************

;       returns the rgb values (in bl, dh & dl) corresponding to the
;       color (passed in ax) entry in dacbox

; dac_to_rgb -----------------------------------------------------------------
; * changed to return dl=blue, dh=green (was dl=green, dh=blue) - bl=red stays
; (is called only by VESAtruewrite, and videoram bgr layout needs this change)
; ------------------------------------------------------------30-06-2002-ChCh-

dac_to_rgb      proc    uses di
        cmp     truemode,0
        jne     @f
        mov     bx,ax
        add     ax,bx
        add     ax,bx                   ; ax * 3
        mov     di,ax
        mov     bl,dacbox+0[di]         ; red
        mov     dh,dacbox+1[di]         ; green
        mov     dl,dacbox+2[di]         ; blue
        ret                             ; we done.
@@:
        cmp     truemode,1
        jne     @f
        mov     dx,word ptr realcoloriter
;        xchg    dh,dl
        mov     ax,word ptr realcoloriter+2
        mov     bl,al                   ; red
        ret                             ; we done.
@@:                                     ; truemode = 2
        cmp     truemode,2
        jne     @f
        mov     dx,word ptr coloriter
;        xchg    dh,dl
        mov     ax,word ptr coloriter+2
        mov     bl,al                   ; red
        ret                             ; we done.
@@:                                     ; truemode = 3
        mov     ax,word ptr coloriter
        mov     dl,al                   ; blue
        mov     cx,4
        shl     ax,cl
        mov     dh,ah                   ; green
;        mov     ax,word ptr coloriter+2
        neg     ax
        mov     bl,ah                   ; red
        ret                             ; we done.
dac_to_rgb      endp

; ************** Function rgb_to_dac() *******************

;       returns the dac index value (in al, ah=0) corresponding to the
;       rgb values (passed in bl, dh & dl)

; rgb_to_dac -----------------------------------------------------------------
; * changed to await dl=blue, dh=green (was dl=green, dh=blue) - bl=red stays
; (is called only by VESAtrueread, and videoram bgr layout needs this change)
; ------------------------------------------------------------30-06-2002-ChCh-

rgb_to_dac      proc
LOCAL red:word, green:word, blue:word
        xor     bh,bh
        mov     red,bx
        mov     bl,dh
        mov     green,bx
        mov     bl,dl
        mov     blue,bx
        mov     si,0                    ; look for the nearest DAC value
        mov     di,0                    ; di = closest DAC
        mov     cx,65535                ; cx = closest squared error
nextentry:
;        mov     bx,0                    ; bx = this entry's squared error
        mov     ax,red                  ; ax = red portion
        mov     dh,0
        mov     dl,dacbox[si]
        sub     ax,dx                   ; ax = (color - DAC color)
        mov     dx,ax
        imul    dx                      ; ax = color squared error
        mov     bx,ax                   ; bx = total squared error
        mov     ax,green                ; ax = green portion
        mov     dh,0
        mov     dl,dacbox+1[si]
        sub     ax,dx                   ; ax = (color - DAC color)
        mov     dx,ax
        imul    dx                      ; ax = color squared error
        add     bx,ax                   ; bx = total squared error
        mov     ax,blue                 ; ax = blue portion
        mov     dh,0
        mov     dl,dacbox+2[si]
        sub     ax,dx                   ; ax = (color - DAC color)
        mov     dx,ax
        imul    dx                      ; ax = color squared error
        add     bx,ax                   ; bx = total squared error
        cmp     bx,cx                   ; new closest value?
        jae     @f                      ;  nope
        mov     di,si                   ; yes - save this entry
        mov     cx,bx                   ;  and its error
@@:     cmp     cx,0                    ; did we find a perfect match?
        je      @f                      ;  yup - we're done!
        add     si,3                    ; move to a new DAC entry
        cmp     si,256*3                ; are we out of entries?
        jb      nextentry               ;  nope
@@:     mov     ax,di                   ; convert DI back into a palette value
        mov     bx,3                    ;  by dividing by 3
        div     bl
        mov     ah,0
        ret                             ; we done.
rgb_to_dac      endp

; VESAtruewrite --------------------------------------------------------------
; * the major change is calling VESAtrueaddr just once (was three times!)
; * this also frees use of some registers, so local variables are removed
; * a minor fix in unusual r-g-b layout (was b-g-b) does any card use it?
; ------------------------------------------------------------30-06-2002-ChCh-

VESAtruewrite   proc near               ; VESA true-color write-a-dot
; color index is passed in ax
        push    ax
        call    VESAtrueaddr            ; calculate address and switch banks
        pop     ax
        push    dx                      ; bank needed later
        push    bx                      ; offset as well
        call    dac_to_rgb              ; ax=color -> dx=gb, bl=r
        mov     ax,vesa_winaseg         ; VESA video starts here
        cmp     vesa_bitsppixel,17      ; 8-8-8 and 8-8-8-8
        mov     es,ax
        jnb     over_hi
        mov     cx,111111b              ; mask
        mov     al,dh                   ; green
        and     dx,cx                   ; blue
        and     bx,cx                   ; red
        and     ax,cx
        mov     cl,vesa_redpos
        cmp     vesa_greensize,6        ; 5-5-5 or 5-6-5
        je      got_6g
        shr     ax,1
got_6g:
        shr     bx,1
        shr     dx,1
        shl     bx,cl
        mov     cl,vesa_greenpos
        or      dx,bx                   ; r-_-b
        shl     ax,cl
        pop     bx
        or      dx,ax                   ; r-g-b
        pop     ax
        mov     word ptr es:[bx],dx     ; write two bytes for the dot
        jmp     short wedone
over_hi:                                ; 8-8-8 or 8-8-8-8?
        mov     cx,bx                   ; r
        shl     dx,1                    ; well, 6-6-6 is not true-true
        shl     cx,1
        shl     dx,1                    ; b-g
        shl     cx,1
        cmp     vesa_redpos,0           ; common b-g-r model?
        jne     doit_slow
        xchg    dl,cl                   ; else turn to unusual r-g-b
doit_slow:
        pop     bx                      ; get offset
        pop     ax                      ; get bank
        inc     bx                      ; does a word fit to this bank?
        jz      badbank1                ; no, switch one byte after
        mov     word ptr es:[bx-1],dx   ; else plot that word
        push    cx                      ; hand it over to the switch
        inc     bx                      ; bank-end?
        jz      badbank2                ; yes, switch it
        mov     es:[bx],cl              ; else write the third byte
        pop     cx                      ; wasn't needed - no switching
        jmp     short wedone
badbank1:
        dec     bx                      ; bx=0ffffh
        mov     es:[bx],dl              ; plot the first byte
        xchg    dh,dl                   ; second color for badbank2
        push    dx                      ; hand it over
badbank2:
        inc     ax                      ; next bank needed
        call    far ptr newbank
        pop     ax                      ; get next color
        inc     bx                      ; badbank1 or badbank2?
        mov     es:[0],al               ; plot that color
        jnz     wedone                  ; badbank2 - nothing more to do
        mov     es:[1],cl               ; badbank1 - plot the last byte
wedone:
        ret                             ; we done.
VESAtruewrite   endp

; VESAtrueread ---------------------------------------------------------------
; * similar changes as in VESAtruewrite
; * hi-color-word rgb extraction quite shortened
; ------------------------------------------------------------30-06-2002-ChCh-

VESAtrueread    proc near               ; VESA true-color read-a-dot
; color index is returned in ax
        call    VESAtrueaddr            ; calculate address and switch banks
        mov     ax,vesa_winaseg         ; VESA video starts here
        cmp     vesa_bitsppixel,17      ; 8-8-8 and 8-8-8-8
        mov     es,ax
        jnb     over_hi
        mov     dx,word ptr es:[bx]     ; read two bytes
        mov     cl,3
        mov     bx,dx
        shl     dx,cl
        mov     cl,vesa_redpos
        shr     dl,1
        shr     bx,cl
        shr     dl,1                    ; blue in dl
        cmp     vesa_greensize,6
        je      got_6g
        shl     dh,1
got_6g:
        shl     bx,1                    ; red in bl
        and     dh,111111b              ; green in dh
        jmp     short wedone
over_hi:                                ; 8-8-8 or 8-8-8-8?
        mov     ax,dx                   ; newbank expects bank in ax
        inc     bx                      ; does a word fit to this bank?
        jz      badbank1                ; no, switch one byte after
        mov     dx,word ptr es:[bx-1]   ; else read that word
        inc     bx                      ; bank-end?
        jz      badbank2                ; yes, switch it
        mov     bl,es:[bx]              ; else read the third byte
        jmp     short colors_in
badbank1:
        dec     bx                      ; bx=0ffffh
        mov     dl,es:[bx]              ; read the first byte
badbank2:
        inc     ax                      ; next bank needed
        call    far ptr newbank
        inc     bx                      ; badbank1 or badbank2?
        mov     bl,es:[0]               ; read next color
        jnz     colors_in               ; badbank2 - nothing more to do
        mov     dh,bl
        mov     bl,es:[1]               ; badbank1 - read the last byte
colors_in:
        and     dx,0FCFCh               ; mask-out 2 g & 2 b lsbs for shift
        shr     bl,1
        shr     dx,1
        shr     bl,1
        shr     dx,1
        cmp     vesa_redpos,0
        jne     wedone
        xchg    bl,dl
wedone:
        call    rgb_to_dac              ; put dac index in ax
        ret                             ; we done.
VESAtrueread    endp

; VESAtrueaddr ---------------------------------------------------------------
; * changed to not to take into account vesabyteoffset
; ------------------------------------------------------------30-06-2002-ChCh-

VESAtrueaddr    proc near
        mov     bx,cx                   ; adjust the pixel location
        cmp     vesa_bitsppixel,17      ; 2 bytes/pixel
        jb      depth_ok                ; write a word at a time, no offset
        shl     bx,1                    ; +x
        cmp     vesa_bitsppixel,25      ; 3 bytes/pixel
        jb      depth_ok                ; else 4 bytes/pixel
        add     bx,cx                   ; +x
depth_ok:
        mov     ax,vxdots               ; this many dots / line
        add     bx,cx                   ; +x
        mul     dx                      ; times this many lines - ans in dx:ax
        add     bx,ax                   ; plus this many x-dots
        adc     dx,0                    ; answer in dx:ax - dl=bank, ax=offset
        cmp     dx,curbk                ; see if bank changed
        je      same_bank               ; jump if old bank not ok
        mov     ax,dx                   ; newbank expects bank in al
        call    far ptr newbank
same_bank:
        ret                             ; we done.
VESAtrueaddr    endp

;
;       The following 'Super256' code is courtesy of Timothy Wegner.
;

super256write   proc near               ; super-VGA ...x256 colors write-a-dot
        call    super256addr            ; calculate address and switch banks
        mov     es:[bx],al              ; write the dot
        ret                             ; we done.
super256write   endp

super256read    proc near               ; super-VGA ...x256 colors read-a-dot
        call    super256addr            ; calculate address and switch banks
        mov     al,es:[bx]              ; read the dot
        ret                             ; we done.
super256read    endp

super256addr    proc near               ; can be put in-line but shared by
                                        ; read and write routines
        clc                             ; clear carry flag
        push    ax                      ; save this for a tad
        mov     ax,vxdots               ; this many dots / line
        mul     dx                      ; times this many lines - ans in dx:ax
        add     ax,cx                   ; plus this many x-dots
        adc     dx,0                    ; answer in dx:ax - dl=bank, ax=offset
        mov     bx,ax                   ; save this in BX
        cmp     dx,curbk                ; see if bank changed
        je      same_bank               ; jump if old bank ok
        mov     ax,dx                   ; newbank expects bank in al
        call    far ptr newbank
same_bank:
        pop     ax                      ; restore AX
        ret
super256addr    endp

;
;       BANKS.ASM was used verbatim except:
;          1) removed ".model small"
;          2) deleted "end"
;       Integrated by Tim Wegner 8/15/89
;       (switched to John's 9/7/89 version on 9/10/89 - Bert)
;       (switched to John's 1/5/90 version on 1/9/90  - Bert)
;       (switched to John's version 3 on 4/27/90  - Bert)
;       (added logic for various resolution on 9/10/90 - Bert)
;       (upgraded to John's version 3.5 on 5/14/91 - Bert)
;

;       .MODEL medium,c

;
;       Copyright 1988,89,90,91 John Bridges
;       Free for use in commercial, shareware or freeware applications
;
;       SVGAMODE.ASM
;
.data

OSEG    equ     SS:                     ;segment override for variable access

bankadr dw      offset $nobank
if @CodeSize
bankseg dw      seg $nobank
endif

vesa_bankswitch         dd      $vesa_nullbank ; initially, do-nothing
vesa_mapper             dd      $nobank
vesa_detect             dw      1       ; set to 0 to disable VESA-detection
vesa_gran_offset        dw      0
vesa_low_window         dw      0
vesa_high_window        dw      1
vesa_granularity        db      0       ; BDT VESA Granularity value

istruecolor		dw	0	; set to 1 if VESA truecolor mode
align 2
; vesabytes		db	0,0,0,0	; our true-color pixel
; vesabyteoffset		dw	0	; used in true-color routines

;	the first 40 bytes of the 256-byte VESA mode-info block

vesa_mode_info	dw	0		; mode info: attributes
vesa_winaattrib	db	0		; Win AA attribs 
vesa_winbattrib	db	0		; Win BB attribs 
vesa_wingran	dw	0		; window granularity
vesa_winsize	dw	0		; window size
vesa_winaseg	dw	0		; window AA segment
vesa_winbseg	dw	0		; window BB segment
vesa_funcptr	dd	0		; bank_switcher
vesa_bytespscan	dw	0		; bytes per scan line
vesa_xres	dw	0		; X-resolution
vesa_yres	dw	0		; Y-resolution
vesa_xcharsize	db	0		; X charsize
vesa_ycharsize	db	0		; Y charsize
vesa_numplanes	db	0		; number of planes
vesa_bitsppixel	db	0		; bits / pixel
vesa_numbanks	db	0		; number of banks
vesa_memmodel	db	0		; memory-model type
vesa_banksize	db	0		; bank size in KB
vesa_numpages	db	0		; number of complete images
vesa_rsvd	db	0		; reserved for page function
vesa_redsize	db	0		; red mask size
vesa_redpos	db	0		; red mask position
vesa_greensize	db	0		; green mask size
vesa_greenpos	db	0		; green mask position
vesa_bluesize	db	0		; blue mask size
vesa_bluepos	db	0		; blue mask position
vesa_rsvdsize	db	0		; reserved mask size
vesa_rsvdpos	db	0		; reserved mask position
vesa_directinfo	db	0		; direct-color-mode attributes

        public  curbk

                align   2
curbk   dw      0

vga512  dw      0
vga1024 dw      0


        public  supervga_list   ; pointer to head of the SuperVGA list

supervga_list   db      "aheada"
aheada  dw      0
        db      "ati   "
ativga  dw      0
        db      "chi   "
chipstech dw    0
        db      "eve   "
everex  dw      0
        db      "gen   "
genoa   dw      0
        db      "ncr   "
ncr     dw      0
        db      "oak   "
oaktech dw      0
        db      "par   "
paradise dw     0
        db      "tri   "
trident dw      0
        db      "tseng3"
tseng   dw      0
        db      "tseng4"
tseng4  dw      0
        db      "vid   "
video7  dw      0
        db      "aheadb"
aheadb  dw      0
        db      "vesa  "
vesa    dw      0
        db      "cirrus"
cirrus  dw      0
        db      "t8900 "
t8900   dw      0
        db      "compaq"
compaq  dw      0
        db      "xga   "
xga     dw      0
        db      "      "        ; end-of-the-list
        dw      0

done_detect dw  0               ;flag to call adapter_detect & whichvga once

                ; this part is new - Bert
.code

vesa_entries    dw      0
        dw       640, 400,256, 4f02h,100h
        dw       640, 480,256, 4f02h,101h
        dw       800, 600, 16, 4f02h,102h
        dw       800, 600,256, 4f02h,103h
        dw      1024, 768, 16, 4f02h,104h
        dw      1024, 768,256, 4f02h,105h
        dw      1280,1024, 16, 4f02h,106h
        dw      1280,1024,256, 4f02h,107h
ahead_entries   dw      0
        dw       800, 600, 16, 06ah,0
        dw       800, 600, 16, 071h,0
        dw      1024, 768, 16, 074h,0
        dw       640, 400,256, 060h,0
        dw       640, 480,256, 061h,0
        dw       800, 600,256, 062h,0
        dw      1024, 768,256, 063h,0
ati_entries     dw      0
        dw       800, 600, 16, 054h,0
        dw      1024, 768, 16, 065h,0ffh        ; (non-standard mode flag)
;       dw      1024, 768, 16, 055h,0
        dw       640, 400,256, 061h,0
        dw       640, 480,256, 062h,0
        dw       800, 600,256, 063h,0
        dw      1024, 768,256, 064h,0
chips_entries   dw      0
        dw       800, 600, 16, 070h,0
        dw      1024, 768, 16, 072h,0
        dw       640, 400,256, 078h,0
        dw       640, 480,256, 079h,0
        dw       800, 600,256, 07bh,0
compaq_entries  dw      0
        dw       640, 480,256, 02eh,0fdh        ; (non-standard mode flag)
everex_entries  dw      0
        dw       752, 410, 16, 070h,01h
        dw       800, 600, 16, 070h,02h
        dw      1280, 350,  4, 070h,11h
        dw      1280, 600,  4, 070h,12h
        dw       640, 350,256, 070h,13h
        dw       640, 400,256, 070h,14h
        dw       512, 480,256, 070h,15h
        dw      1024, 768, 16, 070h,20h
        dw       640, 480,256, 070h,30h
        dw       800, 600,256, 070h,31h
        dw      1024, 768,256, 070h,32h
genoa_entries   dw      0
        dw      1024, 768,  4, 07fh,0
        dw       720, 512, 16, 059h,0
        dw       800, 600, 16, 079h,0
        dw      1024, 768, 16, 05fh,0
        dw       640, 350,256, 05bh,0
        dw       640, 400,256, 07eh,0
        dw       640, 480,256, 05ch,0
        dw       720, 512,256, 05dh,0
        dw       800, 600,256, 05eh,0
ncr_entries      dw     0
        dw      1024, 768,  2, 5ah,0
        dw       800, 600, 16, 58h,0
        dw      1024, 768, 16, 5dh,0
        dw       640, 400,256, 5eh,0
        dw       640, 480,256, 5fh,0
        dw       800, 600,256, 5ch,0
;       dw      0       ; this appears to be extra! JCO
oaktech_entries dw      0
        dw       800, 600, 16, 52h,0
        dw       640, 480,256, 53h,0
        dw       800, 600,256, 54h,0
        dw      1024, 768, 16, 56h,0
        dw      1024, 768,256, 59h,0
        dw      1280,1024, 16, 58h,0
;       dw      0
paradise_entries        dw      0
        dw       800, 600,  2, 059h,0
        dw       800, 600, 16, 058h,0
        dw       640, 400,256, 05eh,0
        dw       640, 480,256, 05fh,0
        dw      1024, 768, 16, 05dh,0
        dw       800, 600,256, 05ch,0   ; Chuck Ebbert, 910524
trident_entries dw     0
        dw      1024, 768,  4, 060h,0
        dw       800, 600, 16, 05bh,0
        dw      1024, 768, 16, 05fh,0
        dw       640, 400,256, 05ch,0
        dw       640, 480,256, 05dh,0
        dw       800, 600,256, 05eh,0
        dw      1024, 768,256, 062h,0
tseng_entries   dw      0
        dw       800, 600, 16, 029h,0
        dw      1024, 768, 16, 037h,0
        dw       640, 350,256, 02dh,0
        dw       640, 400,256, 0,0feh   ; (non-standard mode flag)
        dw       640, 480,256, 02eh,0
        dw       720, 512,256, 02fh,0
        dw       800, 600,256, 030h,0
        dw      1024, 768,256, 038h,0
tseng4_entries  dw      0
        dw       800, 600, 16, 29h,0
        dw      1024, 768, 16, 37h,0
        dw       640, 350,256, 2dh,0
        dw       640, 400,256, 2fh,0
        dw       640, 480,256, 2eh,0
        dw       800, 600,256, 30h,0
        dw      1024, 768,256, 38h,0
video7_entries  dw      0
        dw       752, 410, 16, 6f05h,60h
        dw       720, 540, 16, 6f05h,61h
        dw       800, 600, 16, 6f05h,62h
        dw      1024, 768,  2, 6f05h,63h
        dw      1024, 768,  4, 6f05h,64h
        dw      1024, 768, 16, 6f05h,65h
        dw       640, 400,256, 6f05h,66h
        dw       640, 480,256, 6f05h,67h
        dw       720, 540,256, 6f05h,68h
        dw       800, 600,256, 6f05h,69h
        dw      1024, 768,256, 6f05h,6ah
xga_entries     dw      0
        dw      1024, 768,256,0ffffh,02h
        dw      1024, 768, 16,0ffffh,03h
        dw       640, 400,256,0ffffh,04h
        dw       640, 480,256,0ffffh,04h
        dw       800, 600, 16,0ffffh,06h
        dw       800, 600,256,0ffffh,07h
no_entries      dw      0
        dw       320, 200,256, 13h,0
        dw       640, 480, 16, 12h,0
        dw      0

.code

newbank proc                    ;bank number is in AX
        cli
        mov     OSEG[curbk],ax
if @CodeSize
        call    dword ptr OSEG[bankadr]
else
        call    word ptr OSEG[bankadr]
endif
        ret
newbank endp

$tseng  proc            ;Tseng
        push    ax
        push    dx
        and     al,7
        mov     ah,al
        shl     al,1
        shl     al,1
        shl     al,1
        or      al,ah
        or      al,01000000b
        mov     dx,3cdh
        out     dx,al
        sti
        pop     dx
        pop     ax
        ret
$tseng  endp

$tseng4 proc            ;Tseng 4000 series
        push    ax
        push    dx
        mov     ah,al
        mov     dx,3bfh                 ;Enable access to extended registers
        mov     al,3
        out     dx,al
        mov     dl,0d8h
        mov     al,0a0h
        out     dx,al
        and     ah,15
        mov     al,ah
        shl     al,1
        shl     al,1
        shl     al,1
        shl     al,1
        or      al,ah
        mov     dl,0cdh
        out     dx,al
        sti
        pop     dx
        pop     ax
        ret
$tseng4 endp

$trident proc           ;Trident
        push    ax
        push    dx
        mov     dx,3ceh         ;set page size to 64k
        mov     al,6
        out     dx,al
        inc     dl
        in      al,dx
        dec     dl
        or      al,4
        mov     ah,al
        mov     al,6
        out     dx,ax

        mov     dl,0c4h         ;switch to BPS mode
        mov     al,0bh
        out     dx,al
        inc     dl
        in      al,dx
        dec     dl

        mov     ah,byte ptr OSEG[curbk]
        xor     ah,2
        mov     dx,3c4h
        mov     al,0eh
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$trident endp

$video7 proc            ;Video 7
        push    ax
        push    dx
        push    cx
; Video-7 1024x768x16 mode patch (thanks to Frank Lozier 11/8/89).
        cmp     colors,16
        jne     video7xx
        shl     ax,1
        shl     ax,1
video7xx:
        and     ax,15
        mov     ch,al
        mov     dx,3c4h
        mov     ax,0ea06h
        out     dx,ax
        mov     ah,ch
        and     ah,1
        mov     al,0f9h
        out     dx,ax
        mov     al,ch
        and     al,1100b
        mov     ah,al
        shr     ah,1
        shr     ah,1
        or      ah,al
        mov     al,0f6h
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        and     al,not 1111b
        or      ah,al
        mov     al,0f6h
        out     dx,ax
        mov     ah,ch
        mov     cl,4
        shl     ah,cl
        and     ah,100000b
        mov     dl,0cch
        in      al,dx
        mov     dl,0c2h
        and     al,not 100000b
        or      al,ah
        out     dx,al
        sti
        pop     cx
        pop     dx
        pop     ax
        ret
$video7 endp

$paradise proc          ;Paradise
        push    ax
        push    dx
        mov     dx,3ceh
        mov     ax,50fh         ;turn off write protect on VGA registers
        out     dx,ax
        mov     ah,byte ptr OSEG[curbk]
        shl     ah,1
        shl     ah,1
        shl     ah,1
        shl     ah,1
        mov     al,9
        out     dx,ax
        mov     ax,000Fh        ;reprotect registers, 910512
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$paradise endp

$chipstech proc         ;Chips & Tech
        push    ax
        push    dx
        mov     dx,46e8h        ;place chip in setup mode
        mov     ax,1eh
        out     dx,ax
        mov     dx,103h         ;enable extended registers
        mov     ax,0080h        ; (patched per JB's msg - Bert)
        out     dx,ax
        mov     dx,46e8h        ;bring chip out of setup mode
        mov     ax,0eh
        out     dx,ax
        mov     ah,byte ptr OSEG[curbk]
        shl     ah,1            ;change 64k bank number into 16k bank number
        shl     ah,1
        mov     al,10h
        mov     dx,3d6h
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$chipstech endp

$ativga proc            ;ATI VGA Wonder
        push    ax
        push    dx
        mov     ah,al
        mov     dx,1ceh
        mov     al,0b2h
        out     dx,al
        inc     dl
        in      al,dx
        shl     ah,1
        and     al,0e1h
        or      ah,al
        mov     al,0b2h
        dec     dl
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$ativga endp

$everex proc            ;Everex
        push    ax
        push    dx
        push    cx
        mov     cl,al
        mov     dx,3c4h
        mov     al,8
        out     dx,al
        inc     dl
        in      al,dx
        dec     dl
        shl     al,1
        shr     cl,1
        rcr     al,1
        mov     ah,al
        mov     al,8
        out     dx,ax
        mov     dl,0cch
        in      al,dx
        mov     dl,0c2h
        and     al,0dfh
        shr     cl,1
        jc      nob2
        or      al,20h
nob2:   out     dx,al
        sti
        pop     cx
        pop     dx
        pop     ax
        ret
$everex endp

$aheada proc
        push    ax
        push    dx
        push    cx
        mov     ch,al
        mov     dx,3ceh         ;Enable extended registers
        mov     ax,200fh
        out     dx,ax
        mov     dl,0cch         ;bit 0
        in      al,dx
        mov     dl,0c2h
        and     al,11011111b
        shr     ch,1
        jnc     temp_1
        or      al,00100000b
temp_1: out     dx,al
        mov     dl,0cfh         ;bits 1,2,3
        mov     al,0
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        and     al,11111000b
        or      al,ch
        mov     ah,al
        mov     al,0
        out     dx,ax
        sti
        pop     cx
        pop     dx
        pop     ax
        ret
$aheada endp

$aheadb proc
        push    ax
        push    dx
        push    cx
        mov     ch,al
        mov     dx,3ceh         ;Enable extended registers
        mov     ax,200fh
        out     dx,ax
        mov     ah,ch
        mov     cl,4
        shl     ah,cl
        or      ah,ch
        mov     al,0dh
        out     dx,ax
        sti
        pop     cx
        pop     dx
        pop     ax
        ret
$aheadb endp

$oaktech proc           ;Oak Technology Inc OTI-067
        push    ax
        push    dx
        and     al,15
        mov     ah,al
        shl     al,1
        shl     al,1
        shl     al,1
        shl     al,1
        or      ah,al
        mov     al,11h
        mov     dx,3deh
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$oaktech endp

$genoa  proc                    ;GENOA GVGA
        push    ax
        push    dx
        mov     ah,al
        shl     al,1
        shl     al,1
        shl     al,1
        or      ah,al
        mov     al,6
        or      ah,40h
        mov     dx,3c4h
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$genoa  endp

$ncr    proc                            ;NCR 77C22E
        push    ax
        push    dx
        shl     al,1            ;change 64k bank number into 16k bank number
        shl     al,1
        mov     ah,al
        mov     al,18h
        mov     dx,3c4h
        out     dx,ax
        mov     ax,19h
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$ncr    endp

$compaq proc                    ;Compaq
        push    ax
        push    dx
        mov     dx,3ceh
        mov     ax,50fh         ;unlock extended registers
        out     dx,ax
        mov     ah,byte ptr OSEG[curbk]
        shl     ah,1            ;change 64k bank number into 4k bank number
        shl     ah,1
        shl     ah,1
        shl     ah,1
        mov     al,45h
        out     dx,ax
        sti
        pop     dx
        pop     ax
        ret
$compaq endp

;
;  Read/Write 64K pages
;
$vesa1  proc                            ; VESA bank switching
        push    ax
        push    bx
        push    dx
        mul     vesa_granularity        ; Adjust for the granularity factor
        mov     dx,ax                   ; Select window position
        mov     bx,0                    ; select window (bank) sub-command
        call    dword ptr vesa_bankswitch  ; do it!
        pop     dx
        pop     bx
        pop     ax
        sti
        ret
$vesa1  endp
;
;  Read-only/Write-only 64K pages
;
$vesa2  proc                            ; VESA bank switching
        push    ax
        push    bx
        push    dx
        mul     vesa_granularity        ; Adjust for the granularity factor
        mov     dx,ax                   ; Select window position
        push    dx
        mov     bx,0                    ; select window (bank) sub-command
        call    dword ptr vesa_bankswitch  ; do it!
        pop     dx
        inc     bx
        call    dword ptr vesa_bankswitch
        pop     dx
        pop     bx
        pop     ax
        sti
        ret
$vesa2  endp
;
;  Read/Write 32K pages
;
$vesa3  proc                            ; VESA bank switching
        push    ax
        push    bx
        push    dx
        mul     vesa_granularity        ; Adjust for the granularity factor
        mov     dx,ax                   ; Select window position
        push    dx
        mov     bx,vesa_low_window      ; select window (bank) sub-command
        call    dword ptr vesa_bankswitch  ; do it!
        pop     dx
        add     dx,vesa_gran_offset     ; 2nd window is at 32K offset from 1st
        mov     bx,vesa_high_window
        call    dword ptr vesa_bankswitch
        pop     dx
        pop     bx
        pop     ax
        sti
        ret
$vesa3  endp

$vesa_nullbank proc ; null routine for vesa_bankswitch when unknown
        ret
$vesa_nullbank endp

$nobank proc
        sti
        ret
$nobank endp

bkadr   macro   flag,func,entries               ; Bert
        mov     video_entries, offset entries   ; Bert
        mov     [flag],1
        mov     [bankadr],offset func
if @CodeSize
        mov     [bankseg],seg func
endif
        endm

nojmp   macro
        local   lbl
        jmp     lbl
lbl:
        endm

whichvga proc   near
        push    bp                      ; save it around all the int 10s

        cmp     svga_type,0             ; was a SuperVGA adapter forced?
        jne     type1_forced            ;  yup - wade through the options
        jmp     not_forced              ;  nope - skip this section
type1_forced:
        cmp     svga_type,1
        jne     type2_forced
        bkadr   aheada,$aheada,ahead_entries
type2_forced:
        cmp     svga_type,2
        jne     type3_forced
        bkadr   ativga,$ativga,ati_entries
type3_forced:
        cmp     svga_type,3
        jne     type4_forced
        bkadr   chipstech,$chipstech,chips_entries
type4_forced:
        cmp     svga_type,4
        jne     type5_forced
        bkadr   everex,$everex,everex_entries
type5_forced:
        cmp     svga_type,5
        jne     type6_forced
        bkadr   genoa,$genoa,genoa_entries
type6_forced:
        cmp     svga_type,6
        jne     type7_forced
        bkadr   ncr,$ncr,ncr_entries
type7_forced:
        cmp     svga_type,7
        jne     type8_forced
        bkadr   oaktech,$oaktech,oaktech_entries
type8_forced:
        cmp     svga_type,8
        jne     type9_forced
        bkadr   paradise,$paradise,paradise_entries
type9_forced:
        cmp     svga_type,9
        jne     type10_forced
        bkadr   trident,$trident,trident_entries
type10_forced:
        cmp     svga_type,10
        jne     type11_forced
        bkadr   tseng,$tseng,tseng_entries
type11_forced:
        cmp     svga_type,11
        jne     type12_forced
        bkadr   tseng4,$tseng4,tseng4_entries
type12_forced:
        cmp     svga_type,12
        jne     type13_forced
        bkadr   video7,$video7,video7_entries
type13_forced:
        cmp     svga_type,13
        jne     type14_forced
        bkadr   aheadb,$aheadb,ahead_entries
type14_forced:
        jmp     fini
not_forced:

        cmp     vesa_detect,0           ; is VESA-detection disabled?
        je      notvesa                 ;  yup - skip this
        mov     ax,4f00h                ; check for VESA adapter
        push    ds                      ; set ES == DS
        pop     es                      ;  ...
        mov     di, offset dacbox       ; answer goes here (a safe place)
        int     10h                     ; do it.
        cmp     ax,004fh                ; successful response?
        jne     notvesa                 ; nope.  Not a VESA adapter

        cmp     byte ptr 0[di],'V'      ; string == 'VESA'?
        jne     notvesa                 ; nope.  Not a VESA adapter
        cmp     byte ptr 1[di],'E'      ; string == 'VESA'?
        jne     notvesa                 ; nope.  Not a VESA adapter
        cmp     byte ptr 2[di],'S'      ; string == 'VESA'?
        jne     notvesa                 ; nope.  Not a VESA adapter
        cmp     byte ptr 3[di],'A'      ; string == 'VESA'?
        jne     notvesa                 ; nope.  Not a VESA adapter
        mov     ax,word ptr 18[di]
        mov     video_vram,ax           ; store video memory size
        bkadr   vesa,$vesa_nullbank, vesa_entries
        jmp     fini
notvesa:

        call    xga_detect              ; XGA Adapter?
        cmp     ax,0
        je      notxga                  ; nope
        bkadr   xga,xga_newbank, xga_entries
        jmp     fini
notxga:

        mov     si,1
        mov     ax,0c000h
        mov     es,ax
        cmp     word ptr es:[40h],'13'
        jnz     noati
        bkadr   ativga,$ativga,ati_entries              ; Bert
        mov     dx,es:[10h]             ; Get value of ATI extended register
        mov     bl,es:[43h]             ; Get value of ATI chip version
        cmp     bl,'3'
        jae     v6up                    ; Use different method to determine
        mov     al,0bbh                 ; memory size of chip version is 3 or higher
        cli
        out     dx,al
        inc     dx
        in      al,dx                   ; Get ramsize byte for chip versions 1 & 2
        sti
        test    al,20h
        jz      no512
        mov     [vga512],1
        jmp     short no512

v6up:   mov     al,0b0h                 ; Method used for newer ATI chip versions
        cli
        out     dx,al
        inc     dx
        in      al,dx                   ; Get ramsize byte for versions 3-5
        sti
        test    al,10h                  ; Check if ramsize byte indicates 256K or 512K bytes
        jz      v7up
        mov     [vga512],1
v7up:   cmp     bl,'4'                  ; Check for ramsize for ATI chip versions 4 & 5
        jb      no512
        test    al,8                    ; Check if version 5 ATI chip has 1024K
        jz      no512
        mov     [vga1024],1
no512:  jmp     fini

noati:  mov     ax,7000h                ;Test for Everex
        xor     bx,bx
        cld
        int     10h
        cmp     al,70h
        jnz     noev
        bkadr   everex,$everex, everex_entries          ; Bert
        and     ch,11000000b
        jz      temp_2
        mov     [vga512],1
temp_2: and     dx,0fff0h
        cmp     dx,6780h
        jz      yeste
        cmp     dx,2360h
        jnz     note
yeste:  bkadr   trident,$trident, everex_entries        ; Bert
        mov     everex,0
note:   jmp     fini

noev:
        mov     ax,0bf03h               ;Test for Compaq
        xor     bx,bx
        mov     cx,bx
        int     10h
        cmp     ax,0bf03h
        jnz     nocp
        test    cl,40h                  ;is 640x480x256 available? ;(??)
        jz      nocp
        bkadr   compaq,$compaq,compaq_entries           ; Bert
        mov     [vga512],1
        jmp     fini

nocp:   mov     dx,3c4h                 ;Test for NCR 77C22E
        mov     ax,0ff05h
        call    $isport2
        jnz     noncr
        mov     ax,5                    ;Disable extended registers
        out     dx,ax
        mov     ax,0ff10h               ;Try to write to extended register 10
        call    $isport2                ;If it writes then not NCR
        jz      noncr
        mov     ax,105h                 ;Enable extended registers
        out     dx,ax
        mov     ax,0ff10h
        call    $isport2
        jnz     noncr                   ;If it does NOT write then not NCR
        bkadr   ncr,$ncr,ncr_entries            ; Bert
        mov     [vga512],1
        jmp     fini

noncr:  mov     dx,3c4h                 ;Test for Trident
        mov     al,0bh
        out     dx,al
        inc     dl
        in      al,dx
        and     al,0fh
        cmp     al,06h
        ja      notri
        cmp     al,2
        jb      notri
        bkadr   trident,$trident, trident_entries       ; Bert
        cmp     al,3
        jb      no89
        mov     [t8900],1
        mov     dx,3d4h         ; (was 3d5h in version 17.2)
        mov     al,1fh
        out     dx,al
        inc     dx
        in      al,dx
        and     al,3
        cmp     al,1
        jb      notmem
        mov     [vga512],1
        je      notmem
        mov     [vga1024],1
notmem: jmp     fini

no89:   mov     [vga512],1
        jmp     fini

notri:  mov     ax,6f00h                ;Test for Video 7
        xor     bx,bx
        cld
        int     10h
        cmp     bx,'V7'
        jnz     nov7
        bkadr   video7,$video7, video7_entries          ; Bert
        mov     ax,6f07h
        cld
        int     10h
        and     ah,7fh
        cmp     ah,1
        jbe     temp_3
        mov     [vga512],1
temp_3: cmp     ah,3
        jbe     temp_4
        mov     [vga1024],1
temp_4: jmp     fini

nov7:   mov     dx,3d4h                 ;Test for GENOA GVGA
        mov     al,2eh                  ;check for Herchi Register top 6 bits
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        test    al,11111100b            ;top 6 bits should be zero
        jnz     nogn
        mov     ax,032eh                ;check for Herchi Register
        call    $isport2
        jnz     nogn
        mov     dx,3c4h
        mov     al,7
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        test    al,10001000b
        jnz     nogn
        mov     al,10h
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        and     al,00110000b
        cmp     al,00100000b
        jnz     nogn
        mov     dx,3ceh
        mov     ax,0ff0bh
        call    $isport2
        jnz     nogn
        mov     dx,3c4h                 ;check for memory segment register
        mov     ax,3f06h
        call    $isport2
        jnz     nogn
        mov     dx,3ceh
        mov     ax,0ff0ah
        call    $isport2
        jnz     nogn
        bkadr   genoa,$genoa, genoa_entries             ; Bert
        mov     [vga512],1
        jmp     fini

nogn:   call    $cirrus                 ;Test for Cirrus
        cmp     [cirrus],0
        je      noci
        jmp     fini

noci:   mov     dx,3ceh                 ;Test for Paradise
        mov     al,9                    ;check Bank switch register
        out     dx,al
        inc     dx
        in      al,dx
        dec     dx
        or      al,al
        jnz     nopd

        mov     ax,50fh                 ;turn off write protect on VGA registers
        out     dx,ax
        mov     dx,offset $pdrsub
        mov     cx,1
        call    $chkbk
        jc      nopd                    ;if bank 0 and 1 same not paradise
        bkadr   paradise,$paradise, paradise_entries    ; Bert
        mov     dx,3ceh
        mov     al,0bh                  ;512k detect from Bob Berry
        out     dx,al
        inc     dx
        in      al,dx
        test    al,80h                  ;if top bit set then 512k
        jz      nop512
        test    al,40h
        jz      nop1024
        mov     [vga1024],1
        jmp     fini
nop1024:
        mov     [vga512],1
nop512: jmp     fini

nopd:   mov     ax,5f00h                ;Test for Chips & Tech
        xor     bx,bx
        cld
        int     10h
        cmp     al,5fh
        jnz     noct
        bkadr   chipstech,$chipstech, chips_entries     ; Bert
        cmp     bh,1
        jb      temp_5
        mov     [vga512],1
temp_5:
        jmp     fini

noct:   mov     ch,0
        mov     dx,3dah                 ;Test for Tseng 4000 & 3000
        in      al,dx                   ;bit 8 is opposite of bit 4
        mov     ah,al                   ;(vertical retrace bit)
        shr     ah,1
        shr     ah,1
        shr     ah,1
        shr     ah,1
        xor     al,ah
        test    al,00001000b
;       jz      nots
        jnz     @F
        jmp     nots
@@:
        mov     dx,3d4h                 ;check for Tseng 4000 series
        mov     ax,0f33h
        call    $isport2
        jnz     not4
        mov     ax,0ff33h               ;top 4 bits should not be there
        call    $isport2
;       jz      nots
        jnz     @F
        jmp     nots
@@:
        mov     ch,1

not4:   mov     dx,3bfh                 ;Enable access to extended registers
        mov     al,3
        out     dx,al
        mov     dx,3d8h
        mov     al,0a0h
        out     dx,al
        cmp     ch,0
        jnz     yes4

        mov     dx,3d4h                 ;Test for Tseng 3000 or 4000
        mov     ax,1f25h                ;is the Overflow High register there?
        call    $isport2
        jnz     nots
        mov     al,03fh                 ;bottom six bits only
        jmp     short yes3
yes4:   mov     al,0ffh
yes3:   mov     dx,3cdh                 ;test bank switch register
        call    $isport1
        jnz     nots
        bkadr   tseng,$tseng, tseng_entries             ; Bert
        cmp     ch,0
        jnz     t4mem
;       mov     [vga512],1
        call    $t3memchk
        jmp     fini

t4mem:  mov     dx,3d4h                 ;Tseng 4000 memory detect 1meg
        mov     al,37h
        out     dx,al
        inc     dx
        in      al,dx
        test    al,1000b                ;if using 64kx4 RAMs then no more than 256k
        jz      nomem
        and     al,3
        cmp     al,1                    ;if 8 bit wide bus then only two 256kx4 RAMs
        jbe     nomem
        mov     [vga512],1
        cmp     al,2                    ;if 16 bit wide bus then four 256kx4 RAMs
        je      nomem
        mov     [vga1024],1             ;full meg with eight 256kx4 RAMs
nomem:  bkadr   tseng4,$tseng4, tseng4_entries          ; Bert
        jmp     fini

nots:
        mov     dx,3ceh         ;Test for Above A or B chipsets
        mov     ax,0ff0fh               ;register should not be fully available
        call    $isport2
        jz      noab
        mov     ax,200fh
        out     dx,ax
        inc     dx
        nojmp
        in      al,dx
        cmp     al,21h
        jz      verb
        cmp     al,20h
        jnz     noab
        bkadr   aheada,$aheada, ahead_entries           ; Bert
        mov     [vga512],1
        jmp     short fini

verb:   bkadr   aheadb,$aheadb, ahead_entries           ; Bert
        mov     [vga512],1
        jmp     short fini

noab:   mov     dx,3deh                 ;Test for Oak Technology
        mov     ax,0ff11h               ;look for bank switch register
        call    $isport2
        jnz     nooak
        bkadr   oaktech,$oaktech, oaktech_entries               ; Bert
        mov     al,0dh
        out     dx,al
        inc     dx
        nojmp
        in      al,dx
        test    al,11000000b
        jz      no4ram
        mov     [vga512],1
        test    al,01000000b
        jz      no4ram
        mov     [vga1024],1
no4ram: jmp     short fini

nooak:  mov     si,0

fini:   mov     ax,si
        pop     bp
        ret
whichvga endp


;Segment to access video buffer (based on GR[6])
buftbl  dw      0A000h,0A000h,0B000h,0B800h

$t3memchk proc near                     ;[Charles Marslett -- ET3000 memory ck]
        mov     dx,3dah
        in      al,dx                   ;Reset the attribute flop (read 0x3DA)
        mov     dx,03c0h
        mov     al,36h
        out     dx,al
        inc     dx
        in      al,dx                   ;Save contents of ATTR[0x16]
        push    ax
        or      al,10h
        dec     dx
        out     dx,al
        mov     dx,3ceh                 ;Find the RAM buffer...
        mov     al,6
        out     dx,al
        inc     dx
        in      al,dx
        and     ax,000Ch
        shr     ax,1

        mov     bx,ax
        push    es
        mov     es,cs:buftbl[bx]
        mov     ax,09C65h
        mov     bx,1
        mov     es:[bx],ax
        mov     es:[bx+2],ax
        inc     bx
        mov     ax,es:[bx]
        pop     es
        cmp     ax,0659Ch
        jne     et3k_256
        mov     [vga512],1
et3k_256:
        mov     dx,3c0h
        mov     al,36h
        out     dx,al
        pop     ax
        out     dx,al                   ;Restore ATTR[16h]
        ret
$t3memchk endp


$cirrus proc    near
        mov     dx,3d4h         ; assume 3dx addressing
        mov     al,0ch          ; screen a start address hi
        out     dx,al           ; select index
        inc     dx              ; point to data
        mov     ah,al           ; save index in ah
        in      al,dx           ; get screen a start address hi
        xchg    ah,al           ; swap index and data
        push    ax              ; save old value
        push    dx              ; save crtc address
        xor     al,al           ; clear crc
        out     dx,al           ; and out to the crtc

        mov     al,1fh          ; Eagle ID register
        dec     dx              ; back to index
        out     dx,al           ; select index
        inc     dx              ; point to data
        in      al,dx           ; read the id register
        mov     bh,al           ; and save it in bh

        mov     cl,4            ; nibble swap rotate count
        mov     dx,3c4h         ; sequencer/extensions
        mov     bl,6            ; extensions enable register

        ror     bh,cl           ; compute extensions disable value
        mov     ax,bx           ; extensions disable
        out     dx,ax           ; disable extensions
        inc     dx              ; point to data
        in      al,dx           ; read enable flag
        or      al,al           ; disabled ?
        jnz     exit            ; nope, not an cirrus

        ror     bh,cl           ; compute extensions enable value
        dec     dx              ; point to index
        mov     ax,bx           ; extensions enable
        out     dx,ax           ; enable extensions
        inc     dx              ; point to data
        in      al,dx           ; read enable flag
        cmp     al,1            ; enabled ?
        jne     exit            ; nope, not an cirrus
        mov     [cirrus],1
        mov     video_entries, offset no_entries        ; Bert
        mov     [bankadr],offset $nobank
if @CodeSize
        mov     [bankseg],seg $nobank
endif
exit:   pop     dx              ; restore crtc address
        dec     dx              ; point to index
        pop     ax              ; recover crc index and data
        out     dx,ax           ; restore crc value
        ret
$cirrus endp

$chkbk  proc    near            ;paradise bank switch check
        mov     di,0b800h
        mov     es,di
        xor     di,di
        mov     bx,1234h
        call    $gochk
        jnz     nopd
        mov     bx,4321h
        call    $gochk
        jnz     nopd
        clc
        ret
nopd:   stc
        ret
$chkbk  endp

$gochk  proc    near
        push    si
        mov     si,bx

        mov     al,cl
        call    dx
        xchg    bl,es:[di]
        mov     al,ch
        call    dx
        xchg    bh,es:[di]

        xchg    si,bx

        mov     al,cl
        call    dx
        xor     bl,es:[di]
        mov     al,ch
        call    dx
        xor     bh,es:[di]

        xchg    si,bx

        mov     al,ch
        call    dx
        mov     es:[di],bh
        mov     al,cl
        call    dx
        mov     es:[di],bl

        mov     al,0
        call    dx
        or      si,si
        pop     si
        ret
$gochk  endp


$pdrsub proc    near            ;Paradise
        push    dx
        mov     ah,al
        mov     dx,3ceh
        mov     al,9
        out     dx,ax
        pop     dx
        ret
$pdrsub endp

$isport2 proc   near
        push    bx
        mov     bx,ax
        out     dx,al
        mov     ah,al
        inc     dx
        in      al,dx
        dec     dx
        xchg    al,ah
        push    ax
        mov     ax,bx
        out     dx,ax
        out     dx,al
        mov     ah,al
        inc     dx
        in      al,dx
        dec     dx
        and     al,bh
        cmp     al,bh
        jnz     noport
        mov     al,ah
        mov     ah,0
        out     dx,ax
        out     dx,al
        mov     ah,al
        inc     dx
        in      al,dx
        dec     dx
        and     al,bh
        cmp     al,0
noport: pop     ax
        out     dx,ax
        pop     bx
        ret
$isport2 endp

$isport1 proc   near
        mov     ah,al
        in      al,dx
        push    ax
        mov     al,ah
        out     dx,al
        in      al,dx
        and     al,ah
        cmp     al,ah
        jnz     noport
        mov     al,0
        out     dx,al
        in      al,dx
        and     al,ah
        cmp     al,0
noport: pop     ax
        out     dx,al
        ret
$isport1 endp

videowrite      proc    near            ; your-own-video write routine
        mov     ah,0                    ; clear the high-order color byte
        push    ax                      ; colors parameter
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr writevideo      ; let the external routine do it
        add     sp,6                    ; pop the parameters
        ret                             ; we done.
videowrite      endp

videoread       proc    near            ; your-own-video read routine
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr readvideo       ; let the external routine do it
        add     sp,4                    ; pop the parameters
        ret                             ; we done.
videoread       endp

diskwrite       proc    near            ; disk-video write routine
        push    ax                      ; colors parameter
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr writedisk       ; let the external routine do it
        add     sp,6                    ; pop the parameters
        ret                             ; we done.
diskwrite       endp

diskread        proc    near            ; disk-video read routine
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr readdisk        ; let the external routine do it
        add     sp,4                    ; pop the parameters
        ret                             ; we done.
diskread        endp


; ***********************************************************************
;
; TARGA MODIFIED 1 JUNE 89 - j mclain
;
tgawrite        proc    near
        push    ax                      ; colors parameter
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr WriteTGA        ; writeTGA( x, y, color )
        add     sp,6                    ; pop the parameters
        ret
tgawrite        endp

tgaread         proc    near
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr ReadTGA         ; readTGA( x, y )
        add     sp,4                    ; pop the parameters
        ret
tgaread endp


; TARGA+ Code 2-11-91, Mark Peterson

TPlusWrite      PROC    NEAR
        push    ax
        push    dx
        push    cx
        call    FAR PTR WriteTPlusBankedPixel
        add     sp, 6
        ret
TPlusWrite     ENDP

TPlusRead      PROC    NEAR
        push    dx
        push    cx
        call    FAR PTR ReadTPlusBankedPixel
        add     sp, 4
        ret
TPlusRead      ENDP

; 8514/a afi routines JCO, not needed, 4/11/92
;f85start    proc    near
;       call   far ptr open8514
;       ret
;f85start    endp

;f85end proc    near
;       call   far ptr close8514
;       ret
;f85end endp

; hardware
f85hwwrite     proc    near
       call   far ptr fr85hwwdot
       ret
f85hwwrite    endp

f85hwread proc   near
       call   far ptr fr85hwrdot
       ret
f85hwread endp

f85hwline proc   near
       call    far ptr fr85hwwbox        ;put out the box
       ret
f85hwline endp

f85hwreadline proc    near
       call    far ptr fr85hwrbox        ;read the box
       ret
f85hwreadline endp

; afi
f85write     proc    near
       call   far ptr fr85wdot
       ret
f85write    endp

f85read proc   near
       call   far ptr fr85rdot
       ret
f85read endp

f85line proc   near
       call    far ptr fr85wbox        ;put out the box
       ret
f85line endp

f85readline proc    near
       call    far ptr fr85rbox        ;read the box
       ret
f85readline endp

hgcwrite proc near
        mov     ah,0                    ; clear the high-order color byte
        push    ax                      ; colors parameter
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr writehgc        ; let the Herc. Write dot routine do it
        add     sp,6                    ; pop the parameters
        ret
hgcwrite endp

hgcread proc near
        push    dx                      ; 'y' parameter
        push    cx                      ; 'x' parameter
        call    far ptr readhgc         ; call the Hercules Read dot routine
        add     sp,4                    ; pop the parameters
        ret
hgcread endp

hgcstart        proc    near            ; hercules start routine
        call    far ptr inithgc         ; let the external routine do it
        ret                             ; we done.
hgcstart        endp

hgcend          proc    near            ; hercules end routine
        call    far ptr termhgc         ; let the external routine do it
        ret                             ; we done.
hgcend          endp

; **************** video adapter initialization *******************
;
; adapter_init:
;       called from general.asm once per run

adapter_init    proc    far             ; initialize the video adapter (to VGA)
        mov     ax,[bankadr]            ; Initialize the bank-switching
        mov     video_bankadr,ax        ;  logic to the do-nothing routine
        mov     ax,[bankseg]            ;  ...
        mov     video_bankseg,ax        ;  ...
        mov     bx,0                    ; clear out all of the 256-mode flags
        mov     tseng,bx                ;  ...
        mov     trident,bx              ;  ...
        mov     video7,bx               ;  ...
        mov     paradise,bx             ;  ...
        mov     chipstech,bx    ;  ...
        mov     ativga,bx               ;  ...
        mov     everex,bx               ;  ...
        mov     cirrus,bx               ;  ...
        mov     aheada,bx               ;  ...
        mov     aheadb,bx               ;  ...
        mov     tseng4,bx               ;  ...
        mov     oaktech,bx              ;  ...
        mov     [bankadr],offset $nobank
        mov     [bankseg],seg $nobank
        mov     video_entries, offset no_entries        ; ...
        ret
adapter_init    endp

; adapter_detect:
;       This routine performs a few quick checks on the type of
;       video adapter installed.
;       It sets variables video_type and textsafe,
;       and fills in a few bank-switching routines.

adapter_detect  proc    uses di si es
        push    bp                      ; some bios's don't save during int 10h
        cmp     done_detect,0           ; been called already?
        je      adapter_detect2         ;  nope
        jmp     adapter_ret             ; yup, do nothing
adapter_detect2:
        inc     done_detect             ; don't get called again

        cmp     video_type,0            ; video_type preset by command line arg?
        jne     go_adapter_set          ;  yup, use what we're told

        cmp     TPlusFlag, 0
        je      NotTPlus
        call    far ptr CheckForTPlus
        or      ax, ax
        jz      NotTPlus
        mov     TPlusInstalled, 1       ; flag it and check for primary adapter

NotTPlus:
        mov     ax,1a00h                ; start by trying int 10 func 1A
        int     10h                     ;  ...
        cmp     al,1ah                  ; was AL modified?
        je      adapter_detect_4        ;  yup. go decode what we got
        mov     ax,1200h                ; try this vga-only function
        mov     bl,34h                  ;  enable cursor emulation
        int     10h                     ;  ...
        cmp     al,12h                  ; did it work?
        je      adapter_detect_vga      ;  yup, vga
        mov     ah,12h                  ; look for an EGA
        mov     bl,10h                  ;  by using an EGA-specific call
        int     10h                     ;  ...
        cmp     bl,10h                  ; was BL modified?
        je      adapter_detect_notega   ;  nope, < EGA
        mov     video_type,3            ; set the video type: EGA
        cmp     bh,1                    ; monochrome monitor?
        jne     go_adapter_set          ;  nope
        mov     mode7text,1             ; yup, use mode 7 for text
        jmp     short go_adapter_set    ; We done.
adapter_detect_4:
        cmp     bl,1                    ; =1?
        jne     adapter_detect_4a       ;  nope
        jmp     adapter_detect_hgc      ; MDA, assume HGC (nothing else works)
adapter_detect_4a:
        mov     video_type,2            ; set the video type: CGA
        cmp     bl,3                    ; <=2?
        jb      go_adapter_set          ;  exit with type CGA
        mov     video_type,3            ; set the video type: EGA
        cmp     bl,5                    ; =5?
        jne     adapter_detect_5        ;  nope
        mov     mode7text,1             ; yup, monochrome monitor, mode 7 text
go_adapter_set:
        jmp     adapter_set
adapter_detect_5:
        cmp     bl,6                    ; <=5?
        jb      go_adapter_set          ;  exit with type EGA
        cmp     bl,10                   ; <=9?
        jb      adapter_detect_vga      ;  vga, go check which kind
        mov     video_type,4            ; set the video type: MCGA
        cmp     bl,13                   ; <=12?
        jb      go_adapter_set          ;  exit with type MCGA
adapter_detect_vga:
        mov     video_type,5            ; set the video type: VGA
        call    whichvga                ; autodetect which VGA is there
        mov     ax,[bankadr]            ; save the results
        mov     video_bankadr,ax        ;  ...
        mov     ax,[bankseg]            ;  ...
        mov     video_bankseg,ax        ;  ...
        jmp     adapter_set
adapter_detect_notega:
        mov     video_type,2            ; set the video type: CGA
        ; HGC detect code from book by Richard Wilton follows
        mov     dx,3B4h                 ; check for MDA, use MDA CRTC address
        mov     al,0Fh                  ; select 6845 reg 0Fh (Cursor Low)
        out     dx,al
        inc     dx
        in      al,dx                   ; AL := current Cursor Low value
        mov     ah,al                   ; preserve in AH
        mov     al,66h                  ; AL := arbitrary value
        out     dx,al                   ; try to write to 6845
        mov     cx,200h
mdalp:  loop    mdalp                   ; wait for 6845 to respond
        in      al,dx                   ; read cursor low again
        xchg    ah,al
        out     dx,al                   ; restore original value
        cmp     ah,66h                  ; test whether 6845 responded
        jne     adapter_set             ;  nope, exit with type CGA
        mov     dl,0BAh                 ; DX := 3BAh (status port)
        in      al,dx
        and     al,80h
        mov     ah,al                   ; AH := bit 7 (vertical sync on HGC)
        mov     cx,8000h                ; do this 32768 times
mdalp2: in      al,dx
        and     al,80h                  ; isolate bit 7
        cmp     ah,al
        loope   mdalp2                  ; wait for bit 7 to change
        je      adapter_set             ;  didn't change, exit with type CGA
;;      in      al,dx
;;      and     al,01100000b            ; mask off bits 5 and 6
;; Next line probably backwards but doesn't matter, the test in this area
;; distinguishes HGC/HGC+/InColor, which we don't care about anyway.
;;      jnz     adapter_set             ; not hgc/hgc+, exit with type CGA
adapter_detect_hgc:
        mov     video_type,1            ; HGC
        mov     mode7text,1             ; use mode 7 for text

adapter_set:
        ; ensure a nice safe standard state
        mov     ax,3                    ; set 80x25x16 text mode, clear screen
        cmp     mode7text,0             ; use mode 7 for text?
        je      adapter_set2            ;  nope
        mov     ax,7                    ; set mono text mode, clear screen
adapter_set2:
        int     10h                     ; set text mode
        mov     ax,0500h                ; select display page zero
        int     10h                     ;  ...

        ; now the color text stuff
        cmp     textsafe,2              ; command line textsafe=no?
        je      adapter_go_ret          ;  yup, believe the user
        cmp     video_type,3            ; >= ega?
        jae     adapter_setup           ;  yup
        mov     textsafe,2              ; textsafe=no
adapter_go_ret:                         ; a label for some short jumps
        jmp     adapter_ret             ;  to the exit

adapter_setup:
        ; more standard state, ega and up stuff
        mov     ax,1003h                ; top attribute bit means blink
        mov     bl,01h                  ;  ...
        int     10h                     ;  ...
        mov     ax,1103h                ; font block 0, 256 chars (not 512)
        mov     bl,00h                  ;  ...
        int     10h                     ;  ...
        mov     ax,1202h                ; 400 scan lines in text mode (vga)
        mov     bl,30h                  ;  ...
        int     10h                     ;  ...
        mov     ax,1200h                ; cga cursor emulation (vga)
        mov     bl,34h                  ;  ...
        int     10h                     ;  ...
        mov     ax,1200h                ; enable default palette loading
        mov     bl,31h                  ;  ...
        int     10h                     ;  ...
        cmp     textsafe,0              ; were we told textsafe=yes|bios|save?
        jne     adapter_ret             ;  yup
        mov     textsafe,1              ; set textsafe=yes
adapter_ret:
        cld                             ; some MSC 6.0 libraries assume this!
        pop     bp
        ret
adapter_detect  endp


; select_vga_plane:
;       Call this routine with cx = plane number.
;       It works for vga and for ega.  (I hope.)
;       It uses no local variables, caller may have ds register modified.
;       On return from this routine, the requested vid mem plane is mapped
;       to A0000;  this means that the sequencer and graphics controller
;       states are not very useful for further real work - before any further
;       screen painting, better reset video mode.

select_vga_plane proc near              ; cl = plane number
        ; some callers may have ds modified, use no variables in here!
        mov     dx,SC_INDEX             ; sequencer controller
        mov     ax,0102h                ; select plane
        shl     ah,cl                   ;  bit for desired plane
        out     dx,ax                   ;  ...
        mov     ax,0604h                ; no chaining
        out     dx,ax                   ;  ...
        mov     dx,GC_INDEX             ; graphics controller
        mov     ax,0001h                ; use processor data
        out     dx,ax                   ;  ...
        mov     al,  04h                ; select read plane
        mov     ah,cl                   ;  desired plane
        out     dx,ax                   ;  ...
        mov     ax,0005h                ; no even/odd, write mode 0
        out     dx,ax                   ;  ...
        mov     ax,0106h                ; map to a000, no chain, graphics
        out     dx,ax                   ;  ...
        mov     ax,0ff08h               ; enable 8 bits per write
        out     dx,ax                   ;  ...
        ret                             ; all done
select_vga_plane endp


; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
;       They assume the following register values:
;
;               si = offset of array of colors for a row (write routines)
;               di = offset of array of colors for a row (read routines)
;
;               ax = stopping column
;               bx =
;               cx = starting column
;               dx = row
;
; Note: so far have converted only normaline, normalineread, mcgaline,
;       mcgareadline, super256line, super256readline -- Tim

normaline       proc    near            ; Normal Line
normal_line1:
        push    ax                      ; save stop col
        mov     al,[si]                 ; retrieve the color
        xor     ah,ah                   ; MCP 6-7-91
        push    cx                      ; save the counter around the call
        push    dx                      ; save column around the call
        push    si                      ; save the pointer around the call also
        call    dotwrite                ; write the dot via the approved method
        pop     si                      ; restore the pointer
        pop     dx                      ; restore the column
        pop     cx                      ; restore the counter
        inc     si                      ; bump it up
        inc     cx                      ; bump it up
        pop     ax                      ; retrieve number of dots
        cmp     cx,ax                   ; more to go?
        jle     normal_line1            ; yup.  do it.
        ret
normaline       endp

normalineread   proc    near            ; Normal Line
        mov     bx,videomem
        mov     es,bx
normal_lineread1:
        push    ax                      ; save stop col
        push    cx                      ; save the counter around the call
        push    dx                      ; save column around the call
        push    di                      ; save the pointer around the call also
        call    dotread                 ; read the dot via the approved method
        pop     di                      ; restore the pointer
        pop     dx                      ; restore the column
        pop     cx                      ; restore the counter
        mov     bx,di                   ; locate the actual pixel color
        mov     [bx],al                 ; retrieve the color
        inc     di                      ; bump it up
        inc     cx                      ; bump it up
        pop     ax                      ; retrieve number of dots
        cmp     cx,ax                   ; more to go?
        jle     normal_lineread1        ; yup.  do it.
        ret
normalineread   endp

mcgaline        proc    near            ; MCGA 320*200, 246 colors
        sub     ax,cx                   ; last col - first col
        inc     ax                      ;   + 1

        xchg    dh,dl                   ; bx := 256*y
        mov     bx,cx                   ; bx := x
        add     bx,dx                   ; bx := 256*y + x
        shr     dx,1
        shr     dx,1                    ; dx := 64*y
        add     bx,dx                   ; bx := 320*y + x
        mov     di,bx                   ; di = offset of row in video memory

        mov     cx,ax                   ; move this many bytes
        rep     movsb                   ; zap line into memory
        ret
mcgaline        endp

mcgareadline    proc    near            ; MCGA 320*200, 246 colors

        sub     ax,cx                   ; last col - first col
        inc     ax                      ;   + 1

        xchg    dh,dl                   ; bx := 256*y
        mov     bx,cx                   ; bx := x
        add     bx,dx                   ; bx := 256*y + x
        shr     dx,1
        shr     dx,1                    ; dx := 64*y
        add     bx,dx                   ; bx := 320*y + x
        mov     si,bx                   ; di = offset of row in video memory

        mov     cx,ax                   ; move this many bytes
        mov     ax,ds                   ; copy data segment to ...
        mov     es,ax                   ;  ... es
        mov     ax,videomem             ; copy video segment to ...
        mov     ds,ax                   ;  ... ds
        rep     movsb                   ; zap line into memory
        mov     ax,es
        mov     ds,ax                   ; restore data segement to ds
        ret
mcgareadline    endp

vgaline proc    near            ; Bank Switch EGA/VGA line write
        push    cx                      ; save a few registers
        push    ax                      ;  ...
        push    dx                      ;  ...

        mov     bx,dx                   ; save the rowcount
        mov     ax,vxdots               ; compute # of dots / pass
        shr     ax,1                    ;  (given 8 passes)
        shr     ax,1                    ;  ...
        shr     ax,1                    ;  ...
        mov     di,ax
        neg     di                      ; temp: to see if line will overflow
        mul     bx                      ; now calc first video addr
        cmp     dx,curbk                ; see if bank changed
        jne     bank_is_changing        ; if bank change call normaline
        cmp     ax,di
        ja      bank_is_changing        ; if bank WILL change, call normaline

        mov     di,cx                   ; compute the starting destination
        shr     di,1                    ; divide by 8
        shr     di,1                    ;  ...
        shr     di,1                    ;  ...
        add     di,ax                   ; add the first pixel offset

        mov     dx,03ceh                ; set up graphics cntrlr addr
        mov     ax,8008h                ; set up for the bit mask
        and     cx,7                    ; adjust for the first pixel offset
        ror     ah,cl                   ;  ...

        pop     bx                      ; flush old DX value
        pop     bx                      ; flush old AX value
        pop     cx                      ; flush old CX value
        sub     bx,cx                   ; convert to a length value
        add     bx,si                   ; locate the last source locn

        mov     cx,ax                   ; save the bit mask

vgaline1:
        out     dx,ax                   ; set the graphics bit mask
        push    ax                      ; save registers for a tad
        push    si                      ;  ...
        push    di                      ;  ...
vgaline2:
        mov     ah,ds:[si]              ; get the color
        mov     al,0                    ; set set/reset registers
        out     dx,ax                   ;  do it.
        mov     ax,0f01h                ; enable set/reset registers
        out     dx,ax                   ;  do it.
        or      es:[di],al              ; update all bit planes
        inc     di                      ; set up the next video addr
        add     si,8                    ;  and the next source addr
        cmp     si,bx                   ; are we beyond the end?
        jbe     vgaline2                ; loop if more dots this pass
        pop     di                      ; restore the saved registers
        pop     si                      ;  ...
        pop     ax                      ;  ...
        inc     si                      ; offset the source 1 byte
        cmp     si,bx                   ; are we beyond the end?
        ja      vgaline4                ; stop if no more dots this pass
        ror     ah,1                    ; alter bit mask value
        cmp     ah,80h                  ; time to update DI:
        jne     vgaline3                ;  nope
        inc     di                      ;  yup
vgaline3:
        cmp     ah,ch                   ; already done all 8 of them?
        jne     vgaline1                ;  nope.  do another one.
vgaline4:
;;;     call    videocleanup            ; else cleanup time.
        ret                             ;  and we done.

bank_is_changing:
        pop     dx                      ; restore the registers
        pop     ax                      ;  ...
        pop     cx                      ;  ...
        call    normaline               ; just calling newbank didn't quite
        ret                             ;  work. This depends on no bank
vgaline endp                            ;  change mid line (ok for 1024 wide)

vgareadline     proc    near            ; Bank Switch EGA/VGA line read
        push    cx                      ; save a few registers
        push    ax                      ;  ...
        push    dx                      ;  ...

        mov     bx,dx                   ; save the rowcount
        mov     ax,vxdots               ; compute # of dots / pass
        shr     ax,1                    ;  (given 8 passes)
        shr     ax,1                    ;  ...
        shr     ax,1                    ;  ...
        mul     bx                      ; now calc first video addr
        cmp     dx,curbk                ; see if bank changed
        jne     bank_is_changing        ; if bank change call normaline

        mov     si,cx                   ; compute the starting destination
        shr     si,1                    ; divide by 8
        shr     si,1                    ;  ...
        shr     si,1                    ;  ...
        add     si,ax                   ; add the first pixel offset

        and     cx,7                    ; adjust for the first pixel offset
        mov     ch,cl                   ; save the original offset value

        pop     bx                      ; flush old DX value
        pop     bx                      ; flush old AX value
        pop     ax                      ; flush old CX value
        sub     bx,ax                   ; convert to a length value
        add     bx,di                   ; locate the last dest locn

        mov     ax,0a000h               ; EGA/VGA screen starts here
        mov     es,ax                   ;  ...

        mov     dx,03ceh                ; set up graphics cntrlr addr

vgaline1:
        push    bx                      ; save BX for a tad
        mov     ch,80h                  ; bit mask to shift
        shr     ch,cl                   ;  ...
        mov     bx,0                    ; initialize bits-read value (none)
        mov     ax,0304h                ; set up controller address register
vgareadloop:
        out     dx,ax                   ; do it
        mov     bh,es:[si]              ; retrieve the old value
        and     bh,ch                   ; mask one bit
        neg     bh                      ; set bit 7 correctly
        rol     bx,1                    ; rotate the bit into bl
        dec     ah                      ; go for another bit?
        jge     vgareadloop             ;  sure, why not.
        mov     ds:[di],bl              ; returned pixel value
        pop     bx                      ; restore BX
        inc     di                      ; set up the next dest addr
        cmp     di,bx                   ; are we beyond the end?
        ja      vgaline3                ;  yup.  We done.
        inc     cl                      ; alter bit mask value
        cmp     cl,8                    ; time to update SI:
        jne     vgaline2                ;  nope
        inc     si                      ;  yup
        mov     cl,0                    ;  ...
vgaline2:
        jmp     short vgaline1          ;  do another one.

vgaline3:
;;;     call    videocleanup            ; else cleanup time.
        ret                             ;  and we done.

bank_is_changing:
        pop     dx                      ; restore the registers
        pop     ax                      ;  ...
        pop     cx                      ;  ...
        call    normalineread           ; just calling newbank didn't quite
        ret                             ;  work. This depends on no bank
vgareadline     endp                    ;  change mid line (ok for 1024 wide)

super256lineaddr    proc    near                ; super VGA 256 colors
        mov     ax,vxdots        ; this many dots / line
        mov     bx,dx            ; rowcount
        mul     bx               ; times this many lines
        push    ax               ; save pixel address for later
        cmp     dx,curbk         ; bank ok?
        push    dx               ; save bank
        je      bank_is_ok       ; jump if bank ok
        mov     al,dl            ; newbank needs bank in al
        call    far ptr newbank
bank_is_ok:
        inc     bx               ; next row
        mov     ax,vxdots        ; this many dots / line
        mul     bx               ; times this many lines
        sub     ax,1             ; back up some to the last pixel of the
        sbb     dx,0             ; previous line
        pop     bx               ; bank at start of row
        pop     ax               ; ax = offset of row in video memory
        ret
super256lineaddr endp

super256line    proc    near            ; super VGA 256 colors
        push    ax                      ; stop col
        push    dx                      ; row
        call super256lineaddr           ; ax=video,dl=newbank,bl=oldbank
        mov     di,ax                   ; video offset
        cmp     dl,bl                   ; did bank change?
        pop     dx                      ; row
        pop     ax                      ; stop col
        jne     bank_did_chg
        add     di,cx                   ; add start col to video address
        sub     ax,cx                   ; ax = stop - start
        mov     cx,ax                   ;  + start column
        inc     cx                      ; number of bytes to move
        rep     movsb                   ; zap line into memory
        jmp     short linedone
bank_did_chg:
        call    normaline               ; normaline can handle bank change
linedone:
        ret
super256line    endp

super256readline    proc    near     ; super VGA 256 colors
        push    ax                      ; stop col
        push    dx                      ; row
        call super256lineaddr           ; ax=video,dl=newbank,bl=oldbank
        mov     si,ax                   ; video offset
        cmp     dl,bl                   ; did bank change?
        pop     dx                      ; row
        pop     ax                      ; stop col
        jne     bank_did_chg

        add     si,cx                   ; add start col to video address
        sub     ax,cx                   ; ax = stop - start
        mov     cx,ax                   ;  + start column
        inc     cx                      ; number of bytes to move
        mov     ax,ds                   ; save data segment to es
        mov     es,ax
        mov     ax,videomem             ; video segment to es
        mov     ds,ax
        rep     movsb                   ; zap line into memory
        mov     ax,es                   ; restore data segment to ds
        mov     ds,ax
        jmp     short linedone
bank_did_chg:
        call    normalineread           ; normaline can handle bank change
linedone:
        ret
super256readline    endp

tweak256line    proc    near            ; Normal Line:  no assumptions
  local plane:byte
        mov     bx,ax                   ; bx = stop col
        sub     bx,cx                   ; bx = stop-start
        inc     bx                      ; bx = how many pixels to write
        cmp     bx,3                    ; less than four points?
        jg      nottoosmall             ; algorithm won't work as written
        call    normaline               ;  - give up and call normaline
        ret                             ; we done
nottoosmall:                            ; at least four points - go for it!
        push    bx                      ; save number of pixels
        and     bx,3                    ;  pixels modulo 4 = no of extra pts
        mov     ax,vxdots               ; width of video row
;;      shr     ax, 1
;;      shr     ax, 1                   ; now ax = vxdots/4
        mul     dx                      ; ax points to start of desired row
        push    cx                      ; Save starting column for later
        shr     cx,1                    ; There are 4 pixels at each address
        shr     cx,1                    ;   so divide X by 4
        add     ax,cx                   ; Point to pixel's address
        mov     di,ax                   ; video offset of first point
        pop     cx                      ; Retrieve starting column
        and     cl,3                    ; Get the plane number of the pixel
        mov     ah,1
        shl     ah,cl                   ; Set the bit corresponding to the plane
                                        ;  the pixel is in
        mov     plane,ah                ; Save starting plane for ending test
        mov     al,MAP_MASK             ;
        mov     dx,SC_INDEX
        pop     cx                      ; number of pixels to write
        shr     cx,1
        shr     cx,1                    ; cx = number of pixels/4
        cmp     bx,0                    ; extra pixels?
        je      tweak256line1           ; nope - don't add one
        inc     cx                      ; yup - add one more pixel
tweak256line1:
        OUT      DX,AX                  ; set up VGA registers for plane
        push    cx                      ; save registers changed by movsb
        push    si                      ;  ...
        push    di                      ;  ...
tweak256line2:
        movsb                           ; move the next pixel
        add     si,3                    ; adjust the source addr (+4, not +1)
        loop    tweak256line2           ; loop if more dots this pass
        pop     di                      ; restore the saved registers
        pop     si                      ;  ...
        pop     cx                      ;  ...
        dec     bx                      ; one less extra pixel
        cmp     bx,0                    ; out of extra pixels?
        jne     noextra
        dec     cx                      ; yup - next time one fewer to write
noextra:
        inc     si                      ; offset the source 1 byte
        shl     ah,1                    ; set up for the next video plane
        cmp     ah,16                   ; at last plane?
        jne     notlastplane
        mov     ah,1                    ; start over with plane 0
        inc     di                      ; bump up video memory
notlastplane:
        cmp     ah,plane                ; back to first plane?
        jne     tweak256line1           ;  nope.  perform another loop.
        ret
tweak256line    endp

tweak256readline        proc    near            ; Normal Line:  no assumptions
  local plane:byte
        mov     bx,ax                   ; bx = stop col
        sub     bx,cx                   ; bx = stop-start
        inc     bx                      ; bx = how many pixels to write
        cmp     bx,3                    ; less than four points?
        jg      nottoosmall             ; algorithm won't work as written
        call    normalineread           ;  - give up and call normalineread
        ret                             ; we done
nottoosmall:                            ; at least four points - go for it!
        push    bx                      ; save number of pixels
        and     bx,3                    ;  pixels modulo 4 = no of extra pts
        mov     ax,vxdots               ; width of video row
;;      shr     ax, 1
;;      shr     ax, 1                   ; now ax = vxdots/4
        mul     dx                      ; ax points to start of desired row
        push    cx                      ; Save starting column for later
        shr     cx,1                    ; There are 4 pixels at each address
        shr     cx,1                    ;   so divide X by 4
        add     ax,cx                   ; Point to pixel's address
        mov     si,ax
        pop     cx                      ; Retrieve starting column
        and     cl,3                    ; Get the plane number of the pixel
        mov     ah,cl
        mov     plane,ah                ; Save starting plane
        mov     al,READ_MAP
        mov     dx,GC_INDEX
        pop     cx                      ; number of pixels to write
        shr     cx,1
        shr     cx,1                    ; cx = number of pixels/4
        cmp     bx,0                    ; extra pixels?
        je      tweak256line1           ; nope - don't add one
        inc     cx                      ; yup - add one more pixel
tweak256line1:
        out     dx,ax
        push    ax                      ; save registers
        push    cx                      ;  ...
        push    di                      ;  ...
        push    si                      ;  ...
        mov     ax,ds                   ; copy data segment to es
        mov     es,ax                   ;  ...
        mov     ax,videomem             ; copy video segment to ds
        mov     ds,ax                   ;  ...
tweak256line2:
        movsb                           ; move the next pixel
        add     di,3                    ; adjust the source addr (+4, not +1)
        loop    tweak256line2           ; loop if more dots this pass
        mov     ax,es
        mov     ds,ax                   ; restore data segement to ds
        pop     si                      ; restore the saved registers
        pop     di                      ;  ...
        pop     cx                      ;  ...
        pop     ax                      ;  ...
        dec     bx                      ; one less extra pixel
        cmp     bx,0                    ; out of extra pixels?
        jne     noextra
        dec     cx                      ; yup - next time one fewer to write
noextra:
        inc     di                      ; offset the source 1 byte
        inc     ah                      ; set up for the next video plane
        and     ah,3
        cmp     ah,0                    ; at last plane?
        jne     notlastplane
        inc     si                      ; bump up video memory
notlastplane:
        cmp     ah,plane                ; back to first plane?
        jne     tweak256line1           ;  nope.  perform another loop.
        ret
tweak256readline        endp


; ******************** Function videocleanup() **************************

;       Called at the end of any assembler video read/writes to make
;       the world safe for 'printf()'s.
;       Currently, only ega/vga needs cleanup work, but who knows?
;

;;videocleanup    proc    near
;;        mov     ax,dotwrite             ; check: were we in EGA/VGA mode?
;;        cmp     ax,offset vgawrite      ;  ...
;;        jne     short videocleanupdone  ; nope.  no adjustments
;;        mov     dx,03ceh                ; graphics controller address
;;        mov     ax,0ff08h               ; restore the default bit mask
;;        out     dx,ax                   ; ...
;;        mov     ax,0003h                ; restore the function select
;;        out     dx,ax                   ;  ...
;;        mov     ax,0001h                ; restore the enable set/reset
;;        out     dx,ax                   ;  ...
;;videocleanupdone:
;;        ret
;;videocleanup    endp

; ********************** Function setvideotext() ************************

;       Sets video to text mode, using setvideomode to do the work.

setvideotext    proc
        sub     ax,ax
        mov     dotmode,ax              ; make this zero to avoid trouble
        push    ax
        push    ax
        push    ax
        mov     ax,3
        push    ax
        call    far ptr setvideomode    ; (3,0,0,0)
        add     sp,8
        ret
setvideotext    endp


; **************** Function setvideomode(ax, bx, cx, dx) ****************

;       This function sets the (alphanumeric or graphic) video mode
;       of the monitor.   Called with the proper values of AX thru DX.
;       No returned values, as there is no particular standard to
;       adhere to in this case.

;       (SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;       genuine VGA or register compatable adapter and program the registers
;       directly using the coded value in DX)

setvideomode    proc    uses di si es,argax:word,argbx:word,argcx:word,argdx:word

        mov     ax,sxdots               ; initially, set the virtual line
        mov     vxdots,ax               ; to be the scan line length
        xor     ax,ax
        mov     istruecolor,ax          ; assume not truecolor
        mov     vesa_xres,ax            ; reset indicators used for
        mov     vesa_yres,ax            ;  virtual screen limits estimation

        cmp     dotmode,0
        je      its_text
        mov     setting_text,0          ; try to set virtual stuff
        jmp     short its_graphics
its_text:
        mov     setting_text,1          ; don't set virtual stuff
its_graphics:

        cmp     dotmode, 29		; Targa truecolor mode?
        jne     NotTrueColorMode
        jmp     TrueColorAuto		; yup.
NotTrueColorMode:
        cmp     diskflag,1              ; is disk video active?
        jne     nodiskvideo             ;  nope.
        call    far ptr enddisk         ; yup, external disk-video end routine
nodiskvideo:
        cmp     videoflag,1             ; say, was the last video your-own?
        jne     novideovideo            ;  nope.
        call    far ptr endvideo        ; yup, external your-own end routine
        mov     videoflag,0             ; set flag: no your-own-video
        jmp     short notarga
novideovideo:
        cmp     tgaflag,1               ; TARGA MODIFIED 2 June 89 j mclain
        jne     notarga
        call    far ptr EndTGA          ; endTGA( void )
        mov     tgaflag,0               ; set flag: targa cleaned up
notarga:

        cmp     xga_isinmode,0          ; XGA in graphics mode?
        je      noxga                   ; nope
        mov     ax,0                    ; pull it out of graphics mode
        push    ax
        mov     xga_clearvideo,al
        call    far ptr xga_mode
        pop     ax
noxga:

        cmp     f85flag, 1              ; was the last video 8514?
        jne     no8514                  ; nope.
        cmp     ai_8514, 0              ;check afi flag, JCO 4/11/92
        jne     f85endafi
        call    far ptr close8514hw     ;use registers, JCO 4/11/92
        jmp     f85enddone
f85endafi:
        call    far ptr close8514       ;use afi, JCO 4/11/92
;       call    f85end          ;use afi
f85enddone:
        mov     f85flag, 0
no8514:
        cmp     HGCflag, 1              ; was last video Hercules
        jne     noHGC                   ; nope
        call    hgcend
        mov     HGCflag, 0
noHGC:
        mov     oktoprint,1             ; say it's OK to use printf()
        mov     goodmode,1              ; assume a good video mode
        mov     xga_loaddac,1           ; tell the XGA to fake a 'loaddac'
        mov     ax,video_bankadr        ; restore the results of 'whichvga()'
        mov     [bankadr],ax            ;  ...
        mov     ax,video_bankseg        ;  ...
        mov     [bankseg],ax            ;  ...

        mov     ax,argax                ; load up for the interrupt call
        mov     bx,argbx                ;  ...
        mov     cx,argcx                ;  ...
        mov     dx,argdx                ;  ...

        mov     videoax,ax              ; save the values for future use
        mov     videobx,bx              ;  ...
        mov     videocx,cx              ;  ...
        mov     videodx,dx              ;  ...

        call    setvideo                ; call the internal routine first

        cmp     goodmode,0              ; is it still a good video mode?
        jne     videomodeisgood         ; yup.
        mov     ax,offset nullwrite     ; set up null write-a-dot routine
        mov     bx,offset mcgaread      ; set up null read-a-dot  routine
        mov     cx,offset normaline     ; set up normal linewrite routine
        mov     dx,offset mcgareadline  ; set up normal linewrite routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        jmp     videomode               ; return to common code

videomodeisgood:
        mov     bx,dotmode              ; set up for a video table jump
        cmp     bx,30                   ; are we within the range of dotmodes?
        jbe     videomodesetup          ; yup.  all is OK
        mov     bx,0                    ; nope.  use dullnormalmode
videomodesetup:
        shl     bx,1                    ; switch to a word offset
        mov     bx,cs:videomodetable[bx]        ; get the next step
        jmp     bx                      ; and go there
videomodetable  dw      offset dullnormalmode   ; mode 0
        dw      offset dullnormalmode   ; mode 1
        dw      offset vgamode          ; mode 2
        dw      offset mcgamode         ; mode 3
        dw      offset tseng256mode     ; mode 4
        dw      offset paradise256mode  ; mode 5
        dw      offset video7256mode    ; mode 6
        dw      offset tweak256mode     ; mode 7
        dw      offset everex16mode     ; mode 8
        dw      offset targaMode        ; mode 9
        dw      offset hgcmode          ; mode 10
        dw      offset diskmode         ; mode 11
        dw      offset f8514mode        ; mode 12
        dw      offset cgamode          ; mode 13
        dw      offset tandymode        ; mode 14
        dw      offset trident256mode   ; mode 15
        dw      offset chipstech256mode ; mode 16
        dw      offset ati256mode       ; mode 17
        dw      offset everex256mode    ; mode 18
        dw      offset yourownmode      ; mode 19
        dw      offset ati1024mode      ; mode 20
        dw      offset tseng16mode      ; mode 21
        dw      offset trident16mode    ; mode 22
        dw      offset video716mode     ; mode 23
        dw      offset paradise16mode   ; mode 24
        dw      offset chipstech16mode  ; mode 25
        dw      offset everex16mode     ; mode 26
        dw      offset VGAautomode      ; mode 27
        dw      offset VESAmode         ; mode 28
        dw      offset TrueColorAuto    ; mode 29
        dw      offset dullnormalmode   ; mode 30
        dw      offset dullnormalmode   ; mode 31

tandymode:      ; from Joseph Albrecht
        mov     tandyseg,0b800h         ; set video segment address
        mov     tandyofs,0              ; set video offset address
        mov     ax,offset plottandy16   ; set up write-a-dot
        mov     bx,offset gettandy16    ; set up read-a-dot
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        cmp     videoax,8               ; check for 160x200x16 color mode
        je      tandy16low              ; ..
        cmp     videoax,9               ; check for 320x200x16 color mode
        je      tandy16med              ; ..
        cmp     videoax,0ah             ; check for 640x200x4 color mode
        je      tandy4high              ; ..
        cmp     videoax,0bh             ; check for 640x200x16 color mode
        je      tandy16high             ; ..
tandy16low:
        mov     tandyscan,offset scan16k; set scan line address table
        jmp     videomode               ; return to common code
tandy16med:
        mov     tandyscan,offset scan32k; set scan line address table
        jmp     videomode               ; return to common code
tandy4high:
        mov     ax,offset plottandy4    ; set up write-a-dot
        mov     bx,offset gettandy4     ; set up read-a-dot
        jmp     videomode               ; return to common code
tandy16high:
        mov     tandyseg,0a000h         ; set video segment address
        mov     tandyofs,8000h          ; set video offset address
        mov     tandyscan,offset scan64k; set scan line address table
        jmp     videomode               ; return to common code
dullnormalmode:
        mov     ax,offset normalwrite   ; set up the BIOS write-a-dot routine
        mov     bx,offset normalread    ; set up the BIOS read-a-dot  routine
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        jmp     videomode               ; return to common code
mcgamode:
        mov     ax,offset mcgawrite     ; set up MCGA write-a-dot routine
        mov     bx,offset mcgaread      ; set up MCGA read-a-dot  routine
        mov     cx,offset mcgaline      ; set up the MCGA linewrite routine
        mov     dx,offset mcgareadline  ; set up the MCGA lineread  routine
        mov     si,offset swap256       ; set up the MCGA swap routine
        jmp     videomode               ; return to common code
tseng16mode:
        mov     tseng,1                 ; set chipset flag
        mov     [bankadr],offset $tseng
        mov     [bankseg],seg $tseng
        jmp     vgamode         ; set ega/vga functions
trident16mode:
        mov     trident,1               ; set chipset flag
        mov     [bankadr],offset $trident
        mov     [bankseg],seg $trident
        jmp     vgamode
video716mode:
        mov     video7,1                ; set chipset flag
        mov     [bankadr],offset $video7
        mov     [bankseg],seg $video7
        jmp     vgamode
paradise16mode:
        mov     paradise,1              ; set chipset flag
        mov     [bankadr],offset $paradise
        mov     [bankseg],seg $paradise
        jmp     vgamode
chipstech16mode:
        mov     chipstech,1             ; set chipset flag
        mov     [bankadr],offset $chipstech
        mov     [bankseg],seg $chipstech
        jmp     vgamode
everex16mode:
        mov     everex,1                ; set chipset flag
        mov     [bankadr],offset $everex
        mov     [bankseg],seg $everex
        jmp     vgamode
VESAmode:                               ; set VESA 16-color mode
        mov     ax,word ptr vesa_mapper
        mov     [bankadr],ax
        mov     ax,word ptr vesa_mapper+2
        mov     [bankseg],ax
        cmp     vesa_bitsppixel,15
        jae     VESAtruecolormode
VGAautomode:                            ; set VGA auto-detect mode
        cmp     colors,256              ; 256 colors?
        je      VGAauto256mode          ; just like SuperVGA
        cmp     xga_isinmode,0          ; in an XGA mode?
        jne     xgamode
        cmp     colors,16               ; 16 colors?
        je      vgamode                 ; just like a VGA
	jmp     dullnormalmode          ; otherwise, use the BIOS

VESAtruecolormode:
        mov     istruecolor,1
	mov	ax, offset VESAtruewrite   ; set up VESA true-color write-a-dot routine	
	mov	bx, offset VESAtrueread    ; set up VESA true-color read-a-dot routine	
	mov	cx, offset normaline       ; set up dullnormal linewrite routine	
	mov	dx, offset normalineread   ; set up dullnormal lineread routine	
        mov     si,offset swap256          ; set up the swap routine
        jmp     videomode                  ; return to common code

xgamode:
        mov     ax,offset xga_16write      ; set up XGA write-a-dot routine
        mov     bx,offset xga_16read       ; set up XGA read-a-dot  routine
        mov     cx,offset xga_16linewrite  ; set up the XGA linewrite routine
        mov     dx,offset normalineread    ; set up the XGA lineread  routine
        mov     si,offset swap256          ; set up the swap routine
        jmp     videomode                  ; return to common code
	
VGAauto256mode:
        jmp     super256mode            ; just like a SuperVGA
egamode:
vgamode:
;;;     shr     vxdots,1                ; scan line increment is in bytes...
;;;     shr     vxdots,1
;;;     shr     vxdots,1
        mov     ax,offset vgawrite      ; set up EGA/VGA write-a-dot routine.
        mov     bx,offset vgaread       ; set up EGA/VGA read-a-dot  routine
        mov     cx,offset vgaline       ; set up the EGA/VGA linewrite routine
        mov     dx,offset vgareadline   ; set up the EGA/VGA lineread  routine
        mov     si,offset swapvga       ; set up the EGA/VGA swap routine
        jmp     videomode               ; return to common code
tseng256mode:
        mov     tseng,1                 ; set chipset flag
        mov     [bankadr],offset $tseng
        mov     [bankseg],seg $tseng
        jmp     super256mode            ; set super VGA linear memory functions
paradise256mode:
        mov     paradise,1              ; set chipset flag
        mov     [bankadr],offset $paradise
        mov     [bankseg],seg $paradise
        jmp     super256mode            ; set super VGA linear memory functions
video7256mode:
        mov     video7, 1               ; set chipset flag
        mov     [bankadr],offset $video7
        mov     [bankseg],seg $video7
        jmp     super256mode            ; set super VGA linear memory functions
trident256mode:
        mov     trident,1               ; set chipset flag
        mov     [bankadr],offset $trident
        mov     [bankseg],seg $trident
        jmp     super256mode            ; set super VGA linear memory functions
chipstech256mode:
        mov     chipstech,1             ; set chipset flag
        mov     [bankadr],offset $chipstech
        mov     [bankseg],seg $chipstech
        jmp     super256mode            ; set super VGA linear memory functions
ati256mode:
        mov     ativga,1                ; set chipset flag
        mov     [bankadr],offset $ativga
        mov     [bankseg],seg $ativga
        jmp     super256mode            ; set super VGA linear memory functions
everex256mode:
        mov     everex,1                ; set chipset flag
        mov     [bankadr],offset $everex
        mov     [bankseg],seg $everex
        jmp     super256mode            ; set super VGA linear memory functions
VGA256automode:                         ; Auto-detect SuperVGA
        jmp     super256mode            ; set super VGA linear memory functions
VESA256mode:                            ; set VESA 256-color mode
        mov     ax,word ptr vesa_mapper
        mov     [bankadr],ax
        mov     ax,word ptr vesa_mapper+2
        mov     [bankseg],ax
        jmp     super256mode            ; set super VGA linear memory functions
super256mode:
        mov     ax,offset super256write ; set up superVGA write-a-dot routine
        mov     bx,offset super256read  ; set up superVGA read-a-dot  routine
        mov     cx,offset super256line  ; set up the  linewrite routine
        mov     dx,offset super256readline ; set up the normal lineread  routine
        mov     si,offset swap256       ; set up the swap routine
        jmp     videomode               ; return to common code
tweak256mode:
        shr     vxdots,1                ; scan line increment is in bytes...
        shr     vxdots,1
        mov     oktoprint,0             ; NOT OK to printf() in this mode
        mov     ax,offset tweak256write ; set up tweaked-256 write-a-dot
        mov     bx,offset tweak256read  ; set up tweaked-256 read-a-dot
        mov     cx,offset tweak256line  ; set up tweaked-256 read-a-line
        mov     dx,offset tweak256readline ; set up the normal lineread  routine
        mov     si,offset swapvga       ; set up the swap routine
        jmp     videomode               ; return to common code
cgamode:
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        cmp     videoax,4               ; check for 320x200x4 color mode
        je      cga4med                 ; ..
        cmp     videoax,5               ; ..
        je      cga4med                 ; ..
        cmp     videoax,6               ; check for 640x200x2 color mode
        je      cga2high                ; ..
cga4med:
        mov     ax,offset plotcga4      ; set up CGA write-a-dot
        mov     bx,offset getcga4       ; set up CGA read-a-dot
        jmp     videomode               ; return to common code
cga2high:
        mov     ax,offset plotcga2      ; set up CGA write-a-dot
        mov     bx,offset getcga2       ; set up CGA read-a-dot
        jmp     videomode               ; return to common code
ati1024mode:
        mov     ativga,1                ; set ATI flag.
        mov     ax,offset ati1024write  ; set up ATI1024 write-a-dot
        mov     bx,offset ati1024read   ; set up ATI1024 read-a-dot
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swap256       ; set up the swap routine
        jmp     videomode               ; return to common code
diskmode:
        call    far ptr startdisk       ; external disk-video start routine
        mov     ax,offset diskwrite     ; set up disk-vid write-a-dot routine
        mov     bx,offset diskread      ; set up disk-vid read-a-dot routine
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        jmp     videomode               ; return to common code
yourownmode:
        call    far ptr startvideo      ; external your-own start routine
        mov     ax,offset videowrite    ; set up ur-own-vid write-a-dot routine
        mov     bx,offset videoread     ; set up ur-own-vid read-a-dot routine
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        mov     videoflag,1             ; flag "your-own-end" needed.
        jmp     videomode               ; return to common code
targaMode:                              ; TARGA MODIFIED 2 June 89 - j mclain
        call    far ptr StartTGA
        mov     ax,offset tgawrite      ;
        mov     bx,offset tgaread       ;
        mov     cx,offset normaline     ; set up the normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        mov     tgaflag,1               ;
        jmp     videomode               ; return to common code
f8514mode:                              ; 8514 modes
        cmp     ai_8514, 0              ; check if afi flag is set, JCO 4/11/92
        jne     f85afi          ; yes, try afi
        call    far ptr open8514hw      ; start the 8514a, try registers first JCO
        jnc     f85ok
        mov     ai_8514, 1              ; set afi flag
f85afi:
        call    far ptr open8514        ; start the 8514a, try afi
        jnc     f85ok
        mov     ai_8514, 0              ; clear afi flag, JCO 4/11/92
        mov     goodmode,0              ; oops - problems.
        mov     dotmode, 0              ; if problem starting use normal mode
        jmp     dullnormalmode
hgcmode:
        mov     oktoprint,0             ; NOT OK to printf() in this mode
        call    hgcstart                ; Initialize the HGC card
        mov     ax,offset hgcwrite      ; set up HGC write-a-dot routine
        mov     bx,offset hgcread       ; set up HGC read-a-dot  routine
        mov     cx,offset normaline     ; set up normal linewrite routine
        mov     dx,offset normalineread ; set up the normal lineread  routine
        mov     si,offset swapnormread  ; set up the normal swap routine
        mov     HGCflag,1               ; flag "HGC-end" needed.
        jmp     videomode               ; return to common code
f85ok:
        cmp     ai_8514, 0
        jne     f85okafi                        ; afi flag is set JCO 4/11/92
        mov     ax,offset f85hwwrite    ;use register routines
        mov     bx,offset f85hwread    ;changed to near calls
        mov     cx,offset f85hwline    ;
        mov     dx,offset f85hwreadline    ;
        mov     si,offset swapnormread  ; set up the normal swap routine
        mov     f85flag,1               ;
        mov     oktoprint,0             ; NOT OK to printf() in this mode
        jmp     videomode               ; return to common code
f85okafi:
        mov     ax,offset f85write      ;use afi routines, JCO 4/11/92
        mov     bx,offset f85read      ;changed to near calls
        mov     cx,offset f85line      ;
        mov     dx,offset f85readline      ;
        mov     si,offset swapnormread  ; set up the normal swap routine
        mov     f85flag,1               ;
        mov     oktoprint,0             ; NOT OK to printf() in this mode
        jmp     videomode               ; return to common code
TrueColorAuto:
        cmp     TPlusInstalled, 1
        jne     NoTPlus
        push    NonInterlaced
        push    PixelZoom
        push    MaxColorRes
        push    ydots
        push    xdots
        call    far ptr MatchTPlusMode
        add     sp, 10
        or      ax, ax
        jz      NoTrueColorCard

        cmp     ax, 1                     ; Are we limited to 256 colors or less?
        jne     SetTPlusRoutines          ; All right! True color mode!

        mov     cx, MaxColorRes           ; Aw well, give'm what they want.
        shl     ax, cl
        mov     colors, ax

SetTPlusRoutines:
        mov     goodmode, 1
        mov     oktoprint, 1
        mov     ax, offset TPlusWrite
        mov     bx, offset TPlusRead
        mov     cx, offset normaline
        mov     dx, offset normalineread
        mov     si, offset swapnormread
        jmp     videomode

NoTPlus:
NoTrueColorCard:
        mov     goodmode, 0
        jmp     videomode

videomode:
        mov     dotwrite,ax             ; save the results
        mov     dotread,bx              ;  ...
        mov     linewrite,cx            ;  ...
        mov     lineread,dx             ;  ...
        mov     word ptr swapsetup,si   ;  ...
        mov     ax,cs                   ;  ...
        mov     word ptr swapsetup+2,ax ;  ...

        mov     ax,colors               ; calculate the "and" value
        dec     ax                      ; to use for eventual color
        mov     andcolor,ax             ; selection

        mov     boxcount,0              ; clear the zoom-box counter

        mov     daclearn,0              ; set the DAC rotates to learn mode
        mov     daccount,6              ; initialize the DAC counter
        cmp     cpu,88                  ; say, are we on a 186/286/386?
        jbe     setvideoslow            ;  boo!  hiss!
        mov     daclearn,1              ; yup.  bypass learn mode
        mov     ax,cyclelimit           ;  and go as fast as he wants
        mov     daccount,ax             ;  ...
setvideoslow:
        call    far ptr loaddac         ; load the video dac, if we can
        ret
setvideomode    endp

set_vesa_mapping_func proc near
        mov     cx, word ptr vesa_winaattrib  ; puts vesa_winbattrib in ch
        and     cx,0707h                      ; so we can check them together
        cmp     cx,0305h
        je      use_vesa2
        cmp     cx,0503h
        je      use_vesa2
        test    ch,01h
        jz      use_vesa1
        cmp     vesa_winsize,32    ; None of the above -- 2 32K R/W?
        jne     use_vesa1               ;    if not, use original 1 64K R/W!
        mov     word ptr vesa_mapper,offset $vesa3
        mov     ax,32
        div     vesa_wingran       ; Get number of pages in 32K
        mov     vesa_gran_offset,ax     ; Save it for mapping function
        xor     dx,dx
        mov     ax,1
        test    vesa_winaseg, 00800h
        jz      low_high_seq
        xchg    ax,dx
low_high_seq:
        mov     vesa_low_window,dx      ; Window number at A000-A7FF
        mov     vesa_high_window,ax     ; Window number at A800-AFFF
        jmp     short vesamapselected
use_vesa2:
        mov     word ptr vesa_mapper,offset $vesa2
        jmp     short vesamapselected
use_vesa1:
        mov     word ptr vesa_mapper,offset $vesa1
vesamapselected:
        mov     word ptr vesa_mapper+2,seg $vesa1
        ret
set_vesa_mapping_func endp

setnullvideo proc
        mov     ax,offset nullwrite     ; set up null write-a-dot routine
        mov     dotwrite,ax             ;  ...
        mov     ax,offset nullread      ; set up null read-a-dot routine
        mov     dotread,ax              ;  ...
        ret
setnullvideo endp

setvideo        proc    near            ; local set-video more

        cmp     xga_isinmode,0          ; XGA in graphics mode?
        je      noxga                   ; nope
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ax,0                    ; pull it out of graphics mode
        push    ax
        mov     xga_clearvideo,al
        call    far ptr xga_mode
        pop     ax
        pop     dx
        pop     cx
        pop     bx
        pop     ax
noxga:

        push    bp                      ; save it around all the int 10s
        mov     text_type,2             ; set to this for most exit paths
        mov     si,offset $vesa_nullbank ; set to do nothing if mode not vesa
        mov     word ptr vesa_bankswitch,si
        mov     si,seg $vesa_nullbank
        mov     word ptr vesa_bankswitch+2,si
        mov     word ptr vesa_mapper,offset $nobank
        mov     word ptr vesa_mapper+2,seg $nobank
	mov	vesa_bitsppixel,8
	mov	istruecolor,0
;	mov	vesabyteoffset,0
        mov     tweakflag,0

        cmp     ax,0                    ; TWEAK?:  look for AX==BX==CX==0
        jne     short setvideobios      ;  ...
        cmp     bx,0                    ;  ...
        jne     short setvideobios      ;  ...
        cmp     cx,0                    ;  ...
        jne     short setvideobios      ;  ...

        cmp     dotmode, 27             ; check for auto-detect modes
        je      setvideoauto1
        cmp     dotmode, 20             ; check for auto-detect modes
        je      setvideoauto1
        cmp     dotmode, 4              ; check for auto-detect modes
        je      setvideoauto1
        cmp     dotmode, 28             ; check for auto-detect modes
        je      setvideoauto1

        jmp     setvideoregs            ; anything else - assume register tweak

setvideoauto1:
        jmp     setvideoauto            ; stupid short 'je' instruction!!

setvideobios:
        mov     text_type,0             ; if next branch taken this is true
        cmp     ax,3                    ; text mode?
        jne     setvideobios2           ;  nope
        mov     textaddr,0b800h
        cmp     mode7text,0             ; egamono/hgc?
        je      setvideobios_doit       ;  nope.  Just do it.
        mov     textaddr,0b000h
        mov     ax,7                    ; use mode 7
        call    maybeor                 ; maybe or AL or (for Video-7s) BL
        push    bp                      ; weird but necessary, set mode twice
        int     10h                     ;  get colors right on vga systems
        pop     bp                      ;  ..
        mov     ax,7                    ; for the 2nd hit
        jmp     short setvideobios_doit
setvideobios2:
        mov     text_type,1             ; if next branch taken this is true
        cmp     ax,6                    ; 640x200x2 mode?
        je      setvideobios_doit       ;  yup.  Just do it.
        mov     text_type,2             ; not mode 3 nor 6, so this is true
        mov     si,dotmode              ; compare the dotmode against
        mov     di,video_type           ; the video type
        add     si,si                   ; (convert to a word pointer)
        cmp     cs:video_requirements[si],di
        jbe     setvideobios_doit       ; ok
        jmp     setvideoerror           ;  Error.
setvideobios_doit:
        cmp     dotmode,14              ; check for Tandy 1000 mode
        jne     setvideobios_doit2      ; ..
        cmp     ax,0ah                  ; check for Tandy 640x200x4 color mode
        jne     setvideobios_doit1      ; ..
        push    bp                      ; setup Tandy 640x200x4 color mode
        int     10h                     ; ..
        pop     bp                      ; ..
        mov     di,16                   ;port offset for palette registers
        mov     bx,0b01h                ; remap colors for better display on
        call    settandypal             ; .. Tandy 640x200x4 color mode
        mov     di,16                   ;port offset for palette registers
        mov     bx,0d02h                ; ..
        call    settandypal             ; ..
        mov     di,16                   ;port offset for palette registers
        mov     bx,0f03h                ; ..
        call    settandypal             ; ..
        jmp     setvideobios_worked
setvideobios_doit1:
        cmp     ax,0bh                  ; check for Tandy 640x200x16 color mode
        jne     setvideobios_doit2      ; ..
        call    tandysetup              ; setup Tandy 640x200x16 color mode
        jmp     setvideobios_worked
setvideobios_doit2:
        call    maybeor                 ; maybe or AL or (for Video-7s) BL
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it via the BIOS.
        pop     bp                      ; restore the saved register
        cmp     dotmode,28              ; VESA mode?
;        jne     setvideobios_worked     ;  Nope. Return.
        je      over_setvideobios
        jmp     setvideobios_worked     ;  Nope. Return.
over_setvideobios:
        cmp     ah,0                    ; did it work?
;        jne     setvideoerror           ;  Nope. Failed.
        je      over_setvideoerror
        jmp     setvideoerror           ;  Nope. Failed.
over_setvideoerror:
        mov     vesa_granularity,1      ; say use 64K granules
        push    es                      ; set ES == DS
        mov     ax,ds                   ;  ...
        mov     es,ax                   ;  ...
        mov     ax,4f01h                ; ask about this video mode
        mov     cx,bx                   ;  this mode
        and     cx,07fffh               ; (oops - correct for the high-bit)
        mov     di, offset suffix       ; (a safe spot for 256 bytes)
        int     10h                     ; do it
        cmp     ax,004fh                ; did the call work?
        je      okaysofar
        jmp     nogoodvesamode          ;  nope
okaysofar:
        mov     si, offset suffix  ; save the first 40 bytes of the mode info
        mov     di, offset vesa_mode_info ; (the truecolor routines need it)
        mov     cx,40                   ; but since we have them, we'll use
        rep     movsb                   ; the variables in the first 40 bytes
        mov     cx, vesa_mode_info      ; get the attributes
        test    cx,1                    ; available video mode?
        jnz     overnogood
        jmp     nogoodvesamode          ; nope.  skip some code
overnogood:
;;;    A sanity check, which ATI cards don't pass.  JCO  12 MAY 2001
;;;     test    cx,60h                  ; not vga compatible or not windowed
;;;     jnz     nogoodvesamode          ; yup.  skip some code
        call    set_vesa_mapping_func
        mov     cx, word ptr vesa_funcptr  ; get the Bank-switching routine
        cmp     cx,0    ; could make this use the bios, bailout for now JCO
        jz      nogoodvesamode          ; nope.  skip some code
        mov     word ptr vesa_bankswitch, cx  ;  ...
        mov     cx, word ptr vesa_funcptr+2      ;   ...
        mov     word ptr vesa_bankswitch+2, cx  ;  ...
        mov     cx, vesa_bytespscan     ; get the bytes / scan line
        cmp     cx,0                    ; is this entry filled in?
        je      skipvesafix             ;  nope
;;;     cmp     colors,256              ; 256-color mode?
;;;     jne     skipvesafix             ;  nope
        cmp     vesa_numplanes,1
        jbe     store_vesa_bytes
        shl     cx,1                    ; if a planar mode, bits are pixels
        shl     cx,1                    ; so we multiply bytes by 8
        shl     cx,1
store_vesa_bytes:
        mov     vxdots,cx               ; adjust the screen width accordingly XXX
;       cmp     cx,sxdots               ; 8/93 JRS textsafe=save fix
;       je      skipvesafix
;       mov     ax,offset swapnormread  ; use the slow swap routine
;       mov     word ptr swapsetup,ax   ;  ...
skipvesafix:
        cmp     virtual,0               ; virtual modes turned off
        jne     check_next
        cmp     video_scroll,0          ; not turned on
        je      dont_go_there           ;  don't need next
        mov     ax,vesa_xres            ; get the physical resolution
        mov     bx,vesa_yres
        mov     sxdots,ax
        mov     sydots,bx
check_next:
        cmp     setting_text,1          ; don't set virtual stuff
        je      dont_go_there
        mov     video_scroll,0          ; reset scrolling flag
        cmp     dotmode,28
        jne     dont_go_there
        call    VESAvirtscan            ; virtual scanline setup
dont_go_there:
        mov     cx, vesa_wingran        ; get the granularity
        cmp     cl,1                    ; ensure the divide won't blow out
        jb      nogoodvesamode          ;  granularity == 0???
        mov     ax,64                   ;  ...
        div     cl                      ; divide 64K by granularity
        mov     vesa_granularity,al     ; multiply the bank number by this
        cmp     vesa_bitsppixel,15      ; true-color mode?
        jb      nogoodvesamode
        mov     istruecolor,1           ;  yup
nogoodvesamode:
        pop     es                      ; restore ES
        mov     ax,4f02h                ; restore the original call
setvideobios_worked:
        jmp     setvideoreturn          ;  Return.

setvideoerror:                          ; oops.  No match found.
        mov     goodmode,0              ; note that the video mode is bad
        mov     ax,3                    ; switch to text mode
        jmp     setvideobios_doit

setvideoauto:
        mov     si, video_entries       ; look for the correct resolution
        sub     si,8                    ; get a running start
setvideoloop:
        add     si,10                   ; get next entry
        mov     ax,cs:0[si]             ; check X-res
        cmp     ax,0                    ; anything there?
        je      setvideoerror           ; nope.  No match
        cmp     ax,sxdots
        jne     setvideoloop
        mov     ax,cs:2[si]             ; check Y-res
        cmp     ax,sydots
        jne     setvideoloop
        mov     ax,cs:4[si]             ; check Colors
        cmp     ax,colors
        jne     setvideoloop
        mov     ax,cs:6[si]             ; got one!  Load AX
        mov     bx,cs:8[si]             ;           Load BX

        cmp     ax,0ffffh               ; XGA special?
        jne     notxgamode
        mov     al,orvideo
        mov     xga_clearvideo,al
        cmp     al,0                    ; clearing the video?
        jne     xgask1                  ;  yup
        mov     ax,03h                  ; switch to text mode (briefly)
        int     10h
xgask1:
        push    bx
        call    far ptr xga_mode
        pop     bx
        cmp     ax,0
        je      setvideoloop
        jmp     setvideoreturn
notxgamode:

        cmp     bx,0ffh                 ; ATI 1024x768x16 special?
        jne     notatimode
        mov     dotmode,20              ; Convert to ATI specs
        mov     al,65h
        mov     bx,0
        jmp     setvideobios
notatimode:
        cmp     bx,0feh                 ; Tseng 640x400x256 special?
        jne     nottsengmode
        mov     ax,0                    ; convert to Tseng specs
        mov     bx,0
        mov     cx,0
        mov     dx,10
        mov     dotmode,4
        jmp     setvideoregs
nottsengmode:
        cmp     bx,0fdh                 ; Compaq 640x480x256 special?
        jne     notcompaqmode
        mov     vxdots,1024             ; (compaq uses 1024-byte scanlines)
        mov     bx,offset swapnormread  ; use the slow swap routine
        mov     word ptr swapsetup,bx   ;  ...
        mov     bx,0
        jmp     setvideobios
notcompaqmode:
        cmp     ax,4f02h                ; VESA mode?
        jne     notvesamode
        mov     dotmode,28              ; convert to VESA specs
notvesamode:
        jmp     setvideobios

setvideoregs:                           ; assume genuine VGA and program regs
        mov     si, dotmode             ; compare the dotmode against
        mov     di,video_type           ; the video type
        add     si,si                   ; (convert to a word pointer)
        cmp     cs:video_requirements[si],di
        jbe     setvideoregs_doit       ;  good value.  Do it.
        jmp     setvideoerror           ;  bad value.  Error.
setvideoregs_doit:
        mov     si,dx                   ; get the video table offset
        shl     si,1                    ;  ...
        mov     si,word ptr tweaks[si]  ;  ...

        mov     tweaktype, dx           ; save tweaktype
        cmp     dx,8                    ; 360x480 tweak256mode?
        je      isatweaktype            ; yup
        cmp     dx,9                    ; 320x400 tweak256mode?
        je      isatweaktype            ; yup
        cmp     dx,18                   ; 320x480 tweak256mode?
        je      isatweaktype            ; yup
        cmp     dx,19                   ; 320x240 tweak256mode?
        je      isatweaktype            ; yup
        cmp     dx,10                   ; Tseng tweak?
        je      tsengtweak              ; yup
;Patch - Michael D. Burkey (5/22/90)
        cmp     dx,14                   ; ATI Mode Support
        je      ATItweak
        cmp     dx,15
        je      ATItweak
        cmp     dx,16
        je      ATItweak
        cmp     dx,17
        je      ATItweak2               ; ATI 832x616 mode
        cmp     dx,11                   ; tweak256mode? (11 & up)
        jae     isatweaktype            ; yup
;End Patch
        jmp     not256                  ; nope none of the above
tsengtweak:
        mov     ax,46                   ; start with S-VGA mode 2eh
        call    maybeor                 ; maybe don't clear the video memory
        int     10h                     ; let the bios clear the video memory
        mov     dx,3c2h                 ; misc output
        mov     al,063h                 ; dot clock
        out     dx,al                   ; select it
        mov     dx,3c4h                 ; sequencer again
        mov     ax,0300h                ; restart sequencer
        out     dx,ax                   ; running again
        jmp     is256;
ATItweak:
        mov     ax,62h
;; pb, why no maybeor call here?
        int     10h
        mov     dx,3c2h
        mov     al,0e3h
        out     dx,al
        mov     dx,3c4h
        mov     ax,0300h
        out     dx,ax
        jmp     is256

ATItweak2:
        mov     ax,63h
;; pb, why no maybeor call here?
        int     10h
        mov     dx,3c4h
        mov     ax,0300h
        out     dx,ax
        jmp     is256

isatweaktype:
        mov     tweakflag,1
        mov     ax,0013h                ; invoke video mode 13h
        call    maybeor                 ; maybe or AL or (for Video-7s) BL
        int     10h                     ; do it

        mov     dx,3c4h                 ; alter sequencer registers
        mov     ax,0604h                ; disable chain 4
        out     dx,ax

        cmp     orvideo,0               ; are we supposed to clear RAM?
        jne     noclear256              ;  (nope)

        mov     dx,03c4h                ; alter sequencer registers
        mov     ax,0f02h                ; enable writes to all planes
        OUT_WORD

        push    es                      ; save ES for a tad
        mov     ax,VGA_SEGMENT          ; clear out all 256K of
        mov     es,ax                   ;  video memory
        sub     di,di                   ;  (64K at a time, but with
        mov     ax,di                   ;  all planes enabled)
        mov     cx,8000h                ;# of words in 64K
        cld
        rep stosw                       ;clear all of display memory
        pop     es                      ; restore ES

noclear256:
        mov     dx,3c4h                 ; alter sequencer registers
        mov     ax,0604h                ; disable chain 4
        out     dx,ax

        jmp     short is256             ; forget the ROM characters

not256:

        mov     ax,0012h                ; invoke video mode 12h
        call    maybeor                 ; maybe or AL or (for Video-7s) BL
        int     10h                     ; do it.

is256:  push    es                      ; save ES for a tad
        mov     ax,40h                  ; Video BIOS DATA area
        mov     es,ax                   ;  ...

        mov     dx,word ptr es:[63h]    ; say, where's the 6845?
        add     dx,6                    ; locate the status register
vrdly1: in      al,dx                   ; loop until vertical retrace is off
        test    al,8                    ;   ...
        jnz     vrdly1                  ;   ...
vrdly2: in      al,dx                   ; now loop until it's on!
        test    al,8                    ;   ...
        jz      vrdly2                  ;   ...

        cli                             ; turn off all interrupts
        mov     dx,tweaktype
        cmp     dx,9                    ; 320x400 mode?
        je      not256mode              ; yup - skip this stuff
        cmp     dx,10                   ; Tseng tweak mode?
        je      not256mode              ; yup - skip this stuff
;patch #2 (M. Burkey 5/22/90)
        cmp     dx,17                   ; for 832x616 ATI Mode
        je      not256mode
;patch end
        mov     cl,0E7h                 ; value for misc output reg
        cmp     dx,18                   ; 320x480 mode?
        je      setmisc320              ;  nope, use above value
        cmp     dx,19                   ; 320x240 mode?
        jne     setmiscoreg             ;  nope, use above value
setmisc320:
        mov     cl,0E3h                 ; value for misc output reg
setmiscoreg:
        mov     dx,03c4h                ; Sequencer Synchronous reset
        mov     ax,0100h                ; set sequencer reset
        out     dx,ax
        mov     dx,03c2h                ; Update Misc Output Reg
        mov     al,cl
        out     dx,al
        mov     dx,03c4h                ; Sequencer Synchronous reset
        mov     ax,0300h                ; clear sequencer reset
        out     dx,ax
not256mode:
        mov     dx,word ptr es:[63h]    ; say, where's the 6845?
        add     si,2                    ; point SI to the CRTC registers table
        mov     al,11h                  ; deprotect registers 0-7
        mov     ah,byte ptr [si+11h]
        and     ah,7fh
        out     dx,ax

        mov     cx,18h                  ; update this many registers
        mov     bx,00                   ; starting with this one.
crtcloop:
        mov     al,bl                   ; update this register
        mov     ah,byte ptr [bx+si]     ; to this
        out     dx,ax
        inc     bx                      ; ready for the next register
        loop    crtcloop                ; (if there is a next register)
        sti                             ; restore interrupts

        pop     es                      ; restore ES

setvideoreturn:
        mov     curbk,0ffffh            ; stuff impossible value into cur-bank
        mov     orvideo,0               ; reset the video to clobber memory
        pop     bp
        ret
setvideo        endp

maybeor proc    near                    ; or AL or BL for mon-destr switch
        cmp     ah,4fh                  ; VESA special mode?
        je      maybeor2                ;  yup.  Do this one different
        cmp     ah,6fh                  ; video-7 special mode?
        je      maybeor1                ;  yup.  do this one different
        or      al,orvideo              ; normal non-destructive switch
        jmp     short maybeor99         ; we done.
maybeor1:
        or      bl,orvideo              ; video-7 switch
        jmp     short maybeor99
maybeor2:
        or      bh,orvideo              ; VESA switch
maybeor99:
        ret                             ; we done.
maybeor endp


; ************* function scroll_center(tocol, torow) *************************

; scroll_center --------------------------------------------------------------
; * this is meant to be an universal scrolling redirection routine
;   (if scrolling will be coded for the other modes too, the VESAscroll
;   call should be replaced by a preset variable (like in proc newbank))
; * arguments passed are the coords of the screen center because
;   there is no universal way to determine physical screen resolution
; ------------------------------------------------------------12-08-2002-ChCh-

scroll_center   proc    tocol: word, torow: word
        cmp     video_scroll,0        ; is the scrolling on?
        jne     okletsmove              ;  ok, lets move
        jmp     staystill               ;  no, stay still
okletsmove:
        mov     cx,tocol                ; send center-x to the routine
        mov     dx,torow                ; send center-y to the routine
        call    VESAscroll              ; replace this later with a variable
staystill:
        ret
scroll_center   endp

; ************* function scroll_relative(bycol, byrow) ***********************

; scroll_relative ------------------------------------------------------------
; * relative screen center scrolling, arguments passed are signed deltas
; ------------------------------------------------------------16-08-2002-ChCh-

scroll_relative proc    bycol: word, byrow: word
        cmp     video_scroll,0        ; is the scrolling on?
        jne     okletsmove              ;  ok, lets move
        jmp     staystill               ;  no, stay still
okletsmove:
        mov     cx,video_startx         ; where we already are..
        mov     dx,video_starty
        add     cx,video_cofs_x         ; find the screen center
        add     dx,video_cofs_y
        add     cx,bycol                ; add the relative shift
        add     dx,byrow
        call    VESAscroll              ; replace this later with a variable
staystill:
        ret
scroll_relative endp

; ************* function scroll_state(what) **********************************

; scroll_state ---------------------------------------------------------------
; * what == 0: saves the position of the scrolled screen center
; * what != 0: restores saved position and scrolls the screen
; ------------------------------------------------------------16-08-2002-ChCh-

scroll_state    proc    what: word
        cmp     what,0                  ; save or restore?
        jne     restore
        mov     cx,video_startx         ; where we already are..
        mov     dx,video_starty
        add     cx,video_cofs_x         ; find the screen center
        add     dx,video_cofs_y
        mov     scroll_savex,cx         ; save it for restoring
        mov     scroll_savey,dx
        jmp     wedone                  ; return
restore:
        mov     cx,scroll_savex         ; get the saved coords
        mov     dx,scroll_savey
        call    VESAscroll              ; scroll there
wedone:
        ret
scroll_state    endp

; VESAscroll ---------------------------------------------------------------
; * this does the scrolling of the screen center then (cx=to_col, dx=to_row)
;   (first, it has to figure out the top-left corner coords)
; ------------------------------------------------------------12-08-2002-ChCh-

VESAscroll      proc      near
        sub     cx,video_cofs_x
        js      colbad                  ; to_col too small
        cmp     cx,video_slim_x
        jna     colok
        mov     cx,video_slim_x         ; to_col too big
        jmp     short colok
colbad:
        xor     cx,cx
colok:
        sub     dx,video_cofs_y
        js      rowbad                  ; to_row too small
        cmp     dx,video_slim_y
        jna     rowok
        mov     dx,video_slim_y         ; to_row too big
        jmp     short rowok
rowbad:
        xor     dx,dx
rowok:
        cmp     cx,video_startx         ; moves left/right?
        jne     ok_letsmove
        cmp     dx,video_starty         ; move up/down?
        jne     ok_letsmove
        jmp     short stay_still        ;  otherwise do nothing
ok_letsmove:
        mov     ax,4f07h                ; fn set display start
;        mov     bx,0080h                ; subfn 80 during the vertical retrace
;        xor     bx,bx                   ; subfn 0 instantly (if sf 80 doesn't work)
        mov     bx,wait_retrace         ; wait for retrace?
        int     10h                     ; move there the left-top corner
        mov     video_startx,cx         ; what was really set?
        mov     video_starty,dx         ; (save the result)
stay_still:
        ret
VESAscroll      endp

; VESAvirtscan ---------------------------------------------------------------
; * tests whether the virtual scanline is needed and tries to set it up;
;   when nothing fails it sets the video_scroll flag to 1, else to 0
; * if sxdots is too small, vesa_xres is used instead to preserve nice
;   current behavior (small window by small sx/ydots got from cfgfile)
; ------------------------------------------------------------12-08-2002-ChCh-

VESAvirtscan    proc    near
        xor     ax,ax
once_again:
        mov     cx,sxdots               ; this should be passed twice
        mov     dx,sydots
        cmp     cx,vesa_xres            ; asked wider than screen x?
        ja      setvirtscan
        cmp     dx,vesa_yres            ; asked higher than screen y?
        ja      setvirtscan
        jmp     wedone                  ;  otherwise do nothing
setvirtscan:
        cmp     ax,004fh                ; is this second pass?
        je      secondpass
        mov     chkd_vvs,1
        mov     ax,4f06h                ; VESA fn 6 scanline length
        mov     bx,1                    ; subfn 1 get line width
        int     10h
        cmp     ax,004fh                ; did that work?
;;        jne     bad_vesa                ;  bad vesa.. try it anyway
        je      over_bailout
        jmp     wedonebad               ; bailout now
over_bailout:
        mov     ax,4f06h                ; VESA fn 6 scanline length
        mov     bx,3                    ; subfn 3 get max line width
        int     10h
        cmp     ax,004fh                ; did that work?
        jne     bad_vesa                ;  bad vesa.. try it anyway
        mov     ax,4f06h
        mov     cx,bx                   ; get the max width in bytes
        mov     bx,2                    ; subfn 2 set width in bytes
        and     cx,0fff8h               ; 8 bytes width align
        int     10h
        cmp     ax,004fh                ; did that work?
        jne     bad_vesa                ;  bad vesa.. try it anyway
        cmp     sxdots,cx               ; max width in pixels in cx
        jae     alreadyset              ;  can't set more
bad_vesa:
        mov     ax,4f06h
        mov     cx,sxdots
        xor     bx,bx                   ; subfn 0 set width in pixels
        cmp     cx,vesa_xres            ; asked more than screen's width?
        jnb     width_ok
        mov     cx,vesa_xres            ;  can't set less
        int     10h
        cmp     ax,004fh                ; did that work?
        je      nofullscr               ;  don't update sxdots
        jmp     wedonebad               ;  bad luck..
width_ok:
        int     10h                     ; set the virtual scanline
        cmp     ax,004fh                ; did that work?
        je      alreadyset
        jmp     wedonebad               ;  bad luck..
alreadyset:
        mov     sxdots,cx               ; update sxdots to what was set
nofullscr:
        cmp     sydots,dx               ; enough lines available?
        jna     once_more
        mov     sydots,dx               ;  no, save the limit
once_more:
        jmp     once_again              ; go compare the result
secondpass:
        mov     video_scroll,1          ; turn on the scroll flag
        mov     vxdots,bx               ; save the line byte size
        mov     xdots,cx                ;  save it
        mov     ydots,dx                ;  save it
        mov     ax,vesa_xres            ; get the physical resolution
        mov     bx,vesa_yres
        sub     cx,ax                   ; cx=sxdots-vesa_xres
        jns     slimxok
        xor     cx,cx                   ; line too short
slimxok:
        sub     dx,bx                   ; dx=sydots-vesa_yres
        jns     slimyok
        xor     dx,dx                   ; too few lines
slimyok:
        mov     video_slim_x,cx         ; save scrolling limits
        mov     video_slim_y,dx
        shr     ax,1                    ; the middle of the screen
        shr     bx,1
        mov     video_cofs_x,ax         ; save screen center offset
        mov     video_cofs_y,bx
        mov     cx,sxdots               ; get virtual resolution
        mov     dx,sydots
;;        xor     ax,ax
        mov     ax,1                    ; try to move a tad
        shr     cx,1                    ; find the center of it
        shr     dx,1
        dec     cx                      ; zero-based coords
        dec     dx                      ; (no sf, s?dots is always >= 2)
        mov     video_startx,ax         ; video didn't scroll yet
        mov     video_starty,ax
        push    dx                      ; store the center coords
        push    cx
        mov     wait_retrace,0080h      ; wait for vertical retrace
        call    VESAscroll              ; scroll there the screen
        pop     cx                      ; restore the center coords
        pop     dx
        cmp     ax,004fh                ; did it scroll?
        jne     noretrace               ;  no, try it another way..
        jmp     short wedone
noretrace:
;;        xor     ax,ax
        mov     ax,2                    ; try to move a tad
        mov     wait_retrace,0          ; don't wait for v. retrace
        mov     video_startx,ax         ; video didn't scroll yet
        mov     video_starty,ax
        call    VESAscroll              ; try the instant scrolling
        cmp     ax,004fh                ; did this scroll?
        je      wedone
bad_luck:
        mov     cx,vesa_xres            ; VESA failed.. restore dimensions
        mov     dx,vesa_yres
        cmp     sxdots,cx               ; asked wider than screen?
        jna     width_notbad
        mov     sxdots,cx               ;  correct it
width_notbad:
        cmp     sydots,dx               ; asked higher than screen?
        jna     wedonebad
        mov     sydots,dx               ;  correct it
wedonebad:
        mov     video_scroll,0          ;  no, disable the scrolling
wedone:
        ret
VESAvirtscan    endp


; ********* Functions setfortext() and setforgraphics() ************

;       setfortext() resets the video for text mode and saves graphics data
;       setforgraphics() restores the graphics mode and data
;       setclear() clears the screen after setfortext()

monocolors db  0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

setfortext      proc    uses es si di
        push    bp                      ; save it around the int 10s
        mov     setting_text,1
        cmp     dotmode, 12             ;check for 8514
        jne     tnot8514
        cmp     f85flag, 0              ;check 8514 active flag
        je      go_dosettext
        cmp     ai_8514, 0              ;using registers? JCO 4/11/92
        jne     not85reg
        call    far ptr close8514hw             ;close adapter if not, with registers
        jmp     done85close
not85reg:
        call    far ptr close8514               ;close adapter if not, with afi
done85close:
        mov     f85flag, 0
go_dosettext:
        jmp     dosettext               ; safe to go to mode 3
tnot8514:
        cmp     dotmode,9               ; Targa?
        je      go_dosettext            ;  yup, leave it open & go to text

        cmp     xga_isinmode,0          ; XGA in graphics mode?
        je      noxga                   ;  nope
        mov     ax,xga_isinmode         ; remember if we were in XGA mode
        mov     xga_clearvideo,80h      ; don't clear the video!
        mov     ax,0                    ; switch to VGA (graphics) mode
        push    ax
        call    far ptr xga_mode
        pop     ax                      ; proceed on like nothing happened
        mov     xga_clearvideo,0        ; reset the clear-video flag
noxga:

        cmp     dotmode,14              ; (klooge for Tandy 640x200x16)
        je      setfortextcga
        cmp     videoax,0               ; check for CGA modes
        je      go_setfortextnocga      ;  not this one
        cmp     dotmode,10              ; Hercules?
        je      setfortextcga           ;  yup
        cmp     videoax,7               ; <= vid mode 7?
        jbe     setfortextcga           ;  yup
go_setfortextnocga:
        jmp     setfortextnocga         ;  not this one
setfortextcga:  ; from mode ensures we can go to mode 3, so do it
        mov     ax,extraseg             ; set ES == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     es,ax                   ;  ...
        mov     di,4000h                ; save the video data here
        mov     ax,0b800h               ; video data starts here <XXX>
        mov     si,0                    ;  ...
        cmp     videoax,3               ; from mode 3?
        jne     setfortextcga2          ;  nope
        cmp     mode7text,0             ; egamono/hgc?
        je      setfortextcga2          ;  nope
        mov     ax,0b000h               ; video data starts here
setfortextcga2:
        mov     cx,2000h                ; save this many words
        cmp     dotmode,10              ; Hercules?
        jne     setfortextcganoherc     ;  nope
        mov     di,0                    ; (save 32K)
        mov     ax,0b000h               ; (from here)
        mov     cx,4000h                ; (save this many words)
setfortextcganoherc:
        cmp     dotmode,14              ; check for Tandy 1000 specific modes
        jne     setfortextnotandy       ; ..
        mov     ax,tandyseg             ; video data starts here
        mov     si,tandyofs             ; save video data here
        mov     di,0                    ; save the video data here
        mov     cx,4000h                ; save this many words
setfortextnotandy:
        push    ds                      ; save DS for a tad
        mov     ds,ax                   ;  reset DS
        cld                             ; clear the direction flag
        rep     movsw                   ;  save them.
        pop     ds                      ; restore DS
        cmp     dotmode,10              ; Hercules?
        jne     dosettext               ;  nope
        cmp     HGCflag, 0              ; check HGC active flag
        je      dosettext
        call    hgcend                  ; close adapter
        mov     HGCflag, 0
dosettext:
        mov     ax,3                    ; set up the text call
        mov     bx,0                    ;  ...
        mov     cx,0                    ;  ...
        mov     dx,0                    ;  ...
        call    setvideo                ; set the video
        jmp     setfortextreturn
setfortextnocga:
        mov     bios_vidsave,0          ; default, not using bios for state save
        mov     ax,textsafe2            ; videotable override?
        cmp     ax,0                    ;  ...
        jne     setfortextsafe          ;  yup
        mov     ax,textsafe             ; nope, use general setting
setfortextsafe:
        cmp     ax,2                    ; textsafe=no?
        jne     setforcolortext         ;  nope
        jmp     setfordummytext         ;  yup, use 640x200x2
setforcolortext:
        ; must be ega, mcga, or vga, else we'd have set textsafe=no in runup
        cmp     ax,4                    ; textsafe=save?
        jne     setforcolortext2        ;  nope
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        call    far ptr savegraphics    ; C rtn which uses swapsetup
        call    swapvga_reset           ; some cleanup for swapvga case
        mov     orvideo,80h             ; preserve memory, just for speed
        cmp     videoax,0fh             ; ega 640x350x2?
        jne     dosettext               ;  nope, go call bios for mode 3
        mov     ax,83h                  ; I know this is silly! have to set
        int     10h                     ; mode twice when coming from mode 0fh
        jmp     dosettext               ; on some machines to get right colors
setforcolortext2:
        cmp     ax,3                    ; textsafe=bios?
        jne     setforcolortext3        ;  nope
        cmp     video_type,5            ; vga?
        jl      setforcolortext3        ;  nope
        mov     ax,1c00h                ; check size of reqd save area
        mov     cx,3                    ;  for hardware + bios states
        int     10h                     ;  ask...
        cmp     al,1ch                  ; function recognized?
        jne     setforcolortext3        ;  nope
        cmp     bx,4                    ; buffer big enough? (3 seems usual)
        ja      setforcolortext3        ;  nope
        mov     bios_vidsave,1          ; using bios to save vid state
        mov     ax,cs                   ; ptr to save buffer
        mov     es,ax                   ;  ...
        mov     bx,offset bios_savebuf  ;  ...
        mov     ax,1c01h                ;  save state
        mov     cx,3                    ;  hardware + bios
        int     10h                     ;  ...
setforcolortext3:
        push    ds                      ; save ds
        mov     ax,extraseg             ; set ES == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     es,ax                   ;  ...
        cld
        mov     ax,0a000h               ; video mem address
        cmp     video_type,4            ; mcga?
        jne     setfortextegavga        ;  nope
        mov     ds,ax                   ; set ds to video mem
        xor     di,di                   ; from vid offset 0
        xor     si,si                   ; to save offset 0
        mov     cx,1000h                ; save 4k words
        rep movsw                       ; font info
        mov     si,8000h                ; from vid offset 8000h
        mov     cx,0800h                ; save 2k words
        rep movsw                       ; characters and attributes
        pop     ds                      ; restore ds
        mov     orvideo,80h             ; set the video to preserve memory
        jmp     dosettext               ; (else more than we saved gets cleared)
setfortextegavga:
        sub     ax,ax                   ; set bank just in case
        call far ptr newbank
        mov     ax,8eh                  ; switch to a mode with known mapping
        cmp     videoax,0fh             ; coming from ega 640x350x2?
        jne     sftknownmode            ;  nope
        mov     ax,8fh                  ; yup, stay in it but do the set mode
sftknownmode:
        int     10h                     ; set the safe mode
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        mov     ax,0a000h               ; video mem address
        mov     ds,ax                   ; set ds to video mem
        xor     di,di                   ; to offset 0 in save area
;;      mov     cx,0                    ; set to plane 0
;;      call    select_vga_plane        ;  ...
;;      mov     cx,0800h                ; save 2k words
;;      xor     si,si                   ; from offset 0 in vid mem
;;      rep movsw                       ; save plane 0 2k bytes (char values)
        mov     cx,2                    ; set to plane 2
        call    select_vga_plane        ;  ...
        mov     cx,1000h                ; save 4k words
        xor     si,si                   ; from offset 0 in vid mem
        rep movsw                       ; save plane 2 8k bytes (font)
;;      mov     cx,1                    ; set to plane 1
;;      call    select_vga_plane        ;  ...
;;      mov     cx,0800h                ; save 2k words
;;      xor     si,si                   ; from offset 0 in vid mem
;;      rep movsw                       ; save plane 1 2k bytes (attributes)
;;      push    ds                      ; now zap attributes to zero to
;;      pop     es                      ;  avoid flicker in the next stages
;;      xor     di,di                   ;  ...
;;      xor     ax,ax                   ;  ...
;;      mov     cx,0400h                ;  ...
;;      rep stosw                       ;  ...
        pop     ds                      ; restore ds
        mov     orvideo,80h             ; set the video to preserve memory
        mov     ax,3                    ; set up the text call
        mov     bx,0                    ;  ...
        mov     cx,0                    ;  ...
        mov     dx,0                    ;  ...
        call    setvideo                ; set the video
        push    ds                      ; save ds
        mov     ax,extraseg             ; set ES == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     es,ax                   ;  ...
        mov     ax,textaddr
        mov     ds,ax
        cld
        xor     si,si
        mov     di,2000h                ; past the saved font info
        mov     cx,0800h                ; 2k words (text & attrs)
        rep     movsw                   ; save them
        pop     ds
        jmp     setfortextreturn
setfordummytext:                        ; use 640x200x2 simulated text mode
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        mov     orvideo,80h             ; set the video to preserve memory
        mov     ax,6                    ; set up the text call
        mov     bx,0                    ;  ...
        mov     cx,0                    ;  ...
        mov     dx,0                    ;  ...
        call    setvideo                ; set the video
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        cld                             ; clear the direction flag
        mov     ax,extraseg             ; set ES == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     es,ax                   ;  ...
        mov     di,4000h                ; save the video data here
        mov     ax,0b800h               ; video data starts here
        push    ds                      ; save DS for a tad
        mov     ds,ax                   ;  reset DS
        mov     si,0                    ;  ...
        mov     cx,4000                 ; save this many words
        rep     movsw                   ;  save them.
        mov     si,2000h                ;  ...
        mov     cx,4000                 ; save this many words
        rep     movsw                   ;  save them.
        pop     ds                      ; restore DS
        mov     ax,0b800h               ; clear the video buffer
        mov     es,ax                   ;  ...
        mov     di,0                    ;  ...
        mov     ax,0                    ; to blanks
        mov     cx,4000                 ; this many blanks
        rep     stosw                   ; do it.
        mov     di,2000h                ;  ...
        mov     cx,4000                 ; this many blanks
        rep     stosw                   ; do it.
        mov     ax,20h                  ; enable the video (I think)
        call    disablevideo            ;  ...
        mov     bx,23                   ; set mode 6 fgrd to grey
        mov     cx,2a2ah                ; register 23, rgb white
        mov     dh,2ah                  ;  ...
        mov     ax,1010h                ; int 10 10-10 affects mcga,vga
        int     10h                     ;  ...
        mov     ax,cs                   ; do it again, another way
        mov     es,ax                   ;  ...
        mov     dx,offset monocolors    ;  ...
        mov     ax,1002h                ; int 10 10-02 handles pcjr,ega,vga
        int     10h                     ;  ...

setfortextreturn:
        call    far ptr setclear        ; clear and home the cursor
        pop     bp
        ret
setfortext      endp

setforgraphics  proc    uses es si di
        push    bp                      ; save it around the int 10s
        mov     setting_text,0
        cmp     dotmode, 12             ;check for 8514
        jne     gnot8514
        cmp     f85flag, 0
        jne     go_graphicsreturn
        cmp     ai_8514, 0              ;check afi flag JCO 4/11/92
        jne     reopenafi
        call    far ptr reopen8514hw    ;use registers
        jmp     reopen85done
reopenafi:
        call    far ptr reopen8514              ;use afi
reopen85done:
        mov     f85flag, 1
go_graphicsreturn:
        jmp     setforgraphicsreturn
gnot8514:
        cmp     dotmode,9               ; Targa?
        jne     gnottarga               ;  nope
        call    far ptr ReopenTGA
        jmp     short go_graphicsreturn
gnottarga:

        cmp     dotmode,14              ; check for Tandy 1000 specific modes
        je      setforgraphicscga       ;  yup
        cmp     videoax,0               ; check for CGA modes
        je      setforgraphicsnocga_x   ;  not this one
        cmp     dotmode,10              ; Hercules?
        je      setforgraphicscga       ;  yup
        cmp     videoax,7               ; vid mode <=7?
        jbe     setforgraphicscga       ;  CGA mode
setforgraphicsnocga_x:
        jmp     setforgraphicsnocga
setforgraphicscga:
        cmp     dotmode,10              ; Hercules?
        jne     tnotHGC2                ;  (nope.  dull-normal stuff)
        call    hgcstart                ; Initialize the HGC card
        mov     HGCflag,1               ; flag "HGC-end" needed.
        jmp     short twasHGC2          ; bypass the normal setvideo call
tnotHGC2:
        cmp     dotmode,14              ; Tandy?
        jne     tnottandy1              ;  nope
        mov     orvideo,80h             ; set the video to preserve memory
tnottandy1:
        mov     ax,videoax              ; set up the video call
        mov     bx,videobx              ;  ...
        mov     cx,videocx              ;  ...
        mov     dx,videodx              ;  ...
        call    setvideo                ; do it.
twasHGC2:
        mov     bx,extraseg             ; restore is from Extraseg
        add     bx,1000h                ; (plus 64K)
        mov     si,4000h                ; video data is saved here
        mov     ax,0b800h               ; restore the video area
        mov     di,0                    ;  ...
        cmp     videoax,3               ; from mode 3?
        jne     setforgraphicscga2      ;  nope
        cmp     mode7text,0             ; egamono/hgc?
        je      setforgraphicscga2      ;  nope
        mov     ax,0b000h               ; video data starts here
setforgraphicscga2:
        mov     cx,2000h                ; restore this many words
        cmp     dotmode,10              ; Hercules?
        jne     setforgraphicscganoherc ;  nope
        mov     si,0                    ; (restore 32K)
        mov     ax,0b000h               ; (to here)
        mov     cx,4000h                ; (restore this many words)
setforgraphicscganoherc:
        cmp     dotmode,14              ; check for Tandy 1000 specific modes
        jne     tnottandy2              ; ..
        mov     ax,tandyseg             ; video data starts here
        mov     di,tandyofs             ; save video data here
        mov     si,0                    ; video data is saved here
        mov     cx,4000h                ; save this many words
tnottandy2:
        push    ds                      ; save DX for a tad
        mov     es,ax                   ; load the dest seg into ES
        mov     ds,bx                   ; restore it from the source seg
        cld                             ; clear the direction flag
        rep     movsw                   ; restore them.
        pop     ds                      ; restore DS
        jmp     setforgraphicsreturn
setforgraphicsnocga:
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        cld                             ; clear the direction flag
        mov     ax,textsafe2            ; videotable override?
        cmp     ax,0                    ;  ...
        jne     setforgraphicssafe      ;  yup
        mov     ax,textsafe             ; nope, use general setting
setforgraphicssafe:
        cmp     ax,2                    ; textsafe=no?
        jne     setforgraphicsnocga2
        jmp     setfordummygraphics     ;  yup, 640x200x2
setforgraphicsnocga2:
        ; must be ega, mcga, or vga, else we'd have set textsafe=no in runup
        cmp     ax,4                    ; textsafe=save?
        jne     setforgraphicsnocga3    ;  nope
;; pb, always clear video here:
;;  need if for ega/vga with < 16 colors to clear unused planes
;;  and, some bios's don't implement ah or'd with 80h?!?, dodge their bugs
;;      cmp     dotmode,2               ; ega/vga, <=16 colors?
;;      jne     setfgncfast             ;  nope
;;      cmp     colors,16               ; < 16 colors?
;;      jae     setfgncfast             ;  nope
;;      jmp     short setfgncsetvid     ; special, need unused planes clear
;;setfgncfast:
;;      mov     orvideo,80h             ; preserve memory (just to be fast)
;;setfgncsetvid:
        mov     orvideo,00h   ; JRS          ; preserve memory (just to be fast)
        mov     ax,videoax              ; set up the video call
        mov     bx,videobx              ;  ...
        mov     cx,videocx              ;  ...
        mov     dx,videodx              ;  ...
        call    setvideo                ; do it.
        sub     ax,ax                   ; disable the video (I think)
        call    disablevideo            ;  ...
        call    far ptr restoregraphics ; C rtn which uses swapsetup
        call    swapvga_reset           ; some cleanup for swapvga case
        mov     ax,20h                  ; enable the video (I think)
        call    disablevideo            ;  ...
        jmp     setforgraphicsreturn
setforgraphicsnocga3:
        push    ds                      ; save ds
        mov     ax,0a000h               ; set es to video mem
        mov     es,ax                   ;  ...
        cmp     video_type,4            ; mcga?
        jne     setforgraphicsegavga    ;  nope
        mov     ax,extraseg             ; set DS == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     ds,ax                   ;  ...
        xor     si,si                   ; from save offset 0
        xor     di,di                   ; to vid offset 0
        mov     cx,1000h                ; restore 4k words
        rep movsw                       ; font info
        mov     di,8000h                ; to vid offset 8000h
        mov     cx,0800h                ; restore 2k words
        rep movsw                       ; characters and attributes
        jmp     short setforgraphicsdoit
setforgraphicsegavga:
;;      mov     cx,0                    ; set to plane 0
;;      call    select_vga_plane        ;  ...
;;      mov     ax,extraseg             ; set DS == Extra Segment
;;      add     ax,1000h                ; (plus 64K)
;;      mov     ds,ax                   ;  ...
;;      mov     cx,0800h                ; restore 2k words
;;      xor     si,si                   ; from offset 0 in save area
;;      xor     di,di                   ; to offset 0 in vid mem
;;      rep movsw                       ; restore plane 0 2k bytes (char values)
        mov     ax,textaddr
        mov     es,ax
        mov     ax,extraseg             ; set DS == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     ds,ax                   ;  ...
        cld
        xor     di,di
        mov     si,2000h                ; past the saved font info
        mov     cx,0800h                ; 2k words (text & attrs)
        rep     movsw                   ; restore them
        pop     ds
        mov     ax,8eh                  ; switch to a mode with known mapping
        cmp     videoax,0fh             ; returning to ega 640x350x2?
        jne     sfgnotega               ;  nope
        mov     ax,8fh                  ; yup, go directly to it
sfgnotega:
        int     10h                     ; set the safe mode
        mov     ax,0                    ; disable the video (I think)
        call    disablevideo            ;  ...
        mov     cx,2                    ; set to plane 2
        call    select_vga_plane        ;  ...
        push    ds
        mov     ax,0a000h               ; set es to video mem
        mov     es,ax                   ;  ...
        mov     ax,extraseg             ; set DS == Extra Segment
        add     ax,1000h                ; (plus 64K)
        mov     ds,ax                   ;  ...
        mov     cx,1000h                ; restore 4k words
        xor     di,di                   ; to offset 0 in vid mem
        xor     si,si                   ; from offset 0 in extraseg
        rep movsw                       ; restore plane 2 8k bytes (font)
;;      mov     cx,1                    ; set to plane 1
;;      call    select_vga_plane        ;  ...
;;      mov     cx,0800h                ; restore 2k words
;;      xor     di,di                   ; to offset 0 in vid mem
;;      rep movsw                       ; restore plane 1 2k bytes (attributes)
        jmp     short setforgraphicsdoit
setfordummygraphics:
        push    ds                      ; save ds
        mov     ax,0b800h               ; restore the video area
        mov     es,ax                   ; ES == video addr
        mov     di,0                    ;  ...
        mov     ax,extraseg             ;  ...
        add     ax,1000h                ; (plus 64K)
        mov     ds,ax                   ;  ...
        mov     si,4000h                ; video data is saved here
        mov     cx,4000                 ; restore this many words
        rep     movsw                   ; restore them.
        mov     di,2000h
        mov     cx,4000                 ; restore this many words
        rep     movsw                   ; restore them.
setforgraphicsdoit:
        pop     ds                      ; restore DS
        cmp     bios_vidsave,0          ; did setfortext use bios state save?
        je      setforgraphicssetvid    ;  nope
        mov     ax,cs                   ; ptr to save buffer
        mov     es,ax                   ;  ...
        mov     bx,offset bios_savebuf  ;  ...
        mov     ax,1c02h                ;  restore state
        mov     cx,3                    ;  hardware + bios
        int     10h                     ;  ...
        jmp     short setforgraphicsreturn
setforgraphicssetvid:
        mov     orvideo,80h             ; set the video to preserve memory
        mov     ax,videoax              ; set up the video call
        mov     bx,videobx              ;  ...
        mov     cx,videocx              ;  ...
        mov     dx,videodx              ;  ...
        call    setvideo                ; do it.
        mov     ax,20h                  ; enable the video (I think)
        call    disablevideo            ;  ...
setforgraphicsreturn:
        mov     curbk,0ffffh            ; stuff impossible value into cur-bank
        mov     ax,1                    ; set up call to spindac(0,1)
        push    ax                      ;  ...
        mov     ax,0                    ;  ...
        push    ax                      ;  ...
        call    far ptr spindac         ; do it.
        pop     ax                      ; restore the registers
        pop     ax                      ;  ...
        pop     bp
        ret
setforgraphics  endp

; swapxxx routines: indirect call via swapsetup, used by savegraphics
;                   and restoregraphics

swap256         proc uses es si di      ; simple linear banks version
        mov     ax,word ptr swapoffset+2; high word of offset is bank
        call    far ptr newbank         ; map in the bank
        mov     ax,0a000h               ; high word of vid addr
        mov     word ptr swapvidbuf+2,ax; store it
        mov     ax,word ptr swapoffset  ; offset in bank
        mov     word ptr swapvidbuf,ax  ; store as low word of vid addr
        neg     ax                      ; 65536-offset
        jz      swap256ret              ; offset 0
        cmp     ax,swaplength           ; rest of bank smaller than req length?
        jae     swap256ret              ;  nope
        mov     swaplength,ax           ; yup, reduce length to what's available
swap256ret:
        ret
swap256         endp

swapvga         proc uses es si di      ; 4 (or less) planes version
        xor     si,si                   ; this will be plane number
        mov     ax,word ptr swaptotlen  ; dx,ax = total length
        mov     dx,word ptr swaptotlen+2
        mov     cx,word ptr swapoffset  ; bx,cx = offset
        mov     bx,word ptr swapoffset+2
        cmp     colors,4                ; 4 or 2 color mode?
        jle     swapvga_2               ;  yup, no plane 2/3
        shr     dx,1                    ; 1/2 of total
        rcr     ax,1                    ;  ...
        cmp     dx,bx                   ; 1/2 total <= offset?
        jb      swapvga_1               ;  yup
        ja      swapvga_2               ;  nope
        cmp     ax,cx                   ; ...
        ja      swapvga_2               ;  nope
swapvga_1:
        mov     si,2                    ; plane 2
        sub     cx,ax                   ; subtract 1/2 total from offset
        sbb     bx,dx                   ;  ...
swapvga_2:
        cmp     colors,2                ; 2 color mode?
        jle     swapvga_4               ;  yup, just plane 0
        shr     dx,1                    ; 1/4 of total (or 1/2 if 4 colors)
        rcr     ax,1                    ;  ...
        cmp     dx,bx                   ; 1/4 total <= remaining offset?
        jb      swapvga_3               ;  yup
        ja      swapvga_4               ;  nope
        cmp     ax,cx                   ; ...
        ja      swapvga_4               ;  nope
swapvga_3:
        inc     si                      ; plane 1 or 3
        sub     cx,ax                   ; subtract 1/4 total from offset
        sbb     bx,dx                   ;  ...
swapvga_4:
        mov     di,0                    ; bank size (65536 actually)
        cmp     bx,dx                   ; in last bank?
        jne     swapvga_5               ;  nope
        mov     di,ax                   ; yup, note its size
swapvga_5:
        mov     ax,0a000h               ; high word of vid addr
        mov     word ptr swapvidbuf+2,ax; store it
        mov     word ptr swapvidbuf,cx  ; low word of vid addr (offset in bank)
        sub     di,cx                   ; bank size - offset
        je      swapvga_6               ; 0 implies bank = 65536
        cmp     di,swaplength           ; rest of bank smaller than req length?
        jae     swapvga_6               ;  nope
        mov     swaplength,di           ; yup, reduce length to what's available
swapvga_6:
        push    si                      ; remember plane
        mov     ax,bx                   ; top word of offset is bank
        call    far ptr newbank         ; map in the bank
        mov     dx,03ceh                ; graphics controller address
        pop     cx                      ; plane
        mov     ah,cl                   ;  ...
        mov     al,04h                  ; set up controller address register
        out     dx,ax                   ; map the plane into memory for reads
        mov     ax,0001h                ; set/reset enable - all bits processor
        out     dx,ax                   ;  ...
        mov     ax,0ff08h               ; bit mask - all bits processor
        out     dx,ax                   ;  ...
        mov     dx,03c4h                ; sequencer address
        mov     ah,1                    ; 1 << plane number
        shl     ah,cl                   ;  ...
        mov     al,02h                  ; sequencer plane write enable register
        out     dx,ax                   ; enable just this plane for writing
        ret
swapvga         endp

swapvga_reset   proc near
        cmp     word ptr swapsetup,offset swapvga; swapvga being used?
        jne     swapvga_reset_ret
        mov     ax,0f02h                ; write enable register, all planes
        mov     dx,03c4h                ; sequencer address
        out     dx,ax
swapvga_reset_ret:
        ret
swapvga_reset   endp

swapnormread    proc uses es si di      ; the SLOW version
        local   xdot:word,ydot:word,bytesleft:word,bits:word,bitctr:word
        local   savebyte:byte
        mov     ax,swaplength           ; bytes to save
        mov     bytesleft,ax            ;  ...
        mov     bits,8                  ; bits/pixel
        mov     bx,word ptr swapoffset  ; pixel offset to save
        mov     cx,word ptr swapoffset+2
        mov     ax,colors               ; colors
swapnorm_bits:
        cmp     ax,256                  ; accounted for 8 bits yet?
        jae     swapnorm_gotbits        ;  yup
        shr     bits,1                  ; nope, halve the bits/pixel
        shl     bx,1                    ; double the pixel offset
        rcl     cx,1                    ;  ...
        mul     ax                      ; square colors accounted for
        jmp     short swapnorm_bits     ; check if enough yet
swapnorm_gotbits:
        mov     ax,bx                   ; pixel offset now in dx:ax
        mov     dx,cx                   ;  ...
        div     sxdots                  ; translate pixel offset to row/col
        mov     ydot,ax                 ;  ...
        mov     xdot,dx                 ;  ...
        mov     ax,extraseg             ; Extra Segment
        add     ax,1000h                ;  plus 64K
        mov     word ptr swapvidbuf+2,ax; temp buffer to return to caller
        mov     word ptr swapvidbuf,0   ;  ...
        mov     word ptr tmpbufptr+2,ax ; running ptr for building buffer
        mov     word ptr tmpbufptr,0    ;  ...
        mov     bitctr,0
swapnorm_loop:
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ;  for dotread
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ;  for the video routine
        call    dotread                 ; read the dot via the approved method
        mov     cx,bits                 ; bits per pixel
        cmp     cx,8                    ; 1 byte per pixel?
        je      swapnorm_store          ;  yup, go store a byte
        add     bitctr,cx               ; nope, add how many we're storing
        mov     bl,savebyte             ; load the byte being built
swapnorm_shift:
        shr     al,1                    ; shift pixel into byte
        rcr     bl,1                    ;  ...
        loop    swapnorm_shift          ; for number of bits/pixel
        mov     savebyte,bl             ; save byte we're building
        cmp     bitctr,8                ; filled a byte yet?
        jb      swapnorm_nxt            ;  nope
        mov     al,bl                   ; yup, set up to store
        mov     bitctr,0                ; clear counter for next time
swapnorm_store:
        les     di,tmpbufptr            ; buffer pointer
        stosb                           ; store byte
        dec     bytesleft               ; finished?
        jz      swapnorm_ret            ;  yup
        mov     word ptr tmpbufptr,di   ; store incremented buffer pointer
swapnorm_nxt:
        inc     xdot                    ; for next dotread call
        mov     ax,xdot                 ; past row length?
        cmp     ax,sxdots               ;  ...
        jb      swapnorm_loop           ;  nope, go for next pixel
        inc     ydot                    ; yup, increment row
        mov     xdot,0                  ; and reset column
        jmp     short swapnorm_loop     ; go for next pixel
swapnorm_ret:
        ret
swapnormread    endp

swapnormwrite   proc uses es si di      ; the SLOW way
        local   xdot:word,ydot:word,bytesleft:word,bits:word,bitctr:word
        local   savebyte:byte
        mov     ax,swaplength           ; bytes to restore
        inc     ax                      ; +1
        mov     bytesleft,ax            ; save it
        mov     bits,8                  ; bits/pixel
        mov     bx,word ptr swapoffset  ; pixel offset to restore
        mov     cx,word ptr swapoffset+2;  ...
        mov     ax,colors               ; colors
swapnormw_bits:
        cmp     ax,256                  ; accounted for 8 bits yet?
        jae     swapnormw_gotbits       ;  yup
        shr     bits,1                  ; nope, halve the bits/pixel
        shl     bx,1                    ; double the pixel offset
        rcl     cx,1                    ;  ...
        mul     ax                      ; square colors accounted for
        jmp     short swapnormw_bits    ; check if enough yet
swapnormw_gotbits:
        mov     ax,bx                   ; pixel offset now in dx:ax
        mov     dx,cx                   ;  ...
        div     sxdots                  ; translate pixel offset to row/col
        mov     ydot,ax                 ;  ...
        mov     xdot,dx                 ;  ...
        mov     dx,word ptr swapvidbuf+2; temp buffer from caller
        mov     ax,word ptr swapvidbuf  ;  ...
        mov     word ptr tmpbufptr+2,dx ; running ptr in buffer
        mov     word ptr tmpbufptr,ax   ;  ...
        mov     bitctr,0
swapnormw_loop:
        mov     cx,bits                 ; bits per pixel
        sub     bitctr,cx               ; subtract a pixel from counter
        jg      swapnormw_extract       ;  got some left in savebyte
        dec     bytesleft               ; all done?
        jz      swapnormw_ret           ;  yup
        les     si,tmpbufptr            ; buffer pointer
        mov     dl,es:[si]              ; next byte
        inc     word ptr tmpbufptr      ; incr buffer pointer for next time
        mov     bitctr,8                ; reset unused bit count
        cmp     cx,8                    ; 1 byte/pixel?
        je      swapnormw_store         ;  yup, go the fast way
        mov     bl,dl                   ; byte to extract from
        jmp     short swapnormw_extract2
swapnormw_extract:
        mov     bl,savebyte             ; byte with bits left to use
        mov     dl,bl                   ; current pixel in bottom n bits
swapnormw_extract2:
        shr     bl,1                    ; for next time
        loop    swapnormw_extract2      ; for bits per pixel
        mov     savebyte,bl             ; save remaining bits
swapnormw_store:
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ;  for dotread
        mov     al,dl                   ; color to write
        and     ax,andcolor             ;  ...
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ;  for the video routine
        call    dotwrite                ; write the dot via the approved method
        inc     xdot                    ; for next dotread call
        mov     ax,xdot                 ; past row length?
        cmp     ax,sxdots               ;  ...
        jb      swapnormw_loop          ;  nope, go for next pixel
        inc     ydot                    ; yup, increment row
        mov     xdot,0                  ; and reset column
        jmp     short swapnormw_loop    ; go for next pixel
swapnormw_ret:
        ret
swapnormwrite   endp

; far move routine for savegraphics/restoregraphics

movewords proc  uses es di si, len:word, fromptr:dword, toptr:dword
        push    ds                      ; save DS
        mov     cx,len                  ; words to move
        les     di,toptr                ; destination buffer
        lds     si,fromptr              ; source buffer
        cld                             ; direction=forward
        rep     movsw                   ; do it.
        pop     ds                      ; restore DS
        ret                             ; we done.
movewords endp

; clear text screen

setclear        proc    uses es si di   ; clear the screen after setfortext
        call    far ptr home            ; home the cursor
        mov     ax,textaddr
        mov     es,ax
        xor     di,di
        cmp     text_type,0             ; real text mode?
        jne     setcbw                  ;  nope
        mov     ax,0720h                ; blank with white attribute
        mov     cx,2000                 ; 80x25
        rep     stosw                   ; clear it
        jmp     short setcdone
setcbw: xor     ax,ax                   ; graphics text
        mov     cx,4000                 ; 640x200x2 / 8
        rep     stosw                   ; clear it
        mov     di,2000h                ; second part
        mov     cx,4000
        rep     stosw                   ; clear it
setcdone:
        xor     ax,ax                   ; zero
        mov     textrbase,ax            ; clear this
        mov     textcbase,ax            ;  and this
        ret                             ; we done.
setclear        endp


disablevideo    proc    near            ; wierd video trick to disable/enable
        push    dx                      ; save some registers
        push    ax                      ;  ...
        mov     dx,03bah                ; set attribute comtroller flip-flop
        in      al,dx                   ;  regardless of video mode
        mov     dx,03dah                ;  ...
        in      al,dx                   ;  ...
        mov     dx,03c0h                ; attribute controller address
        pop     ax                      ; 00h = disable, 20h = enable
        out     dx,al                   ;  trust me.
        pop     dx                      ; restore DX and we done.
        ret
disablevideo    endp

; ************** Function findfont(n) ******************************

;       findfont(0) returns far pointer to 8x8 font table if it can
;                   find it, NULL otherwise;
;                   nonzero parameter reserved for future use

findfont        proc    uses es si di, fontparm:word
        mov     ax,fontparm             ; to quiet warning
        mov     ax,01130h               ; func 11, subfunc 30
        mov     bh,03h                  ; 8x8 font, bottom 128 chars
        sub     cx,cx                   ; so we can tell if anything happens
        int     10h                     ; ask bios
        sub     ax,ax                   ; default return, NULL
        sub     dx,dx                   ;  ...
        or      cx,cx                   ; did he set cx?
        jz      findfontret             ; nope, return with NULL
        mov     dx,es                   ; yup, return far pointer
        mov     ax,bp                   ;  ...
findfontret:
        ret                             ; note that "uses" gets bp reset here
findfont        endp

; **************** Function home()  ********************************

;       Home the cursor (called before printfs)

home    proc
        mov     ax,0200h                ; force the cursor
        mov     bx,0                    ; in page 0
        mov     dx,0                    ; to the home position
        mov     textrow,dx              ; update our local values
        mov     textcol,dx              ;  ...
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret
home    endp

; **************** Function movecursor(row, col)  **********************

;       Move the cursor (called before printfs)

movecursor      proc    row:word, col:word
        mov     ax,row                  ; row specified?
        cmp     ax,-1                   ;  ...
        je      mccol                   ;  nope, inherit it
        mov     textrow,ax              ; yup, store it
mccol:  mov     ax,col                  ; col specified?
        cmp     ax,-1                   ;  ...
        je      mcdoit                  ;  nope, inherit it
        mov     textcol,ax              ; yup, store it
mcdoit: mov     ax,0200h                ; force the cursor
        mov     bx,0                    ; in page 0
        mov     dh,byte ptr textrow     ; move to this row
        add     dh,byte ptr textrbase   ;  ...
        mov     dl,byte ptr textcol     ; move to this column
        add     dl,byte ptr textcbase   ;  ...
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret
movecursor      endp

; **************** Function keycursor(row, col)  **********************

;       Subroutine to wait cx ticks, or till keystroke pending

tickwait  proc
        FRAME   <es>                    ; std stack frame for TC++ overlays
tickloop1:
        push    cx                      ; save loop ctr
        sub     ax,ax                   ; set ES to BIOS data area
        mov     es,ax                   ;  ...
        mov     bx,es:046ch             ; obtain the current timer value
        push    bx
        call    far ptr keypressed      ; check if keystroke pending
        pop     bx
        pop     cx                      ; restore loop ctr
        cmp     ax,0                    ; keystroke?
        jne     tickret                 ;  yup, return
tickloop2:
        cmp     bx,es:046ch             ; a new clock tick started yet?
        je      tickloop2               ;  nope
        loop    tickloop1               ; wait another tick?
        xor     ax,ax                   ; nope, exit no key, ticks done
tickret:
        UNFRAME <es>                    ; pop stack frame
        ret
tickwait        endp

;       Show cursor, wait for a key, disable cursor, return key

keycursor       proc   uses es si di, row:word, col:word
        mov     cursortyp,0607h         ; default cursor
        mov     ax,row                  ; row specified?
        cmp     ax,-1                   ;  ...
        je      ckcol                   ;  nope, inherit it
        test    ax,08000h               ; top bit on?
        je      ckrow                   ;  nope
        and     ax,07fffh               ; yup, clear it
        mov     cursortyp,0507h         ; and use a bigger cursor
ckrow:  mov     textrow,ax              ; store row
ckcol:  mov     ax,col                  ; col specified?
        cmp     ax,-1                   ;  ...
        je      ckmode                  ;  nope, inherit it
        mov     textcol,ax              ; yup, store it
ckmode: cmp     text_type,1             ; are we in 640x200x2 mode?
        jne     ck_text                 ;  nope.  do it the easy way
ck_bwloop:                              ; 640x200x2 cursor loop
        mov     cx,3                    ; wait 3 ticks for keystroke
        call    far ptr tickwait        ;  ...
        cmp     ax,0                    ; got a keystroke?
        je      ckwait                  ;  nope
        jmp     ck_get                  ; yup, fetch it and return
ckwait: mov     ax,320                  ; offset in vid mem of top row 1st byte:
        mov     bx,textrow              ;  row
        add     bx,textrbase            ;  ...
        mul     bx                      ;  row*320 + col
        add     ax,textcol              ;  ...
        add     ax,textcbase            ;  ...
        mov     di,ax                   ;  ...
        mov     ax,0b800h               ; set es to vid memory
        mov     es,ax                   ;  ...
        mov     ah,byte ptr es:00f0h[di]; row 6 of character
        mov     al,byte ptr es:20f0h[di]; row 7 of character
        push    ax                      ; save them
        mov     cx,8                    ; count on bits in orig value
        xor     bx,bx                   ;  ...
ckbits: shl     al,1                    ;  ...
        adc     bx,0                    ;  ...
        loop    ckbits                  ;  ...
        mov     al,0                    ; black cursor
        cmp     bx,4                    ; >= 4 on bits in orig value?
        jge     ckbwgo                  ;  yup, use black cursor
        not     al                      ; nope, use white cursor
ckbwgo: mov     byte ptr es:20f0h[di],al; turn on cursor, row 7
        cmp     cursortyp,0607h         ; small cursor?
        je      ckbwwt                  ;  yup
        mov     byte ptr es:00f0h[di],al; nope, turn on cursor row 6
ckbwwt: mov     cx,3                    ; wait 3 ticks for keystroke
        call    far ptr tickwait        ;  ...
        pop     bx                      ; saved orig value
        mov     byte ptr es:00f0h[di],bh; turn off cursor, row 6
        mov     byte ptr es:20f0h[di],bl; turn off cursor, row 7
        cmp     ax,0                    ; got a keystroke?
        jne     ck_get                  ;  yup, fetch it and return
        jmp     short ck_bwloop         ; and keep waiting
ck_text:
        cmp     text_type,0             ; real text mode?
        jne     ckgetw                  ;  nope, no cursor at all
        push    bp                      ; some bios's don't save this
        mov     ah,1                    ; set cursor type
        mov     cx,cursortyp            ;  ...
        int     10h                     ;  ...
        mov     ah,02                   ; move cursor
        xor     bx,bx                   ;  page
        mov     dh,byte ptr textrow     ;  row
        add     dh,byte ptr textrbase   ;  ...
        mov     dl,byte ptr textcol     ;  col
        add     dh,byte ptr textcbase   ;  ...
        int     10h                     ;  ...
        pop     bp
ckgetw: call    far ptr keypressed      ; not getakey, help/tab mey be enabled
        cmp     ax,0                    ; key available?
        je      ckgetw                  ;  nope, keep waiting
ck_get: call    far ptr getakey         ; get the keystroke
        cmp     text_type,0             ; real text mode?
        jne     ck_ret                  ;  nope, done
        push    bp                      ; some bios's don't save this
        push    ax                      ; save it
        mov     ah,1                    ; make cursor normal size
        mov     cx,0607h                ;  ...
        int     10h                     ;  ...
        mov     ah,02                   ; move cursor
        xor     bx,bx                   ;  page
        mov     dx,1950h                ;  off the display
        int     10h                     ;  ...
        pop     ax                      ; keystroke value
        pop     bp
ck_ret:
        ret
keycursor       endp

; ************* Function scrollup(toprow, botrow) ******************

;       Scroll the screen up (from toprow to botrow)

scrollup        proc    uses    es, toprow:word, botrow:word

        mov     ax,0601h                ; scroll up one line
        mov     bx,0700h                ; new line is black
        mov     cx,toprow               ; this row,
        mov     ch,cl                   ;  ...
        mov     cl,0                    ;  first column
        mov     dx,botrow               ; to this row,
        mov     dh,dl                   ;  ...
        mov     dl,79                   ;  last column
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret                             ; we done.
scrollup        endp

; ************* Function scrolldown(toprow, botrow) ******************

;       Scroll the screen down (from toprow to botrow)

scrolldown      proc    uses    es, toprow:word, botrow:word

        mov     ax,0701h                ; scroll down one line
        mov     bx,0700h                ; new line is black
        mov     cx,toprow               ; this row,
        mov     ch,cl                   ;  ...
        mov     cl,0                    ;  first column
        mov     dx,botrow               ; to this row,
        mov     dh,dl                   ;  ...
        mov     dl,79                   ;  last column
        push    bp                      ; some BIOS's don't save this
        int     10h                     ; do it.
        pop     bp                      ; restore the saved register
        ret                             ; we done.
scrolldown      endp


; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point

getcolor        proc    uses di si es, xdot:word, ydot:word
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ; save it here during this routine
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ;  for the video routine
        add     cx,sxoffs               ;  ...
        add     dx,syoffs               ;  ...
        call    dotread                 ; read the dot via the approved method
        mov     ah,0                    ; clear the high-order bits
        ret                             ; we done.
getcolor        endp

; ********* Function gettruecolor(xdot, ydot, &red, &green, &blue) **************

;       Return the color on the screen at the (xdot,ydot) point

; gettruecolor ---------------------------------------------------------------
; * this is just version of VESAtrueread, so the changes are identical
; ------------------------------------------------------------30/06/2002/ChCh-

gettruecolor    proc    uses es, xdot:word, ydot:word, red:ptr word, green:ptr word, blue:ptr word
        cmp     istruecolor,1           ; are we in a truecolor mode?
        je      overdone
        jmp     wedone
overdone:
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ; for the video routine
        add     cx,sxoffs               ; add window offsets
        add     dx,syoffs               ; (dotwrite does this for VESAtrueread)
        call    VESAtrueaddr            ; calculate address and switch banks
        mov     ax,vesa_winaseg         ; VESA video starts here
        cmp     vesa_bitsppixel,17
        mov     es,ax
        jge     over_hi
        mov     ax,word ptr es:[bx]     ; read two bytes
        mov     cl,3
        mov     dx,ax
        shl     ax,cl
        mov     cl,vesa_redpos
        shr     al,1
        shr     dx,cl
        shr     al,1                    ; blue in al
        cmp     vesa_greensize,6
        je      got_6g
        shl     ah,1
got_6g:
        shl     dx,1                    ; red in dl
        and     ah,111111b              ; green in ah
        jmp     short give_out
over_hi:                                ; 8-8-8 or 8-8-8-8?
        inc     bx                      ; does a word fit to this bank?
        jz      badbank1                ; no, switch one byte after
        mov     ax,word ptr es:[bx-1]   ; else read that word
        inc     bx                      ; bank-end?
        jz      badbank2                ; yes, switch it
        mov     dl,es:[bx]              ; else read the third byte
        jmp     short colors_in
badbank1:
        dec     bx                      ; bx=0ffffh
        mov     al,es:[bx]              ; read the first byte
badbank2:
        inc     dx                      ; next bank needed
        push    ax
        mov     ax,dx                   ; newbank expects bank in ax
        call    far ptr newbank
        pop     ax
        mov     dl,es:[0]               ; read next color
        inc     bx                      ; badbank1 or badbank2?
        jnz     colors_in               ; badbank2 - nothing more to do
        mov     ah,dl
        mov     dl,es:[1]               ; badbank1 - read the last byte
colors_in:
        cmp     vesa_redpos,0
        jne     layout_ok
        xchg    al,dl
layout_ok:
        mov     cl,2
        xor     dh,dh
        and     ax,0FCFCh               ; mask-out 2 g & 2 b lsbs for shift
        shr     dx,cl
        shr     ax,cl
give_out:                               ; both for hi- & true- color,
        mov     bx,red
        mov     [bx],dx                 ; return red
        mov     bx,green
        mov     dl,ah
        mov     [bx],dx                 ; return green
        mov     bx,blue
        mov     dl,al
        mov     [bx],dx                 ; return blue
wedone:
        ret                             ; we done.
gettruecolor    endp

; Fastcall version, called when C programs are compiled by MSC 6.00A:

@getcolor       proc    FORTRAN ; ax=xdot, dx=ydot
        push    si                      ; preserve these
        push    di                      ;  ...
        mov     cx,ax                   ; load up the registers
        add     cx,sxoffs               ;  ...
        add     dx,syoffs               ;  ...
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ;  ...
        call    dotread                 ; read the dot via the approved method
        xor     ah,ah                   ; clear the high-order bits
        pop     di                      ; restore
        pop     si                      ;  ...
        ret                             ; we done.
@getcolor       endp

; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point

putcolor_a      proc    uses di si es, xdot:word, ydot:word, xcolor:word
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ; save it here during this routine
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ;  for the video routine
        add     cx,sxoffs               ;  ...
        add     dx,syoffs               ;  ...
        mov     ax,xcolor               ;  ...
        and     ax,andcolor             ; (ensure that 'color' is in the range)
        call    dotwrite                ; write the dot via the approved method
;;;     call    videocleanup            ; perform any video cleanup required
        ret                             ; we done.
putcolor_a      endp


; ******* Function puttruecolor(xdot, ydot, red, green, blue) *************

;       write the color on the screen at the (xdot,ydot) point

; puttruecolor ---------------------------------------------------------------
; * this is just version of VESAtruewrite, so the changes are identical
; ------------------------------------------------------------30-06-2002-ChCh-

puttruecolor    proc    uses es, xdot:word, ydot:word, red:word, green:word, blue:word
        cmp     istruecolor,1           ; are we in a truecolor mode?
        je      overdone
        jmp     wedone
overdone:
        mov     cx,xdot                 ; load up the registers
        mov     dx,ydot                 ; for the video routine
        add     cx,sxoffs               ; add window offsets
        add     dx,syoffs               ; (dotwrite does this for VESAtruewrite)
        call    VESAtrueaddr            ; calculate address and switch banks
        mov     ax,vesa_winaseg
        cmp     vesa_bitsppixel,17      ; 8-8-8 and 8-8-8-8
        mov     es,ax
        jge     over_hi
        push    bx
        mov     cx,111111b              ; mask - possibly a public variable
        mov     dx,green
        mov     bx,red
        mov     ax,blue
        and     dx,cx
        and     bx,cx
        and     ax,cx
        mov     cl,vesa_redpos
        cmp     vesa_greensize,6        ; 5-5-5 or 5-6-5
        je      got_6g
        shr     dx,1
got_6g:
        shr     bx,1
        shr     ax,1
        shl     bx,cl
        mov     cl,vesa_greenpos
        or      ax,bx                   ; r-_-b
        shl     dx,cl
        pop     bx
        or      ax,dx                   ; r-g-b
        mov     word ptr es:[bx],ax     ; write two bytes for the dot
        jmp     short wedone
over_hi:
        mov     al,byte ptr blue        ; 8-8-8 (and 8-8-8-8) style
        mov     cx,red
        mov     ah,byte ptr green
        shl     cx,1                    ; well, 6-6-6 is not true-true
        shl     ax,1
        shl     cx,1
        shl     ax,1
        cmp     vesa_redpos,0           ; common b-g-r model?
        jne     doit_slow
        xchg    al,cl                   ; else turn to unusual r-g-b
doit_slow:
        inc     bx                      ; does a word fit to the current bank?
        jz      badbank1                ; no, switch one byte after
        mov     word ptr es:[bx-1],ax   ; else plot that word
        push    cx                      ; hand it over to the switch
        inc     bx                      ; bank-end?
        jz      badbank2                ; yes, switch it
        mov     es:[bx],cl              ; else write the third byte
        pop     cx                      ; wasn't needed - no switching
        jmp     short wedone
badbank1:
        dec     bx                      ; bx=0ffffh
        mov     es:[bx],al              ; plot the first byte
        xchg    ah,al                   ; second color for badbank2
        push    ax                      ; hand it over
badbank2:
        inc     dx                      ; next bank needed
        mov     ax,dx                   ; newbank expects bank in ax
        call    far ptr newbank
        pop     ax                      ; get next color
        inc     bx                      ; badbank1 or badbank2?
        mov     es:[0],al               ; plot that color
        jnz     wedone                  ; badbank2 - nothing more to do
        mov     es:[1],cl               ; badbank1 - plot the last byte
wedone:
        ret                             ; we done.
puttruecolor    endp

; Fastcall version, called when C programs are compiled by MSC 6.00A:

@putcolor_a     proc    FORTRAN ; ax=xdot, dx=ydot, bx=color
        push    si                      ; preserve these
        push    di                      ;  ...
        mov     cx,ax                   ; load up the registers
        add     cx,sxoffs               ;  ...
        add     dx,syoffs               ;  ...
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ;  ...
        mov     ax,bx                   ; color
        and     ax,andcolor             ; ensure that 'color' is in range
        call    dotwrite                ; write the dot via the approved method
;;;     call    videocleanup            ; perform any video cleanup required
        pop     di                      ; restore
        pop     si                      ;  ...
        ret                             ; we done.
@putcolor_a     endp

; ***************Function out_line(pixels,linelen) *********************

;       This routine is a 'line' analog of 'putcolor_a()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder

out_line        proc    uses di si es, pixels:ptr byte, linelen:word
        mov     cx,sxoffs               ; start at left side of logical screen
        mov     dx,rowcount             ; sanity check: don't proceed
        add     dx,syoffs               ;  ...
        cmp     dx,sydots               ; beyond the end of the screen
        ja      out_lineret             ;  ...
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ; save it here during this routine
        mov     ax, linelen             ; last pixel column
        add     ax, sxoffs              ;  ...
        dec     ax                      ;  ...
        mov     si,pixels               ; get the color for dot 'x'
        call    linewrite               ; mode-specific linewrite routine
        inc     rowcount                ; next row
out_lineret:
        xor     ax,ax                   ; return 0
        ret

out_line        endp

; ***Function get_line(int row,int startcol,int stopcol, unsigned char *pixels) ***

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder

get_line        proc uses di si es, row:word, startcol:word, stopcol:word, pixels:ptr byte
        mov     cx,startcol             ; sanity check: don't proceed
        add     cx,sxoffs               ;  ...
        cmp     cx,sxdots               ; beyond the right end of the screen
        ja      get_lineret             ;  ...
        mov     dx,row                  ; sanity check: don't proceed
        add     dx,syoffs               ;  ...
        cmp     dx,sydots               ; beyond the bottom of the screen
        ja      get_lineret             ;  ...
        mov     ax, stopcol             ; last pixel to read
        add     ax, sxoffs              ;  ...
        mov     di, pixels              ; get the color for dot 'x'
        call    lineread                ; mode-specific lineread routine
get_lineret:
        xor     ax,ax                   ; return 0
        ret
get_line        endp

; ***Function put_line(int row,int startcol,int stopcol, unsigned char *pixels) ***

;       This routine is a 'line' analog of 'putcolor_a()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder

put_line        proc uses di si es, row:word, startcol:word, stopcol:word, pixels:ptr byte
        mov     cx,startcol             ; sanity check: don't proceed
        add     cx,sxoffs               ;  ...
        cmp     cx,sxdots               ; beyond the right end of the screen
        ja      put_lineret             ;  ...
        mov     dx,row                  ; sanity check: don't proceed
        add     dx,syoffs               ;  ...
        cmp     dx,sydots               ; beyond the bottom of the screen
        ja      put_lineret             ;  ...
        mov     ax,0a000h               ; EGA, VGA, MCGA starts here
        mov     es,ax                   ; save it here during this routine
        mov     ax, stopcol             ; last column
        add     ax, sxoffs;             ;  ...
        cmp     ax,sxdots               ; beyond the right end of the screen?
        ja      put_lineret             ;  ...
        mov     si,pixels               ; put the color for dot 'x'
        call    linewrite               ; mode-specific linewrite routine
put_lineret:
        xor     ax,ax                   ; return 0
        ret
put_line        endp


;-----------------------------------------------------------------

; setattr(row, col, attr, count) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         count = number of characters to set
;         This routine works only in real color text mode.

setattr         proc uses es di, row:word, col:word, attr:word, count:word
        mov     ax,row                  ; row specified?
        cmp     ax,-1                   ;  ...
        je      setac                   ;  nope, inherit it
        mov     textrow,ax              ; yup, store it
setac:  mov     ax,col                  ; col specified?
        cmp     ax,-1                   ;  ...
        je      setago                  ;  nope, inherit it
        mov     textcol,ax              ; yup, store it
setago: cmp     text_type,0             ; real color text mode?
        jne     setax                   ;  nope, do nothing
        mov     ax,textrow              ; starting row
        add     ax,textrbase            ;  ...
        mov     cx,160                  ; x 2 bytes/row (char & attr bytes)
        imul    cx
        mov     bx,textcol              ; add starting column
        add     bx,textcbase            ;  ...
        add     ax,bx                   ; twice since 2 bytes/char
        add     ax,bx                   ;  ...
        mov     di,ax                   ; di -> start location in Video segment
        mov     cx,count                ; number of bytes to set
        jcxz    setax                   ; none?
        mov     ax,attr                 ; get color attributes in al
        mov     dx,0b800h               ; set video pointer
        cmp     mode7text,0             ; egamono/hgc?
        je      setalstore              ;  nope
        mov     dx,0b000h               ; mode 7 address
        mov     al,07h                  ; normal mda ttribute
        cmp     ah,0                    ; inverse?
        jge     setalchkbright          ;  nope
        mov     al,70h                  ; yup
        jmp     short setalstore
setalchkbright:
        test    ah,40h                  ; bright?
        jz      setalstore              ;  nope
        mov     al,0Fh                  ; yup
setalstore:
        mov     es,dx                   ;  ...
setalp: mov     byte ptr es:1[di],al    ; set attribute
        add     di,2                    ; for next one
        loop    setalp                  ; do next char
setax:  ret
setattr         endp


;-----------------------------------------------------------------

; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         string = far pointer to the null terminated string to print.
;    Written for the A86 assembler (which has much less 'red tape' than MASM)
;    by Bob Montgomery, Orlando, Fla.             7-11-88
;    Adapted for MASM 5.1 by Tim Wegner          12-11-89
;    Furthur mucked up to handle graphics
;       video modes by Bert Tyler                 1-07-90
;    Reworked for:  row,col update/inherit;
;       620x200x2 inverse video;  far ptr to string;
;       fix to avoid scrolling when last posn chgd;
;       divider removed;  newline ctl chars;  PB  9-25-90

putstring       proc uses es di si, row:word, col:word, attr:word, string:far ptr byte
        mov     ax,row                  ; row specified?
        cmp     ax,-1                   ;  ...
        je      putscol                 ;  nope, inherit it
        mov     textrow,ax              ; yup, store it
putscol:
        mov     ax,col                  ; col specified?
        cmp     ax,-1                   ;  ...
        je      putsmode                ;  nope, inherit it
        mov     textcol,ax              ; yup, store it
putsmode:
        les     si,string               ; load buffer pointer
        cmp     text_type,0             ; are we in color text mode?
        jne     short put_substring     ;  nope
        jmp     put_text                ; yup

put_substring:                          ; graphics mode substring loop
        push    si                      ; save start pointer
        push    textcol                 ; save start column
put_loop:
        mov     al,byte ptr es:[si]     ; get next char
        cmp     al,0                    ; end of string?
        je      puts_chk_invert         ;  yup
        cmp     al,10                   ; end of line?
        je      puts_chk_invert         ;  yup
        push    si                      ; save offset
        push    es                      ; save this
        push    bp                      ; and this
        push    ax                      ; and this last, needed soonest
        mov     ah,02h                  ; set up bios set cursor call
        xor     bh,bh                   ;  page 0
        mov     dl,byte ptr textcol     ;  screen location
        add     dl,byte ptr textcbase   ;   ...
        mov     dh,byte ptr textrow     ;   ...
        add     dh,byte ptr textrbase   ;   ...
        int     10h                     ;  invoke the bios
        pop     ax                      ; the character to write
        mov     ah,09h                  ; set up the bios write char call
        mov     bx,7                    ;  page zero, color
        mov     cx,1                    ;  write 1 character
        int     10h                     ;  invoke the bios
        pop     bp                      ; restore
        pop     es                      ; restore
        pop     si                      ; restore
        inc     si                      ; on to the next character
        inc     textcol                 ;  ...
        jmp     short put_loop          ;  ...
puts_chk_invert:
        pop     bx                      ; starting column number
        pop     dx                      ; restore start offset
        cmp     attr,0                  ; top bit of attribute on?
        jge     short puts_endsubstr    ;  nope, nothing more to do
        cmp     text_type,1             ; 640x200x2 mode?
        jne     short puts_endsubstr    ;  nope, can't do anything more
        mov     cx,si                   ; calc string length
        sub     cx,dx                   ;  ...
        jcxz    short puts_endsubstr    ; empty string?
        push    ax                      ; remember terminating char
        push    es                      ;  and this
        ; make the string inverse video on display
        mov     ax,320                  ; offset in vid mem of 1st byte's top
        mov     di,textrow              ;  row*320 + col
        add     di,textrbase            ;  ...
        mul     di                      ;  ...
        add     ax,bx                   ;  ...
        add     ax,textcbase            ;  ...
        mov     di,ax                   ;  ...
        mov     ax,0b800h               ; set es to vid memory
        mov     es,ax                   ;  ...
puts_invert:
        not     byte ptr es:0000h[di]   ; invert the 8x8 making up a character
        not     byte ptr es:2000h[di]
        not     byte ptr es:0050h[di]
        not     byte ptr es:2050h[di]
        not     byte ptr es:00a0h[di]
        not     byte ptr es:20a0h[di]
        not     byte ptr es:00f0h[di]
        not     byte ptr es:20f0h[di]
        inc     di
        loop    puts_invert             ; on to the next character
        pop     es                      ; restore
        pop     ax                      ;  ...
puts_endsubstr:
        cmp     al,0                    ; the very end?
        je      short putstring_ret     ; we done.
        inc     si                      ; go past the newline char
        mov     textcol,0               ; do newline
        inc     textrow                 ;  ...
        jmp     put_substring           ; on to the next piece

put_text:                               ; text mode substring loop
        mov     ax,textrow              ; starting row
        add     ax,textrbase            ;  ...
        mov     cx,160                  ; x 2 bytes/row (char & attr bytes)
        imul    cx
        mov     bx,textcol              ; add starting column
        add     bx,textcbase            ;  ...
        add     ax,bx                   ; twice since 2 bytes/char
        add     ax,bx                   ;  ...
        mov     di,ax                   ; di -> start location in Video segment
        mov     ax,attr                 ; get color attributes in ah
        cmp     mode7text,0             ; egamono/hgc? (MDA)
        je      B0                      ; nope, use color attr
        mov     al,07h                  ; default, white on blank
        cmp     ah,0                    ; top bit of attr set?
        jge     mdachkbright            ;  nope
        mov     al,70h                  ; inverse video
        jmp     short B0
mdachkbright:
        test    ah,40h                  ; 2nd bit of attr set?
        jz      B0                      ;  nope
        mov     al,0Fh                  ; bright
B0:     mov     ah,al
B1:     mov     al,byte ptr es:[si]     ; get a char in al
        cmp     al,0                    ; end of string?
        je      putstring_ret           ;  yes, done
        inc     si                      ; bump for next time
        cmp     al,10                   ; newline?
        jne     B2                      ;  nope
        mov     textcol,0               ; yup, do it
        inc     textrow                 ; ...
        jmp     short put_text          ; on to the next substring
B2:     push    es                      ; No, store char & attribute
        mov     dx,textaddr             ;  ...
        mov     es,dx                   ;  ...
        stosw                           ;  ...
        pop     es                      ;  ...
        inc     textcol                 ; update local var
        jmp     short B1                ; do next char

putstring_ret:
        ret
putstring endp


; ****************  EGA Palette <==> VGA DAC Conversion Routines **********

;       paltodac        converts a 16-palette EGA value to a 256-color VGA
;                       value (duplicated 16 times)
;       dactopal        converts the first 16 VGA values to a 16-palette
;                       EGA value

;       local routines called with register values
;               BH = VGA Red Color      xxRRRRRR
;               BL = VGA Green Color    xxGGGGGG
;               CH = VGA Blue Color     xxBBBBBB
;               CL = EGA Palette        xxrgbRGB
;
;       palettetodac    converts CL to BH/BL/CH
;       dactopalette    converte BH/BL/CH to CL

; *************************************************************************

palettetodac    proc    near
        mov     bx,0                    ; initialize RGB values to 0
        mov     ch,0                    ;  ...
        test    cl,20h                  ; low-red high?
        jz      palettetodac1           ;  nope
        or      bh,10h                  ; set it
palettetodac1:
        test    cl,10h                  ; low-green high?
        jz      palettetodac2           ;  nope
        or      bl,10h                  ; set it
palettetodac2:
        test    cl,08h                  ; low-blue high?
        jz      palettetodac3           ;  nope
        or      ch,10h                  ; set it
palettetodac3:
        test    cl,04h                  ; high-red high?
        jz      palettetodac4           ;  nope
        or      bh,20h                  ; set it
palettetodac4:
        test    cl,02h                  ; high-green high?
        jz      palettetodac5           ;  nope
        or      bl,20h                  ; set it
palettetodac5:
        test    cl,01h                  ; high-blue high?
        jz      palettetodac6           ;  nope
        or      ch,20h                  ; set it
palettetodac6:
        ret
palettetodac    endp

dactopalette    proc    near
        mov     cl,0                    ; initialize RGB values to 0
        test    bh,10h                  ; low-red high?
        jz      dactopalette1           ;  nope
        or      cl,20h                  ; set it
dactopalette1:
        test    bl,10h                  ; low-green high?
        jz      dactopalette2           ;  nope
        or      cl,10h                  ; set it
dactopalette2:
        test    ch,10h                  ; low-blue high?
        jz      dactopalette3           ;  nope
        or      cl,08h                  ; set it
dactopalette3:
        test    bh,20h                  ; high-red high?
        jz      dactopalette4           ;  nope
        or      cl,04h                  ; set it
dactopalette4:
        test    bl,20h                  ; high-green high?
        jz      dactopalette5           ;  nope
        or      cl,02h                  ; set it
dactopalette5:
        test    ch,20h                  ; high-blue high?
        jz      dactopalette6           ;  nope
        or      cl,01h                  ; set it
dactopalette6:
        ret
dactopalette    endp

paltodac        proc    uses es si di
        mov     si,0                    ; initialize the loop values
        mov     di,0
paltodacloop:
        mov     cl,palettega[si]        ; load up a single palette register
        call    palettetodac            ; convert it to VGA colors
        mov     dacbox+0[di],bh         ; save the red value
        mov     dacbox+1[di],bl         ;  and the green value
        mov     dacbox+2[di],ch         ;  and the blue value
        inc     si                      ; bump up the registers
        add     di,3                    ;  ...
        cmp     si,16                   ; more to go?
        jne     paltodacloop            ;  yup.
        push    ds                      ; set ES to DS temporarily
        pop     es                      ;  ...
        mov     ax,15                   ; do this 15 times to get to 256
        mov     di,offset dacbox+48     ; set up the first destination
paltodacloop2:
        mov     cx,24                   ; copy another block of 16 registers
        mov     si,offset dacbox        ; set up for the copy
        rep     movsw                   ;  do it
        dec     ax                      ; need to do another block?
        jnz     paltodacloop2           ;  yup.  do it.
        ret                             ;  we done.
paltodac        endp

dactopal        proc    uses es si di
        mov     si,0                    ; initialize the loop values
        mov     di,0
dactopalloop:
        mov     bh,dacbox+0[di]         ; load up the VGA red value
        mov     bl,dacbox+1[di]         ;  and the green value
        mov     ch,dacbox+2[di]         ;  and the blue value
        call    dactopalette            ; convert it to an EGA palette
        mov     palettega[si],cl        ; save as a single palette register
        inc     si                      ; bump up the registers
        add     di,3                    ;  ...
        cmp     si,16                   ; more to go?
        jne     dactopalloop            ;  yup.
        mov     cl,palettega            ; copy palette 0
        mov     palettega+16,cl         ;  to the overscan register
        ret                             ;  we done.
dactopal        endp


; *********************** Function loaddac() ****************************

;       Function to Load the dacbox[][] array, if it can
;       (sets gotrealdac to 0 if it can't, 1 if it can)

loaddac proc    uses es
        cmp     dotmode, 29             ; truecolor?  MCP 5-29-91
        jne     NotTPlusLoaddac
        mov     ax,4402h
        push    ax
        mov     ax,256 * 3
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET dacbox
        push    ax
        call    far ptr TPlusLUT
        add     sp, 10
        or      ax, ax
        jz      NotTPlusLoaddac         ; Didn't work, try a regular palette
        jmp     loaddacdone
NotTPlusLoaddac:
        cmp     dotmode,19              ; roll-your-own video mode?
        jne     loaddac_notyourown
        call    far ptr readvideopalette
        cmp     ax,-1                   ; palette-write handled yet?
        jne     go_loaddacdone          ;  yup.
loaddac_notyourown:
        mov     reallyega,0             ; set flag: not an EGA posing as a VGA
        cmp     dotmode,9               ; TARGA 3 June 89 j mclain
        je      go_loaddacdone
        cmp     f85flag, 0
        jne     go_loaddacdone

        cmp     xga_isinmode,0          ; XGA graphics mode?
        jne     go_loaddacdone

        cmp     istruecolor,0           ; truecolor graphics mode?
        jne     go_loaddacdone

        mov     dacbox,255              ; a flag value to detect invalid DAC
        cmp     debugflag,16            ; pretend we're not a VGA?
        je      loaddacdebug            ;  yup.
        push    ds                      ;  ...
        pop     es                      ;  ...
        mov     ax,1017h                ; get the old DAC values
        mov     bx,0                    ;  (assuming, of course, they exist)
        mov     cx,256                  ;  ...
        mov     dx,offset dacbox        ;  ...
        push    bp
        int     10h                     ; do it.
        pop     bp
loaddacdebug:
        cmp     dacbox,255              ; did it work?  do we have a VGA?
        je      loaddacega              ;  nope, go check ega
        cmp     colors,16               ; 16 color vga?
        jne     go_loaddacdone          ;  nope, all done
        cld                             ; yup, must straighten out dacbox,
        push    ds                      ;  16 color vga uses indirection thru
        pop     es                      ;  palette select
        mov     si,offset dacbox+60     ; dac[20] is used for color 6 so
        mov     di,offset dacbox+18     ;  copy dacbox[20] to dacbox[6]
        mov     cx,3                    ;  ...
        rep     movsb                   ;  ...
        mov     si,offset dacbox+168    ; dac[56-63] are used for colors 8-15 so
        mov     di,offset dacbox+24     ;  copy dacbox[56-63] to dacbox[8-15]
        mov     cx,24                   ;  ...
        rep     movsb                   ;  ...
go_loaddacdone:
        jmp     short loaddacdone
loaddacega:
        cmp     colors,16               ; are we using 16 or more colors?
        jb      loaddacdone             ;  nope.  forget it.
;;      cmp     sydots,350              ; 640x350 range?
        cmp     video_type,3            ; EGA or better?
        jb      loaddacdone             ;  nope.  forget it.
        mov     bx,offset palettega     ; make up a dummy palette
        mov     cx,3800h                ; start with color 0 == black
loaddacega1:                            ; and        color 8 == low-white
        mov     0[bx],cl                ; save one color
        mov     8[bx],ch                ; and another color
        inc     bx                      ; bump up the DAC
        add     cx,0101h                ; and the colors
        cmp     cl,8                    ; finished 8 colors?
        jne     loaddacega1             ;  nope.  get more.
        mov     reallyega,1             ; note that this is really an EGA
        call    far ptr paltodac        ; "convert" it to a VGA DAC
        mov     daclearn,1              ; bypass learn mode
        mov     ax,cyclelimit           ;  and spin as fast as he wants
        mov     daccount,ax             ;  ...
loaddacdone:
        cmp     colors,16               ; 16 color mode?
        jne     loaddacdone2            ;  nope
        cld                             ; yup, clear the excess dacbox
        mov     cx,360                  ;  entries to all zeros for editpal
        sub     ax,ax
        push    ds
        pop     es
        mov     di,offset dacbox+48
        rep     stosw
loaddacdone2:
        cld                             ; clear the top 8 entries in dacbox
        mov     cx,12                   ; bios doesn't reset them to 0's
        sub     ax,ax
        push    ds
        pop     es
        mov     di,offset dacbox+744
        rep     stosw
        cmp     tweakflag,0             ; tweaked mode?
        je      loaddacdone3            ;  nope
        mov     dx,3c4h                 ; alter sequencer registers
        mov     ax,0604h                ; disable chain 4
        out     dx,ax
loaddacdone3:
        mov     gotrealdac,1            ; flag for whether mode supports DAC
        cmp     dacbox,255              ; did DAC get loaded or fudged?
        jne     loaddacret              ;  yup
        mov     gotrealdac,0            ; nope
loaddacret:
        ret
loaddac endp

; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.

spindac proc    uses di si es, direction:word, rstep:word
        cmp     dotmode,9               ; TARGA 3 June 89 j mclain
        je      spinbailout
        cmp     dotmode,11              ; disk video mode?
        je      spinbailout
        cmp     istruecolor,1           ; truecolor mode:
        je      spinbailout
        cmp     gotrealdac,0            ; do we have DAC registers to spin?
        je      spinbailout             ;  nope.  bail out.
        cmp     colors,16               ; at least 16 colors?
        jge     spindacdoit             ;  yup.  spin away.
spinbailout:
        jmp     spindacreturn           ;  nope.  bail out.

spindacdoit:
        push    ds                      ; need ES == DS here
        pop     es                      ;  ...
        cmp     direction,0             ; just replace it?
        je      newDAC                  ;  yup.

        mov     cx, rstep               ; loop through the rotate "rstep" times
stepDAC:
        push    cx                      ; save the loop counter for a tad
        mov     si,offset dacbox
        mov     di,si
        mov     ax,rotate_lo            ; calc low end of rotate range
        cmp     ax,colors               ; safety check for 16 color mode
        jae     nextDAC                 ;  out of range, none to rotate
        add     si,ax
        add     si,ax
        add     si,ax
        mov     ax,rotate_hi            ; calc high end of rotate range
        cmp     ax,colors               ; safety check for 16 color mode
        jb      stepDAC2                ;  ok, in range
        mov     ax,colors               ; out of range, use colors-1
        dec     ax                      ;  ...
stepDAC2:
        add     di,ax
        add     di,ax
        add     di,ax
        mov     cx,di                   ; size of rotate range - 1
        sub     cx,si
        jcxz    nextDAC                 ; do nothing if range 0
        cmp     direction,1             ; rotate upwards?
        jne     short downDAC           ;  nope.  downwards
        mov     bx,word ptr [si]        ; save the first entry
        mov     dl,byte ptr [si+2]      ;  ...
        cld                             ; set the direction
        mov     di,si                   ; set up the rotate
        add     si,3                    ;  ...
        rep     movsb                   ; rotate it
        mov     word ptr [di],bx        ; store the last entry
        mov     byte ptr [di+2],dl      ;  ...
        jmp     short nextDAC           ; set the new DAC
downDAC:
        std                             ; set the direction
        mov     bx,word ptr [di]        ; save the last entry
        mov     dl,byte ptr [di+2]      ;  ...
        mov     si,di                   ; set up the rotate
        dec     si                      ;  ...
        add     di,2                    ;  ...
        rep     movsb                   ; rotate it
        mov     word ptr [si+1],bx      ; store the first entry
        mov     byte ptr [si+3],dl      ;  ...
        cld                             ; reset the direction
nextDAC:
        pop     cx                      ; restore the loop counter
        loop    stepDAC                 ; and loop until done.

newDAC:
        cmp     dotmode,19              ; roll-your-own video?
        jne     spin_notyourown         ; nope
        call    far ptr writevideopalette
        cmp     ax,-1                   ; negative result?
        je      go_spindoit             ; yup.  handle it locally.
        jmp     spindacreturn           ; else we done.
go_spindoit:
        jmp     spindoit

spin_notyourown:
        cmp     dotmode, 29
        je      TPlusOrXGAspindac
        cmp     xga_isinmode,0          ; XGA extended graphics?
        je      notxga

TPlusOrXGAspindac:
        mov     si,offset dacbox
        push    si
        mov     bx,0
xgalp1: mov     al,[si+bx]              ; adjust VGA -> XGA
        shl     al,1
        shl     al,1
        mov     [si+bx],al
        inc     bx
        cmp     bx,768
        jne     xgalp1

        cmp     dotmode, 29             ; Are we a TARGA+?
        jne     XGAPaletteCall          ;  nope - XGA
        mov     ax,4403h
        push    ax
        mov     ax,256 * 3
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET dacbox
        push    ax
        call    far ptr TPlusLUT
        add     sp, 10
        jmp     TPlusOrXGASet

XGAPaletteCall:
        call    far ptr xga_setpalette

TPlusOrXGASet:
        pop     si
        mov     bx,0
xgalp2: mov     al,[si+bx]              ; adjust XGA -> VGA
        shr     al,1
        shr     al,1
        mov     [si+bx],al
        inc     bx
        cmp     bx,768
        jne     xgalp2
        jmp     spindacreturn
notxga:

        cmp     bios_palette,0          ; BIOS palette updates forced?
        je      not_bios_palette        ;  nope
        mov     ax,1012h                ; use a BIOS update
        mov     bx,0
        mov     cx,256
        push    ds
        pop     es
        mov     dx,offset dacbox
        int     10h
        jmp     spindacreturn
not_bios_palette:

        cmp     f85flag, 0              ; if 8514a then update pallette
        je      spindoit
        jmp     spin8514

spindoit:
        cmp     colors,16               ; 16 color vga?
        jne     spindoit2               ;  nope
        cmp     reallyega,1             ; is this really an EGA?
        je      spindoit2               ;  yup
        cld                             ; vga 16 color, straighten out dacbox,
        push    ds                      ;  16 color vga uses indirection thru
        pop     es                      ;  palette select
        mov     si,offset dacbox+18     ; dac[20] is used for color 6 so
        mov     di,offset dacbox+60     ;  copy dacbox[6] to dacbox[20]
        mov     cx,3                    ;  ...
        rep     movsb                   ;  ...
        mov     si,offset dacbox+24     ; dac[56-63] are used for colors 8-15 so
        mov     di,offset dacbox+168    ;  copy dacbox[8-15] to dacbox[56-63]
        mov     cx,24                   ;  ...
        rep     movsb                   ;  ...
spindoit2:
        mov     bx,0                    ;  set up to update the DAC
        mov     dacnorm,0               ;  indicate no overflow
dacupdate:
        cmp     direction,0             ; just replace it?
        je      fastupdate              ;  yup.
        mov     cx,daccount             ;  ...
        jmp     short overfast
fastupdate:
        mov     cx,256
overfast:
        mov     ax,256                  ; calculate 256 - BX
        sub     ax,bx                   ;  ...
        cmp     ax,cx                   ; is that less than the update count?
        jge     retrace1                ;  nope.  no adjustment
        mov     cx,ax                   ;  else adjust
        mov     dacnorm,1               ; and indicate overflow
retrace1:
        mov     dx,03dah                ; wait for no retrace
        in      al,dx                   ;  ...
        and     al,8                    ; this bit is high during a retrace
        jnz     retrace1                ;  so loop until it goes low
retrace2:
        in      al,dx                   ; wait for no retrace
        and     al,8                    ; this bit is high during a retrace
        jz      retrace2                ;  so loop until it goes high
        cmp     reallyega,1             ; is this really an EGA?
        je      spinega                 ;  yup.  spin it that way.
        cmp     cpu,88                  ; are we on a (yuck, ugh) 8088/8086?
        jle     spinbios                ;  yup. go through the BIOS
.186
        mov     dx,03c8h                ; set up for a blitz-write
        mov     ax,bx                   ; from this register
        cli                             ; critical section:  no ints
        out     dx,al                   ; starting register
        inc     dx                      ; set up to update colors
        mov     si, offset dacbox       ; get starting addr in SI
        add     si,bx                   ;  ...
        add     si,bx                   ;  ...
        add     si,bx                   ;  ...
        mov     ax,cx                   ; triple the value in CX
        add     cx,ax                   ;  ...
        add     cx,ax                   ;  ...
        rep     outsb                   ; whap!  Zango!  They're updated!
        sti                             ; end of critical section
        mov     cx,ax                   ; restore CX for code below
        jmp     spindone                ; skip over the BIOS version.
.8086
spinbios:
        mov     dx,offset dacbox        ; set up the DAC box offset
        add     dx,bx                   ;  ...
        add     dx,bx                   ;  ...
        add     dx,bx                   ;  ...
        push    bp                      ;  save some registers
        push    cx                      ;  (AMSTRAD might need this)
        push    dx                      ;  ...
        push    bx                      ;  ...
        mov     ax,1012h                ; update the DAC
        int     10h                     ; do it.
        pop     bx                      ; restore the registers
        pop     dx                      ;  ...
        pop     cx                      ;  ...
        pop     bp                      ;  ...
        jmp     spindone                ; jump to common code
spinega:
        cmp     bx,0                    ; skip this if not the first time thru
        jne     spindone                ;  ...
        push    bx                      ; save some registers
        push    cx                      ;  aroud the call
        call    far ptr dactopal        ; convert the VGA DAC to an EGA palette
        pop     cx                      ; restore the registers
        pop     bx                      ;  from prior to the call
        mov     ax,1002h                ; update the EGA palette
        mov     dx,offset palettega     ;  ...
        int     10h                     ; do it.
spindone:
        cmp     daclearn,0              ; are we still in learn mode?
        jne     nolearn                 ;  nope.
        mov     dx,03dah                ; check for the retrace
        in      al,dx                   ;  ...
        and     al,1                    ; this bit is high if display disabled
        jz      donelearn               ;  oops.  retrace finished first.
        cmp     dacnorm,0               ; was this a "short" update?
        jne     short nolearn           ;  then don't increment it
        inc     daccount                ; increment the daccount value
        inc     daccount                ; increment the daccount value
        inc     daccount                ; increment the daccount value
        mov     ax,cyclelimit           ; collect the cycle-limit value
        cmp     daccount,ax             ; sanity check: don't update too far
        jle     short nolearn           ;  proceed if reasonable.
donelearn:
        sub     daccount,6              ; done learning: reduce the daccount
        mov     daclearn,1              ; set flag: no more learning
        cmp     daccount,4              ; there's a limit to how slow we go
        jge     nolearn                 ;  ...
        mov     daccount,4              ;  ...
nolearn:
        add     bx,cx                   ; set up for the next batch
        cmp     bx,256                  ; more to go?
        jge     spindacreturn           ;  nope.  we done.
        jmp     dacupdate               ;  yup.  do it.

spin8514:
        cmp     ai_8514, 0              ;check afi flag JCO 4/11/92
        jne     spin85afi
        call    far ptr w8514hwpal              ; AW
        jmp     spindacreturn
spin85afi:
        call    far ptr w8514pal                ;use afi

spindacreturn:
        ret
spindac endp

; *************** Function find_special_colors ********************

;       Find the darkest and brightest colors in palette, and a medium
;       color which is reasonably bright and reasonably grey.

find_special_colors proc uses si
        mov     color_dark,0            ; for default cases
        mov     color_medium,7          ;  ...
        mov     color_bright,15         ;  ...
        cmp     colors,2                ; 2 color mode?
        jg      fscnot2
        mov     color_medium,1          ; yup, set assumed values and return
        mov     color_bright,1
        ret
fscnot2:
        cmp     colors,16               ; < 16 color mode? (ie probably 4)
        jge     fscnot4
        mov     color_medium,2          ; yup, set assumed values and return
        mov     color_bright,3
        ret
fscnot4:
        cmp     gotrealdac,0            ; dac valid?
        je      fscret                  ; nope, return with defaults set earlier
        mov     bh,255                  ; bh is lowest brightness found yet
        sub     bl,bl                   ; bl is highest found yet
        sub     ah,ah                   ; ah is best found for medium choice yet
        mov     si,offset dacbox        ; use si as pointer to dac
        sub     cx,cx                   ; use cx for color number
fscloop:
        mov     al,byte ptr 0[si]       ; add red,green,blue (assumed all <= 63)
        add     al,byte ptr 1[si]       ;  ...
        add     al,byte ptr 2[si]       ;  ...
        cmp     al,bh                   ; less than lowest found so far?
        jae     fscchkbright
        mov     color_dark,cx           ; yup, note new darkest
        mov     bh,al                   ;  ...
fscchkbright:
        cmp     al,bl                   ; > highest found so far?
        jbe     fscchkmedium
        mov     color_bright,cx         ; yup, note new brightest
        mov     bl,al                   ;  ...
fscchkmedium:
        cmp     al,150                  ; too bright?
        jae     fscnextcolor            ; yup, don't check for medium
        add     al,80                   ; so the subtract below will be safe
        cmp     al,ah                   ; already less than best found?
        jbe     fscnextcolor
        mov     dh,byte ptr 0[si]       ; penalize by (maxgun-mingun)/2
        mov     dl,byte ptr 1[si]
        cmp     dh,dl                   ; set dh to max gun
        jae     fscmed1
        xchg    dh,dl                   ; now dh=max(0,1), dl=min(0,1)
fscmed1:
        cmp     dh,byte ptr 2[si]       ; 2 > dh?
        jae     fscmed2
        mov     dh,byte ptr 2[si]
fscmed2:
        cmp     dl,byte ptr 2[si]       ; 2 < dl?
        jbe     fscmed3
        mov     dl,byte ptr 2[si]
fscmed3:
        sub     dh,dl                   ; now subtract the penalty
        shr     dh,1
        sub     al,dh
        cmp     al,ah                   ; a new best?
        jbe     fscnextcolor
        mov     color_medium,cx         ; yup, note new medium
        mov     ah,al                   ;  ...
fscnextcolor:
        add     si,3                    ; point to next dac entry
        inc     cx                      ; next color number
        cmp     cx,colors               ; scanned them all?
        jl      fscloop                 ; nope, go around again
        cmp     ah,0                    ; find any medium color?
        jne     fscret                  ; yup, all done
        mov     ax,color_bright         ; must be a pretty bright image,
        mov     color_medium,ax         ; use the brightest for medium
fscret:
        ret
find_special_colors endp


; *************** Functions get_a_char, put_a_char ********************

;       Get and put character and attribute at cursor
;       Hi nybble=character, low nybble attribute. Text mode only

get_a_char proc
        mov     ah,8
        xor     bh,bh
        int     10h
        ret
get_a_char endp

put_a_char proc character:word
        mov     ax,character
        mov     bl,ah
        mov     ah,9
        xor     bh,bh
        mov     cx,1
        int     10h
        ret
put_a_char endp

        end

