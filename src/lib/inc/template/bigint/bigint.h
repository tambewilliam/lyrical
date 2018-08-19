
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


// Integer arbitrary-precision arithmetic library.

// This library can only work on a little-endian processor.
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error little-endian processor required
#endif

#include <stdtypes.h>
#include <string.h>

// The precision is set through the macro BIGINTBITSZ
// which when defined generate the corresponding bigint.

// The bigint and machine-word bitsize are
// manually set when BIGINTBITSZ is not defined;
// where BIGINTWBITSZ is the machine-word bitsize.
#if defined(BIGINTBITSZ)
#if !defined(BIGINTWBITSZ)
// Due to GCC not providing a macro which
// expand to the bitsize of a pointer,
// these successive checks are used.
#if __SIZEOF_POINTER__ == 1 || BIGINTBITSZ == 8
#define BIGINTWBITSZ 8
#elif __SIZEOF_POINTER__ == 2 || BIGINTBITSZ == 16
#define BIGINTWBITSZ 16
#elif __SIZEOF_POINTER__ == 4 || BIGINTBITSZ == 32
#define BIGINTWBITSZ 32
#elif __SIZEOF_POINTER__ == 8 || BIGINTBITSZ == 64
#define BIGINTWBITSZ 64
#else
#error unsupported machine-word bitsize.
#endif
#endif // !defined(BIGINTWBITSZ)
#else // defined(BIGINTBITSZ)
// When set manually, BIGINTBITSZ must
// be a power of 2 value greater than
// or equal to the machine-word bitsize.
//#define BIGINTBITSZ 32
//#define BIGINTWBITSZ 8
#error BIGINTBITSZ not set
#endif // defined(BIGINTBITSZ)

// BIGINTWBITSZ must be less than or
// equal to the actual machine-word bitsize.
#if BIGINTWBITSZ > (8*__SIZEOF_POINTER__)
#error invalid machine-word bitsize
#endif

// BIGINTBITSZ must be a power
// of 2 value greater than or equal
// to the machine-word bitsize.
#if BIGINTBITSZ < BIGINTWBITSZ
#error invalid machine-word bitsize
#endif

// The machine-word bitsize should
// be large enough to represent
// a bigint bitsize value.
// ### Commented out for now
// ### due to GCC overflowing when
// ### computing (1<<BIGINTWBITSZ) .
//#if BIGINTBITSZ >= (1<<BIGINTWBITSZ)
//#error machine-word bitsize too small
//#endif

// Extra layers of indirection needed
// to get BIGINTTYPE concatenation,
// which is the bigint type.
#define __BIGINTTYPE(x,y) x##y
#define _BIGINTTYPE(x,y) __BIGINTTYPE(x,y)
#define BIGINTTYPE _BIGINTTYPE(i,BIGINTBITSZ)

// Extra layers of indirection needed
// to get BIGINTWSTYPE concatenation,
// which is the signed type which
// fit the machine-word.
#define __BIGINTWSTYPE(x,y) x##y
#define _BIGINTWSTYPE(x,y) __BIGINTWSTYPE(x,y)
#define BIGINTWSTYPE _BIGINTWSTYPE(s,BIGINTWBITSZ)

// Extra layers of indirection needed
// to get BIGINTWUTYPE concatenation,
// which is the unsigned type which
// fit the machine-word.
#define __BIGINTWUTYPE(x,y) x##y
#define _BIGINTWUTYPE(x,y) __BIGINTWUTYPE(x,y)
#define BIGINTWUTYPE _BIGINTWUTYPE(u,BIGINTWBITSZ)

// Extra layers of indirection needed
// to get BIGINTTOUINT concatenation.
#define __BIGINTTOUINT(x,y,z) x##y##z
#define _BIGINTTOUINT(x,y,z) __BIGINTTOUINT(x,y,z)
#define BIGINTTOUINT _BIGINTTOUINT(i,BIGINTBITSZ,touint)

// Extra layers of indirection needed
// to get BIGINTTOSINT concatenation.
#define __BIGINTTOSINT(x,y,z) x##y##z
#define _BIGINTTOSINT(x,y,z) __BIGINTTOSINT(x,y,z)
#define BIGINTTOSINT _BIGINTTOSINT(i,BIGINTBITSZ,tosint)

// Extra layers of indirection needed
// to get BIGINTFROMUINT concatenation.
#define __BIGINTFROMUINT(x,y,z) x##y##z
#define _BIGINTFROMUINT(x,y,z) __BIGINTFROMUINT(x,y,z)
#define BIGINTFROMUINT _BIGINTFROMUINT(i,BIGINTBITSZ,fromuint)

// Extra layers of indirection needed
// to get BIGINTFROMSINT concatenation.
#define __BIGINTFROMSINT(x,y,z) x##y##z
#define _BIGINTFROMSINT(x,y,z) __BIGINTFROMSINT(x,y,z)
#define BIGINTFROMSINT _BIGINTFROMSINT(i,BIGINTBITSZ,fromsint)

// Extra layers of indirection needed
// to get BIGINTMAXDECDIGITS concatenation.
#define __BIGINTMAXDECDIGITS(x,y,z) x##y##z
#define _BIGINTMAXDECDIGITS(x,y,z) __BIGINTMAXDECDIGITS(x,y,z)
#define BIGINTMAXDECDIGITS _BIGINTMAXDECDIGITS(I,BIGINTBITSZ,MAXDECDIGITS)

// Extra layers of indirection needed
// to get BIGINTMAXHEXDIGITS concatenation.
#define __BIGINTMAXHEXDIGITS(x,y,z) x##y##z
#define _BIGINTMAXHEXDIGITS(x,y,z) __BIGINTMAXHEXDIGITS(x,y,z)
#define BIGINTMAXHEXDIGITS _BIGINTMAXHEXDIGITS(I,BIGINTBITSZ,MAXHEXDIGITS)

// Extra layers of indirection needed
// to get BIGINTMAXOCTDIGITS concatenation.
#define __BIGINTMAXOCTDIGITS(x,y,z) x##y##z
#define _BIGINTMAXOCTDIGITS(x,y,z) __BIGINTMAXOCTDIGITS(x,y,z)
#define BIGINTMAXOCTDIGITS _BIGINTMAXOCTDIGITS(I,BIGINTBITSZ,MAXOCTDIGITS)

// Extra layers of indirection needed
// to get BIGINTMAXBINDIGITS concatenation.
#define __BIGINTMAXBINDIGITS(x,y,z) x##y##z
#define _BIGINTMAXBINDIGITS(x,y,z) __BIGINTMAXBINDIGITS(x,y,z)
#define BIGINTMAXBINDIGITS _BIGINTMAXBINDIGITS(I,BIGINTBITSZ,MAXBINDIGITS)

// Extra layers of indirection needed
// to get BIGINTZERO concatenation.
#define __BIGINTZERO(x,y,z) x##y##z
#define _BIGINTZERO(x,y,z) __BIGINTZERO(x,y,z)
#define BIGINTZERO _BIGINTZERO(i,BIGINTBITSZ,zero)

// Extra layers of indirection needed
// to get BIGINTONE concatenation.
#define __BIGINTONE(x,y,z) x##y##z
#define _BIGINTONE(x,y,z) __BIGINTONE(x,y,z)
#define BIGINTONE _BIGINTONE(i,BIGINTBITSZ,one)

