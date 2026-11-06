#include <cstdlib>
#include <exception>
#include <iostream>
#include "ui/App.h"

//-------------------------------------------------
// main
//-------------------------------------------------

int main()
{
    try
    {
        sg::chess::ui::App app;
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
