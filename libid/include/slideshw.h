#pragma once

#include <string>

enum class slides_mode
{
    OFF = 0,
    PLAY = 1,
    RECORD = 2
};

extern std::string           g_auto_name;
extern bool                  g_busy;
extern slides_mode           g_slides;

int slideshw();
slides_mode startslideshow();
void stopslideshow();
void recordshw(int);
int handle_special_keys(int ch);
