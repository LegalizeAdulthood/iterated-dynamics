#if !defined(FILESYSTEM_H)
#define FILESYSTEM_H

extern int merge_path_names(char *old_full_path, char *new_filename, int mode);
extern int merge_path_names(std::string &old_full_path, char *new_filename, int mode);
extern void ensure_slash_on_directory(char *dirname);
extern int fr_find_first(char *path);
extern int fr_find_next();
extern void make_path(char *file_template, const char *drive, const char *dir, const char *fname, const char *ext);
extern void split_path(const char *file_template, char *drive, char *dir, char *filename, char *extension);
extern bool is_a_directory(char *s);
extern int get_a_filename(const char *hdg, char *file_template, char *flname);
extern int get_a_filename(const char *hdg, char *file_template, std::string &filename);
extern int dir_remove(const std::string &dir, const std::string &filename);
extern FILE *dir_fopen(const char *dir, const char *filename, const char *mode);
extern void extract_filename(char *target, const char *source);
extern const char *has_extension(const char *source);
extern const char *has_extension(const std::string &source);
extern void find_path(const char *filename, char *fullpathname);
extern void check_write_file(char *filename, const char *ext);
extern void update_save_name(char *filename);

#endif
