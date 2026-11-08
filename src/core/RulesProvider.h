#pragma once

#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // RulesProvider
    //-------------------------------------------------

    /**
     * @brief The source of truth for the rules of chess.
     *
     * The program does not know the rules itself. It asks a RulesProvider which moves are legal,
     * what a position looks like after a move and whether the king is in check.
     * Today the answers come from Stockfish (engine::StockfishRules). Because everything else
     * only knows this interface, a move generator of our own could replace it later.
     */
    class RulesProvider
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        RulesProvider() = default;

        RulesProvider(const RulesProvider& t_other) = delete;
        RulesProvider(RulesProvider&& t_other) noexcept = delete;
        RulesProvider& operator=(const RulesProvider& t_other) = delete;
        RulesProvider& operator=(RulesProvider&& t_other) noexcept = delete;

        /**
         * @brief Virtual, because implementations are deleted through a pointer to this base class.
         */
        virtual ~RulesProvider() noexcept = default;

        //-------------------------------------------------
        // Queries
        //-------------------------------------------------

        /**
         * @brief Describes a position: its legal moves and whether the king is in check.
         *
         * @param t_position The position.
         *
         * @return The position with its legal moves and check state.
         *
         * @throws std::runtime_error If the answer cannot be determined.
         */
        [[nodiscard]] virtual PositionInfo Describe(const Position& t_position) = 0;

        /**
         * @brief Plays a move and describes the resulting position.
         *
         * The move must be legal in the given position.
         *
         * @param t_position The position before the move.
         * @param t_move The move.
         *
         * @return The position after the move with its legal moves and check state.
         *
         * @throws std::runtime_error If the answer cannot be determined.
         */
        [[nodiscard]] virtual PositionInfo Play(const Position& t_position, const Move& t_move) = 0;
    };
}
