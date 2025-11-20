# Plotter documentation

## General statements

The first object you can manipulate is the `plotter::Plotter`. Internally, the instanciation of a `plotter::Plotter` creates a `plotter::SubPlot`, which is responsible for the title bar and the plot area, but you can add others.

After that, you can add a `plotter::Collection` or a `plotter::Function`, which respectively hold data to display point collections and functions. Both can be added to the corresponding subplot, or directly to the plotter.

If you already feel bored, please go to the **Examples** section.

## Subplot

- `SubPlot::SubPlot(string title, optional<string> x_title, optional<string> y_title)` : Creates a subplot with `title`, and `x_title` and `y_title` as titles for abscissa and ordinate.
    Note : you cannot call it directly. You have to call it via `Plotter::Plotter` or `Plotter::add_sub_plot`.
- `SubPlot::add_collection(Collection collection)` : add `collection` to the subplot.
- `SubPlot::emplace_collection(args)` : takes the arguments needed to build a `Collection`, and constructs it in place.
- `SubPlot::add_function(Function function)` : add `function` to the subplot.
- `SubPlot::emplace_collection(args)` : takes the arguments needed to build a `Function`, and constructs it in place.
- `SubPlot::set_orthonormal(Orthonormal o)` : Sets wether axis has to be orthonormal or not. (Note : axis are orthogonal anyway ;-) )
- `SubPlot::set_window(double x, double y, double w, double h, int n = 0)` : this sets the top-left point of the displayed area to (x, y).
    The subplot will then adapt $x/y$ ratio and zoom to make the displayed area represent exactly the (x, x+w, y, y-h) rectangle. If `SubPlot::set_orthonormal` was called, it is overriden.


## Plotter

- `Plotter::Plotter(args, ColorPalette p)` : constructs the plotter with a first subplot, created with `args`, with color palette `p`.
- `Plotter::plot()` : displays the current plot.
- `Plotter::save(string name)` : Saves the current plot to `name` as a png image.
- `Plotter::add_collection(Collection collection, int n)` : add `collection` to the n-th subplot.
- `Plotter::emplace_collection<int n = 0>(args)` : takes the arguments needed to build a `Collection`, and constructs it in place, in the n-th subplot.
- `Plotter::add_function(Function function, int n)` : add `function` to the n-th subplot.
- `Plotter::emplace_collection<int n = 0>(args)` : takes the arguments needed to build a `Function`, and constructs it in place, in the n-th subplot.
- `Plotter::set_window(double x, double y, double w, double h, int n = 0)` : call `Plotter::set_window` on the n-th subplot.
- `Plotter::set_stacking_direction(StackingDirection d)` : sets the stacking direction of subplots to vertical or horizontal.
- `Plotter::add_sub_plot(args)` : adds a subplot to the plotter, and returns a reference to it. Note : you can discard it, if you prefer to acces the subplot via the plotter itself.

## Collection

- `Collection::Collection(vector<Coordinate> p, string n, DisplayPoints dp, DisplayLines dl, PointType pt, LineStyle ls, Color c)` : constructs a collection of points of coordinates `p`.
    Default values are `DisplayPoints::Yes`, `DisplayLines::No`, `PointType::Square`, `LineStype::Solid`, `c = default_color`.
- `Collection::Collection(vector<double> x, vector<double> y, string n, DisplayPoints dp, DisplayLines dl, PointType pt, LineStyle ls, Color c)` : same as before, but the coordinates are in `x` for x coordinates and `y` for y coordinates.


## Functions

Function have auto-sampling : A certain number of points are used to display the function in the display range. This means that the rendering quality in not affected if you zoom in or out.

- `Function(function<double(double)> function, string name, LineStyle ls, Color c)` : constructs a `plotter::Function` which represents the mathematical function given in `function`.
    Default values are `LineStyle::Solid`, `c = default_color`.


## Color

- `Color(uint8_t r, uint8_t g, uint8_t b)` : constructs a rgb color with (r, g, b).
- `Color()` : represents an undefined color. Same as `default_color`.

## Coordinate

Struct containing four `double` fields : respectively x and y position, and x and y error, which defaults to zero. If they are set to a non-zero value, uncertainty bars are displayed.

## Enums

For clarity reasons, the library uses a lot of enum as parameters, instead of booleans or integer values.

### Yes/No enums

These enums have two values, `Yes` and `No`.

- `DisplayPoints`
- `DisplayLines`
- `Orthonormal`

### PointType

This enum has three values : `Square`, `Circle` and `Cross`.

### LineStyle

This enum has two values : `Solid` or `Dashed`.

### ColorPalette

This enum has four values : `Default`, `Pastel`, `Fire` and `Ice`. It refers to sets of colors that are used in a cyclic way for functions and collection whose color has not explicitly given.

- `Default` : 10 colors, easy to see ones.
- `Pastel` : 5 colors, pastel.
- `Fire` : 5 colors, between red and orange.
- `Ice` : 4 colors, between blue and white.

### StackingDirection

This enum has two values : `Vertical` and `Horizontal`.

## Examples

What would be a documentation without examples ?

A full and easy one :

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
    plotter.add_collection({ coordinates, "A sinus", DisplayPoints::No, DisplayLines::Yes }); // Display a function with manual (fixed) sampling.
    plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful set", DisplayPoints::Yes, DisplayLines::No });
    return !plotter.plot();
}
```

A more complex one which uses a lot of functionalities :


```cpp
Plotter plotter { "Test Plot", "x axis", "y axis", ColorPalette::Default };
plotter.set_window(-10, 10, 20, 20); // The displayed zone is the rectangle from x = -10 to x = 10, and from y = 10 to y = -10.
plotter.add_function({ [](double x) { return sin(x); }, "A sinus" , LineStyle::Dashed});
plotter.add_collection({ vector<Coordinate> { { 0, 0 }, { 10, -8 }, { -10, -8 }, { -10, 8 }, { 10, 8 }, { 0, 0 } }, "A beautiful curve", DisplayPoints::Yes, DisplayLines::Yes, PointType::Circle,  LineStyle::Solid, Color(128, 128, 128)}, 0);
plotter.emplace_collection(vector<Coordinate> { { 0, 0 }, { 20, 16 } }, "A beautiful curve", DisplayPoints::Yes, DisplayLines::Yes);

plotter.add_sub_plot("Second sub plot", "x axis", "y axis"); // Add a new subplot
vector<Coordinate> coordinates1;
vector<double> coordinates2_x;
vector<double> coordinates2_y;
vector<Coordinate> coordinates3;
size_t n = 10;
for (size_t i = 0; i < n; i++)
{
    coordinates1.emplace_back(i, i * i, 0.1 * i, 2 * i);
    coordinates2_x.push_back(n - i);
    coordinates2_y.push_back(i * i);
    coordinates3.emplace_back(i, 10 * cos(i));
}
plotter.add_collection({ coordinates1, "First sequence of points", DisplayPoints::Yes, DisplayLines::No, PointType::Square, LineStyle::Solid, default_color }, 1); // Specify the subplot as a function parameter
plotter.emplace_collection<2>(coordinates2_x, coordinates2_y, "Second sequence of points", DisplayPoints::Yes, DisplayLines::No, PointType::Circle); // Or as a template argument
plotter.emplace_collection<2>(coordinates3, "Third sequence of points", DisplayPoints::Yes, DisplayLines::No, PointType::Cross);
plotter.plot();
plotter.set_stacking_direction(StackingDirection::Vertical); // Changes the stacking direction after display, but before saving.
plotter.save("test"); // Saves as test.png

```