#pragma once
#if !defined(DECODER_H)
#define DECODER_H

extern int                 (*g_out_line)(BYTE *, int);

extern short decoder(short);
extern void set_byte_buff(BYTE *ptr);

#endif
