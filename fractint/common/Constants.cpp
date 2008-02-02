#include "port.h"
#include "id.h"

/* Assumed overall screen dimensions, y/x  */
float const DEFAULT_ASPECT_RATIO = 0.75f;

/* drift of < 2% is forced to 0% */
float const DEFAULT_ASPECT_DRIFT = 0.02f;

char const *INFO_ID = "Fractal";

double const AUTO_INVERT  = -123456.789;

char const *DEFAULTFRACTALTYPE = ".gif";
char const *ALTERNATEFRACTALTYPE = ".fra";
