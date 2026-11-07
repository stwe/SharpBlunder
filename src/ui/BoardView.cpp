#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "ui/BoardView.h"

//-------------------------------------------------
// Helper
//-------------------------------------------------

namespace
{
    constexpr sf::Color LIGHT_SQUARE{ 240, 217, 181 };
    constexpr sf::Color DARK_SQUARE{ 181, 136, 99 };

    constexpr int BOARD_SIZE{sg::chess::core::Position::BOARD_SIZE };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::BoardView::BoardView(const sf::Vector2f t_origin, const float t_squareSize, const PieceSet& t_pieces)
    : m_origin(t_origin)
    , m_squareSize(t_squareSize)
    , m_pieces(t_pieces)
{
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::BoardView::Draw(sf::RenderTarget& t_target, const core::Position& t_position) const
{
    DrawSquares(t_target);
    DrawPieces(t_target, t_position);
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

sf::Vector2f sg::chess::ui::BoardView::SquarePosition(const int t_file, const int t_rank) const
{
    return {
        m_origin.x + static_cast<float>(t_file) * m_squareSize,
        m_origin.y + static_cast<float>(BOARD_SIZE - 1 - t_rank) * m_squareSize
    };
}

void sg::chess::ui::BoardView::DrawSquares(sf::RenderTarget& t_target) const
{
    sf::RectangleShape square({ m_squareSize, m_squareSize });

    for (int rank{ 0 }; rank < BOARD_SIZE; ++rank)
    {
        for (int file{ 0 }; file < BOARD_SIZE; ++file)
        {
            // a1 (file 0, rank 0) is a dark square
            square.setFillColor((file + rank) % 2 == 0 ? DARK_SQUARE : LIGHT_SQUARE);
            square.setPosition(SquarePosition(file, rank));
            t_target.draw(square);
        }
    }
}

void sg::chess::ui::BoardView::DrawPieces(sf::RenderTarget& t_target, const core::Position& t_position) const
{
    for (int rank{ 0 }; rank < BOARD_SIZE; ++rank)
    {
        for (int file{ 0 }; file < BOARD_SIZE; ++file)
        {
            const auto piece{ t_position.At(file, rank) };
            if (!piece)
            {
                continue;
            }

            const sf::Texture& texture{ m_pieces.Get(*piece) };
            const float scale{ m_squareSize / static_cast<float>(texture.getSize().x) };

            sf::Sprite sprite(texture);
            sprite.setScale({ scale, scale });
            sprite.setPosition(SquarePosition(file, rank));
            t_target.draw(sprite);
        }
    }
}
