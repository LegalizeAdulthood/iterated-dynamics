/* TPLUS.C, (C) 1991 The Yankee Programmer
   All Rights Reserved.

   This code may be distributed only when bundled with the Fractint
   source code.

   Mark C. Peterson
   The Yankee Programmer
   405-C Queen Street, Suite #181
   Southington, CT 06489
   (203) 276-9721

*/

#include <conio.h>
#include <string.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "tplus.h"

struct TPWrite far WriteOffsets = {
      0,       1,       2,       3,       0x400,   0x401,      0x402,
      0x403,   0x800,   0x801,   0x802,   0x803,   0xc00,      0xc01,
      0xc02,   0xc03
};

struct TPRead far ReadOffsets = {
      0,                2,       3,       0x400,   0x401,      0x402,
      0x403,   0x800,   0x801,   0x802,   0x803,   0xc00,      0xc01,
      0xc02,   0xc03
};

struct _BOARD far TPlus;
int TPlusErr = 0;

void WriteTPWord(unsigned Register, unsigned Number) {
   OUTPORTB(TPlus.Write.INDIRECT, Register);
   OUTPORTW(TPlus.Write.WBL, Number);
}

void WriteTPByte(unsigned Register, unsigned Number) {
   OUTPORTB(TPlus.Write.INDIRECT, Register);
   OUTPORTB(TPlus.Write.WBL, Number);
}

unsigned ReadTPWord(unsigned Register) {
   OUTPORTB(TPlus.Write.INDIRECT, Register);
   return(INPORTW(TPlus.Read.RBL));
}

BYTE ReadTPByte(unsigned Register) {
   OUTPORTB(TPlus.Write.INDIRECT, Register);
   return((BYTE)INPORTB(TPlus.Read.RBL));
}

void DisableMemory(void) {
   unsigned Mode1;

   Mode1 = INPORTB(TPlus.Read.MODE1);
   Mode1 &= 0xfe;
   OUTPORTB(TPlus.Write.MODE1, Mode1);
}

void EnableMemory(void) {
   unsigned Mode1;

   Mode1 = INPORTB(TPlus.Read.MODE1);
   Mode1 |= 1;
   OUTPORTB(TPlus.Write.MODE1, Mode1);
}

struct TPLUS_IO {
   unsigned Cmd;
   int Initx, Finalx, Inity, Finaly, Destx, Desty;
   unsigned long Color;
   unsigned RegsOffset, RegsSegment, RegListOffset, RegListSegment,
            BoardNumber, StructSize;
} far TPlusIO;

/* TARGAP.SYS Commands */
#define READALL    0
#define WRITEALL   0
#define NUMBOARDS  3
#define FILLBLOCK  4
#define GRABFIELD  4
#define RESET      5
#define GRABFRAME  5
#define WAITFORVB  6
#define SETBOARD   8
#define IOBASE     9

/* DOS IO Commands */
#define DOS_READ   0x4402
#define DOS_WRITE  0x4403

int hTPlus = -1;
unsigned NumTPlus = 0;

int TargapSys(int Command, unsigned DOS) {
   struct TPLUS_IO far *IOPtr;
   unsigned far *RegPtr;
   union REGS r;
   struct SREGS s;

   if(hTPlus != -1) {
      r.x.ax = DOS;
      r.x.bx = hTPlus;
      r.x.cx = sizeof(TPlusIO);
      IOPtr = &TPlusIO;
      RegPtr = TPlus.Reg;
      r.x.dx = FP_OFF(IOPtr);
      s.ds = FP_SEG(IOPtr);
      TPlusIO.Cmd = Command;
      TPlusIO.StructSize = sizeof(TPlus.Reg);
      TPlusIO.RegsOffset = FP_OFF(RegPtr);
      TPlusIO.RegsSegment = FP_SEG(RegPtr);
      intdosx(&r, &r, &s);
      return(!r.x.cflag);
   }
   return(0);
}

int _SetBoard(int BoardNumber) {
   TPlusIO.BoardNumber = BoardNumber;
   return(TargapSys(SETBOARD, DOS_WRITE));
}

