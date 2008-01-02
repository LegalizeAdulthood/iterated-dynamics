#pragma once

#include <boost/math/quaternion.hpp>

typedef boost::math::quaternion<double> QuaternionD;

extern QuaternionD g_c_quaternion;

extern int quaternion_orbit_fp();
extern int quaternion_per_pixel_fp();
extern int quaternion_julia_per_pixel_fp();
