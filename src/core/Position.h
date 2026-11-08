#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include "core/Piece.h"
#include "core/Square.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Position
    //-------------------------------------------------

    /**
     * @brief A chess position: the pieces on the 64 squares and the state needed to continue the game.
     *
     * This is a plain value type that can be copied freely. It knows nothing about drawing
     * and it does not know which moves are legal; that is the job of a RulesProvider.
     */
    class Position
    {
    public:
        //-------------------------------------------------
        // Constants
        //-------------------------------------------------

        /**
         * @brief The standard starting position in FEN (Forsyth-Edwards Notation) format.
         */
        static constexpr std::string_view START_FEN{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };

        //-------------------------------------------------
        // Creation
        //-------------------------------------------------

        /**
         * @brief Creates an empty board with white to move.
         */
        Position() = default;

        /**
         * @brief Creates a position from a FEN string.
         *
         * The first field (piece placement) is required. The other fields (side to move, castling rights,
         * en passant square, halfmove clock, fullmove number) may be omitted and default to "w - - 0 1".
         *
         * @param t_fen A FEN string, e.g. START_FEN.
         *
         * @return The position.
         *
         * @throws std::invalid_argument If the FEN is malformed.
         */
        [[nodiscard]] static Position FromFen(std::string_view t_fen);

        //-------------------------------------------------
        // Access
        //-------------------------------------------------

        /**
         * @brief Writes the position as a FEN string.
         *
         * @return The FEN with all six fields.
         */
        [[nodiscard]] std::string ToFen() const;

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

        /**
         * @brief Returns the piece on a square.
         *
         * @param t_square The square index, 0 to 63.
         *
         * @return The piece, or std::nullopt if the square is empty.
         *
         * @throws std::out_of_range If the square is outside the board.
         */
        [[nodiscard]] std::optional<Piece> At(int t_square) const;

        /**
         * @brief Finds the king of a side.
         *
         * @param t_color The side.
         *
         * @return The square of the king, or std::nullopt if there is none.
         */
        [[nodiscard]] std::optional<int> KingSquare(Color t_color) const noexcept;

        [[nodiscard]] Color SideToMove() const noexcept { return m_sideToMove; }
        [[nodiscard]] int HalfmoveClock() const noexcept { return m_halfmoveClock; }
        [[nodiscard]] int FullmoveNumber() const noexcept { return m_fullmoveNumber; }

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::array<std::optional<Piece>, SQUARE_COUNT> m_squares{};  ///< Index = rank * 8 + file

        Color m_sideToMove{ Color::WHITE };  ///< The side that moves next
        std::string m_castling{ "-" };       ///< Castling rights as in FEN, e.g. "KQkq" or "-"
        std::string m_enPassant{ "-" };      ///< En passant target square as in FEN, e.g. "e3" or "-"
        int m_halfmoveClock{ 0 };            ///< Halfmoves since the last capture or pawn move (50-move rule)
        int m_fullmoveNumber{ 1 };           ///< Starts at 1 and is incremented after Black's move
    };
}
