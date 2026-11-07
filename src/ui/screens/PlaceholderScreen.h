#pragma once

#include <string>
#include <string_view>
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // PlaceholderScreen
    //-------------------------------------------------

    /**
     * @brief An empty page that only shows a title and a short description.
     *
     * Stands in for pages that are not implemented yet. Once a page gets real content,
     * it becomes a class of its own and the placeholder is dropped.
     */
    class PlaceholderScreen final : public Screen
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates a placeholder page.
         *
         * @param t_title The title, also used as the name in the navigation bar.
         * @param t_description What this page will do later.
         */
        PlaceholderScreen(std::string t_title, std::string t_description);

        PlaceholderScreen(const PlaceholderScreen& t_other) = delete;
        PlaceholderScreen(PlaceholderScreen&& t_other) noexcept = delete;
        PlaceholderScreen& operator=(const PlaceholderScreen& t_other) = delete;
        PlaceholderScreen& operator=(PlaceholderScreen&& t_other) noexcept = delete;

        ~PlaceholderScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        [[nodiscard]] std::string_view Title() const noexcept override;

        void DrawGui() override;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::string m_title;        ///< Title of the page
        std::string m_description;  ///< Text shown on the page
    };
}
