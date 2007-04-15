#ifndef WINPROT_H
#define WINPROT_H

/* This file contains prototypes for win specific functions. */

/*  calmanp5 -- assembler file prototypes */

extern long cdecl calculate_mandelbrot_fp_p5_asm(void);
extern void cdecl calculate_mandelbrot_fp_p5_asm_start(void);

/*  wgeneral -- assembler file prototypes */

extern long cdecl multiply(long, long, int);
extern long cdecl divide(long, long, int);

/*  dialog -- C file prototypes */

extern void PrintFile(void);
extern int Win_OpenFile(unsigned char *);
extern Win_SaveFile(unsigned char *);

/*  prompts1 -- C file prototypes */

extern void set_default_parms(void);

/*  windos -- C file prototypes */

extern void debugmessage(char *, char *);
extern int stop_message(int , char far *);
extern int  farread(int, VOIDPTR, unsigned);
extern int  farwrite(int, VOIDPTR, unsigned);
extern void far_memcpy(void far *, void far *, int);
extern void far_memset(void far *, int , int);
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void putcolor_a (int, int, int);
extern void spindac(int, int);
extern void buzzer (int);
extern int thinking(int, char far *);
extern void CalibrateDelay(void);
extern void start_wait(void);
extern void end_wait(void);

extern int get_video_mode(struct fractal_info *,struct ext_blk_formula_info *);
extern int check_vidmode_keyname(char *);
extern void video_mode_key_name(int, char *);
extern int check_video_mode_key(int, int);
extern void put_line(int, int, int, BYTE *);
extern void get_line(int, int, int, BYTE *);
extern void restoredac(void);
extern void reset_zoom_corners(void);
extern void flush_screen(void);

extern int win_load(void);
extern void win_save(void);
extern void win_cycle(void);

extern void winfract_help(void);

/*  windos2 -- C file prototypes */

extern void movecursor(int, int);
extern void setattr(int, int, int, int);
extern int  put_string_center(int, int, int, int, char far *);
extern void putstring(int, int, int, unsigned char far *);
extern int  strncasecmp(char far *,char far *,int);
extern int  input_field(int, int, char *, int, int, int, int (*)(int) );
extern void help_title(void);
extern void stackscreen(void);
extern void unstackscreen(void);
extern void discardscreen(void);
extern int load_palette(void);
extern void save_palette(void);
extern void fractint_help(void);
extern int getakeynohelp(void);
extern int win_make_batch_file(void);
extern int fractint_getkeypress(int);

extern int main_menu(int);

/*  winfract -- C file prototypes */

extern void win_set_title_text(void);
extern void win_savedac(void);

/*  winstubs -- C file prototypes */

extern void rotate(int);
extern void find_special_colors(void);
extern int show_temp_message(char far *);
extern void clear_temp_message(void);
extern void free_temp_message(void);
extern int disk_from_memory(long, int, void far *);
extern int disk_to_memory(long, int, void far *);
extern int _fastcall disk_start_common(long, long, int);
extern long cdecl normalize(char far *);
extern void zoom_box_draw(int);

extern void farmessage(unsigned char far *);
extern void setvideomode(int, int, int, int);
extern int fromvideotable(void);
extern void home(void);

extern int intro_overlay(void);
extern int rotate_overlay(void);
extern int printer_overlay(void);
extern int disk_start_potential(void);
extern int disk_start(void);
extern void disk_end(void);
extern void disk_write_targa(unsigned int,unsigned int,BYTE,BYTE,BYTE);
extern void disk_read_targa(unsigned int,unsigned int,BYTE *,BYTE *,BYTE *);
extern int set_color_palette_name(char *);
extern BYTE far *findfont(int);
extern long cdecl readticker(void);
extern void tga_end(void);

extern int key_count(int);

extern void display_box(void);
extern void clear_box(void);
extern void _fastcall add_box(struct coords);
extern void _fastcall draw_lines(struct coords, struct coords, int, int);
extern int show_vid_length(void);

extern int get_sound_params(void);
extern int soundon(int);
extern void soundoff(void);
extern int initfm(void);
extern void mute(void);

extern void disk_video_status(int, char far *);
extern int tovideotable(void);
extern void TranspPerPixel(void);
extern void stop_slide_show(void);
extern void aspect_ratio_crop(float, float);
extern void setvideotext(void);

/* added for Win32 port */
extern void gettruecolor(int, int, int*, int*, int*);
extern void puttruecolor(int, int, int, int, int);
extern void setnullvideo(void);
extern void delay(int);
extern void initasmvars(void);
extern void adapter_detect(void);

#endif
