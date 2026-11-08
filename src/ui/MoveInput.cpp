#include <algorithm>
#include "ui/MoveInput.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::MoveInput::MoveInput(const BoardView& t_view)
    : m_view(t_view)
{
}

//-------------------------------------------------
// Input
//-------------------------------------------------

std::optional<sg::chess::core::Move> sg::chess::ui::MoveInput::HandleEvent(const sf::Event& t_event,
                                                                          const core::PositionInfo& t_info)
{
    if (const auto* moved = t_event.getIf<sf::Event::MouseMoved>())
    {
        m_cursor = sf::Vector2f(moved->position);
    }
    else if (const auto* pressed = t_event.getIf<sf::Event::MouseButtonPressed>())
    {
        m_cursor = sf::Vector2f(pressed->position);

        if (pressed->button == sf::Mouse::Button::Left)
        {
            return OnPress(m_cursor, t_info);
        }

        if (pressed->button == sf::Mouse::Button::Right)
        {
            Reset();
        }
    }
    else if (const auto* released = t_event.getIf<sf::Event::MouseButtonReleased>())
    {
        m_cursor = sf::Vector2f(released->position);

        if (released->button == sf::Mouse::Button::Left)
        {
            return OnRelease(m_cursor, t_info);
        }
    }

    return std::nullopt;
}

void sg::chess::ui::MoveInput::Reset() noexcept
{
    m_selected.reset();
    m_dragging = false;
    m_promotion.reset();
}

//-------------------------------------------------
// State (for drawing)
//-------------------------------------------------

std::vector<int> sg::chess::ui::MoveInput::TargetSquares(const core::PositionInfo& t_info) const
{
    std::vector<int> targets;

    if (!m_selected)
    {
        return targets;
    }

    for (const core::Move& move : t_info.legalMoves)
    {
        // A promotion appears four times with the same target square. It is listed once.
        if (move.from == *m_selected && std::ranges::find(targets, move.to) == targets.end())
        {
            targets.push_back(move.to);
        }
    }

    return targets;
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

std::optional<sg::chess::core::Move> sg::chess::ui::MoveInput::OnPress(const sf::Vector2f t_position,
                                                                      const core::PositionInfo& t_info)
{
    // While the promotion picker is shown, the next click chooses a piece or cancels.
    if (m_promotion)
    {
        const PendingPromotion pending{ *m_promotion };

        for (std::size_t i{ 0 }; i < BoardView::PROMOTION_TYPES.size(); ++i)
        {
            if (m_view.PromotionRect(i).contains(t_position))
            {
                Reset();
                return core::Move{ pending.from, pending.to, BoardView::PROMOTION_TYPES[i] };
            }
        }

        Reset();
        return std::nullopt;
    }

    const auto square{ m_view.SquareAt(t_position) };
    if (!square)
    {
        Reset();
        return std::nullopt;
    }

    // A piece is selected and the user clicks one of its target squares.
    if (m_selected && *square != *m_selected && HasMove(t_info, *m_selected, *square))
    {
        return TryMove(t_info, *m_selected, *square);
    }

    if (IsOwnPiece(t_info, *square))
    {
        m_selected = square;
        m_dragging = true;
    }
    else
    {
        Reset();
    }

    return std::nullopt;
}

std::optional<sg::chess::core::Move> sg::chess::ui::MoveInput::OnRelease(const sf::Vector2f t_position,
                                                                        const core::PositionInfo& t_info)
{
    if (!m_dragging || !m_selected)
    {
        return std::nullopt;
    }

    m_dragging = false;

    const auto square{ m_view.SquareAt(t_position) };

    if (square && *square != *m_selected && HasMove(t_info, *m_selected, *square))
    {
        return TryMove(t_info, *m_selected, *square);
    }

    // Released on its own square: it was a click, the piece stays selected.
    // Released somewhere else: the drop was not a legal move, so the selection is cancelled.
    if (square != m_selected)
    {
        m_selected.reset();
    }

    return std::nullopt;
}

std::optional<sg::chess::core::Move> sg::chess::ui::MoveInput::TryMove(const core::PositionInfo& t_info,
                                                                      const int t_from, const int t_to)
{
    std::vector<core::Move> candidates;

    for (const core::Move& move : t_info.legalMoves)
    {
        if (move.from == t_from && move.to == t_to)
        {
            candidates.push_back(move);
        }
    }

    if (candidates.empty())
    {
        return std::nullopt;
    }

    // More than one candidate means a promotion: ask which piece. The selection stays, the piece is highlighted.
    if (candidates.size() > 1)
    {
        m_promotion = PendingPromotion{ t_from, t_to };
        m_dragging = false;

        return std::nullopt;
    }

    Reset();
    return candidates.front();
}

bool sg::chess::ui::MoveInput::HasMove(const core::PositionInfo& t_info, const int t_from, const int t_to)
{
    return std::ranges::any_of(t_info.legalMoves,
                               [t_from, t_to](const core::Move& t_move) { return t_move.from == t_from && t_move.to == t_to; });
}

bool sg::chess::ui::MoveInput::IsOwnPiece(const core::PositionInfo& t_info, const int t_square)
{
    const auto piece{ t_info.position.At(t_square) };

    return piece && piece->color == t_info.position.SideToMove();
}
