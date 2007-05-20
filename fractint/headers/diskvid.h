#if !defined(DISK_VID_H)
#define DISK_VID_H

extern int disk_start_potential();
extern int disk_start_targa(FILE *, int);
extern int disk_start();
extern void disk_end();
extern int disk_read(int, int);
extern void disk_write(int, int, int);
extern void disk_read_targa(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void disk_write_targa(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void disk_video_status(int, char *);
extern int  _fastcall disk_start_common(long, long, int);
extern int disk_from_memory(long, int, void *);
extern int disk_to_memory(long, int, void *);

#endif
