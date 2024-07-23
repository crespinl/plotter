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

    float x_max = ((float)width - (float)width / 2.) / m_zoom - m_x_offset;
    float x_min = (0. - (float)width / 2.) / m_zoom - m_x_offset;
    float y_max = (0. + (float)height / 2.) / m_zoom - m_y_offset;
    float y_min = (-(float)height + (float)height / 2.) / m_zoom - m_y_offset;

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
        renderer.FillRect(Rect::FromCorners(0, height / 2 - (rounded_y_min + i * y_step) * m_zoom - line_width_half - y_offset, width, height / 2 - (rounded_y_min + i * y_step) * m_zoom + line_width_half - y_offset));
    }
    for (int i = 0; i < nb_vertical_axis; i++)
    {
        renderer.FillRect(Rect::FromCorners(width / 2 + (rounded_x_min + i * x_step) * m_zoom - line_width_half + x_offset, 0, width / 2 + (rounded_x_min + i * x_step) * m_zoom + line_width_half + x_offset, height));
    }

    // Draw main axis :
    renderer.SetDrawColor(120, 120, 120, 255);
    renderer.FillRect(Rect::FromCorners(0, height / 2 - line_width_half - y_offset, width, height / 2 + line_width_half - y_offset));
    renderer.FillRect(Rect::FromCorners(width / 2 - line_width_half + x_offset, 0, width / 2 + line_width_half + x_offset, height));
}

void Plotter::draw_point(int x, int y, Renderer& renderer)
{
    x *= m_zoom;
    y *= m_zoom;
    int x_offset = compute_x_offset();
    int y_offset = compute_y_offset();
    renderer.FillRect(Rect::FromCorners(width / 2 + x - 5 + x_offset, height / 2 - y - 5 - y_offset, width / 2 + x + 5 + x_offset, height / 2 - y + 5 - y_offset));
}