# plotter

plotter is a plotting library for C++, which aims to provide easy to use functionalities.

## Installation

### Install SDL2pp

First install basic dependencies :

- A compiler
- make
- cmake
- ninja

On Debian-based systems, run :

```sh
sudo apt install build-essentials ninja cmake
```

Follow instruction at : <https://github.com/libSDL2pp/libSDL2pp?tab=readme-ov-file#building>, install the library system-wide.

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
cmake_minimum_required(VERSION 3.26)

project(test)
find_package(plotter REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test plotter::plotter)
```

In a main.cpp file :

```cpp
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
