#if !defined(EVOLVE_H)
#define EVOLVE_H

extern void save_parameter_history();
extern void restore_parameter_history();
extern  int get_evolve_parameters();
extern  void set_current_parameters();
extern  void fiddle_parameters(GENEBASE gene[], int ecount);
extern  void set_evolve_ranges();
extern  void set_mutation_level(int);
extern  void draw_parameter_box(int);
extern  void spiral_map(int);
extern  int unspiral_map();
extern  void setup_parameter_box();
extern  void release_parameter_box();

#endif
