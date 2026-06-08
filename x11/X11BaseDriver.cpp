// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11BaseDriver.h"

#include <config/port_config.h>

#include <config/cmd_shell.h>
#include <config/path_limits.h>
#include <config/string_case_compare.h>
#include <engine/Browse.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/video_mode.h>
#include <engine/VideoInfo.h>
#include <geometry/plot3d.h>
#include <io/find_path.h>
#include <io/path_match.h>
#include <io/save_timer.h>
#include <io/trim_filename.h>
#include <ui/diskvid.h>
#include <ui/full_screen_choice.h>
#include <ui/id_keys.h>
#include <ui/shell_sort.h>
#include <ui/slideshw.h>
#include <ui/stop_msg.h>
#include <ui/text_screen.h>
#include <ui/video.h>
#include <ui/zoom.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using namespace id::config;
using namespace id::engine;
using namespace id::ui;

namespace id::engine
{
extern int g_and_color;
}

namespace id::misc
{

namespace
{

namespace fs = std::filesystem;

constexpr std::chrono::milliseconds OUTPUT_FLUSH_INTERVAL{100};
constexpr int OUTPUT_FLUSH_CHECK_PIXELS{256};
constexpr int MAX_NUM_FILES{2977};
constexpr const char *NO_FILES{"*nofiles*"};

enum class FilenameSpeedState
{
    MATCHING,
    TEMPLATE,
    SEARCH_PATH
};

FilenameSpeedState g_filename_speed_state{FilenameSpeedState::MATCHING};
std::string g_font_name;
std::string g_geometry;

std::string window_title()
{
    std::string title{ID_PROGRAM_NAME " "        //
        + std::to_string(ID_VERSION_MAJOR) + "." //
        + std::to_string(ID_VERSION_MINOR) + "." //
        + std::to_string(ID_VERSION_PATCH)};
    if (ID_VERSION_TWEAK)
    {
        title += "." + std::to_string(ID_VERSION_TWEAK);
    }
    return title;
}

struct FileChoice
{
    char full_name[id::config::ID_FILE_MAX_PATH]{};
    bool sub_dir{};
    fs::path path;
};

bool has_graphics_mode()
{
    return g_adapter >= 0 && g_adapter < MAX_VIDEO_MODES && g_video_table[g_adapter].x_dots > 0 &&
        g_video_table[g_adapter].y_dots > 0;
}

void remove_args(int *argc, char **argv, const int first, const int count)
{
    for (int i{first}; i + count <= *argc; ++i)
    {
        argv[i] = argv[i + count];
    }
    *argc -= count;
}

std::string consume_geometry_arg(int *argc, char **argv)
{
    std::string geometry;
    for (int i{1}; i < *argc;)
    {
        const std::string_view arg{argv[i]};
        if (arg == "-geometry" || arg == "--geometry" || arg == "-g")
        {
            if (i + 1 < *argc)
            {
                geometry = argv[i + 1];
                remove_args(argc, argv, i, 2);
            }
            else
            {
                remove_args(argc, argv, i, 1);
            }
            continue;
        }

        constexpr std::string_view GEOMETRY_OPTION{"-geometry="};
        constexpr std::string_view GEOMETRY_LONG_OPTION{"--geometry="};
        if (arg.rfind(GEOMETRY_OPTION, 0) == 0)
        {
            geometry = std::string{arg.substr(GEOMETRY_OPTION.size())};
            remove_args(argc, argv, i, 1);
            continue;
        }
        if (arg.rfind(GEOMETRY_LONG_OPTION, 0) == 0)
        {
            geometry = std::string{arg.substr(GEOMETRY_LONG_OPTION.size())};
            remove_args(argc, argv, i, 1);
            continue;
        }
        ++i;
    }
    return geometry;
}

std::string consume_font_arg(int *argc, char **argv)
{
    std::string font_name;
    for (int i{1}; i < *argc;)
    {
        const std::string_view arg{argv[i]};
        if (arg == "-fn" || arg == "--font")
        {
            if (i + 1 < *argc)
            {
                font_name = argv[i + 1];
                remove_args(argc, argv, i, 2);
            }
            else
            {
                remove_args(argc, argv, i, 1);
            }
            continue;
        }

        constexpr std::string_view FONT_LONG_OPTION{"--font="};
        if (arg.rfind(FONT_LONG_OPTION, 0) == 0)
        {
            font_name = std::string{arg.substr(FONT_LONG_OPTION.size())};
            remove_args(argc, argv, i, 1);
            continue;
        }
        ++i;
    }
    return font_name;
}

template <std::size_t N>
void copy_cstr(char (&buffer)[N], const std::string &text)
{
    const std::size_t length{std::min(text.size(), N - 1)};
    std::memcpy(buffer, text.data(), length);
    buffer[length] = '\0';
}

bool has_wildcard(const std::string &filename)
{
    return filename.find_first_of("*?") != std::string::npos;
}

bool has_path_component(const std::string &filename)
{
    return filename.find_first_of("/\\") != std::string::npos;
}

fs::path initial_directory(const std::string &filename)
{
    if (filename.empty())
    {
        return fs::path{"."};
    }

    std::error_code err;
    const fs::path path{filename};
    if (fs::is_directory(path, err))
    {
        return path;
    }
    if (path.has_parent_path())
    {
        return path.parent_path();
    }
    return fs::path{"."};
}

std::string initial_filename(const std::string &filename)
{
    if (filename.empty())
    {
        return {};
    }

    std::error_code err;
    const fs::path path{filename};
    if (fs::is_directory(path, err))
    {
        return {};
    }
    return path.filename().string();
}

std::string initial_pattern(const char *type_wildcard)
{
    return type_wildcard != nullptr && type_wildcard[0] != '\0' ? type_wildcard : "*";
}

void add_file_choice(
    std::vector<FileChoice> &choices, const std::string &name, const fs::path &path, const bool sub_dir)
{
    if (choices.size() >= MAX_NUM_FILES)
    {
        return;
    }

    choices.emplace_back();
    FileChoice &choice{choices.back()};
    copy_cstr(choice.full_name, name);
    choice.sub_dir = sub_dir;
    choice.path = path;
}

void build_file_choices(const fs::path &directory, const std::string &pattern, std::vector<FileChoice> &choices)
{
    choices.clear();
    add_file_choice(choices, "..", directory / "..", true);

    std::error_code err;
    const auto match{id::io::match_fn(pattern)};
    for (fs::directory_iterator it{directory, err}; !err && it != fs::directory_iterator{}; it.increment(err))
    {
        const fs::directory_entry &entry{*it};
        const std::string name{entry.path().filename().string()};
        std::error_code entry_err;
        if (entry.is_directory(entry_err))
        {
            add_file_choice(choices, name + "/", entry.path(), true);
        }
        else if (!entry_err && entry.is_regular_file(entry_err) && match(entry.path().filename()))
        {
            add_file_choice(choices, name, entry.path(), false);
        }
    }

    if (choices.empty())
    {
        add_file_choice(choices, NO_FILES, {}, false);
    }
}

int check_file_browser_key(const int key, const int)
{
    if (key == ID_KEY_F4 || key == ID_KEY_F6)
    {
        return -key;
    }
    return key;
}

int filename_speed_prompt(const int row, const int col, const int vid, const char *speed_string, const int speed_match)
{
    const std::string speed{speed_string};
    const char *prompt{};
    if (has_wildcard(speed) || has_path_component(speed))
    {
        g_filename_speed_state = FilenameSpeedState::TEMPLATE;
        prompt = "File Template";
    }
    else if (speed_match)
    {
        g_filename_speed_state = FilenameSpeedState::SEARCH_PATH;
        prompt = "Search Path for";
    }
    else
    {
        g_filename_speed_state = FilenameSpeedState::MATCHING;
        prompt = "Speed key string";
    }
    driver_put_string(row, col, vid, prompt);
    return static_cast<int>(std::strlen(prompt));
}

std::vector<FileChoice *> sorted_file_choices(std::vector<FileChoice> &storage, const bool sort)
{
    std::vector<FileChoice *> choices;
    choices.reserve(storage.size());
    for (FileChoice &choice : storage)
    {
        choices.push_back(&choice);
    }
    if (sort)
    {
        shell_sort(choices.data(), static_cast<int>(choices.size()), sizeof(FileChoice *));
    }
    return choices;
}

std::string make_file_browser_heading(const char *hdg, const fs::path &directory, const std::string &pattern)
{
    return std::string{hdg} + "\nTemplate: " + id::io::trim_filename(directory / pattern, 66);
}

fs::path typed_template_path(const fs::path &directory, const std::string &speed_string)
{
    const fs::path typed{speed_string};
    return typed.has_parent_path() ? typed : directory / typed;
}

bool choose_typed_template(
    const std::string &speed_string, fs::path &directory, std::string &pattern, std::string &result_filename)
{
    const fs::path typed{typed_template_path(directory, speed_string)};
    const std::string typed_filename{typed.filename().string()};
    if (typed_filename.empty())
    {
        directory = typed.lexically_normal();
        return true;
    }

    if (has_wildcard(typed_filename))
    {
        directory = typed.parent_path().empty() ? fs::path{"."} : typed.parent_path();
        pattern = typed_filename;
        return true;
    }

    std::error_code err;
    if (fs::is_directory(typed, err))
    {
        directory = typed.lexically_normal();
        return true;
    }

    result_filename = typed.lexically_normal().string();
    g_browse.name = typed.filename().string();
    return false;
}

bool choose_search_path(const fs::path &directory, const std::string &speed_string, std::string &result_filename)
{
    const std::string found{id::io::find_path(speed_string.c_str())};
    fs::path selected{found.empty() ? typed_template_path(directory, speed_string) : fs::path{found}};
    selected = selected.lexically_normal();
    result_filename = selected.string();
    g_browse.name = selected.filename().string();
    return false;
}

bool choice_matches_speed(const FileChoice *choice, const std::string &speed_string)
{
    return speed_string.empty() ||
        id::config::string_case_equal(speed_string.c_str(), choice->full_name, speed_string.size());
}

bool get_a_file_name(const char *hdg, const char *type_wildcard, std::string &result_filename)
{
    const std::string old_filename{result_filename};
    fs::path directory{initial_directory(result_filename)};
    std::string selected_name{initial_filename(result_filename)};
    std::string pattern{initial_pattern(type_wildcard)};
    bool do_sort{true};

    for (;;)
    {
        std::vector<FileChoice> storage;
        build_file_choices(directory, pattern, storage);
        std::vector<FileChoice *> choices{sorted_file_choices(storage, do_sort)};
        std::vector<const char *> labels;
        std::vector<int> attributes(choices.size(), 1);
        labels.reserve(choices.size());
        for (const FileChoice *choice : choices)
        {
            labels.push_back(choice->full_name);
        }

        char speed_string[81]{};
        copy_cstr(speed_string, selected_name);
        const std::string heading{make_file_browser_heading(hdg, directory, pattern)};
        const std::string instructions{
            std::string{"Press F4 to toggle sort "} + (do_sort ? "off" : "on") + ", F6 for current directory"};
        g_filename_speed_state = FilenameSpeedState::MATCHING;
        const ChoiceFlags flags{ChoiceFlags::INSTRUCTIONS | (do_sort ? ChoiceFlags::NONE : ChoiceFlags::NOT_SORTED)};
        const int choice{full_screen_choice(flags, heading.c_str(), nullptr, instructions.c_str(),
            static_cast<int>(labels.size()), labels.data(), attributes.data(), 0, 99, 0, 0, nullptr, speed_string,
            filename_speed_prompt, check_file_browser_key)};

        if (choice == -ID_KEY_F4)
        {
            do_sort = !do_sort;
            continue;
        }
        if (choice == -ID_KEY_F6)
        {
            directory = fs::path{"."};
            selected_name.clear();
            continue;
        }
        if (choice < 0)
        {
            result_filename = old_filename;
            return true;
        }

        const std::string speed{speed_string};
        const FileChoice *selected{choices[choice]};
        if (!speed.empty() && g_filename_speed_state == FilenameSpeedState::TEMPLATE)
        {
            if (choose_typed_template(speed, directory, pattern, result_filename))
            {
                selected_name.clear();
                continue;
            }
            return false;
        }
        if (!speed.empty() && g_filename_speed_state == FilenameSpeedState::SEARCH_PATH)
        {
            return choose_search_path(directory, speed, result_filename);
        }
        if (!choice_matches_speed(selected, speed))
        {
            return choose_search_path(directory, speed, result_filename);
        }

        if (std::strcmp(selected->full_name, NO_FILES) == 0)
        {
            selected_name.clear();
            continue;
        }
        if (selected->sub_dir)
        {
            directory = selected->path.lexically_normal();
            selected_name.clear();
            continue;
        }

        const fs::path selected_path{selected->path.lexically_normal()};
        result_filename = selected_path.string();
        g_browse.name = selected_path.filename().string();
        return false;
    }
}

} // namespace

X11BaseDriver::X11BaseDriver(const char *name, const char *description) :
    m_name(name),
    m_description(description)
{
}

bool X11BaseDriver::init(int *argc, char **argv)
{
    if (!m_frame.init(window_title()))
    {
        return false;
    }
    const std::string geometry{consume_geometry_arg(argc, argv)};
    if (!geometry.empty())
    {
        g_geometry = geometry;
    }
    m_frame.set_geometry(g_geometry);
    const std::string font_name{consume_font_arg(argc, argv)};
    if (!font_name.empty())
    {
        g_font_name = font_name;
    }
    m_text.set_font_name(g_font_name);

    m_frame.set_event_handler(
        [this](const XEvent &event)
        {
            m_text.handle_event(event);
            m_plot.handle_event(event);
        });
    return m_text.init(m_frame.connection()) && m_plot.init(m_frame.connection());
}

bool X11BaseDriver::validate_mode(const VideoInfo &mode)
{
    int width{};
    int height{};
    get_max_screen(width, height);
    return is_valid_display_video_mode(mode, width, height);
}

void X11BaseDriver::get_max_screen(int &width, int &height)
{
    m_frame.get_max_screen(width, height);
}

void X11BaseDriver::terminate()
{
    m_saved_screens.clear();
    m_saved_cursor.clear();
    m_saved_text_state.clear();
    m_have_graphics_mode = false;
    m_plot.destroy();
    m_text.destroy();
    m_frame.set_event_handler({});
    m_frame.terminate();
}

void X11BaseDriver::pause()
{
    m_frame.pause();
}

void X11BaseDriver::resume()
{
    if (m_frame.window() == None)
    {
        create_window();
        return;
    }
    m_frame.resume();
    if (m_text_not_graphics)
    {
        set_for_text();
    }
    else
    {
        set_for_graphics();
    }
}

void X11BaseDriver::schedule_alarm(int /*secs*/)
{
}

void X11BaseDriver::create_window()
{
    const WindowLayout layout{get_window_layout()};
    m_frame.create_window(layout.frame_width, layout.frame_height);
    if (m_text.create(m_frame.window(), layout.text_x, layout.text_y))
    {
        m_frame.add_input_window(m_text.window());
    }
    if (m_plot.create(m_frame.window(), layout.plot_width, layout.plot_height))
    {
        m_frame.add_input_window(m_plot.window());
    }
    center_windows(layout);
    if (m_text_not_graphics)
    {
        set_for_text();
    }
    else
    {
        set_for_graphics();
    }
}

bool X11BaseDriver::resize()
{
    const WindowLayout layout{get_window_layout()};
    const bool frame_resized{m_frame.resize(layout.frame_width, layout.frame_height)};
    const bool plot_resized{m_plot.resize(layout.plot_width, layout.plot_height)};
    center_windows(layout);
    return frame_resized || plot_resized;
}

void X11BaseDriver::read_palette()
{
    m_plot.read_palette();
}

void X11BaseDriver::write_palette()
{
    m_plot.write_palette();
}

int X11BaseDriver::read_pixel(const int x, const int y)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return 0;
    }
    return m_plot.read_pixel(x, y);
}

