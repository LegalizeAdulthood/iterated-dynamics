#if !defined(PROMPTS_2_H)
#define PROMPTS_2_H

/* speed key state values */
enum SpeedStateType
{
	SPEEDSTATE_MATCHING = 0,		/* string matches list - speed key mode */
	SPEEDSTATE_TEMPLATE = -2,		/* wild cards present - buiding template */
	SPEEDSTATE_SEARCH_PATH = -3		/* no match - building path search name */
};

extern const std::string GREY_MAP;
extern char *g_masks[2];
extern int g_speed_state;

extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern int get_browse_parameters();
extern int get_command_string();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int integer_unsupported();
extern int get_a_filename(const std::string &hdg, char *file_template, char *flname);
extern int get_a_filename(const std::string &heading, std::string &fileTemplate, std::string &filename);

#endif
