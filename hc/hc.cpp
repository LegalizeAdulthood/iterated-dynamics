#include "compiler.h"

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        return hc::compiler(argc, argv).process();
    }
    catch (const std::exception &bang)
    {
        std::cerr << "Unexpected exception: " << bang.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unxpected exception.\n";
        return 2;
    }
}
