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

#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>
#include "core/I18n.h"
#include "core/Settings.h"
#include "ui/App.h"
#include "ui/screens/AnalysisScreen.h"
#include "ui/screens/PlaceholderScreen.h"

namespace
{
    constexpr sf::Color BACKGROUND_COLOR{ 38, 36, 33 };

    constexpr float GUI_FONT_SIZE{ 18.0f };

    // The number of lines the analysis calculates (MultiPV).
    constexpr int ANALYSIS_LINES{ 3 };

    // Set by CMake: absolute path of the assets directory in the source tree.
    constexpr std::string_view ASSET_DIR{ SHARPBLUNDER_ASSET_DIR };
    constexpr std::string_view GUI_FONT{ "Roboto-Medium.ttf" };
    constexpr std::string_view PIECE_SET{ "cburnett" };
    constexpr std::string_view OPENING_FILE{ "openings.tsv" };
    constexpr std::string_view TRANSLATIONS_DIR{ "i18n" };

    // Reads the settings. Whatever is wrong with the file, the program starts: the user is told on the console.
    sg::chess::core::Settings LoadSettings()
    {
        auto loaded{ sg::chess::core::load_settings(sg::chess::core::settings_path()) };

        if (!loaded.problem.empty())
        {
            std::cerr << "Warning: " << loaded.problem << '\n';
        }

        return std::move(loaded.settings);
    }
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::App::App(const std::string& t_stockfish)
    : m_engine(t_stockfish)
    , m_rules(m_engine)
    , m_analyzer(t_stockfish, ANALYSIS_LINES)
    , m_window(WindowConfig{})
    , m_gui(m_window, std::filesystem::path{ ASSET_DIR } / "fonts" / GUI_FONT, GUI_FONT_SIZE)
    , m_pieces(std::filesystem::path{ ASSET_DIR } / "pieces" / PIECE_SET)
    , m_openings(core::OpeningBook::FromFile(std::filesystem::path{ ASSET_DIR } / "openings" / OPENING_FILE))
    , m_settings(LoadSettings())
{
    // The language is set before a screen is made: the texts are translated when they are shown.
    ApplyLanguage();

    m_screens.Add(ScreenId::ANALYSIS, std::make_unique<AnalysisScreen>(m_pieces, m_rules, m_analyzer, m_openings));
    m_screens.Add(ScreenId::GAMES, std::make_unique<PlaceholderScreen>(
        core::tr_noop("My Games"), core::tr_noop("Import and analyze your games from lichess.org and chess.com.")));
    m_screens.Add(ScreenId::TRAINING, std::make_unique<PlaceholderScreen>(
        core::tr_noop("Training"), core::tr_noop("Learn and repeat your opening repertoire.")));
    m_screens.Add(ScreenId::SETTINGS, std::make_unique<PlaceholderScreen>(
        core::tr_noop("Settings"), core::tr_noop("Engine, accounts and appearance.")));

    m_screens.SwitchTo(ScreenId::ANALYSIS);
}

//-------------------------------------------------
// Language
//-------------------------------------------------

void sg::chess::ui::App::ApplyLanguage()
{
    const std::filesystem::path directory{ std::filesystem::path{ ASSET_DIR } / TRANSLATIONS_DIR };

    const std::string code{ core::resolve_language(m_settings.language, core::system_language(),
                                                   core::available_languages(directory)) };

    if (const auto problem{ core::set_language(code, directory) })
    {
        std::cerr << "Warning: " << *problem << '\n';
    }
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
