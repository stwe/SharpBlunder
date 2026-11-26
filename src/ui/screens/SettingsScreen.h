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

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "core/I18n.h"
#include "core/Settings.h"
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // SettingsScreen
    //-------------------------------------------------

    /**
     * @brief The page where the user changes the settings of the program.
     *
     * Every change is applied and saved at once: there is no button to confirm. The page does not know how: it changes
     * the settings and calls the function that it was given.
     */
    class SettingsScreen final : public Screen
    {
    public:
        /**
         * @brief What the page calls after a change: applies the settings and saves them.
         *
         * @return std::nullopt if it worked, or what went wrong (it is shown on the page).
         */
        using ApplyFunction = std::function<std::optional<std::string>()>;

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the settings page.
         *
         * @param t_settings The settings that the page changes. Not owned: they must outlive this page.
         * @param t_languages The languages that the user can choose from, see core::available_languages().
         * @param t_apply Called after every change.
         */
        SettingsScreen(core::Settings& t_settings, std::vector<core::Language> t_languages, ApplyFunction t_apply);

        SettingsScreen(const SettingsScreen& t_other) = delete;
        SettingsScreen(SettingsScreen&& t_other) noexcept = delete;
        SettingsScreen& operator=(const SettingsScreen& t_other) = delete;
        SettingsScreen& operator=(SettingsScreen&& t_other) noexcept = delete;

        ~SettingsScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        /**
         * @brief Returns the name shown in the navigation bar.
         *
         * @return The title, in the language that is in use.
         */
        [[nodiscard]] std::string_view Title() const noexcept override;

        /**
         * @brief Shows the settings on a page that fills the area to the right of the navigation bar.
         */
        void DrawGui() override;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Draws the list for the language.
         */
        void DrawLanguage();

        /**
         * @brief Applies and saves the settings after a change, and keeps the error if there is one.
         */
        void Changed();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        core::Settings& m_settings;               ///< What the page changes
        std::vector<core::Language> m_languages;  ///< The languages that there are, English first
        ApplyFunction m_apply;                    ///< Applies and saves the settings
        std::string m_error;                      ///< What went wrong when the settings were saved, or empty
    };
}
