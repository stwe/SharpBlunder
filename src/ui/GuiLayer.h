#pragma once

#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <filesystem>
#include "ui/Window.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // GuiLayer
    //-------------------------------------------------

    /**
     * @brief Connects Dear ImGui (via ImGui-SFML) to a Window.
     *
     * ImGui is an "immediate mode" GUI: widgets are not objects that live between frames.
     * Instead the widget calls (ImGui::Button, ...) are repeated every frame, between BeginFrame() and Render().
     *
     * Dear ImGui is initialized in the constructor and shut down in the destructor.
     */
    class GuiLayer
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Initializes Dear ImGui for a window and loads the GUI font.
         *
         * Dear ImGui's built-in font is not used because it is tiny and has no umlauts.
         *
         * @param t_window The window to render into. It must outlive this object.
         * @param t_fontFile Path to a TrueType font.
         * @param t_fontSize Font size in pixels.
         *
         * @throws std::runtime_error If ImGui cannot be initialized or the font cannot be loaded.
         */
        GuiLayer(Window& t_window, const std::filesystem::path& t_fontFile, float t_fontSize);

        GuiLayer(const GuiLayer& t_other) = delete;
        GuiLayer(GuiLayer&& t_other) noexcept = delete;
        GuiLayer& operator=(const GuiLayer& t_other) = delete;
        GuiLayer& operator=(GuiLayer&& t_other) noexcept = delete;

        ~GuiLayer() noexcept;

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Gives an event to Dear ImGui. Call it for every event, before anyone else handles it.
         *
         * @param t_event The event.
         */
        void ProcessEvent(const sf::Event& t_event);

        /**
         * @brief Tells whether Dear ImGui wants an event for itself.
         *
         * True for mouse events while the mouse is over a GUI panel (or a widget is being dragged)
         * and for keyboard events while a text field has the focus. Such events must not reach the board.
         *
         * @param t_event The event.
         *
         * @return True if the event belongs to the GUI.
         */
        [[nodiscard]] bool WantsEvent(const sf::Event& t_event) const;

        /**
         * @brief Starts a new GUI frame. All ImGui calls of the frame come after this.
         *
         * @param t_dt The time since the previous frame.
         */
        void BeginFrame(sf::Time t_dt);

        /**
         * @brief Draws everything that was built since BeginFrame(). Call it right before Window::Display().
         */
        void Render();

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        Window& m_window; ///< The window ImGui renders into (not owned)
    };
}
