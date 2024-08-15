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
#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <plotter/firacode.hpp>
#include <plotter/notosans.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace plotter
{

struct Coordinate
{
    double x;
    double y;
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

struct Collection
{
    std::vector<Coordinate> points;
    std::string name;
    Color color;
    DisplayPoints display_points { DisplayPoints::Yes };
    DisplayLines display_lines { DisplayLines::No };
    SDL2pp::Color get_color() const { return SDL2pp::Color(color.red, color.green, color.blue, 255); }
};

constexpr Color default_color = Color {};

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


class Plotter
{
public:
    Plotter(std::string const& title, std::optional<std::string> x_title, std::optional<std::string> y_title)
        : m_width(800)
        , m_height(600)
        , m_running(false)
        , m_title(title)
        , m_x_title(x_title)
        , m_y_title(y_title)
        , m_big_font_ops(SDL2pp::RWops::FromConstMem(notosans_ttf, notosans_ttf_len))
        , m_small_font_ops(SDL2pp::RWops::FromConstMem(firacode_ttf, firacode_ttf_len))
        , m_big_font(m_big_font_ops, big_font_size)
        , m_small_font(m_small_font_ops, small_font_size)
        , m_mouse_x(NAN)
        , m_mouse_y(NAN)
        , m_size_cursor(nullptr)
        , m_arrow_cursor(nullptr)
        , m_small_font_advance(m_small_font.GetGlyphAdvance(' '))
        , m_hmargin(30 + 5 * m_small_font_advance + y_axis_name_size())
    {
        if (!m_small_font.IsFixedWidth())
        {
            throw std::runtime_error("The small font has to be fixed width");
        }
    }
    bool plot(Orthonormal orthonormal = Orthonormal::No);
    void add_collection(Collection const& c);

private:
    void draw_axis(SDL2pp::Renderer& renderer);
    void draw_point(double x, double y, SDL2pp::Renderer& renderer); // Absolute coordinates
    int to_plot_x(double x) const;
    int to_plot_y(double y) const;
    double from_plot_x(int x) const;
    double from_plot_y(int x) const;
    bool x_is_in_plot(int x) const;
    bool y_is_in_plot(int y) const;
    void draw_vertical_line_number(double nb, int x, SDL2pp::Renderer& renderer);
    void draw_horizontal_line_number(double nb, int y, SDL2pp::Renderer& renderer);
    void draw_axis_titles(SDL2pp::Renderer& renderer);
    void static center_sprite(SDL2pp::Renderer& renderer, SDL2pp::Texture& texture, int x, int y);
    std::string to_str(double nb);
    void plot_collection(Collection const& c, SDL2pp::Renderer& renderer, SDL2pp::Texture& into);
    SDL2pp::Point to_point(Coordinate const& c) const;
    void draw_line(SDL2pp::Point const& p1, SDL2pp::Point const& p2, SDL2pp::Renderer& renderer, SDL2pp::Texture& into, std::unordered_map<int, SDL2pp::Texture>& textures_pool);
    int info_height() const { return (2 + m_collections.size()) * info_margin + (1 + m_collections.size() / 2) * m_small_font.GetHeight(); }
    void update_mouse_position();
    void draw_info_box(SDL2pp::Renderer& renderer);
    void initialize_zoom_and_offset(Orthonormal orthonormal);
    void draw_content(SDL2pp::Renderer& renderer);
    int x_axis_name_size() const;
    int y_axis_name_size() const;
    double static compute_grid_step(int min_nb, int max_nb, double range);

    int m_width;
    int m_height;
    bool m_running;
    double m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    double m_y_offset;
    double m_x_zoom;
    double m_y_x_ratio; // This is the interesting data
    double m_y_zoom;    // This is a cached value
    std::string m_title;
    std::optional<std::string> m_x_title;
    std::optional<std::string> m_y_title;
    SDL2pp::SDLTTF m_ttf;
    SDL2pp::RWops m_big_font_ops;
    SDL2pp::RWops m_small_font_ops;
    SDL2pp::Font m_big_font;
    SDL2pp::Font m_small_font;
    std::vector<Collection> m_collections;
    double m_mouse_x;
    double m_mouse_y;
    bool m_mouse_down;
    SDL_Cursor* m_size_cursor;
    SDL_Cursor* m_arrow_cursor;
    ColorGenerator m_color_generator;
    int m_small_font_advance;
    int m_hmargin;

    static constexpr int top_margin = 50;
    static constexpr int plot_info_margin = 25;
    static constexpr int info_margin = 5;
    static constexpr int bottom_margin = 25;
    static constexpr int line_width_half = 1;
    static constexpr int half_point_size = 4;
    static constexpr double zoom_factor = 1.3;
    static constexpr int big_font_size = 24;
    static constexpr int small_font_size = 15;
    static constexpr int text_margin = 5;
    static constexpr int min_width = 160;
    static constexpr int min_height = 120;
    static constexpr double min_spacing_between_axis = 80; // In px
    static constexpr double max_spacing_between_axis = 200; // In px
};
}

/*
Warning :
for SDL, axis are like that :

0
------------> x
|
|
|
|
|
\/
y

the idea is to have Plotter coordinates work that way :

            y
            /\
            |
            |
            |0
-----------------------> x
            |
            |
            |
*/
