// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/shell_sort.h"

#include <config/string_case_compare.h>

using namespace id::config;

namespace id::ui
{

void shell_sort(void *v1, int n, unsigned sz)
{
    const auto lc_compare = [](void *arg1, void *arg2) // for sort
    {
        char **choice1 = static_cast<char **>(arg1);
        char **choice2 = static_cast<char **>(arg2);

        return string_case_compare(*choice1, *choice2);
    };

    char *v = static_cast<char *>(v1);
    for (int gap = n/2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < n; i++)
        {
            for (int j = i-gap; j >= 0; j -= gap)
            {
                if (lc_compare(v + j * sz, v + (j + gap) * sz) <= 0)
                {
                    break;
                }
                void *temp = *(char **) (v + j * sz);
                *(char **)(v+j*sz) = *(char **)(v+(j+gap)*sz);
                *(char **)(v+(j+gap)*sz) = static_cast<char *>(temp);
            }
        }
    }
}

} // namespace id::ui
