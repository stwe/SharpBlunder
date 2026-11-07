#include "ui/screens/AnalysisScreen.h"
#include "ui/Layout.h"

namespace
{
    constexpr float SQUARE_SIZE{ 80.0f };
    constexpr float BOARD_MARGIN{ 30.0f };

    // The board starts to the right of the navigation bar.
    constexpr sf::Vector2f BOARD_ORIGIN{ sg::chess::ui::NAVIGATION_WIDTH + BOARD_MARGIN, 40.0f };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::AnalysisScreen::AnalysisScreen(const PieceSet& t_pieces)
    : m_position(core::Position::FromFen(core::Position::START_FEN))
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, t_pieces)
{
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::AnalysisScreen::Title() const noexcept
{
    return "Analysis";
}

void sg::chess::ui::AnalysisScreen::Draw(sf::RenderTarget& t_target)
{
    m_boardView.Draw(t_target, m_position);
}
