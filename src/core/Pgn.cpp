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

#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>
#include <utility>
#include "core/Pgn.h"
#include "core/San.h"

namespace
{
    using Annotation = sg::chess::core::GameState::Annotation;
    using sg::chess::core::GameState;

    constexpr std::size_t LINE_WIDTH{ 80 };
    constexpr std::string_view WHITESPACE{ " \t\r\n" };
    constexpr std::string_view NO_DATE{ "????.??.??" };
    constexpr std::string_view BOM{ "\xEF\xBB\xBF" };

    // The seven tags that every game has, in this order, and the value that stands for "not known".
    constexpr std::array<std::pair<std::string_view, std::string_view>, 7> ROSTER{
        { { "Event", "?" }, { "Site", "?" }, { "Date", "????.??.??" }, { "Round", "?" },
          { "White", "?" }, { "Black", "?" }, { "Result", "*" } }
    };

    //-------------------------------------------------
    // Text
    //-------------------------------------------------

    std::string Trim(const std::string_view t_text)
    {
        const std::size_t first{ t_text.find_first_not_of(WHITESPACE) };
        if (first == std::string_view::npos)
        {
            return "";
        }

        const std::size_t last{ t_text.find_last_not_of(WHITESPACE) };

        return std::string{ t_text.substr(first, last - first + 1) };
    }

    bool IsResult(const std::string_view t_word)
    {
        return t_word == "1-0" || t_word == "0-1" || t_word == "1/2-1/2" || t_word == "*";
    }

    //-------------------------------------------------
    // Comments
    //-------------------------------------------------

    // A comment as it is in the file, split into the text and the commands such as [%clk 0:03:00].
    struct SplitComment
    {
        std::string text;
        std::string commands;
    };

    SplitComment SplitRawComment(const std::string_view t_raw)
    {
        SplitComment result;
        std::string text;

        std::size_t pos{ 0 };
        while (pos < t_raw.size())
        {
            const std::size_t start{ t_raw.find("[%", pos) };
            const std::size_t end{ start == std::string_view::npos ? std::string_view::npos : t_raw.find(']', start) };

            if (end == std::string_view::npos)
            {
                text += t_raw.substr(pos);
                break;
            }

            text += t_raw.substr(pos, start - pos);
            text += ' ';

            if (!result.commands.empty())
            {
                result.commands += ' ';
            }

            result.commands += t_raw.substr(start, end - start + 1);
            pos = end + 1;
        }

        // The places of the commands leave double spaces behind. Line breaks in a comment stay.
        std::string collapsed;
        for (const char c : text)
        {
            if (c == ' ' && !collapsed.empty() && collapsed.back() == ' ')
            {
                continue;
            }

            collapsed += c;
        }

        result.text = Trim(collapsed);

        return result;
    }

    // What is written between the braces for an annotation: the text first, then the commands.
    std::string CommentContent(const std::string& t_text, const std::string& t_commands)
    {
        std::string content{ t_text };

        // PGN has no way to write a closing brace inside a comment.
        std::ranges::replace(content, '}', ')');

        if (!t_commands.empty())
        {
            content += content.empty() ? "" : " ";
            content += t_commands;
        }

        return content;
    }

    //-------------------------------------------------
    // Writing
    //-------------------------------------------------

    std::string BraceComment(const std::string& t_text, const std::string& t_commands)
    {
        return "{ " + CommentContent(t_text, t_commands) + " }";
    }