// Extra layers of indirection needed
// to get BIGINTNEGONE concatenation.
#define __BIGINTNEGONE(x,y,z) x##y##z
#define _BIGINTNEGONE(x,y,z) __BIGINTNEGONE(x,y,z)
#define BIGINTNEGONE _BIGINTNEGONE(i,BIGINTBITSZ,negone)

// Extra layers of indirection needed
// to get BIGINTTWO concatenation.
#define __BIGINTTWO(x,y,z) x##y##z
#define _BIGINTTWO(x,y,z) __BIGINTTWO(x,y,z)
#define BIGINTTWO _BIGINTTWO(i,BIGINTBITSZ,two)

// Extra layers of indirection needed
// to get BIGINTTHREE concatenation.
#define __BIGINTTHREE(x,y,z) x##y##z
#define _BIGINTTHREE(x,y,z) __BIGINTTHREE(x,y,z)
#define BIGINTTHREE _BIGINTTHREE(i,BIGINTBITSZ,three)

// Extra layers of indirection needed
// to get BIGINTFOUR concatenation.
#define __BIGINTFOUR(x,y,z) x##y##z
#define _BIGINTFOUR(x,y,z) __BIGINTFOUR(x,y,z)
#define BIGINTFOUR _BIGINTFOUR(i,BIGINTBITSZ,four)

// Extra layers of indirection needed
// to get BIGINTEIGHT concatenation.
#define __BIGINTEIGHT(x,y,z) x##y##z
#define _BIGINTEIGHT(x,y,z) __BIGINTEIGHT(x,y,z)
#define BIGINTEIGHT _BIGINTEIGHT(i,BIGINTBITSZ,eight)

// Extra layers of indirection needed
// to get BIGINTTEN concatenation.
#define __BIGINTTEN(x,y,z) x##y##z
#define _BIGINTTEN(x,y,z) __BIGINTTEN(x,y,z)
#define BIGINTTEN _BIGINTTEN(i,BIGINTBITSZ,ten)

// Extra layers of indirection needed
// to get BIGINTSIXTEEN concatenation.
#define __BIGINTSIXTEEN(x,y,z) x##y##z
#define _BIGINTSIXTEEN(x,y,z) __BIGINTSIXTEEN(x,y,z)
#define BIGINTSIXTEEN _BIGINTSIXTEEN(i,BIGINTBITSZ,sixteen)

// Extra layers of indirection needed
// to get BIGINTFROMUINTBIGINTBITSZ concatenation.
#define __BIGINTFROMUINTBIGINTBITSZ(x,y,z) x##y##z
#define _BIGINTFROMUINTBIGINTBITSZ(x,y,z) __BIGINTFROMUINTBIGINTBITSZ(x,y,z)
#define BIGINTFROMUINTBIGINTBITSZ _BIGINTFROMUINTBIGINTBITSZ(i,BIGINTBITSZ,bigintbitsz)

// Extra layers of indirection needed
// to get BIGINTFROMUINTBIGINTWBITSZ concatenation.
#define __BIGINTFROMUINTBIGINTWBITSZ(x,y,z) x##y##z
#define _BIGINTFROMUINTBIGINTWBITSZ(x,y,z) __BIGINTFROMUINTBIGINTWBITSZ(x,y,z)
#define BIGINTFROMUINTBIGINTWBITSZ _BIGINTFROMUINTBIGINTWBITSZ(i,BIGINTBITSZ,bigintwbitsz)

// Extra layers of indirection needed
// to get BIGINTCTOR concatenation.
#define __BIGINTCTOR(x,y,z) x##y##z
#define _BIGINTCTOR(x,y,z) __BIGINTCTOR(x,y,z)
#define BIGINTCTOR _BIGINTCTOR(i,BIGINTBITSZ,ctor)

// Extra layers of indirection needed
// to get BIGINTADD concatenation.
#define __BIGINTADD(x,y,z) x##y##z
#define _BIGINTADD(x,y,z) __BIGINTADD(x,y,z)
#define BIGINTADD _BIGINTADD(i,BIGINTBITSZ,add)

// Extra layers of indirection needed
// to get BIGINTNOT concatenation.
#define __BIGINTNOT(x,y,z) x##y##z
#define _BIGINTNOT(x,y,z) __BIGINTNOT(x,y,z)
#define BIGINTNOT _BIGINTNOT(i,BIGINTBITSZ,not)

// Extra layers of indirection needed
// to get BIGINTNEG concatenation.
#define __BIGINTNEG(x,y,z) x##y##z
#define _BIGINTNEG(x,y,z) __BIGINTNEG(x,y,z)
#define BIGINTNEG _BIGINTNEG(i,BIGINTBITSZ,neg)

// Extra layers of indirection needed
// to get BIGINTSUB concatenation.
#define __BIGINTSUB(x,y,z) x##y##z
#define _BIGINTSUB(x,y,z) __BIGINTSUB(x,y,z)
#define BIGINTSUB _BIGINTSUB(i,BIGINTBITSZ,sub)

// Extra layers of indirection needed
// to get BIGINTMULRESULT concatenation.
#define __BIGINTMULRESULT(x,y,z) x##y##z
#define _BIGINTMULRESULT(x,y,z) __BIGINTMULRESULT(x,y,z)
#define BIGINTMULRESULT _BIGINTMULRESULT(i,BIGINTBITSZ,mulresult)

// Extra layers of indirection needed
// to get BIGINTMULU concatenation.
#define __BIGINTMULU(x,y,z) x##y##z
#define _BIGINTMULU(x,y,z) __BIGINTMULU(x,y,z)
#define BIGINTMULU _BIGINTMULU(i,BIGINTBITSZ,mulu)

// Extra layers of indirection needed
// to get BIGINTMUL concatenation.
#define __BIGINTMUL(x,y,z) x##y##z
#define _BIGINTMUL(x,y,z) __BIGINTMUL(x,y,z)
#define BIGINTMUL _BIGINTMUL(i,BIGINTBITSZ,mul)

// Extra layers of indirection needed
// to get BIGINTDIVRESULT concatenation.
#define __BIGINTDIVRESULT(x,y,z) x##y##z
#define _BIGINTDIVRESULT(x,y,z) __BIGINTDIVRESULT(x,y,z)
#define BIGINTDIVRESULT _BIGINTDIVRESULT(i,BIGINTBITSZ,divresult)

// Extra layers of indirection needed
// to get BIGINTDIVU concatenation.
#define __BIGINTDIVU(x,y,z) x##y##z
#define _BIGINTDIVU(x,y,z) __BIGINTDIVU(x,y,z)
#define BIGINTDIVU _BIGINTDIVU(i,BIGINTBITSZ,divu)

// Extra layers of indirection needed
// to get BIGINTDIV concatenation.
#define __BIGINTDIV(x,y,z) x##y##z
#define _BIGINTDIV(x,y,z) __BIGINTDIV(x,y,z)
#define BIGINTDIV _BIGINTDIV(i,BIGINTBITSZ,div)

// Extra layers of indirection needed
// to get BIGINTSNZ concatenation.
#define __BIGINTSNZ(x,y,z) x##y##z
#define _BIGINTSNZ(x,y,z) __BIGINTSNZ(x,y,z)
#define BIGINTSNZ _BIGINTSNZ(i,BIGINTBITSZ,snz)

