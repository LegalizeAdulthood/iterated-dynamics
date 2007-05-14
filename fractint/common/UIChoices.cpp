#include <string.h>

#include <string>

#include "port.h"
#include "fractint.h"
#include "prototyp.h"
#include "fihelp.h"
#include "UIChoices.h"

UIChoices::UIChoices(const char *heading, int key_mask)
	: m_help_mode(-1),
	m_heading(heading),
	m_key_mask(key_mask),
	m_extra_info(NULL),
	m_choices(),
	m_values()
{
}

UIChoices::UIChoices(int help_mode, const char *heading, int key_mask, char *extra_info)
	: m_help_mode(help_mode),
	m_heading(heading),
	m_key_mask(key_mask),
	m_extra_info(extra_info),
	m_choices(),
	m_values()
{
}

UIChoices::~UIChoices()
{
}

void UIChoices::push(const char *label, const char *values[], int num_values, int existing)
{
	int widest_value = static_cast<int>(::strlen(values[0]));
	for (int i = 1; i < num_values; i++)
	{
		int len = static_cast<int>(::strlen(values[i]));
		if (len > widest_value)
		{
			widest_value = len;
		}
	}
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'l';
	fsv.uval.ch.vlen = widest_value;
	fsv.uval.ch.llen = num_values;
	fsv.uval.ch.list = values;
	fsv.uval.ch.val = existing;
	m_values.push_back(fsv);
}

void UIChoices::push(const char *label, bool value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'y';
	fsv.uval.ch.val = value ? 1 : 0;
	m_values.push_back(fsv);
}

void UIChoices::push(const char *label, int value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'i';
	fsv.uval.ival = value;
	m_values.push_back(fsv);
}

void UIChoices::push(const char *label, long value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'L';
	fsv.uval.Lval = value;
	m_values.push_back(fsv);
}

void UIChoices::push(const char *label)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = '*';
	m_values.push_back(fsv);
}

void UIChoices::push(const char *label, const char *value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 's';
	::strcpy(fsv.uval.sval, value);
}

void UIChoices::push(const char *label, float value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'f';
	fsv.uval.dval = value;
}

void UIChoices::push(const char *label, double value)
{
	m_choices.push_back(label);
	full_screen_values fsv;
	fsv.type = 'd';
	fsv.uval.dval = value;
}

int UIChoices::prompt()
{
	if (m_help_mode != -1)
	{
		return full_screen_prompt_help(m_help_mode, m_heading,
			static_cast<int>(m_choices.size()),
			&m_choices[0], &m_values[0], m_key_mask, m_extra_info);
	}
	else
	{
		return full_screen_prompt(m_heading, static_cast<int>(m_choices.size()),
			&m_choices[0], &m_values[0], m_key_mask, m_extra_info);
	}
}
