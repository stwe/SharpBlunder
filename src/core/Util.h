#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Constants
    //-------------------------------------------------

    /**
     * @brief The standard starting position in FEN (Forsyth-Edwards Notation) format.
     */
    inline constexpr auto* START_FEN{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };

    /**
     * @brief Separator between moves in a serialized move sequence (e.g., "e2e4 e7e5 g1f3").
     *
     * Used by JoinMoves() and for parsing with SplitStr(). The same format is used in the repertoire files,
     * the evaluation cache, the spaced-repetition keys and the UCI "position ... moves" command.
     */
    inline constexpr char MOVE_SEPARATOR{ ' ' };

    //-------------------------------------------------
    // Utils
    //-------------------------------------------------

    /**
     * @brief Resolves the path to the Stockfish executable.
     *
     * Checks the provided command line argument first, then the system environment
     * variable 'STOCKFISH', and falls back to the default "stockfish" binary name.
     *
     * @param t_arg Optional command-line argument specifying the path.
     *
     * @return std::string The resolved executable path.
     */
    std::string stockfish_path(const char* t_arg = nullptr);

    /**
     * @brief Retrieves and ensures the existence of the application's data directory.
     *
     * Creates a directory named '.chess' in the user's home folder (or working directory if HOME is unset).
     *
     * @return std::filesystem::path Path to the application data directory.
     */
    std::filesystem::path data_dir();

    /**
     * @brief Splits a string view by a given delimiter.
     *
     * Empty tokens between and after delimiters are preserved ("a\t\t" yields three tokens).
     * An empty input yields no tokens.
     *
     * @param t_s The target string view to split.
     * @param t_d The character delimiter.
     *
     * @return std::vector<std::string> A vector of tokens extracted from the string.
     */
    std::vector<std::string> split_str(std::string_view t_s, char t_d);

    /**
     * @brief Joins a sequence of moves into a single string separated by MOVE_SEPARATOR.
     *
     * The result identifies a position by the moves leading to it, e.g. {"e2e4", "e7e5"} yields "e2e4 e7e5".
     * An empty sequence (the starting position) yields an empty string.
     * To parse the result again, use SplitStr(result, MOVE_SEPARATOR).
     *
     * @param t_moves The moves in order (UCI notation).
     *
     * @return std::string The serialized move sequence.
     */
    std::string join_moves(const std::vector<std::string>& t_moves);
}
