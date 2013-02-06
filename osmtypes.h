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

#ifndef OSMTYPES_H
#define OSMTYPES_H

#include <stdint.h>

/* Should be enough to hold max id for any osm object */
typedef int64_t osmid_t;

/* Holds fixed point coordinate in range [-1'800'000'000; 1'800'000'000]
 * 32 bit is not enough to hold difference between coordinates so
 * convertsion to osmcoord_long_t is required in that case;
 * This is also used for storing height with 1mm resolution */
typedef signed int osmint_t;

/* Useful for results of operations on osmcoord_t which won't stick
 * into osmcoord_t, such as difference between points and vector
 * operations involving multiplication */
typedef signed long long osmlong_t;

#endif
