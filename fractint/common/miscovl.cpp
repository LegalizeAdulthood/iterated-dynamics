/*
		Overlayed odds and ends that don't fit anywhere else.
*/
#include <algorithm>
#include <fstream>
#include <string>

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "fractype.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "FiniteAttractor.h"
#include "fractalp.h"
#include "framain2.h"
#include "FullScreenChooser.h"
#include "loadfile.h"
#include "MathUtil.h"
#include "miscovl.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"

#include "EscapeTime.h"
#include "Formula.h"
#include "SoundState.h"
#include "ThreeDimensionalState.h"
#include "UIChoices.h"
#include "ViewWindow.h"

/* routines in this module      */
using boost::format;

void write_batch_parms(const char *colorinf, bool colors_only, int maxcolor, int i, int j);
void expand_comments(char *target, const char *source);
void expand_comments(std::string &target, const char *source);
static void put_parm(const char *parm);
static void put_parm(const std::string &message);
static void put_parm(const format &message);
static void put_parm_line();
static int getprec(double, double, double);
int get_precision_bf(int rezflag);
static void put_float(int, double, int);
static void put_bf(int slash, bf_t r, int prec);
static void put_filename(const char *keyword, const char *fname);
static void put_filename(const char *keyword, const std::string &fname);
#ifndef XFRACT
static int check_modekey(int curkey, int choice);
#endif
static int entcompare(const void *p1, const void *p2);
static void strip_zeros(char *buf);

char par_comment[4][MAX_COMMENT];

static std::ofstream s_parameter_file;

static char par_key(int x)
{
	return (x < 10) ? ('0' + x) : ('a' - 10 + x);
}

static const char *truecolor_bits_text(int truecolorbits)
{
	static const char *bits_text[] = { "???", "32k", "64k", "16m", " 4g" };
	int index = ((truecolorbits < 1) || (truecolorbits > 4)) ? 0 : truecolorbits;
	return bits_text[index];
}

class MakeBatchFile
{
public:
	MakeBatchFile()		{}
	~MakeBatchFile()	{}

	void execute();

private:
	void execute_step1(std::ofstream &fpbat, int i, int j);
	void execute_step2();
	void execute_step3(int i, int j);
	bool check_resolution();
	bool check_bounds_xm_ym();
	bool check_video_mode();
	void fill_prompts(UIChoices &dialog);
	void initialize();
	void set_color_spec();
	void set_max_color();
	bool has_clut();
	std::string out_name() { return m_out_name; }

	enum
	{
		MAX_PROMPTS = 18
	};
	const char *m_choices[MAX_PROMPTS];
	full_screen_values m_param_values[18];
	int m_max_color;
	char m_color_spec[14];
	char m_in_parameter_command_file[80];
	char m_in_parameter_command_name[ITEMNAMELEN + 1];
	char m_in_parameter_command_comment[4][MAX_COMMENT];
	unsigned int m_pxdots;
	unsigned int m_pydots;
	int m_xm;
	int m_ym;
	char m_video_mode[5];
	bool m_colors_only;
	double m_pdelx;
	double m_pdely;
	double m_pdelx2;
	double m_pdely2;
	double m_pxxmin;
	double m_pyymax;
	bool m_have_3rd;
	char m_out_name[FILE_MAX_PATH + 1];
	int m_max_color_index;
	int m_prompts;
	int m_pieces_prompts;
};

bool MakeBatchFile::has_clut()
{
	return
#ifndef XFRACT
		(g_.RealDAC()) || (g_is_true_color && !g_true_mode_iterates);
#else
		(g_.RealDAC()) || (g_is_true_color && !g_true_mode_iterates) || g_fake_lut;
#endif
}

void make_batch_file()
{
	MakeBatchFile().execute();
}

void MakeBatchFile::set_max_color()
{
	--m_max_color;
	/* if (g_max_iteration < maxcolor)  remove 2 lines */
	/* maxcolor = g_max_iteration;   so that whole palette is always saved */
	if (g_inside > 0 && g_inside > m_max_color)
	{
		m_max_color = g_inside;
	}
	if (g_outside > 0 && g_outside > m_max_color)
	{
		m_max_color = g_outside;
	}
	if (g_distance_test < 0 && -g_distance_test > m_max_color)
	{
		m_max_color = int(-g_distance_test);
	}
	if (g_decomposition[0] > m_max_color)
	{
		m_max_color = g_decomposition[0] - 1;
	}
	if (g_potential_flag && g_potential_parameter[0] >= m_max_color)
	{
		m_max_color = int(g_potential_parameter[0]);
	}
	if (++m_max_color > 256)
	{
		m_max_color = 256;
	}
}

void MakeBatchFile::set_color_spec()
{
	std::string color_spec_name;
	if (g_.ColorState() == COLORSTATE_DEFAULT)
	{                         /* default colors */
		if (g_.MapDAC())
		{
			m_color_spec[0] = '@';
			color_spec_name = g_.MapName();
		}
	}
	else if (g_.ColorState() == COLORSTATE_MAP)
	{                         /* colors match g_color_file */
		m_color_spec[0] = '@';
		color_spec_name = g_color_file;
	}
	else                      /* colors match no .map that we know of */
	{
		strcpy(m_color_spec, "y");
	}

	if (m_color_spec[0] == '@')
	{
		const char *sptr2 = strrchr(color_spec_name.c_str(), SLASHC);
		if (sptr2 != 0)
		{
			color_spec_name = sptr2 + 1;
		}
		sptr2 = strrchr(color_spec_name.c_str(), ':');
		if (sptr2 != 0)
		{
			color_spec_name = sptr2 + 1;
		}
		strncpy(&m_color_spec[1], color_spec_name.c_str(), 12);
		m_color_spec[13] = 0;
	}
}

void MakeBatchFile::initialize()
{
	/* makepar map case */
	driver_stack_screen();
	HelpModeSaver saved_help(FIHELP_PARAMETER_FILES);

	m_max_color = g_colors;
	strcpy(m_color_spec, "y");
	if (has_clut())
	{
		set_max_color();
		set_color_spec();
	}
	strcpy(m_in_parameter_command_file, g_command_file.c_str());
	strcpy(m_in_parameter_command_name, g_command_name.c_str());
	for (int i = 0; i < 4; i++)
	{
		expand_comments(g_command_comment[i], par_comment[i]);
		strcpy(m_in_parameter_command_comment[i], g_command_comment[i].c_str());
	}

	if (g_command_name[0] == 0)
	{
		strcpy(m_in_parameter_command_name, "test");
	}
	m_pxdots = g_x_dots;
	m_pydots = g_y_dots;
	m_xm = 1;
	m_ym = 1;
	video_mode_key_name(g_.VideoEntry().keynum, m_video_mode);
	m_colors_only = (g_make_par_colors_only == false);
	m_pdelx = 0.0;
	m_pdely = 0.0;
	m_pdelx2 = 0.0;
	m_pdely2 = 0.0;
	m_pxxmin = 0.0;
	m_pyymax = 0.0;
	m_have_3rd = false;
}

