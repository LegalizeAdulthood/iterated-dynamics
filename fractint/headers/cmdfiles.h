#if !defined(CMD_FILES_H)
#define CMD_FILES_H

extern int command_files(int, char **);
extern int load_commands(FILE *);
extern int get_curarg_len(char *curarg);
extern int get_max_curarg_len(char *floatvalstr[], int totparm);
extern int init_msg(const char *, char *, int);
extern int process_command(char *curarg, int mode);
extern int get_power_10(LDBL x);
extern void pause_error(int);
extern int bad_arg(const char *curarg);

#endif
