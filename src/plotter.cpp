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
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <plotter/plotter.hpp>
#include <sstream>

namespace plotter
{

using namespace std;

namespace
{
sf::Color to_sf(plotter::Color const& c)
{
    return sf::Color(c.red, c.green, c.blue);
}

sf::Text make_text(sf::Font const& font, string const& str, unsigned int size)
{
    sf::Text text(font, sf::String::fromUtf8(str.begin(), str.end()), size);
    text.setFillColor(sf::Color::Black);
    return text;
}

void draw_centered(sf::RenderTarget& target, sf::Text& text, float x, float y)
{
    sf::FloatRect const bounds = text.getLocalBounds();
    sf::Vector2f const center = bounds.position + bounds.size / 2.f;
    text.setOrigin({ round(center.x), round(center.y) }); // A fractional origin would put the glyphs between pixels and blur them
    text.setPosition({ round(x), round(y) });
    target.draw(text);
}

// Maps the area of the target starting at (x, y) of size (w, h) to the coordinates starting at (world_x, world_y).
// Everything drawn outside of this area is clipped.
void set_view(sf::RenderTarget& target, float x, float y, float w, float h, float world_x, float world_y)
{
    sf::Vector2f const size(target.getSize());
    sf::View view(sf::FloatRect({ world_x, world_y }, { w, h }));
    view.setViewport(sf::FloatRect({ x / size.x, y / size.y }, { w / size.x, h / size.y }));
    target.setView(view);
}

// Coordinates are in pixels : the rectangle covers the pixels from (x, y) to (x + w - 1, y + h - 1)
void add_rect(sf::VertexArray& triangles, float x, float y, float w, float h, sf::Color const& color)
{
    sf::Vector2f const a { x, y };
    sf::Vector2f const b { x + w, y };
    sf::Vector2f const c { x + w, y + h };
    sf::Vector2f const d { x, y + h };
    for (sf::Vector2f const& p : { a, b, c, a, c, d })
        triangles.append({ p, color });
}

// Inclusive corners
void add_rect_from_corners(sf::VertexArray& triangles, int x1, int y1, int x2, int y2, sf::Color const& color)
{
    add_rect(triangles, x1, y1, x2 - x1 + 1, y2 - y1 + 1, color);
}

// One pixel wide line, between the centers of the pixels (x1, y1) and (x2, y2)
void add_line(sf::VertexArray& lines, int x1, int y1, int x2, int y2, sf::Color const& color)
{
    lines.append({ { x1 + .5f, y1 + .5f }, color });
    lines.append({ { x2 + .5f, y2 + .5f }, color });
}

void add_circle(sf::VertexArray& triangles, int x, int y, float radius, sf::Color const& color)
{
    constexpr int nb_sides = 24;
    sf::Vector2f const center { x + .5f, y + .5f };
    for (int i = 0; i < nb_sides; i++)
    {
        float const a1 = 2.f * numbers::pi_v<float> * i / nb_sides;
        float const a2 = 2.f * numbers::pi_v<float> * (i + 1) / nb_sides;
        triangles.append({ center, color });
        triangles.append({ center + radius * sf::Vector2f { cos(a1), sin(a1) }, color });
        triangles.append({ center + radius * sf::Vector2f { cos(a2), sin(a2) }, color });
    }
}

void add_cross(sf::VertexArray& lines, int x, int y, int length, sf::Color const& color)
{
    add_line(lines, x + length / 2, y, x - length / 2, y, color);
    add_line(lines, x, y + length / 2, x, y - length / 2, color);
}

// A segment of the given thickness, with square caps so that consecutive segments join nicely
void add_thick_segment(sf::VertexArray& triangles, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color const& color)
{
    sf::Vector2f const d = p2 - p1;
    float const length = d.length();
    if (length == 0.f)
        return;
    sf::Vector2f const u = d / length * (thickness / 2.f);
    sf::Vector2f const n { -u.y, u.x };
    sf::Vector2f const offset { .5f, .5f }; // Center of the pixels
    sf::Vector2f const a = p1 - u + n + offset;
    sf::Vector2f const b = p2 + u + n + offset;
    sf::Vector2f const c = p2 + u - n + offset;
    sf::Vector2f const e = p1 - u - n + offset;
    for (sf::Vector2f const& p : { a, b, c, a, c, e })
        triangles.append({ p, color });
}
}

Plotter::Plotter(string const& title, optional<string> x_title, optional<string> y_title, ColorPalette p)
    : m_big_font(make_unique<sf::Font>(notosans_ttf, notosans_ttf_len))
    , m_small_font(make_unique<sf::Font>(firacode_ttf, firacode_ttf_len))
    , m_mouse_x(out_of_the_screen)
    , m_mouse_y(out_of_the_screen)
    , m_subplot_mouse_selected(no_sub_plot_hovered)
    , m_color_generator(p)
    , m_small_font_advance(m_small_font->getGlyph(U' ', small_font_size, false).advance)
    , m_stacking_direction(StackingDirection::Horizontal)
{
    if (m_small_font->getGlyph(U'i', small_font_size, false).advance != m_small_font->getGlyph(U'W', small_font_size, false).advance)
    {
        throw runtime_error("The small font has to be fixed width");
    }
    m_sub_plots.push_back(SubPlot { *this, title, x_title, y_title, m_small_font_advance });
}

Plotter::~Plotter() = default;

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

