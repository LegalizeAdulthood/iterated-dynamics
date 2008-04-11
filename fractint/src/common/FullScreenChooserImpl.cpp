#include "Externals.h"
#include "FullScreenChooser.h"
#include "idhelp.h"
#include "miscres.h"
#include "realdos.h"

class FullScreenChooserApp : public IFullScreenChooserApp
{
public:
	virtual ~FullScreenChooserApp() { }

	virtual int put_string_center(int row, int col, int width, int attr, const char *msg)
	{ return ::put_string_center(row, col, width, attr, msg); }
	virtual void help_title()
	{ ::help_title(); }
	virtual bool ends_with_slash(const char *text)
	{ return ::ends_with_slash(text); }
};

class ProductionFullScreenChooser : public AbstractFullScreenChooser
{
public:
	ProductionFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
			int numChoices, char **choices, const int *attributes,
			int boxWidth, int boxDepth, int columnWidth, int current,
			void (*formatItem)(int, char*), char *speedString,
			int (*speedPrompt)(int, int, int, char *, int), int (*checkKeystroke)(int, int),
			IFullScreenChooserApp &app,
			Externals &externs,
			AbstractDriver *driver = DriverManager::current())
		: AbstractFullScreenChooser(options, heading, heading2, instructions,
			numChoices, choices, attributes,
			boxWidth, boxDepth, columnWidth, current,
			formatItem, speedString,
			speedPrompt, checkKeystroke, app, externs, driver)
	{
	}
	virtual ~ProductionFullScreenChooser()
	{
	}
};


/*
return is:
	n >= 0 for choice n selected,
	-1 for escape
	k for check_keystroke routine return value k (if not 0 nor -1)
	speedstring[0] != 0 on return if string is present
*/
int full_screen_choice(
	int options,
	const char *heading,				// heading info, \n delimited
	const char *heading2,				// column heading or 0
	const char *instructions,			// instructions, \n delimited, or 0
	int num_choices,					// How many choices in list
	char **choices,						// array of choice strings
	const int *attributes,				// &3: 0 normal color, 1, 3 highlight
										// &256 marks a dummy entry
	int box_width,						// box width, 0 for calc (in items)
	int box_depth,						// box depth, 0 for calc, 99 for max
	int column_width,					// data width of a column, 0 for calc
	int current,						// start with this item
	void (*format_item)(int, char*),	// routine to display an item or 0
	char *speed_string,					// returned speed key value, or 0
	int (*speed_prompt)(int, int, int, char *, int), // routine to display prompt or 0
	int (*check_keystroke)(int, int)			// routine to check keystroke or 0
	)
{
	FullScreenChooserApp app;
	return ProductionFullScreenChooser(options, heading, heading2, instructions,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_keystroke,
		app, g_externs, DriverManager::current()).Execute();
}

int full_screen_choice(int options,
	const std::string &heading, const std::string &heading2,
	const std::string &instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int item, char *text),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int))
{
	return full_screen_choice(options, heading.c_str(),
		heading2.length() ? heading2.c_str() : 0,
		instructions.length() ? instructions.c_str() : 0,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_keystroke);
}

int full_screen_choice_help(int help_mode, int options,
	const char *heading, const char *heading2, const char *instr,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char*),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int))
{
	int result;
	HelpModeSaver saved_help(help_mode);
	result = full_screen_choice(options, heading, heading2, instr,
		num_choices, choices, attributes, box_width, box_depth, column_width,
		current, format_item, speed_string, speed_prompt, check_keystroke);
	return result;
}
