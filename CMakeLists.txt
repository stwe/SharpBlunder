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

# JSON for the settings file (MIT). Its own tests and installation are not needed.
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(json)

# libcurl for HTTPS (SFML has no HTTPS). It has a license of its own, similar to MIT, and is linked dynamically.
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

add_executable(SharpBlunder
        src/main.cpp

        src/core/ChessComParser.cpp
        src/core/DrawRules.cpp
        src/core/GameList.cpp
        src/core/GameState.cpp
        src/core/GameSummary.cpp
        src/core/I18n.cpp
        src/core/Material.cpp
        src/core/Move.cpp
        src/core/MoveQuality.cpp
        src/core/OpeningBook.cpp
        src/core/Pgn.cpp
        src/core/PgnTags.cpp
        src/core/Position.cpp
        src/core/PositionCheck.cpp
        src/core/San.cpp
        src/core/Score.cpp
        src/core/Settings.cpp
        src/core/Square.cpp
        src/core/TimeControl.cpp
        src/core/Util.cpp

        src/engine/Analyzer.cpp
        src/engine/Engine.cpp
        src/engine/StockfishRules.cpp
        src/engine/UciInfo.cpp

        src/net/ChessComClient.cpp
        src/net/CurlHttpClient.cpp
        src/net/GamesSyncer.cpp

        src/ui/App.cpp
        src/ui/BoardView.cpp
        src/ui/EnginePanel.cpp
        src/ui/EvalBar.cpp
        src/ui/GameDetails.cpp
        src/ui/GuiLayer.cpp
        src/ui/Icons.cpp
        src/ui/MoveInput.cpp
        src/ui/NotationPanel.cpp
        src/ui/PgnMenu.cpp
        src/ui/PlayerStrip.cpp
        src/ui/NavigationBar.cpp
        src/ui/PieceSet.cpp
        src/ui/ScreenManager.cpp
        src/ui/Window.cpp

        src/ui/screens/AnalysisScreen.cpp
        src/ui/screens/MyGamesScreen.cpp
        src/ui/screens/PlaceholderScreen.cpp
        src/ui/screens/SettingsScreen.cpp
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
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

# Tests: they need neither a window nor Stockfish. Run them with "ctest" in the build directory.
enable_testing()

add_executable(SharpBlunderTests
        tests/UciInfoTests.cpp

        src/core/I18n.cpp
        src/core/Move.cpp
        src/core/MoveQuality.cpp
        src/core/Score.cpp
        src/core/Square.cpp
        src/core/Util.cpp

        src/engine/UciInfo.cpp
)

target_include_directories(SharpBlunderTests PRIVATE src tests)
target_compile_options(SharpBlunderTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME UciInfoTests COMMAND SharpBlunderTests)

add_executable(OpeningBookTests
        tests/OpeningBookTests.cpp

        src/core/Move.cpp
        src/core/OpeningBook.cpp
        src/core/Position.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(OpeningBookTests PRIVATE src tests)
target_compile_definitions(OpeningBookTests PRIVATE SHARPBLUNDER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_compile_options(OpeningBookTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME OpeningBookTests COMMAND OpeningBookTests)

add_executable(GameStateTests
        tests/GameStateTests.cpp

        src/core/GameState.cpp
        src/core/Move.cpp
        src/core/Position.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(GameStateTests PRIVATE src tests)
target_compile_options(GameStateTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME GameStateTests COMMAND GameStateTests)

add_executable(SanTests
        tests/SanTests.cpp

        src/core/Move.cpp
        src/core/Position.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(SanTests PRIVATE src tests)
target_compile_options(SanTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME SanTests COMMAND SanTests)

add_executable(PgnTests
        tests/PgnTests.cpp

        src/core/GameState.cpp
        src/core/I18n.cpp
        src/core/Move.cpp
        src/core/Pgn.cpp
        src/core/Position.cpp
        src/core/PositionCheck.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(PgnTests PRIVATE src tests)
target_compile_options(PgnTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME PgnTests COMMAND PgnTests)

add_executable(PositionCheckTests
        tests/PositionCheckTests.cpp

        src/core/Move.cpp
        src/core/Position.cpp
        src/core/PositionCheck.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(PositionCheckTests PRIVATE src tests)
target_compile_options(PositionCheckTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME PositionCheckTests COMMAND PositionCheckTests)

add_executable(MaterialTests
        tests/MaterialTests.cpp

        src/core/GameState.cpp
        src/core/Material.cpp
        src/core/Move.cpp
        src/core/Position.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(MaterialTests PRIVATE src tests)
target_compile_options(MaterialTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME MaterialTests COMMAND MaterialTests)

add_executable(TimeControlTests
        tests/TimeControlTests.cpp

        src/core/TimeControl.cpp
)

target_include_directories(TimeControlTests PRIVATE src tests)
target_compile_options(TimeControlTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME TimeControlTests COMMAND TimeControlTests)

add_executable(PgnTagsTests
        tests/PgnTagsTests.cpp

        src/core/GameState.cpp
        src/core/I18n.cpp
        src/core/Move.cpp
        src/core/OpeningBook.cpp
        src/core/Pgn.cpp
        src/core/PgnTags.cpp
        src/core/Position.cpp
        src/core/PositionCheck.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(PgnTagsTests PRIVATE src tests)
target_compile_options(PgnTagsTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME PgnTagsTests COMMAND PgnTagsTests)

add_executable(DrawRulesTests
        tests/DrawRulesTests.cpp

        src/core/DrawRules.cpp
        src/core/GameState.cpp
        src/core/I18n.cpp
        src/core/Move.cpp
        src/core/Position.cpp
        src/core/San.cpp
        src/core/Square.cpp
        src/core/Util.cpp
)

target_include_directories(DrawRulesTests PRIVATE src tests)
target_compile_options(DrawRulesTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME DrawRulesTests COMMAND DrawRulesTests)

add_executable(DataDirTests
        tests/DataDirTests.cpp

        src/core/Util.cpp
)

target_include_directories(DataDirTests PRIVATE src tests)
target_compile_options(DataDirTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME DataDirTests COMMAND DataDirTests)

add_executable(SettingsTests
        tests/SettingsTests.cpp

        src/core/Settings.cpp
        src/core/Util.cpp
)

target_include_directories(SettingsTests PRIVATE src tests)
target_compile_options(SettingsTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(SettingsTests PRIVATE nlohmann_json::nlohmann_json)

add_test(NAME SettingsTests COMMAND SettingsTests)

add_executable(I18nTests
        tests/I18nTests.cpp

        src/core/I18n.cpp
)

target_include_directories(I18nTests PRIVATE src tests)
target_compile_options(I18nTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME I18nTests COMMAND I18nTests)

add_executable(ChessComParserTests
        tests/ChessComParserTests.cpp

        src/core/ChessComParser.cpp
        src/core/GameSummary.cpp
        src/core/I18n.cpp
        src/core/TimeControl.cpp
)

target_include_directories(ChessComParserTests PRIVATE src tests)
target_compile_definitions(ChessComParserTests PRIVATE
        SHARPBLUNDER_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)
target_compile_options(ChessComParserTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ChessComParserTests PRIVATE nlohmann_json::nlohmann_json)

add_test(NAME ChessComParserTests COMMAND ChessComParserTests)

add_executable(GameListTests
        tests/GameListTests.cpp

        src/core/GameList.cpp
        src/core/GameSummary.cpp
        src/core/I18n.cpp
        src/core/TimeControl.cpp
)

target_include_directories(GameListTests PRIVATE src tests)
target_compile_options(GameListTests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME GameListTests COMMAND GameListTests)

add_executable(ChessComClientTests
        tests/ChessComClientTests.cpp

        src/core/ChessComParser.cpp
        src/core/GameList.cpp
        src/core/GameSummary.cpp
        src/core/I18n.cpp
        src/core/TimeControl.cpp

        src/net/ChessComClient.cpp
)

target_include_directories(ChessComClientTests PRIVATE src tests)
target_compile_definitions(ChessComClientTests PRIVATE
        SHARPBLUNDER_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)
target_compile_options(ChessComClientTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ChessComClientTests PRIVATE nlohmann_json::nlohmann_json)

add_test(NAME ChessComClientTests COMMAND ChessComClientTests)

add_executable(CurlHttpClientTests
        tests/CurlHttpClientTests.cpp

        src/net/CurlHttpClient.cpp
)

target_include_directories(CurlHttpClientTests PRIVATE src tests)
target_compile_options(CurlHttpClientTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(CurlHttpClientTests PRIVATE CURL::libcurl)

add_test(NAME CurlHttpClientTests COMMAND CurlHttpClientTests)

add_executable(GamesSyncerTests
        tests/GamesSyncerTests.cpp

        src/core/ChessComParser.cpp
        src/core/GameList.cpp
        src/core/GameSummary.cpp
        src/core/I18n.cpp
        src/core/TimeControl.cpp

        src/net/ChessComClient.cpp
        src/net/GamesSyncer.cpp
)

target_include_directories(GamesSyncerTests PRIVATE src tests)
target_compile_options(GamesSyncerTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(GamesSyncerTests PRIVATE nlohmann_json::nlohmann_json Threads::Threads)

add_test(NAME GamesSyncerTests COMMAND GamesSyncerTests)