// Extra layers of indirection needed
// to get BIGINTSEQ concatenation.
#define __BIGINTSEQ(x,y,z) x##y##z
#define _BIGINTSEQ(x,y,z) __BIGINTSEQ(x,y,z)
#define BIGINTSEQ _BIGINTSEQ(i,BIGINTBITSZ,seq)

// Extra layers of indirection needed
// to get BIGINTSNE concatenation.
#define __BIGINTSNE(x,y,z) x##y##z
#define _BIGINTSNE(x,y,z) __BIGINTSNE(x,y,z)
#define BIGINTSNE _BIGINTSNE(i,BIGINTBITSZ,sne)

// Extra layers of indirection needed
// to get BIGINTSLT concatenation.
#define __BIGINTSLT(x,y,z) x##y##z
#define _BIGINTSLT(x,y,z) __BIGINTSLT(x,y,z)
#define BIGINTSLT _BIGINTSLT(i,BIGINTBITSZ,slt)

// Extra layers of indirection needed
// to get BIGINTSLTE concatenation.
#define __BIGINTSLTE(x,y,z) x##y##z
#define _BIGINTSLTE(x,y,z) __BIGINTSLTE(x,y,z)
#define BIGINTSLTE _BIGINTSLTE(i,BIGINTBITSZ,slte)

// Extra layers of indirection needed
// to get BIGINTSLTU concatenation.
#define __BIGINTSLTU(x,y,z) x##y##z
#define _BIGINTSLTU(x,y,z) __BIGINTSLTU(x,y,z)
#define BIGINTSLTU _BIGINTSLTU(i,BIGINTBITSZ,sltu)

// Extra layers of indirection needed
// to get BIGINTSLTEU concatenation.
#define __BIGINTSLTEU(x,y,z) x##y##z
#define _BIGINTSLTEU(x,y,z) __BIGINTSLTEU(x,y,z)
#define BIGINTSLTEU _BIGINTSLTEU(i,BIGINTBITSZ,slteu)

// Extra layers of indirection needed
// to get BIGINTAND concatenation.
#define __BIGINTAND(x,y,z) x##y##z
#define _BIGINTAND(x,y,z) __BIGINTAND(x,y,z)
#define BIGINTAND _BIGINTAND(i,BIGINTBITSZ,and)

// Extra layers of indirection needed
// to get BIGINTOR concatenation.
#define __BIGINTOR(x,y,z) x##y##z
#define _BIGINTOR(x,y,z) __BIGINTOR(x,y,z)
#define BIGINTOR _BIGINTOR(i,BIGINTBITSZ,or)

// Extra layers of indirection needed
// to get BIGINTXOR concatenation.
#define __BIGINTXOR(x,y,z) x##y##z
#define _BIGINTXOR(x,y,z) __BIGINTXOR(x,y,z)
#define BIGINTXOR _BIGINTXOR(i,BIGINTBITSZ,xor)

// Extra layers of indirection needed
// to get BIGINTSLL concatenation.
#define __BIGINTSLL(x,y,z) x##y##z
#define _BIGINTSLL(x,y,z) __BIGINTSLL(x,y,z)
#define BIGINTSLL _BIGINTSLL(i,BIGINTBITSZ,sll)

// Extra layers of indirection needed
// to get BIGINTSRL concatenation.
#define __BIGINTSRL(x,y,z) x##y##z
#define _BIGINTSRL(x,y,z) __BIGINTSRL(x,y,z)
#define BIGINTSRL _BIGINTSRL(i,BIGINTBITSZ,srl)

// Extra layers of indirection needed
// to get BIGINTSRA concatenation.
#define __BIGINTSRA(x,y,z) x##y##z
#define _BIGINTSRA(x,y,z) __BIGINTSRA(x,y,z)
#define BIGINTSRA _BIGINTSRA(i,BIGINTBITSZ,sra)

// Extra layers of indirection needed
// to get BIGINTFROMSTR_ concatenation.
#define __BIGINTFROMSTR_(x,y,z) x##y##z
#define _BIGINTFROMSTR_(x,y,z) __BIGINTFROMSTR_(x,y,z)
#define BIGINTFROMSTR_ _BIGINTFROMSTR_(i,BIGINTBITSZ,fromstr_)

// Extra layers of indirection needed
// to get BIGINTFROMSTR concatenation.
#define __BIGINTFROMSTR(x,y,z) x##y##z
#define _BIGINTFROMSTR(x,y,z) __BIGINTFROMSTR(x,y,z)
#define BIGINTFROMSTR _BIGINTFROMSTR(i,BIGINTBITSZ,fromstr)

// Extra layers of indirection needed
// to get BIGINTTODECSTR_ concatenation.
#define __BIGINTTODECSTR_(x,y,z) x##y##z
#define _BIGINTTODECSTR_(x,y,z) __BIGINTTODECSTR_(x,y,z)
#define BIGINTTODECSTR_ _BIGINTTODECSTR_(i,BIGINTBITSZ,todecstr_)

// Extra layers of indirection needed
// to get BIGINTTODECSTR concatenation.
#define __BIGINTTODECSTR(x,y,z) x##y##z
#define _BIGINTTODECSTR(x,y,z) __BIGINTTODECSTR(x,y,z)
#define BIGINTTODECSTR _BIGINTTODECSTR(i,BIGINTBITSZ,todecstr)

// Extra layers of indirection needed
// to get BIGINTTOHEXSTR_ concatenation.
#define __BIGINTTOHEXSTR_(x,y,z) x##y##z
#define _BIGINTTOHEXSTR_(x,y,z) __BIGINTTOHEXSTR_(x,y,z)
#define BIGINTTOHEXSTR_ _BIGINTTOHEXSTR_(i,BIGINTBITSZ,tohexstr_)

// Extra layers of indirection needed
// to get BIGINTTOHEXSTR concatenation.
#define __BIGINTTOHEXSTR(x,y,z) x##y##z
#define _BIGINTTOHEXSTR(x,y,z) __BIGINTTOHEXSTR(x,y,z)
#define BIGINTTOHEXSTR _BIGINTTOHEXSTR(i,BIGINTBITSZ,tohexstr)

// Extra layers of indirection needed
// to get BIGINTTOOCTSTR_ concatenation.
#define __BIGINTTOOCTSTR_(x,y,z) x##y##z
#define _BIGINTTOOCTSTR_(x,y,z) __BIGINTTOOCTSTR_(x,y,z)
#define BIGINTTOOCTSTR_ _BIGINTTOOCTSTR_(i,BIGINTBITSZ,tooctstr_)

// Extra layers of indirection needed
// to get BIGINTTOOCTSTR concatenation.
#define __BIGINTTOOCTSTR(x,y,z) x##y##z
#define _BIGINTTOOCTSTR(x,y,z) __BIGINTTOOCTSTR(x,y,z)
#define BIGINTTOOCTSTR _BIGINTTOOCTSTR(i,BIGINTBITSZ,tooctstr)

// Extra layers of indirection needed
// to get BIGINTTOBINSTR_ concatenation.
#define __BIGINTTOBINSTR_(x,y,z) x##y##z
#define _BIGINTTOBINSTR_(x,y,z) __BIGINTTOBINSTR_(x,y,z)
#define BIGINTTOBINSTR_ _BIGINTTOBINSTR_(i,BIGINTBITSZ,tobinstr_)

