# Copyright (C) 2024 Louis Crespin
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 
# SPDX itentifier : GPL-3.0-or-later

cmake_minimum_required(VERSION 3.26)

project(plotter VERSION 1.0.0)

find_package(SDL2 REQUIRED)

set(SDL2PP_WITH_IMAGE ON)
set(SDL2PP_WITH_TTF ON)
add_subdirectory(libSDL2pp)
#find_package(SDL2pp REQUIRED)

set(LIBRARY_OUTPUT_PATH lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_BUILD_TYPE "Release")

set(SRCS
    src/plotter.cpp
    fonts/firacode.cpp
    fonts/notosans.cpp
    )

set(HEADERS
    include/plotter/plotter.hpp
    include/plotter/firacode.hpp
    include/plotter/notosans.hpp
    )

add_library(plotter SHARED ${SRCS} ${HEADERS})

target_include_directories(plotter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/plotter>
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(plotter PUBLIC SDL2::SDL2 SDL2pp::SDL2pp)


include(GNUInstallDirs)

install(TARGETS SDL2pp plotter
    EXPORT plotterTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/plotter/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/plotter
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT plotterTargets
    FILE plotterTargets.cmake
    NAMESPACE plotter::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plotter
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/plotterConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/plotterConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plotterConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plotter
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/plotterConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/plotterConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plotter
)
