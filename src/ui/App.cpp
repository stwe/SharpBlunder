#include "ui/App.h"

namespace
{
    constexpr sf::Color BACKGROUND_COLOR{ 38, 36, 33 };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::App::App()
    : m_window(WindowConfig{ .title = "SharpBlunder" })
{
}

//-------------------------------------------------
// Main loop
//-------------------------------------------------

void sg::chess::ui::App::Run()
{
    while (m_window.IsOpen())
    {
        ProcessEvents();
        Update();
        Render();
    }
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

void sg::chess::ui::App::ProcessEvents()
{
    while (const auto event{ m_window.PollEvent() })
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.Close();
        }
        else if (const auto* key{ event->getIf<sf::Event::KeyPressed>() };
                 key && key->code == sf::Keyboard::Key::Escape)
        {
            m_window.Close();
        }
    }
}

void sg::chess::ui::App::Update()
{
    // Nothing to update yet.
}

void sg::chess::ui::App::Render()
{
    m_window.Clear(BACKGROUND_COLOR);
    // The board and the side panel will be drawn here.
    m_window.Display();
}
