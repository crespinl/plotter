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
                        default : break;
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
    // TODO : consider m_zoom when drawing other axis
    renderer.SetDrawColor(150, 150, 150, 255);
    int x_offset = compute_x_offset();
    int y_offset = compute_y_offset();
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