void X11BaseDriver::write_pixel(const int x, const int y, const int color)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return;
    }
    m_plot.resize(g_screen_x_dots, g_screen_y_dots);
    m_plot.write_pixel(x, y, color);
    if (--m_output_flush_countdown <= 0)
    {
        m_output_flush_countdown = OUTPUT_FLUSH_CHECK_PIXELS;
        flush_output();
    }
}

void X11BaseDriver::draw_line(const int x1, const int y1, const int x2, const int y2, const int color)
{
    id::geometry::draw_line(x1, y1, x2, y2, color);
}

void X11BaseDriver::draw_xor_line(const int x1, const int y1, const int x2, const int y2)
{
    m_plot.draw_xor_line(x1, y1, x2, y2);
}

void X11BaseDriver::clear_xor_lines()
{
    m_plot.clear_xor_lines();
}

void X11BaseDriver::display_string(const int x, const int y, const int fg, const int bg, const char *text)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return;
    }
    m_plot.resize(g_screen_x_dots, g_screen_y_dots);
    m_plot.display_string(x, y, fg, bg, text);
}

void X11BaseDriver::save_graphics()
{
    m_plot.save_graphics();
}

void X11BaseDriver::restore_graphics()
{
    m_plot.restore_graphics();
}

int X11BaseDriver::get_key()
{
    int ch{};
    do
    {
        if (m_key_buffer)
        {
            ch = m_key_buffer;
            m_key_buffer = 0;
        }
        else
        {
            ch = handle_special_keys(m_frame.get_key_press(true));
        }
    } while (ch == 0);

    return ch;
}

