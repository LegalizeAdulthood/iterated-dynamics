#if !defined(MISC_OVL_H)
#define MISC_OVL_H

extern void make_batch_file();
extern void edit_text_colors();
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int get_precision_dbl(int);
extern int get_precision_bf(int);
extern int get_precision_mag_bf();
extern void parse_comments(char *value);
extern void init_comments();
extern void write_batch_parms(const char *, int, int, int, int);
extern void expand_comments(char *, char *);

#endif
