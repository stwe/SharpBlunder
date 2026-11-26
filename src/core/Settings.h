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

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Settings
    //-------------------------------------------------

    /**
     * @brief The settings of the program, which the user changes on the settings page.
     *
     * They are kept in the file settings.json in the data directory (see load_settings() and save_settings()). Every
     * member has a default, so a missing file or a missing entry is not a problem. What a later version of the program
     * has added to the file is kept when this version saves it again.
     */
    struct Settings
    {
        /**
         * @brief The settings of the analysis page.
         */
        struct Analysis
        {
            bool engineOnAtStart{ true };  ///< Whether the engine is running when the analysis page is first shown

            /**
             * @brief Compares the settings of the analysis page.
             *
             * @return True if everything is the same.
             */
            [[nodiscard]] bool operator==(const Analysis&) const = default;
        };

        /**
         * @brief The accounts of the user on the websites that the games come from.
         */
        struct Accounts
        {
            /**
             * @brief The account on chess.com.
             */
            struct ChessCom
            {
                std::string username; ///< The name on chess.com. Empty if the user has not set one

                /**
                 * @brief Compares the account.
                 *
                 * @return True if everything is the same.
                 */
                [[nodiscard]] bool operator==(const ChessCom&) const = default;
            };

            ChessCom chesscom; ///< chess.com

            /**
             * @brief Compares the accounts.
             *
             * @return True if everything is the same.
             */
            [[nodiscard]] bool operator==(const Accounts&) const = default;
        };

        /**
         * @brief The longest user name that is kept. Names on chess.com are shorter. A longer text is a mistake.
         */
        static constexpr std::size_t MAX_USERNAME_LENGTH{ 50 };

        /**
         * @brief The value of language that means: use the language of the system.
         */
        static constexpr const char* AUTO_LANGUAGE{ "auto" };

        std::string language{ AUTO_LANGUAGE };  ///< "auto" or the code of a language, e.g. "en" or "de"
        Analysis analysis;                      ///< The analysis page
        Accounts accounts;                      ///< The accounts on websites

        std::string extra;  ///< Entries of the file that this version does not know, as JSON text. Kept, not used

        /**
         * @brief Compares all settings, the unknown entries included.
         *
         * @return True if everything is the same.
         */
        [[nodiscard]] bool operator==(const Settings&) const = default;
    };

    /**
     * @brief The result of load_settings().
     */
    struct LoadedSettings
    {
        Settings settings;    ///< The settings: what was read, and the defaults for what was not in the file
        std::string problem;  ///< What went wrong, in English. Empty if the file was fine or did not exist
    };

    //-------------------------------------------------
    // File
    //-------------------------------------------------

    /**
     * @brief Returns the path of the settings file: settings.json in the data directory.
     *
     * @return The path. The file may not exist.
     */
    [[nodiscard]] std::filesystem::path settings_path();

    /**
     * @brief Reads the settings from a file.
     *
     * A file that does not exist is not an error: the defaults are used. Entries with a wrong type or a value that
     * makes no sense are replaced by their defaults. A file that is not JSON at all is renamed to "<name>.bak" (an
     * older backup is replaced), so that the user can look at it, and the defaults are used. This function does not
     * throw.
     *
     * @param t_path The path of the file.
     *
     * @return The settings, and the problem if there was one.
     */
    [[nodiscard]] LoadedSettings load_settings(const std::filesystem::path& t_path);

    /**
     * @brief Writes the settings to a file.
     *
     * The text is written to "<name>.tmp" first and then renamed, so a crash cannot leave a half-written file. The
     * directory is made if it does not exist. This function does not throw.
     *
     * @param t_path The path of the file.
     * @param t_settings The settings.
     *
     * @return std::nullopt if it worked, or what went wrong, in English.
     */
    [[nodiscard]] std::optional<std::string> save_settings(const std::filesystem::path& t_path,
                                                           const Settings& t_settings);

    //-------------------------------------------------
    // Text
    //-------------------------------------------------

    /**
     * @brief Parses the text of a settings file.
     *
     * @param t_text The JSON text.
     *
     * @return The settings, or std::nullopt if the text is not a JSON object.
     */
    [[nodiscard]] std::optional<Settings> settings_from_json(const std::string& t_text);

    /**
     * @brief Writes settings as the text of a settings file: JSON with indentation and a line feed at the end.
     *
     * @param t_settings The settings.
     *
     * @return The text.
     */
    [[nodiscard]] std::string settings_to_json(const Settings& t_settings);
} // namespace sg::chess::core
