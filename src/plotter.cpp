/*
Copyright (C) 2024-2025 Louis Crespin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX identifier : GPL-3.0-or-later
*/
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <plotter/plotter.hpp>
#include <sstream>
#include <thread>

namespace plotter
{

using namespace std;
using namespace SDL2pp;

bool Plotter::plot()
{
    return internal_plot(false, "");
}

bool Plotter::save(string const& name)
{
    return internal_plot(true, name);
}

bool Plotter::internal_plot(bool save, std::string const& name)
{
    try
    {
        SDL sdl(SDL_INIT_VIDEO);
        SDLImage sdl_image(IMG_INIT_PNG);

        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) { s.initialize(); });

        int h = 0;
        int w = 0;
        int min_h = 0;
        int min_w = 0;
        if (m_stacking_direction == StackingDirection::Vertical)
        {
            for_each(m_sub_plots.begin(), m_sub_plots.end(), [&h](SubPlot const& s) { h += s.height(); });
            for_each(m_sub_plots.begin(), m_sub_plots.end(), [&min_h](SubPlot const& s) { min_h += s.min_height(); });
            w = m_sub_plots.front().width();
            min_w = m_sub_plots.front().min_width();
        }
        else
        {
            for_each(m_sub_plots.begin(), m_sub_plots.end(), [&w](SubPlot const& s) { w += s.width(); });
            for_each(m_sub_plots.begin(), m_sub_plots.end(), [&min_w](SubPlot const& s) { min_w += s.min_width(); });
            h = m_sub_plots.front().height();
            min_h = m_sub_plots.front().min_height();
        }

        Window window("Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h + plot_info_margin + info_height(), (save ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN) | SDL_WINDOW_RESIZABLE);

        window.SetMinimumSize(min_w, min_h + plot_info_margin + info_height());
        Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

        m_running = true;
        m_subplot_mouse_selected = no_sub_plot_hovered;
        m_arrow_cursor = SDL_GetCursor();
        m_size_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        SDL_Event event;
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        while (m_running)
        {
            auto start = chrono::steady_clock::now();
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    m_running = false;
                }
                else if (event.type == SDL_KEYDOWN)
                {
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_RIGHT:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_x_move(-10);
                        });
                        break;
                    }
                    case SDLK_LEFT:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_x_move(10);
                        });
                        break;
                    }
                    case SDLK_UP:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_y_move(-10);
                        });
                        break;
                    }
                    case SDLK_DOWN:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_y_move(10);
                        });
                        break;
                    }
                    default:
                        break;
                    }
                    update_mouse_position();
                }
                else if (event.type == SDL_MOUSEWHEEL)
                {
                    update_mouse_position();
                    size_t hovered = hovered_sub_plot();
                    if (hovered != no_sub_plot_hovered)
                        m_sub_plots[hovered].event_zoom(event.wheel.preciseY, m_mouse_x - base_x_of_hovered_subplot(), m_mouse_y - base_y_of_hovered_subplot());
                }
                else if (event.type == SDL_MOUSEMOTION)
                {
                    if (m_subplot_mouse_selected != no_sub_plot_hovered)
                    {
                        m_sub_plots[m_subplot_mouse_selected].event_x_move(event.motion.xrel);
                        m_sub_plots[m_subplot_mouse_selected].event_y_move(-event.motion.yrel);
                    }
                    update_mouse_position();
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    update_mouse_position();
                    m_subplot_mouse_selected = hovered_sub_plot();
                    SDL_SetCursor(m_size_cursor);
                }
                else if (event.type == SDL_MOUSEBUTTONUP)
                {
                    m_subplot_mouse_selected = no_sub_plot_hovered;
                    SDL_SetCursor(m_arrow_cursor);
                }
                else if (event.type == SDL_WINDOWEVENT)
                {
                    switch (event.window.event)
                    {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [&](SubPlot& s) {
                            // TODO : if step is less than m_sub_plots.size()
                            if (m_stacking_direction == StackingDirection::Vertical)
                                s.event_resize(event.window.data1, (event.window.data2 - plot_info_margin - info_height()) / m_sub_plots.size());
                            else
                                s.event_resize(event.window.data1 / m_sub_plots.size(), event.window.data2 - plot_info_margin - info_height());
                        });
                        break;
                    }
                    default:
                    {
                        break;
                    }
                    }
                }
            }

            renderer.SetDrawColor(255, 255, 255, 255); // Clear the screen
            renderer.Clear();

            renderer.SetDrawColor(0, 0, 0, 255);
            draw_info_box(renderer);

            if (m_stacking_direction == StackingDirection::Vertical)
            {
                int offset = 0;
                for (auto& e : m_sub_plots)
                {
                    auto texture = e.internal_plot(renderer);
                    renderer.SetTarget();
                    renderer.Copy(*texture, NullOpt, Point { 0, offset });
                    offset += e.height();
                }
            }
            else
            {
                int offset = 0;
                for (auto& e : m_sub_plots)
                {
                    auto texture = e.internal_plot(renderer);
                    renderer.SetTarget();
                    renderer.Copy(*texture, NullOpt, Point { offset, 0 });
                    offset += e.width();
                }
            }

            renderer.Present();

            if (save)
            {
                save_img(window, renderer, name);
                m_running = false;
            }
            else
            {
                // Frame limiter
                auto end = chrono::steady_clock::now();
                std::chrono::duration<double> diff = end - start;
                int duration_ms = round(diff.count() * 1000);
                int to_wait = max(40 - duration_ms, 0); // Make the whole iteration take at least 1/25 s
                this_thread::sleep_for(chrono::milliseconds(to_wait));
            }
        }
    }
    catch (exception const& e)
    {
        cerr << e.what() << endl;
    }
    SDL_SetCursor(m_arrow_cursor);
    SDL_FreeCursor(m_size_cursor);
    return true;
}