void MakeBatchFile::fill_prompts(UIChoices &dialog)
{
	dialog.push("Parameter file", m_in_parameter_command_file, MAX_COMMENT - 1);
	dialog.push("Name", m_in_parameter_command_name, ITEMNAMELEN);
	dialog.push("Main comment", m_in_parameter_command_comment[0], MAX_COMMENT - 1);
	dialog.push("Second comment", m_in_parameter_command_comment[1], MAX_COMMENT - 1);
	dialog.push("Third comment", m_in_parameter_command_comment[2], MAX_COMMENT - 1);
	dialog.push("Fourth comment", m_in_parameter_command_comment[3], MAX_COMMENT - 1);
	if (has_clut())
	{
		dialog.push("Record colors?", m_color_spec, 13);
		dialog.push("    (no | yes | only for full info | @filename to point to a map file)");
		dialog.push("# of colors", m_max_color);
		m_max_color_index = dialog.num_prompts();
		dialog.push("    (if recording full color info)");
	}
	dialog.push("Maximum line length", g_max_line_length);
	dialog.push("");
	dialog.push("    **** The following is for generating images in pieces ****");
	dialog.push("X Multiples", m_xm);
	m_pieces_prompts = dialog.num_prompts();
	dialog.push("Y Multiples", m_ym);
	dialog.push("Video mode", m_video_mode, 4);
}
bool MakeBatchFile::check_video_mode()
{
	/* get resolution from the video name (which must be valid) */
	m_pxdots = 0;
	m_pydots = 0;
	int i = check_vidmode_keyname(m_video_mode);
	if (i > 0)
	{
		i = check_video_mode_key(i);
		if (i >= 0)
		{
			/* get the resolution of this video mode */
			m_pxdots = g_.VideoTable(i).x_dots;
			m_pydots = g_.VideoTable(i).y_dots;
		}
	}
	if (m_pxdots == 0 && (m_xm > 1 || m_ym > 1))
	{
		/* no corresponding video mode! */
		stop_message(STOPMSG_NORMAL, "Invalid video mode entry!");
		return true;
	}
	return false;
}
bool MakeBatchFile::check_bounds_xm_ym()
{
	/* bounds range on xm, ym */
	if (m_xm < 1 || m_xm > 36 || m_ym < 1 || m_ym > 36)
	{
		stop_message(STOPMSG_NORMAL, "X and Y components must be 1 to 36");
		return true;
	}
	return false;
}
bool MakeBatchFile::check_resolution()
{
	/* another sanity check: total resolution cannot exceed 65535 */
	if (m_xm*m_pxdots > 65535L || m_ym*m_pydots > 65535L)
	{
		stop_message(STOPMSG_NORMAL, "Total resolution (X or Y) cannot exceed 65535");
		return true;
	}
	return false;
}
void MakeBatchFile::execute_step1(std::ofstream &fpbat, int i, int j)
{
	if (m_xm > 1 || m_ym > 1)
	{
		int w;
		char c;
		char PCommandName[80];
		w = 0;
		while (w < int(strlen(g_command_name.c_str())))
		{
			c = g_command_name[w];
			if (isspace(c) || c == 0)
			{
				break;
			}
			PCommandName[w] = c;
			w++;
		}
		PCommandName[w] = 0;
		{
			strcat(PCommandName, str(boost::format("_%c%c") % par_key(i) % par_key(j)).c_str());
		}
		s_parameter_file << boost::format("%-19s{") % PCommandName;
		g_escape_time_state.m_grid_fp.x_min() = m_pxxmin + m_pdelx*(i*m_pxdots) + m_pdelx2*(j*m_pydots);
		g_escape_time_state.m_grid_fp.x_max() = m_pxxmin + m_pdelx*((i + 1)*m_pxdots - 1) + m_pdelx2*((j + 1)*m_pydots - 1);
		g_escape_time_state.m_grid_fp.y_min() = m_pyymax - m_pdely*((j + 1)*m_pydots - 1) - m_pdely2*((i + 1)*m_pxdots - 1);
		g_escape_time_state.m_grid_fp.y_max() = m_pyymax - m_pdely*(j*m_pydots) - m_pdely2*(i*m_pxdots);
		if (m_have_3rd)
		{
			g_escape_time_state.m_grid_fp.x_3rd() = m_pxxmin + m_pdelx*(i*m_pxdots) + m_pdelx2*((j + 1)*m_pydots - 1);
			g_escape_time_state.m_grid_fp.y_3rd() = m_pyymax - m_pdely*((j + 1)*m_pydots - 1) - m_pdely2*(i*m_pxdots);
		}
		else
		{
			g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
			g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
		}
		fpbat << boost::format("id batch=yes overwrite=yes @%s/%s\n") % g_command_file % PCommandName;
		fpbat << "If Errorlevel 2 goto oops\n";
	}
	else
	{
		s_parameter_file << boost::format("%-19s{") % g_command_name;
	}
}
void MakeBatchFile::execute_step2()
{
	/* guarantee that there are no blank comments above the last
	non-blank par_comment */
	int last = -1;
	for (int n = 0; n < 4; n++)
	{
		if (*par_comment[n])
		{
			last = n;
		}
	}
	for (int n = 0; n < last; n++)
	{
		if (g_command_comment[n].length() == 0)
		{
			g_command_comment[n] = ";";
		}
	}
}
void MakeBatchFile::execute_step3(int i, int j)
{
	if (g_command_comment[0].length() > 0)
	{
		s_parameter_file << " ; " << g_command_comment[0];
	}
	s_parameter_file << '\n';
	{
		char buf[25];
		memset(buf, ' ', 23);
		buf[23] = 0;
		buf[21] = ';';
		for (int k = 1; k < 4; k++)
		{
			if (g_command_comment[k].length() > 0)
			{
				s_parameter_file << buf << g_command_comment[k] << "\n";
			}
		}
		if (g_patch_level != 0 && !m_colors_only)
		{
			s_parameter_file << boost::format("Iterated Dynamics %s Version %d Patchlevel %d\n")
				% buf % g_release % g_patch_level;
		}
	}
	write_batch_parms(m_color_spec, m_colors_only, m_max_color, i, j);
	if (m_xm > 1 || m_ym > 1)
	{
		s_parameter_file << "  video=" << m_video_mode;
		s_parameter_file << boost::format(" savename=frmig_%c%c\n") % par_key(i) % par_key(j);
	}
	s_parameter_file << "  }\n\n";
}
void MakeBatchFile::execute()
{
	initialize();
	if (!g_make_par_flag)
	{
		goto skip_UI;
	}
	m_max_color_index = 0;
	while (true)
	{
prompt_user:
		{
			UIChoices dialog("Save Current Parameters", 0);
			fill_prompts(dialog);

			if (dialog.prompt() < 0)
			{
				break;
			}

			if (m_color_spec[0] == 'o' || !g_make_par_colors_only)
			{
				strcpy(m_color_spec, "y");
				m_colors_only = true;
			}

			g_command_file = m_in_parameter_command_file;
			if (has_extension(g_command_file) == 0)
			{
				g_command_file.append(".par");   /* default extension .par */
			}
			g_command_name = m_in_parameter_command_name;
			for (int i = 0; i < 4; i++)
			{
				g_command_comment[i] = m_in_parameter_command_comment[i];
			}
			if (has_clut())
			{
				if (dialog.values(m_max_color_index).uval.ival > 0 &&
					dialog.values(m_max_color_index).uval.ival <= 256)
				{
					m_max_color = dialog.values(m_max_color_index).uval.ival;
				}
			}
			m_prompts = m_pieces_prompts;
			{
				int newmaxlinelength = dialog.values(m_prompts - 3).uval.ival;
				if (g_max_line_length != newmaxlinelength &&
					newmaxlinelength >= MIN_MAX_LINE_LENGTH &&
					newmaxlinelength <= MAX_MAX_LINE_LENGTH)
				{
					g_max_line_length = newmaxlinelength;
				}
			}
			m_xm = dialog.values(m_prompts++).uval.ival;
			m_ym = dialog.values(m_prompts++).uval.ival;

			/* sanity checks */
			if (check_video_mode() || check_bounds_xm_ym() || check_resolution())
			{
				goto prompt_user;
			}
		}
skip_UI:
		if (!g_make_par_flag)
		{
			strcpy(m_color_spec, (g_file_colors > 0) ? "y" : "n");
			m_max_color = (!g_make_par_colors_only) ? 256 : g_file_colors;
		}
		strcpy(m_out_name, g_command_file.c_str());
		bool got_input_file = false;
		std::ifstream input_file;
		if (exists(g_command_file))
		{                         /* file exists */
			got_input_file = true;
			if (read_write_access(g_command_file.c_str()))
			{
				stop_message(STOPMSG_NORMAL, "Can't write " + g_command_file);
				continue;
			}
			int i = int(strlen(m_out_name));
			while (--i >= 0 && m_out_name[i] != SLASHC)
			{
				m_out_name[i] = 0;
			}
			strcat(m_out_name, "fractint.tmp");
			input_file.open(g_command_file.c_str(), std::ios::in);
		}
		s_parameter_file.open(m_out_name, std::ios::out);
		if (!s_parameter_file.is_open())
		{
			stop_message(STOPMSG_NORMAL, "Can't create " + out_name());
			if (got_input_file)
			{
				input_file.close();
			}
			continue;
		}

		if (got_input_file)
		{
			char buf[256];
			while (file_gets(buf, NUM_OF(buf)-1, input_file) >= 0)
			{
				char buf2[128];
				if (strchr(buf, '{')/* entry heading? */
					&& sscanf(buf, " %40[^ \t({]", buf2)
					&& stricmp(buf2, g_command_name.c_str()) == 0)
				{                   /* entry with same name */
					std::string prompt = "File already has an entry named " + g_command_name + "\n" +
						(g_make_par_flag ? "Continue to replace it, Cancel to back out" : "... Replacing ...");
					if (stop_message(STOPMSG_CANCEL | STOPMSG_INFO_ONLY, prompt) < 0)
					{                /* cancel */
						input_file.close();
						s_parameter_file.close();
						unlink(m_out_name);
						goto prompt_user;
					}
					while (strchr(buf, '}') == 0
							&& file_gets(buf, NUM_OF(buf)-1, input_file) > 0)
					{
						/* skip to end of set */
					}
					break;
				}
				s_parameter_file << buf << '\n';
			}
		}
/***** start here*/
		std::ofstream fpbat;
		if (m_xm > 1 || m_ym > 1)
		{
			m_have_3rd = (g_escape_time_state.m_grid_fp.x_min() != g_escape_time_state.m_grid_fp.x_3rd()
					|| g_escape_time_state.m_grid_fp.y_min() != g_escape_time_state.m_grid_fp.y_3rd());
			fpbat.open((g_work_dir / "makemig.bat").string().c_str());
			if (!fpbat.is_open())
			{
				m_xm = 0;
				m_ym = 0;
			}
			m_pdelx  = (g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd())/(m_xm*m_pxdots - 1);   /* calculate stepsizes */
			m_pdely  = (g_escape_time_state.m_grid_fp.y_max() - g_escape_time_state.m_grid_fp.y_3rd())/(m_ym*m_pydots - 1);
			m_pdelx2 = (g_escape_time_state.m_grid_fp.x_3rd() - g_escape_time_state.m_grid_fp.x_min())/(m_ym*m_pydots - 1);
			m_pdely2 = (g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_min())/(m_xm*m_pxdots - 1);

			/* save corners */
			m_pxxmin = g_escape_time_state.m_grid_fp.x_min();
			m_pyymax = g_escape_time_state.m_grid_fp.y_max();
		}
		for (int i = 0; i < int(m_xm); i++)  /* columns */
		{
			for (int j = 0; j < int(m_ym); j++)  /* rows    */
			{
				execute_step1(fpbat, i, j);
				execute_step2();
				execute_step3(i, j);
			}
		}
		if (m_xm > 1 || m_ym > 1)
		{
			fpbat << boost::format("Fractint makemig=%d/%d\n") % m_xm % m_ym
				<< "Rem Simplgif fractmig.gif simplgif.gif  in case you need it\n"
					":oops\n";
			fpbat.close();
		}
		/*******end here */

		if (got_input_file)
		{                         /* copy the rest of the file */
			int i;
			char buf[256];
			do
			{
				i = file_gets(buf, NUM_OF(buf)-1, input_file);
			}
			while (i == 0); /* skip blanks */
			while (i >= 0)
			{
				s_parameter_file << buf << '\n';
				i = file_gets(buf, NUM_OF(buf)-1, input_file);
			}
			input_file.close();
		}
		s_parameter_file.close();
		if (got_input_file)
		{                         /* replace the original file with the new */
			_unlink(g_command_file.c_str());   /* success assumed on these lines       */
			rename(m_out_name, g_command_file.c_str());  /* since we checked earlier with access */
		}
		break;
	}
	driver_unstack_screen();
}

static struct write_batch_data /* buffer for parms to break lines nicely */
{
	int len;
	char buf[10000];
} s_wbdata;

static void write_universal_3d_parameters()
{
	if (g_display_3d)  /* universal 3d */
	{
		/***** common (fractal & transform) 3d parameters in this section *****/
		if (!g_3d_state.sphere() || g_display_3d == DISPLAY3D_GENERATED)
		{
			put_parm(format(" rotation=%d/%d/%d")
				% g_3d_state.x_rotation() % g_3d_state.y_rotation() % g_3d_state.z_rotation());
		}
		put_parm(format(" perspective=%d") % g_3d_state.z_viewer());
		put_parm(format(" xyshift=%d/%d") % g_3d_state.x_shift() % g_3d_state.y_shift());
		if (g_3d_state.x_trans() || g_3d_state.y_trans())
		{
			put_parm(format(" xyadjust=%d/%d") % g_3d_state.x_trans() % g_3d_state.y_trans());
		}
		if (g_3d_state.glasses_type())
		{
			put_parm(format(" stereo=%d") % g_3d_state.glasses_type());
			put_parm(format(" interocular=%d") % g_3d_state.eye_separation());
			put_parm(format(" converge=%d") % g_3d_state.x_adjust());
			put_parm(format(" crop=%d/%d/%d/%d")
				% g_3d_state.red().crop_left() % g_3d_state.red().crop_right()
				% g_3d_state.blue().crop_left() % g_3d_state.blue().crop_right());
			put_parm(format(" bright=%d/%d") % g_3d_state.red().bright() % g_3d_state.blue().bright());
		}
	}
}

static void write_3d_parameters()
{
	if ((g_display_3d == DISPLAY3D_YES) || (g_display_3d == DISPLAY3D_OVERLAY))
	{
		/***** 3d transform only parameters in this section *****/
		if (g_display_3d == DISPLAY3D_OVERLAY)
		{
			put_parm(" 3d=overlay");
		}
		else
		{
			put_parm(" 3d=yes");
		}
		if (g_loaded_3d == 0)
		{
			put_filename("filename", g_read_name);
		}

		if (g_3d_state.sphere())
		{
			put_parm(" sphere=y");
			put_parm(format(" latitude=%d/%d") % g_3d_state.theta1() % g_3d_state.theta2());
			put_parm(format(" longitude=%d/%d") % g_3d_state.phi1() % g_3d_state.phi2());
			put_parm(format(" radius=%d") % g_3d_state.radius());
		}
		put_parm(format(" scalexyz=%d/%d") % g_3d_state.x_scale() % g_3d_state.y_scale());
		put_parm(format(" roughness=%d") % g_3d_state.roughness());
		put_parm(format(" waterline=%d") % g_3d_state.water_line());
		if (g_3d_state.fill_type())
		{
			put_parm(format(" filltype=%d") % g_3d_state.fill_type());
		}
		if (g_3d_state.transparent0() || g_3d_state.transparent1())
		{
			put_parm(format(" transparent=%d/%d") % g_3d_state.transparent0() % g_3d_state.transparent1());
		}
		if (g_3d_state.preview())
		{
			put_parm(" preview=yes");
			if (g_3d_state.show_box())
			{
				put_parm(" showbox=yes");
			}
			put_parm(format(" coarse=%d") % g_3d_state.preview_factor());
		}
		if (g_3d_state.raytrace_output())
		{
			put_parm(format(" ray=%d") % g_3d_state.raytrace_output());
			if (g_3d_state.raytrace_brief())
			{
				put_parm(" brief=y");
			}
		}
		if (g_3d_state.fill_type() > FillType::Bars)
		{
			put_parm(format(" lightsource=%d/%d/%d")
				% g_3d_state.x_light() % g_3d_state.y_light() % g_3d_state.z_light());
			if (g_3d_state.light_avg())
			{
				put_parm(format(" smoothing=%d") % g_3d_state.light_avg());
			}
		}
		if (g_3d_state.randomize_colors())
		{
			put_parm(format(" randomize=%d") % g_3d_state.randomize_colors());
		}
		if (g_targa_output)
		{
			put_parm(" fullcolor=y");
		}
		if (g_grayscale_depth)
		{
			put_parm(" usegrayscale=y");
		}
		if (g_3d_state.ambient())
		{
			put_parm(format(" ambient=%d") % g_3d_state.ambient());
		}
		if (g_3d_state.haze())
		{
			put_parm(format(" haze=%d") % g_3d_state.haze());
		}
		if (g_3d_state.background_red() != 51
			|| g_3d_state.background_green() != 153
			|| g_3d_state.background_blue() != 200)
		{
			put_parm(format(" background=%d/%d/%d")
				% g_3d_state.background_red()
				% g_3d_state.background_green()
				% g_3d_state.background_blue());
		}
	}
}

