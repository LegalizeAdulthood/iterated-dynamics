#pragma once

// reads the current colormap into dacbox.
int readvideopalette();

// writes the current colormap from dacbox.
int writevideopalette();

// initializes the graphics window, colormap, etc.
void initUnixWindow();
