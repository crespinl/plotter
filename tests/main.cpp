/*
Copyright (C) 2024-2025 Louis Crespin

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

SPDX identifier : GPL-3.0-or-later
*/
#include <iostream>
#include <plotter/plotter.hpp>

using namespace std;
using namespace plotter;

int main()
{
    Plotter plotter { "Test Plot", "x axis", "y axis" };

    plotter.set_window(-10, 10, 20, 20);
    plotter.add_function({ [](double x) { return sin(x); }, "A sinus", default_color });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes, PointType::Circle });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, -8 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, 8 } }, "A beautiful curve with a looooooooooooooong name", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    plotter.emplace_collection(vector<Coordinate> { { 0, 0 }, { 20, 16 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes);

    plotter.add_sub_plot("Test sub plot", "other x axis", "other y axis");
    /*auto weierstrass_function = [](double x) -> double {
        double y = 0;
        for (int i = 0; i < 20; i++)
        {
            constexpr double a = 0.5;
            constexpr double b = 2;
            y += pow(a, (double)i) * cos(pow(b, (double)i) * numbers::pi_v<double> * x);
        }
        return y;
    };
    plotter.emplace_function<1>(weierstrass_function, "Weierstrass function", default_color);*/
    vector<Coordinate> coordinates1;
    vector<Coordinate> coordinates2;
    vector<Coordinate> coordinates3;
    size_t n = 10;
    for (size_t i = 0; i < n; i++)
    {
        coordinates1.emplace_back(i, i * i, 0.1 * i, 2 * i);
        coordinates2.emplace_back(n - i, i * i);
        coordinates3.emplace_back(i, 10 * cos(i));
    }
    plotter.add_collection({ coordinates1, "First sequence of points", default_color }, 1);
    plotter.add_collection({ coordinates2, "Second sequence of points", default_color, DisplayPoints::Yes, DisplayLines::No, PointType::Circle }, 1);
    plotter.emplace_collection<1>(coordinates3, "Third sequence of points", default_color, DisplayPoints::Yes, DisplayLines::No, PointType::Cross);
    plotter.plot();
    plotter.set_stacking_direction(StackingDirection::Vertical);
    plotter.save("test");
    return 0;
}