void write_batch_parms_julibrot()
{
	put_parm(format(" julibrotfromto=%.15g/%.15g/%.15g/%.15g")
		% g_m_x_max_fp % g_m_x_min_fp % g_m_y_max_fp % g_m_y_min_fp);
	/* these rarely change */
	if (g_origin_fp != 8 || g_height_fp != 7 || g_width_fp != 10 || g_screen_distance_fp != 24
		|| g_depth_fp != 8 || g_z_dots != 128)
	{
		put_parm(format(" julibrot3d=%d/%g/%g/%g/%g/%g")
			% g_z_dots % g_origin_fp % g_depth_fp % g_height_fp % g_width_fp % g_screen_distance_fp);
	}
	if (g_eyes_fp != 0)
	{
		put_parm(format(" julibroteyes=%g") % g_eyes_fp);
	}
	if (g_new_orbit_type != FRACTYPE_JULIA)
	{
		put_parm(format(" orbitname=%s") % g_fractal_specific[g_new_orbit_type].get_type());
	}
	if (g_juli_3d_mode != JULI3DMODE_MONOCULAR)
	{
		put_parm(format(" 3dmode=%s") % g_juli_3d_options[g_juli_3d_mode]);
	}
}

void write_batch_parms_formula()
{
	put_filename("formulafile", g_formula_state.get_filename());
	put_parm(format(" formulaname=%s") % g_formula_state.get_formula());
	if (g_formula_state.uses_is_mand())
	{
		put_parm(format(" ismand=%c") % (g_is_mandelbrot ? 'y' : 'n'));
	}
}

void write_batch_parms_l_system()
{
	put_filename("lfile", g_l_system_filename);
	put_parm(" lname=" + g_l_system_name);
}

void write_batch_parms_ifs()
{
	put_filename("ifsfile", g_ifs_filename);
	put_parm(" ifs=" + g_ifs_name);
}

void write_batch_parms_inverse_julia()
{
	put_parm(format(" miim=%s/%s") % g_jiim_method[g_major_method] % g_jiim_left_right[g_minor_method]);
}

void write_batch_parms_center_mag_yes(bf_t bfXctr, bf_t bfYctr)
{
	LDBL Magnification;
	double Xmagfactor;
	double Rotation;
	double Skew;
	if (g_bf_math)
	{
		int digits;
		convert_center_mag_bf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
		digits = get_precision_bf(MAXREZ);
		put_parm(" center-mag=");
		put_bf(0, bfXctr, digits);
		put_bf(1, bfYctr, digits);
	}
	else /* !g_bf_math */
	{
		double Xctr;
		double Yctr;
		convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
		put_parm(" center-mag=");
		/* convert 1000 fudged long to double, 1000/1<<24 = 6e-5 */
		put_parm(format(g_delta_min_fp > 6e-5 ? "%g/%g" : "%+20.17lf/%+20.17lf") % Xctr % Yctr);
	}
#ifdef USE_LONG_DOUBLE
	put_parm(format("/%.7Lg") % Magnification); /* precision of magnification not critical, but magnitude is */
#else
	put_parm("/%.7lg") % Magnification); /* precision of magnification not critical, but magnitude is */
#endif
	/* Round to avoid ugly decimals, precision here is not critical */
	/* Don't round Xmagfactor if it's small */
	if (fabs(Xmagfactor) > 0.5) /* or so, exact value isn't important */
	{
		Xmagfactor = (sign(Xmagfactor)*long(fabs(Xmagfactor)*1e4 + 0.5))/1e4;
	}
	/* Just truncate these angles.  Who cares about 1/1000 of a degree */
	/* Somebody does.  Some rotated and/or skewed images are slightly */
	/* off when recreated from a PAR using 1/1000. */
	/* JCO 08052001 */
#if 0
	Rotation   = long(Rotation*1e3)/1e3;
	Skew       = long(Skew*1e3)/1e3;
#endif
	if (Xmagfactor != 1 || Rotation != 0 || Skew != 0)
	{	/* Only put what is necessary */
		/* The difference with Xmagfactor is that it is normally */
		/* near 1 while the others are normally near 0 */
		if (fabs(Xmagfactor) >= 1)
		{
			put_float(1, Xmagfactor, 5); /* put_float() uses %g */
		}
		else /* abs(Xmagfactor) is < 1 */
		{
			put_float(1, Xmagfactor, 4); /* put_float() uses %g */
		}
		if (Rotation != 0 || Skew != 0)
		{
			/* Use precision = 6 here.  These angle have already been rounded        */
			/* to 3 decimal places, but angles like 123.456 degrees need 6         */
			/* sig figs to get 3 decimal places.  Trailing 0's are dropped anyway. */
			/* Changed to 18 to address rotated and skewed problem w/ PARs */
			/* JCO 08052001 */
			put_float(1, Rotation, 18);
			if (Skew != 0)
			{
				put_float(1, Skew, 18);
			}
		}
	}
}

void write_batch_parms_center_mag_no()
{
	put_parm(" corners=");
	if (g_bf_math)
	{
		int digits;
		digits = get_precision_bf(MAXREZ);
		put_bf(0, g_escape_time_state.m_grid_bf.x_min(), digits);
		put_bf(1, g_escape_time_state.m_grid_bf.x_max(), digits);
		put_bf(1, g_escape_time_state.m_grid_bf.y_min(), digits);
		put_bf(1, g_escape_time_state.m_grid_bf.y_max(), digits);
		if (cmp_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min()) || cmp_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min()))
		{
			put_bf(1, g_escape_time_state.m_grid_bf.x_3rd(), digits);
			put_bf(1, g_escape_time_state.m_grid_bf.y_3rd(), digits);
		}
	}
	else
	{
		int xdigits = getprec(g_escape_time_state.m_grid_fp.x_min(), g_escape_time_state.m_grid_fp.x_max(), g_escape_time_state.m_grid_fp.x_3rd());
		int ydigits = getprec(g_escape_time_state.m_grid_fp.y_min(), g_escape_time_state.m_grid_fp.y_max(), g_escape_time_state.m_grid_fp.y_3rd());
		put_float(0, g_escape_time_state.m_grid_fp.x_min(), xdigits);
		put_float(1, g_escape_time_state.m_grid_fp.x_max(), xdigits);
		put_float(1, g_escape_time_state.m_grid_fp.y_min(), ydigits);
		put_float(1, g_escape_time_state.m_grid_fp.y_max(), ydigits);
		if (g_escape_time_state.m_grid_fp.x_3rd() != g_escape_time_state.m_grid_fp.x_min() || g_escape_time_state.m_grid_fp.y_3rd() != g_escape_time_state.m_grid_fp.y_min())
		{
			put_float(1, g_escape_time_state.m_grid_fp.x_3rd(), xdigits);
			put_float(1, g_escape_time_state.m_grid_fp.y_3rd(), ydigits);
		}
	}
}

void write_batch_parms_parameters()
{
	int i;
	for (i = (MAX_PARAMETERS-1); i >= 0; --i)
	{
		if (type_has_parameter(fractal_type_julibrot(g_fractal_type) ?
				g_new_orbit_type : g_fractal_type, i))
		{
			break;
		}
	}

	if (i >= 0)
	{
		if (fractal_type_ant_or_cellular(g_fractal_type))
		{
			put_parm(format(" params=%.1f") % g_parameters[0]);
		}
		else
		{
#ifdef USE_LONG_DOUBLE
			if (DEBUGMODE_MORE_DIGITS == g_debug_mode)
			{
				put_parm(format(" params=%.17Lg") % (long double)g_parameters[0]);
			}
			else
#endif
			{
				put_parm(format(" params=%.17g") % g_parameters[0]);
			}
		}
		for (int j = 1; j <= i; ++j)
		{
			if (fractal_type_ant_or_cellular(g_fractal_type))
			{
				put_parm(format("/%.1f") % g_parameters[j]);
			}
			else
			{
#ifdef USE_LONG_DOUBLE
				if (DEBUGMODE_MORE_DIGITS == g_debug_mode)
				{
					put_parm(format("/%.17Lg") % (long double)g_parameters[j]);
				}
				else
#endif
				{
					put_parm(format("/%.17g") % g_parameters[j]);
				}
			}
		}
	}
}

void write_batch_parms_initial_orbit()
{
	if (g_use_initial_orbit_z == INITIALZ_PIXEL)
	{
		put_parm(" initorbit=pixel");
	}
	else if (g_use_initial_orbit_z == INITIALZ_ORBIT)
	{
		put_parm(format(" initorbit=%.15g/%.15g") % g_initial_orbit_z.x % g_initial_orbit_z.y);
	}
}

void write_batch_parms_passes()
{
	if (g_user_standard_calculation_mode != CALCMODE_SOLID_GUESS)
	{
		put_parm(format(" passes=%c") % char(g_user_standard_calculation_mode));
	}
	if (g_stop_pass != 0)
	{
		put_parm(format(" passes=%c%c") % char(g_user_standard_calculation_mode) % char(g_stop_pass + '0'));
	}
}

void write_batch_parms_reset()
{
	put_parm(format(" reset=%d") % (check_back() ?
		std::min(g_save_release, g_release) : g_release));
}

void write_batch_parms_type()
{
	put_parm(format(" type=%s") % g_current_fractal_specific->get_type());

	if (fractal_type_julibrot(g_fractal_type))
	{
		write_batch_parms_julibrot();
	}
	if (fractal_type_formula(g_fractal_type))
	{
		write_batch_parms_formula();
	}
	if (g_fractal_type == FRACTYPE_L_SYSTEM)
	{
		write_batch_parms_l_system();
	}
	if (fractal_type_ifs(g_fractal_type))
	{
		write_batch_parms_ifs();
	}
	if (fractal_type_inverse_julia(g_fractal_type))
	{
		write_batch_parms_inverse_julia();
	}
}

void write_batch_parms_function()
{
	char buf[81];
	show_function(buf); /* this function is in miscres.c */
	if (buf[0])
	{
		put_parm(buf);
	}
}

void write_batch_parms_float()
{
	if (g_float_flag)
	{
		put_parm(" float=y");
	}
}

void write_batch_parms_max_iteration()
{
	if (g_max_iteration != 150)
	{
		put_parm(format(" maxiter=%ld") % g_max_iteration);
	}
}

