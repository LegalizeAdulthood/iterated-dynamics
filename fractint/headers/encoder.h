#if !defined(ENCODER_H)
#define ENCODER_H

extern int save_to_disk(char *filename);
extern int save_to_disk(std::string &filename);
extern int encoder();
extern int _fastcall new_to_old(int new_fractype);

#endif
