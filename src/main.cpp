#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include "ui/App.h"
#include "core/Util.h"

int main(const int argc, char** argv)
{
    // Writing to a pipe whose reader has gone must not kill the program (e.g. Stockfish crashed).
    // The write then fails with an error, which the engine class reports as an exception.
    std::signal(SIGPIPE, SIG_IGN);

    try
    {
        sg::chess::ui::App app(sg::chess::core::stockfish_path(argc, argv));
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
