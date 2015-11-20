/*
 * DIPlib 3.0
 * This file contains numeric algorithms unrelated to images.
 *
 * (c)2015, Cris Luengo.
 * Based on original DIPlib code: (c)1995-2014, Delft University of Technology.
 */

#include "dip_types.h"
#include "dip_numeric.h"

// While std::experimental::gcd is not in the standard...
dip::uint dip::gcd( dip::uint a, dip::uint b ) {
   return b == 0 ? a : dip::gcd( b, a % b );
}