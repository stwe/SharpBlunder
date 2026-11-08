#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Constants
    //-------------------------------------------------

    inline constexpr int BOARD_SIZE{ 8 };                          ///< Number of files and ranks
    inline constexpr int SQUARE_COUNT{ BOARD_SIZE * BOARD_SIZE };  ///< Number of squares on the board

    //-------------------------------------------------
    // Squares
    //-------------------------------------------------

    // A square is an int from 0 to 63: 0 = a1, 7 = h1, 8 = a2, ..., 63 = h8.
    // File and rank are counted from zero: file 0 is the a-file, rank 0 is the first rank.

    /**
     * @brief Converts file and rank to a square index.
     *
     * @param t_file The file, 0 to 7.
     * @param t_rank The rank, 0 to 7.
     *
     * @return The square index (0 = a1, 63 = h8).
     */
    [[nodiscard]] constexpr int square_index(const int t_file, const int t_rank) noexcept
    {
        return t_rank * BOARD_SIZE + t_file;
    }

    /**
     * @brief Returns the file of a square.
     *
     * @param t_square The square index.
     *
     * @return The file, 0 to 7.
     */
    [[nodiscard]] constexpr int file_of(const int t_square) noexcept
    {
        return t_square % BOARD_SIZE;
    }

    /**
     * @brief Returns the rank of a square.
     *
     * @param t_square The square index.
     *
     * @return The rank, 0 to 7.
     */
    [[nodiscard]] constexpr int rank_of(const int t_square) noexcept
    {
        return t_square / BOARD_SIZE;
    }

    /**
     * @brief Returns the algebraic name of a square.
     *
     * @param t_square The square index, 0 to 63.
     *
     * @return The name, e.g. "e4".
     */
    [[nodiscard]] std::string square_name(int t_square);

    /**
     * @brief Parses the algebraic name of a square.
     *
     * @param t_name A name such as "e4".
     *
     * @return The square index, or std::nullopt if the text is not a square.
     */
    [[nodiscard]] std::optional<int> square_from_name(std::string_view t_name);
}