int X11BaseDriver::key_cursor(const int row, const int col)
{
    int result{};
    if (key_pressed())
    {
        result = get_key();
    }
    else
    {
        move_cursor(row, col);
        result = get_key();
        hide_text_cursor();
    }

    return result;
}

int X11BaseDriver::key_pressed()
{
    flush_output();
    if (m_key_buffer)
    {
        return m_key_buffer;
    }
    if (io::auto_save_needed())
    {
        unget_key(ID_KEY_FAKE_AUTOSAVE);
        assert(m_key_buffer == ID_KEY_FAKE_AUTOSAVE);
        return m_key_buffer;
    }

    const int ch{handle_special_keys(m_frame.get_key_press(false))};
    if (m_key_buffer)
    {
        return m_key_buffer;
    }
    m_key_buffer = ch;
    return ch;
}

int X11BaseDriver::wait_key_pressed(const bool timeout)
{
    int count{10};
    while (!key_pressed())
    {
        delay(25);
        if (timeout)
        {
            if (count == 0 || g_zoom_box_width != 0.0)
            {
                break;
            }
            --count;
        }
    }

    return key_pressed();
}

void X11BaseDriver::unget_key(const int key)
{
    assert(m_key_buffer == 0);
    m_key_buffer = key;
}

void X11BaseDriver::shell()
{
    const auto timeout{[] { driver_flush(); }};
    if (config::cmd_shell(timeout))
    {
        return;
    }
    stop_msg("Couldn't run shell '" + config::cmd_shell_command() + "', error " +
        std::to_string(config::get_cmd_shell_error()));
}

