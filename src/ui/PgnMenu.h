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

#include <array>
#include <string>
#include "core/GameState.h"
#include "core/Pgn.h"
#include "core/RulesProvider.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // PgnMenu
    //-------------------------------------------------

    /**
     * @brief The button "PGN" with its menu: copy the game to the clipboard, paste a game from it, save it to a
     * file and open a file.
     *
     * Games are saved and opened in the PGN format, so other programs (lichess, chess.com, Scid, ...) can read them.
     * The default folder for files is "games" in the data directory of the program.
     *
     * Opening a game replaces the current one. If the current game has moves, the user is asked first.
     * Whatever goes wrong while reading or writing is shown in a window with the reason.
     */
    class PgnMenu
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the menu.
         *
         * @param t_rules The source of the rules, needed to read a game. Not owned: it must outlive this menu.
         */
        explicit PgnMenu(core::RulesProvider& t_rules);

        PgnMenu(const PgnMenu& t_other) = delete;
        PgnMenu(PgnMenu&& t_other) noexcept = delete;
        PgnMenu& operator=(const PgnMenu& t_other) = delete;
        PgnMenu& operator=(PgnMenu&& t_other) noexcept = delete;

        ~PgnMenu() noexcept = default;

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Builds the button, the menu and the windows with Dear ImGui. Call it while a GUI frame is being built.
         *
         * The button is placed at the current position of the cursor.
         *
         * @param t_game The game. It is written, and it is replaced if a game is opened.
         * @param t_headers The tags of the game. They are written, and replaced if a game is opened.
         *
         * @return True in the frame in which another game was opened. The caller has to update what depends on the
         *         game: the position is the start position of the new game.
         */
        [[nodiscard]] bool Draw(core::GameState& t_game, core::PgnHeaders& t_headers);

        /**
         * @brief Returns the width of the button.
         *
         * @return The width in pixels.
         */
        [[nodiscard]] static float ButtonWidth();

    private:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief What the user chose in the menu.
         */
        enum class Action
        {
            NONE,
            COPY,
            PASTE,
            SAVE,
            OPEN
        };

        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Opens a game from PGN text, or asks first if the current game has moves.
         *
         * @param t_text The PGN text.
         * @param t_game The game.
         * @param t_headers The tags.
         *
         * @return True if the game was opened.
         */
        [[nodiscard]] bool Load(const std::string& t_text, core::GameState& t_game, core::PgnHeaders& t_headers);

        /**
         * @brief Replaces the game with the one in a PGN text. An error is shown in a window.
         *
         * @param t_text The PGN text.
         * @param t_game The game.
         * @param t_headers The tags.
         *
         * @return True if the game was opened.
         */
        [[nodiscard]] bool Replace(const std::string& t_text, core::GameState& t_game, core::PgnHeaders& t_headers);

        /**
         * @brief Writes the game to the file in the path field. An error is shown in a window.
         *
         * @param t_game The game.
         * @param t_headers The tags.
         *
         * @return True if the file was written.
         */
        [[nodiscard]] bool SaveFile(const core::GameState& t_game, const core::PgnHeaders& t_headers);

        /**
         * @brief Reads the file in the path field and opens the game. An error is shown in a window.
         *
         * @param t_game The game.
         * @param t_headers The tags.
         *
         * @return True if the game was opened.
         */
        [[nodiscard]] bool OpenFile(core::GameState& t_game, core::PgnHeaders& t_headers);

        /**
         * @brief Writes a text into the path field.
         *
         * @param t_path The text. A text that is too long is cut off.
         */
        void SetPath(const std::string& t_path);

        /**
         * @brief Remembers an error to show in a window.
         *
         * @param t_message What went wrong.
         */
        void ShowError(const std::string& t_message);

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        core::RulesProvider& m_rules;  ///< The source of the rules (not owned)

        std::array<char, 512> m_path{};  ///< The path in the windows for saving and opening
        std::string m_pending;           ///< PGN text that waits for the answer to "Replace the game?"
        std::string m_error;             ///< The text of the last error
        std::string m_notice;            ///< What the button says for a moment: "Copied", "Saved"
        double m_noticeUntil{ 0.0 };     ///< Until when (ImGui time) the button says the notice

        // A window must be opened from the place where it is built, not from inside another window that is closed
        // in the same frame. So the helpers only note that a window is wanted, and Draw() opens it.
        bool m_wantReplacePopup{ false };  ///< The question "Replace the game?" is to be opened
        bool m_wantErrorPopup{ false };    ///< The window with the error is to be opened
    };
}