void Plotter::center_sprite(Renderer& renderer, Texture& texture, int x, int y)
{
    renderer.Copy(texture, NullOpt, { x - texture.GetWidth() / 2, y - texture.GetHeight() / 2 });
}

std::string Plotter::to_str(double nb, int nb_digits)
{
    ostringstream out;
    out << setprecision(nb_digits);
    out << nb;
    return out.str();
}

int Plotter::info_height() const
{
    int infos_lines = 1 + m_infos.size() / 2; // The space for mouse coordinates + functions and collections
    if (m_infos.size() % 2 != 0)
        infos_lines += 1; // Add one more line if the number is not even
    return (infos_lines + 1) * info_margin + infos_lines * m_small_font.GetHeight();
}

int Plotter::width() const
{
    int w = 0;
    if (m_stacking_direction == StackingDirection::Horizontal)
    {
        for_each(m_sub_plots.cbegin(), m_sub_plots.cend(), [&w](SubPlot const& s) { w += s.width(); });
    }
    else
    {
        w = m_sub_plots.front().width();
    }
    return w;
}

int Plotter::height() const
{
    int h = 0;
    if (m_stacking_direction == StackingDirection::Vertical)
    {
        for_each(m_sub_plots.cbegin(), m_sub_plots.cend(), [&h](SubPlot const& s) { h += s.height(); });
    }
    else
    {
        h = m_sub_plots.front().height();
    }
    return h + plot_info_margin + info_height();
}

void Plotter::draw_info_box(SDL2pp::Renderer& renderer)
{
    int offset = plot_info_margin + info_margin;
    if (m_stacking_direction == StackingDirection::Vertical)
    {
        for_each(m_sub_plots.begin(), m_sub_plots.end(), [&offset](SubPlot const& s) {
            offset += s.height();
        });
    }
    else
    {
        offset = m_sub_plots.front().height();
    }

    int const w = width() - 2 * info_box_hmargin;
    size_t i = 0;
    for (; i < m_infos.size(); i++)
    {
        int hpos = (i % 2 == 0) ? 0 : w / 2;
        renderer.SetDrawColor(m_infos[i].color);
        renderer.FillRect(Rect { info_box_hmargin + hpos, offset, m_small_font.GetHeight(), m_small_font.GetHeight() });
        string text = m_infos[i].name;
        if (m_small_font.GetHeight() + info_margin + (text.size() + 1) * m_small_font_advance > (size_t)w / 2) // make sure it will not take too much space
        {
            int extra_chars = ((m_small_font.GetHeight() + info_margin + (text.size() + 1) * m_small_font_advance) - w / 2) / m_small_font_advance;
            text.resize(text.size() - extra_chars - 4);
            text += "...";
        }
        Texture name = { renderer, m_small_font.RenderUTF8_Blended(text, SDL_Color(0, 0, 0, 255)) };
        renderer.Copy(name, NullOpt, { info_box_hmargin + hpos + m_small_font.GetHeight() + info_margin, offset });
        offset += (i % 2 == 0) ? 0 : info_margin + m_small_font.GetHeight();
    }
    if (i % 2 == 1)
    {
        offset += info_margin + m_small_font.GetHeight();
    }

    update_mouse_position();
    size_t h = hovered_sub_plot();
    if (h == no_sub_plot_hovered)
    {
        return;
    }
    int x_offset = base_x_of_hovered_subplot();
    int y_offset = base_y_of_hovered_subplot();
    if (x_offset == out_of_the_screen || y_offset == out_of_the_screen)
    {
        return;
    }
    if (!m_sub_plots[h].x_is_in_plot(m_mouse_x - x_offset) || !m_sub_plots[h].y_is_in_plot(m_mouse_y - y_offset))
    {
        return;
    }
    double x = m_sub_plots[h].from_plot_x(m_mouse_x - x_offset);
    double y = m_sub_plots[h].from_plot_y(m_mouse_y - y_offset);
    string text = "x : " + to_str(x) + ", y : " + to_str(y);
    Texture mouse_sprite { renderer, m_small_font.RenderUTF8_Blended(text, SDL_Color(0, 0, 0, 255)) };
    renderer.Copy(mouse_sprite, NullOpt, { info_box_hmargin, offset });
}