void X11BaseDriver::set_video_mode(const VideoInfo &mode)
{
    assert(g_video_table[g_adapter].x_dots == mode.x_dots);
    assert(g_video_table[g_adapter].y_dots == mode.y_dots);
    assert(g_video_table[g_adapter].colors == mode.colors);
    assert(g_video_table[g_adapter].driver == this);

    g_is_true_color = false;
    g_vesa_x_res = 0;
    g_vesa_y_res = 0;
    g_good_mode = true;
    g_and_color = g_colors - 1;
    g_box_count = 0;
    g_dac_count = g_cycle_limit;
    g_got_real_dac = true;
    read_palette();

    m_have_graphics_mode = true;
    resize();
    m_plot.clear();
    if (g_disk_flag)
    {
        end_disk();
    }
    set_normal_dot();
    set_normal_span();
    set_for_graphics();
    set_clear();
    m_output_flush_countdown = 0;
    m_flush_started = false;
}

void X11BaseDriver::put_string(const int row, const int col, const int attr, const char *msg)
{
    if (row != -1)
    {
        g_text_row = row;
    }
    if (col != -1)
    {
        g_text_col = col;
    }

    const int abs_row{g_text_row_base + g_text_row};
    const int abs_col{g_text_col_base + g_text_col};
    assert(abs_row >= 0 && abs_row < X11_TEXT_MAX_ROW);
    assert(abs_col >= 0 && abs_col < X11_TEXT_MAX_COL);
    m_text.put_string(abs_col, abs_row, attr, msg, g_text_row, g_text_col);
}

