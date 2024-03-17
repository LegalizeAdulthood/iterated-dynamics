#pragma once

#include "cmdfiles.h"

extern bool                  g_busy;

int slideshw();
slides_mode startslideshow();
void stopslideshow();
void recordshw(int);
int handle_special_keys(int ch);