int TPlusLUT(BYTE far *LUTData, unsigned Index, unsigned Number,
             unsigned DosFlag)
{
   struct TPLUS_IO far *IOPtr;
   union REGS r;
   struct SREGS s;

   if(hTPlus != -1) {
      r.x.ax = DosFlag;
      r.x.bx = hTPlus;
      r.x.cx = sizeof(TPlusIO);
      IOPtr = &TPlusIO;
      r.x.dx = FP_OFF(IOPtr);
      s.ds = FP_SEG(IOPtr);
      TPlusIO.Cmd = 9;
      TPlusIO.StructSize = sizeof(TPlus.Reg);
      TPlusIO.RegsOffset = FP_OFF(LUTData);
      TPlusIO.RegsSegment = FP_SEG(LUTData);
      TPlusIO.BoardNumber = Number;
      TPlusIO.RegListOffset = Index;
      intdosx(&r, &r, &s);
      return(!r.x.cflag);
   }
   return(0);
}

int SetVGA_LUT(void) {
   char PathName[FILE_MAX_PATH];
   FILE *Data = NULL;
   BYTE LUTData[256 * 3];

   findpath("tplus.dat", PathName);
   if(PathName[0]) {
      if((Data = fopen(PathName, "rb")) != NULL) {
         if(!fseek(Data, 16L << 8, SEEK_SET)) {
            if(fread(LUTData, 1, sizeof(LUTData), Data) == sizeof(LUTData)) {
               fclose(Data);
               return(TPlusLUT(LUTData, 0, sizeof(LUTData), DOS_WRITE));
            }
         }
      }
   }
   if(Data != NULL)
      fclose(Data);
   return(0);
}

int SetColorDepth(int Depth) {
   if(TPlus.Reg[HIRES] && Depth == 4)
      Depth = 2;
   switch(Depth) {
      case 1:
         if(TPlus.Reg[XDOTS] > 512) {
            TPlus.Reg[PERM] = 1;
            TPlus.Reg[BYCAP] = 3;
            TPlus.RowBytes = 10;
         }
         else {
            TPlus.Reg[PERM] = 0;
            TPlus.Reg[BYCAP] = 1;
            TPlus.RowBytes = 9;
         }
         TPlus.Reg[BUFFPORTSRC] = 0;
         TPlus.Reg[CM1] = 0;
         TPlus.Reg[CM2] = 0;
         TPlus.Reg[DEPTH] = 1;
         TPlus.Reg[LIVE8] = 1;
         TPlus.Reg[DISPMODE] = 0;
         TPlus.Reg[LIVEPORTSRC] = 1;
         TPlus.Reg[LUTBYPASS] = 0;
         break;
      case 2:
         if(TPlus.Reg[XDOTS] > 512) {
            TPlus.Reg[PERM] = 3;
            TPlus.Reg[BYCAP] = 15;
            TPlus.Reg[CM2] = 1;
            TPlus.RowBytes = 11;
         }
         else {
            TPlus.Reg[PERM] = 1;
            TPlus.Reg[BYCAP] = 3;
            TPlus.Reg[CM2] = 0;
            TPlus.RowBytes = 10;
         }
         TPlus.Reg[BUFFPORTSRC] = 1;
         TPlus.Reg[CM1] = 0;
         TPlus.Reg[DEPTH] = 2;
         TPlus.Reg[LIVE8] = 0;
         TPlus.Reg[DISPMODE] = 0;
         TPlus.Reg[LIVEPORTSRC] = 1;
         TPlus.Reg[LUTBYPASS] = 1;
         break;
      case 3:
      case 4:
         TPlus.Reg[PERM] = (Depth == 3) ? 2 : 3;
         TPlus.Reg[BYCAP] = 0xf;
         TPlus.Reg[BUFFPORTSRC] = 3;
         TPlus.Reg[CM1] = 1;
         TPlus.Reg[CM2] = 1;
         TPlus.Reg[DEPTH] = 4;
         TPlus.Reg[LIVE8] = 0;
         TPlus.Reg[DISPMODE] = 0;
         TPlus.Reg[LIVEPORTSRC] = 1;
         TPlus.Reg[LUTBYPASS] = 1;
         TPlus.RowBytes = 11;
         break;
      default:
         return(0);
   }
   TPlus.Plot = WriteTPlusBankedPixel;
   TPlus.GetColor = ReadTPlusBankedPixel;
   TPlus.Reg[LIVEMIXSRC] = 0;
   TPlus.Reg[CM3] = 1;
   TPlus.RowsPerBank = 16 - TPlus.RowBytes;
   if(TargapSys(WRITEALL, DOS_WRITE)) {
      if(Depth == 1)
         SetVGA_LUT();
      if(TPlus.ClearScreen)
         ClearTPlusScreen();
      TargapSys(READALL, DOS_READ);
      return(Depth);
   }
   return(0);
}