void write_batch_parms_bail_out()
{
	if (g_bail_out && (!g_potential_flag || g_potential_parameter[2] == 0.0))
	{
		put_parm(format(" bailout=%ld") % g_bail_out);
	}

	if (g_bail_out_test != BAILOUT_MODULUS)
	{
		put_parm(" bailoutest=");
		if (g_bail_out_test == BAILOUT_REAL)
		{
			put_parm("real");
		}
		else if (g_bail_out_test == BAILOUT_IMAGINARY)
		{
			put_parm("imag");
		}
		else if (g_bail_out_test == BAILOUT_OR)
		{
			put_parm("or");
		}
		else if (g_bail_out_test == BAILOUT_AND)
		{
			put_parm("and");
		}
		else if (g_bail_out_test == BAILOUT_MANHATTAN)
		{
			put_parm("manh");
		}
		else if (g_bail_out_test == BAILOUT_MANHATTAN_R)
		{
			put_parm("manr");
		}
		else
		{
			put_parm("mod"); /* default, just in case */
		}
	}
}

void write_batch_parms_fill_color()
{
	if (g_fill_color != -1)
	{
		put_parm(format(" fillcolor=%d") % g_fill_color);
	}
}

void write_batch_parms_inside()
{
	if (g_inside != 1)
	{
		put_parm(" inside=");
		if (g_inside == COLORMODE_ITERATION)
		{
			put_parm("maxiter");
		}
		else if (g_inside == COLORMODE_Z_MAGNITUDE)
		{
			put_parm("zmag");
		}
		else if (g_inside == COLORMODE_BEAUTY_OF_FRACTALS_60)
		{
			put_parm("bof60");
		}
		else if (g_inside == COLORMODE_BEAUTY_OF_FRACTALS_61)
		{
			put_parm("bof61");
		}
		else if (g_inside == COLORMODE_EPSILON_CROSS)
		{
			put_parm("epsiloncross");
		}
		else if (g_inside == COLORMODE_STAR_TRAIL)
		{
			put_parm("startrail");
		}
		else if (g_inside == COLORMODE_PERIOD)
		{
			put_parm("period");
		}
		else if (g_inside == COLORMODE_FLOAT_MODULUS_INTEGER)
		{
			put_parm("fmod");
		}
		else if (g_inside == COLORMODE_INVERSE_TANGENT_INTEGER)
		{
			put_parm("atan");
		}
		else
		{
			put_parm(format("%d") % g_inside);
		}
	}
}

void write_batch_parms_proximity()
{
	if (g_proximity != 0.01
		&& (g_inside == COLORMODE_EPSILON_CROSS
		|| g_inside == COLORMODE_FLOAT_MODULUS_INTEGER
		|| g_outside == COLORMODE_FLOAT_MODULUS))
	{
		put_parm(format(" proximity=%.15g") % g_proximity);
	}
}

void write_batch_parms_outside()
{
	if (g_outside != COLORMODE_ITERATION)
	{
		put_parm(" outside=");
		if (g_outside == COLORMODE_REAL)
		{
			put_parm("real");
		}
		else if (g_outside == COLORMODE_IMAGINARY)
		{
			put_parm("imag");
		}
		else if (g_outside == COLORMODE_MULTIPLY)
		{
			put_parm("mult");
		}
		else if (g_outside == COLORMODE_SUM)
		{
			put_parm("summ");
		}
		else if (g_outside == COLORMODE_INVERSE_TANGENT)
		{
			put_parm("atan");
		}
		else if (g_outside == COLORMODE_FLOAT_MODULUS)
		{
			put_parm("fmod");
		}
		else if (g_outside == COLORMODE_TOTAL_DISTANCE)
		{
			put_parm("tdis");
		}
		else
		{
			put_parm(format("%d") % g_outside);
		}
	}
}

void write_batch_parms_log_map()
{
	if (g_log_palette_mode && !g_ranges_length)
	{
		put_parm(" logmap=");
		if (g_log_palette_mode == LOGPALETTE_OLD)
		{
			put_parm("old");
		}
		else if (g_log_palette_mode == LOGPALETTE_STANDARD)
		{
			put_parm("yes");
		}
		else
		{
			put_parm(format("%ld") % g_log_palette_mode);
		}
	}
}

void write_batch_parms_log_mode()
{
	if (g_log_dynamic_calculate && g_log_palette_mode && !g_ranges_length)
	{
		put_parm(" logmode=");
		if (g_log_dynamic_calculate == LOGDYNAMIC_DYNAMIC)
		{
			put_parm("fly");
		}
		else if (g_log_dynamic_calculate == LOGDYNAMIC_TABLE)
		{
			put_parm("table");
		}
	}
}

void write_batch_parms_potential()
{
	if (g_potential_flag)
	{
		put_parm(format(" potential=%d/%g/%d")
			% int(g_potential_parameter[0]) % g_potential_parameter[1] % int(g_potential_parameter[2]));
		if (g_potential_16bit)
		{
			put_parm("/16bit");
		}
	}
}

void write_batch_parms_invert()
{
	if (g_invert)
	{
		put_parm(format(" invert=%-1.15lg/%-1.15lg/%-1.15lg")
			% g_inversion[0] % g_inversion[1] % g_inversion[2]);
	}
}

void write_batch_parms_decomp()
{
	if (g_decomposition[0])
	{
		put_parm(format(" decomp=%d") % g_decomposition[0]);
	}
}

void write_batch_parms_center_mag(big_t bfXctr, big_t bfYctr)
{
	if (g_use_center_mag)
	{
		write_batch_parms_center_mag_yes(bfXctr, bfYctr);
	}
	else
	{
		write_batch_parms_center_mag_no();
	}
}

void write_batch_parms_distest()
{
	if (g_distance_test)
	{
		put_parm(format(" distest=%ld/%d/%d/%d") % g_distance_test % g_distance_test_width %
			(g_pseudo_x ? g_pseudo_x : g_x_dots) % (g_pseudo_y ? g_pseudo_y : g_y_dots));
	}
}

void write_batch_parms_old_dem_colors()
{
	if (g_old_demm_colors)
	{
		put_parm(" olddemmcolors=y");
	}
}

void write_batch_parms_biomorph()
{
	if (g_user_biomorph != -1)
	{
		put_parm(format(" biomorph=%d") % g_user_biomorph);
	}
}

void write_batch_parms_finite_attractor()
{
	if (g_finite_attractor)
	{
		// TODO: not quite right, doesn't allow FINITE_ATTRACTOR_PHASE
		put_parm(" finattract=y");
	}
}

void write_batch_parms_symmetry(int ii, int jj)
{
	if (g_force_symmetry != FORCESYMMETRY_NONE)
	{
		if (g_force_symmetry == FORCESYMMETRY_SEARCH && ii == 1 && jj == 1)
		{
			stop_message(STOPMSG_NORMAL, "Regenerate before <b> to get correct symmetry");
		}
		put_parm(" symmetry=");
		if (g_force_symmetry == SYMMETRY_X_AXIS)
		{
			put_parm("xaxis");
		}
		else if (g_force_symmetry == SYMMETRY_Y_AXIS)
		{
			put_parm("yaxis");
		}
		else if (g_force_symmetry == SYMMETRY_XY_AXIS)
		{
			put_parm("xyaxis");
		}
		else if (g_force_symmetry == SYMMETRY_ORIGIN)
		{
			put_parm("origin");
		}
		else if (g_force_symmetry == SYMMETRY_PI)
		{
			put_parm("pi");
		}
		else
		{
			put_parm("none");
		}
	}
}

void write_batch_parms_periodicity()
{
	if (g_periodicity_check != 1)
	{
		put_parm(format(" periodicity=%d") % g_periodicity_check);
	}
}

void write_batch_parms_random_see()
{
	if (g_use_fixed_random_seed)
	{
		put_parm(format(" rseed=%d") % g_random_seed);
	}
}

void write_batch_parms_ranges()
{
	if (g_ranges_length)
	{
		put_parm(" ranges=");
		int i = 0;
		while (i < g_ranges_length)
		{
			if (i)
			{
				put_parm("/");
			}
			if (g_ranges[i] == -1)
			{
				put_parm(format("-%d/") % g_ranges[++i]);
				++i;
			}
			put_parm(format("%d") % g_ranges[i++]);
		}
	}
}

void write_batch_parms_view_windows()
{
	put_parm(g_viewWindow.CommandParameters());
}

void write_batch_parms_math_tolerance()
{
	if (g_math_tolerance[0] != 0.05 || g_math_tolerance[1] != 0.05)
	{
		put_parm(format(" mathtolerance=%g/%g") % g_math_tolerance[0] % g_math_tolerance[1]);
	}
}

void write_batch_parms_cycle_range()
{
	if (g_rotate_lo != 1 || g_rotate_hi != 255)
	{
		put_parm(format(" cyclerange=%d/%d") % g_rotate_lo % g_rotate_hi);
	}
}

void write_batch_parms_no_beauty_of_fractals()
{
	if (!g_beauty_of_fractals)
	{
		put_parm(" nobof=yes");
	}
}

void write_batch_parms_orbit_delay()
{
	if (g_orbit_delay > 0)
	{
		put_parm(format(" orbitdelay=%d") % g_orbit_delay);
	}
}

void write_batch_parms_orbit_interval()
{
	if (g_orbit_interval != 1)
	{
		put_parm(format(" orbitinterval=%d") % g_orbit_interval);
	}
}

void write_batch_parms_show_orbit()
{
	if (g_start_show_orbit)
	{
		put_parm(" showorbit=yes");
	}
}

void write_batch_parms_screen_coords()
{
	if (g_keep_screen_coords)
	{
		put_parm(" screencoords=yes");
	}
}

void write_batch_parms_orbit_corners()
{
	if (g_user_standard_calculation_mode == CALCMODE_ORBITS && g_set_orbit_corners && g_keep_screen_coords)
	{
		put_parm(" orbitcorners=");
		int xdigits = getprec(g_orbit_x_min, g_orbit_x_max, g_orbit_x_3rd);
		int ydigits = getprec(g_orbit_y_min, g_orbit_y_max, g_orbit_y_3rd);
		put_float(0, g_orbit_x_min, xdigits);
		put_float(1, g_orbit_x_max, xdigits);
		put_float(1, g_orbit_y_min, ydigits);
		put_float(1, g_orbit_y_max, ydigits);
		if (g_orbit_x_3rd != g_orbit_x_min || g_orbit_y_3rd != g_orbit_y_min)
		{
			put_float(1, g_orbit_x_3rd, xdigits);
			put_float(1, g_orbit_y_3rd, ydigits);
		}
	}
}

void write_batch_parms_orbit_draw_mode()
{
	if (g_orbit_draw_mode != ORBITDRAW_RECTANGLE)
	{
		char args[3] = { 'r', 'l', 'f' };
		assert(g_orbit_draw_mode >= 0 && g_orbit_draw_mode <= NUM_OF(args));
		put_parm(format(" orbitdrawmode=%c") % args[g_orbit_draw_mode]);
	}
}

