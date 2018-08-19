
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

#include <byt.h>
#include <mm.h>
#include <string.h>

// Structure representing a token.
typedef struct token {
	// Hold the address of the array of pointers to
	// token onto which this token is attached to.
	// This field is important to pamsynmatch1()
	// during repetitions.
	struct token** holder;
	
	// str contains the character to match.
	// The field str.sz represent the number
	// of character str is meant to match.
	// if str.ptr == 0 and str.sz == 0 then the token
	// will be ignored during processing
	// if str.ptr == 0 and str.sz == n then the token
	// will match any n characters found, that mean
	// n characters will be skipped.
	struct {
		
		u8* ptr;
		uint sz;
		
	} str;
	
	struct tokenflags {
		
		uint isnot; // When 1, I have a match if the token do not match. It is used by '!' .
		uint ishidden; // When 1, the field str is not part of the result of matching. It is used by '#' .
		uint isignorecharcase; // When 1, characters' case are ignored. It is used by '_' .
		uint isfirst; // When 1, the token represent the metachar '<' .
		uint islast; // When 1, the token represent the metachar '>' .
		uint isany; // When 1, the token represent the metachar '*' which is a variable number of '?' .
		uint isnochar; // When 1, the token represent the metachar '.' .
		
		// When 1, the token represent a set of single
		// or range characters that are used within
		// the brackets metacharacter.
		// The field str point to a set of 2 characters
		// (terminated with a null char), each representing
		// a range. ei: [ade-h] yield "aaddeh";
		uint ischarset;
		
	} flags;
	
	struct token* alt; // Alternate pattern.
	struct token* prev; // Used to be able to go back up the tree.
	struct token* next; // To the next token.
	
	// Point to the head token of a
	// repetion or tokencompare block.
	struct token* head;
	
	// Hold the id of a tokencompare block.
	uint tokcmpid;
	
	// Structure used for repetitions.
	struct {
		// Pointer to an array of integer holding
		// the repeat sequence elements.
		// ei: +2,1-5,5-0,9{string} will be represented
		// as 2,2,1,5,5,0,9,9
		// Single numbers such as 2 in the previous example
		// are internally represented as 2,2.
		uint* seq;
		
		// Number of elements in the array of integer.
		// ei: +2,1-5,5-0,9{otherstuffs} has 4 elements.
		uint seqsz;
		
		// Hold the index of the curent element
		// in the array of integer pointed by seq.
		uint sel;
		
		// Hold the count of the lower value
		// of a repeat sequence
		uint count;
		
	} repeat;
	
} token;

// Struct used by pamsyntokenize() to return the
// tokenized form of the pattern that it is given.
typedef struct {
	// Pointer to an array of pointers to token.
	token** tokenlist;
	
} pamsyntokenized;

// This structure is used by pamsynsearch()
// to pass matches found, to its callback function.
// All fields of this structure can safely be modified
// by the callback function; ei: the field next
// can be modified to remove a pamsynmatchfound from
// the linkedlist.
typedef struct pamsynmatchfound {
	// Memory address of where the match start
	// within the memory region given to pamsynmatch1()
	// through its arguments start and size.
	u8* start;
	
	// Memory address of where the match end
	// within the memory region given to pamsynmatch1()
	// through its arguments start and size.
	u8* end;
	
	// When 0 it mean that the match is a regular string.
	// When 1, the match is a nochar which is before
	// 	the character pointed by the fields start and end.
	// When 2, the match is a nochar which is after the character
	// 	pointed by the fields start and end.
	// For applications that do not make use of nochar,
	// removing all nochar pamsynmatchfound from the linkedlist
	// can be done first to make processing easier.
	// The metacharacter '.' is useful only to applications
	// that do substitutions.
	uint nochar;
	
	// Pointer to the next pamsynmatchfound.
	// The field next of the last pamsynmatchfound
	// of the linkedlist is null.
	struct pamsynmatchfound* next;
	
} pamsynmatchfound;

// This structure is used by pamsynmatch2()
// to return its result.
typedef struct {
	
	u8* start; // Address of the first non-hidden character of what was matched.
	u8* end; // Address of the last non-hidden character of what was matched.
	
} pamsynmatched;

// This function return 1 if the character given as argument
// is a metacharacter, otherwise 0 is returned.
static uint ismetachar (u8 c) {
	
	if (c == ',' || c == '-' || c == '.' || c == '*' || c == '?' ||
		c == '<' || c == '>' || c == '[' || c == ']' || c == '{' || c == '}' ||
		c == '!' || c == '#' || c == '_' || c == '+' || c == '%' || c == '\\')
		return 1;
	
	return 0;
}

#include "escape.pamsyn.c"
#include "free.pamsyn.c"
#include "tokenize.pamsyn.c"
#include "match.pamsyn.c"
#include "extract.pamsyn.c"