bool X11BaseDriver::is_text()
{
    return m_text_not_graphics;
}

void X11BaseDriver::set_for_text()
{
    m_text_not_graphics = true;
    m_plot.hide();
    m_text.show();
}

void X11BaseDriver::set_for_graphics()
{
    m_text_not_graphics = false;
    hide_text_cursor();
    m_text.hide();
    m_plot.show();
}

void X11BaseDriver::set_clear()
{
    if (m_text_not_graphics)
    {
        m_text.clear();
    }
    else
    {
        if (g_screen_x_dots > 0 && g_screen_y_dots > 0)
        {
            m_plot.resize(g_screen_x_dots, g_screen_y_dots);
            m_plot.clear();
        }
    }
}

void X11BaseDriver::move_cursor(const int row, const int col)
{
    if (row != -1)
    {
        m_cursor.row = row;
        g_text_row = row;
    }
    if (col != -1)
    {
        m_cursor.col = col;
        g_text_col = col;
    }
    m_cursor_shown = true;
    m_text.move_cursor(g_text_row_base + m_cursor.row, g_text_col_base + m_cursor.col);
}

void X11BaseDriver::hide_text_cursor()
{
    m_cursor_shown = false;
    m_text.hide_cursor();
}

void X11BaseDriver::set_attr(const int row, const int col, const int attr, const int count)
{
    if (row != -1)
    {
        g_text_row = row;
    }
    if (col != -1)
    {
        g_text_col = col;
    }
    m_text.set_attr(g_text_row_base + g_text_row, g_text_col_base + g_text_col, attr, count);
}

