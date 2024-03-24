#include "shell_sort.h"

#include <string.h>

void shell_sort(void *v1, int n, unsigned sz)
{
    auto lccompare = [](void *arg1, void *arg2) // for sort
    {
        char **choice1 = (char **) arg1;
        char **choice2 = (char **) arg2;

        return stricmp(*choice1, *choice2);
    };

    void *temp;
    char *v;
    v = (char *)v1;
    for (int gap = n/2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < n; i++)
        {
            for (int j = i-gap; j >= 0; j -= gap)
            {
                if (lccompare((char **)(v+j*sz), (char **)(v+(j+gap)*sz)) <= 0)
                {
                    break;
                }
                temp = *(char **)(v+j*sz);
                *(char **)(v+j*sz) = *(char **)(v+(j+gap)*sz);
                *(char **)(v+(j+gap)*sz) = static_cast<char *>(temp);
            }
        }
    }
}
