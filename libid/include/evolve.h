#pragma once

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

// smallest part of a fractint 'gene'
struct GENEBASE
{
    void *addr;             // address of variable to be referenced
    void (*varyfunc)(GENEBASE *genes, int randval, int gene); // pointer to func used to vary it
                            // takes random number and pointer to var
    variations mutate;      // flag to switch on variation of this variable
    char name[16];          // name of variable (for menu )
    char level;             // mutation level at which this should become active
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