        sf::Vector2u const size(w, h + plot_info_margin + info_height());
        sf::ContextSettings settings;
        settings.antiAliasingLevel = antialiasing_level;

        m_subplot_mouse_selected = no_sub_plot_hovered;
        m_mouse_x = out_of_the_screen;
        m_mouse_y = out_of_the_screen;

        if (save)
        {
            sf::RenderTexture texture(size, settings);
            render(texture);
            texture.display();
            return texture.getTexture().copyToImage().saveToFile(name + ".png");
        }

        sf::RenderWindow window(sf::VideoMode(size), "Plotter", sf::Style::Default, sf::State::Windowed, settings);
        window.setMinimumSize(sf::Vector2u(min_w, min_h + plot_info_margin + info_height()));
        window.setFramerateLimit(frame_rate);
        auto const arrow_cursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
        auto const size_cursor = sf::Cursor::createFromSystem(sf::Cursor::Type::SizeAll);

        while (window.isOpen())
        {
            while (auto const event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                {
                    window.close();
                }
                else if (auto const* key = event->getIf<sf::Event::KeyPressed>())
                {
                    switch (key->code)
                    {
                    case sf::Keyboard::Key::Right:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_x_move(-10);
                        });
                        break;
                    }
                    case sf::Keyboard::Key::Left:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_x_move(10);
                        });
                        break;
                    }
                    case sf::Keyboard::Key::Up:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_y_move(-10);
                        });
                        break;
                    }
                    case sf::Keyboard::Key::Down:
                    {
                        for_each(m_sub_plots.begin(), m_sub_plots.end(), [](SubPlot& s) {
                            s.event_y_move(10);
                        });
                        break;
                    }
                    default:
                        break;
                    }
                }
                else if (auto const* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
                {
                    if (wheel->wheel != sf::Mouse::Wheel::Vertical)
                        continue;
                    m_mouse_x = wheel->position.x;
                    m_mouse_y = wheel->position.y;
                    size_t hovered = hovered_sub_plot();
                    if (hovered != no_sub_plot_hovered)
                        m_sub_plots[hovered].event_zoom(wheel->delta, m_mouse_x - base_x_of_hovered_subplot(), m_mouse_y - base_y_of_hovered_subplot());
                }
                else if (auto const* move = event->getIf<sf::Event::MouseMoved>())
                {
                    if (m_subplot_mouse_selected != no_sub_plot_hovered)
                    {
                        m_sub_plots[m_subplot_mouse_selected].event_x_move(move->position.x - m_mouse_x);
                        m_sub_plots[m_subplot_mouse_selected].event_y_move(-(move->position.y - m_mouse_y));
                    }
                    m_mouse_x = move->position.x;
                    m_mouse_y = move->position.y;
                }
                else if (auto const* press = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    m_mouse_x = press->position.x;
                    m_mouse_y = press->position.y;
                    m_subplot_mouse_selected = hovered_sub_plot();
                    if (size_cursor)
                        window.setMouseCursor(*size_cursor);
                }
                else if (event->is<sf::Event::MouseButtonReleased>())
                {
                    m_subplot_mouse_selected = no_sub_plot_hovered;
                    if (arrow_cursor)
                        window.setMouseCursor(*arrow_cursor);
                }
                else if (auto const* resize = event->getIf<sf::Event::Resized>())
                {
                    int const new_w = resize->size.x;
                    int const new_h = resize->size.y;
                    for_each(m_sub_plots.begin(), m_sub_plots.end(), [&](SubPlot& s) {
                        // TODO : if step is less than m_sub_plots.size()
                        if (m_stacking_direction == StackingDirection::Vertical)
                            s.event_resize(new_w, (new_h - plot_info_margin - info_height()) / m_sub_plots.size());
                        else
                            s.event_resize(new_w / m_sub_plots.size(), new_h - plot_info_margin - info_height());
                    });
                }
            }

            if (!window.isOpen())
                break;
            render(window);
            window.display();
        }
    }
    catch (exception const& e)
    {
        cerr << e.what() << endl;
        return false;
    }
    return true;
}

