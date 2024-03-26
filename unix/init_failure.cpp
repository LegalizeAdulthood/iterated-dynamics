#include "init_failure.h"

#include <cstdio>

void init_failure(char const *message)
{
    std::printf("FAILED TO INITIALIZE\n%s\n", message);
}
