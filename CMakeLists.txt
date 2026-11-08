cmake_minimum_required(VERSION 3.22)

project(SharpBlunder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# SFML 3.x
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

# Dear ImGui has no CMake support. FetchContent only downloads it, ImGui-SFML compiles it.
# Versions are pinned: ImGui-SFML 3.0 does not build with Dear ImGui 1.91.9 or newer.
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# SFML was found above. Without this, ImGui-SFML searches for it again, and for a static SFML
# when BUILD_SHARED_LIBS is not set, which fails with a system-wide (shared) SFML.
set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG v3.0
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui-sfml)

add_executable(SharpBlunder
        src/main.cpp

        src/core/GameState.cpp
        src/core/Move.cpp
        src/core/Position.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp

        src/engine/Engine.cpp
        src/engine/StockfishRules.cpp

        src/ui/App.cpp
        src/ui/BoardView.cpp
        src/ui/GuiLayer.cpp
        src/ui/MoveInput.cpp
        src/ui/NavigationBar.cpp
        src/ui/PieceSet.cpp
        src/ui/ScreenManager.cpp
        src/ui/Window.cpp

        src/ui/screens/AnalysisScreen.cpp
        src/ui/screens/PlaceholderScreen.cpp
)

# Includes are relative to src/, e.g. #include "ui/Window.h"
target_include_directories(SharpBlunder PRIVATE src)

# The program finds its images and fonts through this absolute path
# (fine for development; installing needs another solution).
target_compile_definitions(SharpBlunder PRIVATE
        SHARPBLUNDER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_compile_options(SharpBlunder PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(SharpBlunder PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        ImGui-SFML::ImGui-SFML
)