ColorGenerator::ColorGenerator(ColorPalette p)
    : m_index(0)
    , m_palette(p)
{ }

plotter::Color ColorGenerator::get_color()
{
    plotter::Color c = { 0, 0, 0 };
    size_t max = 1;
    switch (m_palette)
    {
    case ColorPalette::Default:
        c = colors_default[m_index];
        max = colors_default.size();
        break;
    case ColorPalette::Pastel:
        c = colors_pastel[m_index];
        max = colors_pastel.size();
        break;
    case ColorPalette::Fire:
        c = colors_fire[m_index];
        max = colors_fire.size();
        break;
    case ColorPalette::Ice:
        c = colors_ice[m_index];
        max = colors_ice.size();
        break;
    }
    m_index = (m_index + 1) % max;
    return c;
}

void Plotter::add_collection(Collection const& c, int n)
{
    m_sub_plots.at(n).add_collection(c);
}

void Plotter::add_function(Function const& f, int n)
{
    m_sub_plots.at(n).add_function(f);
}
void Plotter::set_window(double x, double y, double w, double h, int n)
{
    m_sub_plots.at(n).set_window(x, y, w, h);
}

SubPlot& Plotter::add_sub_plot(string const& title, optional<string> x_title, optional<string> y_title)
{
    m_sub_plots.push_back(SubPlot { *this, title, x_title, y_title, m_small_font_advance });
    return m_sub_plots.back();
}

void Plotter::save_img(Window const& window, Renderer& renderer, string name)
{
    Surface s { SDL_PIXELFORMAT_RGBA32, window.GetWidth(), window.GetHeight(), 32, R_MASK, G_MASK, B_MASK, A_MASK };
    {
        auto l = s.Lock();
        renderer.ReadPixels(NullOpt, SDL_PIXELFORMAT_RGBA32, l.GetPixels(), l.GetPitch());
    }
    name += ".png";
    IMG_SavePNG(s.Get(), name.c_str());
}

void Plotter::construct(string const& title, optional<string> x_title, optional<string> y_title)
{
    m_sub_plots.push_back(SubPlot { *this, title, x_title, y_title, m_small_font_advance });
    if (!m_small_font.IsFixedWidth())
    {
        throw runtime_error("The small font has to be fixed width");
    }
}

std::unique_ptr<SDL2pp::Texture> SubPlot::internal_plot(Renderer& renderer)
{
    m_texture = make_unique<Texture>(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width(), height());
    renderer.SetTarget(*m_texture);

    if (m_dirty_axis)
    {
        m_axis = determine_axis();
        m_dirty_axis = false;
    }

    renderer.SetDrawColor(255, 255, 255, 255); // Clear the screen
    renderer.Clear();
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.DrawRect(Rect { hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size(), m_width, m_height }); // Draw the plot box
    Texture title_sprite { renderer, m_plotter.m_big_font.RenderUTF8_Blended(m_title, SDL_Color(0, 0, 0, 255)) };
    if (m_title_height != title_sprite.GetHeight())
    {
        m_title_height = title_sprite.GetHeight();
        m_dirty_axis = true;
    }
    Plotter::center_sprite(renderer, title_sprite, hmargin + m_width / 2 + y_axis_name_size() + m_x_label_margin, (top_margin + title_size()) / 2);
    draw_axis_titles(renderer);
    draw_axis(m_axis, renderer);
    draw_content(renderer);
    return move(m_texture);
}

void SubPlot::draw_content(SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width() - hmargin, top_margin + title_size() + m_height };
    sprite.SetBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetTarget(sprite);
    renderer.SetDrawColor(0, 0, 0, 0);
    renderer.Clear();
    renderer.SetTarget(*m_texture);
    for (auto const& e : m_collections)
    {
        plot_collection(e, renderer, sprite);
    }
    for (auto const& e : m_functions)
    {
        plot_function(e, renderer, sprite);
    }
    renderer.Copy(sprite, Rect { hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size(), m_width, m_height }, { hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size() });
}

int SubPlot::x_axis_name_size() const
{
    if (m_x_title)
        return m_plotter.text_margin + m_plotter.small_font_size; // Just one margin is enough
    return 0;
}
int SubPlot::y_axis_name_size() const
{
    if (m_y_title)
        return 2 * m_plotter.text_margin + m_plotter.small_font_size;
    return 0;
}
int SubPlot::title_size() const
{
    return m_title_height;
}

