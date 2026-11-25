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

#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Catalog
    //-------------------------------------------------

    /**
     * @brief The translations of one language: for every English text, the text in that language.
     *
     * A catalog is read from a file in the .po format of GNU gettext, so the usual tools work on it (xgettext,
     * msgmerge, Poedit). Entries that are marked "fuzzy" or have no translation are left out, and so are entries with
     * plural forms, which the program does not use. The English text is the key. A text can have a context to tell
     * apart equal English texts that need different translations.
     */
    class Catalog
    {
    public:
        /**
         * @brief Reads a catalog from the text of a .po file.
         *
         * The text is read as it comes: lines that cannot be understood are skipped, and a file that is not a .po file
         * gives an empty catalog. This function does not throw.
         *
         * @param t_text The text, in UTF-8.
         *
         * @return The catalog.
         */
        [[nodiscard]] static Catalog FromPo(std::string_view t_text);

        /**
         * @brief Looks up the translation of a text.
         *
         * @param t_text The English text.
         * @param t_context The context of the text, or empty if it has none.
         *
         * @return The translation, or nullptr if there is none.
         */
        [[nodiscard]] const std::string* Find(std::string_view t_text, std::string_view t_context = {}) const;

        /**
         * @brief Returns the number of translations.
         *
         * @return The number of entries.
         */
        [[nodiscard]] std::size_t Size() const noexcept { return m_entries.size(); }

        /**
         * @brief Returns the name of the language in that language, e.g. "Deutsch".
         *
         * It comes from the header "X-Native-Name" of the file.
         *
         * @return The name, or an empty text if the file has none.
         */
        [[nodiscard]] const std::string& NativeName() const noexcept { return m_nativeName; }

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        /**
         * @brief Hash for texts, which lets std::string_view find entries without making a std::string.
         */
        struct Hash
        {
            using is_transparent = void;

            [[nodiscard]] std::size_t operator()(const std::string_view t_text) const noexcept
            {
                return std::hash<std::string_view>{}(t_text);
            }
        };

        std::unordered_map<std::string, std::string, Hash, std::equal_to<>> m_entries;  ///< "context\x04text" or "text"
        std::string m_nativeName;                                                         ///< The name of the language
    };

    //-------------------------------------------------
    // Languages
    //-------------------------------------------------

    /**
     * @brief A language that the program can show.
     */
    struct Language
    {
        std::string code;        ///< The code, e.g. "en" or "de"
        std::string nativeName;  ///< The name in that language, e.g. "Deutsch"

        [[nodiscard]] bool operator==(const Language&) const = default;
    };

    /**
     * @brief The code of the source language of the texts in the program.
     */
    inline constexpr const char* SOURCE_LANGUAGE{ "en" };

    /**
     * @brief Finds the languages: English, which needs no file, and one for every file "<code>.po" in a folder.
     *
     * @param t_directory The folder with the .po files. It may not exist.
     *
     * @return The languages, English first and the others sorted by their code.
     */
    [[nodiscard]] std::vector<Language> available_languages(const std::filesystem::path& t_directory);

    /**
     * @brief Takes the language out of a locale name: "de_DE.UTF-8" gives "de_DE", "de" gives "de".
     *
     * @param t_locale The name, e.g. from the environment variable LANG.
     *
     * @return The code, or an empty text for "C", "POSIX" and anything that is not a locale name.
     */
    [[nodiscard]] std::string language_from_locale(std::string_view t_locale);

    /**
     * @brief Finds the language of the system from the environment variables LC_ALL, LC_MESSAGES, LANGUAGE and LANG.
     *
     * @return The code, e.g. "de_DE", or an empty text if the system gives none.
     */
    [[nodiscard]] std::string system_language();

    /**
     * @brief Decides which language to show.
     *
     * "auto" means the language of the system. If the exact code is not available, the language without the region
     * is tried ("pt_BR" becomes "pt"). If nothing fits, the result is English.
     *
     * @param t_setting The language from the settings: "auto" or a code.
     * @param t_system The language of the system, see system_language().
     * @param t_available The languages that there are.
     *
     * @return The code of a language that is in the list, or "en".
     */
    [[nodiscard]] std::string resolve_language(std::string_view t_setting, std::string_view t_system,
                                               const std::vector<Language>& t_available);

    //-------------------------------------------------
    // Translation
    //-------------------------------------------------

    /**
     * @brief Chooses the language of all texts that are translated from now on.
     *
     * The catalog is read from "<code>.po" in the folder. Catalogs that have been read stay in memory, so texts that
     * tr() gave out earlier stay valid when the language is changed. Use it from the thread of the user interface.
     *
     * @param t_code The code of the language. English needs no file.
     * @param t_directory The folder with the .po files.
     *
     * @return std::nullopt if it worked, or what went wrong, in English. Then the texts stay English.
     */
    [[nodiscard]] std::optional<std::string> set_language(const std::string& t_code,
                                                          const std::filesystem::path& t_directory);

    /**
     * @brief Returns the code of the language that is in use.
     *
     * @return The code, "en" until set_language() has worked.
     */
    [[nodiscard]] const std::string& current_language();

    /**
     * @brief Translates a text.
     *
     * Write the English text and nothing else: tr("Save"). The tool tools/update_translations.sh finds the texts in
     * the source code by this name.
     *
     * @param t_text The English text.
     *
     * @return The translation, or the English text if there is none. The pointer is valid for the whole run of the
     * program if the argument is, as with a string literal.
     */
    [[nodiscard]] const char* tr(const char* t_text);

    /**
     * @brief Translates a text that has a context, for equal English texts that need different translations.
     *
     * @param t_context The context, e.g. "noun". It is not shown.
     * @param t_text The English text.
     *
     * @return The translation, or the English text if there is none.
     */
    [[nodiscard]] const char* tr_c(const char* t_context, const char* t_text);

    /**
     * @brief Translates a text with places for values, and puts the values in.
     *
     * The places are written as in std::format: "{} mates in {}", or with numbers "{1} mates in {0}" if the order is
     * different in another language. If a translation is broken (wrong places), the English text is used.
     *
     * @tparam Args The types of the values.
     * @param t_text The English text with the places.
     * @param t_args The values.
     *
     * @return The text.
     */
    template <typename... Args>
    [[nodiscard]] std::string tr_format(const char* t_text, Args&&... t_args)
    {
        try
        {
            return std::vformat(tr(t_text), std::make_format_args(t_args...));
        }
        catch (const std::format_error&)
        {
            return std::vformat(t_text, std::make_format_args(t_args...));
        }
    }
} // namespace sg::chess::core
