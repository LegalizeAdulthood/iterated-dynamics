#pragma once

extern int plasma();
extern int bifurcation();
extern int bifurcation_lambda();
extern int bifurcation_set_trig_pi_fp();
extern int bifurcation_set_trig_pi();
extern int bifurcation_add_trig_pi_fp();
extern int bifurcation_add_trig_pi();
extern int bifurcation_may_fp();
extern bool bifurcation_may_setup();
extern int bifurcation_may();
extern int bifurcation_lambda_trig_fp();
extern int bifurcation_lambda_trig();
extern int bifurcation_verhulst_trig();
extern int bifurcation_verhulst_trig_fp();
extern int bifurcation_stewart_trig_fp();
extern int bifurcation_stewart_trig();
extern int popcorn();
extern int lyapunov();
extern bool lyapunov_setup();
extern int cellular();
extern bool cellular_setup();
extern std::string precision_format(const char *specifier, int precision);
