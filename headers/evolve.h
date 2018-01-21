#pragma once
#if !defined(EVOLVE_H)
#define EVOLVE_H

#include "fractint.h"

extern void copy_genes_to_bank(GENEBASE const gene[NUMGENES]);
extern void copy_genes_from_bank(GENEBASE gene[NUMGENES]);
extern  void initgene();
extern  void param_history(int);
extern  int get_variations();
extern  int get_evolve_Parms();
extern  void set_current_params();
extern  void fiddleparms(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges();
extern  void set_mutation_level(int);
extern  void drawparmbox(int);
extern  void spiralmap(int);
extern  int unspiralmap();
extern  void SetupParamBox();
extern  void ReleaseParamBox();

#endif