tuple<vector<SubPlot::Axis>, vector<SubPlot::Axis>> SubPlot::determine_axis()
{
    vector<SubPlot::Axis> x;
    vector<SubPlot::Axis> y;
    // Secondary axis :
    double x_max = from_plot_x(m_width);
    double x_min = from_plot_x(0);
    double y_max = from_plot_y(0);
    double y_min = from_plot_y(m_height);

    double delta_x = x_max - x_min;
    double delta_y = y_max - y_min;

    int max_nb_vertical_axis = m_width / min_spacing_between_axis;
    int min_nb_vertical_axis = m_width / max_spacing_between_axis;
    int max_nb_horizontal_axis = m_height / min_spacing_between_axis;
    int min_nb_horizontal_axis = m_height / max_spacing_between_axis;

    double x_step = compute_grid_step(min_nb_vertical_axis, max_nb_vertical_axis, delta_x);
    double y_step = compute_grid_step(min_nb_horizontal_axis, max_nb_horizontal_axis, delta_y);

    double rounded_x_min = round(x_min / x_step) * x_step;
    double rounded_y_min = round(y_min / y_step) * y_step;

    size_t max_ordinate_length = 3;

    for (int i = 0; i < max_nb_horizontal_axis + 1; i++)
    {
        double f_ordinate = rounded_y_min + i * y_step;
        int ordinate = to_plot_y<int>(f_ordinate);
        if (y_is_in_plot(ordinate))
        {
            if (Plotter::to_str(f_ordinate).size() > max_ordinate_length)
                max_ordinate_length = Plotter::to_str(f_ordinate).size();
            x.push_back({ ordinate, f_ordinate, false });
        }
    }

    // Update the horizontal margin according to label text length
    int old_x_label_margin = m_x_label_margin;
    int old_width = width();
    m_x_label_margin = max_ordinate_length * m_plotter.m_small_font_advance + 2 * m_plotter.text_margin;
    if (old_x_label_margin != m_x_label_margin)
        event_resize(old_width, height());

    for (int i = 0; i < max_nb_vertical_axis + 1; i++)
    {
        int abscissa = to_plot_x<int>(rounded_x_min + i * x_step);
        if (x_is_in_plot(abscissa))
        {
            y.push_back({ abscissa, rounded_x_min + i * x_step, false });
        }
    }
    // Main axis :
    if (y_is_in_plot(to_plot_y<int>(0)))
    {
        x.push_back({ to_plot_y<int>(0.), 0., true });
    }
    if (x_is_in_plot(to_plot_x<int>(0)))
    {
        y.push_back({ to_plot_x<int>(0.), 0., true });
    }

    return { x, y };
}

void SubPlot::draw_axis(tuple<vector<Axis>, vector<Axis>> const& axis, Renderer& renderer)
{
    for (auto& e : get<0>(axis))
    {
        if (e.is_main)
        {
            renderer.SetDrawColor(120, 120, 120, 255);
            draw_horizontal_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(hmargin + y_axis_name_size() + m_x_label_margin, e.plot_coordinate - line_width_unit, hmargin + y_axis_name_size() + m_x_label_margin + m_width, e.plot_coordinate + line_width_unit));
        }
        else
        {
            renderer.SetDrawColor(180, 180, 180, 255);
            draw_horizontal_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(hmargin + y_axis_name_size() + m_x_label_margin, e.plot_coordinate - line_width_unit, hmargin + y_axis_name_size() + m_x_label_margin + m_width, e.plot_coordinate + line_width_unit));
        }
    }

    for (auto& e : get<1>(axis))
    {
        if (e.is_main)
        {
            renderer.SetDrawColor(120, 120, 120, 255);
            draw_vertical_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(e.plot_coordinate - line_width_unit, top_margin + title_size(), e.plot_coordinate + line_width_unit, top_margin + title_size() + m_height));
        }
        else
        {
            renderer.SetDrawColor(180, 180, 180, 255);
            draw_vertical_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(e.plot_coordinate - line_width_unit, top_margin + title_size(), e.plot_coordinate + line_width_unit, top_margin + title_size() + m_height));
        }
    }
}

double SubPlot::compute_grid_step(int min_nb, int max_nb, double range)
{
    int factor = 0;
    int exponent = 0;
    int constexpr candidate_factor[3] = { 1, 2, 5 };
    for (int i = 0; i < 3; i++)
    {
        int current_factor = candidate_factor[i];
        int exponent_1 = floor(log10(range / (double)(current_factor * max_nb)));
        int exponent_2 = floor(log10(range / (double)(current_factor * min_nb)));
        if (exponent_1 != exponent_2)
        {
            exponent = exponent_2;
            factor = current_factor;
        }
    }
    return factor * pow(10., exponent);
}

