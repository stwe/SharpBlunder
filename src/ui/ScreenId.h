#pragma once

#include <cstdint>

namespace sg::chess::ui
{
    //-------------------------------------------------
    // ScreenId
    //-------------------------------------------------

    /**
     * @brief Identifies one of the pages of the application.
     */
    enum class ScreenId : std::uint8_t
    {
        ANALYSIS,  ///< Start page: a free board for analysis
        GAMES,     ///< Analysis of the user's online games
        TRAINING,  ///< Learning the opening repertoire
        SETTINGS   ///< Program settings
    };
}
