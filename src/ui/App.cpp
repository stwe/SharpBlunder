#include <filesystem>
#include <memory>
#include <string_view>
#include "ui/App.h"
#include "ui/screens/AnalysisScreen.h"
#include "ui/screens/PlaceholderScreen.h"

namespace
{
    constexpr sf::Color BACKGROUND_COLOR{ 38, 36, 33 };

    constexpr float GUI_FONT_SIZE{ 18.0f };

    // Set by CMake: absolute path of the assets directory in the source tree.
    constexpr std::string_view ASSET_DIR{ SHARPBLUNDER_ASSET_DIR };
    constexpr std::string_view GUI_FONT{ "Roboto-Medium.ttf" };
    constexpr std::string_view PIECE_SET{ "cburnett" };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::App::App()
    : m_window(WindowConfig{})
    , m_gui(m_window, std::filesystem::path{ ASSET_DIR } / "fonts" / GUI_FONT, GUI_FONT_SIZE)
    , m_pieces(std::filesystem::path{ ASSET_DIR } / "pieces" / PIECE_SET)
{
    m_screens.Add(ScreenId::ANALYSIS, std::make_unique<AnalysisScreen>(m_pieces));
    m_screens.Add(ScreenId::GAMES, std::make_unique<PlaceholderScreen>(
        "My Games", "Import and analyze your games from lichess.org and chess.com."));
    m_screens.Add(ScreenId::TRAINING, std::make_unique<PlaceholderScreen>(
        "Training", "Learn and repeat your opening repertoire."));
    m_screens.Add(ScreenId::SETTINGS, std::make_unique<PlaceholderScreen>(
        "Settings", "Engine, accounts and appearance."));

    m_screens.SwitchTo(ScreenId::ANALYSIS);
}

//-------------------------------------------------
// Main loop
//-------------------------------------------------

void sg::chess::ui::App::Run()
{
    while (m_window.IsOpen())
    {
        const sf::Time dt{ m_clock.restart() };

        ProcessEvents();
        Update(dt);
        Render();
    }
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

void sg::chess::ui::App::ProcessEvents()
{
    while (const auto event = m_window.PollEvent())
    {
        // ImGui must see every event, otherwise it loses track of the mouse and the keys.
        m_gui.ProcessEvent(*event);

        if (event->is<sf::Event::Closed>())
        {
            m_window.Close();
            continue;
        }

        // A click on a GUI panel or typing into a text field must not reach the board.
        if (m_gui.WantsEvent(*event))
        {
            continue;
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape)
        {
            m_window.Close();
            continue;
        }

        m_screens.HandleEvent(*event);
    }
}

void sg::chess::ui::App::Update(const sf::Time t_dt)
{
    // From here on until Render(), the GUI widgets of this frame are built.
    m_gui.BeginFrame(t_dt);

    m_screens.Update(t_dt);
}

void sg::chess::ui::App::Render()
{
    m_window.Clear(BACKGROUND_COLOR);

    // First everything drawn by SFML (board, pieces), then the GUI on top of it.
    m_screens.Draw(m_window.GetRenderTarget());

    m_navigation.DrawGui(m_screens);
    m_screens.DrawGui();
    m_gui.Render();

    m_window.Display();
}
