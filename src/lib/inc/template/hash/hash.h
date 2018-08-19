
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

#if !defined(HASHTYPE)
#error HASHTYPE not set
#endif

// Extra layers of indirection needed
// to get HASHKIND concatenation.
#define __HASHKIND(x,y) x##y
#define _HASHKIND(x,y) __HASHKIND(x,y)
#define HASHKIND _HASHKIND(hash,HASHTYPE)

// Hash macro which return a digest
// from a single hashing iteration.
// It is meant to be used iteratively
// whereby the seed is the previously
// returned digest.
// Hence the bitsize of the seed
// is the bitsize of result.
// The hashing iteration is done using
// a right bit-rotation because when
// the seed is zero, it is more likely
// to yield non-zero most-significant bits
// in the result faster than it would have
// been the case with a left bit-rotation.
#define HASH(SEED, DATA) (BITROR((SEED+DATA), ((uint){1})))

// Hash function which take a seed as argument
// (which could be a previous hash) and
// return a digest of the memory region given
// through the argument ptr; hashing a byte
// at a time for the count of bytes
// given through the argument sz.
// Null is returned if the argument sz is null.
HASHTYPE HASHKIND (HASHTYPE seed, u8* ptr, uint sz) {
	
	if (!sz) return seed;
	
	while (1) {
		
		seed = HASH(seed, *ptr);
		
		if (--sz) ++ptr; else break;
	}
	
	// Hash the seed with itself for
	// a count of 8*sizeof(HASHTYPE).
	
	ptr = &((u8*)&seed)[sizeof(HASHTYPE)-1];
	
	sz = 8*sizeof(HASHTYPE);
	
	do seed = HASH(seed, *ptr); while (--sz);
	
	return seed;
}

#define __HASHKIND_(x,y) x##y
#define _HASHKIND_(x,y) __HASHKIND_(x,y)
#define HASHKIND_ _HASHKIND_(HASHKIND,_)

// Same as HASHKIND (HASHTYPE seed, u8* ptr, uint sz),
// but hash again the memory region given through
// the arguments ptr and sz for additional
// iterations given by the argument iterationcnt;
// in other words when the argument iterationcnt
// is null, the memory region is hashed once only.
HASHTYPE HASHKIND_ (HASHTYPE seed, u8* ptr, uint sz, uint iterationcnt) {
	
	if (!sz) return seed;
	
	while (1) {
		
		u8* p = ptr;
		
		uint i = sz;
		
		while (1) {
			
			seed = HASH(seed, *p);
			
			if (--i) ++p; else break;
		}
		
		if (!iterationcnt) break;
		
		--iterationcnt;
	}
	
	// Hash the seed with itself for
	// a count of 8*sizeof(HASHTYPE).
	
	ptr = &((u8*)&seed)[sizeof(HASHTYPE)-1];
	
	sz = 8*sizeof(HASHTYPE);
	
	do seed = HASH(seed, *ptr); while (--sz);
	
	return seed;
}

#define __HASHKIND__(x,y) x##y
#define _HASHKIND__(x,y) __HASHKIND__(x,y)
#define HASHKIND__ _HASHKIND__(HASHKIND,__)

// Similar to HASHKIND (HASHTYPE seed, u8* ptr, uint sz),
// but also generate a nonce which get hashed
// before the memory region described by
// the arguments ptr and sz.
// The memory region pointed by the argument nonce
// must be valid with an initial value from which
// the nonce will be computed; in fact there can
// be multiple valid nonce for the same noncestrength.
// The argument noncestrength is the number of
// least significant bits that must be zero in
// the hash result for the nonce to be acceptable.
HASHTYPE HASHKIND__ (HASHTYPE seed, u8* ptr, uint sz, HASHTYPE* nonce, uint noncestrength) {
	
	if (!sz) return seed;
	
	while (1) {
		// Hash the nonce.
		
		u8* p = (u8*)nonce;
		
		uint i = sizeof(HASHTYPE);
		
		while (1) {
			
			seed = HASH(seed, *p);
			
			if (--i) ++p; else break;
		}
		
		// Hash the memory region described
		// by the arguments ptr and sz.
		
		p = ptr;
		
		i = sz;
		
		while (1) {
			
			seed = HASH(seed, *p);
			
			if (--i) ++p; else break;
		}
		
		// Hash the seed with itself for
		// a count of 8*sizeof(HASHTYPE).
		
		p = &((u8*)&seed)[sizeof(HASHTYPE)-1];
		
		i = 8*sizeof(HASHTYPE);
		
		do seed = HASH(seed, *p); while (--i);
		
		// Check whether there is at least a count of noncestrength
		// least significant bits that are all zeros in the hash result,
		// otherwise increment the nonce and keep hashing for another round.
		
		i = noncestrength;
		
		while (i && !((seed>>(i-1))&1)) --i;
		
		if (i) ++*nonce; else return seed;
	}
}

#define __HASHKIND___(x,y) x##y
#define _HASHKIND___(x,y) __HASHKIND___(x,y)
#define HASHKIND___ _HASHKIND___(HASHKIND,___)

// Same as HASHKIND__ (HASHTYPE seed, u8* ptr, uint sz, HASHTYPE* nonce, uint noncestrength),
// but hash again the memory region given through
// the arguments ptr and sz for additional
// iterations given by the argument iterationcnt;
// in other words when the argument iterationcnt
// is null, the memory region is hashed once only.
HASHTYPE HASHKIND___ (HASHTYPE seed, u8* ptr, uint sz, HASHTYPE* nonce, uint noncestrength, uint iterationcnt) {
	
	if (!sz) return seed;
	
	while (1) {
		// Hash the nonce.
		
		u8* p = (u8*)nonce;
		
		uint i = sizeof(HASHTYPE);
		
		while (1) {
			
			seed = HASH(seed, *p);
			
			if (--i) ++p; else break;
		}
		
		// Hash the memory region described
		// by the arguments ptr and sz.
		
		i = iterationcnt;
		
		while (1) {
			
			p = ptr;
			
			uint ii = sz;
			
			while (1) {
				
				seed = HASH(seed, *p);
				
				if (--ii) ++p; else break;
			}
			
			if (!i) break;
			
			--i;
		}
		
		// Hash the seed with itself for
		// a count of 8*sizeof(HASHTYPE).
		
		p = &((u8*)&seed)[sizeof(HASHTYPE)-1];
		
		i = 8*sizeof(HASHTYPE);
		
		do seed = HASH(seed, *p); while (--i);
		
		// Check whether there is at least a count of noncestrength
		// least significant bits that are all zeros in the hash result,
		// otherwise increment the nonce and keep hashing for another round.
		
		i = noncestrength;
		
		while (i && !((seed>>(i-1))&1)) --i;
		
		if (i) ++*nonce; else return seed;
	}
}
