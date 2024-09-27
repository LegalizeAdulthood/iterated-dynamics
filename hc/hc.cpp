// SPDX-License-Identifier: GPL-3.0-only
//
#include "compiler.h"

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        return create_compiler(hc::parse_options(argc, argv))->process();
    }
    catch (const std::exception &bang)
    {
        std::cerr << "Unexpected exception: " << bang.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unexpected exception.\n";
        return 2;
    }
}
