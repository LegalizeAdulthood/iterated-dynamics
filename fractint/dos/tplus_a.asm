
IFDEF ??version   ; EAN
MASM51
QUIRKS
ENDIF

.model medium, c

; CAE removed this for TC users only 10 Oct 1998
IFNDEF ??version
.DATA
ENDIF

XDOTS        =       0
YDOTS        =       1  SHL 1
ASP_RATIO    =       2  SHL 1
DEPTH        =       4  SHL 1
TPAGE        =       5  SHL 1
BOTBANK      =       7  SHL 1
TOPBANK      =       8  SHL 1
ZOOM         =       10 SHL 1
DISPMODE     =       11 SHL 1
DAC567DATA   =       15 SHL 1
TOP          =       51 SHL 1
BOT          =       53 SHL 1
VPAN         =       55 SHL 1
NOT_INT      =       56 SHL 1
HIRES        =       61 SHL 1
PERM         =       64 SHL 1
BYCAP        =       68 SHL 1
PE           =       69 SHL 1
OVLE         =       70 SHL 1
HPAN         =       71 SHL 1
MEM_BASE     =       73 SHL 1
MEM_MAP      =       74 SHL 1
LIVEMIXSRC   =       91 SHL 1
RGB          =       93 SHL 1
SVIDEO       =       97 SHL 1
BUFFPORTSRC  =       98 SHL 1
CM1          =       101 SHL 1
CM2          =       102 SHL 1
LUTBYPASS    =       107 SHL 1
CM3          =       113 SHL 1
LIVEPORTSRC  =       115 SHL 1
LIVE8        =       116 SHL 1
VGASRC       =       123 SHL 1

BOARD    STRUC
   ThisBoard      dw    ?
   ClearScreen    dw    ?
   Screen         dd    ?
   VerPan         dw    ?
   HorPan         dw    ?
   Top            dw    ?
   Bottom         dw    ?
   xdots          dw    ?
   ydots          dw    ?
   Bank64k        dw    ?
   RowBytes       dw    ?
   RowsPerBank    dw    ?
   Reg            dw    128 DUP (?)
   Plot           dd    ?
   GetColor       dd    ?
      rVIDSTAT  dw    ?
      rCTL      dw    ?
      rMASKL    dw    ?
      rLBNK     dw    ?
      rREADAD   dw    ?
      rMODE1    dw    ?
      rOVSTRT   dw    ?
      rUSCAN    dw    ?
      rMASKH    dw    ?
      rOSCAN    dw    ?
      rHBNK     dw    ?
      rROWCNT   dw    ?
      rMODE2    dw    ?
      rRBL      dw    ?
      rRBH      dw    ?
      wCOLOR0   dw    ?
      wCOLOR1   dw    ?
      wCOLOR2   dw    ?
      wCOLOR3   dw    ?
      wVIDCON   dw    ?
      wINDIRECT dw    ?
      wHUESAT   dw    ?
      wOVSTRT   dw    ?
      wMASKL    dw    ?
      wMASKH    dw    ?
      wLBNK     dw    ?
      wHBNK     dw    ?
      wMODE1    dw    ?
      wMODE2    dw    ?
      wWBL      dw    ?
      wWBH      dw    ?
BOARD    ENDS

; CAE removed this for TC users only 10 Oct 1998
IFNDEF ??version
.FARDATA
ENDIF

extrn TPlus:WORD

.CODE

ReadTPlusBankedPixel      PROC     uses si di es ds, x:WORD, y:WORD
   mov   ax, SEG TPlus
   mov   ds, ax
   mov   di, OFFSET TPlus
   mov   bx, [di].Reg[YDOTS]
   dec   bx
   sub   bx, y
   mov   ax, [di].Reg[TPAGE]
   mov   cl, 9
   shl   ax, cl
   add   bx, ax
   mov   si, bx

   mov   cx, 16
   sub   cx, [di].RowBytes
   shr   si, cl
   cmp   si, [di].Bank64k
   je    CorrectBank

   mov   [di].Bank64k, si
   mov   ax, si
   shl   ax, 1
   mov   dx, [di].wLBNK
   mov   ah, al
   inc   ah
   out   dx, ax

CorrectBank:
   mov   cx, [di].RowsPerBank
   shl   si, cl
   sub   bx, si
   mov   cx, [di].RowBytes
   shl   bx, cl
   mov   ax, WORD PTR [[di].Screen+2]
   mov   es, ax
   mov   cx, [di].Reg[DEPTH]
   dec   cx
   jnz   CheckDepth2

   add   bx, x
   mov   al, es:[bx]
   xor   ah, ah
   jmp   ExitPlotBankedPixel

CheckDepth2:
   dec   cx
   jnz   Read4Bytes

   mov   cx, x
   shl   cx, 1
   add   bx, cx
   mov   ax, es:[bx]

   mov   dx, ax
   mov   cl, 10
   shr   dx, cl
   mov   cl, 3
   shl   dx, cl
   shl   ax, cl
   shl   ah, cl

   jmp   ExitPlotBankedPixel

Read4Bytes:
   mov   cx, x
   shl   cx, 1
   shl   cx, 1
   add   bx, cx
   mov   ax, es:[bx]
   mov   dx, es:[bx+2]

ExitPlotBankedPixel:
   ret
ReadTPlusBankedPixel      ENDP



WriteTPlusBankedPixel      PROC     uses si di es ds, x:WORD, y:WORD, Color:WORD
   mov   ax, SEG TPlus
   mov   ds, ax
   mov   di, OFFSET TPlus
   mov   bx, [di].Reg[YDOTS]
   dec   bx
   sub   bx, y
   mov   ax, [di].Reg[TPAGE]
   mov   cl, 9
   shl   ax, cl
   add   bx, ax
   mov   si, bx

   mov   cx, 16
   sub   cx, [di].RowBytes
   shr   si, cl
   cmp   si, [di].Bank64k
   je    CorrectBank

   mov   [di].Bank64k, si
   mov   ax, si
   shl   ax, 1
   mov   dx, [di].wLBNK
   mov   ah, al
   inc   ah
   out   dx, ax

CorrectBank:
   mov   cx, [di].RowsPerBank
   shl   si, cl
   sub   bx, si
   mov   cx, [di].RowBytes
   shl   bx, cl
   mov   ax, WORD PTR [[di].Screen+2]
   mov   es, ax
   mov   cx, [di].Reg[DEPTH]
   dec   cx
   jnz   CheckDepth2

   mov   ax, Color
   add   bx, x
   mov   es:[bx], al
   jmp   ExitPlotBankedPixel

CheckDepth2:
   dec   cx
   jnz   Write4Bytes

   mov   ax, Color
   mov   cl, 3
   shr   ah, cl
   shr   ax, cl
   mov   dx, Color+2
   shr   dx, cl
   mov   cx, 10
   shl   dx, cl
   or    ax, dx

   mov   cx, x
   shl   cx, 1
   add   bx, cx
   mov   es:[bx], ax
   jmp   ExitPlotBankedPixel

Write4Bytes:
   mov   ax, Color
   mov   cx, x
   shl   cx, 1
   shl   cx, 1
   add   bx, cx
   mov   es:[bx], ax
   mov   ax, Color+2
   mov   es:[bx+2], ax

ExitPlotBankedPixel:
   ret
WriteTPlusBankedPixel      ENDP



END
