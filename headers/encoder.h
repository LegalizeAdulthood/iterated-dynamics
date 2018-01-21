#pragma once
#if !defined(ENCODER_H)

#include <string>

extern int savetodisk(char *filename);
extern int savetodisk(std::string &filename);
extern bool encoder();
extern int new_to_old(int new_fractype);

#endif
