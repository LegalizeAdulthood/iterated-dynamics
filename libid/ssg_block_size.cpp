// SPDX-License-Identifier: GPL-3.0-only
//
#include "ssg_block_size.h"

#include "id_data.h"

int ssg_block_size() // used by solidguessing and by zoom panning
{
    // blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200
    int block_size = 4;
    int i = 300;
    while (i <= g_logical_screen_y_dots)
    {
        block_size += block_size;
        i += i;
    }
    // increase blocksize if prefix array not big enough
    while (block_size*(MAX_X_BLK-2) < g_logical_screen_x_dots || block_size*(MAX_Y_BLK-2)*16 < g_logical_screen_y_dots)
    {
        block_size += block_size;
    }
    return block_size;
}
