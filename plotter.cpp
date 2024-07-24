#include <plotter.hpp>

using namespace std;
using namespace SDL2pp;

bool Plotter::plot()
{
    try
    {
        SDL sdl(SDL_INIT_VIDEO);
        Window window("Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 2 * hmargin + width, top_margin + height + bottom_margin, SDL_WINDOW_SHOWN);
        Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

        m_running = true;
        SDL_Event event;
        while (m_running)
        {
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
                        m_x_offset -= 10 / m_zoom;
                        break;
                    }
                    case SDLK_LEFT:
                    {
                        m_x_offset += 10 / m_zoom;
                        break;
                    }
                    case SDLK_UP:
                    {
                        m_y_offset -= 10 / m_zoom;
                        break;
                    }
                    case SDLK_DOWN:
                    {
                        m_y_offset += 10 / m_zoom;
                        break;
                    }
                    default:
                        break;
                    }
                }
                else if (event.type == SDL_MOUSEWHEEL)
                {
                    m_zoom = m_zoom * pow(zoom_factor, event.wheel.preciseY);
                }
            }

            renderer.SetDrawColor(255, 255, 255, 255); // Clear the screen
            renderer.Clear();

            renderer.SetDrawColor(0, 0, 0, 255); // Draw the plot box
            renderer.DrawRect(Rect::FromCorners(hmargin, top_margin, hmargin + width, top_margin + height));

            draw_axis(renderer);
            renderer.SetDrawColor(255, 0, 0, 255);
            draw_point(20, 20, renderer);

            renderer.Present();
            SDL_Delay(10); // Limit number of frames
        }
    }
    catch (exception const& e)
    {
        cerr << e.what() << endl;
        return false;
    }
    return true;
}

void Plotter::draw_axis(Renderer& renderer)
{
    int x_offset = compute_x_offset();
    int y_offset = compute_y_offset();
    // Draw secondary axis :
    int nb_vertical_axis = width / 130;
    int nb_horizontal_axis = height / 130;

    float x_max = from_plot_x(width);
    float x_min = from_plot_x(0);
    float y_max = from_plot_y(0);
    float y_min = from_plot_y(height);

    float delta_x = x_max - x_min;
    float delta_y = y_max - y_min;

    float x_step = pow(10., round(log10(delta_x) - 1));
    float y_step = pow(10., round(log10(delta_y) - 1));

    int nb_vertical = delta_x / x_step + 1;
    int nb_horizontal = delta_y / y_step + 1;

    x_step *= nb_vertical / nb_vertical_axis + 1;
    y_step *= nb_horizontal / nb_horizontal_axis + 1;

    float rounded_x_min = round(x_min / x_step) * x_step;
    float rounded_y_min = round(y_min / y_step) * y_step;

    renderer.SetDrawColor(180, 180, 180, 255);
    for (int i = 0; i < nb_horizontal_axis; i++)
    {
        int ordinate = to_plot_y(rounded_y_min + i * y_step);
        if (y_is_in_plot(ordinate))
            renderer.FillRect(Rect::FromCorners(hmargin, ordinate - line_width_half, hmargin + width, ordinate + line_width_half));
    }
    for (int i = 0; i < nb_vertical_axis; i++)
    {
        int abscissa = to_plot_x(rounded_x_min + i * x_step);
        if (x_is_in_plot(abscissa))
            renderer.FillRect(Rect::FromCorners(abscissa - line_width_half, top_margin, abscissa + line_width_half, top_margin + height));
    }

    // Draw main axis :
    renderer.SetDrawColor(120, 120, 120, 255);
    if (y_is_in_plot(to_plot_y(0)))
        renderer.FillRect(Rect::FromCorners(hmargin, to_plot_y(0) - line_width_half, hmargin + width, to_plot_y(0) + line_width_half)); // abscissa
    if (x_is_in_plot(to_plot_x(0)))
        renderer.FillRect(Rect::FromCorners(to_plot_x(0) - line_width_half, top_margin, to_plot_x(0) + line_width_half, top_margin + height)); // ordinate
}

void Plotter::draw_point(int x, int y, Renderer& renderer)
{
    int abscissa = to_plot_x(x);
    int ordinate = to_plot_y(y);
    if (x_is_in_plot(abscissa) && y_is_in_plot(ordinate))
        renderer.FillRect(Rect::FromCorners(abscissa - 5, ordinate - 5, abscissa + 5, ordinate + 5));
}

int Plotter::to_plot_x(float x) const
{
    return width / 2 + x * m_zoom + compute_x_offset() + hmargin;
}
int Plotter::to_plot_y(float y) const
{
    return height / 2 - y * m_zoom - compute_y_offset() + top_margin;
}
float Plotter::from_plot_x(int x) const
{
    return ((float)x - hmargin - (float)width / 2.) / m_zoom - m_x_offset;
}
float Plotter::from_plot_y(int x) const
{
    return (-x + top_margin + (float)height / 2.) / m_zoom - m_y_offset;
}
bool Plotter::x_is_in_plot(int x) const
{
    return x > hmargin && x < hmargin + width;
}
bool Plotter::y_is_in_plot(int y) const
{
    return y > top_margin && y < top_margin + height;
}