    // Writes the moves of a line, with their variations. The line starts with t_first.
    void EmitLine(const GameState& t_game, const GameState::NodeId t_first, const bool t_nested,
                  std::vector<std::string>& t_tokens)
    {
        if (t_nested)
        {
            t_tokens.emplace_back("(");

            const std::string& lineComment{ t_game.At(t_first).lineComment };
            if (!lineComment.empty())
            {
                t_tokens.push_back(BraceComment(lineComment, ""));
            }
        }

        std::optional<GameState::NodeId> id{ t_first };

        // Black's move gets a number at the start of a line and after a comment or a variation.
        bool forceNumber{ true };

        while (id)
        {
            const GameState::Node& node{ t_game.At(*id) };
            const GameState::Node& parent{ t_game.At(*node.parent) };
            const bool whiteMoves{ parent.info.position.SideToMove() == sg::chess::core::Color::WHITE };

            if (whiteMoves || forceNumber)
            {
                t_tokens.push_back(std::to_string(parent.info.position.FullmoveNumber()) + (whiteMoves ? "." : "..."));
            }

            forceNumber = false;
            t_tokens.push_back(node.san);

            for (const int nag : node.note.nags)
            {
                t_tokens.push_back("$" + std::to_string(nag));
            }

            if (!node.note.comment.empty() || !node.note.commands.empty())
            {
                t_tokens.push_back(BraceComment(node.note.comment, node.note.commands));
                forceNumber = true;
            }

            // The other moves of the position are alternatives to this one, if this one is the main move.
            if (parent.children.size() > 1 && parent.children.front() == *id)
            {
                for (std::size_t i{ 1 }; i < parent.children.size(); ++i)
                {
                    EmitLine(t_game, parent.children[i], true, t_tokens);
                }

                forceNumber = true;
            }

            id = node.children.empty() ? std::nullopt : std::optional{ node.children.front() };
        }

        if (t_nested)
        {
            t_tokens.emplace_back(")");
        }
    }

    // Puts the tokens in lines of at most LINE_WIDTH characters. Nothing is split after "(" or before ")".
    std::string Layout(const std::vector<std::string>& t_tokens)
    {
        std::string text;
        std::string line;
        bool afterOpen{ false };

        for (const std::string& token : t_tokens)
        {
            const bool isClose{ token == ")" };
            const bool glued{ line.empty() || afterOpen || isClose };

            if (!glued && line.size() + 1 + token.size() > LINE_WIDTH)
            {
                text += line + '\n';
                line.clear();
            }
            else if (!line.empty() && !glued)
            {
                line += ' ';
            }

            line += token;
            afterOpen = token == "(";
        }

        return text + line + '\n';
    }

    std::string EscapeTagValue(const std::string& t_value)
    {
        std::string escaped;

        for (const char c : t_value)
        {
            if (c == '"' || c == '\\')
            {
                escaped += '\\';
            }

            escaped += c;
        }

        return escaped;
    }

    //-------------------------------------------------
    // Reading
    //-------------------------------------------------

    // The glyphs that may follow a move, and their numbers.
    int NagOfGlyph(const std::string_view t_glyph)
    {
        if (t_glyph == "!")
        {
            return 1;
        }

        if (t_glyph == "?")
        {
            return 2;
        }

        if (t_glyph == "!!")
        {
            return 3;
        }

        if (t_glyph == "??")
        {
            return 4;
        }

        if (t_glyph == "!?")
        {
            return 5;
        }

        return t_glyph == "?!" ? 6 : 0;
    }

    [[noreturn]] void Fail(const std::string_view t_text, const std::size_t t_pos, const std::string& t_message)
    {
        const auto line{ std::count(t_text.begin(), t_text.begin() + static_cast<std::ptrdiff_t>(t_pos), '\n') + 1 };

        throw std::runtime_error("PGN, line " + std::to_string(line) + ": " + t_message);
    }

    // Reads the tags at the start of the text. Returns the position where the moves start.
    std::size_t ReadTags(const std::string_view t_text, sg::chess::core::PgnHeaders& t_headers, std::string& t_fen)
    {
        std::size_t pos{ 0 };

        // Some programs write a byte order mark at the start of a file.
        if (t_text.starts_with(BOM))
        {
            pos = BOM.size();
        }

        while (true)
        {
            pos = t_text.find_first_not_of(WHITESPACE, pos);
            if (pos == std::string_view::npos || t_text[pos] != '[')
            {
                return pos == std::string_view::npos ? t_text.size() : pos;
            }

            const std::size_t tagStart{ pos };
            ++pos;

            const std::size_t nameEnd{ t_text.find_first_of(WHITESPACE, pos) };
            if (nameEnd == std::string_view::npos)
            {
                Fail(t_text, tagStart, "a tag is not finished");
            }

            const std::string name{ t_text.substr(pos, nameEnd - pos) };
            pos = t_text.find_first_not_of(WHITESPACE, nameEnd);

            if (pos == std::string_view::npos || t_text[pos] != '"')
            {
                Fail(t_text, tagStart, "the value of the tag " + name + " must be in double quotes");
            }

            std::string value;
            ++pos;
            while (pos < t_text.size() && t_text[pos] != '"')
            {
                if (t_text[pos] == '\\' && pos + 1 < t_text.size())
                {
                    ++pos;
                }

                value += t_text[pos];
                ++pos;
            }

            if (pos >= t_text.size())
            {
                Fail(t_text, tagStart, "the value of the tag " + name + " is not finished");
            }

            pos = t_text.find(']', pos);
            if (pos == std::string_view::npos)
            {
                Fail(t_text, tagStart, "the tag " + name + " has no closing bracket");
            }

            ++pos;

            if (name == "FEN")
            {
                t_fen = value;
            }
            else if (name != "SetUp")
            {
                t_headers.tags.push_back({ name, value });
            }
        }
    }

