#pragma once
#if !defined(LINE3D_H)
#define LINE3D_H

extern int line3d(BYTE *, unsigned int);
extern int targa_color(int, int, int);
extern bool targa_validate(char const *File_Name);
bool startdisk1(char const *File_Name2, FILE *Source, bool overlay);

#endif
