#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <iostream>
#include <string>

class Plotter
{
public:
    Plotter(std::string const& title)
        : m_running(false)
        , m_x_offset(50.)
        , m_y_offset(20.)
        , m_zoom(1.)
        , m_title(title)
        , m_big_font("./notosans.ttf", big_font_size)
        , m_small_font("./firacode.ttf", small_font_size)
    { }
    bool plot();

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
    bool m_running;
    float m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    float m_y_offset;
    float m_zoom;
    std::string m_title;
    SDL2pp::SDLTTF m_ttf;
    SDL2pp::Font m_big_font;
    SDL2pp::Font m_small_font;

    static constexpr int width = 800;
    static constexpr int height = 600;
    static constexpr int top_margin = 50;
    static constexpr int bottom_margin = 50;
    static constexpr int line_width_half = 1;
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