void X11BaseDriver::scroll_up(const int top, const int bot)
{
    m_text.scroll_up(top, bot);
}

void X11BaseDriver::stack_screen()
{
    m_saved_text_state.push_back(m_text_not_graphics);
    set_for_text();
    m_saved_cursor.push_back(TextLocation{g_text_row, g_text_col});
    m_saved_screens.push_back(m_text.get_screen());
    set_clear();
}

void X11BaseDriver::unstack_screen()
{
    assert(!m_saved_cursor.empty());
    assert(!m_saved_screens.empty());
    assert(!m_saved_text_state.empty());
    const bool restore_text{m_saved_text_state.back()};
    m_saved_text_state.pop_back();
    const TextLocation packed{m_saved_cursor.back()};
    m_saved_cursor.pop_back();
    g_text_row = packed.row;
    g_text_col = packed.col;
    m_text.set_screen(m_saved_screens.back());
    m_saved_screens.pop_back();
    move_cursor(-1, -1);
    if (restore_text)
    {
        set_for_text();
    }
    else
    {
        set_for_graphics();
    }
}

void X11BaseDriver::discard_screen()
{
    if (!m_saved_screens.empty())
    {
        assert(!m_saved_cursor.empty());
        assert(!m_saved_text_state.empty());
        const bool restore_text{m_saved_text_state.back()};
        m_saved_screens.pop_back();
        m_saved_cursor.pop_back();
        m_saved_text_state.pop_back();
        if (restore_text)
        {
            set_for_text();
        }
        else
        {
            set_for_graphics();
        }
    }
}

