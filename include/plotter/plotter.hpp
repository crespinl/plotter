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
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <plotter/firacode.hpp>
#include <plotter/notosans.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace sf
{
// Forward declarations for SFML classes
class Color;
class Font;
class RenderTarget;
class VertexArray;
}

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

constexpr Color default_color = Color { };

enum class LineStyle
{
    Solid,
    Dashed,
};

struct Collection
{
    std::vector<Coordinate> points;
    std::string name;
    Color color;
    DisplayPoints display_points;
    DisplayLines display_lines;
    PointType point_type;
    LineStyle line_style;
    Collection(std::vector<double> const& x, std::vector<double> const& y, std::string const& n, DisplayPoints dp = DisplayPoints::Yes, DisplayLines dl = DisplayLines::No, PointType pt = PointType::Square, LineStyle ls = LineStyle::Solid, Color c = default_color)
        : name(n)
        , color(c)
        , display_points(dp)
        , display_lines(dl)
        , point_type(pt)
        , line_style(ls)
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
    Collection(std::vector<Coordinate> const& p, std::string const& n, DisplayPoints dp = DisplayPoints::Yes, DisplayLines dl = DisplayLines::No, PointType pt = PointType::Square, LineStyle ls = LineStyle::Solid, Color c = default_color)
        : points(p)
        , name(n)
        , color(c)
        , display_points(dp)
        , display_lines(dl)
        , point_type(pt)
        , line_style(ls)
    { }
};

struct Function
{
    std::function<double(double)> function;
    std::string name;
    LineStyle line_style { LineStyle::Solid };
    Color color { default_color };
};
enum class ColorPalette : uint8_t
{
    Default = 0,
    Pastel = 1,
    Fire = 2,
    Ice = 3,
};

class ColorGenerator
{
public:
    ColorGenerator(ColorPalette p);
    Color get_color();
    static constexpr std::array<Color, 10> colors_default {
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
    }; // Source : https://en.wikipedia.org/wiki/List_of_colors_(compact)
    static constexpr std::array<Color, 5> colors_pastel {
        Color { 255, 204, 153 }, // Peach orange
        Color { 255, 153, 204 }, // Himalayan balsam
        Color { 204, 153, 255 }, // Lilàs
        Color { 153, 204, 255 }, // Apocyan
        Color { 204, 255, 204 }  // Distilled moss
    }; // Source : https://colorswall.com/fr/palette/239046
    static constexpr std::array<Color, 5> colors_fire {
        Color { 155, 41, 72 },
        Color { 255, 114, 81 },
        Color { 223, 131, 70 },
        Color { 255, 205, 116 },
        Color { 255, 237, 191 },
    }; // Adapted from https://www.color-hex.com/color-palette/1499
    static constexpr std::array<Color, 4> colors_ice {
        Color { 214, 228, 232 },
        Color { 120, 166, 180 },
        Color { 41, 109, 121 },
        Color { 22, 81, 109 },
    }; // Source : https://www.color-hex.com/color-palette/85246

private:
    int m_index;
    ColorPalette m_palette;
};

enum class Orthonormal : bool
{
    Yes = true,
    No = false
};

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
        Color color;
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
    void internal_plot(sf::RenderTarget& target, int x, int y); // (x, y) is the top-left corner of the sub plot in the target
    void initialize();                                          // This has to be called each time before a plot

    void draw_axis(std::tuple<std::vector<Axis>, std::vector<Axis>> const& axis, sf::RenderTarget& target);
    void draw_point(Coordinate c, PointType point_type, sf::Color const& color, sf::VertexArray& triangles, sf::VertexArray& lines); // Absolute coordinates
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
    void draw_vertical_line_number(double nb, int x, sf::RenderTarget& target);
    void draw_horizontal_line_number(double nb, int y, sf::RenderTarget& target);
    void draw_axis_titles(sf::RenderTarget& target);
    void plot_collection(Collection const& c, sf::RenderTarget& target);
    void plot_function(Function const& f, sf::RenderTarget& target);
    ScreenPoint to_point(Coordinate const& c) const;
    void draw_line(ScreenPoint const& p1, ScreenPoint const& p2, LineStyle line_style, sf::Color const& color, sf::VertexArray& triangles, double& length_drawn);
    void initialize_zoom_and_offset();
    void draw_content(sf::RenderTarget& target, int x, int y);
    std::tuple<std::vector<Axis>, std::vector<Axis>> determine_axis();
    double static compute_grid_step(int min_nb, int max_nb, double range);
    bool static intersect_rect_and_line(int64_t rx, int64_t ry, int64_t rw, int64_t rh, int64_t& x1, int64_t& x2, int64_t& y1, int64_t& y2);

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
    int m_small_font_advance;
    int m_x_label_margin;
    int m_bottom_margin;
    int m_title_height { 20 };

    static constexpr int top_margin = 20;
    static constexpr int hmargin = 10;
    static constexpr int line_width_unit = 1;
    static constexpr float line_thickness = 2.f * line_width_unit;
    static constexpr double dash_length = 15.; // In px
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
    Plotter(std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title, ColorPalette p = ColorPalette::Default);
    ~Plotter();
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
    bool internal_plot(bool save, std::string const& name);
    void render(sf::RenderTarget& target);
    std::string static to_str(double nb, int digits = nb_digits);
    int small_font_height() const;
    int info_height() const;
    void draw_info_box(sf::RenderTarget& target);
    size_t hovered_sub_plot() const;
    void add_info_line(SubPlot::InfoLine const& i);
    int base_x_of_hovered_subplot() const;
    int base_y_of_hovered_subplot() const;
    int width() const;
    int height() const;

    std::unique_ptr<sf::Font> m_big_font;
    std::unique_ptr<sf::Font> m_small_font;
    int m_mouse_x;
    int m_mouse_y;
    size_t m_subplot_mouse_selected;
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
    static constexpr unsigned int frame_rate = 25;
    static constexpr unsigned int antialiasing_level = 4;
    static constexpr int out_of_the_screen = -1;
    static constexpr int nb_digits = 5;
    static constexpr int info_box_hmargin = 40;
    static constexpr size_t no_sub_plot_hovered = -1;
};
}