// Extra layers of indirection needed
// to get BIGINTTOBINSTR concatenation.
#define __BIGINTTOBINSTR(x,y,z) x##y##z
#define _BIGINTTOBINSTR(x,y,z) __BIGINTTOBINSTR(x,y,z)
#define BIGINTTOBINSTR _BIGINTTOBINSTR(i,BIGINTBITSZ,tobinstr)

#define BIGINTWORDSZ (BIGINTBITSZ/BIGINTWBITSZ)


// Enums resolving to the number of digits
// needed for the encoding of the largest
// bigint value, respectively to its decimal,
// hexadecimal, octale and binary notations.
enum {
	BIGINTMAXDECDIGITS = (BIGINTBITSZ/4),
	BIGINTMAXHEXDIGITS = (BIGINTBITSZ/4),
	BIGINTMAXOCTDIGITS = (BIGINTBITSZ/3),
	BIGINTMAXBINDIGITS = BIGINTBITSZ
};


// Structure represeting a bigint.
typedef struct {
	BIGINTWUTYPE w[BIGINTWORDSZ];
} BIGINTTYPE;


// Convert a bigint to its equivalent uint.
static uint BIGINTTOUINT (BIGINTTYPE arg) {
	
	uint result = 0;
	
	uint i = 0;
	
	do {
		uint shiftamount = i*BIGINTWBITSZ;
		
		result |= (arg.w[i] << shiftamount);
		
	} while (++i < BIGINTWORDSZ);
	
	return result;
}


// Convert a bigint to its equivalent sint.
static sint BIGINTTOSINT (BIGINTTYPE arg) {
	
	sint result = BIGINTTOUINT(arg);
	
	if (BIGINTBITSZ < (8*sizeof(uint))) {
		// Sign extend the value to return.
		uint shiftamount = ((8*sizeof(uint)) - BIGINTBITSZ);
		result <<= shiftamount;
		result >>= shiftamount; // Arithmetic right shift will be done.
	}
	
	return result;
}


// Convert a uint to its equivalent bigint.
static BIGINTTYPE BIGINTFROMUINT (uint arg) {
	
	BIGINTTYPE result;
	
	uint i = 0;
	
	do {
		uint shiftamount = i*BIGINTWBITSZ;
		
		result.w[i] = ((shiftamount<(8*sizeof(uint))) ? (arg >> shiftamount) : 0); // Logical right shift will be done.
		
	} while (++i < BIGINTWORDSZ);
	
	return result;
}


// Convert a sint to its equivalent bigint.
static BIGINTTYPE BIGINTFROMSINT (sint arg) {
	
	BIGINTTYPE result;
	
	// Value to use with the most significant bytes.
	sint msb = (arg >> ((8*sizeof(uint))-1)); // Arithmetic right shift will be done.
	
	uint i = 0;
	
	do {
		uint shiftamount = i*BIGINTWBITSZ;
		
		result.w[i] = ((shiftamount<(8*sizeof(uint))) ? (arg >> shiftamount) : msb); // Arithmetic right shift will be done.
		
	} while (++i < BIGINTWORDSZ);
	
	return result;
}


static BIGINTTYPE BIGINTZERO; // bigint for 0.
static BIGINTTYPE BIGINTONE; // bigint for 1.
static BIGINTTYPE BIGINTNEGONE; // bigint for -1.
static BIGINTTYPE BIGINTTWO; // bigint for 2.
static BIGINTTYPE BIGINTTHREE; // bigint for 3.
static BIGINTTYPE BIGINTFOUR; // bigint for 4.
static BIGINTTYPE BIGINTEIGHT; // bigint for 8.
static BIGINTTYPE BIGINTTEN; // bigint for 10.
static BIGINTTYPE BIGINTSIXTEEN; // bigint for 16.
static BIGINTTYPE BIGINTFROMUINTBIGINTBITSZ; // bigint for BIGINTBITSZ.
static BIGINTTYPE BIGINTFROMUINTBIGINTWBITSZ; // bigint for BIGINTWBITSZ.

// Function which will be automatically
// called by GCC before main().
__attribute__ ((__constructor__))
static void BIGINTCTOR () {
	
	BIGINTZERO = BIGINTFROMUINT(0);
	
	BIGINTONE = BIGINTFROMUINT(1);
	
	BIGINTNEGONE = BIGINTFROMSINT(-1);
	
	BIGINTTWO = BIGINTFROMUINT(2);
	
	BIGINTTHREE = BIGINTFROMUINT(3);
	
	BIGINTFOUR = BIGINTFROMUINT(4);
	
	BIGINTEIGHT = BIGINTFROMUINT(8);
	
	BIGINTTEN = BIGINTFROMUINT(10);
	
	BIGINTSIXTEEN = BIGINTFROMUINT(16);
	
	BIGINTFROMUINTBIGINTBITSZ = BIGINTFROMUINT(BIGINTBITSZ);
	
	BIGINTFROMUINTBIGINTWBITSZ = BIGINTFROMUINT(BIGINTWBITSZ);
}


// Addition.
static BIGINTTYPE BIGINTADD (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTTYPE result;
	
	BIGINTWUTYPE carry = 0;
	
	uint i = 0;
	
	do {
		BIGINTWUTYPE arg1wi = arg1.w[i];
		
		BIGINTWUTYPE arg2wi = arg2.w[i];
		
		carry = (((result.w[i] = (carry + arg1wi + arg2wi)) <= arg1wi) & ((carry > 0) | (arg2wi > 0)));
		
	} while (++i < BIGINTWORDSZ);
	
	return result;
}


// Bitwise not.
static BIGINTTYPE BIGINTNOT (BIGINTTYPE arg) {
	
	uint i = 0;
	
	do arg.w[i] = ~arg.w[i];
	while (++i < BIGINTWORDSZ);
	
	return arg;
}


// Negation.
static BIGINTTYPE BIGINTNEG (BIGINTTYPE arg) {
	return BIGINTADD(BIGINTNOT(arg), BIGINTONE);
}


// Substraction.
static BIGINTTYPE BIGINTSUB (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	return BIGINTADD(arg1, BIGINTNEG(arg2));
}


typedef struct {
	BIGINTTYPE l, h;
} BIGINTMULRESULT;

// Unsigned multiplication.
static BIGINTMULRESULT BIGINTMULU (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	struct {
		
		union {
			
			struct {
				
				BIGINTTYPE l, h;
			};
			
			BIGINTWUTYPE w[2*BIGINTWORDSZ];
		};
		
	} result = {
		.h = BIGINTZERO,
		.l = arg1
	};
	
	uint i = 0;
	
	do {
		BIGINTWUTYPE carry = 0;
		
		// Increment result.h by arg2.
		void incrementresulthbyarg2 () {
			
			uint i = 0;
			
			do {
				BIGINTWUTYPE resulthwi = result.h.w[i];
				
				BIGINTWUTYPE arg2wi = arg2.w[i];
				
				carry = (((result.h.w[i] = (carry + resulthwi + arg2wi)) <= resulthwi) & ((carry > 0) | (arg2wi > 0)));
				
			} while (++i < BIGINTWORDSZ);
		}
		
		if (result.l.w[0] & 1) incrementresulthbyarg2();
		
		// Shift result right logical.
		void resultsrl () {
			
			enum {SHIFTAMOUNT = 1}; // Must be less than BIGINTWBITSZ.
			
			uint i = 0;
			
			while (1) {
				
				result.w[i] >>= SHIFTAMOUNT;
				
				if (i == ((2*BIGINTWORDSZ)-1)) return;
				
				uint j = i+1;
				
				result.w[i] |= (result.w[j]<<(BIGINTWBITSZ-SHIFTAMOUNT));
				
				i = j;
			}
		}
		
		resultsrl();
		
		result.h.w[BIGINTWORDSZ-1] |= (carry<<(BIGINTWBITSZ-1));
		
	} while (++i < BIGINTBITSZ);
	
	return (BIGINTMULRESULT){.h = result.h, .l = result.l};
}

