/*
 * Copyright (C) 2012 Dmitry Marakasov
 *
 * This file is part of rail routing demo.
 *
 * rail routing demo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rail routing demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rail routing demo.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEOMATH_HH
#define GEOMATH_HH

#include <cmath>

#include "ObjectBases.hh"

static inline double Distance(const LonLat& a, const LonLat& b) {
	const static double earth_r = 6378137.0;

	double alon = a.GetLonD() / 180.0 * M_PI;
	double alat = a.GetLatD() / 180.0 * M_PI;

	double blon = b.GetLonD() / 180.0 * M_PI;
	double blat = b.GetLatD() / 180.0 * M_PI;

	double sin1 = sin((blat - alat) / 2.0);
	double sin2 = sin((blon - alon) / 2.0);

	return 2.0 * earth_r * asin(sqrt(sin1 * sin1 + cos(alat) * cos(blat) * sin2 * sin2));
}

static inline double Bearing(const LonLat& a, const LonLat& b) {
	double alat = a.GetLatD() / 180.0 * M_PI;
	double blat = b.GetLatD() / 180.0 * M_PI;

	double dlon = (b.GetLonD() - a.GetLonD()) / 180.0 * M_PI;

	double y = sin(dlon) * cos(blat);
	double x = cos(alat) * sin(blat) - sin(alat) * cos(blat) * cos(dlon);

	return atan2(y, x);
}

#endif