int SetBoard(int BoardNumber) {
   unsigned ioBase, n;
   unsigned long MemBase;

   if(TPlus.ThisBoard != -1)
      DisableMemory();
   if(!_SetBoard(BoardNumber))
      return(0);
   if(!TargapSys(READALL, DOS_READ))
      return(0);
   TPlus.VerPan       = TPlus.Reg[VPAN];
   TPlus.HorPan       = TPlus.Reg[HPAN];
   TPlus.Top          = TPlus.Reg[TOP];
   TPlus.Bottom       = TPlus.Reg[BOT];
   TPlus.Bank64k      = 0xffff;                 /* Force a bank switch */

   MemBase        = TPlus.Reg[MEM_BASE];
   MemBase += (TPlus.Reg[MEM_MAP] != 3) ? 8 : 0;
   TPlus.Screen = (BYTE far *)(MemBase << 28);

   if(!TargapSys(IOBASE, DOS_READ))
      return(0);
   ioBase = TPlusIO.BoardNumber;
   TPlus.Read = ReadOffsets;
   TPlus.Write = WriteOffsets;
   for(n = 0; n < sizeof(TPlus.Read) / sizeof(unsigned); n++)
      ((unsigned far *)&(TPlus.Read))[n] += ioBase;
   for(n = 0; n < sizeof(TPlus.Write) / sizeof(unsigned); n++)
      ((unsigned far *)&(TPlus.Write))[n] += ioBase;

   EnableMemory();
   return(1);
}

int ResetBoard(int BoardNumber) {
   int CurrBoard, Status = 0;

   CurrBoard = TPlus.ThisBoard;
   if(_SetBoard(BoardNumber))
      Status = TargapSys(RESET, DOS_WRITE);
   if(CurrBoard > 0)
      _SetBoard(CurrBoard);
   return(Status);
}

#include <fcntl.h>
#include <io.h>

int CheckForTPlus(void) {
   unsigned n;

   if((hTPlus = open("TARGPLUS", O_RDWR | O_BINARY )) != -1) {
      if(!TargapSys(NUMBOARDS, DOS_READ))
         return(0);
      NumTPlus = TPlusIO.BoardNumber;
      TPlus.ThisBoard = -1;
      TPlus.ClearScreen = 1;
      for(n = 0; n < NumTPlus; n++)
         if(!ResetBoard(n))
            return(0);
      if(SetBoard(0))
         return(1);
   }
   return(0);
}

int SetTPlusMode(int Mode, int NotIntFlag, int Depth, int Zoom) {
   unsigned n;
   char PathName[FILE_MAX_PATH];
   FILE *Data = NULL;
   unsigned NewRegs[128];

   findpath("tplus.dat", PathName);
   if(PathName[0]) {
      if((Data = fopen(PathName, "rb")) != NULL) {
         if(!fseek(Data, (long)Mode << 8, SEEK_SET)) {
            if(fread(NewRegs, 1, 256, Data) == 256) {
               fclose(Data);
               NewRegs[PE] = TPlus.Reg[PE];
               NewRegs[OVLE] = TPlus.Reg[OVLE];
               NewRegs[RGB] = TPlus.Reg[RGB];
               NewRegs[SVIDEO] = TPlus.Reg[SVIDEO];
               NewRegs[DAC567DATA] = TPlus.Reg[DAC567DATA];
               NewRegs[VGASRC] = TPlus.Reg[VGASRC];
               for(n = 0; n < 128; n++)
                  TPlus.Reg[n] = NewRegs[n];
               if(TPlus.Reg[VTOP + 1] == 0xffff)
                  TPlus.Reg[NOT_INT] = 0;
               else if(TPlus.Reg[VTOP] == 0xffff && !NotIntFlag) {
                  TPlusErr = 1;
                  return(0);
               }
               else
                  TPlus.Reg[NOT_INT] = NotIntFlag;
               TPlus.xdots = TPlus.Reg[XDOTS];
               TPlus.ydots = TPlus.Reg[YDOTS];
               if(Zoom) {
                  TPlus.Reg[ZOOM] = Zoom;
                  TPlus.xdots >>= Zoom;
                  TPlus.ydots >>= Zoom;
               }
               return(SetColorDepth(Depth));
            }
         }
      }
   }
   if(Data != NULL)
      fclose(Data);
   return(0);
}

