#include <stdexcept>
#include <string>
#include "ui/PieceSet.h"

//-------------------------------------------------
// Helper
//-------------------------------------------------

namespace
{
    constexpr std::size_t TYPES_PER_COLOR{ sg::chess::core::PIECE_LETTERS.size() };

    std::size_t TextureIndex(const sg::chess::core::Piece t_piece)
    {
        return static_cast<std::size_t>(t_piece.color) * TYPES_PER_COLOR + static_cast<std::size_t>(t_piece.type);
    }

    std::string FileName(const sg::chess::core::Piece t_piece)
    {
        std::string name;
        name += t_piece.color == sg::chess::core::Color::WHITE ? 'w' : 'b';
        name += type_letter(t_piece.type);
        name += ".png";

        return name;
    }
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::PieceSet::PieceSet(const std::filesystem::path& t_directory)
{
    for (const auto color : { core::Color::WHITE, core::Color::BLACK })
    {
        for (std::size_t i{ 0 }; i < TYPES_PER_COLOR; ++i)
        {
            const core::Piece piece{ static_cast<core::PieceType>(i), color };
            const auto path{ t_directory / FileName(piece) };

            sf::Texture& texture{ m_textures[TextureIndex(piece)] };
            if (!texture.loadFromFile(path))
            {
                throw std::runtime_error("Cannot load piece image: " + path.string());
            }

            // The images are larger than the squares. Smoothing plus mipmaps keep the edges clean when scaled down.
            texture.setSmooth(true);
            static_cast<void>(texture.generateMipmap()); // not fatal if unsupported
        }
    }
}

//-------------------------------------------------
// Access
//-------------------------------------------------

const sf::Texture& sg::chess::ui::PieceSet::Get(const core::Piece t_piece) const
{
    return m_textures[TextureIndex(t_piece)];
}