    // The state of reading the moves.
    class MoveReader
    {
    public:
        MoveReader(const std::string_view t_text, GameState& t_game, sg::chess::core::PgnHeaders& t_headers)
            : m_text(t_text)
            , m_game(t_game)
            , m_headers(t_headers)
        {
        }

        void Read(const std::size_t t_start)
        {
            m_pos = t_start;

            while (true)
            {
                m_pos = m_text.find_first_not_of(WHITESPACE, m_pos);
                if (m_pos == std::string_view::npos)
                {
                    break;
                }

                const std::size_t start{ m_pos };
                const char c{ m_text[m_pos] };

                if (c == '{')
                {
                    const std::size_t end{ m_text.find('}', m_pos) };
                    if (end == std::string_view::npos)
                    {
                        Fail(m_text, start, "a comment is not finished (the closing brace is missing)");
                    }

                    Comment(m_text.substr(m_pos + 1, end - m_pos - 1));
                    m_pos = end + 1;
                }
                else if (c == ';')
                {
                    const std::size_t end{ m_text.find('\n', m_pos) };
                    const std::size_t stop{ end == std::string_view::npos ? m_text.size() : end };

                    Comment(m_text.substr(m_pos + 1, stop - m_pos - 1));
                    m_pos = stop;
                }
                else if (c == '%' && IsLineStart(start))
                {
                    // A line that starts with a percent sign is meant for programs and is skipped.
                    const std::size_t end{ m_text.find('\n', m_pos) };
                    m_pos = end == std::string_view::npos ? m_text.size() : end;
                }
                else if (c == '(')
                {
                    Open(start);
                    ++m_pos;
                }
                else if (c == ')')
                {
                    Close(start);
                    ++m_pos;
                }
                else if (c == '$')
                {
                    Glyph(start);
                }
                else if (Word(start))
                {
                    return;
                }
            }

            if (!m_resume.empty())
            {
                Fail(m_text, m_text.size(), "a variation is not closed (a closing parenthesis is missing)");
            }
        }

    private:
        // True at the start of a line, also behind a byte order mark.
        [[nodiscard]] bool IsLineStart(const std::size_t t_pos) const
        {
            return t_pos == 0 || m_text[t_pos - 1] == '\n' || (t_pos == BOM.size() && m_text.starts_with(BOM));
        }

        // A comment belongs to the move before it. Right after an opening parenthesis it belongs to the variation.
        void Comment(const std::string_view t_raw)
        {
            const SplitComment split{ SplitRawComment(t_raw) };

            if (!m_resume.empty() && !m_lineHasMove)
            {
                m_pendingLine += (m_pendingLine.empty() || split.text.empty() ? "" : "\n") + split.text;
                m_pendingCommands += (m_pendingCommands.empty() || split.commands.empty() ? "" : " ") + split.commands;

                return;
            }

            Annotation note{ m_game.Current().note };
            note.comment += (note.comment.empty() || split.text.empty() ? "" : "\n") + split.text;
            note.commands += (note.commands.empty() || split.commands.empty() ? "" : " ") + split.commands;
            m_game.SetAnnotation(m_game.CurrentId(), std::move(note));
        }

