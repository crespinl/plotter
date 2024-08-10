cmake_minimum_required(VERSION 3.26)

project(testplotter)
find_package(plotter REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_BUILD_TYPE "Release")

set(SRCS
    main.cpp
    )

set(HEADERS
    )

add_executable(testplotter ${SRCS} ${HEADERS})
target_link_libraries(testplotter plotter::plotter)