void write_batch_parms_colors_table(int maxcolor)
{
	int curc = 0;
	int force = 0;
	int diffmag = -1;
	while (true)
	{
		/* emit color in rgb 3 char encoded form */
		char buf[81];
		put_parm(" colors=");
		for (int j = 0; j < 3; ++j)
		{
			int k = g_.DAC().Channel(curc, j);
			if (k < 10)
			{
				k += '0';
			}
			else if (k < 36)
			{
				k += ('A' - 10);
			}
			else
			{
				k += ('_' - 36);
			}
			buf[j] = char(k);
		}
		buf[3] = 0;
		put_parm(buf);
		if (++curc >= maxcolor)      /* quit if done last color */
		{
			break;
		}
		if (DEBUGMODE_COLORS_LOSSLESS == g_debug_mode)  /* lossless compression */
		{
			continue;
		}
		/* Next a P Branderhorst special, a tricky scan for smooth-shaded
		ranges which can be written as <nn> to compress .par file entry.
		Method used is to check net change in each color value over
		spans of 2 to 5 color numbers.  First time for each span size
		the value change is noted.  After first time the change is
		checked against noted change.  First time it differs, a
		a difference of 1 is tolerated and noted as an alternate
		acceptable change.  When change is not one of the tolerated
		values, loop exits. */
		if (force)
		{
			--force;
			continue;
		}
		int scanc = curc;
		int k;
		while (scanc < maxcolor)  /* scan while same diff to next */
		{
			int i = scanc - curc;
			if (i > 3) /* check spans up to 4 steps */
			{
				i = 3;
			}
			for (k = 0; k <= i; ++k)
			{
				int j;
				int diff1[4][3];
				int diff2[4][3];
				for (j = 0; j < 3; ++j)  /* check pattern of chg per color */
				{
					if (g_debug_mode != DEBUGMODE_NO_COLORS_FIX && scanc > (curc + 4) && scanc < maxcolor-5)
					{
						if (abs(2*g_.DAC().Channel(scanc, j) - g_.DAC().Channel(scanc-5, j)
						- g_.DAC().Channel(scanc + 5, j)) >= 2)
						{
							break;
						}
					}
					int delta = int(g_.DAC().Channel(scanc, j)) - int(g_.DAC().Channel(scanc-k-1, j));
					if (k == scanc - curc)
					{
						diff1[k][j] = delta;
						diff2[k][j] = delta;
					}
					else if (delta != diff1[k][j] && delta != diff2[k][j])
					{
						diffmag = abs(delta - diff1[k][j]);
						if (diff1[k][j] != diff2[k][j] || diffmag != 1)
						{
							break;
						}
						diff2[k][j] = delta;
					}
				}
				if (j < 3) /* must've exited from inner loop above  */
				{
					break;
				}
			}
			if (k <= i) /* must've exited from inner loop above  */
			{
				break;
			}
			++scanc;
		}
		/* now scanc-1 is next color which must be written explicitly */
		if (scanc - curc > 2)  /* good, we have a shaded range */
		{
			if (scanc != maxcolor)
			{
				if (diffmag < 3)  /* not a sharp slope change? */
				{
					force = 2;       /* force more between ranges, to stop  */
					--scanc;         /* "drift" when load/store/load/store/ */
				}
				if (k)  /* more of the same                    */
				{
					force += k;
					--scanc;
				}
			}
			if (--scanc - curc > 1)
			{
				put_parm(format("<%d>") % (scanc-curc));
				curc = scanc;
			}
			else                /* changed our mind */
			{
				force = 0;
			}
		}
	}
}

void write_batch_parms_flush()
{
	while (s_wbdata.len) /* flush the buffer */
	{
		put_parm_line();
	}
}

void write_batch_parms_colors_header(char const *colorinf)
{
	if (g_record_colors == RECORDCOLORS_COMMENT && colorinf[0] == '@')
	{
		put_parm_line();
		put_parm("; colors=");
		put_parm(colorinf);
		put_parm_line();
	}
}

void write_batch_parms_colors(const char *colorinf, int maxcolor)
{
	if (g_record_colors != RECORDCOLORS_COMMENT
		&& g_record_colors != RECORDCOLORS_YES
		&& colorinf[0] == '@')
	{
		put_parm(colorinf);
	}
	else
	{
		write_batch_parms_colors_table(maxcolor);
	}
}

void write_batch_parms(const char *colorinf, bool colors_only, int maxcolor, int ii, int jj)
{
	bf_t bfXctr = 0;
	bf_t bfYctr = 0;
	int saved = save_stack();
	if (g_bf_math)
	{
		bfXctr = alloc_stack(g_bf_length + 2);
		bfYctr = alloc_stack(g_bf_length + 2);
	}

	s_wbdata.len = 0; /* force first parm to start on new line */

	/* Using near string g_box_x for buffer after saving to extraseg */

	if (colors_only)
	{
		write_batch_parms_colors(colorinf, maxcolor);
	}
	else
	{
		if (g_display_3d == DISPLAY3D_NONE || g_display_3d == DISPLAY3D_GENERATED)
		{
			/****** fractal only parameters in this section *******/
			write_batch_parms_reset();
			write_batch_parms_type();
			write_batch_parms_function();
			write_batch_parms_passes();
			write_batch_parms_center_mag(bfXctr, bfYctr);
			write_batch_parms_parameters();
			write_batch_parms_initial_orbit();
			write_batch_parms_float();
			write_batch_parms_max_iteration();
			write_batch_parms_bail_out();
			write_batch_parms_fill_color();
			write_batch_parms_inside();
			write_batch_parms_proximity();
			write_batch_parms_outside();
			write_batch_parms_log_map();
			write_batch_parms_log_mode();
			write_batch_parms_potential();
			write_batch_parms_invert();
			write_batch_parms_decomp();
			write_batch_parms_distest();
			write_batch_parms_old_dem_colors();
			write_batch_parms_biomorph();
			write_batch_parms_finite_attractor();
			write_batch_parms_symmetry(ii, jj);
			write_batch_parms_periodicity();
			write_batch_parms_random_see();
			write_batch_parms_ranges();
		}

		write_3d_parameters();
		write_universal_3d_parameters();

		/***** universal parameters in this section *****/

		write_batch_parms_view_windows();
		write_batch_parms_cycle_range();
		put_parm(g_sound_state.parameter_text());
		write_batch_parms_no_beauty_of_fractals();
		write_batch_parms_orbit_delay();
		write_batch_parms_orbit_interval();
		write_batch_parms_show_orbit();
		write_batch_parms_screen_coords();
		write_batch_parms_orbit_corners();
		write_batch_parms_orbit_draw_mode();
		write_batch_parms_math_tolerance();

		if (colorinf[0] != 'n')
		{
			write_batch_parms_colors_header(colorinf);
			write_batch_parms_colors(colorinf, maxcolor);
		}
	}

	write_batch_parms_flush();
	restore_stack(saved);
}

static void put_filename(const char *keyword, const char *fname)
{
	if (*fname && !ends_with_slash(fname))
	{
		const char *p = strrchr(fname, SLASHC);
		if (p != 0)
		{
			fname = p + 1;
			if (*fname == 0)
			{
				return;
			}
		}
		put_parm(format(" %s=%s") % keyword % fname);
	}
}

static void put_filename(const char *keyword, const std::string &fname)
{
	put_filename(keyword, fname.c_str());
}

static void put_parm(const char *parm)
{
	if (parm[0] == ' '             /* starting a new parm */
			&& s_wbdata.len == 0)       /* skip leading space */
	{
		++parm;
	}
	char *bufptr = s_wbdata.buf + s_wbdata.len;
	strcpy(bufptr, parm);
	s_wbdata.len += int(strlen(parm));
	while (s_wbdata.len > 200)
	{
		put_parm_line();
	}
}

static void put_parm(const std::string &message)
{
	put_parm(message.c_str());
}

static void put_parm(const format &message)
{
	put_parm(message.str());
}

int g_max_line_length = 72;
#define MAXLINELEN  g_max_line_length
#define NICELINELEN (MAXLINELEN-4)

static void put_parm_line()
{
	int c;
	int len = s_wbdata.len;
	if (len > NICELINELEN)
	{
		len = NICELINELEN + 1;
		while (--len != 0 && s_wbdata.buf[len] != ' ')
		{
		}
		if (len == 0)
		{
			len = NICELINELEN-1;
			while (++len < MAXLINELEN
				&& s_wbdata.buf[len] && s_wbdata.buf[len] != ' ')
			{
			}
		}
	}
	c = s_wbdata.buf[len];
	s_wbdata.buf[len] = 0;
	s_parameter_file << "  " << s_wbdata.buf;
	if (c && c != ' ')
	{
		s_parameter_file << '\\';
	}
	s_parameter_file << '\n';
	s_wbdata.buf[len] = char(c);
	if (c == ' ')
	{
		++len;
	}
	s_wbdata.len -= len;
	strcpy(s_wbdata.buf, s_wbdata.buf + len);
}

