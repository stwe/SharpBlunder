# SharpBlunder

A desktop chess opening trainer and analysis board for Linux, written in C++23 with
[SFML 3](https://www.sfml-dev.org/) and [Dear ImGui](https://github.com/ocornut/imgui).
The rules of chess come from [Stockfish](https://stockfishchess.org/), which runs as a child process.

> **Status: work in progress.** The analysis board works. The other pages are placeholders.

## Features

Working today:

- Analysis board with the Lichess `cburnett` pieces
- Move pieces by drag and drop or by clicking the piece and then the target square
- Legal moves, check, checkmate and stalemate come from Stockfish
- Highlights for the last move, the king in check and the target squares of the selected piece
- Pawn promotion with a choice of the new piece
- Move list in Standard Algebraic Notation (`Nbd7`, `exd6`, `O-O-O`, `Rd8#`), click a move to jump to it
- Step through the game, flip the board, start a new game
- Sidebar navigation between the pages

Planned, not implemented yet:

- Live analysis with Stockfish (evaluation bar, best moves)
- Import and analysis of your games from lichess.org and chess.com
- Training of your own opening repertoire with spaced repetition
- Settings
- Synchronization of the learning data with a server, and an Android app that uses it

## Requirements

- Linux (the engine is started with `fork` and `pipe2`; Windows and macOS are not supported)
- A C++23 compiler with `std::ranges::to` and `std::views::join_with`: GCC 14 or newer
- CMake 3.22 or newer
- SFML **3.x** (Graphics, Window, System)
- OpenGL
- Stockfish
- An internet connection for the first CMake run, which downloads Dear ImGui 1.91.8 and ImGui-SFML 3.0

## Building

### Arch Linux / CachyOS

```bash
sudo pacman -S --needed base-devel cmake git sfml stockfish
```

The `sfml` package must be version 3.x. Check it with `pacman -Qi sfml`.

### Debian / Ubuntu

The SFML packages of Debian and Ubuntu are version 2.x (Ubuntu 24.04 has 2.6.1), but SharpBlunder needs
SFML 3. Build SFML 3 from source:

```bash
sudo apt install build-essential cmake git g++-14 stockfish \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype-dev

git clone --branch 3.0.0 --depth 1 https://github.com/SFML/SFML.git
cmake -S SFML -B SFML/build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
    -DSFML_BUILD_AUDIO=OFF -DSFML_BUILD_NETWORK=OFF -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_DOC=OFF
cmake --build SFML/build -j
sudo cmake --install SFML/build
sudo ldconfig
```

### SharpBlunder

```bash
git clone https://github.com/stwe/SharpBlunder.git
cd SharpBlunder
cmake -B build -DCMAKE_BUILD_TYPE=Release          # on Ubuntu add: -DCMAKE_CXX_COMPILER=g++-14
cmake --build build -j
```

## Running

```bash
./build/SharpBlunder [path/to/stockfish]
```

The Stockfish executable is taken from the first argument. Without an argument, the environment variable
`STOCKFISH` is used, and without that the name `stockfish` is looked up in the `PATH`. If that fails, the program
also tries `/usr/games/stockfish`, where the Debian/Ubuntu package installs it.

The program finds its images and fonts in the `assets` directory of the source tree. CMake stores the absolute path
of that directory in the executable, so run CMake again if you move the source directory.

### Controls

| Input                                | Action                                         |
|--------------------------------------|------------------------------------------------|
| Drag a piece, or click piece, target | Move                                           |
| Right click                          | Cancel the selection                           |
| `Left` / `Right`                     | One move back / forward                        |
| `Home` / `End`                       | Go to the start / end of the game              |
| `F`                                  | Flip the board                                 |
| `Esc`                                | Quit                                           |
| Click on a move in the list          | Jump to that position                          |

## Project layout

```
assets/        piece images (pieces/cburnett), GUI font (fonts), each with its license
src/core/      chess model without any graphics: Position, Move, Piece, SAN notation, GameState, RulesProvider
src/engine/    Stockfish as a child process (Engine) and the rules from Stockfish (StockfishRules)
src/ui/        window, Dear ImGui layer, navigation, board view, mouse input, pages (screens/)
tools/         render_pieces.sh turns the SVG pieces into the PNG files the program loads
```

`core` does not depend on anything else. The rest of the program only knows the `RulesProvider` interface,
so the source of the rules (today Stockfish) can be replaced.

## Code style

The formatting is defined in `.clang-format`, the naming rules are in `.clang-tidy`. In short:

- Types and methods in `CamelCase`, global functions in `lower_case`, functions in anonymous namespaces in `CamelCase`
- Enum constants and constants in `UPPER_CASE`, namespaces in `lower_case`
- Parameters start with `t_`, members with `m_`
- Every class, method and function has a Doxygen comment

## License

SharpBlunder is free software under the **GNU General Public License, version 3 or (at your option) any later
version**. See [LICENSE.md](LICENSE.md).

It uses these third-party works, which keep their own licenses:

| Work                                                       | License            | Where                               |
|------------------------------------------------------------|--------------------|-------------------------------------|
| Piece set `cburnett` by Colin M. L. Burnett (via Lichess)  | GPL 2 or later     | `assets/pieces/cburnett`            |
| Font Roboto Medium by Christian Robertson                  | Apache License 2.0 | `assets/fonts`                      |
| [SFML](https://www.sfml-dev.org/)                          | zlib/libpng        | linked, installed on your system    |
| [Dear ImGui](https://github.com/ocornut/imgui)             | MIT                | downloaded by CMake                 |
| [ImGui-SFML](https://github.com/SFML/imgui-sfml)           | MIT                | downloaded by CMake                 |
| [Stockfish](https://stockfishchess.org/)                   | GPL 3              | separate program, not included      |
