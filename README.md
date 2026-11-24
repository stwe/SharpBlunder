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
- Variations like on lichess: take a move back and play another one, and the game keeps both. The main line is a
  table, variations are written below the move they belong to (nested ones in parentheses). A right click on a move
  makes its line the main line or deletes it
- Comments, as in PGN: on single moves (a speech bubble next to the move, shown when the mouse rests on it or after a
  click), on a variation (in its header) and on the whole game. A check box shows all of them at once
- Games are read and written as PGN, with variations, comments, the lichess clock and evaluation commands
  (`[%clk ...]`, `[%eval ...]` are kept as they are) and the glyphs (`$1`, `!`, `?`). The PGN button copies the game to
  the clipboard, pastes a game from it, and saves or opens a file. Files go to `~/.chess/games` by default
- FEN: copy the FEN of the position on the board, and start a game from a FEN. A FEN that cannot be played (no king,
  a pawn on the last rank, the side that is not to move in check) is refused with the reason
- Details of the game as in chess.com (a pencil above the move list): names and ratings of White and Black, result,
  event, place, round, date, time control and how the game ended. They are PGN tags and are saved with the game.
  The date and the time are typed the way people write them and are saved the way the
  [PGN standard](https://en.wikipedia.org/wiki/Portable_Game_Notation) wants them: `24.11.2026` becomes `2026.11.24`,
  and a time control like `15+10` (15 minutes, 10 seconds per move) becomes `900+10` (seconds). Without a unit the
  first number is minutes and the second is seconds, `90 s` and `1h 30min` work too, and a list offers the usual
  controls (Bullet, Blitz, Rapid, Classical)
- The players are shown above and below the board with a picture, name, rating and the pieces they have captured
- Step through the game, flip the board (the labels a-h and 1-8 outside the board turn with it), start a new game
- Live analysis with a second Stockfish process: evaluation bar next to the board and an arrow for the best move.
  The three best moves are listed as rows: a colored dot (how much weaker than the best move), the score and the
  move with the icon of the piece. A click on a row plays the move, a magnifying glass opens the whole line. One line
  says in words who is better ("Equal", "White is slightly better", ...). A check box switches the analysis off
- Name and ECO code of the opening (for example `B90 Sicilian Defense: Najdorf Variation`), found by position, so
  transpositions work too. After the game leaves the book, the last known name stays. The opening is also written
  to the PGN tags `ECO`, `Opening`, `Variation` and `SubVariation`
- Sidebar navigation between the pages

Planned, not implemented yet:

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
assets/        piece images (pieces/cburnett), GUI font (fonts), opening names (openings), each with its license
src/core/      chess model without any graphics: Position, Move, Piece, SAN notation, GameState, RulesProvider,
               Score, OpeningBook
src/engine/    Stockfish as a child process (Engine), the rules from Stockfish (StockfishRules), the live
               analysis (Analyzer) and the parser for its UCI lines
src/ui/        window, Dear ImGui layer, navigation, board view, evaluation bar, mouse input, pages (screens/).
               The side panel of the analysis page has three zones: the game (name, status, Flip, New),
               the engine (EnginePanel) and the moves with the navigation (NotationPanel)
tests/         test programs, run them with ctest in the build directory
tools/         render_pieces.sh turns the SVG pieces into the PNG files the program loads,
               update_openings.sh builds assets/openings/openings.tsv again from the Lichess data
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
| [Opening names](https://github.com/lichess-org/chess-openings) by Lichess | CC0        | `assets/openings`                   |
| [SFML](https://www.sfml-dev.org/)                          | zlib/libpng        | linked, installed on your system    |
| [Dear ImGui](https://github.com/ocornut/imgui)             | MIT                | downloaded by CMake                 |
| [ImGui-SFML](https://github.com/SFML/imgui-sfml)           | MIT                | downloaded by CMake                 |
| [Stockfish](https://stockfishchess.org/)                   | GPL 3              | separate program, not included      |
