// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once
#include <windows.h>
#include <stdio.h>
#include "Big.h"
#include "Point.h"
#include "filter.h"
#include "Complex.h"
#include "id_keys.h"
#include "fractalb.h"
#include "biginit.h"
#include "id.h"
/*
#include "Dib.h"
#include "colour.h"
*/
//#include "BigDouble.h"
//#include "BigComplex.h"

#define	MAXPOWER    28
#define MAXFILTER   9

class CPertEngine 
    {
    public:
    int     initialiseCalculateFrame(int WidthIn, int HeightIn, int threshold, bf_t xBigZoomPointin, bf_t yBigZoomPointin,
            double xZoomPointin, double yZoomPointin, double ZoomRadiusIn, bool IsPotentialIn, bf_math_type math_typeIn, double g_params[] /*, CTZfilter *TZfilter*/);
    int     calculateOneFrame(double bailout, char *StatusBarInfo, int powerin, int InsideFilterIn, int OutsideFilterIn, int biomorph, int subtype, Complex RSRA, bool RSRsign, int user_data(),
                            void (*plot)(int, int, int), int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/);
    private:
    int     calculatePoint(int x, int y, double tempRadius, int window_radius, double bailout,
            Point *glitchPoints, int user_data(), void (*plot)(int, int, int), int potential(double, long)/*, CTZfilter *TZfilter, CTrueCol *TrueCol*/);
    int     BigReferenceZoomPoint(BFComplex *BigCentre, int maxIteration, int user_data(), char *StatusBarInfo);
    int     ReferenceZoomPoint(Complex *centre, int maxIteration, int user_data(), char *StatusBarInfo);
	void	LoadPascal(long PascalArray[], int n);
	double	DiffAbs(const double c, const double d);
	void	PertFunctions(Complex *XRef, Complex *DeltaSubN, Complex *DeltaSub0);
    void    BigRefFunctions(BFComplex *centre, BFComplex *Z, BFComplex *ZTimes2);
    void    RefFunctions(Complex *centre, Complex *Z, Complex *ZTimes2);
	void	CloseTheDamnPointers(void);
    void    CPolynomial(BFComplex *out, BFComplex in, int degree);
    void    CCube(BFComplex *out, BFComplex in);

	Complex *XSubN = NULL;
	double	*PerturbationToleranceCheck = NULL;
	double	calculatedRealDelta, calculatedImaginaryDelta;
	double	ZCoordinateMagnitudeSquared;
	int	    skippedIterations = 0;

	long	PascalArray[MAXPOWER];
	Point	*pointsRemaining = NULL;
	Point	*glitchPoints = NULL;
  	double	param[MAX_PARAMS];

//	Complex	q;			// location of current pixel
	Complex	rsrA;			// TheRedshiftRider value of a
	bool	rsrSign;		// TheRedshiftRider sign true if positive
    bool    IsPotential = false;

	int	    width, height;
	int	    MaxIteration;
	int	    power, subtype, biomorph; 
	int	    InsideMethod;			// the number of the inside filter
	int	    OutsideMethod;			// the number of the outside filter
	long	GlitchPointCount;
	long	RemainingPointCount;
    bf_t	xBigZoomPt, yBigZoomPt;
    double	xZoomPt, yZoomPt;
	double	ZoomRadius;
    double  xCentre, yCentre;
	bool	calculateGlitches = true;
	bool	seriesApproximation = false;
	unsigned int numCoefficients = 5;
	//What percentage of the image is okay to be glitched. 
	double	percentGlitchTolerance = 0.1;
	int	    referencePoints = 0;
    int     kbdchar;            // keyboard character
    int     saved;              // keep track of bigflt memory
    bf_math_type math_type;
    };


