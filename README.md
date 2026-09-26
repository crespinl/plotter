# plotter

plotter is a plotting library for C++, which aims to provide easy to use functionalities.

## Installation

### Dependencies

plotter uses [SFML](https://www.sfml-dev.org/), which is downloaded and built automatically by CMake and embedded in the library : you don't have to install it.

You only need :

- A C++20 compiler
- make
- cmake (3.28 or newer)
- ninja
- the system libraries needed to build SFML (windowing, OpenGL and fonts)

On Debian-based systems, run :

```sh
sudo apt install build-essential ninja-build cmake libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype-dev
```

### Linux

Then, run :

```sh
git clone https://github.com/crespinl/plotter.git
cd plotter
make
```

### Windows

I never tried

### MacOS

I never tried

## Run

Create a basic test program in a separate directory :
In a CMakeLists.txt file :

```cmake
cmake_minimum_required(VERSION 3.28)

project(test)
find_package(plotter REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test plotter::plotter)
```

In a main.cpp file :

```cpp
#include <cmath>
#include <vector>
#include <plotter/plotter.hpp>

using namespace std;
using namespace plotter;

int main()
{
    Plotter plotter { "Test Plot", "x axis", "y axis" };
    vector<Coordinate> coordinates;
    for (int i = 0; i < 100; i++)
    {
        float v = (float)i / 10.;
        coordinates.push_back({ v, sin(v) });
    }
    plotter.add_collection({ coordinates, "A sinus", DisplayPoints::No, DisplayLines::Yes });
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful set", DisplayPoints::Yes, DisplayLines::No });
    return !plotter.plot();
}
```

Build the CMake project and enjoy !

For further usage, please read [the documentation](./doc.md)