int get_precision_mag_bf()
{
	double Xmagfactor;
	double Rotation;
	double Skew;
	LDBL Magnification;
	big_t bXctr;
	big_t bYctr;
	int saved;
	int dec;

	saved = save_stack();
	bXctr            = alloc_stack(g_bf_length + 2);
	bYctr            = alloc_stack(g_bf_length + 2);
	/* this is just to find Magnification */
	convert_center_mag_bf(bXctr, bYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
	restore_stack(saved);

	/* I don't know if this is portable, but something needs to */
	/* be used in case compiler's LDBL_MAX is not big enough    */
	if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
	{
		return -1;
	}

	dec = get_power_10(Magnification) + 4; /* 4 digits of padding sounds good */
	return dec;
}

static int getprec(double a, double b, double c)
{
	double diff;
	double temp;
	int digits;
	double highv = 1.0E20;
	diff = fabs(a - b);
	if (diff == 0.0)
	{
		diff = highv;
	}
	temp = fabs(a - c);
	if (temp == 0.0)
	{
		temp = highv;
	}
	if (temp < diff)
	{
		diff = temp;
	}
	temp = fabs(b - c);
	if (temp == 0.0)
	{
		temp = highv;
	}
	if (temp < diff)
	{
		diff = temp;
	}
	digits = 7;
	if (g_debug_mode >= DEBUGMODE_SET_DIGITS_MIN && g_debug_mode < DEBUGMODE_SET_DIGITS_MAX)
	{
		digits =  g_debug_mode - DEBUGMODE_SET_DIGITS_MIN;
	}
	while (diff < 1.0 && digits <= DBL_DIG + 1)
	{
		diff *= 10;
		++digits;
	}
	return digits;
}

/* This function calculates the precision needed to distiguish adjacent
	pixels at Fractint's maximum resolution of MAX_PIXELS by MAX_PIXELS
	(if rez == MAXREZ) or at current resolution (if rez == CURRENTREZ)    */
int get_precision_bf(int rezflag)
{
	big_t del1;
	big_t del2;
	big_t one;
	big_t bfxxdel;
	big_t bfxxdel2;
	big_t bfyydel;
	big_t bfyydel2;
	int digits;
	int dec;
	int saved;
	int rez;
	saved    = save_stack();
	del1     = alloc_stack(g_bf_length + 2);
	del2     = alloc_stack(g_bf_length + 2);
	one      = alloc_stack(g_bf_length + 2);
	bfxxdel   = alloc_stack(g_bf_length + 2);
	bfxxdel2  = alloc_stack(g_bf_length + 2);
	bfyydel   = alloc_stack(g_bf_length + 2);
	bfyydel2  = alloc_stack(g_bf_length + 2);
	floattobf(one, 1.0);
	rez = (rezflag == MAXREZ) ? (OLD_MAX_PIXELS - 1) : (g_x_dots - 1);

	/* bfxxdel = (bfxmax - bfx3rd)/(g_x_dots-1) */
	sub_bf(bfxxdel, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_3rd());
	div_a_bf_int(bfxxdel, (U16)rez);

	/* bfyydel2 = (bfy3rd - bfymin)/(g_x_dots-1) */
	sub_bf(bfyydel2, g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min());
	div_a_bf_int(bfyydel2, (U16)rez);

	if (rezflag == CURRENTREZ)
	{
		rez = g_y_dots-1;
	}

	/* bfyydel = (bfymax - bfy3rd)/(g_y_dots-1) */
	sub_bf(bfyydel, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
	div_a_bf_int(bfyydel, (U16)rez);

	/* bfxxdel2 = (bfx3rd - bfxmin)/(g_y_dots-1) */
	sub_bf(bfxxdel2, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
	div_a_bf_int(bfxxdel2, (U16)rez);

	abs_a_bf(add_bf(del1, bfxxdel, bfxxdel2));
	abs_a_bf(add_bf(del2, bfyydel, bfyydel2));
	if (cmp_bf(del2, del1) < 0)
	{
		copy_bf(del1, del2);
	}
	if (cmp_bf(del1, clear_bf(del2)) == 0)
	{
		restore_stack(saved);
		return -1;
	}
	digits = 1;
	while (cmp_bf(del1, one) < 0)
	{
		digits++;
		mult_a_bf_int(del1, 10);
	}
	digits = std::max(digits, 3);
	restore_stack(saved);
	dec = get_precision_mag_bf();
	return std::max(digits, dec);
}

/* This function calculates the precision needed to distiguish adjacent
	pixels at Fractint's maximum resolution of MAX_PIXELS by MAX_PIXELS
	(if rez == MAXREZ) or at current resolution (if rez == CURRENTREZ)    */
int get_precision_dbl(int rezflag)
{
	LDBL del1, del2, xdel, xdel2, ydel, ydel2;
	int digits;
	LDBL rez;

	rez = (rezflag == MAXREZ) ? (OLD_MAX_PIXELS -1) : (g_x_dots-1);

	xdel =  ((LDBL)g_escape_time_state.m_grid_fp.x_max() - (LDBL)g_escape_time_state.m_grid_fp.x_3rd())/rez;
	ydel2 = ((LDBL)g_escape_time_state.m_grid_fp.y_3rd() - (LDBL)g_escape_time_state.m_grid_fp.y_min())/rez;

	if (rezflag == CURRENTREZ)
	{
		rez = g_y_dots-1;
	}

	ydel = ((LDBL)g_escape_time_state.m_grid_fp.y_max() - (LDBL)g_escape_time_state.m_grid_fp.y_3rd())/rez;
	xdel2 = ((LDBL)g_escape_time_state.m_grid_fp.x_3rd() - (LDBL)g_escape_time_state.m_grid_fp.x_min())/rez;

	del1 = fabsl(xdel) + fabsl(xdel2);
	del2 = fabsl(ydel) + fabsl(ydel2);
	if (del2 < del1)
	{
		del1 = del2;
	}
	if (del1 == 0)
	{
#ifdef DEBUG
		show_corners_dbl("get_precision_dbl");
#endif
		return -1;
	}
	digits = 1;
	while (del1 < 1.0)
	{
		digits++;
		del1 *= 10;
	}
	digits = std::max(digits, 3);
	return digits;
}

/*
	Strips zeros from the non-exponent part of a number. This logic
	was originally in put_bf(), but is split into this routine so it can be
	shared with put_float(), which had a bug in Fractint 19.2 (used to strip
	zeros from the exponent as well.)
*/

static void strip_zeros(char *buf)
{
	char *dptr;
	char *bptr;
	char *exptr;
	strlwr(buf);
	dptr = strchr(buf, '.');
	if (dptr != 0)
	{
		++dptr;
		exptr = strchr(buf, 'e');
		/* scientific notation with 'e'? */
		bptr = (exptr != 0) ? exptr : (buf + strlen(buf));
		while (--bptr > dptr && *bptr == '0')
		{
			*bptr = 0;
		}
		if (exptr && bptr < exptr -1)
		{
			strcat(buf, exptr);
		}
	}
}

static void put_float(int slash, double fnum, int prec)
{
	char buf[40];
	char *bptr = buf;
	if (slash)
	{
		*(bptr++) = '/';
	}
#ifdef USE_LONG_DOUBLE
	/* Idea of long double cast is to squeeze out another digit or two
		which might be needed (we have found cases where this digit makes
		a difference.) But lets not do this at lower precision */
	if (prec > 15)
	{
		sprintf(bptr, "%1.*Lg", prec, (long double) fnum);
	}
	else
#endif
	{
		sprintf(bptr, "%1.*g", prec, double(fnum));
	}
	strip_zeros(bptr);
	put_parm(buf);
}

static void put_bf(int slash, bf_t r, int prec)
{
	char *buf; /* "/-1.xxxxxxE-1234" */
	char *bptr;
	buf = s_wbdata.buf + 5000;  /* end of use g_suffix buffer, 5000 bytes safe */
	bptr = buf;
	if (slash)
	{
		*(bptr++) = '/';
	}
	bftostr(bptr, prec, r);
	strip_zeros(bptr);
	put_parm(buf);
}

void edit_text_colors()
{
	int save_debugflag = g_debug_mode;
	g_debug_mode = 0;	 /*	don't get called recursively */
	MouseModeSaver saved_mouse(LOOK_MOUSE_TEXT); /* text mouse sensitivity */
	int row = 0;
	int col = 0;
	int background = 0;
	int rowt = 0;
	int rowf = 0;
	int colt = 0;
	int colf = 0;

	while (true)
	{
		row = MathUtil::Clamp(row, 0, 24);
		col = MathUtil::Clamp(col, 0, 79);
		driver_move_cursor(row, col);

		int key = toupper(driver_get_key());
		switch (key)
		{
		case IDK_ESC:
			g_debug_mode = save_debugflag;
			driver_hide_text_cursor();
			return;

		case '/':
			driver_hide_text_cursor();
			{
				ScreenStacker stacker;
				for (int i = 0; i < 8; ++i)		  /* 8 bkgrd attrs */
				{
					for (int j = 0; j < 16; ++j) /*	16 fgrd	attrs */
					{
#if defined(_WIN32)
						_ASSERTE(_CrtCheckMemory());
#endif
						int k = (i*16 + j);
						driver_put_char_attr_rowcol(i*2, j*5, (' ' << 8) | k);
						driver_put_char_attr_rowcol(i*2, j*5 + 1, ((i + '0') << 8)| k);
						driver_put_char_attr_rowcol(i*2, j*5 + 2, (((j < 10) ? j + '0' : j + 'A'-10) << 8) | k);
						driver_put_char_attr_rowcol(i*2, j*5 + 3, (' ' << 8) | k);
					}
				}
				driver_get_key();
			}
			driver_move_cursor(row, col);
			break;
		case ',':
			rowf = row;
			colf = col;
			break;
		case '.':
			rowt = row;
			colt = col;
			break;
		case ' ': /* next color is background */
			background = 1;
			break;
		case IDK_LEFT_ARROW: /* cursor left  */
			--col;
			break;
		case IDK_RIGHT_ARROW: /* cursor right */
			++col;
			break;
		case IDK_UP_ARROW:	/* cursor up	*/
			--row;
			break;
		case IDK_DOWN_ARROW: /* cursor down  */
			++row;
			break;
		case IDK_ENTER:   /* enter */
			{
				int char_attr = driver_get_char_attr_rowcol(row, col);
				char_attr &= ~0xFF;
				char_attr |= driver_get_key() & 0xFF;
				driver_put_char_attr_rowcol(row, col, char_attr);
			}
			break;
		default:
			if (key >= '0' && key <= '9')
			{
				key -= '0';
			}
			else if (key >= 'A' && key <= 'F')
			{
				key -= 'A' - 10;
			}
			else
			{
				break;
			}
			for (int j = rowf; j <= rowt; ++j)
			{
				for (int k = colf; k <= colt; ++k)
				{
					int char_attr = driver_get_char_attr_rowcol(j, k);
					if (background)
					{
						char_attr &= ~0xF0;
						char_attr |= (key & 0xF) << 4;
					}
					else
					{
						char_attr &= ~0xF;
						char_attr |= key & 0xF;
					}
					driver_put_char_attr_rowcol(j, k, char_attr);
				}
			}
			background = 0;
		}
	}
}

static const int *s_entries;

int select_video_mode(int curmode)
{
	int entries[MAXVIDEOMODES];
	int attributes[MAXVIDEOMODES];
	int i;
	int k;
	int ret;

	for (i = 0; i < g_.VideoTableLength(); ++i)  /* init tables */
	{
		entries[i] = i;
		attributes[i] = 1;
	}
	s_entries = entries;           /* for indirectly called subroutines */

	qsort(entries, g_.VideoTableLength(), sizeof(entries[0]), entcompare); /* sort modes */

	/* pick default mode */
	if (curmode < 0)
	{
		g_.SetVideoEntryColors(256);
	}
	else
	{
		g_.SetVideoEntry(curmode);
	}

	for (i = 0; i < g_.VideoTableLength(); ++i)  /* find default mode */
	{
		if (g_.VideoEntry().colors == g_.VideoTable(entries[i]).colors &&
			(curmode < 0 || g_.VideoEntry() == g_.VideoTable(entries[i])))
		{
			break;
		}
	}
	if (i >= g_.VideoTableLength()) /* no match, default to first entry */
	{
		i = 0;
	}

	bool save_tab_display_enabled = g_tab_display_enabled;
	g_tab_display_enabled = false;
	i = full_screen_choice_help(FIHELP_VIDEO_MODE, CHOICE_HELP,
		"Select Video Mode",
		"key...name.......................xdot..ydot.colr.driver......comment......",
		0, g_.VideoTableLength(), 0, attributes,
		1, 16, 74, i, format_vid_table, 0, 0, check_modekey);
	g_tab_display_enabled = save_tab_display_enabled;
	if (i == -1)
	{
		return -1;
	}
	/* picked by function key or ENTER key */
	i = (i < 0) ? (-1 - i) : entries[i];

	/* the selected entry now in g_video_entry */
	g_.SetVideoEntry(i);

	/* copy fractint.cfg table to resident table, note selected entry */
	k = 0;
	for (i = 0; i < g_.VideoTableLength(); ++i)
	{
		if (g_.VideoTable(i).keynum > 0)
		{
			if (g_.VideoEntry() == g_.VideoTable(i))
			{
				k = g_.VideoTable(i).keynum;
			}
		}
	}
	ret = k;
	if (k == 0)  /* selected entry not a copied (assigned to key) one */
	{
		g_.SetVideoTable(MAXVIDEOMODES-1, g_.VideoEntry());
		ret = 1400; /* special value for check_video_mode_key */
	}

	return ret;
}

void format_vid_table(int choice, char *buf)
{
	g_.SetVideoEntry(s_entries[choice]);
	std::string kname = video_mode_key_name(g_.VideoEntry().keynum);
	strcpy(buf, boost::format("%-5s %-25s %5d %5d %3d %.12s %.12s")
		% kname % g_.VideoEntry().name
		% g_.VideoEntry().x_dots % g_.VideoEntry().y_dots % g_.VideoEntry().colors
		% g_.VideoEntry().driver->name() % g_.VideoEntry().comment);
}

static int check_modekey(int curkey, int choice)
{
	int i = check_video_mode_key(curkey);
	if (i >= 0)
	{
		return -1-i;
	}
	int entry = s_entries[choice];
	int ret = 0;
	if (g_.VideoTable(entry).keynum == 0 || g_.VideoTable(entry).keynum >= IDK_SF1)
	{
		if (curkey == '-') /* deassign key? */ 
		{
			if (g_.VideoTable(entry).keynum >= IDK_SF1)
			{
				g_.SetVideoTableKey(entry, 0);
			}
		}
		else if (curkey == '+') /* assign key? */
		{
			int key = get_key_no_help();
			if (key >= IDK_SF1 && key <= IDK_ALT_F10)
			{
				for (int k = 0; k < g_.VideoTableLength(); ++k)
				{
					if (g_.VideoTable(k).keynum == key)
					{
						g_.SetVideoTableKey(k, 0);
						ret = -1;
						/* force redisplay */
					}
				}
				g_.SetVideoTableKey(entry, key);
			}
		}
	}
	return ret;
}

static int entcompare(const void *p1, const void *p2)
{
	int i;
	int j;
	i = g_.VideoTable(*((int *)p1)).keynum;
	if (i == 0)
	{
		i = 9999;
	}
	j = g_.VideoTable(*((int *)p2)).keynum;
	if (j == 0)
	{
		j = 9999;
	}
	if (i < j || (i == j && *((int *)p1) < *((int *)p2)))
	{
		return -1;
	}
	return 1;
}

/* make_mig() takes a collection of individual GIF images (all
	presumably the same resolution and all presumably generated
	by Fractint and its "divide and conquer" algorithm) and builds
	a single multiple-image GIF out of them.  This routine is
	invoked by the "batch=stitchmode/x/y" option, and is called
	with the 'x' and 'y' parameters
*/
class MakeMIG
{
public:
	MakeMIG(unsigned int xmult, unsigned int ymult)
		: m_xmult(xmult), m_ymult(ymult)
	{
	}
	~MakeMIG()
	{
	}

	void execute();

private:
	void process_input_image(unsigned int y_step, unsigned int x_step);
	void extension_block(unsigned int y_step, unsigned int x_step);
	void image_descriptor_block(unsigned int y_step, unsigned int x_step);
	void read_block_header();
	void display_error_messages();
	void delete_input_files();
	void set_gif_input_filename(unsigned int y_step, unsigned int x_step);

	void finish_output_file();
	void write_image_header();
	void read_header();
	bool find_color_table_size(unsigned int y_step, unsigned int x_step);

	unsigned int m_xmult;
	unsigned int m_ymult;
	int m_error_flag;
	int m_input_error_flag;
	unsigned int m_all_x_res;
	unsigned int m_all_y_res;
	unsigned int m_all_i_table;
	FILE *m_out;
	FILE *m_in;
	unsigned char m_buffer[4096];
	std::string m_gif_output_filename;
	std::string m_gif_input_filename;
	unsigned int m_x_res;
	unsigned int m_y_res;
	unsigned int m_input_color_table_size;
};

void make_mig(unsigned int xmult, unsigned int ymult)
{
	MakeMIG(xmult, ymult).execute();
}

bool MakeMIG::find_color_table_size(unsigned int y_step, unsigned int x_step)
{
	unsigned char input_char = char(m_buffer[10] & 0x07);

	/* find the color table size */
	m_input_color_table_size = 1 << (++input_char);
	input_char = char(m_buffer[10] & 0x80);        /* is there a global color table? */
	if (x_step == 0 && y_step == 0)   /* first time through? */
	{
		m_all_i_table = m_input_color_table_size;             /* save the color table size */
	}
	if (input_char != 0)                /* yup */
	{
		/* (read, but only copy this if it's the first time through) */
		if (fread(m_buffer, 3*m_input_color_table_size, 1, m_in) != 1)    /* read the global color table */
		{
			m_input_error_flag = 2;
		}
		if (x_step == 0 && y_step == 0)       /* first time through? */
		{
			if (fwrite(m_buffer, 3*m_input_color_table_size, 1, m_out) != 1)     /* write out the GCT */
			{
				m_error_flag = 2;
			}
		}
	}
	return (m_x_res != m_all_x_res || m_y_res != m_all_y_res || m_input_color_table_size != m_all_i_table);
}

void MakeMIG::read_header()
{
	/* (read, but only copy this if it's the first time through) */
	if (fread(m_buffer, 13, 1, m_in) != 1)   /* read the header and LDS */
	{
		m_input_error_flag = 1;
	}
	// TODO: byte order dependent
	memcpy(&m_x_res, &m_buffer[6], 2);     /* X-resolution */
	memcpy(&m_y_res, &m_buffer[8], 2);     /* Y-resolution */
}
void MakeMIG::write_image_header()
{
	m_all_x_res = m_x_res;             /* save the "master" resolution */
	m_all_y_res = m_y_res;
	unsigned int x_total = m_x_res*m_xmult;
	unsigned int y_total = m_y_res*m_ymult;        /* adjust the image size */
	// TODO: byte ordering
	memcpy(&m_buffer[6], &x_total, 2);
	memcpy(&m_buffer[8], &y_total, 2);
	if (g_gif87a_flag)
	{
		m_buffer[3] = '8';
		m_buffer[4] = '7';
		m_buffer[5] = 'a';
	}
	m_buffer[12] = 0; /* reserved */
	if (fwrite(m_buffer, 13, 1, m_out) != 1)     /* write out the header */
	{
		m_error_flag = 1;
	}
}
void MakeMIG::finish_output_file()
{
	m_buffer[0] = 0x3b;                 /* end-of-stream indicator */
	if (fwrite(m_buffer, 1, 1, m_out) != 1)
	{
		m_error_flag = 12;
	}
	fclose(m_out);                    /* done with the output GIF */
}

void MakeMIG::set_gif_input_filename(unsigned int y_step, unsigned int x_step)
{
	m_gif_input_filename = str(boost::format("frmig_%c%c.gif") % par_key(x_step) % par_key(y_step));
}

void MakeMIG::delete_input_files()
{
	if (m_error_flag == 0 && m_input_error_flag == 0)
	{
		for (unsigned int y_step = 0; y_step < m_ymult; y_step++)
		{
			for (unsigned int x_step = 0; x_step < m_xmult; x_step++)
			{
				set_gif_input_filename(y_step, x_step);
				remove(m_gif_input_filename.c_str());
			}
		}
		stop_message(STOPMSG_NORMAL,
			"File " + m_gif_output_filename + " has been created (and its component files deleted)");
	}
}

void MakeMIG::display_error_messages()
{
	if (m_input_error_flag != 0)       /* uh-oh - something failed */
	{
		stop_message(STOPMSG_NORMAL, "Process failed = early EOF on input file " + m_gif_input_filename);
	}

	if (m_error_flag != 0)            /* uh-oh - something failed */
	{
		stop_message(STOPMSG_NORMAL, "Process failed = out of disk space?");
	}
}
void MakeMIG::read_block_header()
{
	memset(m_buffer, 0, 10);
	if (fread(m_buffer, 1, 1, m_in) != 1)    /* read the block identifier */
	{
		m_input_error_flag = 3;
	}
}
void MakeMIG::image_descriptor_block(unsigned int y_step, unsigned int x_step)
{
	if (fread(&m_buffer[1], 9, 1, m_in) != 1)    /* read the Image Descriptor */
	{
		m_input_error_flag = 4;
	}
	unsigned int xloc;
	// TODO: byte order dependent
	memcpy(&xloc, &m_buffer[1], 2); /* X-location */
	unsigned int yloc;
	// TODO: byte order dependent
	memcpy(&yloc, &m_buffer[3], 2); /* Y-location */
	xloc += (x_step*m_x_res);     /* adjust the locations */
	yloc += (y_step*m_y_res);
	// TODO: byte order dependent
	memcpy(&m_buffer[1], &xloc, 2);
	memcpy(&m_buffer[3], &yloc, 2);
	if (fwrite(m_buffer, 10, 1, m_out) != 1)     /* write out the Image Descriptor */
	{
		m_error_flag = 4;
	}

	unsigned char input_char = char(m_buffer[9] & 0x80);     /* is there a local color table? */
	if (input_char != 0)            /* yup */
	{
		if (fread(m_buffer, 3*m_input_color_table_size, 1, m_in) != 1)       /* read the local color table */
		{
			m_input_error_flag = 5;
		}
		if (fwrite(m_buffer, 3*m_input_color_table_size, 1, m_out) != 1)     /* write out the LCT */
		{
			m_error_flag = 5;
		}
	}

	if (fread(m_buffer, 1, 1, m_in) != 1)        /* LZH table size */
	{
		m_input_error_flag = 6;
	}
	if (fwrite(m_buffer, 1, 1, m_out) != 1)
	{
		m_error_flag = 6;
	}
	while (true)
	{
		if (m_error_flag != 0 || m_input_error_flag != 0)      /* oops - did something go wrong? */
		{
			break;
		}
		if (fread(m_buffer, 1, 1, m_in) != 1)    /* block size */
		{
			m_input_error_flag = 7;
		}
		if (fwrite(m_buffer, 1, 1, m_out) != 1)
		{
			m_error_flag = 7;
		}
		unsigned char input_char = m_buffer[0];
		if (input_char == 0)
		{
			break;
		}
		if (fread(m_buffer, input_char, 1, m_in) != 1)    /* LZH data block */
		{
			m_input_error_flag = 8;
		}
		if (fwrite(m_buffer, input_char, 1, m_out) != 1)
		{
			m_error_flag = 8;
		}
	}
}
void MakeMIG::extension_block(unsigned int y_step, unsigned int x_step)
{
	/* (read, but only copy this if it's the last time through) */
	if (fread(&m_buffer[2], 1, 1, m_in) != 1)    /* read the block type */
	{
		m_input_error_flag = 9;
	}
	if (!g_gif87a_flag && (x_step == m_xmult-1) && (y_step == m_ymult-1))
	{
		if (fwrite(m_buffer, 2, 1, m_out) != 1)
		{
			m_error_flag = 9;
		}
	}
	while (true)
	{
		if (m_error_flag != 0 || m_input_error_flag != 0)      /* oops - did something go wrong? */
		{
			break;
		}
		if (fread(m_buffer, 1, 1, m_in) != 1)    /* block size */
		{
			m_input_error_flag = 10;
		}
		if ((!g_gif87a_flag) && x_step == m_xmult-1 && y_step == m_ymult-1)
		{
			if (fwrite(m_buffer, 1, 1, m_out) != 1)
			{
				m_error_flag = 10;
			}
		}
		unsigned int input_char = m_buffer[0];
		if (input_char == 0)
		{
			break;
		}
		if (fread(m_buffer, input_char, 1, m_in) != 1)    /* data block */
		{
			m_input_error_flag = 11;
		}
		if (!g_gif87a_flag && (x_step == m_xmult-1) && (y_step == m_ymult-1))
		{
			if (fwrite(m_buffer, input_char, 1, m_out) != 1)
			{
				m_error_flag = 11;
			}
		}
	}
}
void MakeMIG::process_input_image(unsigned int y_step, unsigned int x_step)
{
	while (true)                       /* process each information block */
	{
		read_block_header();
		if (m_buffer[0] == 0x2c)           /* image descriptor block */
		{
			image_descriptor_block(y_step, x_step);
		}

		if (m_buffer[0] == 0x21)           /* extension block */
		{
			extension_block(y_step, x_step);
		}

		if (m_buffer[0] == 0x3b)           /* end-of-stream indicator */
		{
			break;                      /* done with this file */
		}

		if (m_error_flag != 0 || m_input_error_flag != 0)      /* oops - did something go wrong? */
		{
			break;
		}
	}
	fclose(m_in);                     /* done with an input GIF */
}
void MakeMIG::execute()
{
	m_error_flag = 0;
	m_input_error_flag = 0;
	m_all_x_res = 0;
	m_all_y_res = 0;
	m_all_i_table = 0;
	m_out = 0;
	m_in = 0;

	m_gif_output_filename = "fractmig.gif";

	g_gif87a_flag = true;                        /* for now, force this */

	/* process each input image, one at a time */
	for (unsigned int y_step = 0; y_step < m_ymult; y_step++)
	{
		for (unsigned int x_step = 0; x_step < m_xmult; x_step++)
		{
			if (x_step == 0 && y_step == 0)          /* first time through? */
			{
				stop_message(STOPMSG_NORMAL,
					str(boost::format("Generating multi-image GIF file %s\n"
						" using %d X and %d Y components") % m_gif_output_filename % m_xmult % m_ymult));
				/* attempt to create the output file */
				m_out = fopen(m_gif_output_filename.c_str(), "wb");
				if (m_out == 0)
				{
					stop_message(STOPMSG_NORMAL,
						"Cannot create output file " + m_gif_output_filename + "!\n");
					return;
				}
			}

			set_gif_input_filename(x_step, y_step);

			m_in = fopen(m_gif_input_filename.c_str(), "rb");
			if (m_in == 0)
			{
				stop_message(STOPMSG_NORMAL, "Can't open file " + m_gif_input_filename + "!");
				return;
			}

			read_header();
			if (x_step == 0 && y_step == 0)
			{
				write_image_header();
			}

			if (find_color_table_size(y_step, x_step))
			{
				/* Oops - our pieces don't match */
				stop_message(STOPMSG_NORMAL,
					"File " + m_gif_input_filename + " doesn't have the same resolution as its predecessors!");
				return;
			}

			process_input_image(y_step, x_step);
			if (m_error_flag != 0 || m_input_error_flag != 0)      /* oops - did something go wrong? */
			{
				break;
			}
		}

		if (m_error_flag != 0 || m_input_error_flag != 0)  /* oops - did something go wrong? */
		{
			break;
		}
	}

	finish_output_file();
	display_error_messages();
	delete_input_files();
}

static void reverse_x_axis()
{
	for (int i = 0; i < g_x_dots/2; i++)
	{
		if (driver_key_pressed())
		{
			break;
		}
		for (int j = 0; j < g_y_dots; j++)
		{
			int temp = get_color(i, j);
			g_plot_color_put_color(i, j, get_color(g_x_dots-1-i, j));
			g_plot_color_put_color(g_x_dots-1-i, j, temp);
		}
	}
	g_sx_min = g_escape_time_state.m_grid_fp.x_max() + g_escape_time_state.m_grid_fp.x_min() - g_escape_time_state.m_grid_fp.x_3rd();
	g_sy_max = g_escape_time_state.m_grid_fp.y_max() + g_escape_time_state.m_grid_fp.y_min() - g_escape_time_state.m_grid_fp.y_3rd();
	g_sx_max = g_escape_time_state.m_grid_fp.x_3rd();
	g_sy_min = g_escape_time_state.m_grid_fp.y_3rd();
	g_sx_3rd = g_escape_time_state.m_grid_fp.x_max();
	g_sy_3rd = g_escape_time_state.m_grid_fp.y_min();
	if (g_bf_math)
	{
		add_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min()); /* g_sx_min = g_xx_max + g_xx_min - g_xx_3rd; */
		sub_a_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_3rd());
		add_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_min()); /* g_sy_max = g_yy_max + g_yy_min - g_yy_3rd; */
		sub_a_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_3rd());
		copy_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_3rd());        /* g_sx_max = g_xx_3rd; */
		copy_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_3rd());        /* g_sy_min = g_yy_3rd; */
		copy_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_max());        /* g_sx_3rd = g_xx_max; */
		copy_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_min());        /* g_sy_3rd = g_yy_min; */
	}
}

