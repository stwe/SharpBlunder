cmake_minimum_required(VERSION 3.17)

project(SharpBlunder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(SharpBlunder
    src/main.cpp

    src/core/Position.cpp
    src/core/Util.cpp

    src/ui/App.cpp
    src/ui/BoardView.cpp
    src/ui/PieceSet.cpp
    src/ui/Window.cpp
)

# Includes are relative to src/, e.g. #include "ui/Window.h"
target_include_directories(SharpBlunder PRIVATE src)

# The program finds its images through this absolute path (fine for development; installing needs another solution).
target_compile_definitions(SharpBlunder PRIVATE SHARPBLUNDER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_compile_options(SharpBlunder PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(SharpBlunder PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)
