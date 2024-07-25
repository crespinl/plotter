#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Coordinate
{
    float x;
    float y;
};

struct Collection
{
    std::vector<Coordinate> points;
    std::string name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    bool draw_points { true };
    bool draw_lines { false };
    SDL2pp::Color color() const { return SDL2pp::Color(red, green, blue, 255); }
};

class Plotter
{
public:
    Plotter(std::string const& title)
        : m_running(false)
        , m_x_offset(-5.)
        , m_y_offset(-5.)
        , m_zoom(50.)
        , m_title(title)
        , m_big_font("./notosans.ttf", big_font_size)
        , m_small_font("./firacode.ttf", small_font_size)
    { }
    bool plot();
    void add_collection(Collection const& c) { m_collections.push_back(c); }

private:
    void draw_axis(SDL2pp::Renderer& renderer);
    void draw_point(int x, int y, SDL2pp::Renderer& renderer); // Absolute coordinates
    int compute_x_offset() const { return m_x_offset * m_zoom; }
    int compute_y_offset() const { return m_y_offset * m_zoom; }
    int to_plot_x(float x) const;
    int to_plot_y(float y) const;
    float from_plot_x(int x) const;
    float from_plot_y(int x) const;
    bool x_is_in_plot(int x) const;
    bool y_is_in_plot(int y) const;
    void draw_vertical_line_number(float nb, int x, SDL2pp::Renderer& renderer);
    void draw_horizontal_line_number(float nb, int y, SDL2pp::Renderer& renderer);
    void static center_sprite(SDL2pp::Renderer& renderer, SDL2pp::Texture& texture, int x, int y);
    std::string to_str(float nb);
    void plot_collection(Collection const& c, SDL2pp::Renderer& renderer, SDL2pp::Texture& into);
    SDL2pp::Point to_point(Coordinate const& c) const;
    void draw_line(SDL2pp::Point const& p1, SDL2pp::Point const& p2, SDL2pp::Renderer& renderer, SDL2pp::Texture& into);
    bool m_running;
    float m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    float m_y_offset;
    float m_zoom;
    std::string m_title;
    SDL2pp::SDLTTF m_ttf;
    SDL2pp::Font m_big_font;
    SDL2pp::Font m_small_font;
    std::vector<Collection> m_collections;

    static constexpr int width = 800;
    static constexpr int height = 600;
    static constexpr int top_margin = 50;
    static constexpr int bottom_margin = 50;
    static constexpr int line_width_half = 1;
    static constexpr int half_point_size = 4;
    static constexpr float zoom_factor = 1.3;
    static constexpr int big_font_size = 24;
    static constexpr int small_font_size = 15;
    static constexpr int text_margin = 5;
    static constexpr int hmargin = 5 * small_font_size;
};

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