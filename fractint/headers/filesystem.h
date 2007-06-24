#if !defined(FILESYSTEM_H)
#define FILESYSTEM_H

extern int merge_path_names(char *, char *, int);
extern void fix_dir_name(char *dirname);
extern int fr_find_first(char *path);
extern int fr_find_next();
extern int make_path(char *file_template, const char *drive, const char *dir, const char *fname, const char *ext);
extern int split_path(const char *file_template, char *drive, char *dir, char *fname, char *ext);
extern bool is_a_directory(char *s);
extern int get_a_filename(char *, char *, char *);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(const char *, const char *, const char *);
extern void extract_filename(char *target, const char *source);
extern char *has_extension(char *source);
extern void findpath(const char *, char *);
extern int check_write_file(char *filename, const char *ext);
extern void update_save_name(char *);

#endif
