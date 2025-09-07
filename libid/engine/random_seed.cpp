#include "engine/random_seed.h"

#include <cstdlib>

namespace id
{

bool g_random_seed_flag{};                                //
int g_random_seed{};                                      // Random number seeding flag and value

void set_random_seed()
{
    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }
}

} // namespace id
