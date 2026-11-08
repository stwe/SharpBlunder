#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <optional>
#include <string>

namespace sg::chess::ui
{
    //-------------------------------------------------
    // WindowConfig
    //-------------------------------------------------

    /**
     * @brief Settings used to create a Window.
     */
    struct WindowConfig
    {
        sf::Vector2u size{ 1280, 720 };       ///< Client area size in pixels
        std::string title{ "SharpBlunder" };  ///< Text shown in the title bar
        unsigned int frameRateLimit{ 60 };    ///< Maximum frames per second (0 = unlimited)
    };

    //-------------------------------------------------
    // Window
    //-------------------------------------------------

    /**
     * @brief Thin RAII wrapper around sf::RenderWindow.
     *
     * The window is opened in the constructor and closed automatically in the destructor.
     * The rest of the application talks to this class instead of using SFML's window directly,
     * so window handling lives in exactly one place.
     */
    class Window
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Opens a fixed-size window (title bar and close button, not resizable).
         *
         * Also serves as the default constructor, because every parameter has a default.
         *
         * @param t_config Size, title and frame rate limit of the window.
         */
        explicit Window(const WindowConfig& t_config = {});

        Window(const Window& t_other) = delete;
        Window(Window&& t_other) noexcept = delete;
        Window& operator=(const Window& t_other) = delete;
        Window& operator=(Window&& t_other) noexcept = delete;

        ~Window() noexcept = default;

        //-------------------------------------------------
        // Window state
        //-------------------------------------------------

        /**
         * @brief Tells whether the window is still open.
         *
         * @return True until Close() has been called or the user closed the window.
         */
        [[nodiscard]] bool IsOpen() const;

        /**
         * @brief Closes the window. IsOpen() returns false afterwards.
         */
        void Close();

        /**
         * @brief Returns the size of the client area.
         *
         * @return Width and height in pixels.
         */
        [[nodiscard]] sf::Vector2u Size() const;

        //-------------------------------------------------
        // Events
        //-------------------------------------------------

        /**
         * @brief Fetches the next pending event from the operating system.
         *
         * Call it in a loop until it returns an empty optional:
         * @code
         * while (const auto event = window.PollEvent()) { ... }
         * @endcode
         *
         * @return The next event, or std::nullopt if the queue is empty.
         */
        [[nodiscard]] std::optional<sf::Event> PollEvent();

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Fills the whole window with a color. Call once at the start of every frame.
         *
         * @param t_color The background color.
         */
        void Clear(sf::Color t_color = sf::Color::Black);

        /**
         * @brief Shows everything drawn since the last Clear(). Call once at the end of every frame.
         */
        void Display();

        /**
         * @brief Gives access to the surface that things are drawn onto.
         *
         * @return The render target of this window.
         */
        [[nodiscard]] sf::RenderTarget& GetRenderTarget() noexcept { return m_window; }

        /**
         * @brief Gives access to the SFML window itself.
         *
         * Only meant for the GUI backend (Dear ImGui), which needs the window and not just a render target.
         * Everything else should use the functions of this class.
         *
         * @return The underlying SFML window.
         */
        [[nodiscard]] sf::RenderWindow& GetRenderWindow() noexcept { return m_window; }

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        sf::RenderWindow m_window; ///< The underlying SFML window
    };
}
