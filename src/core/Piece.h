#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Color / PieceType
    //-------------------------------------------------

    /**
     * @brief The two sides.
     */
    enum class Color : std::uint8_t
    {
        WHITE,
        BLACK
    };

    /**
     * @brief The six kinds of chess pieces.
     *
     * The order matches PIECE_LETTERS, so a PieceType can be used as an index into it.
     */
    enum class PieceType : std::uint8_t
    {
        KING,
        QUEEN,
        ROOK,
        BISHOP,
        KNIGHT,
        PAWN
    };

    //-------------------------------------------------
    // Piece
    //-------------------------------------------------

    /**
     * @brief A piece on the board: its kind and its color.
     */
    struct Piece
    {
        PieceType type;
        Color color;

        [[nodiscard]] constexpr bool operator==(const Piece&) const noexcept = default;
    };

    //-------------------------------------------------
    // Letters (FEN)
    //-------------------------------------------------

    /**
     * @brief The upper case letters of the piece types, indexed by PieceType.
     */
    inline constexpr std::string_view PIECE_LETTERS{ "KQRBNP" };

    /**
     * @brief Returns the upper case letter of a piece type.
     *
     * @param t_type The piece type.
     *
     * @return One of 'K', 'Q', 'R', 'B', 'N', 'P'.
     */
    [[nodiscard]] constexpr char type_letter(const PieceType t_type) noexcept
    {
        return PIECE_LETTERS[static_cast<std::size_t>(t_type)];
    }

    /**
     * @brief Returns the FEN character of a piece: upper case for white, lower case for black.
     *
     * @param t_piece The piece.
     *
     * @return The FEN character, e.g. 'K' for the white king or 'p' for a black pawn.
     */
    [[nodiscard]] constexpr char to_fen_char(const Piece t_piece) noexcept
    {
        const char letter{ type_letter(t_piece.type) };

        return t_piece.color == Color::WHITE ? letter : static_cast<char>(letter - 'A' + 'a');
    }

    /**
     * @brief Converts a FEN character to a piece.
     *
     * @param t_c A FEN character such as 'K', 'q' or 'P'.
     *
     * @return The piece, or std::nullopt if the character does not denote a piece.
     */
    [[nodiscard]] constexpr std::optional<Piece> from_fen_char(const char t_c) noexcept
    {
        const bool isLower{ t_c >= 'a' && t_c <= 'z' };
        const char upper{ isLower ? static_cast<char>(t_c - 'a' + 'A') : t_c };

        const auto index{ PIECE_LETTERS.find(upper) };
        if (index == std::string_view::npos)
        {
            return std::nullopt;
        }

        return Piece{ static_cast<PieceType>(index), isLower ? Color::BLACK : Color::WHITE };
    }
}
