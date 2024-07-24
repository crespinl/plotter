#include <iostream>
#include <plotter.hpp>

using namespace SDL2pp;
using namespace std;

int main(int argc, char* argv[])
{
    Plotter plotter { "Plot test" };
    vector<Coordinate> coordinates { { 0., 0. }, { 1., 1. }, { 2., 4. }, { 3., 9. }, { 4., 16. } };
    plotter.add_collection({ coordinates, "f(x) = x**2", 255, 0, 0 });
    return !plotter.plot();
}
