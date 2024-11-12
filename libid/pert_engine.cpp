// SPDX-License-Identifier: GPL-3.0-only
//
//////////////////////////////////////////////////////////////////////
//    PERT_ENGINE.CPP a module to explore Perturbation
//    Thanks to Claude Heiland-Allen https://fractalforums.org/programming/11/perturbation-code-for-cubic-and-higher-order-polynomials/2783
//    Written in Microsoft Visual C++ by Paul de Leeuw.
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <time.h>
#include <complex>
#include "Big.h"
//#include "Dib.h"
#include "filter.h"
#include "Big.h"
#include "id.h"
//#include "mpfr.h"
#include "pert_engine.h"
#include "Drivers.h"
#include "calcfrac.h"
#include "complex_fn.h"

//////////////////////////////////////////////////////////////////////
// Initialisation
//////////////////////////////////////////////////////////////////////


int CPertEngine::initialise_frame(int WidthIn, int HeightIn, int threshold, bf_t xBigZoomPointin,
    bf_t yBigZoomPointin, double xZoomPointin, double yZoomPointin, double ZoomRadiusIn, bool IsPotentialIn,
    bf_math_type math_typeIn, double g_params[] /*, CTZfilter *TZfilter*/)
    {
    std::complex<double> q;
    int     i;

    m_width = WidthIn;
    m_height = HeightIn;
    m_max_iteration = threshold;
    m_zoom_radius = ZoomRadiusIn;
    m_is_potential = IsPotentialIn;
    m_math_type = math_typeIn;
    for (i = 0; i < MAX_PARAMS; i++)
        m_param[i] = g_params[i];

    //    method = TZfilter->method;

    if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
    {
        m_saved = save_stack();
        m_x_zoom_pt_bf = alloc_stack(g_bf_length + 2);
        m_y_zoom_pt_bf = alloc_stack(g_bf_length + 2);
        copy_bf(m_x_zoom_pt_bf, xBigZoomPointin);
        copy_bf(m_y_zoom_pt_bf, yBigZoomPointin);
    }
    else
    {
        m_x_zoom_pt = xZoomPointin;
        m_y_zoom_pt = yZoomPointin;
    }

    /*
    if (method >= TIERAZONFILTERS)
	    {
	    q = { mpfr_get_d(xZoomPt, MPFR_RNDN), mpfr_get_d(yZoomPt, MPFR_RNDN) };
	    TZfilter->LoadFilterQ(q);		// initialise the constants used by Tierazon fractals
	    }
*/
    return 0;
    }

//////////////////////////////////////////////////////////////////////
// Full frame calculation
//////////////////////////////////////////////////////////////////////