void SubPlot::draw_point(Coordinate c, Renderer& renderer, PointType point_type)
{
    int abscissa = to_plot_x<int>(c.x);
    int ordinate = to_plot_y<int>(c.y);
    if (x_is_in_plot(abscissa) && y_is_in_plot(ordinate))
    {
        if (point_type == PointType::Square)
            renderer.FillRect(Rect::FromCorners(abscissa - half_point_size, ordinate - half_point_size, abscissa + half_point_size, ordinate + half_point_size));
        else if (point_type == PointType::Circle)
            draw_circle(renderer, abscissa, ordinate, 2 * half_point_size);
        else // if (point_type == PointType::Cross)
            draw_cross(renderer, abscissa, ordinate, 4 * half_point_size);
        if (c.x_error != 0.)
        {
            int x_min = to_plot_x<int>(c.x - (c.x_error) / 2);
            int x_max = to_plot_x<int>(c.x + (c.x_error) / 2);
            renderer.DrawLine(x_min, ordinate, x_max, ordinate);
            renderer.DrawLine(x_min, ordinate - 2 * half_point_size, x_min, ordinate + 2 * half_point_size);
            renderer.DrawLine(x_max, ordinate - 2 * half_point_size, x_max, ordinate + 2 * half_point_size);
        }
        if (c.y_error != 0.)
        {
            int y_min = to_plot_y<int>(c.y - (c.y_error) / 2);
            int y_max = to_plot_y<int>(c.y + (c.y_error) / 2);
            renderer.DrawLine(abscissa, y_max, abscissa, y_min);
            renderer.DrawLine(abscissa - 2 * half_point_size, y_max, abscissa + 2 * half_point_size, y_max);
            renderer.DrawLine(abscissa - 2 * half_point_size, y_min, abscissa + 2 * half_point_size, y_min);
        }
    }
}

double SubPlot::from_plot_x(int x) const
{
    return ((double)x - (hmargin + y_axis_name_size() + m_x_label_margin) - (double)m_width / 2.) / m_x_zoom - m_x_offset;
}
double SubPlot::from_plot_y(int x) const
{
    return (-x + top_margin + title_size() + (double)m_height / 2.) / m_y_zoom - m_y_offset;
}
bool SubPlot::x_is_in_plot(int x) const
{
    return x > hmargin + y_axis_name_size() + m_x_label_margin && x < hmargin + y_axis_name_size() + m_x_label_margin + m_width;
}
bool SubPlot::y_is_in_plot(int y) const
{
    return y > top_margin + title_size() && y < top_margin + title_size() + m_height;
}

void SubPlot::draw_vertical_line_number(double nb, int x, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_plotter.m_small_font.RenderUTF8_Blended(Plotter::to_str(nb), SDL_Color(0, 0, 0, 255)) };
    Plotter::center_sprite(renderer, sprite, x, top_margin + title_size() + m_height + m_bottom_margin / 2);
}

void SubPlot::draw_horizontal_line_number(double nb, int y, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_plotter.m_small_font.RenderUTF8_Blended(Plotter::to_str(nb), SDL_Color(0, 0, 0, 255)) };
    Plotter::center_sprite(renderer, sprite, hmargin + y_axis_name_size() + m_x_label_margin / 2, y);
}

void SubPlot::draw_axis_titles(SDL2pp::Renderer& renderer)
{
    if (m_y_title)
    {
        Texture sprite { renderer, m_plotter.m_small_font.RenderUTF8_Blended(*m_y_title, SDL_Color(0, 0, 0, 255)) };
        renderer.Copy(sprite, NullOpt, { hmargin + m_plotter.text_margin, m_height / 2 + sprite.GetWidth() / 2 + top_margin + title_size() }, 270, Point { 0, 0 });
    }
    if (m_x_title)
    {
        Texture sprite { renderer, m_plotter.m_small_font.RenderUTF8_Blended(*m_x_title, SDL_Color(0, 0, 0, 255)) };
        Plotter::center_sprite(renderer, sprite, hmargin + y_axis_name_size() + m_x_label_margin + m_width / 2, top_margin + title_size() + m_height + m_bottom_margin + x_axis_name_size() / 2);
    }
}

void SubPlot::plot_collection(Collection const& c, SDL2pp::Renderer& renderer, Texture& into)
{
    if (c.points.size() == 0)
        return;

    SDL2pp::Color normal = c.get_color();
    SDL2pp::Color transparent = { normal.r, normal.g, normal.b, 190 }; // Used for home-made antialiasing

    // This builds the maximal segment that can be drawn
    int const max_segment_width = ceil(sqrt((double)m_width * (double)m_width + (double)m_height * (double)m_height));
    Texture total_segment { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 2 * max_segment_width, 5 * line_width_unit };
    total_segment.SetBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetTarget(total_segment);
    // Creates a transparent background to help antialiasing
    renderer.SetDrawColor(255, 255, 255, 0);
    renderer.FillRect(Rect::FromCorners(0, 0, 2 * max_segment_width, 5 * line_width_unit));
    if (c.line_style == LineStyle::Solid)
    {
        renderer.SetDrawColor(transparent);
        renderer.DrawLine(0, line_width_unit, 2 * max_segment_width, line_width_unit);
        renderer.DrawLine(0, 3 * line_width_unit, 2 * max_segment_width, 3 * line_width_unit);

        renderer.SetDrawColor(normal);
        renderer.DrawLine(0, 2 * line_width_unit, 2 * max_segment_width, 2 * line_width_unit);
    }
    else if (c.line_style == LineStyle::Dashed)
    {
        int w = 0;
        int dash_len = 15;
        while (w < 2 * max_segment_width)
        {
            renderer.SetDrawColor(transparent);
            renderer.DrawLine(w, line_width_unit, w + dash_len, line_width_unit);
            renderer.DrawLine(w, 3 * line_width_unit, w + dash_len, 3 * line_width_unit);

            renderer.SetDrawColor(normal);
            renderer.DrawLine(w, 2 * line_width_unit, w + dash_len, 2 * line_width_unit);
            w += 2 * dash_len;
        }
    }
    renderer.SetTarget(into);

    size_t lenght_drawn = 0;
    for (size_t i = 0; i < c.points.size() - 1; i++)
    {
        if ((to_plot_x<int>(c.points[i].x) < hmargin + y_axis_name_size() + m_x_label_margin && to_plot_x<int>(c.points[i + 1].x) < hmargin + y_axis_name_size() + m_x_label_margin)
            || (to_plot_x<int>(c.points[i].x) > hmargin + y_axis_name_size() + m_x_label_margin + m_width && to_plot_x<int>(c.points[i + 1].x) > hmargin + y_axis_name_size() + m_x_label_margin + m_width)
            || (to_plot_y<int>(c.points[i].y) < top_margin + title_size() && to_plot_y<int>(c.points[i + 1].y) < top_margin + title_size())
            || (to_plot_y<int>(c.points[i].y) > top_margin + title_size() + m_height && to_plot_y<int>(c.points[i + 1].y) > top_margin + title_size() + m_height))
            continue; // Both points are outside of the screen, and on the same side : there is nothing to draw
        if (c.display_points == DisplayPoints::Yes)
            draw_point(c.points[i], renderer, c.point_type);
        if (c.display_lines == DisplayLines::Yes)
            draw_line(to_point(c.points[i]), to_point(c.points[i + 1]), renderer, into, lenght_drawn, total_segment);
    }
    if (c.display_points == DisplayPoints::Yes)
        draw_point(c.points.back(), renderer, c.point_type);
    renderer.SetTarget(*m_texture);
}

