cmake_minimum_required(VERSION 3.26)

project(plotter VERSION 1.0.0)

find_package(SDL2 REQUIRED)
find_package(SDL2pp REQUIRED)

set(LIBRARY_OUTPUT_PATH lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_BUILD_TYPE "Debug")

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
