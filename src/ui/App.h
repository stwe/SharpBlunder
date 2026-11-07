#pragma once

#include "core/Position.h"
#include "ui/BoardView.h"
#include "ui/PieceSet.h"
#include "ui/Window.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // App
    //-------------------------------------------------

    /**
     * @brief The application: owns the window and runs the main loop.
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
         * @brief Creates the application: opens the window, loads the pieces and sets up the start position.
         *
         * @throws std::runtime_error If the piece images cannot be loaded.
         */
        App();

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
         * @brief Handles all pending events (close button, key presses, ...).
         */
        void ProcessEvents();

        /**
         * @brief Advances the application state by one frame.
         */
        void Update();

        /**
         * @brief Draws the current state and shows it in the window.
         */
        void Render();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // Members are initialized in the order of their declaration, not in the order of the
        // constructor's initializer list. m_boardView refers to m_pieces, so m_pieces must come first.

        Window m_window;            ///< The main window
        PieceSet m_pieces;          ///< The piece images
        core::Position m_position;  ///< The position that is shown
        BoardView m_boardView;      ///< Draws m_position
    };
}