void SubPlot::plot_function(Function const& f, SDL2pp::Renderer& renderer, Texture& into)
{
    double x_min = from_plot_x(hmargin + y_axis_name_size() + m_x_label_margin);
    double x_max = from_plot_x(hmargin + y_axis_name_size() + m_x_label_margin + m_width);
    vector<Coordinate> coordinates;
    coordinates.reserve(sampling_number_of_points);
    for (int i = 0; i < sampling_number_of_points; i++)
    {
        double v = x_min + i * (x_max - x_min) / sampling_number_of_points;
        coordinates.push_back({ v, f.function(v) });
    }
    plot_collection(Collection { coordinates, f.name, DisplayPoints::No, DisplayLines::Yes, PointType::Square, f.line_style, f.color }, renderer, into);
}

SubPlot::ScreenPoint SubPlot::to_point(Coordinate const& c) const
{
    return { to_plot_x<int64_t>(c.x), to_plot_y<int64_t>(c.y) };
}

bool SubPlot::intersect_rect_and_line(int64_t rx, int64_t ry, int64_t rw, int64_t rh, int64_t& x1, int64_t& x2, int64_t& y1, int64_t& y2)
{
    // This function exists in SDL, but not with int64_t
    auto clip = [](int64_t p, int64_t q, int64_t& t0_num, int64_t& t0_den, int64_t& t1_num, int64_t& t1_den) { // Liang-Barsky algorithm
        if (p == 0)
        {
            return q >= 0;
        }
        int64_t t_num = q;
        int64_t t_den = p;

        if (t_den < 0)
        {
            t_num = -t_num;
            t_den = -t_den;
        }

        if (p < 0)
        {
            if (t_num * t1_den > t_den * t1_num)
                return false; // t > t1
            if (t_num * t0_den > t_den * t0_num)
            { // t > t0
                t0_num = t_num;
                t0_den = t_den;
            }
        }
        else
        {
            if (t_num * t0_den < t_den * t0_num)
                return false; // t < t0
            if (t_num * t1_den < t_den * t1_num)
            { // t < t1
                t1_num = t_num;
                t1_den = t_den;
            }
        }
        return true;
    };
    int64_t t0_num = 0;
    int64_t t0_den = 1;
    int64_t t1_num = 1;
    int64_t t1_den = 1;
    int64_t dx = x2 - x1;
    int64_t dy = y2 - y1;

    // Rectangle borders
    if (!clip(-dx, x1 - rx, t0_num, t0_den, t1_num, t1_den))
        return false; // left
    if (!clip(dx, rx + rw - x1, t0_num, t0_den, t1_num, t1_den))
        return false; // right
    if (!clip(-dy, y1 - ry, t0_num, t0_den, t1_num, t1_den))
        return false; // bottom
    if (!clip(dy, ry + rh - y1, t0_num, t0_den, t1_num, t1_den))
        return false; // top

    if (t1_num < t1_den)
    {
        x2 = x1 + dx * t1_num / t1_den;
        y2 = y1 + dy * t1_num / t1_den;
    }

    if (t0_num > 0)
    {
        x1 = x1 + dx * t0_num / t0_den;
        y1 = y1 + dy * t0_num / t0_den;
    }

    return true;
}

