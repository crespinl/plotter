cmake_minimum_required(VERSION 3.10)

project(plotter)
find_package(SDL2pp REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_BUILD_TYPE "Debug")

set(SRCS
    main.cpp
    plotter.cpp
    fonts/firacode.cpp
    fonts/notosans.cpp
    )

set(HEADERS
    plotter.hpp
    fonts/firacode.hpp
    fonts/notosans.hpp
    )

add_executable(plotter ${SRCS} ${HEADERS})
target_include_directories(plotter PRIVATE "./")
target_include_directories(plotter PRIVATE "./fonts/")
target_link_libraries(plotter SDL2pp::SDL2pp)
