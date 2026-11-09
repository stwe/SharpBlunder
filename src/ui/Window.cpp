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

#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include "ui/Window.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::Window::Window(const WindowConfig& t_config)
    : m_window(sf::VideoMode(t_config.size), t_config.title, sf::Style::Titlebar | sf::Style::Close)
{
    m_window.setFramerateLimit(t_config.frameRateLimit);
}

//-------------------------------------------------
// Window state
//-------------------------------------------------

bool sg::chess::ui::Window::IsOpen() const
{
    return m_window.isOpen();
}

void sg::chess::ui::Window::Close()
{
    m_window.close();
}

sf::Vector2u sg::chess::ui::Window::Size() const
{
    return m_window.getSize();
}

//-------------------------------------------------
// Events
//-------------------------------------------------

std::optional<sf::Event> sg::chess::ui::Window::PollEvent()
{
    return m_window.pollEvent();
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::Window::Clear(const sf::Color t_color)
{
    m_window.clear(t_color);
}

void sg::chess::ui::Window::Display()
{
    m_window.display();
}
