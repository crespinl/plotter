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
#pragma once
#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <plotter/firacode.hpp>
#include <plotter/notosans.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace plotter
{

struct Coordinate
{
    double x;
    double y;
    double x_error { 0. };
    double y_error { 0. };
};

struct Color
{
    constexpr Color()
        : red(255)
        , green(255)
        , blue(255)
        , definite(false)
    { }
    constexpr Color(uint8_t r, uint8_t g, uint8_t b)
        : red(r)
        , green(g)
        , blue(b)
        , definite(true)
    { }
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    bool definite;
};

enum class DisplayPoints : bool
{
    Yes = true,
    No = false
};

enum class DisplayLines : bool
{
    Yes = true,
    No = false
};

enum class PointType
{
    Square,
    Circle,
    Cross,
};

constexpr Color default_color = Color {};

struct Collection
{
    std::vector<Coordinate> points;
    std::string name;
    Color color;
    DisplayPoints display_points;
    DisplayLines display_lines;
    PointType point_type;
    SDL2pp::Color get_color() const { return SDL2pp::Color(color.red, color.green, color.blue, 255); }
    Collection(std::vector<double> const& x, std::vector<double> const& y, std::string const& n, DisplayPoints dp = DisplayPoints::Yes, DisplayLines dl = DisplayLines::No, PointType pt = PointType::Square, Color c = default_color)
        : name(n)
        , color(c)
        , display_points(dp)
        , display_lines(dl)
        , point_type(pt)
    {
        if (x.size() != y.size())
        {
            throw std::runtime_error("x and y must have the same size");
        }
        points.reserve(x.size());
        for (size_t i = 0; i < x.size(); i++)
        {
            points.push_back({ x[i], y[i] });
        }
    }
    Collection(std::vector<Coordinate> const& p, std::string const& n, DisplayPoints dp = DisplayPoints::Yes, DisplayLines dl = DisplayLines::No, PointType pt = PointType::Square, Color c = default_color)
        : points(p)
        , name(n)
        , color(c)
        , display_points(dp)
        , display_lines(dl)
        , point_type(pt)
    { }
};

struct Function
{
    std::function<double(double)> function;
    std::string name;
    Color color { default_color };
    SDL2pp::Color get_color() const { return SDL2pp::Color(color.red, color.green, color.blue, 255); }
};

class ColorGenerator
{
public:
    ColorGenerator();
    Color get_color();
    static constexpr int nb_colors = 10;
    static constexpr Color colors[nb_colors] {
        Color { 0, 72, 186 },    // Absolute zero
        Color { 219, 45, 67 },   // Alizarin
        Color { 123, 182, 97 },  // Bud green
        Color { 230, 103, 206 }, // Brilliant rose
        Color { 150, 75, 0 },    // Brown
        Color { 204, 85, 0 },    // Burnt orange
        Color { 189, 51, 164 },  // Byzantine
        Color { 75, 54, 33 },    // Café noir
        Color { 255, 239, 0 },   // Canary yellow
        Color { 209, 190, 168 }  // Dark vanilla
    };
    // Source : https://en.wikipedia.org/wiki/List_of_colors_(compact)
private:
    int m_index;
};

enum class Orthonormal : bool
{
    Yes = true,
    No = false
};

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
constexpr uint32_t R_MASK = 0xff000000;
constexpr uint32_t G_MASK = 0x00ff0000;
constexpr uint32_t B_MASK = 0x0000ff00;
constexpr uint32_t A_MASK = 0x000000ff;
#else
constexpr uint32_t R_MASK = 0x000000ff;
constexpr uint32_t G_MASK = 0x0000ff00;
constexpr uint32_t B_MASK = 0x00ff0000;
constexpr uint32_t A_MASK = 0xff000000;
#endif

class Plotter;

class SubPlot
{
public:
    SubPlot(Plotter& plotter, std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title, int font_advance)
        : m_plotter(plotter)
        , m_width(800)
        , m_height(600)
        , m_title(title)
        , m_x_title(x_title)
        , m_y_title(y_title)
        , m_window_defined(false)
        , m_dirty_axis(true)
        , m_orthonormal(Orthonormal::No)
        , m_small_font_advance(font_advance)
    { }
    void add_collection(Collection const& c);
    template<class... Args>
    void emplace_collection(Args&&... args)
    {
        add_collection(Collection { std::forward<Args>(args)... });
    }
    void add_function(Function const& f);
    template<class... Args>
    void emplace_function(Args&&... args)
    {
        add_function(Function { std::forward<Args>(args)... });
    }
    void set_window(double x, double y, double w, double h); // (x, y) are the coordinates of the top-left point
    void set_orthonormal(Orthonormal o = Orthonormal::Yes) { m_orthonormal = o; }

private:
    friend class Plotter;
    struct ScreenPoint
    {
        int64_t x;
        int64_t y;
    };
    struct Axis
    {
        int plot_coordinate;
        double coordinate;
        bool is_main;
    };
    struct InfoLine
    {
        std::string name;
        SDL_Color color;
    };

    // Methods that get called by Plotter
    void event_x_move(int x);
    void event_y_move(int y);
    void event_zoom(float mouse_wheel, int mouse_x, int mouse_y);
    void event_resize(int w, int h);
    int min_width() const;
    int min_height() const;
    int width() const;
    int height() const;
    std::vector<InfoLine> infos() const;
    std::unique_ptr<SDL2pp::Texture> internal_plot(SDL2pp::Renderer& renderer);
    void initialize(); // This has to be called each time before a plot

    void draw_axis(std::tuple<std::vector<Axis>, std::vector<Axis>> const& axis, SDL2pp::Renderer& renderer);
    void draw_point(Coordinate c, SDL2pp::Renderer& renderer, PointType point_type); // Absolute coordinates
    template<typename T>
    T to_plot_x(double x) const
    {
        return static_cast<T>(m_width / 2 + x * m_x_zoom + m_x_offset * m_x_zoom + hmargin + y_axis_name_size() + m_x_label_margin);
    }
    template<typename T>
    T to_plot_y(double y) const
    {
        return static_cast<T>(m_height / 2 - y * m_y_zoom - m_y_offset * m_y_zoom + top_margin + title_size());
    }
    double from_plot_x(int x) const;
    double from_plot_y(int x) const;
    bool x_is_in_plot(int x) const;
    bool y_is_in_plot(int y) const;
    void draw_vertical_line_number(double nb, int x, SDL2pp::Renderer& renderer);
    void draw_horizontal_line_number(double nb, int y, SDL2pp::Renderer& renderer);
    void draw_axis_titles(SDL2pp::Renderer& renderer);
    void plot_collection(Collection const& c, SDL2pp::Renderer& renderer, SDL2pp::Texture& into);
    void plot_function(Function const& f, SDL2pp::Renderer& renderer, SDL2pp::Texture& into);
    ScreenPoint to_point(Coordinate const& c) const;
    void draw_line(ScreenPoint const& p1, ScreenPoint const& p2, SDL2pp::Renderer& renderer, SDL2pp::Texture& into, std::unordered_map<int, SDL2pp::Texture>& textures_pool);
    void initialize_zoom_and_offset();
    void draw_content(SDL2pp::Renderer& renderer);
    std::tuple<std::vector<Axis>, std::vector<Axis>> determine_axis();
    double static compute_grid_step(int min_nb, int max_nb, double range);
    bool static intersect_rect_and_line(int64_t rx, int64_t ry, int64_t rw, int64_t rh, int64_t& x1, int64_t& x2, int64_t& y1, int64_t& y2);
    void static draw_circle(SDL2pp::Renderer& renderer, int x, int y, int radius);
    void static draw_cross(SDL2pp::Renderer& renderer, int x, int y, int length);

    int title_size() const;
    int x_axis_name_size() const;
    int y_axis_name_size() const;

    Plotter& m_plotter;
    int m_width;
    int m_height;
    double m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    double m_y_offset;
    double m_x_zoom;
    double m_y_x_ratio; // This is the interesting data
    double m_y_zoom;    // This is a cached value
    std::string m_title;
    std::optional<std::string> m_x_title;
    std::optional<std::string> m_y_title;
    std::vector<Collection> m_collections;
    std::vector<Function> m_functions;
    bool m_window_defined;
    std::tuple<std::vector<Axis>, std::vector<Axis>> m_axis;
    bool m_dirty_axis;
    Orthonormal m_orthonormal;
    std::unique_ptr<SDL2pp::Texture> m_texture;
    int m_small_font_advance;
    int m_x_label_margin;
    int m_bottom_margin;
    int m_title_height { 20 };

    static constexpr int top_margin = 20;
    static constexpr int hmargin = 10;
    static constexpr int line_width_half = 1;
    static constexpr int half_point_size = 4;
    static constexpr double zoom_factor = 1.3;
    static constexpr int plot_min_width = 160;
    static constexpr int plot_min_height = 120;
    static constexpr double min_spacing_between_axis = 80;  // In px
    static constexpr double max_spacing_between_axis = 200; // In px
    static constexpr int sampling_number_of_points = 5'000;
};

enum class StackingDirection
{
    Horizontal,
    Vertical
};

class Plotter
{
public:
    friend class SubPlot;
    Plotter(std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title)
        : m_running(false)
        , m_big_font_ops(SDL2pp::RWops::FromConstMem(notosans_ttf, notosans_ttf_len))
        , m_small_font_ops(SDL2pp::RWops::FromConstMem(firacode_ttf, firacode_ttf_len))
        , m_big_font(m_big_font_ops, big_font_size)
        , m_small_font(m_small_font_ops, small_font_size)
        , m_subplot_mouse_selected(-1)
        , m_size_cursor(nullptr)
        , m_arrow_cursor(nullptr)
        , m_small_font_advance(m_small_font.GetGlyphAdvance(' '))
        , m_stacking_direction(StackingDirection::Horizontal)
    {
        construct(title, x_title, y_title);
    }
    bool plot();
    bool save(std::string const& name);
    void add_collection(Collection const& c, int n = 0);
    template<int n = 0, class... Args>
    void emplace_collection(Args&&... args)
    {
        add_collection(Collection { std::forward<Args>(args)... }, n);
    }
    void add_function(Function const& f, int n = 0);
    template<int n = 0, class... Args>
    void emplace_function(Args&&... args)
    {
        add_function(Function { std::forward<Args>(args)... }, n);
    }
    void set_window(double x, double y, double w, double h, int n = 0); // (x, y) are the coordinates of the top-left point
    SubPlot& add_sub_plot(std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title);
    void set_stacking_direction(StackingDirection d) { m_stacking_direction = d; }

private:
    friend class SubPlot;
    struct ScreenPoint
    {
        int64_t x;
        int64_t y;
    };
    struct Axis
    {
        int plot_coordinate;
        double coordinate;
        bool is_main;
    };
    void construct(std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title);
    bool internal_plot(bool save, std::string const& name);
    void static center_sprite(SDL2pp::Renderer& renderer, SDL2pp::Texture& texture, int x, int y);
    std::string static to_str(double nb, int digits = nb_digits);
    int info_height() const;
    void draw_info_box(SDL2pp::Renderer& renderer);
    void static save_img(SDL2pp::Window const& window, SDL2pp::Renderer& renderer, std::string name);
    void update_mouse_position();
    size_t hovered_sub_plot() const;
    void add_info_line(SubPlot::InfoLine const& i);
    int base_x_of_hovered_subplot() const;
    int base_y_of_hovered_subplot() const;
    int width() const;
    int height() const;

    bool m_running;
    SDL2pp::SDLTTF m_ttf;
    SDL2pp::RWops m_big_font_ops;
    SDL2pp::RWops m_small_font_ops;
    SDL2pp::Font m_big_font;
    SDL2pp::Font m_small_font;
    int m_mouse_x;
    int m_mouse_y;
    size_t m_subplot_mouse_selected;
    SDL_Cursor* m_size_cursor;
    SDL_Cursor* m_arrow_cursor;
    ColorGenerator m_color_generator;
    int m_small_font_advance;
    std::vector<SubPlot::InfoLine> m_infos;
    std::vector<SubPlot> m_sub_plots;
    StackingDirection m_stacking_direction;

    static constexpr int plot_info_margin = 10;
    static constexpr int info_margin = 5;
    static constexpr int big_font_size = 24;
    static constexpr int small_font_size = 15;
    static constexpr int text_margin = 5;
    static constexpr int min_width = 160;
    static constexpr int min_height = 120;
    static constexpr int out_of_the_screen = -1;
    static constexpr int nb_digits = 5;
    static constexpr int info_box_hmargin = 40;
    static constexpr size_t no_sub_plot_hovered = -1;
};
}