int CPertEngine::calculate_one_frame(double bailout, char *status_bar_info, int powerin, int InsideFilterIn, int OutsideFilterIn, int biomorphin, int subtypein,
    void (*plot)(int, int, int), int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/)

    {
    int i;
    BFComplex c_bf, reference_coordinate_bf;
    std::complex<double> c, reference_coordinate;

    m_reference_points = 0;
    m_glitch_point_count = 0L;
    m_remaining_point_count = 0L;

    // get memory for all point arrays
    m_points_remaining = new Point[m_width * m_height];
    if (m_points_remaining == NULL)
	    return -1;
    m_glitch_points = new Point[m_width * m_height];
    if (m_glitch_points == NULL)
        {
	    if (m_points_remaining) { delete[] m_points_remaining; m_points_remaining = NULL; }
	    return -1;
        }
	// get memory for Perturbation Tolerance Check array
        m_perturbation_tolerance_check = new double[m_max_iteration * 2];
        if (m_perturbation_tolerance_check == NULL)
	    {
	    if (m_points_remaining) { delete[] m_points_remaining; m_points_remaining = NULL; }
	    if (m_glitch_points) { delete[] m_glitch_points; m_glitch_points = NULL; }
	    return -2;
	    }
	// get memory for Z array
            m_x_sub_n = new std::complex<double>[m_max_iteration + 1];
	if (m_x_sub_n == NULL)
	    {
	    if (m_points_remaining) { delete[] m_points_remaining; m_points_remaining = NULL; }
	    if (m_glitch_points) { delete[] m_glitch_points; m_glitch_points = NULL; }
	    if (m_perturbation_tolerance_check) { delete[] m_perturbation_tolerance_check; m_perturbation_tolerance_check = NULL; }
	    return -2;
	    }
    m_biomorph = biomorphin;
    m_power = powerin;
    if (m_power < 2)
	    m_power = 2;
    if (m_power > MAXPOWER)
	    m_power = MAXPOWER;
    m_inside_method = InsideFilterIn;
    m_outside_method = OutsideFilterIn;
    m_subtype = subtypein;

    // calculate the pascal's triangle coefficients for powers > 3
    load_pascal(m_pascal_array, m_power);
    //Fill the list of points with all points in the image.
    for (long y = 0; y < m_height; y++) 
	    {
	    for (long x = 0; x < m_width; x++) 
	        {
	        Point pt(x, m_height - 1 - y);
	        *(m_points_remaining + y * m_width + x) = pt;
	        m_remaining_point_count++;
	        }
	    }

    double  magnified_radius = m_zoom_radius;
    int     window_radius = (m_width < m_height) ? m_width : m_height;
    int     cplxsaved;
    bf_t    tmp_bf;

    if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
    {
        cplxsaved = save_stack();
        c_bf.x = alloc_stack(g_r_bf_length + 2);
        c_bf.y = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.x = alloc_stack(g_r_bf_length + 2);
        reference_coordinate_bf.y = alloc_stack(g_r_bf_length + 2);
        tmp_bf = alloc_stack(g_r_bf_length + 2);
    }

    while (m_remaining_point_count > (m_width * m_height) * (m_percent_glitch_tolerance / 100))
	    {
        m_reference_points++;

	    //Determine the reference point to calculate
	    //Check whether this is the first time running the loop. 
	    if (m_reference_points == 1) 
	        {
            if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
                {
                copy_bf(c_bf.x, m_x_zoom_pt_bf);
                copy_bf(c_bf.y, m_y_zoom_pt_bf);
                copy_bf(reference_coordinate_bf.x, c_bf.x);
                copy_bf(reference_coordinate_bf.y, c_bf.y);
                }
            else
                {
                c.real(m_x_zoom_pt);
                c.imag(m_y_zoom_pt);
                reference_coordinate = c;
                }
 
	        m_calculated_real_delta = 0;
	        m_calculated_imaginary_delta = 0;

            int result;
            if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
                {
                result = reference_zoom_point_bf(&reference_coordinate_bf, m_max_iteration, status_bar_info);
                }
            else
                {
                result = reference_zoom_point(&reference_coordinate, m_max_iteration, status_bar_info);
                }
	        if (result < 0)
		        {
		        close_the_damn_pointers();
		        return -1;
		        }
	        }
	    else 
	        {
	        if (m_calculate_glitches == false) 
		        break;

	        int referencePointIndex = 0;
	        int Randomise;

	        srand((unsigned)time(NULL));		// Seed the random-number generator with current time 
	        Randomise = rand();
	        referencePointIndex = (int)((double)Randomise / (RAND_MAX + 1) * m_remaining_point_count);
            Point   pt = *(m_points_remaining + referencePointIndex);
	        //Get the complex point at the chosen reference point
            double deltaReal = ((magnified_radius * (2 * pt.getX() - m_width)) / window_radius);
            double deltaImaginary = ((-magnified_radius * (2 * pt.getY() - m_height)) / window_radius);

	        // We need to store this offset because the formula we use to convert pixels into a complex point does so relative to the center of the image.
	        // We need to offset that calculation when our reference point isn't in the center. The actual offsetting is done in calculate point.

            m_calculated_real_delta = deltaReal;
            m_calculated_imaginary_delta = deltaImaginary;

            if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
                {
                floattobf(tmp_bf, deltaReal);
                add_bf(reference_coordinate_bf.x, c_bf.x, tmp_bf);
                floattobf(tmp_bf, deltaImaginary);
                add_bf(reference_coordinate_bf.y, c_bf.y, tmp_bf);
                }
            else
                {
                reference_coordinate.real(c.real() + deltaReal);
                reference_coordinate.imag(c.imag() + deltaImaginary);
                }

            int result;
            if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
                {
                result = reference_zoom_point_bf(&reference_coordinate_bf, m_max_iteration, status_bar_info);
                }
            else
                {
                result = reference_zoom_point(&reference_coordinate, m_max_iteration, status_bar_info);
                }
             if (result < 0)
                {
                 close_the_damn_pointers();
                return -1;
                }
            }

	    int lastChecked = -1;
	    m_glitch_point_count = 0;
	    for (i = 0; i < m_remaining_point_count; i++)
	        {
            if (i % 1000 == 0)
                if (driver_key_pressed())
                    return -1;
            Point   pt = *(m_points_remaining + i);
            if (calculate_point(pt.getX(), pt.getY(), magnified_radius, window_radius, bailout, m_glitch_points, plot, potential/*, TZfilter, TrueCol*/) < 0)
		        return -1;
	        //Everything else in this loop is just for updating the progress counter. 
	        double progress = (double) i / m_remaining_point_count;
	        if (int(progress * 100) != lastChecked) 
		        {
		        lastChecked = int(progress * 100);
		        sprintf(status_bar_info, "Pass: %d, (%d%%)", m_reference_points, int(progress * 100));
		        }
	        }

	    //These points are glitched, so we need to mark them for recalculation. We need to recalculate them using Pauldelbrot's glitch fixing method (see calculate point).
	    memcpy(m_points_remaining, m_glitch_points, sizeof(Point) * m_glitch_point_count);
	    m_remaining_point_count = m_glitch_point_count;
	    }

    if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        restore_stack(cplxsaved);
    close_the_damn_pointers();
    return 0;
    }

