#pragma once

#include <array>
#include <optional>
#include <string_view>
#include "core/Piece.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Position
    //-------------------------------------------------

    /**
     * @brief The pieces on the 64 squares of the board.
     *
     * Files and ranks are counted from zero: file 0 is the a-file, rank 0 is the first rank.
     * So a1 is (0, 0), h1 is (7, 0) and e4 is (4, 3).
     *
     * This is a plain value type. It can be copied freely and knows nothing about drawing.
     */
    class Position
    {
    public:
        //-------------------------------------------------
        // Constants
        //-------------------------------------------------

        static constexpr int BOARD_SIZE{ 8 };   ///< Number of files and ranks
        static constexpr int SQUARE_COUNT{ BOARD_SIZE * BOARD_SIZE };

        //-------------------------------------------------
        // Creation
        //-------------------------------------------------

        /**
         * @brief Creates an empty board.
         */
        Position() = default;

        /**
         * @brief Creates a position from the piece placement of a FEN string.
         *
         * Only the first field of the FEN is read (e.g. "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR").
         * The remaining fields (side to move, castling rights, ...) are ignored for now.
         *
         * @param t_fen A full FEN string or just its piece placement.
         *
         * @return The position.
         *
         * @throws std::invalid_argument If the piece placement is malformed.
         */
        [[nodiscard]] static Position FromFen(std::string_view t_fen);

        //-------------------------------------------------
        // Access
        //-------------------------------------------------

        /**
         * @brief Converts file and rank to a square index (0 = a1, 63 = h8).
         *
         * @param t_file The file, 0 to 7.
         * @param t_rank The rank, 0 to 7.
         *
         * @return The square index.
         */
        [[nodiscard]] static constexpr int Index(const int t_file, const int t_rank) noexcept
        {
            return t_rank * BOARD_SIZE + t_file;
        }

        /**
         * @brief Returns the piece on a square.
         *
         * @param t_file The file, 0 to 7.
         * @param t_rank The rank, 0 to 7.
         *
         * @return The piece, or std::nullopt if the square is empty.
         *
         * @throws std::out_of_range If file or rank is outside the board.
         */
        [[nodiscard]] std::optional<Piece> At(int t_file, int t_rank) const;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::array<std::optional<Piece>, SQUARE_COUNT> m_squares{}; ///< Index = rank * 8 + file
    };
}
