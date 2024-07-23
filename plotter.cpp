#include <plotter.hpp>

using namespace std;
using namespace SDL2pp;

bool Plotter::plot()
{
    try
    {
        SDL sdl(SDL_INIT_VIDEO);
        Window window("Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
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

            renderer.SetDrawColor(255, 255, 255, 255);
            renderer.Clear();

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

    float x_max = from_screen_x(width);
    float x_min = from_screen_x(0);
    float y_max = from_screen_y(0);
    float y_min = from_screen_y(height);

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
        renderer.FillRect(Rect::FromCorners(0, to_screen_y(rounded_y_min + i * y_step) - line_width_half, width, to_screen_y(rounded_y_min + i * y_step) + line_width_half));
    }
    for (int i = 0; i < nb_vertical_axis; i++)
    {
        renderer.FillRect(Rect::FromCorners(to_screen_x(rounded_x_min + i * x_step) - line_width_half, 0, to_screen_x(rounded_x_min + i * x_step) + line_width_half, height));
    }

    // Draw main axis :
    renderer.SetDrawColor(120, 120, 120, 255);
    renderer.FillRect(Rect::FromCorners(0, to_screen_y(0) - line_width_half, width, to_screen_y(0) + line_width_half));
    renderer.FillRect(Rect::FromCorners(to_screen_x(0) - line_width_half, 0, to_screen_x(0) + line_width_half, height));
}

void Plotter::draw_point(int x, int y, Renderer& renderer)
{
    renderer.FillRect(Rect::FromCorners(to_screen_x(x) - 5, to_screen_y(y) - 5, to_screen_x(x) + 5, to_screen_y(y) + 5));
}

int Plotter::to_screen_x(float x) const
{
    return width / 2 + x * m_zoom + compute_x_offset();
}
int Plotter::to_screen_y(float y) const
{
    return height / 2 - y * m_zoom - compute_y_offset();
}
float Plotter::from_screen_x(int x) const
{
    return ((float)x - (float)width / 2.) / m_zoom - m_x_offset;
}
float Plotter::from_screen_y(int x) const
{
    return (-x + (float)height / 2.) / m_zoom - m_y_offset;
}