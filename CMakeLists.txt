cmake_minimum_required(VERSION 3.10)

project(plotter)
find_package(SDL2pp REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_BUILD_TYPE "Debug")

set(SRCS
    main.cpp
    plotter.cpp
    )

set(HEADERS
    plotter.hpp
    )

add_executable(plotter ${SRCS} ${HEADERS})
target_include_directories(plotter PRIVATE "./")
target_link_libraries(plotter SDL2pp::SDL2pp)