// Signed multiplication.
static BIGINTMULRESULT BIGINTMUL (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	// Compute whether arg1 is negative.
	BIGINTWUTYPE isarg1negative = ((BIGINTWSTYPE)arg1.w[BIGINTWORDSZ-1] < 0);
	
	// Compute whether arg2 is negative.
	BIGINTWUTYPE isarg2negative = ((BIGINTWSTYPE)arg2.w[BIGINTWORDSZ-1] < 0);
	
	if (isarg1negative) arg1 = BIGINTNEG(arg1);
	
	if (isarg2negative) arg2 = BIGINTNEG(arg2);
	
	BIGINTMULRESULT r = BIGINTMULU(arg1, arg2);
	
	struct {
		
		union {
			
			struct {
				
				BIGINTTYPE l, h;
			};
			
			BIGINTWUTYPE w[2*BIGINTWORDSZ];
		};
		
	} result = {
		.h = r.h,
		.l = r.l
	};
	
	// Negate result.
	void resultneg () {
		// Increment result.
		void resultinc () {
			
			BIGINTWUTYPE carry = 0;
			
			uint i = 0;
			
			do {
				BIGINTWUTYPE resultwi = result.w[i];
				
				BIGINTWUTYPE isifalse = !i;
				
				carry = (((result.w[i] = (carry + resultwi + isifalse)) <= resultwi) & ((carry > 0) | (isifalse > 0)));
				
			} while (++i < (2*BIGINTWORDSZ));
		}
		
		// Bitwise not result.
		void resultnot () {
			
			uint i = 0;
			
			do result.w[i] = ~result.w[i];
			while (++i < (2*BIGINTWORDSZ));
		}
		
		resultnot();
		resultinc();
	}
	
	// The sign of the result
	// is positive if the multiplicand
	// and multiplier have the same
	// sign otherwise it is negative.
	if (isarg1negative != isarg2negative) resultneg();
	
	return (BIGINTMULRESULT){.h = result.h, .l = result.l};
}


typedef struct {
	BIGINTTYPE q, r;
} BIGINTDIVRESULT;

// Unsigned division.
static BIGINTDIVRESULT BIGINTDIVU (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	// Implementation of the unsigned division.
	// 
	// 4bits binary division when done by hand:
	// 
	// 11 divided by 3:
	// 
	// 11 (1011) is dividend.
	//  3 (0011) is divider.
	// 
	//  """"""""|
	//     1011 |
	// -0011    |
	//  """"""""|    0  Difference is negative: copy dividend and put 0 in quotient.
	//     1011 |
	//  -0011   |
	//  """"""""|   00  Difference is negative: copy dividend and put 0 in quotient.
	//     1011 |
	//   -0011  |
	//  """"""""|  001  Difference is positive: use difference and put 1 in quotient.
	//     0101 |
	//    -0011 |
	//  """"""""| 0011  Difference is positive: use difference and put 1 in quotient.
	//       10 | 
	// 
	// Remainder 2 (0010); Quotient, 3 (0011).
	// 
	// 
	// The division logic is implemented as follow:
	// The remainder and quotient are computed in
	// the dividend; 0 is shifted-in from the right
	// everytime the divider is greater than
	// the quotien, otherwise 1 is shifted-in
	// from the right.
	// The example below is a 4bits division,
	// hence four shift are needed; rq stand
	// for remainder-quotient which is the dividend.
	// At the end of the four shifts,
	// the remainder is in the 4 msb of rq
	// while the quotient is in the 4 lsb
	// of rq.
	// 
	//  """"""""|
	//     1011 |   0000 1011     <- rq.
	// -0011    |   -001 1        <- divider (never changes).
	//  """"""""|   0000 1011     <- rq before shift.
	//     1011 |   0001 0110     <- after shift.
	//  -0011   |   -001 1
	//  """"""""|   0001 0110     <- rq before shift.
	//     1011 |   0010 1100     <- after shift.
	//   -0011  |   -001 1
	//  """"""""|   0001 0100     <- rq before shift.
	//     0101 |   0010 1001     <- after shift.
	//    -0011 |   -001 1
	//  """"""""|   0001 0001     <- rq before shift.
	//     0010 |   0010 0011     <- after shift.
	// 
	// Remainder 2 (0010); Quotient, 3 (0011).
	
	typedef struct {
		
		union {
			
			struct {
				
				BIGINTTYPE l, h;
			};
			
			BIGINTWUTYPE w[2*BIGINTWORDSZ];
		};
		
	} divoperand;
	
	divoperand dividend = {
		.h = BIGINTZERO,
		.l = arg1
	};
	
	divoperand divider = {
		.h = arg2,
		.l = BIGINTZERO
	};
	
	// Shift dividend left logical.
	void dividendsll () {
		
		enum {SHIFTAMOUNT = 1}; // Must be less than BIGINTWBITSZ.
		
		uint i = ((2*BIGINTWORDSZ)-1);
		
		while (1) {
			
			dividend.w[i] <<= SHIFTAMOUNT;
			
			if (!i) return;
			
			uint j = i-1;
			
			dividend.w[i] |= (dividend.w[j]>>(BIGINTWBITSZ-SHIFTAMOUNT));
			
			i = j;
		}
	}
	
	// Shift divider right logical.
	void dividersrl () {
		
		enum {SHIFTAMOUNT = 1}; // Must be less than BIGINTWBITSZ.
		
		uint i = 0;
		
		while (1) {
			
			divider.w[i] >>= SHIFTAMOUNT;
			
			if (i == ((2*BIGINTWORDSZ)-1)) return;
			
			uint j = i+1;
			
			divider.w[i] |= (divider.w[j]<<(BIGINTWBITSZ-SHIFTAMOUNT));
			
			i = j;
		}
	}
	
	// Negate divider.
	void dividerneg () {
		// Increment divider.
		void dividerinc () {
			
			BIGINTWUTYPE carry = 0;
			
			uint i = 0;
			
			do {
				BIGINTWUTYPE dividerwi = divider.w[i];
				
				BIGINTWUTYPE isifalse = !i;
				
				carry = (((divider.w[i] = (carry + dividerwi + isifalse)) <= dividerwi) & ((carry > 0) | (isifalse > 0)));
				
			} while (++i < (2*BIGINTWORDSZ));
		}
		
		// Bitwise not divider.
		void dividernot () {
			
			uint i = 0;
			
			do divider.w[i] = ~divider.w[i];
			while (++i < (2*BIGINTWORDSZ));
		}
		
		dividernot();
		dividerinc();
	}
	
	dividersrl();
	dividerneg();
	
	divoperand remainder;
	
	// Compute remainder.
	void computeremainder () {
		
		BIGINTWUTYPE carry = 0;
		
		uint i = 0;
		
		do {
			BIGINTWUTYPE dividendwi = dividend.w[i];
			
			BIGINTWUTYPE dividerwi = divider.w[i];
			
			carry = (((remainder.w[i] = (carry + dividendwi + dividerwi)) <= dividendwi) & ((carry > 0) | (dividerwi > 0)));
			
		} while (++i < (2*BIGINTWORDSZ));
	}
	
	uint i = 0;
	
	do {
		computeremainder();
		
		// Compute whether the remainder is positive.
		BIGINTWUTYPE isremainderpositive = ((BIGINTWSTYPE)remainder.w[(2*BIGINTWORDSZ)-1] >= 0);
		
		// If remainder is positive,
		// it become the new dividend.
		if (isremainderpositive) dividend = remainder;
		
		dividendsll();
		
		// Compute quotient.
		dividend.w[0] |= isremainderpositive;
		
	} while (++i < BIGINTBITSZ);
	
	return (BIGINTDIVRESULT){.r = dividend.h, .q = dividend.l};
}

