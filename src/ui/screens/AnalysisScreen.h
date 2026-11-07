#pragma once

#include <string_view>
#include "core/Position.h"
#include "ui/BoardView.h"
#include "ui/PieceSet.h"
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // AnalysisScreen
    //-------------------------------------------------

    /**
     * @brief The start page: a free board for analysis. There is nothing to learn here.
     */
    class AnalysisScreen final : public Screen
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the page with the board in the start position.
         *
         * @param t_pieces The piece images. Not owned: they must outlive this screen.
         */
        explicit AnalysisScreen(const PieceSet& t_pieces);

        AnalysisScreen(const AnalysisScreen& t_other) = delete;
        AnalysisScreen(AnalysisScreen&& t_other) noexcept = delete;
        AnalysisScreen& operator=(const AnalysisScreen& t_other) = delete;
        AnalysisScreen& operator=(AnalysisScreen&& t_other) noexcept = delete;

        ~AnalysisScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        [[nodiscard]] std::string_view Title() const noexcept override;

        void Draw(sf::RenderTarget& t_target) override;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        core::Position m_position;  ///< The position that is shown
        BoardView m_boardView;      ///< Draws m_position
    };
}
