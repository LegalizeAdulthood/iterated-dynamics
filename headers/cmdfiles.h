#pragma once
#if !defined(CMDFILES_H)
#define CMDFILES_H

extern int cmdfiles(int argc, char const *const *argv);
extern int load_commands(FILE *);
extern void set_3d_defaults();
extern int get_curarg_len(char const *curarg);
extern int get_max_curarg_len(char const *floatvalstr[], int totparm);
extern int init_msg(char const *cmdstr, char const *badfilename, cmd_file mode);
extern int cmdarg(char *curarg, cmd_file mode);
extern int getpower10(LDBL x);
extern void dopause(int);

#endif
