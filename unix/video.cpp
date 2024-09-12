#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "id_data.h"
#include "rotate.h"
#include "spindac.h"
#include "text_screen.h"
#include "unixscr.h"
#include "video.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

extern int startdisk();

//WINDOW *curwin;

bool g_fake_lut = false;
int dacnorm = 0;
int ShadowColors;

int videoflag = 0;      // special "your-own-video" flag

int g_row_count = 0;        // row-counter for decoder and out_line
int video_type = 0;     /* actual video adapter type:
                   0  = type not yet determined
                   1  = Hercules
                   2  = CGA (assumed if nothing else)
                   3  = EGA
                   4  = MCGA
                   5  = VGA
                   6  = VESA (not yet checked)
                   11  = 8514/A (not yet checked)
                   12  = TIGA   (not yet checked)
                   13  = TARGA  (not yet checked)
                   100 = x monochrome
                   101 = x 256 colors
                 */
int svga_type = 0;      /*  (forced) SVGA type
                   1 = ahead "A" type
                   2 = ATI
                   3 = C&T
                   4 = Everex
                   5 = Genoa
                   6 = Ncr
                   7 = Oak-Tech
                   8 = Paradise
                   9 = Trident
                   10 = Tseng 3000
                   11 = Tseng 4000
                   12 = Video-7
                   13 = ahead "B" type
                   14 = "null" type (for testing only) */
int mode7text = 0;      // nonzero for egamono and hgc
int textaddr = 0xb800;      // b800 for mode 3, b000 for mode 7
int textsafe = 0;       /* 0 = default, runup chgs to 1
                   1 = yes
                   2 = no, use 640x200
                   3 = bios, yes plus use int 10h-1Ch
                   4 = save, save entire image */
int g_vesa_detect = 1;      // set to 0 to disable VESA-detection
int g_video_start_x = 0;
int g_video_start_y = 0;
int g_vesa_x_res;
int g_vesa_y_res;
int chkd_vvs = 0;
int video_vram = 0;

VIDEOINFO x11_video_table[] = {
    {999, 800, 600, 256, nullptr, "                         "},
};

void loaddac()
{
    readvideopalette();
}
