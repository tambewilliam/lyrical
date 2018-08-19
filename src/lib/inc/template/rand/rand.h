
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


#include <stdtypes.h>
#include <macros.h>

#if !defined(RANDKIND)
#error RANDKIND not set
#endif

#if !defined(RANDPOLYNOMIAL)
#error RANDPOLYNOMIAL not set
#endif

// Extra layers of indirection needed
// to get RANDTYPE concatenation.
#define __RANDTYPE(x,y) x##y
#define _RANDTYPE(x,y) __RANDTYPE(x,y)
#define RANDTYPE _RANDTYPE(rand,RANDKIND)

// Return a pseudo-random integer
// in the mathematical range
// [0, ((1<<(8*sizeof(RANDKIND))) -1)]
// when the argument seed is null,
// otherwise the argument seed is used
// as the seed for a new sequence of
// pseudo-random integers to be returned
// when the argument seed is null.
RANDKIND RANDTYPE (RANDKIND seed) {
	// Function which implement
	// Linear-Feedback Shift Register.
	RANDKIND lfsr (RANDKIND in) {
		
		RANDKIND lsb = in&1;
		
		in >>= 1;
		
		// If the output bit was 1, apply
		// the RANDPOLYNOMIAL bitmask.
		return (in ^ ((-lsb) & RANDPOLYNOMIAL));
	}
	
	// Seed of the pseudo-random sequence.
	static RANDKIND s;
	
	// Since lfsr() can never return zero,
	// these two variables are used to count
	// to when zero will be returned.
	// The count is initialized in z using
	// the lfsr() of the pseudo-random seed
	// so as to not be obvious.
	static RANDKIND z;
	static RANDKIND i; // Count-up to z.
	
	// Variable used to save the current
	// pseudo-random integer, in order
	// to generate the next pseudo-random
	// integer in the sequence.
	static RANDKIND n;
	
	if (seed) {
		
		s = seed;
		
		reseed:
		
		z = lfsr(s);
		i = 0;
		
		n = z;
		
	} else if (n == z && i > 1) {
		// If I get here, all the pseudo-random numbers
		// of the sequence were returned; I re-seed
		// the pseudo-random sequence effectively
		// implementing automatic re-seed
		// when the sequence has been used-up.
		s = ~z;
		
		goto reseed;
	}
	
	RANDKIND retvar;
	
	if (++i == z) retvar = 0;
	else {
		n = lfsr(n);
		
		// BITROL() is used so that
		// the pseudo-random sequence
		// be dependent on the seed, and
		// so that it be non-obvious tracking
		// back to the original sequence.
		// ### Note that BITROL is a macro,
		// and its arguments must be variables
		// not expressions, otherwise those
		// expressions will be evaluated
		// multiple times.
		retvar = BITROL(n, z);
	}
	
	return retvar;
}
