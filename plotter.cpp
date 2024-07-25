#include <chrono>
#include <cmath>
#include <iomanip>
#include <plotter.hpp>
#include <sstream>
#include <thread>

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
            auto start = chrono::steady_clock::now();
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

            Texture title_sprite { renderer, m_big_font.RenderText_Blended(m_title, SDL_Color(0, 0, 0, 255)) };
            center_sprite(renderer, title_sprite, (width + 2 * hmargin) / 2, top_margin / 2);

            draw_axis(renderer);

            Texture sprite { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, hmargin + width, top_margin + height };
            sprite.SetBlendMode(SDL_BLENDMODE_BLEND);
            renderer.SetTarget(sprite);
            renderer.SetDrawColor(0, 0, 0, 0);
            renderer.Clear();
            renderer.SetTarget();
            for (auto const& e : m_collections)
            {
                plot_collection(e, renderer, sprite);
            }
            renderer.Copy(sprite, Rect { hmargin, top_margin, width, height }, { hmargin, top_margin });

            renderer.Present();

            // Frame limiter
            auto end = chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            int duration_ms = round(diff.count() * 1000);
            int to_wait = max(40 - duration_ms, 0); // Make the whole iteration take at least 1/25 s
            this_thread::sleep_for(chrono::milliseconds(to_wait));
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
        {
            draw_horizontal_line_number(rounded_y_min + i * y_step, ordinate, renderer);
            renderer.FillRect(Rect::FromCorners(hmargin, ordinate - line_width_half, hmargin + width, ordinate + line_width_half));
        }
    }
    for (int i = 0; i < nb_vertical_axis; i++)
    {
        int abscissa = to_plot_x(rounded_x_min + i * x_step);
        if (x_is_in_plot(abscissa))
        {
            draw_vertical_line_number(rounded_x_min + i * x_step, abscissa, renderer);
            renderer.FillRect(Rect::FromCorners(abscissa - line_width_half, top_margin, abscissa + line_width_half, top_margin + height));
        }
    }

    // Draw main axis :
    Texture zero_sprite { renderer, m_small_font.RenderText_Blended("0", SDL_Color(0, 0, 0, 255)) };
    renderer.SetDrawColor(120, 120, 120, 255);
    if (y_is_in_plot(to_plot_y(0))) // abscissa
    {
        draw_horizontal_line_number(0., to_plot_y(0.), renderer);
        renderer.FillRect(Rect::FromCorners(hmargin, to_plot_y(0) - line_width_half, hmargin + width, to_plot_y(0) + line_width_half));
    }
    if (x_is_in_plot(to_plot_x(0))) // ordinate
    {
        draw_vertical_line_number(0., to_plot_x(0.), renderer);
        renderer.FillRect(Rect::FromCorners(to_plot_x(0) - line_width_half, top_margin, to_plot_x(0) + line_width_half, top_margin + height));
    }
}

void Plotter::draw_point(int x, int y, Renderer& renderer)
{
    int abscissa = to_plot_x(x);
    int ordinate = to_plot_y(y);
    if (x_is_in_plot(abscissa) && y_is_in_plot(ordinate))
        renderer.FillRect(Rect::FromCorners(abscissa - half_point_size, ordinate - half_point_size, abscissa + half_point_size, ordinate + half_point_size));
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

void Plotter::draw_vertical_line_number(float nb, int x, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_small_font.RenderText_Blended(to_str(nb), SDL_Color(0, 0, 0, 255)) };
    center_sprite(renderer, sprite, x, top_margin + height + text_margin + sprite.GetHeight() / 2);
}

void Plotter::draw_horizontal_line_number(float nb, int y, SDL2pp::Renderer& renderer)
{
    Texture sprite { renderer, m_small_font.RenderText_Blended(to_str(nb), SDL_Color(0, 0, 0, 255)) };
    center_sprite(renderer, sprite, hmargin - text_margin - sprite.GetWidth() / 2, y);
}

void Plotter::center_sprite(Renderer& renderer, Texture& texture, int x, int y)
{
    renderer.Copy(texture, NullOpt, { x - texture.GetWidth() / 2, y - texture.GetHeight() / 2 });
}

std::string Plotter::to_str(float nb)
{
    ostringstream out;
    out << setprecision(4);
    out << nb;
    return out.str();
}

void Plotter::plot_collection(Collection const& c, SDL2pp::Renderer& renderer, Texture& into)
{
    if (c.points.size() == 0)
        return;
    renderer.SetDrawColor(c.color());
    for (size_t i = 0; i < c.points.size() - 1; i++)
    {
        if ((to_plot_x(c.points[i].x) < hmargin && to_plot_x(c.points[i + 1].x) < hmargin)
            || (to_plot_x(c.points[i].x) > hmargin + width && to_plot_x(c.points[i + 1].x) > hmargin + width)
            || (to_plot_y(c.points[i].y) < top_margin && to_plot_y(c.points[i + 1].y) < top_margin)
            || (to_plot_y(c.points[i].y) > top_margin + height && to_plot_y(c.points[i + 1].y) > top_margin + height))
            continue; // Both points are outside of the screen, and on the same side : there is nothing to draw
        if (c.draw_points)
            draw_point(c.points[i].x, c.points[i].y, renderer);
        if (c.draw_lines)
            draw_line(to_point(c.points[i]), to_point(c.points[i + 1]), renderer, into);
    }
    if (c.draw_points)
        draw_point(c.points.back().x, c.points.back().y, renderer);
}

SDL2pp::Point Plotter::to_point(Coordinate const& c) const
{
    return { to_plot_x(c.x), to_plot_y(c.y) };
}

void Plotter::draw_line(Point const& p1, Point const& p2, Renderer& renderer, Texture& into)
{
    int x1 = p1.GetX();
    int x2 = p2.GetX();
    int y1 = p1.GetY();
    int y2 = p2.GetY();
    Rect { hmargin, top_margin, width, height }.IntersectLine(x1, y1, x2, y2); // clips only the needed part of the line
    int const max_w = sqrt(width * width + height + height);
    float w_candidate = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)) + 1;
    int w;
    if (w_candidate > max_w)
    {
        w = max_w;
    }
    else
    {
        w = static_cast<int>(w_candidate);
    }
    float angle = atan2(y2 - y1, x2 - x1);
    Texture sprite { renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, 4 * line_width_half };
    renderer.SetTarget(sprite);
    renderer.Clear();
    renderer.SetTarget(into);
    Point dst_point { x1 + static_cast<int>(2. * line_width_half * sin(angle)), y1 + static_cast<int>(-2. * line_width_half * cos(angle)) }; // offset due to rotation
    angle *= 360 / (2 * numbers::pi_v<float>);                                                                                               // to degree
    renderer.Copy(sprite, NullOpt, dst_point, angle, Point { 0, 0 });
    renderer.SetTarget();
}
