#pragma once

#include "ui/ScreenManager.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // NavigationBar
    //-------------------------------------------------

    /**
     * @brief The menu on the left side of the window: one entry per screen.
     *
     * The bar has no state of its own. It reads the screens from the ScreenManager
     * and asks it to switch when an entry is clicked.
     */
    class NavigationBar
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        NavigationBar() = default;

        NavigationBar(const NavigationBar& t_other) = delete;
        NavigationBar(NavigationBar&& t_other) noexcept = delete;
        NavigationBar& operator=(const NavigationBar& t_other) = delete;
        NavigationBar& operator=(NavigationBar&& t_other) noexcept = delete;

        ~NavigationBar() noexcept = default;

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Builds the navigation bar with Dear ImGui. Call it between GuiLayer::BeginFrame() and Render().
         *
         * @param t_screens The screens to list. The clicked one becomes the active screen.
         */
        void DrawGui(ScreenManager& t_screens) const;
    };
}
