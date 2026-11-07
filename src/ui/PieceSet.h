#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <array>
#include <filesystem>
#include "core/Piece.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // PieceSet
    //-------------------------------------------------

    /**
     * @brief The textures of the 12 chess pieces (6 types, 2 colors).
     *
     * A piece set is a directory with one PNG per piece, named after the lichess convention:
     * wK.png, wQ.png, ... bP.png (color letter 'w' or 'b' followed by the piece letter).
     * To use another set, point the constructor to another directory.
     */
    class PieceSet
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Loads all 12 piece images from a directory.
         *
         * @param t_directory The directory containing wK.png, wQ.png, ... bP.png.
         *
         * @throws std::runtime_error If one of the images cannot be loaded.
         */
        explicit PieceSet(const std::filesystem::path& t_directory);

        PieceSet(const PieceSet& t_other) = delete;
        PieceSet(PieceSet&& t_other) noexcept = delete;
        PieceSet& operator=(const PieceSet& t_other) = delete;
        PieceSet& operator=(PieceSet&& t_other) noexcept = delete;

        ~PieceSet() noexcept = default;

        //-------------------------------------------------
        // Access
        //-------------------------------------------------

        /**
         * @brief Returns the texture of a piece.
         *
         * @param t_piece The piece.
         *
         * @return The texture. It stays valid as long as this PieceSet lives.
         */
        [[nodiscard]] const sf::Texture& Get(core::Piece t_piece) const;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::array<sf::Texture, 12> m_textures; ///< Index = color * 6 + type
    };
}
