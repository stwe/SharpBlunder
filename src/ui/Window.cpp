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
