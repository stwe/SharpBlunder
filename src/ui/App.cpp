#include <filesystem>
#include <string_view>
#include "ui/App.h"
#include "core/Util.h"

//-------------------------------------------------
// Helper
//-------------------------------------------------

namespace
{
    constexpr sf::Color BACKGROUND_COLOR{ 38, 36, 33 };

    constexpr sf::Vector2f BOARD_ORIGIN{ 60.f, 20.f };
    constexpr float SQUARE_SIZE{ 80.f };

    // Set by CMake: absolute path of the assets directory in the source tree.
    constexpr std::string_view ASSET_DIR{ SHARPBLUNDER_ASSET_DIR };
    constexpr std::string_view PIECE_SET{ "cburnett" };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::App::App()
    : m_window(WindowConfig{ .title = "SharpBlunder" })
    , m_pieces(std::filesystem::path{ ASSET_DIR } / "pieces" / PIECE_SET)
    , m_position(core::Position::FromFen(core::START_FEN))
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, m_pieces)
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
    m_boardView.Draw(m_window.GetRenderTarget(), m_position);
    // The side panel will be drawn here.
    m_window.Display();
}