static void reverse_y_axis()
{
	for (int j = 0; j < g_y_dots/2; j++)
	{
		if (driver_key_pressed())
		{
			break;
		}
		for (int i = 0; i < g_x_dots; i++)
		{
			int temp = get_color(i, j);
			g_plot_color_put_color(i, j, get_color(i, g_y_dots-1-j));
			g_plot_color_put_color(i, g_y_dots-1-j, temp);
		}
	}
	g_sx_min = g_escape_time_state.m_grid_fp.x_3rd();
	g_sy_max = g_escape_time_state.m_grid_fp.y_3rd();
	g_sx_max = g_escape_time_state.m_grid_fp.x_max() + g_escape_time_state.m_grid_fp.x_min() - g_escape_time_state.m_grid_fp.x_3rd();
	g_sy_min = g_escape_time_state.m_grid_fp.y_max() + g_escape_time_state.m_grid_fp.y_min() - g_escape_time_state.m_grid_fp.y_3rd();
	g_sx_3rd = g_escape_time_state.m_grid_fp.x_min();
	g_sy_3rd = g_escape_time_state.m_grid_fp.y_max();
	if (g_bf_math)
	{
		copy_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_3rd());        /* g_sx_min = g_xx_3rd; */
		copy_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_3rd());        /* g_sy_max = g_yy_3rd; */
		add_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min()); /* g_sx_max = g_xx_max + g_xx_min - g_xx_3rd; */
		sub_a_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_3rd());
		add_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_min()); /* g_sy_min = g_yy_max + g_yy_min - g_yy_3rd; */
		sub_a_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_3rd());
		copy_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_min());        /* g_sx_3rd = g_xx_min; */
		copy_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_max());        /* g_sy_3rd = g_yy_max; */
	}
}

