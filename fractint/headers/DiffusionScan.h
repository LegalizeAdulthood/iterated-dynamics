#if !defined(DIFFUSION_SCAN_H)
#define DIFFUSION_SCAN_H

extern int diffusion_scan();
extern void diffusion_get_calculation_time(char *message);
extern void diffusion_get_status(char *message);

extern unsigned long		g_diffusion_limit;

#endif