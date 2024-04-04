#pragma once

#include <cstdint>

enum class variations
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
struct GENEBASE
{
    void *addr;             // address of variable to be referenced
    void (*varyfunc)(GENEBASE *genes, int randval, int gene); // pointer to func used to vary it
                            // takes random number and pointer to var
    variations mutate;      // flag to switch on variation of this variable
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
 * marshalling routines in decode_info.h.
 */
struct EVOLUTION_INFO      // for saving evolution data in a GIF file
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
    std::int16_t discrete_y_paramter_offset;
    std::int16_t px;
    std::int16_t py;
    std::int16_t sxoffs;
    std::int16_t syoffs;
    std::int16_t xdots;
    std::int16_t ydots;
    std::int16_t mutate[NUM_GENES];
    std::int16_t ecount; // count of how many images have been calc'ed so far
    std::int16_t future[66 - NUM_GENES];      // total of 200 bytes
};

bool operator==(const EVOLUTION_INFO &lhs, const EVOLUTION_INFO &rhs);
inline bool operator!=(const EVOLUTION_INFO &lhs, const EVOLUTION_INFO &rhs)
{
    return !(lhs == rhs);
}

// more bitmasks for evolution mode flag
enum
{
    FIELDMAP = 1,  // steady field varyiations across screen
    RANDWALK = 2,  // newparm = lastparm +- rand()
    RANDPARAM = 4, // newparm = constant +- rand()
    NOGROUT = 8    // no gaps between images
};

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
extern int                   g_evolving;
extern GENEBASE              g_gene_bank[NUM_GENES];

void copy_genes_to_bank(GENEBASE const gene[NUM_GENES]);
void copy_genes_from_bank(GENEBASE gene[NUM_GENES]);
void initgene();
void param_history(int);
int get_variations();
int get_evolve_Parms();
void set_current_params();
void fiddleparms(GENEBASE gene[], int ecount);
void set_evolve_ranges();
void set_mutation_level(int);
void drawparmbox(int);
void spiralmap(int);
int unspiralmap();
void SetupParamBox();
void ReleaseParamBox();