// Signed division.
static BIGINTDIVRESULT BIGINTDIV (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	// Implementation of the signed division.
	// 
	// Signed division is implemented
	// by using the absolute value
	// of operands and later remembering
	// what were their signs.
	// 
	// The sign of the quotient
	// is positive if the dividend
	// and divisor have the same
	// sign otherwise it is negative.
	// The sign of the remainder
	// is the same as the sign
	// of the dividend.
	// 
	// Dividend  Divisor  |  Quotient  Remainder  |  Example
	// -------------------+-----------------------+---------------------------
	//    +        +      |      +        +       |  +85 / +7 == +12;  R == +1
	//    +        -      |      -        +       |  +85 / -7 == -12;  R == +1
	//    -        +      |      -        -       |  -85 / +7 == -12;  R == -1
	//    -        -      |      +        -       |  -85 / -7 == +12;  R == -1
	
	// Compute whether arg1 is negative.
	BIGINTWUTYPE isarg1negative = ((BIGINTWSTYPE)arg1.w[BIGINTWORDSZ-1] < 0);
	
	// Compute whether arg2 is negative.
	BIGINTWUTYPE isarg2negative = ((BIGINTWSTYPE)arg2.w[BIGINTWORDSZ-1] < 0);
	
	if (isarg1negative) arg1 = BIGINTNEG(arg1);
	
	if (isarg2negative) arg2 = BIGINTNEG(arg2);
	
	BIGINTDIVRESULT result = BIGINTDIVU(arg1, arg2);
	
	// The sign of the remainder
	// is the same as the sign
	// of the dividend.
	if (isarg1negative) result.r = BIGINTNEG(result.r);
	
	// The sign of the quotient
	// is positive if the dividend
	// and divisor have the same
	// sign otherwise it is negative.
	if (isarg1negative != isarg2negative) result.q = BIGINTNEG(result.q);
	
	return result;
}


// Return 1 if not zero, otherwise return 0.
static uint BIGINTSNZ (BIGINTTYPE arg) {
	
	uint i = 0;
	
	do if (arg.w[i]) return 1;
	while (++i < BIGINTWORDSZ);
	
	return 0;
}


// Return (arg1 == arg2).
static uint BIGINTSEQ (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	uint i = 0;
	
	do if (arg1.w[i] != arg2.w[i]) return 0;
	while (++i < BIGINTWORDSZ);
	
	return 1;
}


// Return (arg1 != arg2).
static uint BIGINTSNE (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	uint i = 0;
	
	do if (arg1.w[i] != arg2.w[i]) return 1;
	while (++i < BIGINTWORDSZ);
	
	return 0;
}


// Return signed comparison (arg1 < arg2).
static uint BIGINTSLT (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	// Compute whether arg1 is negative.
	BIGINTWUTYPE isarg1negative = ((BIGINTWSTYPE)arg1.w[BIGINTWORDSZ-1] < 0);
	
	// Compute whether arg2 is negative.
	BIGINTWUTYPE isarg2negative = ((BIGINTWSTYPE)arg2.w[BIGINTWORDSZ-1] < 0);
	
	// Used to invert comparison result
	// when both arg1 and arg2 negative.
	uint x;
	
	if (isarg1negative) {
		if (!isarg2negative) return 1;
		else x = 1;
	} else if (isarg2negative) return 0;
	else x = 0;
	
	BIGINTWUTYPE arg1wi, arg2wi;
	
	uint i = BIGINTWORDSZ;
	
	do {
		--i;
		
		arg1wi = arg1.w[i];
		arg2wi = arg2.w[i];
		
		if (arg1wi != arg2wi) break;
		
	} while (i);
	
	return (arg1wi < arg2wi) ^ x;
}


// Return signed comparison (arg1 <= arg2).
static uint BIGINTSLTE (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	// Compute whether arg1 is negative.
	BIGINTWUTYPE isarg1negative = ((BIGINTWSTYPE)arg1.w[BIGINTWORDSZ-1] < 0);
	
	// Compute whether arg2 is negative.
	BIGINTWUTYPE isarg2negative = ((BIGINTWSTYPE)arg2.w[BIGINTWORDSZ-1] < 0);
	
	// Used to invert comparison result
	// when both arg1 and arg2 negative.
	uint x;
	
	if (isarg1negative) {
		if (!isarg2negative) return 1;
		else x = 1;
	} else if (isarg2negative) return 0;
	else x = 0;
	
	BIGINTWUTYPE arg1wi, arg2wi;
	
	uint i = BIGINTWORDSZ;
	
	do {
		--i;
		
		arg1wi = arg1.w[i];
		arg2wi = arg2.w[i];
		
		if (arg1wi != arg2wi) break;
		
	} while (i);
	
	return (arg1wi <= arg2wi) ^ x;
}


// Return unsigned comparison (arg1 < arg2).
static uint BIGINTSLTU (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTWUTYPE arg1wi, arg2wi;
	
	uint i = BIGINTWORDSZ;
	
	do {
		--i;
		
		arg1wi = arg1.w[i];
		arg2wi = arg2.w[i];
		
		if (arg1wi != arg2wi) break;
		
	} while (i);
	
	return (arg1wi < arg2wi);
}


// Return unsigned comparison (arg1 <= arg2).
static uint BIGINTSLTEU (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTWUTYPE arg1wi, arg2wi;
	
	uint i = BIGINTWORDSZ;
	
	do {
		--i;
		
		arg1wi = arg1.w[i];
		arg2wi = arg2.w[i];
		
		if (arg1wi != arg2wi) break;
		
	} while (i);
	
	return (arg1wi <= arg2wi);
}


// Bitwise AND.
static BIGINTTYPE BIGINTAND (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTTYPE result;
	
	uint i = 0;
	
	do result.w[i] = (arg1.w[i] & arg2.w[i]);
	while (++i < BIGINTWORDSZ);
	
	return result;
}


