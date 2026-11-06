#pragma once

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
         * @brief Creates the application and opens its window.
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

        Window m_window; ///< The main window
    };
}
