#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <string_view>

namespace sg::chess::ui
{
    //-------------------------------------------------
    // Screen
    //-------------------------------------------------

    /**
     * @brief Base class of all pages of the application.
     *
     * Only one screen is active at a time. Every frame the active screen receives, in this order:
     * HandleEvent() for each pending event, Update(), Draw() and DrawGui().
     *
     * All functions except Title() have an empty default implementation, so a screen
     * overrides only what it needs.
     */
    class Screen
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        Screen() = default;

        Screen(const Screen& t_other) = delete;
        Screen(Screen&& t_other) noexcept = delete;
        Screen& operator=(const Screen& t_other) = delete;
        Screen& operator=(Screen&& t_other) noexcept = delete;

        /**
         * @brief Virtual, because screens are deleted through a pointer to this base class.
         */
        virtual ~Screen() noexcept = default;

        //-------------------------------------------------
        // Identity
        //-------------------------------------------------

        /**
         * @brief Returns the name of the screen, shown in the navigation bar.
         *
         * @return The title.
         */
        [[nodiscard]] virtual std::string_view Title() const noexcept = 0;

        //-------------------------------------------------
        // Lifecycle
        //-------------------------------------------------

        /**
         * @brief Called when the screen becomes the active one.
         */
        virtual void OnEnter() {}

        /**
         * @brief Called when another screen takes over.
         */
        virtual void OnExit() {}

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Handles one input event.
         *
         * Events that Dear ImGui wants for itself (e.g. a click on a button) are not passed on.
         *
         * @param t_event The event.
         */
        virtual void HandleEvent(const sf::Event& t_event) { static_cast<void>(t_event); }

        /**
         * @brief Advances the state of the screen by one frame.
         *
         * @param t_dt The time since the previous frame.
         */
        virtual void Update(const sf::Time t_dt) { static_cast<void>(t_dt); }

        /**
         * @brief Draws with SFML (board, pieces, ...). Runs before DrawGui(), so the GUI appears on top.
         *
         * @param t_target The surface to draw on.
         */
        virtual void Draw(sf::RenderTarget& t_target) { static_cast<void>(t_target); }

        /**
         * @brief Builds the Dear ImGui widgets of the screen (panels, lists, buttons, ...).
         */
        virtual void DrawGui() {}
    };
}
