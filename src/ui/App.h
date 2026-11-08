#pragma once

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <string>
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
     * @brief The application: owns the window, the engine and the screens and runs the main loop.
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
         *        loads the pieces and creates the screens.
         *
         * @param t_stockfish Name or path of the Stockfish executable.
         *
         * @throws std::runtime_error If Stockfish cannot be started or the GUI font or the piece images cannot be loaded.
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
        // - m_rules uses m_engine, m_gui needs m_window, and the screens use m_pieces and m_rules,
        //   so all of them come before m_screens.

        engine::Engine m_engine;         ///< The Stockfish process
        engine::StockfishRules m_rules;  ///< Answers the questions about the rules by asking m_engine
        Window m_window;                 ///< The main window
        GuiLayer m_gui;                  ///< Dear ImGui, connected to m_window
        PieceSet m_pieces;               ///< The piece images, shared by all screens with a board
        ScreenManager m_screens;         ///< All pages of the application
        NavigationBar m_navigation;      ///< The menu on the left
        sf::Clock m_clock;               ///< Measures the time of a frame
    };
}