void SubPlot::draw_circle(SDL2pp::Renderer& renderer, int x, int y, int radius)
{
    // This is based on the Midpoint algorithm
    int x_offset = 0;
    int y_offset = radius;
    int d = radius - 1;

    while (y_offset >= x_offset)
    {
        renderer.DrawLine(x - y_offset, y + x_offset, x + y_offset, y + x_offset);
        renderer.DrawLine(x - x_offset, y + y_offset, x + x_offset, y + y_offset);
        renderer.DrawLine(x - x_offset, y - y_offset, x + x_offset, y - y_offset);
        renderer.DrawLine(x - y_offset, y - x_offset, x + y_offset, y - x_offset);

        if (d >= 2 * x_offset)
        {
            d -= 2 * x_offset + 1;
            x_offset += 1;
        }
        else if (d < 2 * (radius - y_offset))
        {
            d += 2 * y_offset - 1;
            y_offset -= 1;
        }
        else
        {
            d += 2 * (y_offset - x_offset - 1);
            y_offset -= 1;
            x_offset += 1;
        }
    }
}

void SubPlot::draw_cross(SDL2pp::Renderer& renderer, int x, int y, int length)
{
    renderer.DrawLine(x + length / 2, y, x - length / 2, y);
    renderer.DrawLine(x, y + length / 2, x, y - length / 2);
}

void SubPlot::draw_line(ScreenPoint const& p1, ScreenPoint const& p2, Renderer& renderer, Texture& into, size_t& lenght_drawn, SDL2pp::Texture& total_segment)
{
    int64_t x1 = p1.x;
    int64_t x2 = p2.x;
    int64_t y1 = p1.y;
    int64_t y2 = p2.y;
    intersect_rect_and_line(hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size(), m_width, m_height, x1, x2, y1, y2);
    double const max_w = sqrt((double)m_width * (double)m_width + (double)m_height * (double)m_height);
    double w_candidate = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)) + 1;
    int w;
    if (w_candidate > max_w)
    {
        w = max_w;
    }
    else
    {
        w = static_cast<int>(w_candidate);
    }
    double angle = atan2(y2 - y1, x2 - x1);
    int x_offset = static_cast<int>(2. * line_width_unit * sin(angle)); // offset due to rotation
    int y_offset = static_cast<int>(-2. * line_width_unit * cos(angle));
    Point dst_point { x1 + x_offset, y1 + y_offset };
    angle *= 360 / (2 * numbers::pi_v<double>); // to degree
    int segment_offset = lenght_drawn % (int)ceil(max_w);
    renderer.Copy(total_segment, Rect { segment_offset, 0, w, 5 * line_width_unit }, dst_point, angle, Point { 0, 0 });
    lenght_drawn += w - line_width_unit;
}

void SubPlot::add_collection(Collection const& c)
{
    m_collections.push_back(c);
    if (!m_collections.back().color.definite)
    {
        m_collections.back().color = m_plotter.m_color_generator.get_color();
    }
    m_plotter.add_info_line(InfoLine { m_collections.back().name, m_collections.back().get_color() });
}

void SubPlot::add_function(Function const& f)
{
    m_functions.push_back(f);
    if (!m_functions.back().color.definite)
    {
        m_functions.back().color = m_plotter.m_color_generator.get_color();
    }
    m_plotter.add_info_line(InfoLine { m_functions.back().name, m_functions.back().get_color() });
}
void SubPlot::set_window(double x, double y, double w, double h)
{
    m_window_defined = true;
    m_x_zoom = (double)m_width / w;
    m_y_zoom = (double)m_height / h;
    m_y_x_ratio = m_y_zoom / m_x_zoom;
    m_x_offset = -(x + w / 2);
    m_y_offset = -(y - h / 2);
}

void SubPlot::initialize()
{
    if (!m_window_defined)
        initialize_zoom_and_offset();
    m_bottom_margin = m_plotter.text_margin + 2 * m_small_font_advance;
    m_x_label_margin = 0; // This has to have a value before determine_axis() is called, but we don't care exactly what
    determine_axis();     // This is needed because it computes m_x_label_margin
}

void SubPlot::initialize_zoom_and_offset()
{
    m_window_defined = true;
    if (m_collections.size() == 0)
    {
        m_x_zoom = 50.;
        m_y_x_ratio = 1.;
        m_y_zoom = m_x_zoom * m_y_x_ratio;
        m_x_offset = 0.;
        m_y_offset = 0.;
        return;
    }

    double x_max = m_collections.front().points.front().x;
    double x_min = m_collections.front().points.front().x;
    double y_max = m_collections.front().points.front().y;
    double y_min = m_collections.front().points.front().y;
    for (auto const& c : m_collections)
    {
        for (auto const& e : c.points)
        {
            if (e.x > x_max)
            {
                x_max = e.x;
            }
            if (e.x < x_min)
            {
                x_min = e.x;
            }
            if (e.y > y_max)
            {
                y_max = e.y;
            }
            if (e.y < y_min)
            {
                y_min = e.y;
            }
        }
    }
    double delta_x = x_max - x_min;
    double delta_y = y_max - y_min;
    m_x_zoom = (double)m_width / delta_x;
    double _y_zoom = (double)m_height / delta_y;
    if (m_orthonormal == Orthonormal::Yes)
    {
        m_x_zoom = min(m_x_zoom, _y_zoom);
        m_y_x_ratio = 1;
    }
    else
        m_y_x_ratio = _y_zoom / m_x_zoom;

    m_x_zoom *= 0.9; // to have margins
    m_y_zoom = m_x_zoom * m_y_x_ratio;

    m_x_offset = -(x_max + x_min) / 2;
    m_y_offset = -(y_max + y_min) / 2;
}