        void Open(const std::size_t t_at)
        {
            if (!m_lineHasMove)
            {
                Fail(m_text, t_at, "a variation needs a move before it");
            }

            // The variation is an alternative to the last move: go to the position before it.
            m_resume.push_back(m_game.CurrentId());
            m_game.GoBack();
            m_lineHasMove = false;
            m_pendingLine.clear();
            m_pendingCommands.clear();
        }

        void Close(const std::size_t t_at)
        {
            if (m_resume.empty())
            {
                Fail(m_text, t_at, "a closing parenthesis without an opening one");
            }

            m_game.GoTo(m_resume.back());
            m_resume.pop_back();
            m_lineHasMove = true;
        }

        void Glyph(const std::size_t t_at)
        {
            std::size_t end{ m_pos + 1 };
            while (end < m_text.size() && m_text[end] >= '0' && m_text[end] <= '9')
            {
                ++end;
            }

            int nag{ 0 };
            const auto digits{ m_text.substr(m_pos + 1, end - m_pos - 1) };
            if (digits.empty() || std::from_chars(digits.data(), digits.data() + digits.size(), nag).ec != std::errc{})
            {
                Fail(m_text, t_at, "a $ must be followed by the number of a glyph");
            }

            m_pos = end;
            AddGlyph(nag);
        }

        void AddGlyph(const int t_nag)
        {
            Annotation note{ m_game.Current().note };
            note.nags.push_back(t_nag);
            m_game.SetAnnotation(m_game.CurrentId(), std::move(note));
        }

        // Reads a word: a move number, a move, or the result. Returns true at the result, where the game ends.
        bool Word(const std::size_t t_at)
        {
            const std::size_t end{ m_text.find_first_of(" \t\r\n{}();$", m_pos) };
            const std::size_t stop{ end == std::string_view::npos ? m_text.size() : end };
            std::string_view word{ m_text.substr(m_pos, stop - m_pos) };
            m_pos = stop;

            if (IsResult(word))
            {
                if (!m_resume.empty())
                {
                    Fail(m_text, t_at, "the game ends, but a variation is still open");
                }

                if (m_headers.Get("Result").empty())
                {
                    m_headers.Set("Result", word);
                }

                return true;
            }

            // A move number such as "12." or "12..." in front of the move, or alone.
            if (word.front() >= '0' && word.front() <= '9')
            {
                const std::size_t digits{ word.find_first_not_of("0123456789") };
                if (digits != std::string_view::npos && word[digits] == '.')
                {
                    word.remove_prefix(word.find_first_not_of('.', digits) == std::string_view::npos
                                           ? word.size()
                                           : word.find_first_not_of('.', digits));
                }
            }

            if (!word.empty())
            {
                Move(t_at, word);
            }

            return false;
        }

        void Move(const std::size_t t_at, std::string_view t_san)
        {
            // The glyphs for good and bad moves at the end of the move.
            std::size_t glyphStart{ t_san.size() };
            while (glyphStart > 0 && (t_san[glyphStart - 1] == '!' || t_san[glyphStart - 1] == '?'))
            {
                --glyphStart;
            }

            const std::string_view glyph{ t_san.substr(glyphStart) };
            const std::string_view san{ t_san.substr(0, glyphStart) };

            const auto move{ sg::chess::core::from_san(m_game.Current().info, san) };
            if (!move)
            {
                Fail(m_text, t_at, "the move \"" + std::string{ san } + "\" is not legal here or not clear");
            }

            static_cast<void>(m_game.Play(*move));

            if (!m_lineHasMove && !m_resume.empty() && (!m_pendingLine.empty() || !m_pendingCommands.empty()))
            {
                m_game.SetLineComment(m_game.CurrentId(), m_pendingLine);

                Annotation note{ m_game.Current().note };
                note.commands += (note.commands.empty() || m_pendingCommands.empty() ? "" : " ") + m_pendingCommands;
                m_game.SetAnnotation(m_game.CurrentId(), std::move(note));
            }

            m_pendingLine.clear();
            m_pendingCommands.clear();
            m_lineHasMove = true;

            if (const int nag{ NagOfGlyph(glyph) }; nag != 0)
            {
                AddGlyph(nag);
            }
        }