void Plotter::render(sf::RenderTarget& target)
{
    target.clear(sf::Color::White);
    sf::Vector2f const size(target.getSize());
    set_view(target, 0, 0, size.x, size.y, 0, 0);
    draw_info_box(target);

    int offset = 0;
    for (auto& e : m_sub_plots)
    {
        if (m_stacking_direction == StackingDirection::Vertical)
        {
            e.internal_plot(target, 0, offset);
            offset += e.height();
        }
        else
        {
            e.internal_plot(target, offset, 0);
            offset += e.width();
        }
    }
}

std::string Plotter::to_str(double nb, int nb_digits)
{
    ostringstream out;
    out << setprecision(nb_digits);
    out << nb;
    return out.str();
}

int Plotter::small_font_height() const
{
    return static_cast<int>(lround(m_small_font->getLineSpacing(small_font_size)));
}

int Plotter::info_height() const
{
    int infos_lines = 1 + m_infos.size() / 2; // The space for mouse coordinates + functions and collections
    if (m_infos.size() % 2 != 0)
        infos_lines += 1; // Add one more line if the number is not even
    return (infos_lines + 1) * info_margin + infos_lines * small_font_height();
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

void Plotter::draw_info_box(sf::RenderTarget& target)
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

    int const font_height = small_font_height();
    int const w = width() - 2 * info_box_hmargin;
    sf::VertexArray squares(sf::PrimitiveType::Triangles);
    size_t i = 0;
    for (; i < m_infos.size(); i++)
    {
        int hpos = (i % 2 == 0) ? 0 : w / 2;
        add_rect(squares, info_box_hmargin + hpos, offset, font_height, font_height, to_sf(m_infos[i].color));
        string text = m_infos[i].name;
        if (font_height + info_margin + (text.size() + 1) * m_small_font_advance > (size_t)w / 2) // make sure it will not take too much space
        {
            int extra_chars = ((font_height + info_margin + (text.size() + 1) * m_small_font_advance) - w / 2) / m_small_font_advance;
            text.resize(text.size() - extra_chars - 4);
            text += "...";
        }
        sf::Text name = make_text(*m_small_font, text, small_font_size);
        name.setPosition(sf::Vector2f(info_box_hmargin + hpos + font_height + info_margin, offset));
        target.draw(name);
        offset += (i % 2 == 0) ? 0 : info_margin + font_height;
    }
    target.draw(squares);
    if (i % 2 == 1)
    {
        offset += info_margin + font_height;
    }

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
    sf::Text mouse_text = make_text(*m_small_font, "x : " + to_str(x) + ", y : " + to_str(y), small_font_size);
    mouse_text.setPosition(sf::Vector2f(info_box_hmargin, offset));
    target.draw(mouse_text);
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

void SubPlot::internal_plot(sf::RenderTarget& target, int x, int y)
{
    if (m_dirty_axis)
    {
        m_axis = determine_axis();
        m_dirty_axis = false;
    }

    set_view(target, x, y, width(), height(), 0, 0);

    sf::RectangleShape box(sf::Vector2f(m_width, m_height)); // Draw the plot box
    box.setPosition(sf::Vector2f(hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size()));
    box.setFillColor(sf::Color::Transparent);
    box.setOutlineColor(sf::Color::Black);
    box.setOutlineThickness(-1.f);
    target.draw(box);

    sf::Text title = make_text(*m_plotter.m_big_font, m_title, m_plotter.big_font_size);
    draw_centered(target, title, hmargin + m_width / 2 + y_axis_name_size() + m_x_label_margin, (top_margin + title_size()) / 2);
    draw_axis_titles(target);
    draw_axis(m_axis, target);
    draw_content(target, x, y);
}

void SubPlot::draw_content(sf::RenderTarget& target, int x, int y)
{
    if (m_width <= 0 || m_height <= 0)
        return;
    int const box_x = hmargin + y_axis_name_size() + m_x_label_margin;
    int const box_y = top_margin + title_size();
    set_view(target, x + box_x, y + box_y, m_width, m_height, box_x, box_y); // Clip everything to the plot box
    for (auto const& e : m_collections)
    {
        plot_collection(e, target);
    }
    for (auto const& e : m_functions)
    {
        plot_function(e, target);
    }
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

void SubPlot::draw_axis(tuple<vector<Axis>, vector<Axis>> const& axis, sf::RenderTarget& target)
{
    sf::Color const main_color(120, 120, 120);
    sf::Color const secondary_color(180, 180, 180);
    sf::VertexArray lines(sf::PrimitiveType::Triangles);
    for (auto& e : get<0>(axis))
    {
        draw_horizontal_line_number(e.coordinate, e.plot_coordinate, target);
        add_rect_from_corners(lines, hmargin + y_axis_name_size() + m_x_label_margin, e.plot_coordinate - line_width_unit, hmargin + y_axis_name_size() + m_x_label_margin + m_width, e.plot_coordinate + line_width_unit, e.is_main ? main_color : secondary_color);
    }

    for (auto& e : get<1>(axis))
    {
        draw_vertical_line_number(e.coordinate, e.plot_coordinate, target);
        add_rect_from_corners(lines, e.plot_coordinate - line_width_unit, top_margin + title_size(), e.plot_coordinate + line_width_unit, top_margin + title_size() + m_height, e.is_main ? main_color : secondary_color);
    }
    target.draw(lines);
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

void SubPlot::draw_point(Coordinate c, PointType point_type, sf::Color const& color, sf::VertexArray& triangles, sf::VertexArray& lines)
{
    int abscissa = to_plot_x<int>(c.x);
    int ordinate = to_plot_y<int>(c.y);
    if (x_is_in_plot(abscissa) && y_is_in_plot(ordinate))
    {
        if (point_type == PointType::Square)
            add_rect_from_corners(triangles, abscissa - half_point_size, ordinate - half_point_size, abscissa + half_point_size, ordinate + half_point_size, color);
        else if (point_type == PointType::Circle)
            add_circle(triangles, abscissa, ordinate, 2 * half_point_size, color);
        else // if (point_type == PointType::Cross)
            add_cross(lines, abscissa, ordinate, 4 * half_point_size, color);
        if (c.x_error != 0.)
        {
            int x_min = to_plot_x<int>(c.x - (c.x_error) / 2);
            int x_max = to_plot_x<int>(c.x + (c.x_error) / 2);
            add_line(lines, x_min, ordinate, x_max, ordinate, color);
            add_line(lines, x_min, ordinate - 2 * half_point_size, x_min, ordinate + 2 * half_point_size, color);
            add_line(lines, x_max, ordinate - 2 * half_point_size, x_max, ordinate + 2 * half_point_size, color);
        }
        if (c.y_error != 0.)
        {
            int y_min = to_plot_y<int>(c.y - (c.y_error) / 2);
            int y_max = to_plot_y<int>(c.y + (c.y_error) / 2);
            add_line(lines, abscissa, y_max, abscissa, y_min, color);
            add_line(lines, abscissa - 2 * half_point_size, y_max, abscissa + 2 * half_point_size, y_max, color);
            add_line(lines, abscissa - 2 * half_point_size, y_min, abscissa + 2 * half_point_size, y_min, color);
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

void SubPlot::draw_vertical_line_number(double nb, int x, sf::RenderTarget& target)
{
    sf::Text text = make_text(*m_plotter.m_small_font, Plotter::to_str(nb), m_plotter.small_font_size);
    draw_centered(target, text, x, top_margin + title_size() + m_height + m_bottom_margin / 2);
}

void SubPlot::draw_horizontal_line_number(double nb, int y, sf::RenderTarget& target)
{
    sf::Text text = make_text(*m_plotter.m_small_font, Plotter::to_str(nb), m_plotter.small_font_size);
    draw_centered(target, text, hmargin + y_axis_name_size() + m_x_label_margin / 2, y);
}

void SubPlot::draw_axis_titles(sf::RenderTarget& target)
{
    if (m_y_title)
    {
        sf::Text text = make_text(*m_plotter.m_small_font, *m_y_title, m_plotter.small_font_size);
        text.setRotation(sf::degrees(-90.f));
        draw_centered(target, text, hmargin + y_axis_name_size() / 2, top_margin + title_size() + m_height / 2);
    }
    if (m_x_title)
    {
        sf::Text text = make_text(*m_plotter.m_small_font, *m_x_title, m_plotter.small_font_size);
        draw_centered(target, text, hmargin + y_axis_name_size() + m_x_label_margin + m_width / 2, top_margin + title_size() + m_height + m_bottom_margin + x_axis_name_size() / 2);
    }
}

void SubPlot::plot_collection(Collection const& c, sf::RenderTarget& target)
{
    if (c.points.size() == 0)
        return;

    sf::Color const color = to_sf(c.color);
    sf::VertexArray line_triangles(sf::PrimitiveType::Triangles);
    sf::VertexArray point_triangles(sf::PrimitiveType::Triangles);
    sf::VertexArray point_lines(sf::PrimitiveType::Lines);

    double length_drawn = 0.;
    for (size_t i = 0; i < c.points.size() - 1; i++)
    {
        if ((to_plot_x<int>(c.points[i].x) < hmargin + y_axis_name_size() + m_x_label_margin && to_plot_x<int>(c.points[i + 1].x) < hmargin + y_axis_name_size() + m_x_label_margin)
            || (to_plot_x<int>(c.points[i].x) > hmargin + y_axis_name_size() + m_x_label_margin + m_width && to_plot_x<int>(c.points[i + 1].x) > hmargin + y_axis_name_size() + m_x_label_margin + m_width)
            || (to_plot_y<int>(c.points[i].y) < top_margin + title_size() && to_plot_y<int>(c.points[i + 1].y) < top_margin + title_size())
            || (to_plot_y<int>(c.points[i].y) > top_margin + title_size() + m_height && to_plot_y<int>(c.points[i + 1].y) > top_margin + title_size() + m_height))
            continue; // Both points are outside of the screen, and on the same side : there is nothing to draw
        if (c.display_points == DisplayPoints::Yes)
            draw_point(c.points[i], c.point_type, color, point_triangles, point_lines);
        if (c.display_lines == DisplayLines::Yes)
            draw_line(to_point(c.points[i]), to_point(c.points[i + 1]), c.line_style, color, line_triangles, length_drawn);
    }
    if (c.display_points == DisplayPoints::Yes)
        draw_point(c.points.back(), c.point_type, color, point_triangles, point_lines);

    target.draw(line_triangles);
    target.draw(point_triangles);
    target.draw(point_lines);
}

void SubPlot::plot_function(Function const& f, sf::RenderTarget& target)
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
    plot_collection(Collection { coordinates, f.name, DisplayPoints::No, DisplayLines::Yes, PointType::Square, f.line_style, f.color }, target);
}

SubPlot::ScreenPoint SubPlot::to_point(Coordinate const& c) const
{
    return { to_plot_x<int64_t>(c.x), to_plot_y<int64_t>(c.y) };
}

bool SubPlot::intersect_rect_and_line(int64_t rx, int64_t ry, int64_t rw, int64_t rh, int64_t& x1, int64_t& x2, int64_t& y1, int64_t& y2)
{
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

void SubPlot::draw_line(ScreenPoint const& p1, ScreenPoint const& p2, LineStyle line_style, sf::Color const& color, sf::VertexArray& triangles, double& length_drawn)
{
    int64_t x1 = p1.x;
    int64_t x2 = p2.x;
    int64_t y1 = p1.y;
    int64_t y2 = p2.y;
    if (!intersect_rect_and_line(hmargin + y_axis_name_size() + m_x_label_margin, top_margin + title_size(), m_width, m_height, x1, x2, y1, y2))
        return;
    sf::Vector2f const start(x1, y1);
    sf::Vector2f const end(x2, y2);
    double const length = (end - start).length();
    if (line_style == LineStyle::Solid)
    {
        add_thick_segment(triangles, start, end, line_thickness, color);
    }
    else if (line_style == LineStyle::Dashed)
    {
        // Only the first half of each period of 2 * dash_length is drawn, and the phase is kept from one segment to the next
        sf::Vector2f const direction = (end - start) / static_cast<float>(length);
        double pos = 0.;
        while (pos < length)
        {
            double const phase = fmod(length_drawn + pos, 2 * dash_length);
            double const next = min(length, pos + (phase < dash_length ? dash_length - phase : 2 * dash_length - phase));
            if (phase < dash_length)
                add_thick_segment(triangles, start + direction * static_cast<float>(pos), start + direction * static_cast<float>(next), line_thickness, color);
            pos = next;
        }
    }
    length_drawn += length;
}

void SubPlot::add_collection(Collection const& c)
{
    m_collections.push_back(c);
    if (!m_collections.back().color.definite)
    {
        m_collections.back().color = m_plotter.m_color_generator.get_color();
    }
    m_plotter.add_info_line(InfoLine { m_collections.back().name, m_collections.back().color });
}

void SubPlot::add_function(Function const& f)
{
    m_functions.push_back(f);
    if (!m_functions.back().color.definite)
    {
        m_functions.back().color = m_plotter.m_color_generator.get_color();
    }
    m_plotter.add_info_line(InfoLine { m_functions.back().name, m_functions.back().color });
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
    m_title_height = static_cast<int>(lround(m_plotter.m_big_font->getLineSpacing(m_plotter.big_font_size)));
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
        r.push_back({ e.name, e.color });
    }
    for (auto const& e : m_functions)
    {
        r.push_back({ e.name, e.color });
    }
    return r;
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
