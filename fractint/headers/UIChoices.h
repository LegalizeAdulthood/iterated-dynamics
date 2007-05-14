#if !defined(UICHOICES_H)
#define UICHOICES_H

#if defined(max)
#undef max
#endif

#if defined(min)
#undef min
#endif

#include <vector>

struct full_screen_values;

class UIChoices
{
public:
	UIChoices(int help_mode, const char *heading, int key_mask, char *extra_info = NULL);
	~UIChoices();

	void push(const char *label, const char *values[], int num_values, int existing);
	void push(const char *label, bool value);
	void push(const char *label, int value);
	void push(const char *label, long value);
	void push(const char *label);
	void push(const char *label, const char *value);
	void push(const char *label, float value);
	void push(const char *label, double value);

	int prompt();

	const full_screen_values &values(int i)	const
	{
		return m_values[i];
	}

private:
	int m_help_mode;
	const char *m_heading;
	int m_key_mask;
	char *m_extra_info;

	std::vector<const char *> m_choices;
	std::vector<full_screen_values> m_values;
};

#endif