// Bitwise OR.
static BIGINTTYPE BIGINTOR (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTTYPE result;
	
	uint i = 0;
	
	do result.w[i] = (arg1.w[i] | arg2.w[i]);
	while (++i < BIGINTWORDSZ);
	
	return result;
}


// Bitwise XOR.
static BIGINTTYPE BIGINTXOR (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	BIGINTTYPE result;
	
	uint i = 0;
	
	do result.w[i] = (arg1.w[i] ^ arg2.w[i]);
	while (++i < BIGINTWORDSZ);
	
	return result;
}


// Logical left shift.
static BIGINTTYPE BIGINTSLL (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	arg2 = BIGINTDIVU(arg2, BIGINTFROMUINTBIGINTBITSZ).r;
	
	BIGINTWUTYPE shiftamount; // shiftamount must be less than BIGINTWBITSZ.
	
	while ((shiftamount = (arg2.w[0] % BIGINTWBITSZ)) || BIGINTSNZ(arg2)) {
		
		uint i = (BIGINTWORDSZ-1);
		
		uint j;
		
		if (shiftamount) {
			
			while (1) {
				
				arg1.w[i] <<= shiftamount;
				
				if (!i) break;
				
				j = i-1;
				
				arg1.w[i] |= (arg1.w[j]>>(BIGINTWBITSZ-shiftamount));
				
				i = j;
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINT(shiftamount));
			
		} else {
			
			while (1) {
				
				if (i) {
					
					j = i-1;
					
					arg1.w[i] = arg1.w[j];
					
					i = j;
					
				} else {
					
					arg1.w[0] = 0;
					
					break;
				}
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINTBIGINTWBITSZ);
		}
	}
	
	return arg1;
}


// Logical right shift.
static BIGINTTYPE BIGINTSRL (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	arg2 = BIGINTDIVU(arg2, BIGINTFROMUINTBIGINTBITSZ).r;
	
	BIGINTWUTYPE shiftamount; // shiftamount must be less than BIGINTWBITSZ.
	
	while ((shiftamount = (arg2.w[0] % BIGINTWBITSZ)) || BIGINTSNZ(arg2)) {
		
		uint i = 0;
		
		uint j;
		
		if (shiftamount) {
			
			while (1) {
				
				arg1.w[i] >>= shiftamount;
				
				if (i == (BIGINTWORDSZ-1)) break;
				
				j = i+1;
				
				arg1.w[i] |= (arg1.w[j]<<(BIGINTWBITSZ-shiftamount));
				
				i = j;
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINT(shiftamount));
			
		} else {
			
			while (1) {
				
				if (i != (BIGINTWORDSZ-1)) {
					
					j = i+1;
					
					arg1.w[i] = arg1.w[j];
					
					i = j;
					
				} else {
					
					arg1.w[BIGINTWORDSZ-1] = 0;
					
					break;
				}
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINTBIGINTWBITSZ);
		}
	}
	
	return arg1;
}


// Arithmetic right shift.
static BIGINTTYPE BIGINTSRA (BIGINTTYPE arg1, BIGINTTYPE arg2) {
	
	arg2 = BIGINTDIVU(arg2, BIGINTFROMUINTBIGINTBITSZ).r;
	
	BIGINTWUTYPE shiftamount; // shiftamount must be less than BIGINTWBITSZ.
	
	while ((shiftamount = (arg2.w[0] % BIGINTWBITSZ)) || BIGINTSNZ(arg2)) {
		
		uint i = 0;
		
		uint j;
		
		if (shiftamount) {
			
			while (1) {
				
				if (i == (BIGINTWORDSZ-1)) {
					
					arg1.w[i] = ((BIGINTWSTYPE)arg1.w[i] >> shiftamount);
					
					break;
					
				} else arg1.w[i] >>= shiftamount;
				
				j = i+1;
				
				arg1.w[i] |= (arg1.w[j]<<(BIGINTWBITSZ-shiftamount));
				
				i = j;
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINT(shiftamount));
			
		} else {
			
			while (1) {
				
				if (i != (BIGINTWORDSZ-1)) {
					
					j = i+1;
					
					arg1.w[i] = arg1.w[j];
					
					i = j;
					
				} else {
					
					arg1.w[BIGINTWORDSZ-1] = ((BIGINTWSTYPE)arg1.w[BIGINTWORDSZ-1] >> (BIGINTWBITSZ-1));
					
					break;
				}
			}
			
			arg2 = BIGINTSUB(arg2, BIGINTFROMUINTBIGINTWBITSZ);
		}
	}
	
	return arg1;
}


