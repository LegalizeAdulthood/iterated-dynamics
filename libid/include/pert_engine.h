// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once
#include <windows.h>
#include <stdio.h>
#include <complex>
#include "Big.h"
#include "Point.h"
#include "filter.h"
#include "id_keys.h"
#include "fractalb.h"
#include "biginit.h"
#include "id.h"
#include "complex_fn.h"

#define	MAXPOWER    28
#define MAXFILTER   9

class CPertEngine 
    {
    public:
    int     initialise_frame(int WidthIn, int HeightIn, int threshold, bf_t xBigZoomPointin, bf_t yBigZoomPointin,
            double xZoomPointin, double yZoomPointin, double m_zoom_radiusZoomRadiusIn, bool IsPotentialIn, bf_math_type math_typeIn, double g_params[] /*, CTZfilter *TZfilter*/);
    int     calculate_one_frame(double bailout, char *status_bar_info, int powerin, int InsideFilterIn, int OutsideFilterIn, int biomorph, int subtype,
            void (*plot)(int, int, int), int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/);
    private:
    int     calculate_point(int x, int y, double tempRadius, int window_radius, double bailout,
            Point *glitchPoints, void (*plot)(int, int, int), int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/);
    int     reference_zoom_point_bf(BFComplex *BigCentre, int maxIteration, char *status_bar_info);
    int     reference_zoom_point(std::complex<double> *centre, int maxIteration, char *status_bar_info);
	void	load_pascal(long PascalArray[], int n);
    double  diff_abs(const double c, const double d);
	void	pert_functions(std::complex<double> *XRef, std::complex<double> *DeltaSubN, std::complex<double> *DeltaSub0);
    void    ref_functions_bf(BFComplex *centre, BFComplex *Z, BFComplex *ZTimes2);
    void    ref_functions(std::complex<double> *centre, std::complex<double> *Z, std::complex<double> *ZTimes2);
	void	close_the_damn_pointers(void);
    void    complex_polynomial_bf(BFComplex *out, BFComplex in, int degree);
    void    complex_cube_bf(BFComplex *out, BFComplex in);

	std::complex<double> *m_x_sub_n = NULL;
	double	*m_perturbation_tolerance_check = NULL;
	double	m_calculated_real_delta, m_calculated_imaginary_delta;
	double	m_z_coordinate_magnitude_squared;

	long	m_pascal_array[MAXPOWER];
	Point	*m_points_remaining = NULL;
	Point	*m_glitch_points = NULL;
  	double	m_param[MAX_PARAMS];

    bool    m_is_potential = false;

	int	    m_width, m_height;
	int	    m_max_iteration;
	int	    m_power, m_subtype, m_biomorph; 
	int	    m_inside_method;			// the number of the inside filter
	int	    m_outside_method;			// the number of the outside filter
	long	m_glitch_point_count;
	long	m_remaining_point_count;
    bf_t	m_x_zoom_pt_bf, m_y_zoom_pt_bf;
    double	m_x_zoom_pt, m_y_zoom_pt;
	double	m_zoom_radius;
    double  m_x_centre, m_y_centre;
	bool	m_calculate_glitches = true;
	unsigned int m_num_coefficients = 5;
    double  m_percent_glitch_tolerance = 0.1; // What percentage of the image is okay to be glitched. 
	int	    m_reference_points = 0;
    int     m_saved;              // keep track of bigflt memory
    bf_math_type m_math_type;
    };