int X11BaseDriver::init_fm()
{
    return 0;
}

void X11BaseDriver::buzzer(Buzzer /*kind*/)
{
    if (!sound_buzzer_enabled() || !m_frame.connection().is_open())
    {
        return;
    }
    XBell(m_frame.connection().display(), 0);
    XFlush(m_frame.connection().display());
}

bool X11BaseDriver::sound_on(int /*frequency*/)
{
    return false;
}

void X11BaseDriver::sound_off()
{
}

void X11BaseDriver::mute()
{
}

bool X11BaseDriver::is_disk() const
{
    return false;
}

int X11BaseDriver::get_char_attr()
{
    return m_text.get_char_attr(g_text_row, g_text_col);
}

void X11BaseDriver::put_char_attr(const int char_attr)
{
    m_text.put_char_attr(g_text_row, g_text_col, char_attr);
}

void X11BaseDriver::delay(const int ms)
{
    m_frame.pump_messages(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(std::max(ms, 0)));
}

void X11BaseDriver::set_keyboard_timeout(const int ms)
{
    m_frame.set_keyboard_timeout(ms);
}

void X11BaseDriver::flush()
{
    m_plot.flush();
    m_frame.pump_messages(false);
}

void X11BaseDriver::debug_text(const char *text)
{
    if (text == nullptr)
    {
        return;
    }
    std::fputs(text, stderr);
    std::fflush(stderr);
}

void X11BaseDriver::get_cursor_pos(int &x, int &y) const
{
    m_frame.get_cursor_pos(x, y);
}

void X11BaseDriver::check_memory()
{
}

bool X11BaseDriver::get_filename(
    const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename)
{
    (void) type_desc;
    return get_a_file_name(hdg, type_wildcard, result_filename);
}

X11BaseDriver::WindowLayout X11BaseDriver::get_window_layout() const
{
    WindowLayout result{};
    const int text_width{m_text.width()};
    const int text_height{m_text.height()};
    const bool graphics_mode{m_have_graphics_mode && has_graphics_mode()};
    result.plot_width = graphics_mode ? g_video_table[g_adapter].x_dots : text_width;
    result.plot_height = graphics_mode ? g_video_table[g_adapter].y_dots : text_height;
    result.frame_width = std::max(text_width, result.plot_width);
    result.frame_height = std::max(text_height, result.plot_height);
    result.text_x = (result.frame_width - text_width) / 2;
    result.text_y = (result.frame_height - text_height) / 2;
    result.plot_x = (result.frame_width - result.plot_width) / 2;
    result.plot_y = (result.frame_height - result.plot_height) / 2;
    return result;
}

void X11BaseDriver::center_windows(const WindowLayout &layout)
{
    m_text.set_position(layout.text_x, layout.text_y);
    m_plot.set_position(layout.plot_x, layout.plot_y);
}

void X11BaseDriver::flush_output()
{
    if (m_text_not_graphics)
    {
        return;
    }

    const auto now{std::chrono::steady_clock::now()};
    if (m_flush_started && now - m_last_flush < OUTPUT_FLUSH_INTERVAL)
    {
        return;
    }

    flush();
    m_last_flush = now;
    m_flush_started = true;
}

} // namespace id::misc