//////////////////////////////////////////////////////////////////////
// Cleanup
//////////////////////////////////////////////////////////////////////

void CPertEngine::close_the_damn_pointers(void)
    {
    if (m_math_type != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        restore_stack(m_saved);
    if (m_points_remaining) {delete[] m_points_remaining; m_points_remaining = NULL;}
    if (m_glitch_points) {delete[] m_glitch_points; m_glitch_points = NULL;}
    if (m_x_sub_n) { delete[] m_x_sub_n; m_x_sub_n = NULL; }
	if (m_perturbation_tolerance_check) { delete[] m_perturbation_tolerance_check; m_perturbation_tolerance_check = NULL; }
    }

//////////////////////////////////////////////////////////////////////
// Individual point calculation
//////////////////////////////////////////////////////////////////////

int CPertEngine::calculate_point(int x, int y, double magnified_radius, int window_radius, double bailout, Point *m_glitch_points, void (*plot)(int, int, int),
            int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/)
    {
// Get the complex number at this pixel.
// This calculates the number relative to the reference point, so we need to translate that to the center when the reference point isn't in the center.
// That's why for the first reference, m_calculated_real_delta and m_calculated_imaginary_delta are 0: it's calculating relative to the center.

    double delta_real = ((magnified_radius * (2 * x - m_width)) / window_radius) - m_calculated_real_delta;
    double  delta_imaginary = ((-magnified_radius * (2 * y - m_height)) / window_radius) - m_calculated_imaginary_delta;
    double	magnitude = 0.0;
    std::complex<double> delta_sub_0{delta_real, delta_imaginary};
    std::complex<double> delta_sub_n;
    delta_sub_n = delta_sub_0;
    int     iteration = 0;
    bool    glitched = false;
    CComplexFn complex_fn;

    double  BOF_magnitude;
    double  min_orbit;      // orbit value closest to origin
    long    min_index;      // iteration of min_orbit
    if (m_inside_method == BOF60 || m_inside_method == BOF61)
        {
        BOF_magnitude = 0.0;
        min_orbit = 100000.0;
        }

    //Iteration loop
    do
	    {
        pert_functions((m_x_sub_n + iteration), &delta_sub_n, &delta_sub_0);
	    iteration++;
        std::complex<double> CoordMag = *(m_x_sub_n + iteration) + delta_sub_n;
        m_z_coordinate_magnitude_squared = sqr(CoordMag.real()) + sqr(CoordMag.imag());

        if (m_inside_method == BOF60 || m_inside_method == BOF61)
	        {
            std::complex<double> z = *(m_x_sub_n + iteration) + delta_sub_n;
	        BOF_magnitude = complex_fn.sum_squared(z);
            if (BOF_magnitude < min_orbit)
		        {
                min_orbit = BOF_magnitude;
		        min_index = iteration + 1L;
		        }
	        }


	    // This is Pauldelbrot's glitch detection method. You can see it here: http://www.fractalforums.com/announcements-and-news/pertubation-theory-glitches-improvement/.
	    // As for why it looks so weird, it's because I've squared both sides of his equation and moved the |ZsubN| to the other side to be precalculated.
	    // For more information, look at where the reference point is calculated.
	    // I also only want to store this point once.

    //	if (method >= TIERAZONFILTERS)
    //	    {
    //	    Complex z = XSubN[iteration] + DeltaSubN;
    //	    TZfilter->DoTierazonFilter(z, (long *)&iteration);
    //	    }

	    if (m_calculate_glitches == true && glitched == false && m_z_coordinate_magnitude_squared < m_perturbation_tolerance_check[iteration])
	        {
	        Point pt(x, y, iteration);
	        m_glitch_points[m_glitch_point_count] = pt;
	        m_glitch_point_count++;
	        glitched = true;
	        break;
	        }

	    //use bailout radius of 256 for smooth coloring.
	    } while (m_z_coordinate_magnitude_squared < bailout && iteration < m_max_iteration);

    if (glitched == false) 
        {
	    int	index;
	    double	rqlim2 = sqrt(bailout);
	    std::complex<double>	w = m_x_sub_n[iteration] + delta_sub_n;

	    if (m_biomorph >= 0)						// biomorph
	        {
            if (iteration == m_max_iteration)
	            index = m_max_iteration;
	        else
		        {
		        if (fabs(w.real()) < rqlim2 || fabs(w.imag()) < rqlim2)
		            index = m_biomorph;
		        else
		            index = iteration % 256;
		        }
	        }
	    else
	        {
	        switch (m_outside_method)
		        {
		        case 0:						// no filter
		            if (iteration == m_max_iteration)
		                index = m_max_iteration;
		            else
			            index = iteration % 256;
		            break;
//		        case PERT1:						// something Shirom Makkad added
//		            if (iteration == MaxIteration)
//			            index = MaxIteration;
//		            else
//			            index = (int)((iteration - log2(log2(m_z_coordinate_magnitude_squared))) * 5) % 256; //Get the index of the color array that we are going to read from. 
//		            break;
//		        case PERT2:						// something Shirom Makkad added
//		            if (iteration == MaxIteration)
//			            index = MaxIteration;
//		            else
//			            index = (int)(iteration - (log(0.5*(m_z_coordinate_magnitude_squared)) - log(0.5*log(256))) / log(2)) % 256;
//		            break;
		        case ZMAG:
		            if (iteration == m_max_iteration)			// Zmag
			            index = (int)((w.real() * w.real() + w.imag() + w.imag()) * (m_max_iteration >> 1) + 1);
		            else
			            index = iteration % 256;
		            break;
		        case REAL:						// "real"
		            if (iteration == m_max_iteration)
			            index = m_max_iteration;
		            else
			            index = iteration + (long)w.real() + 7;
		            break;
		        case IMAG:	    					// "imag"
		            if (iteration == m_max_iteration)
			            index = m_max_iteration;
		            else
			            index = iteration + (long)w.imag() + 7;
		            break;
		        case MULT:						// "mult"
		            if (iteration == m_max_iteration)
			            index = m_max_iteration;
		            else if (w.imag())
			            index = (long)((double)iteration * (w.real() / w.imag()));
                    else
                        index = iteration;
		            break;
		        case SUM:						// "sum"
		            if (iteration == m_max_iteration)
			            index = m_max_iteration;
		            else
			            index = iteration + (long)(w.real() + w.imag());
		            break;
		        case ATAN:						// "atan"
		            if (iteration == m_max_iteration)
			            index = m_max_iteration;
		            else
			            index = (long)fabs(atan2(w.imag(), w.real())*180.0 / PI);
		            break;
		        default:
                    if (m_is_potential)
                        {
                        magnitude = sqr(w.real()) + sqr(w.imag());
                        index = potential(magnitude, iteration);
                        }
//		            else if (method >= TIERAZONFILTERS)			// suite of Tierazon filters and colouring schemes
//			            {
//			            TZfilter->EndTierazonFilter(w, (long *)&iteration, TrueCol);
//			            index = iteration;
//			            }
		            else						// no filter
			            {
			            if (iteration == m_max_iteration)
			                index = m_max_iteration;
			            else
			                index = iteration % 256;
			            }
		            break;
		        }
                        if (m_inside_method >= 0) // no filter
                {
                if (iteration == m_max_iteration)
                    index = m_inside_method;
		        else
			        index = iteration % 256;
                }
            else
                {
                switch (m_inside_method)
		            {
		            case ZMAG:
		                if (iteration == m_max_iteration)			// Zmag
			                index = (int)(complex_fn.sum_squared(w) * (m_max_iteration >> 1) + 1);
		                break;
		            case BOF60:
		                if (iteration == m_max_iteration)
			                index = (int)(sqrt(min_orbit) * 75.0);
		                break;
		            case BOF61:
		                if (iteration == m_max_iteration)
			                index = min_index;
		                break;
/*
		            case POTENTIAL:
		                magnitude = sqr(w.x) + sqr(w.y);
		                index = Pot.potential(magnitude, iteration, MaxIteration, TrueCol, 256, potparam);
		                break;
		            default:
		                if (InsideMethod >= TIERAZONFILTERS)		// suite of Tierazon filters and colouring schemes
			                {
			                TZfilter->EndTierazonFilter(w, (long *)&iteration, TrueCol);
			                index = iteration;
			                }
		                break;
*/
		            }
	            }
	        plot(x, m_height - 1 - y, index);
            }
	    }
    return 0;
    }

//////////////////////////////////////////////////////////////////////
// Reference Zoom Point - BigFlt
//////////////////////////////////////////////////////////////////////

int CPertEngine::reference_zoom_point_bf(BFComplex *centre, int max_iteration, char* status_bar_info)
    {
    // Raising this number makes more calculations, but less variation between each calculation (less chance of mis-identifying a glitched point).
    BFComplex   z_times_2_bf, z_bf;
    bf_t   temp_real_bf, temp_imag_bf, tmp_bf;
    double      glitch_tolerancy = 1e-6;
    int cplxsaved;

    cplxsaved = save_stack();
    z_times_2_bf.x = alloc_stack(g_r_bf_length + 2);
    z_times_2_bf.y = alloc_stack(g_r_bf_length + 2);
    z_bf.x = alloc_stack(g_r_bf_length + 2);
    z_bf.y = alloc_stack(g_r_bf_length + 2);
    temp_real_bf = alloc_stack(g_r_bf_length + 2);
    temp_imag_bf = alloc_stack(g_r_bf_length + 2);
    tmp_bf = alloc_stack(g_r_bf_length + 2);

    copy_bf(z_bf.x, centre->x);
    copy_bf(z_bf.y, centre->y);
    //    Z = *centre;

    for (int i = 0; i <= max_iteration; i++)
	    {
        std::complex<double> c;
	    // pre multiply by two
        double_bf(z_times_2_bf.x, z_bf.x);
        double_bf(z_times_2_bf.y, z_bf.y);

        c.real(bftofloat(z_bf.x));
        c.imag(bftofloat(z_bf.y));

       // The reason we are storing the same value times two is that we can precalculate this value here
       // because multiplying this value by two is needed many times in the program.
	    // Also, for some reason, we can't multiply complex numbers by anything greater than 1 using std::complex, so we have to multiply the individual terms each time.
	    // This is expensive to do above, so we are just doing it here.

	    m_x_sub_n[i] = c; 
	    // Norm is the squared version of abs and 0.000001 is 10^-3 squared.
	    // The reason we are storing this into an array is that we need to check the magnitude against this value to see if the value is glitched. 
	    // We are leaving it squared because otherwise we'd need to do a square root operation, which is expensive, so we'll just compare this to the squared magnitude.
	
	    //Everything else in this loop is just for updating the progress counter. 
	    int last_checked = -1;
	    double progress = (double)i / max_iteration;
        if (int(progress * 100) != last_checked)
	        {
	        last_checked = int(progress * 100);
	        sprintf(status_bar_info, "Pass: %d, Ref (%d%%)", m_reference_points, int(progress * 100));
	        }

        floattobf(tmp_bf, glitch_tolerancy);
        mult_bf(temp_real_bf, z_bf.x, tmp_bf);
        mult_bf(temp_imag_bf, z_bf.y, tmp_bf);
        std::complex<double> tolerancy;
        tolerancy.real(bftofloat(temp_real_bf));
        tolerancy.imag(bftofloat(temp_imag_bf));

        m_perturbation_tolerance_check[i] = sqr(tolerancy.real()) + sqr(tolerancy.imag());

	    // Calculate the set
        ref_functions_bf(centre, &z_bf, &z_times_2_bf);
	    }
    restore_stack(cplxsaved);
    return 0;
    }
    
//////////////////////////////////////////////////////////////////////
// Reference Zoom Point - BigFlt
//////////////////////////////////////////////////////////////////////

int CPertEngine::reference_zoom_point(std::complex<double> *centre, int max_iteration, char* status_bar_info)
    {
    // Raising this number makes more calculations, but less variation between each calculation (less chance of mis-identifying a glitched point).
    std::complex<double> z_times_2, z;
    double  glitch_tolerancy = 1e-6;

    z = *centre;

    for (int i = 0; i <= max_iteration; i++)
	    {
        std::complex<double> c;
	    // pre multiply by two
        z_times_2 = z + z;
	    c = z;

       // The reason we are storing the same value times two is that we can precalculate this value here
       // because multiplying this value by two is needed many times in the program.
	    // Also, for some reason, we can't multiply complex numbers by anything greater than 1 using std::complex, so we have to multiply the individual terms each time.
	    // This is expensive to do above, so we are just doing it here.

	    m_x_sub_n[i] = c; 
	    // Norm is the squared version of abs and 0.000001 is 10^-3 squared.
	    // The reason we are storing this into an array is that we need to check the magnitude against this value to see if the value is glitched. 
	    // We are leaving it squared because otherwise we'd need to do a square root operation, which is expensive, so we'll just compare this to the squared magnitude.
	
	    //Everything else in this loop is just for updating the progress counter. 
	    int last_checked = -1;
        double progress = (double) i / max_iteration;
	    if (int(progress * 100) != last_checked)
	        {
	        last_checked = int(progress * 100);
	        sprintf(status_bar_info, "Pass: %d, Ref (%d%%)", m_reference_points, int(progress * 100));
	        }

	    std::complex<double> tolerancy = z * glitch_tolerancy;
        m_perturbation_tolerance_check[i] = sqr(tolerancy.real()) + sqr(tolerancy.imag());

	    // Calculate the set
        ref_functions(centre, &z, &z_times_2);
	    }
    return 0;
    }
    
