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

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include "ui/Screen.h"
#include "ui/ScreenId.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // ScreenManager
    //-------------------------------------------------

    /**
     * @brief Owns all screens and knows which one is active.
     *
     * The frame steps (HandleEvent, Update, Draw, DrawGui) are forwarded to the active screen,
     * so the rest of the application does not need to know which page is shown.
     */
    class ScreenManager
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief A registered screen together with its id.
         */
        struct Entry
        {
            ScreenId id;
            std::unique_ptr<Screen> screen;
        };

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        ScreenManager() = default;

        ScreenManager(const ScreenManager& t_other) = delete;
        ScreenManager(ScreenManager&& t_other) noexcept = delete;
        ScreenManager& operator=(const ScreenManager& t_other) = delete;
        ScreenManager& operator=(ScreenManager&& t_other) noexcept = delete;

        ~ScreenManager() noexcept = default;

        //-------------------------------------------------
        // Registration and switching
        //-------------------------------------------------

        /**
         * @brief Registers a screen. The manager takes ownership.
         *
         * The screens appear in the navigation bar in the order they are added.
         *
         * @param t_id The id of the screen.
         * @param t_screen The screen.
         *
         * @throws std::invalid_argument If the screen is null or the id is already registered.
         */
        void Add(ScreenId t_id, std::unique_ptr<Screen> t_screen);

        /**
         * @brief Makes a screen the active one.
         *
         * Calls Screen::OnExit() of the previous and Screen::OnEnter() of the new screen.
         * Does nothing if the screen is already active.
         *
         * @param t_id The id of the screen.
         *
         * @throws std::invalid_argument If no screen with this id is registered.
         */
        void SwitchTo(ScreenId t_id);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Returns the id of the active screen.
         *
         * @return The id, or std::nullopt if SwitchTo() has not been called yet.
         */
        [[nodiscard]] std::optional<ScreenId> CurrentId() const noexcept;

        /**
         * @brief Returns all registered screens in the order they were added.
         *
         * @return A read-only view of the entries.
         */
        [[nodiscard]] std::span<const Entry> Entries() const noexcept;

        //-------------------------------------------------
        // Frame steps (forwarded to the active screen)
        //-------------------------------------------------

        void HandleEvent(const sf::Event& t_event);
        void Update(sf::Time t_dt);
        void Draw(sf::RenderTarget& t_target);
        void DrawGui();

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Finds the index of a screen in m_entries.
         *
         * @param t_id The id of the screen.
         *
         * @return The index.
         *
         * @throws std::invalid_argument If no screen with this id is registered.
         */
        [[nodiscard]] std::size_t IndexOf(ScreenId t_id) const;

        /**
         * @brief Returns the active screen or nullptr if there is none.
         *
         * @return The active screen.
         */
        [[nodiscard]] Screen* Active() const noexcept;

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::vector<Entry> m_entries;          ///< All screens
        std::optional<std::size_t> m_current;  ///< Index of the active screen in m_entries
    };
}
