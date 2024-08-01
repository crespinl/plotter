#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Coordinate
{
    float x;
    float y;
};

struct Collection
{
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
    std::vector<Coordinate> points;
    std::string name;
    Color color;
    bool draw_points { true };
    bool draw_lines { false };
    SDL2pp::Color get_color() const { return SDL2pp::Color(color.red, color.green, color.blue, 255); }
};

class ColorGenerator
{
public:
    ColorGenerator();
    Collection::Color get_color();
    static constexpr int nb_colors = 10;
    static constexpr Collection::Color colors[nb_colors] {
        Collection::Color { 0, 72, 186 },    // Absolute zero
        Collection::Color { 219, 45, 67 },   // Alizarin
        Collection::Color { 123, 182, 97 },  // Bud green
        Collection::Color { 230, 103, 206 }, // Brilliant rose
        Collection::Color { 150, 75, 0 },    // Brown
        Collection::Color { 204, 85, 0 },    // Burnt orange
        Collection::Color { 189, 51, 164 },  // Byzantine
        Collection::Color { 75, 54, 33 },    // Café noir
        Collection::Color { 255, 239, 0 },   // Canary yellow
        Collection::Color { 209, 190, 168 }  // Dark vanilla
    };
    // Source : https://en.wikipedia.org/wiki/List_of_colors_(compact)
private:
    int m_index;
};

class Plotter
{
public:
    Plotter(std::string const& title)
        : m_running(false)
        , m_title(title)
        , m_big_font("./notosans.ttf", big_font_size)
        , m_small_font("./firacode.ttf", small_font_size)
        , m_mouse_x(NAN)
        , m_mouse_y(NAN)
        , m_size_cursor(nullptr)
        , m_arrow_cursor(nullptr)
        , m_small_font_advance(m_small_font.GetGlyphAdvance(' '))
        , m_hmargin(30 + 5 * m_small_font_advance)
    {
        if (!m_small_font.IsFixedWidth())
        {
            throw std::runtime_error("The small font has to be fixed width");
        }
    }
    bool plot();
    void add_collection(Collection const& c);

private:
    void draw_axis(SDL2pp::Renderer& renderer);
    void draw_point(float x, float y, SDL2pp::Renderer& renderer); // Absolute coordinates
    int compute_x_offset() const { return m_x_offset * m_x_zoom; }
    int compute_y_offset() const { return m_y_offset * y_zoom(); }
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
    int info_height() const { return (2 + m_collections.size()) * info_margin + (1 + m_collections.size()) * m_small_font.GetHeight(); }
    void update_mouse_position();
    void draw_info_box(SDL2pp::Renderer& renderer);
    float y_zoom() const { return m_x_zoom * m_y_x_ratio; }
    void initialize_zoom_and_offset();
    bool m_running;
    double m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    double m_y_offset;
    float m_x_zoom;
    float m_y_x_ratio;
    std::string m_title;
    SDL2pp::SDLTTF m_ttf;
    SDL2pp::Font m_big_font;
    SDL2pp::Font m_small_font;
    std::vector<Collection> m_collections;
    float m_mouse_x;
    float m_mouse_y;
    bool m_mouse_down;
    SDL_Cursor* m_size_cursor;
    SDL_Cursor* m_arrow_cursor;
    ColorGenerator m_color_generator;
    int m_small_font_advance;
    int m_hmargin;

    static constexpr int width = 800;
    static constexpr int height = 600;
    static constexpr int top_margin = 50;
    static constexpr int plot_info_margin = 25;
    static constexpr int info_margin = 5;
    static constexpr int bottom_margin = 25;
    static constexpr int line_width_half = 1;
    static constexpr int half_point_size = 4;
    static constexpr float zoom_factor = 1.3;
    static constexpr int big_font_size = 24;
    static constexpr int small_font_size = 15;
    static constexpr int text_margin = 5;
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