// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    MAX_Y_BLK = 7,  // MAX_X_BLK*MAX_Y_BLK*2 <= 4096, the size of "prefix"
    MAX_X_BLK = 202 // each maxnblk is oversize by 2 for a "border"
};

int ssg_blocksize();