// Convert a string to its equivalent bigint.
// It take as argument a non-null pointer
// to a pointer to a string, so that the calling
// function can know where the parsing ended
// if it could not be completely done
// due to an invalid character or an overflow.
// It also a take as argument the maximum count
// of characters to parse from the string.
// The string to parse must be null-terminated
// if the argument strsz is larger than
// the size of the string.
// The string to parse can be prefixed
// by a base specifier; ei:
// Decimal: 234
// Hexadecimal: 0xaa13
// Octale: 0o1234
// Binary: 0b101110
// It can only start with '0' if it is
// to be followed by the base specifier.
// An error is detected if upon this function
// returning, the string pointer passed byref
// to this function was not at the end of the string.
// If the error was due to an invalid character,
// the argument *str will be at the location
// of the invalid character.
// If the error was due to an overflow,
// the argument *str will be at the first
// character of the string which should be
// a valid digit within the range '0' to '9'.
static BIGINTTYPE BIGINTFROMSTR_ (u8** str, uint strsz) {
	// Label to return to when an error is thrown.
	__label__ error;
	
	BIGINTTYPE result = BIGINTZERO;
	
	if (!strsz) return result;
	
	uint base;
	
	// Determine the integer base
	// of the string to convert.
	if (**str == '0') {
		
		if (!--strsz) return result;
		
		++*str;
		
		u8 c = **str;
		
		if (c == 'b' || c == 'B') base = 2;
		else if (c == 'o' || c == 'O') base = 8;
		else if (c == 'x' || c == 'X') base = 16;
		else return result;	// Return due to invalid base specifier or
					// because the string to parse was simply "0".
		
		if (!--strsz) return result;
		
		++*str;
		
	} else base = 10;
	
	// Function which determine whether
	// a character is a valid digit in
	// the current base.
	uint isdigitvalid (u8 c) {
		
		if (base == 2) {
			
			if (c == '0' || c == '1') return 1;
			else return 0;
			
		} else if (base == 8) {
			
			if (c >= '0' && c <= '7') return 1;
			else return 0;
			
		} else if (base == 10) {
			
			if (c >= '0' && c <= '9') return 1;
			else return 0;
			
		} else if (base == 16) {
			
			if (	(c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F') ) return 1;
			else return 0;
			
		} else return 0;
	}
	
	// Here i will be used as an offset from
	// the end of the string being parsed.
	uint i = 0;
	
	// Save where the string start
	// so that *str can be restored
	// when an overflow occur, in order
	// to convey that parsing failed.
	u8* savedaddr = *str;
	
	// Function which process each character digit.
	void processdigit (u8 n) {
		
		u8 c;
		
		if (--strsz && (c = *++*str)) {
			if (isdigitvalid(c)) processdigit(c);
			else goto error; // Throw error due to invalid digit.
		}
		
		// Accumulating function.
		// The argument i is used as
		// an index within result.w[];
		// The argument n is the amount
		// to accumulate in the bigint result.
		void accumulate (uint n, uint i) {
			
			if ((result.w[i] += n) < n) {
				// Overflow occured.
				// Carry over the overflow to
				// the most significant bytes.
				if (++i < BIGINTWORDSZ) accumulate(1, i);
				else {
					// Throw error due to overflow,
					// setting *str to the start
					// of the null-terminated string
					// so to convey that parsing failed.
					*str = savedaddr;
					
					goto error;
				}
			}
		}
		
		// This function break higher significant
		// digits into smaller increment that
		// can be accumulated into the bigint result.
		// The argument i is used as an offset from
		// the end of the string being parsed, and
		// which tell how significant the digit is.
		// The argument n is the digit amount.
		void propagate (uint i, uint n) {
			
			if (!i) while (n--) accumulate(base, 0);
			else while (n--) propagate(i-1, base);
		}
		
		// Compute the digit amount.
		if (base == 16) {
			
			if      (n >= '0' && n <= '9') n = (n - '0');
			else if (n >= 'a' && n <= 'f') n = ((n - 'a') + 10);
			else if (n >= 'A' && n <= 'F') n = ((n - 'A') + 10);
			
		} else n = (n - '0');
		
		if (!i) accumulate(n, 0);
		else propagate(i-1, n);
		
		++i;
	}
	
	u8 c = **str;
	
	if (c && isdigitvalid(c)) processdigit(c);
	
	error:
	
	return result;
}

// Same as BIGINTFROMSTR_(u8** str, uint strsz)
// but the string to parse is not passed byref.
static BIGINTTYPE BIGINTFROMSTR (u8* str, uint strsz) {
	return BIGINTFROMSTR_(&str, strsz);
}


// Functions converting their BIGINTTYPE argument
// to their corresponding encoding. ei:
// respectively decimal, hexadecimal,
// octale and binary.
// Their argument buf must point to a buffer
// large enough for the targetted encoding.

// The buffer size must be (BIGINTMAXDECDIGITS+1).
static void BIGINTTODECSTR_ (BIGINTTYPE n, u8* buf) {
	
	u8 s[BIGINTMAXDECDIGITS];
	
	uint i = BIGINTMAXDECDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTTEN, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTTEN).r.w[0] + '0';
		
		--i;
		
		n = BIGINTDIVU(n, BIGINTTEN).q;
	}
	
	*buf = n.w[0] + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	//uint retvar = (BIGINTMAXDECDIGITS-i);
	
	while (++i < BIGINTMAXDECDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
}

// The buffer size must be (BIGINTMAXHEXDIGITS+1).
static void BIGINTTOHEXSTR_ (BIGINTTYPE n, u8* buf) {
	
	u8 s[BIGINTMAXHEXDIGITS+1];
	
	uint i = BIGINTMAXHEXDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTSIXTEEN, n)) {
		
		uint nn = BIGINTDIVU(n, BIGINTSIXTEEN).r.w[0];
		
		if (nn < 10) s[i] = nn + '0';
		else s[i] = (nn - 10) + 'a';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTFOUR); // Do a division by 16.
	}
	
	if (n.w[0] < 10) *buf = n.w[0] + '0';
	else *buf = (n.w[0] - 10) + 'a';
	
	// Compute the number of digits
	// that resulted from the conversion.
	//uint retvar = (BIGINTMAXHEXDIGITS-i);
	
	while (++i < BIGINTMAXHEXDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
}

// The buffer size must be (BIGINTMAXOCTDIGITS+1).
static void BIGINTTOOCTSTR_ (BIGINTTYPE n, u8* buf) {
	
	u8 s[BIGINTMAXOCTDIGITS];
	
	uint i = BIGINTMAXOCTDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTEIGHT, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTEIGHT).r.w[0] + '0';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTTHREE); // Do a division by 8.
	}
	
	*buf = n.w[0] + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	//uint retvar = (BIGINTMAXOCTDIGITS-i);
	
	while (++i < BIGINTMAXOCTDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
}

// The buffer size must be (BIGINTMAXBINDIGITS+1).
static void BIGINTTOBINSTR_ (BIGINTTYPE n, u8* buf) {
	
	u8 s[BIGINTMAXBINDIGITS];
	
	uint i = BIGINTMAXBINDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTTWO, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTTWO).r.w[0] + '0';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTONE); // Do a division by 2.
	}
	
	*buf = n.w[0] + '0';
	
	// Compute the number of digits
	// that resulted from the conversion.
	//uint retvar = (BIGINTMAXBINDIGITS-i);
	
	while (++i < BIGINTMAXBINDIGITS)
		*++buf = s[i];
	
	*++buf = 0;
}


// Functions converting their BIGINTTYPE argument
// to their corresponding encoding. ei:
// respectively decimal, hexadecimal,
// octale and binary.

static string BIGINTTODECSTR (BIGINTTYPE n) {
		
	u8 s[BIGINTMAXDECDIGITS];
	
	uint i = BIGINTMAXDECDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTTEN, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTTEN).r.w[0] + '0';
		
		--i;
		
		n = BIGINTDIVU(n, BIGINTTEN).q;
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = (u8*)mmalloc((BIGINTMAXDECDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n.w[0] + '0';
	
	while (++i < BIGINTMAXDECDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

static string BIGINTTOHEXSTR (BIGINTTYPE n) {
	
	u8 s[BIGINTMAXHEXDIGITS+1];
	
	uint i = BIGINTMAXHEXDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTSIXTEEN, n)) {
		
		uint nn = BIGINTDIVU(n, BIGINTSIXTEEN).r.w[0];
		
		if (nn < 10) s[i] = nn + '0';
		else s[i] = (nn - 10) + 'a';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTFOUR); // Do a division by 16.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = (u8*)mmalloc((BIGINTMAXHEXDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	if (n.w[0] < 10) *ptr = n.w[0] + '0';
	else *ptr = (n.w[0] - 10) + 'a';
	
	while (++i < BIGINTMAXHEXDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

static string BIGINTTOOCTSTR (BIGINTTYPE n) {
	
	u8 s[BIGINTMAXOCTDIGITS];
	
	uint i = BIGINTMAXOCTDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTEIGHT, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTEIGHT).r.w[0] + '0';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTTHREE); // Do a division by 8.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = (u8*)mmalloc((BIGINTMAXOCTDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n.w[0] + '0';
	
	while (++i < BIGINTMAXOCTDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}

static string BIGINTTOBINSTR (BIGINTTYPE n) {
	
	u8 s[BIGINTMAXBINDIGITS];
	
	uint i = BIGINTMAXBINDIGITS-1;
	
	while (BIGINTSLTEU(BIGINTTWO, n)) {
		
		s[i] = BIGINTDIVU(n, BIGINTTWO).r.w[0] + '0';
		
		--i;
		
		n = BIGINTSRL(n, BIGINTONE); // Do a division by 2.
	}
	
	// +1 account for the null terminating character.
	string retvar = {.ptr = (u8*)mmalloc((BIGINTMAXBINDIGITS-i)+1)};
	
	u8* ptr = retvar.ptr;
	
	*ptr = n.w[0] + '0';
	
	while (++i < BIGINTMAXBINDIGITS)
		*++ptr = s[i];
	
	*++ptr = 0;
	
	return retvar;
}
