#pragma once

#include <string>
#include "drivers.h"
#include "Externals.h"

class IFullScreenChooserApp
{
public:
	virtual ~IFullScreenChooserApp() { }
	virtual int put_string_center(int row, int col, int width, int attr, const char *msg) = 0;
	virtual void help_title() = 0;
	virtual bool ends_with_slash(const char *text) = 0;
};

class AbstractFullScreenChooser
{
public:
	AbstractFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
		int numChoices, char **choices, const int *attributes,
		int boxWidth, int boxDepth, int columnWidth, int current,
		void (*formatItem)(int, char*), char *speedString,
		int (*speedPrompt)(int, int, int, char *, int), int (*checkKey)(int, int),
		IFullScreenChooserApp &app,
		Externals &externs,
		AbstractDriver *driver);
	virtual ~AbstractFullScreenChooser() {}

	int Execute();

private:
	bool InitializeCurrent();
	void ComputeBoxDepthAndWidth(int requiredRows);
	int GetRequiredRows(int scrunch);
	void FindWidestColumn();
	void CountTitleLinesAndWidth();
	void Footer(int &i);

	int prompt_color(int attributes);
	void show_speed_string(int speedrow);
	bool is_a_dir_name(const char *name);
	void process_speed_string(int curkey, bool is_unsorted);

	int _options;
	const char *_heading;
	const char *_heading2;
	const char *_instructions;
	int _numChoices;
	char **_choices;
	const int *_attributes;
	int _boxWidth;
	int _boxDepth;
	int _columnWidth;
	int _current;
	void (*_formatItem)(int, char*);
	char *_speedString;
	int (*_speedPrompt)(int, int, int, char *, int);
	int (*_checkKeystroke)(int, int);
	int _titleLines;
	int _titleWidth;
	IFullScreenChooserApp &_app;
	Externals &_externs;
	AbstractDriver *_driver;
};

extern int full_screen_choice(int options,
	const std::string &heading, const std::string &heading2,
	const std::string &instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int item, char *text),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int));
extern int full_screen_choice(int options,
	const char *heading, const char *heading2, const char *instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char *),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int));
extern int full_screen_choice_help(int help_mode, int options,
	const char *heading, const char *heading2, const char *instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char *),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int));
