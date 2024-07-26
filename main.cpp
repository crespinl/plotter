#include <iostream>
#include <plotter.hpp>

using namespace SDL2pp;
using namespace std;

int main(int argc, char* argv[])
{
    Plotter plotter { "Plot test" };
    vector<Coordinate> coordinates;
    for (int i = 0; i < 1000; i++)
    {
        float v = (float)i / 100.;
        coordinates.push_back({ v, sin(v) });
    }
    plotter.add_collection({ coordinates, "A sinus", 255, 0, 0, false, true });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful curve", 0, 0, 255, true, true });
    return !plotter.plot();
}
