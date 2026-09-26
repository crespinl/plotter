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

cmake_minimum_required(VERSION 3.28)

project(plotter VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_BUILD_TYPE "Release")

# SFML is built as a static library and linked privately into plotter, so users of plotter don't need it
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

set(LIBRARY_OUTPUT_PATH lib)

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
)

target_link_libraries(plotter PRIVATE SFML::Graphics SFML::Window)
# Don't export the symbols of the static SFML, so that it can't clash with another SFML used by the application
target_link_options(plotter PRIVATE $<$<PLATFORM_ID:Linux>:LINKER:--exclude-libs,ALL>)


include(GNUInstallDirs)

install(TARGETS plotter
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
