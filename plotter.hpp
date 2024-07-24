#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include <iostream>

class Plotter
{
public:
    Plotter()
        : m_running(false)
        , m_x_offset(50.)
        , m_y_offset(20.)
        , m_zoom(1.)
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
    bool m_running;
    float m_x_offset; // offsets are the coordinate of the actual 0 in reference to the original 0
    float m_y_offset;
    float m_zoom;

    int const width = 800;
    int const height = 600;
    int const hmargin = 50;
    int const top_margin = 50;
    int const bottom_margin = 50;
    int const line_width_half = 1;
    float const zoom_factor = 1.3;
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