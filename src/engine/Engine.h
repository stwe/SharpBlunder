#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

namespace sg::chess::engine
{
    //-------------------------------------------------
    // Engine
    //-------------------------------------------------

    /**
     * @brief A chess engine (Stockfish) running as a child process, spoken to via the UCI protocol.
     *
     * UCI is a text protocol: we write command lines to the engine's standard input
     * and read answer lines from its standard output.
     *
     * Reading blocks until the engine has answered. That is fine for short questions
     * (legal moves take a few milliseconds), but not for a long running analysis.
     * The engine must not be used from several threads at the same time.
     */
    class Engine
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Starts the engine and performs the UCI handshake.
         *
         * If the name cannot be found, "/usr/games/stockfish" (the Debian/Ubuntu location) is tried as well.
         *
         * @param t_executable Name or path of the engine, e.g. "stockfish".
         *
         * @throws std::runtime_error If the engine cannot be started or does not answer.
         */
        explicit Engine(const std::string& t_executable);

        Engine(const Engine& t_other) = delete;
        Engine(Engine&& t_other) noexcept = delete;
        Engine& operator=(const Engine& t_other) = delete;
        Engine& operator=(Engine&& t_other) noexcept = delete;

        ~Engine() noexcept;

        //-------------------------------------------------
        // Communication
        //-------------------------------------------------

        /**
         * @brief Sends one command line to the engine.
         *
         * @param t_command The command without a line break, e.g. "isready".
         *
         * @throws std::runtime_error If the engine is no longer there.
         */
        void Send(std::string_view t_command);

        /**
         * @brief Reads lines from the engine until one starts with the given text.
         *
         * @param t_prefix The beginning of the line to wait for, e.g. "readyok".
         *
         * @return All lines read, including the last one that starts with the prefix.
         *
         * @throws std::runtime_error If the engine ends before such a line arrives.
         */
        [[nodiscard]] std::vector<std::string> ReadUntil(std::string_view t_prefix);

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Reads one line, without the line break.
         *
         * @param t_line Receives the line.
         *
         * @return False if the engine's output has ended.
         */
        [[nodiscard]] bool ReadLine(std::string& t_line);

        /**
         * @brief Stops the process and closes the pipes. Safe to call more than once.
         */
        void Shutdown() noexcept;

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        pid_t m_pid{ -1 };          ///< Process id of the engine
        int m_writeFd{ -1 };        ///< Pipe to the engine's standard input
        FILE* m_reader{ nullptr };  ///< Pipe from the engine's standard output
    };
}
