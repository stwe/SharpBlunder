// This file is part of the SharpBlunder project.
//
// Copyright (c) 2026 stwe <https://github.com/stwe/SharpBlunder>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <imgui.h>
#include <utility>
#include "ui/EvalBar.h"

namespace
{
    constexpr sf::Color BLACK_PART{ 64, 61, 57 };
    constexpr sf::Color WHITE_PART{ 240, 240, 240 };
    constexpr sf::Color MIDDLE_MARK{ 200, 70, 70, 200 };

    constexpr ImU32 TEXT_ON_WHITE{ IM_COL32(40, 40, 40, 255) };
    constexpr ImU32 TEXT_ON_BLACK{ IM_COL32(235, 235, 235, 255) };

    constexpr float LABEL_FONT_SIZE{ 13.0f };
    constexpr float LABEL_PADDING{ 4.0f };
    constexpr float MIDDLE_MARK_HEIGHT{ 2.0f };

    // How fast the bar follows its target: the remaining distance shrinks by this factor per second (as e^-x).
    constexpr double FOLLOW_SPEED{ 8.0 };
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::EvalBar::EvalBar(const sf::Vector2f t_origin, const sf::Vector2f t_size)
    : m_origin(t_origin)
    , m_size(t_size)
{
}

//-------------------------------------------------
// State
//-------------------------------------------------

void sg::chess::ui::EvalBar::SetTarget(const double t_whiteShare, std::string t_label)
{
    m_target = std::clamp(t_whiteShare, 0.0, 1.0);
    m_label = std::move(t_label);
}

void sg::chess::ui::EvalBar::Update(const sf::Time t_dt)
{
    const double step{ 1.0 - std::exp(-FOLLOW_SPEED * static_cast<double>(t_dt.asSeconds())) };

    m_shown += (m_target - m_shown) * step;
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::EvalBar::Draw(sf::RenderTarget& t_target, const bool t_flipped) const
{
    const float whiteHeight{ static_cast<float>(m_shown) * m_size.y };
    const float blackHeight{ m_size.y - whiteHeight };

    // White is at the bottom, like on the board, or at the top if the board is turned around.
    const float blackTop{ t_flipped ? m_origin.y + whiteHeight : m_origin.y };
    const float whiteTop{ t_flipped ? m_origin.y : m_origin.y + blackHeight };

    sf::RectangleShape part({ m_size.x, blackHeight });
    part.setPosition({ m_origin.x, blackTop });
    part.setFillColor(BLACK_PART);
    t_target.draw(part);

    part.setSize({ m_size.x, whiteHeight });
    part.setPosition({ m_origin.x, whiteTop });
    part.setFillColor(WHITE_PART);
    t_target.draw(part);

    // A mark in the middle shows where an equal position is.
    sf::RectangleShape mark({ m_size.x, MIDDLE_MARK_HEIGHT });
    mark.setPosition({ m_origin.x, m_origin.y + m_size.y / 2.0f - MIDDLE_MARK_HEIGHT / 2.0f });
    mark.setFillColor(MIDDLE_MARK);
    t_target.draw(mark);
}

void sg::chess::ui::EvalBar::DrawLabel(const bool t_flipped) const
{
    if (m_label.empty())
    {
        return;
    }

    ImFont* font{ ImGui::GetFont() };
    const ImVec2 textSize{ font->CalcTextSizeA(LABEL_FONT_SIZE, FLT_MAX, 0.0f, m_label.c_str()) };

    // The text sits at the end of the bar of the better side, on that side's color.
    const bool whiteLeads{ m_target >= 0.5 };
    const bool atBottom{ whiteLeads != t_flipped };

    const float x{ m_origin.x + (m_size.x - textSize.x) / 2.0f };
    const float y{ atBottom ? m_origin.y + m_size.y - textSize.y - LABEL_PADDING : m_origin.y + LABEL_PADDING };

    ImGui::GetForegroundDrawList()->AddText(font, LABEL_FONT_SIZE, ImVec2(x, y),
                                            whiteLeads ? TEXT_ON_WHITE : TEXT_ON_BLACK, m_label.c_str());
}
