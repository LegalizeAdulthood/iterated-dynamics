#pragma once
#if !defined(SLIDESHW_H)
#define SLIDESHW_H

#include "cmdfiles.h"

extern bool                  g_busy;

extern int slideshw();
extern slides_mode startslideshow();
extern void stopslideshow();
extern void recordshw(int);
extern int handle_special_keys(int ch);

#endif
