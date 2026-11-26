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

#pragma once

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <optional>
#include <string>
#include "core/OpeningBook.h"
#include "core/Settings.h"
#include "engine/Analyzer.h"
#include "engine/Engine.h"
#include "engine/StockfishRules.h"
#include "ui/GuiLayer.h"
#include "ui/NavigationBar.h"
#include "ui/PieceSet.h"
#include "ui/ScreenManager.h"
#include "ui/Window.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // App
    //-------------------------------------------------

    /**
     * @brief The application: owns the window, the engines and the screens and runs the main loop.
     *
     * Every frame does the same three things in the same order:
     * handle input, update the state, draw the result.
     */
    class App
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the application: starts Stockfish, opens the window, sets up the GUI,
         *        loads the pieces and the opening names and creates the screens.
         *
         * @param t_stockfish Name or path of the Stockfish executable.
         *
         * @throws std::runtime_error If Stockfish cannot be started or the GUI font, the piece images or the opening names cannot be loaded.
         */
        explicit App(const std::string& t_stockfish);

        App(const App& t_other) = delete;
        App(App&& t_other) noexcept = delete;
        App& operator=(const App& t_other) = delete;
        App& operator=(App&& t_other) noexcept = delete;

        ~App() noexcept = default;

        //-------------------------------------------------
        // Main loop
        //-------------------------------------------------

        /**
         * @brief Runs the main loop until the window is closed.
         */
        void Run();

    private:
        //-------------------------------------------------
        // Language
        //-------------------------------------------------

        /**
         * @brief Chooses the language of the texts as the settings say.
         *
         * "auto" is the language of the system. If there is no translation for the language, the texts are English.
         */
        void ApplyLanguage();

        /**
         * @brief Puts the settings to work and saves them. The settings page calls it after every change.
         *
         * @return std::nullopt if the file was saved, or what went wrong.
         */
        [[nodiscard]] std::optional<std::string> ApplySettings();

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Handles all pending events (close button, key presses, mouse, ...).
         */
        void ProcessEvents();

        /**
         * @brief Advances the application state by one frame.
         *
         * @param t_dt The time since the previous frame.
         */
        void Update(sf::Time t_dt);

        /**
         * @brief Draws the current state and shows it in the window.
         */
        void Render();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // Members are initialized in the order of their declaration and destroyed in the reverse order.
        // - The engine comes first: if Stockfish is missing, the program stops before a window appears.
        // - m_rules uses m_engine, m_gui needs m_window, and the screens use m_pieces, m_rules, m_analyzer
        //   and m_openings, so all of them come before m_screens.

        engine::Engine m_engine;         ///< The Stockfish process for the rules
        engine::StockfishRules m_rules;  ///< Answers the questions about the rules by asking m_engine
        engine::Analyzer m_analyzer;     ///< A second Stockfish process that analyzes the position
        Window m_window;                 ///< The main window
        GuiLayer m_gui;                  ///< Dear ImGui, connected to m_window
        PieceSet m_pieces;               ///< The piece images, shared by all screens with a board
        core::OpeningBook m_openings;    ///< The names of the openings
        core::Settings m_settings;       ///< What the user has set, from the file settings.json
        ScreenManager m_screens;         ///< All pages of the application
        NavigationBar m_navigation;      ///< The menu on the left
        sf::Clock m_clock;               ///< Measures the time of a frame
    };
}
