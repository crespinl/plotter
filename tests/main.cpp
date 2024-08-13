/*
Copyright (C) 2024 Louis Crespin

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

SPDX itentifier : GPL-3.0-or-later
*/
#include <iostream>
#include <plotter/plotter.hpp>

using namespace std;
using namespace plotter;

int main()
{
    Plotter plotter { "Test Plot", "x axis", "y axis" };
    vector<Coordinate> coordinates;
    for (int i = 0; i < 10'000; i++)
    {
        float v = (float)i / 1000.;
        coordinates.push_back({ v, sin(v) });
    }
    plotter.add_collection({ coordinates, "A sinus", default_color, DisplayPoints::No, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, -8 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, 8 } }, "A beautiful curve with a looooooooooooooong name", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 20, 16 } }, "A beautiful curve", default_color, DisplayPoints::Yes, DisplayLines::Yes });
    return !plotter.plot();
}