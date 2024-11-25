#include <fractalp.h>

#include <gtest/gtest.h>

TEST(TestFractalSpecific, perturbationFlagRequiresPerturbationFunctions)
{
    for (int i =0; i < g_num_fractal_types; ++i)
    {
        if (!bit_set(g_fractal_specific[i].flags, fractal_flags::PERTURB))
        {
            continue;
        }

        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref_bf)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation bigfloat reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_pt)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation point function";
    }
}
