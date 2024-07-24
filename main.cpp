#include <plotter.hpp>
#include <iostream>

using namespace SDL2pp;
using namespace std;

int main(int argc, char* argv[])
{
    Plotter plotter {"Plot test"};
    return !plotter.plot();
}
