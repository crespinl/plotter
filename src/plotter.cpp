/*
Copyright (C) 2024 Louis Crespin

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

SPDX itentifier : GPL-3.0-or-later
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

bool Plotter::plot(Orthonormal orthonormal)
{
    return internal_plot(orthonormal, false, "");
}

bool Plotter::save(string const& name, Orthonormal orthonormal)
{
    return internal_plot(orthonormal, true, name);
}

bool Plotter::internal_plot(Orthonormal orthonormal, bool save, std::string const& name)
{
    try
    {
        SDL sdl(SDL_INIT_VIDEO);
        SDLImage sdl_image(IMG_INIT_PNG);
        Window window("Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 2 * m_hmargin + m_width, top_margin + m_height + plot_info_margin + x_axis_name_size() + info_height() + bottom_margin, (save ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN) | SDL_WINDOW_RESIZABLE);
        window.SetMinimumSize(2 * m_hmargin + min_width, top_margin + min_height + plot_info_margin + x_axis_name_size() + info_height() + bottom_margin);
        Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

        m_running = true;
        m_mouse_down = false;
        m_arrow_cursor = SDL_GetCursor();
        m_size_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        if (!m_window_defined)
            initialize_zoom_and_offset(orthonormal);
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
                        m_x_offset -= 10 / m_x_zoom;
                        break;
                    }
                    case SDLK_LEFT:
                    {
                        m_x_offset += 10 / m_x_zoom;
                        break;
                    }
                    case SDLK_UP:
                    {
                        m_y_offset -= 10 / m_y_zoom;
                        break;
                    }
                    case SDLK_DOWN:
                    {
                        m_y_offset += 10 / m_y_zoom;
                        break;
                    }
                    default:
                        break;
                    }
                    m_dirty_axis = true;
                    update_mouse_position();
                }
                else if (event.type == SDL_MOUSEWHEEL)
                {
                    update_mouse_position();
                    double back_x_zoom = m_x_zoom;
                    m_x_zoom *= pow(zoom_factor, event.wheel.preciseY);
                    m_y_zoom = m_x_zoom * m_y_x_ratio;
                    if (from_plot_x(m_hmargin) == from_plot_x(m_hmargin + 2 * line_width_half) || from_plot_y(top_margin) == from_plot_y(top_margin + 2 * line_width_half))
                    {
                        // We were too far and can't distinguish two points close to each other anymore
                        m_x_zoom = back_x_zoom;
                        m_y_zoom = m_x_zoom * m_y_x_ratio;
                    }
                    else
                    {
                        double previous_x = m_mouse_x;
                        double previous_y = m_mouse_y;
                        update_mouse_position();
                        if (!isnan(previous_x) && !isnan(previous_y) && !isnan(m_mouse_x) && !isnan(m_mouse_y))
                        {
                            m_x_offset += m_mouse_x - previous_x;
                            m_y_offset += m_mouse_y - previous_y;
                            update_mouse_position();
                        }
                        m_dirty_axis = true;
                    }
                }
                else if (event.type == SDL_MOUSEMOTION)
                {
                    if (m_mouse_down)
                    {
                        m_x_offset += event.motion.xrel / m_x_zoom;
                        m_y_offset -= event.motion.yrel / m_y_zoom;
                        m_dirty_axis = true;
                    }
                    update_mouse_position();
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    m_mouse_down = true;
                    SDL_SetCursor(m_size_cursor);
                }
                else if (event.type == SDL_MOUSEBUTTONUP)
                {
                    m_mouse_down = false;
                    SDL_SetCursor(m_arrow_cursor);
                }
                else if (event.type == SDL_WINDOWEVENT)
                {
                    switch (event.window.event)
                    {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        m_width = event.window.data1 - 2 * m_hmargin;
                        m_height = event.window.data2 - top_margin - plot_info_margin - x_axis_name_size() - info_height() - bottom_margin;
                        m_dirty_axis = true;
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
            renderer.DrawRect(Rect::FromCorners(m_hmargin, top_margin, m_hmargin + m_width, top_margin + m_height)); // Draw the plot box
            draw_info_box(renderer);

            Texture title_sprite { renderer, m_big_font.RenderUTF8_Blended(m_title, SDL_Color(0, 0, 0, 255)) };
            center_sprite(renderer, title_sprite, (m_width + 2 * m_hmargin) / 2, top_margin / 2);
            draw_axis_titles(renderer);

            if (m_dirty_axis)
            {
                m_axis = determine_axis();
                m_dirty_axis = false;
            }
            draw_axis(m_axis, renderer);

            draw_content(renderer);

            renderer.Present();

            if (save)
            {
                save_img(window, renderer, name);
                m_running = false;
            }

            // Frame limiter
            auto end = chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            int duration_ms = round(diff.count() * 1000);
            int to_wait = max(40 - duration_ms, 0); // Make the whole iteration take at least 1/25 s
            this_thread::sleep_for(chrono::milliseconds(to_wait));
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

void Plotter::draw_content(SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, m_hmargin + m_width, top_margin + m_height };
    sprite.SetBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetTarget(sprite);
    renderer.SetDrawColor(0, 0, 0, 0);
    renderer.Clear();
    renderer.SetTarget();
    for (auto const& e : m_collections)
    {
        plot_collection(e, renderer, sprite);
    }
    for (auto const& e : m_functions)
    {
        plot_function(e, renderer, sprite);
    }
    renderer.Copy(sprite, Rect { m_hmargin, top_margin, m_width, m_height }, { m_hmargin, top_margin });
}

int Plotter::x_axis_name_size() const
{
    if (m_x_title)
        return 2 * text_margin + small_font_size;
    return 0;
}
int Plotter::y_axis_name_size() const
{
    if (m_y_title)
        return 2 * text_margin + small_font_size;
    return 0;
}

tuple<vector<Plotter::Axis>, vector<Plotter::Axis>> Plotter::determine_axis() const
{
    vector<Plotter::Axis> x;
    vector<Plotter::Axis> y;
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

    for (int i = 0; i < max_nb_horizontal_axis + 1; i++)
    {
        int ordinate = to_plot_y<int>(rounded_y_min + i * y_step);
        if (y_is_in_plot(ordinate))
        {
            x.push_back({ ordinate, rounded_y_min + i * y_step, false });
        }
    }
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
    return {x, y};
}

void Plotter::draw_axis(tuple<vector<Axis>, vector<Axis>> const& axis, Renderer& renderer)
{
    for (auto& e : get<0>(axis))
    {
        if (e.is_main)
        {
            renderer.SetDrawColor(120, 120, 120, 255);
            draw_horizontal_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(m_hmargin, e.plot_coordinate - line_width_half, m_hmargin + m_width, e.plot_coordinate + line_width_half));
        }
        else
        {
            renderer.SetDrawColor(180, 180, 180, 255);
            draw_horizontal_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(m_hmargin, e.plot_coordinate - line_width_half, m_hmargin + m_width, e.plot_coordinate + line_width_half));
        }
    }

    for (auto& e : get<1>(axis))
    {
        if (e.is_main)
        {
            renderer.SetDrawColor(120, 120, 120, 255);
            draw_vertical_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(e.plot_coordinate - line_width_half, top_margin, e.plot_coordinate + line_width_half, top_margin + m_height));
        }
        else
        {
            renderer.SetDrawColor(180, 180, 180, 255);
            draw_vertical_line_number(e.coordinate, e.plot_coordinate, renderer);
            renderer.FillRect(Rect::FromCorners(e.plot_coordinate - line_width_half, top_margin, e.plot_coordinate + line_width_half, top_margin + m_height));
        }
    }
}

double Plotter::compute_grid_step(int min_nb, int max_nb, double range)
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

void Plotter::draw_point(double x, double y, Renderer& renderer)
{
    int abscissa = to_plot_x<int>(x);
    int ordinate = to_plot_y<int>(y);
    if (x_is_in_plot(abscissa) && y_is_in_plot(ordinate))
        renderer.FillRect(Rect::FromCorners(abscissa - half_point_size, ordinate - half_point_size, abscissa + half_point_size, ordinate + half_point_size));
}

double Plotter::from_plot_x(int x) const
{
    return ((double)x - m_hmargin - (double)m_width / 2.) / m_x_zoom - m_x_offset;
}
double Plotter::from_plot_y(int x) const
{
    return (-x + top_margin + (double)m_height / 2.) / m_y_zoom - m_y_offset;
}
bool Plotter::x_is_in_plot(int x) const
{
    return x > m_hmargin && x < m_hmargin + m_width;
}
bool Plotter::y_is_in_plot(int y) const
{
    return y > top_margin && y < top_margin + m_height;
}

void Plotter::draw_vertical_line_number(double nb, int x, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_small_font.RenderUTF8_Blended(to_str(nb), SDL_Color(0, 0, 0, 255)) };
    center_sprite(renderer, sprite, x, top_margin + m_height + text_margin + sprite.GetHeight() / 2);
}

void Plotter::draw_horizontal_line_number(double nb, int y, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_small_font.RenderUTF8_Blended(to_str(nb), SDL_Color(0, 0, 0, 255)) };
    center_sprite(renderer, sprite, m_hmargin - text_margin - sprite.GetWidth() / 2, y);
}

void Plotter::draw_axis_titles(SDL2pp::Renderer& renderer)
{
    if (m_y_title)
    {
        Texture sprite { renderer, m_small_font.RenderUTF8_Blended(*m_y_title, SDL_Color(0, 0, 0, 255)) };
        renderer.Copy(sprite, NullOpt, { 30 + text_margin - sprite.GetWidth() / 2 - small_font_size / 2, m_height / 2 + top_margin }, 270);
    }
    if (m_x_title)
    {
        Texture sprite { renderer, m_small_font.RenderUTF8_Blended(*m_x_title, SDL_Color(0, 0, 0, 255)) };
        center_sprite(renderer, sprite, m_hmargin + m_width / 2, top_margin + m_height + small_font_size + text_margin + small_font_size / 2);
    }
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

void Plotter::plot_collection(Collection const& c, SDL2pp::Renderer& renderer, Texture& into)
{
    if (c.points.size() == 0)
        return;
    renderer.SetDrawColor(c.get_color());
    unordered_map<int, Texture> textures_pool; // OPTIMIZATION : since draw_line rarely needs a lot of Textures of different size, store them
    renderer.SetTarget(into);
    for (size_t i = 0; i < c.points.size() - 1; i++)
    {
        if ((to_plot_x<int>(c.points[i].x) < m_hmargin && to_plot_x<int>(c.points[i + 1].x) < m_hmargin)
            || (to_plot_x<int>(c.points[i].x) > m_hmargin + m_width && to_plot_x<int>(c.points[i + 1].x) > m_hmargin + m_width)
            || (to_plot_y<int>(c.points[i].y) < top_margin && to_plot_y<int>(c.points[i + 1].y) < top_margin)
            || (to_plot_y<int>(c.points[i].y) > top_margin + m_height && to_plot_y<int>(c.points[i + 1].y) > top_margin + m_height))
            continue; // Both points are outside of the screen, and on the same side : there is nothing to draw
        if (c.display_points == DisplayPoints::Yes)
            draw_point(c.points[i].x, c.points[i].y, renderer);
        if (c.display_lines == DisplayLines::Yes)
            draw_line(to_point(c.points[i]), to_point(c.points[i + 1]), renderer, into, textures_pool);
    }
    if (c.display_points == DisplayPoints::Yes)
        draw_point(c.points.back().x, c.points.back().y, renderer);
    renderer.SetTarget();
}

void Plotter::plot_function(Function const& f, SDL2pp::Renderer& renderer, Texture& into)
{
    double x_min = from_plot_x(m_hmargin);
    double x_max = from_plot_x(m_hmargin + m_width);
    vector<Coordinate> coordinates;
    coordinates.reserve(sampling_number_of_points);
    for (int i = 0; i < sampling_number_of_points; i++)
    {
        double v = x_min + i * (x_max - x_min) / sampling_number_of_points;
        coordinates.push_back({ v, f.function(v) });
    }
    plot_collection(Collection { coordinates, f.name, f.color, DisplayPoints::No, DisplayLines::Yes }, renderer, into);
}

Plotter::ScreenPoint Plotter::to_point(Coordinate const& c) const
{
    return { to_plot_x<int64_t>(c.x), to_plot_y<int64_t>(c.y) };
}

bool Plotter::intersect_rect_and_line(int64_t rx, int64_t ry, int64_t rw, int64_t rh, int64_t& x1, int64_t& x2, int64_t& y1, int64_t& y2)
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

void Plotter::draw_line(ScreenPoint const& p1, ScreenPoint const& p2, Renderer& renderer, Texture& into, unordered_map<int, Texture>& textures_pool)
{
    int64_t x1 = p1.x;
    int64_t x2 = p2.x;
    int64_t y1 = p1.y;
    int64_t y2 = p2.y;
    intersect_rect_and_line(m_hmargin, top_margin, m_width, m_height, x1, x2, y1, y2);
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
    auto texture = textures_pool.find(w);
    if (texture == textures_pool.end()) // Makes sure the pool contains the needed size
    {
        Texture sprite { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, 4 * line_width_half };
        renderer.SetTarget(sprite);
        renderer.Clear();
        renderer.SetTarget(into);
        texture = textures_pool.insert({ w, move(sprite) }).first;
    }
    Point dst_point { x1 + static_cast<int64_t>(2. * line_width_half * sin(angle)), y1 + static_cast<int64_t>(-2. * line_width_half * cos(angle)) }; // offset due to rotation
    angle *= 360 / (2 * numbers::pi_v<double>);                                                                                                      // to degree
    renderer.Copy(texture->second, NullOpt, dst_point, angle, Point { 0, 0 });
}

void Plotter::update_mouse_position()
{
    int x;
    int y;
    SDL_GetMouseState(&x, &y);
    if (x_is_in_plot(x) && y_is_in_plot(y))
    {
        m_mouse_x = from_plot_x(x);
        m_mouse_y = from_plot_y(y);
    }
    else
    {
        m_mouse_x = NAN;
        m_mouse_y = NAN;
    }
}

void Plotter::draw_info_box(SDL2pp::Renderer& renderer)
{
    // renderer.SetDrawColor(0, 0, 0, 255);
    // renderer.DrawRect(Rect::FromCorners(m_hmargin, top_margin + m_height + plot_info_margin + x_axis_name_size(), m_hmargin + m_width, top_margin + m_height + plot_info_margin + x_axis_name_size() + info_height())); // Draw the info box, for debug : TODO

    int offset = top_margin + m_height + plot_info_margin + x_axis_name_size() + info_margin;

    vector<pair<SDL_Color, string>> infos;
    infos.reserve(m_collections.size() + m_functions.size());
    for (auto const& e : m_collections)
    {
        infos.push_back({ e.get_color(), e.name });
    }
    for (auto const& e : m_functions)
    {
        infos.push_back({ e.get_color(), e.name });
    }

    size_t i = 0;
    for (; i < infos.size(); i++)
    {
        int hpos = (i % 2 == 0) ? 0 : m_width / 2;
        renderer.SetDrawColor(infos[i].first);
        renderer.FillRect(Rect { m_hmargin + hpos, offset, m_small_font.GetHeight(), m_small_font.GetHeight() });
        string text = infos[i].second;
        if (m_small_font.GetHeight() + info_margin + (text.size() + 1) * m_small_font_advance > (size_t)m_width / 2) // make sure it will not take too much space
        {
            int extra_chars = ((m_small_font.GetHeight() + info_margin + (text.size() + 1) * m_small_font_advance) - m_width / 2) / m_small_font_advance;
            text.resize(text.size() - extra_chars - 4);
            text += "...";
        }
        Texture name = { renderer, m_small_font.RenderUTF8_Blended(text, SDL_Color(0, 0, 0, 255)) };
        renderer.Copy(name, NullOpt, { m_hmargin + hpos + m_small_font.GetHeight() + info_margin, offset });
        offset += (i % 2 == 0) ? 0 : info_margin + m_small_font.GetHeight();
    }
    if (i % 2 == 1)
    {
        offset += info_margin + m_small_font.GetHeight();
    }
    if (!isnan(m_mouse_x))
    {
        string text = "x : " + to_str(m_mouse_x) + ", y : " + to_str(m_mouse_y);
        Texture mouse_sprite { renderer, m_small_font.RenderUTF8_Blended(text, SDL_Color(0, 0, 0, 255)) };
        renderer.Copy(mouse_sprite, NullOpt, { m_hmargin, offset });
    }
}

ColorGenerator::ColorGenerator()
    : m_index(0)
{ }

plotter::Color ColorGenerator::get_color()
{
    plotter::Color c = colors[m_index];
    m_index = (m_index + 1) % nb_colors;
    return c;
}

void Plotter::add_collection(Collection const& c)
{
    m_collections.push_back(c);
    if (!m_collections.back().color.definite)
    {
        m_collections.back().color = m_color_generator.get_color();
    }
}

void Plotter::add_function(Function const& f)
{
    m_functions.push_back(f);
    if (!m_functions.back().color.definite)
    {
        m_functions.back().color = m_color_generator.get_color();
    }
}
void Plotter::set_window(double x, double y, double w, double h)
{
    m_window_defined = true;
    m_x_zoom = (double)m_width / w;
    m_y_zoom = (double)m_height / h;
    m_y_x_ratio = m_y_zoom / m_x_zoom;
    m_x_offset = -(x + w / 2);
    m_y_offset = -(y - h / 2);
}
void Plotter::initialize_zoom_and_offset(Orthonormal orthonormal)
{
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
    if (orthonormal == Orthonormal::Yes)
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

}