static void reverse_x_y_axes()
{
	for (int i = 0; i < g_x_dots/2; i++)
	{
		if (driver_key_pressed())
		{
			break;
		}
		for (int j = 0; j < g_y_dots; j++)
		{
			int temp = get_color(i, j);
			g_plot_color_put_color(i, j, get_color(g_x_dots-1-i, g_y_dots-1-j));
			g_plot_color_put_color(g_x_dots-1-i, g_y_dots-1-j, temp);
		}
	}
	g_sx_min = g_escape_time_state.m_grid_fp.x_max();
	g_sy_max = g_escape_time_state.m_grid_fp.y_min();
	g_sx_max = g_escape_time_state.m_grid_fp.x_min();
	g_sy_min = g_escape_time_state.m_grid_fp.y_max();
	g_sx_3rd = g_escape_time_state.m_grid_fp.x_max() + g_escape_time_state.m_grid_fp.x_min() - g_escape_time_state.m_grid_fp.x_3rd();
	g_sy_3rd = g_escape_time_state.m_grid_fp.y_max() + g_escape_time_state.m_grid_fp.y_min() - g_escape_time_state.m_grid_fp.y_3rd();
	if (g_bf_math)
	{
		copy_bf(g_sx_min_bf, g_escape_time_state.m_grid_bf.x_max());        /* g_sx_min = g_xx_max; */
		copy_bf(g_sy_max_bf, g_escape_time_state.m_grid_bf.y_min());        /* g_sy_max = g_yy_min; */
		copy_bf(g_sx_max_bf, g_escape_time_state.m_grid_bf.x_min());        /* g_sx_max = g_xx_min; */
		copy_bf(g_sy_min_bf, g_escape_time_state.m_grid_bf.y_max());        /* g_sy_min = g_yy_max; */
		add_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min()); /* g_sx_3rd = g_xx_max + g_xx_min - g_xx_3rd; */
		sub_a_bf(g_sx_3rd_bf, g_escape_time_state.m_grid_bf.x_3rd());
		add_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_min()); /* g_sy_3rd = g_yy_max + g_yy_min - g_yy_3rd; */
		sub_a_bf(g_sy_3rd_bf, g_escape_time_state.m_grid_bf.y_3rd());
	}
}
/* This routine copies the current screen to by flipping x-axis, y-axis,
	or both. Refuses to work if calculation in progress or if fractal
	non-resumable. Clears zoombox if any. Resets corners so resulting fractal
	is still valid. */
void flip_image(int key)
{
	/* fractal must be rotate-able and be finished */
	if (g_current_fractal_specific->no_zoom_box_rotate()
		|| g_calculation_status == CALCSTAT_IN_PROGRESS
		|| g_calculation_status == CALCSTAT_RESUMABLE)
	{
		return;
	}

	if (g_bf_math)
	{
		clear_zoom_box(); /* clear, don't copy, the zoombox */
	}
	switch (key)
	{
	case IDK_CTL_X:            /* control-X - reverse X-axis */
		reverse_x_axis();
		break;
	case IDK_CTL_Y:            /* control-Y - reverse Y-axis */
		reverse_y_axis();
		break;
	case IDK_CTL_Z:            /* control-Z - reverse X and Y axis */
		reverse_x_y_axes();
		break;
	}
	reset_zoom_corners();
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}

static std::string expand_var(const std::string &var)
{
	time_t ltime;
	time(&ltime);
	char *time_text = ctime(&ltime);

	/* ctime format             */
	/* Sat Aug 17 21:34:14 1996 */
	/* 012345678901234567890123 */
	/*           1         2    */
	std::string out;
	if (var == "year")       /* 4 chars */
	{
		time_text[24] = 0;
		out = &time_text[20];
	}
	else if (var == "month") /* 3 chars */
	{
		time_text[7] = 0;
		out = &time_text[4];
	}
	else if (var == "day")   /* 2 chars */
	{
		time_text[10] = 0;
		out = &time_text[8];
	}
	else if (var == "hour")  /* 2 chars */
	{
		time_text[13] = 0;
		out = &time_text[11];
	}
	else if (var == "min")   /* 2 chars */
	{
		time_text[16] = 0;
		out = &time_text[14];
	}
	else if (var == "sec")   /* 2 chars */
	{
		time_text[19] = 0;
		out = &time_text[17];
	}
	else if (var == "time")  /* 8 chars */
	{
		time_text[19] = 0;
		out = &time_text[11];
	}
	else if (var == "date")
	{
		time_text[10] = 0;
		time_text[24] = 0;
		out = &time_text[4];
		out += ", ";
		out += &time_text[20];
	}
	else if (var == "calctime")
	{
		char buf[100];
		get_calculation_time(buf, g_calculation_time);
		out = buf;
	}
	else if (var == "version")  /* 4 chars */
	{
		out = str(boost::format("%d") % g_release);
	}
	else if (var == "patch")   /* 1 or 2 chars */
	{
		out = str(boost::format("%d") % g_patch_level);
	}
	else if (var == "xdots")   /* 2 to 4 chars */
	{
		out = str(boost::format("%d") % g_x_dots);
	}
	else if (var == "ydots")   /* 2 to 4 chars */
	{
		out = str(boost::format("%d") % g_y_dots);
	}
	else if (var == "vidkey")   /* 2 to 3 chars */
	{
		out = video_mode_key_name(g_.VideoEntry().keynum);
	}
	else
	{
		stop_message(STOPMSG_NORMAL, "Unknown comment variable " + var);
		out = "";
	}
	return out;
}

#define MAXVNAME  13

static const char esc_char = '$';

/* extract comments from the comments= command */
void expand_comments(char *target, const char *source)
{
	int i;
	int j;
	int k;
	char c;
	char oldc;
	char varname[MAXVNAME];
	i = 0;
	j = 0;
	k = 0;
	c = 0;
	oldc = 0;
	bool escape = false;
	while (i < MAX_COMMENT && j < MAX_COMMENT && (c = *(source + i++)) != 0)
	{
		if (c == '\\' && oldc != '\\')
		{
			oldc = c;
			continue;
		}
		/* expand underscores to blanks */
		if (c == '_' && oldc != '\\')
		{
			c = ' ';
		}
		/* esc_char marks start and end of variable names */
		if (c == esc_char && oldc != '\\')
		{
			escape = !escape;
		}
		if (c != esc_char && escape) /* if true, building variable name */
		{
			if (k < MAXVNAME-1)
			{
				varname[k++] = c;
			}
		}
		/* got variable name */
		else if (c == esc_char && !escape && oldc != '\\')
		{
			varname[k] = 0;
			std::string varstr = expand_var(varname);
			strncpy(target + j, varstr.c_str(), MAX_COMMENT-j-1);
			j += int(varstr.length());
		}
		else if (c == esc_char && escape && oldc != '\\')
		{
			k = 0;
		}
		else if ((c != esc_char || oldc == '\\') && !escape)
		{
			*(target + j++) = c;
		}
		oldc = c;
	}
	if (*source != 0)
	{
		*(target + std::min(j, MAX_COMMENT-1)) = 0;
	}
}

void expand_comments(std::string &dest, const char *source)
{
	char buffer[MAX_COMMENT];
	strcpy(buffer, dest.c_str());
	expand_comments(buffer, source);
	dest = buffer;
}

/* extract comments from the comments= command */
void parse_comments(char *value)
{
	int i;
	char *next;
	char save;
	for (i = 0; i < 4; i++)
	{
		save = 0;
		if (*value == 0)
		{
			break;
		}
		next = strchr(value, '/');
		if (*value != '/')
		{
			if (next != 0)
			{
				save = *next;
				*next = 0;
			}
			strncpy(par_comment[i], value, MAX_COMMENT);
		}
		if (next == 0)
		{
			break;
		}
		if (save != 0)
		{
			*next = save;
		}
		value = next + 1;
	}
}

void init_comments()
{
	int i;
	for (i = 0; i < 4; i++)
	{
		par_comment[i][0] = 0;
	}
}
