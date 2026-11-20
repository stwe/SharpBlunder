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
#include <optional>
#include <string>
#include <vector>
#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"
#include "core/RulesProvider.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // GameState
    //-------------------------------------------------

    /**
     * @brief A game as a tree of moves, with a current position that can be moved around.
     *
     * Node 0 is the start position, every other node is the position after one move. A node can have several
     * children: the first one is the main line, the others are variations (another move in the same position).
     * Playing a move that differs from the next move of the game does not throw the rest away: it adds a variation.
     *
     * "Ply" means a single move of one side: White's first move is ply 1, Black's reply is ply 2.
     *
     * The rules (legal moves, check) come from a RulesProvider. The game state itself only stores the results.
     *
     * The ids of the nodes stay valid until DeleteFrom() is called. After that, only CurrentId() is reliable.
     */
    class GameState
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief The id of a node: its index. The start position has the id 0.
         */
        using NodeId = std::size_t;

        /**
         * @brief What a person or a program has written about a move.
         *
         * The parts follow the PGN format: a comment in braces, commands inside a comment such as the clock
         * of lichess ("[%clk 0:03:00]"), and numeric annotation glyphs ("$1" for "!").
         */
        struct Annotation
        {
            std::string comment;    ///< The text, without the commands. It is shown and edited.
            std::string commands;   ///< The commands of the comment, e.g. "[%clk 0:03:00]". Kept as they are.
            std::vector<int> nags;  ///< The numeric annotation glyphs, e.g. 1 for "!" and 4 for "??"

            /**
             * @brief Tells whether there is nothing to show or to save.
             *
             * @return True if there is no comment, no command and no glyph.
             */
            [[nodiscard]] bool IsEmpty() const noexcept { return comment.empty() && commands.empty() && nags.empty(); }
        };

        /**
         * @brief One position of the game and the move that led to it.
         *
         * The annotation of the start position is the comment on the whole game.
         */
        struct Node
        {
            PositionInfo info;             ///< The position with its legal moves
            std::optional<Move> move;      ///< The move that led here (empty for the start position)
            std::string san;               ///< That move in SAN (empty for the start position)
            std::optional<NodeId> parent;  ///< The previous position (empty for the start position)
            std::vector<NodeId> children;  ///< The following positions: the first is the main line
            Annotation note;               ///< What is written about this move
            std::string lineComment;       ///< A comment on the whole variation that starts with this move
        };

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Starts a game from the standard start position.
         *
         * @param t_rules The source of the rules. It must outlive this object.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        explicit GameState(RulesProvider& t_rules);

        /**
         * @brief Starts a game from a given position.
         *
         * @param t_rules The source of the rules. It must outlive this object.
         * @param t_start The start position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        GameState(RulesProvider& t_rules, const Position& t_start);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Returns the current node.
         *
         * @return The position the board shows, with the move that led to it.
         */
        [[nodiscard]] const Node& Current() const noexcept { return m_nodes[m_current]; }

        /**
         * @brief Returns the id of the current node.
         *
         * @return The id.
         */
        [[nodiscard]] NodeId CurrentId() const noexcept { return m_current; }

        /**
         * @brief Returns a node.
         *
         * @param t_id The id. It must exist.
         *
         * @return The node.
         */
        [[nodiscard]] const Node& At(const NodeId t_id) const noexcept { return m_nodes[t_id]; }

        /**
         * @brief Returns the number of nodes: the start position and all moves of the main line and the variations.
         *
         * @return The number of nodes.
         */
        [[nodiscard]] std::size_t NodeCount() const noexcept { return m_nodes.size(); }

        /**
         * @brief Tells whether any move has been played.
         *
         * @return True if there is at least one move besides the start position, in the main line or a variation.
         */
        [[nodiscard]] bool HasMoves() const noexcept { return m_nodes.size() > 1; }

        /**
         * @brief Tells whether a node is part of the main line: it and all nodes before it are the first child.
         *
         * @param t_id The id.
         *
         * @return True for the start position and the nodes of the main line.
         */
        [[nodiscard]] bool IsMainLine(NodeId t_id) const noexcept;

        /**
         * @brief Tells whether there is a position before the current one.
         *
         * @return True if the current position is not the start position.
         */
        [[nodiscard]] bool CanGoBack() const noexcept { return m_nodes[m_current].parent.has_value(); }

        /**
         * @brief Tells whether there is a position after the current one.
         *
         * @return True if a move follows the current position.
         */
        [[nodiscard]] bool CanGoForward() const noexcept { return !m_nodes[m_current].children.empty(); }

        //-------------------------------------------------
        // Navigation
        //-------------------------------------------------

        /**
         * @brief Steps back to the previous position. Does nothing at the start position.
         */
        void GoBack() noexcept;

        /**
         * @brief Steps forward to the next position of the main line. Does nothing at the end of a line.
         */
        void GoForward() noexcept;

        /**
         * @brief Jumps to the start position.
         */
        void GoToStart() noexcept;

        /**
         * @brief Jumps to the end of the line the current position is in. Where the line branches, the main line is
         * followed.
         */
        void GoToEnd() noexcept;

        /**
         * @brief Jumps to a position of the game.
         *
         * @param t_id The id of the node. An id that does not exist is ignored.
         */
        void GoTo(NodeId t_id) noexcept;

        //-------------------------------------------------
        // Changes
        //-------------------------------------------------

        /**
         * @brief Plays a move in the current position.
         *
         * If the game already has this move here (as the next move or as a variation), the game just goes there.
         * Otherwise the move is added as a new variation, or as the main line if the position has no move yet.
         * Nothing is thrown away.
         *
         * @param t_move The move.
         *
         * @return True if the move was played, false if it is not legal in the current position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        bool Play(const Move& t_move);

        /**
         * @brief Replaces the annotation of a node.
         *
         * @param t_id The id of the node. An id that does not exist is ignored.
         * @param t_note The new annotation.
         */
        void SetAnnotation(NodeId t_id, Annotation t_note);

        /**
         * @brief Replaces the comment on the whole line that starts with a node.
         *
         * Only the first move of a variation shows it. The main line has no line comment of its own: the comment
         * on the start position is the comment on the game.
         *
         * @param t_id The id of the node. An id that does not exist is ignored.
         * @param t_comment The new comment. An empty text removes it.
         */
        void SetLineComment(NodeId t_id, std::string t_comment);

        /**
         * @brief Makes the line of a node the main line.
         *
         * The node becomes the first child of its parent, and so does that parent in its own parent, up to the
         * start position. The current position does not change.
         *
         * A line that is not a variation any more has no line comment. The comment is not lost: it is put in
         * front of the comment on the first move of the line.
         *
         * @param t_id The id of the node. The start position and ids that do not exist are ignored.
         */
        void Promote(NodeId t_id);

        /**
         * @brief Deletes a node and everything after it.
         *
         * If the current position is among the deleted nodes, the game goes to the position before the node.
         * The ids of the other nodes change.
         *
         * @param t_id The id of the node. The start position and ids that do not exist are ignored.
         */
        void DeleteFrom(NodeId t_id);

        /**
         * @brief Removes all moves and the annotations and returns to the start position.
         */
        void Reset() noexcept;

        /**
         * @brief Takes over the game of another GameState, which is left empty and must not be used any more.
         *
         * The rules of this object stay. It is used to load a game: build it in a GameState of its own and move it
         * here when it is complete, so a game that fails to load does not destroy the current one.
         *
         * @param t_other The other game.
         */
        void ReplaceWith(GameState&& t_other) noexcept;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        RulesProvider& m_rules;     ///< The source of the rules (not owned)
        std::vector<Node> m_nodes;  ///< Index 0 = start position
        NodeId m_current{ 0 };      ///< Id of the current node
    };
}