void SubPlot::event_x_move(int x)
{
    m_x_offset += x / m_x_zoom;
    m_dirty_axis = true;
}

void SubPlot::event_y_move(int y)
{
    m_y_offset += y / m_y_zoom;
    m_dirty_axis = true;
}

void SubPlot::event_zoom(float mouse_wheel, int mouse_x, int mouse_y)
{
    double x = from_plot_x(mouse_x);
    double y = from_plot_y(mouse_y);
    double back_x_zoom = m_x_zoom;
    m_x_zoom *= pow(zoom_factor, mouse_wheel);
    m_y_zoom = m_x_zoom * m_y_x_ratio;
    if (from_plot_x(hmargin + y_axis_name_size() + m_x_label_margin) == from_plot_x(hmargin + y_axis_name_size() + m_x_label_margin + 2 * line_width_unit) || from_plot_y(top_margin + title_size()) == from_plot_y(top_margin + title_size() + 2 * line_width_unit))
    {
        // We were too far and can't distinguish two points close to each other anymore
        m_x_zoom = back_x_zoom;
        m_y_zoom = m_x_zoom * m_y_x_ratio;
    }
    else
    {
        double previous_x = x;
        double previous_y = y;
        double x = from_plot_x(mouse_x);
        double y = from_plot_y(mouse_y);
        if (mouse_x != Plotter::out_of_the_screen && mouse_y != Plotter::out_of_the_screen)
        {
            m_x_offset += x - previous_x;
            m_y_offset += y - previous_y;
        }
        m_dirty_axis = true;
    }
}

void SubPlot::event_resize(int w, int h)
{
    m_width = w - 2 * hmargin - y_axis_name_size() - m_x_label_margin;
    m_height = h - top_margin - title_size() - x_axis_name_size() - m_bottom_margin;
    m_dirty_axis = true;
}

int SubPlot::min_width() const
{
    return 2 * hmargin + plot_min_width + y_axis_name_size() + m_x_label_margin;
}

int SubPlot::min_height() const
{
    return top_margin + title_size() + plot_min_height + x_axis_name_size() + m_bottom_margin;
}

int SubPlot::width() const
{
    return 2 * hmargin + m_width + y_axis_name_size() + m_x_label_margin;
}

int SubPlot::height() const
{
    return top_margin + title_size() + m_height + x_axis_name_size() + m_bottom_margin;
}

vector<SubPlot::InfoLine> SubPlot::infos() const
{
    vector<InfoLine> r;
    for (auto const& e : m_collections)
    {
        r.push_back({ e.name, e.get_color() });
    }
    for (auto const& e : m_functions)
    {
        r.push_back({ e.name, e.get_color() });
    }
    return r;
}

void Plotter::update_mouse_position()
{
    SDL_GetMouseState(&m_mouse_x, &m_mouse_y);
}

size_t Plotter::hovered_sub_plot() const
{
    if (m_stacking_direction == StackingDirection::Vertical)
    {
        int offset = 0;
        for (size_t i = 0; i < m_sub_plots.size(); i++)
        {
            offset += m_sub_plots[i].height();
            if (offset >= m_mouse_y)
                return i;
        }
        return no_sub_plot_hovered;
    }
    else
    {
        int offset = 0;
        for (size_t i = 0; i < m_sub_plots.size(); i++)
        {
            offset += m_sub_plots[i].width();
            if (offset >= m_mouse_x)
                return i;
        }
        return no_sub_plot_hovered;
    }
}

int Plotter::base_x_of_hovered_subplot() const
{
    if (m_stacking_direction == StackingDirection::Vertical)
        return 0;
    int offset = 0;
    for (size_t i = 0; i < m_sub_plots.size(); i++)
    {
        if (offset + m_sub_plots[i].width() >= m_mouse_x)
            return offset;
        offset += m_sub_plots[i].width();
    }
    return out_of_the_screen;
}

int Plotter::base_y_of_hovered_subplot() const
{
    if (m_stacking_direction == StackingDirection::Horizontal)
        return 0;
    int offset = 0;
    for (size_t i = 0; i < m_sub_plots.size(); i++)
    {
        if (offset + m_sub_plots[i].height() >= m_mouse_y)
            return offset;
        offset += m_sub_plots[i].height();
    }
    return out_of_the_screen;
}

void Plotter::add_info_line(SubPlot::InfoLine const& i)
{
    m_infos.push_back(i);
}
}