        std::string_view m_text;
        GameState& m_game;
        sg::chess::core::PgnHeaders& m_headers;
        std::size_t m_pos{ 0 };

        std::vector<GameState::NodeId> m_resume;  // for every open variation: the move it is an alternative to
        bool m_lineHasMove{ false };              // the current line has a move already
        std::string m_pendingLine;                // the comment of a variation, until its first move is read
        std::string m_pendingCommands;            // the commands that came with it
    };
} // namespace

//-------------------------------------------------
// PgnHeaders
//-------------------------------------------------

sg::chess::core::PgnHeaders sg::chess::core::PgnHeaders::MakeDefault(const std::string_view t_date)
{
    PgnHeaders headers;

    for (const auto& [name, unknown] : ROSTER)
    {
        headers.tags.push_back({ std::string{ name }, std::string{ unknown } });
    }

    headers.Set("Event", "Analysis");
    headers.Set("Site", "SharpBlunder");
    headers.Set("Date", t_date.empty() ? NO_DATE : t_date);

    return headers;
}

std::string sg::chess::core::PgnHeaders::Get(const std::string_view t_name) const
{
    const auto it{ std::ranges::find(tags, t_name, &PgnTag::name) };

    return it != tags.end() ? it->value : std::string{};
}

void sg::chess::core::PgnHeaders::Set(const std::string_view t_name, const std::string_view t_value)
{
    const auto it{ std::ranges::find(tags, t_name, &PgnTag::name) };

    if (it != tags.end())
    {
        it->value = t_value;
    }
    else
    {
        tags.push_back({ std::string{ t_name }, std::string{ t_value } });
    }
}

//-------------------------------------------------
// Writing and reading
//-------------------------------------------------

std::string sg::chess::core::pgn_write(const GameState& t_game, const PgnHeaders& t_headers)
{
    std::string text;

    const auto writeTag = [&text](const std::string& t_name, const std::string& t_value)
    {
        text += "[" + t_name + " \"" + EscapeTagValue(t_value) + "\"]\n";
    };

    // The seven tag roster first, then the other tags.
    for (const auto& [name, unknown] : ROSTER)
    {
        const std::string value{ t_headers.Get(name) };
        writeTag(std::string{ name }, value.empty() ? std::string{ unknown } : value);
    }

    for (const PgnTag& tag : t_headers.tags)
    {
        const bool inRoster{ std::ranges::any_of(ROSTER, [&tag](const auto& t_entry)
                                                 { return t_entry.first == tag.name; }) };
        if (!inRoster)
        {
            writeTag(tag.name, tag.value);
        }
    }

    // A game that does not start from the standard position says where it starts.
    const std::string startFen{ t_game.At(0).info.position.ToFen() };
    if (startFen != Position::FromFen(Position::START_FEN).ToFen())
    {
        writeTag("SetUp", "1");
        writeTag("FEN", startFen);
    }

    text += '\n';

    std::vector<std::string> tokens;

    const GameState::Node& start{ t_game.At(0) };
    if (!start.note.comment.empty() || !start.note.commands.empty())
    {
        tokens.push_back(BraceComment(start.note.comment, start.note.commands));
    }

    if (!start.children.empty())
    {
        EmitLine(t_game, start.children.front(), false, tokens);
    }

    const std::string result{ t_headers.Get("Result") };
    tokens.push_back(result.empty() ? "*" : result);

    return text + Layout(tokens);
}

sg::chess::core::PgnGame sg::chess::core::pgn_read(const std::string_view t_text, RulesProvider& t_rules)
{
    PgnHeaders headers;
    std::string fen;
    const std::size_t movesStart{ ReadTags(t_text, headers, fen) };

    Position start{ Position::FromFen(Position::START_FEN) };
    if (!fen.empty())
    {
        try
        {
            start = Position::FromFen(fen);
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("PGN: the FEN tag is not a valid position: " + fen);
        }
    }

    PgnGame result{ std::move(headers), GameState(t_rules, start) };

    MoveReader reader{ t_text, result.game, result.headers };
    reader.Read(movesStart);

    result.game.GoToStart();

    return result;
}
