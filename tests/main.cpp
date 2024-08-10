#include <iostream>
#include <libplotter/plotter.hpp>

using namespace SDL2pp;
using namespace std;

int main()
{
    Plotter plotter { "Test Plot éàêë" };
    vector<Coordinate> coordinates;
    for (int i = 0; i < 100'000; i++)
    {
        float v = (float)i / 1000.;
        coordinates.push_back({ v, sin(v) });
    }
    plotter.add_collection({ coordinates, "A sinus", {}, false, true });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful curve", {}, true, true });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, -8 } }, "A beautiful curve", {}, true, true });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { -10, 8 } }, "A beautiful curve with a looooooooooooooong name", {}, true, true });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 20, 16 } }, "A beautiful curve", {}, true, true });
    return !plotter.plot();
}