int FillTPlusRegion(unsigned x, unsigned y, unsigned xdots, unsigned ydots,
               unsigned long Color) {
   int Status = 0;

   TPlusIO.Initx = x;
   TPlusIO.Inity = TPlus.Reg[YDOTS] - (y + ydots) - 2;
   TPlusIO.Finalx = x + xdots - 1;
   TPlusIO.Finaly = TPlus.Reg[YDOTS] - y - 1;
   TPlusIO.Color = Color;
   Status = TargapSys(FILLBLOCK, DOS_WRITE);
   EnableMemory();
   return(Status);
}

void BlankScreen(unsigned long Color) {
   unsigned BufferPort;

   OUTPORTW(TPlus.Write.COLOR0, ((unsigned*)&Color)[0]);
   OUTPORTB(TPlus.Write.COLOR2, ((unsigned*)&Color)[1]);
   OUTPORTB(TPlus.Write.COLOR3, 0xff);
   BufferPort = ReadTPByte(0xe9);
   BufferPort |= 3;
   WriteTPByte(0xe9, BufferPort);
}

void UnBlankScreen(void) {
   unsigned BufferPort;

   BufferPort = ReadTPByte(0xe9);
   BufferPort &= 0xfe;
   WriteTPByte(0xe9, BufferPort);
}

void EnableOverlayCapture(void) {
   unsigned Mode2;

   Mode2 = INPORTB(TPlus.Read.MODE2);
   Mode2 |= (1 << 6);
   Mode2 &= (0xff ^ (3 << 4));
   Mode2 |= (1 << 5);
   OUTPORTB(TPlus.Write.MODE2, Mode2);
}

void DisableOverlayCapture(void) {
   unsigned Mode2;

   Mode2 = INPORTB(TPlus.Read.MODE2);
   Mode2 &= (0xff ^ (7 << 4));
   OUTPORTB(TPlus.Write.MODE2, Mode2);
}

void ClearTPlusScreen(void) {
   BlankScreen(0L);
   EnableOverlayCapture();
   TargapSys(WAITFORVB, DOS_READ);
   TargapSys(WAITFORVB, DOS_READ);
   TargapSys(WAITFORVB, DOS_READ);
   DisableOverlayCapture();
   UnBlankScreen();
}

static struct {
   unsigned xdots, ydots, Template, Zoom, Depth;
} far ModeTable[] = {
   {512, 400, 0,  0, 4},
   {512, 476, 1,  0, 4},
   {512, 486, 2,  0, 4},
   {512, 576, 3,  0, 4},
   {640, 480, 4,  0, 2},
   {648, 486, 5,  0, 2},
   {720, 486, 6,  0, 2},
   {720, 576, 7,  0, 2},
   {756, 486, 8,  0, 2},
   {768, 576, 9,  0, 2},
   {800, 600, 10, 0, 2},
   {1024,768, 11, 0, 2},
   {640, 400, 13, 0, 2},
   {320, 200, 13, 1, 2},
   {512, 496, 14, 0, 4},
   {640, 496, 15, 0, 2}
};

static unsigned TableSize = (sizeof(ModeTable) / sizeof(ModeTable[0]));

int MatchTPlusMode(unsigned xdots, unsigned ydots, unsigned MaxColorRes,
                   unsigned PixelZoom, unsigned NonInterlaced) {
   unsigned n, Depth;

   for(n = 0; n < TableSize; n++) {
      if(ModeTable[n].xdots == xdots && ModeTable[n].ydots == ydots)
         break;
   }
   if(n < TableSize) {
      if(ModeTable[n].Zoom)
         PixelZoom += ModeTable[n].Zoom;
      if(PixelZoom > 4)
         PixelZoom = 4;
      switch(MaxColorRes) {
         case 24:
            Depth = 4;
            break;
         case 16:
            Depth = 2;
            break;
         case 8:
         default:
            Depth = 1;
            break;
      }
      if(ModeTable[n].Depth < Depth)
         Depth = ModeTable[n].Depth;
      return(SetTPlusMode(ModeTable[n].Template, NonInterlaced, Depth,
             PixelZoom));
   }
   return(0);
}

void TPlusZoom(int Zoom) {
   unsigned Mode2;

   Zoom &= 3;
   Mode2 = INPORTB(TPlus.Read.MODE2);
   Mode2 &= (0xff ^ (3 << 2));
   Mode2 |= (Zoom << 2);
   OUTPORTB(TPlus.Write.MODE2, Mode2);
}
