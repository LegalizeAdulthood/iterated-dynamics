// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdint>

namespace id::ui
{

enum class Variations
{
    NONE = 0,       // don't vary
    X,              // vary with x axis
    Y,              // vary with y axis
    X_PLUS_Y,       // vary with x+y
    X_MINUS_Y,      // vary with x-y
    RANDOM,         // vary randomly
    WEIGHTED_RANDOM, // weighted random mutation, further out = further change
    NUM             // number of variation schemes
};

// smallest part of a fractal 'gene'
struct GeneBase
{
    void *addr;             // address of variable to be referenced
    void (*vary_fn)(GeneBase *genes, int rand_val, int gene); // pointer to func used to vary it
                            // takes random number and pointer to var
    Variations mutate;      // flag to switch on variation of this variable
    char name[16];          // name of variable (for menu )
    char level;             // mutation level at which this should become active
};

enum
{
    NUM_GENES = 21
};

/*
 * Note: because big-endian machines store structures differently, we have
 * to do special processing of the EVOLUTION_INFO structure as it is stored
 * in little-endian format.  If this structure changes, change the big-endian
 * marshalling routines in io/decode_info.h.
 */
struct EvolutionInfo      // for saving evolution data in a GIF file
{
    std::int16_t evolving;
    std::int16_t image_grid_size;
    std::uint16_t this_generation_random_seed;
    double max_random_mutation;
    double x_parameter_range;
    double y_parameter_range;
    double x_parameter_offset;
    double y_parameter_offset;
    std::int16_t discrete_x_parameter_offset;
    std::int16_t discrete_y_parameter_offset;
    std::int16_t px;
    std::int16_t py;
    std::int16_t screen_x_offset;
    std::int16_t screen_y_offset;
    std::int16_t x_dots;
    std::int16_t y_dots;
    std::int16_t mutate[NUM_GENES];
    std::int16_t count; // count of how many images have been calc'ed so far
    std::int16_t future[66 - NUM_GENES];      // total of 200 bytes
};

bool operator==(const EvolutionInfo &lhs, const EvolutionInfo &rhs);
inline bool operator!=(const EvolutionInfo &lhs, const EvolutionInfo &rhs)
{
    return !(lhs == rhs);
}

// bitmasks for evolution mode flag
enum class EvolutionModeFlags
{
    NONE = 0,        //
    FIELD_MAP = 1,   // steady field variations across screen
    RAND_WALK = 2,   // new param = last param +- rand()
    RAND_PARAM = 4,  // new param = constant +- rand()
    NO_GROUT = 8,    // no gaps between images
    PARAM_BOX = 128, //
};
inline int operator+(const EvolutionModeFlags value)
{
    return static_cast<int>(value);
}
inline EvolutionModeFlags operator|(const EvolutionModeFlags lhs, const EvolutionModeFlags rhs)
{
    return static_cast<EvolutionModeFlags>(+lhs | +rhs);
}
inline EvolutionModeFlags &operator|=(EvolutionModeFlags &lhs, const EvolutionModeFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}
inline EvolutionModeFlags operator^(const EvolutionModeFlags lhs, const EvolutionModeFlags rhs)
{
    return static_cast<EvolutionModeFlags>(+lhs ^ +rhs);
}
inline EvolutionModeFlags &operator^=(EvolutionModeFlags &lhs, const EvolutionModeFlags rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}
inline bool bit_set(const EvolutionModeFlags flags, const EvolutionModeFlags bit)
{
    return (+flags & +bit) == +bit;
}

extern char                  g_evolve_discrete_x_parameter_offset;
extern char                  g_evolve_discrete_y_parameter_offset;
extern double                g_evolve_dist_per_x;
extern double                g_evolve_dist_per_y;
extern int                   g_evolve_image_grid_size;
extern double                g_evolve_max_random_mutation;
extern double                g_evolve_mutation_reduction_factor;
extern char                  g_evolve_new_discrete_x_parameter_offset;
extern char                  g_evolve_new_discrete_y_parameter_offset;
extern double                g_evolve_new_x_parameter_offset;
extern double                g_evolve_new_y_parameter_offset;
extern int                   g_evolve_param_grid_x;
extern int                   g_evolve_param_grid_y;
extern int                   g_evolve_param_box_count;
extern double                g_evolve_param_zoom;
extern unsigned int          g_evolve_this_generation_random_seed;
extern double                g_evolve_x_parameter_offset;
extern double                g_evolve_x_parameter_range;
extern double                g_evolve_y_parameter_offset;
extern double                g_evolve_y_parameter_range;
extern EvolutionModeFlags  g_evolving;
extern GeneBase              g_gene_bank[NUM_GENES];

void copy_genes_to_bank(const GeneBase gene[NUM_GENES]);
void copy_genes_from_bank(GeneBase gene[NUM_GENES]);
void init_gene();
void save_param_history();
void restore_param_history();
int get_variations();
int get_evolve_params();
void set_current_params();
void fiddle_params(GeneBase gene[], int count);
void set_evolve_ranges();
void set_mutation_level(int strength);
void draw_param_box(int mode);
void spiral_map(int count);
int unspiral_map();
void setup_param_box();
void release_param_box();

} // namespace id::ui
