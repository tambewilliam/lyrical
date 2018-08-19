
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


// PAMSYN (PAttern Matching SYNtax).
// 
// PAMSYN borrow its syntax from the unix shell with
// the use of the well known metacharacters '*' and '?';
// on top of which additional metacharacters
// are implemented to enhance it.
// 
// Metacharacters:
// 
// *		Match zero or more consecutive characters.
// 
// 
// ?		Match any single character.
// 
// 
// <		Match the begining of the string on which
// 		the pattern is applied.
// 		ei: "<object" will match "object" only if it is
// 		found at the beginning.
// 
// 
// >		Match the end of the string on which the pattern
// 		is applied.
// 		ei: "object>" will match "object" only if it is
// 		found at the end.
// 
// 
// .		Match an empty space; this metacharacter is used
// 		to perform an insertion, and is commonly used with
// 		the metacharacter construct #{,} to match a location
// 		where to do the insertion.
// 		ei: "#{arg}.#{s}" will match the empty space in
// 		"args" that is found between "arg" and "s".
// 
// 
// []		Match any one of the enclosed characters.
// 		A pair of characters seperated by a hyphen denote
// 		a range; any character that is between the range of
// 		those two characters included, is matched. It is done
// 		according to the ASCII table.
// 		ei: "[ab-dz]" will match any of the single characters
// 		'a', 'b' through 'd' and 'z'.
// 		It is also possible to use escape characters within
// 		'[' and ']'.
// 
// 
// {,}		Match either expression within braces which are
// 		separated by commas.
// 		ei: "{plane,ball,note}" will match either "plane",
// 		"ball" or "note".
// 
// 
// !{,}
// ![]		Match expression just like with {,} or [] respectively,
// 		only if a difference is found. The number of characters
// 		is taken into account.
// 		ei: "!{cargo}" will match only if the 5 characters string
// 		is not "cargo".
// 		"!{train,pen} will first match 5 characters against
// 		"train"; if there is no string difference, 3 characters
// 		are matched against "pen", and if there is a string
// 		difference a match has been found otherwise there was
// 		no match; note that the use of metacharacters '?', '*'
// 		or any other metacharacters, keep their meaning. 
// 		A common pitfall is to think that a pattern such as
// 		"!{a,b}" mean not matching 'a' and 'b' which is wrong,
// 		because "!{a,b}" is equivalent to "{!{a},!{b}}" which
// 		mean not matching 'a' or not matching 'b'; to achieve
// 		the meaning of not matching 'a' and 'b' one should
// 		use the brackets metacharacters.
// 		ei: "![ab] which will match only if the character
// 		is not 'a' and 'b'.
// 		The reason why a pattern such as "!{text1,t2,texting3}a"
// 		cannot mean not matching "text1" and "t2" and "texting3"
// 		is because the size is taken into consideration; let's
// 		assume that none of those alternatives work, it is
// 		impossible to determine how many characters to skip
// 		before matching 'a' .
// 
// 
// #{,}
// #[]		Match expression just like with {,} or [] respectively,
// 		but hide any match from the final result.
// 		ei: "#{date: }01/22/#{1985}" will match the string
// 		"date: 01/22/1985", but the final result will be "01/22/".
// 
// 
// _{,}
// _[]		Match expression just like with {,} or []
// 		respectively, ignoring characters' case.
// 
// 
// %{,}
// %[]		Match expression within braces just like with {,} or [] respectively,
// 		and compare the result of the match against another similar block
// 		with the same id. The id is a non-null decimal integer value
// 		that is specified between "%" and "{" or "["; ei: %2{pattern}.
// 		The id can be ommitted; in which case the id is assumed
// 		to be the maximum value of a uint.
// 		Only the non-hidden portion of the match is compared; in
// 		other words any match within a block #{,} is not compared.
// 		ei: "%{[0-9][0-9]#{/22/1985}}::%{[0-9][0-9]#{/12/1970}}"
// 		The above pattern will yield a match if the two digits month
// 		are the same in both dates, and the day and year digits need
// 		to be "/22/1985" and "/12/1970" in the first and second date
// 		respectively; hence a string such as "12/22/1985::13/12/1970"
// 		will fail the match but a string such as "10/22/1985::10/12/1970"
// 		will succeed the match.
// 
// 
// +{,}
// +[]		Match expression within braces just like with {,} or [] respectively,
// 		but repeat the match. The repetition count is specified
// 		between '+' and '{' or '['.
// 		More than one repetition count can be specified as well
// 		as ranges of repetition count. ei: "+2{a}" will match "aa" .
// 		When there is more than one repetition count or a range of
// 		repetition count, each repetition count is tried until
// 		a repetition count that yield a positive match is found.
// 		"+2,4{a}" will match in order "aa" or "aaaa" and is similar
// 			to "{aa,aaaa}" .
// 		"+2-4,1{a}" will match in order "aa" or "aaa" or "aaaa" or "a"
// 			and is similar to "{aa,aaa,aaaa,a}" .
// 		"+4-2{a}" will match in order "aaaa" or "aaa" or "aa" and
// 			is similar to "{aaaa,aaa,aa}" .
// 		"+2{ab,aa}" is similar to "{ab,aa}{ab,aa}" .
// 		Note that when using a range, the first number is the
// 			starting count, while the second value is
// 			the ending count.
// 		
// 		The character '*' can be used instead of a number to specify
// 		an infinite count or a range with an infinite boundary.
// 		ei: "+*-10{d}" will try a match that contain an infinite
// 		count of the character 'd' down to a match that contain
// 		a count of 10 characters 'd'.
// 		
// 		One could write a pattern such as "+*{a}" but it wouldn't
// 		be usefull because such a pattern basically mean matching
// 		a text containing (uint)-1 characters 'a';
// 		
// 		The pattern "+{ag}" is similar to "+1-*{ag}" which will
// 		attempt to match at least one "ag" and keep repeating it
// 		as much as possible in an attempt to find a match.
// 		
// 		A common pitfall is to use "b+{a}" on "baaac" and to think
// 		that "baaa" will be matched; only 'b' and at least one 'a'
// 		will be matched. To match "baaa" one should use one of
// 		the following pattern: "b+*-1{a}", "b+{a}#{c}"; where in
// 		the last pattern 'c' is matched simply to make sure that
// 		all 'a' have been matched, but 'c' is not made part of
// 		the final result.
// 		
// 		"+3{+0-9{a}}" is similar to "+0-9{a}+0-9{a}+0-9{a}" but
// 		if applied to a text such as "a", the pattern "+3{+0-9{a}}" will
// 		fail while the pattern "+0-9{a}+0-9{a}+0-9{a}" will succeed.
// 		This is because a repetition number cannot be larger than
// 		the space left to match, even if within the repetition brace
// 		they could be a pattern which can match a space lesser than
// 		the repetition size as it is the case for "+3{+0-9{a}}".
// 
// To keep the pattern matching syntax clear,
// all metacharacters are required to be escaped
// when used as regular characters, otherwise
// the pattern is incorrect.
// 
// Note about escaping:
// The only escaping sequences accepted are:
// '\\', '\n', '\t', '\x' where x is a metacharacter,
// '\xx' where xx is a two digits hexadecimal number
// that translate to a byte value.



#ifndef LIBPAMSYN
#define LIBPAMSYN

#include <string.h>

// Struct used by pamsyntokenize() to return the
// tokenized form of the pattern that it is given.
typedef struct {
	
	void* ptr;
	
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

// This structure is used by
// pamsynmatch2() to return its result.
typedef struct {
	
	u8* start; // Address of the first non-hidden character of what was matched.
	u8* end; // Address of the last non-hidden character of what was matched.
	
} pamsynmatched;

// This function take as input a null-terminated string
// and return a string in which any characters
// that could be seen as a metacharacter is escaped.
// Useful when given a string containing characters
// that could be seen as metacharacter and which must
// be seen as regular characters before passing it
// to pamsyntokenize().
string pamsynescape (u8* pattern);

// Transform a pattern to a set of tokens.
// The argument pattern should be valid and non-null.
// Null is returned if the pattern is incorrect.
pamsyntokenized pamsyntokenize (u8* pattern);

// This function search for a match within the memory
// region given by the arguments start and size, and pass
// a match found in the form of a pamsynmatchfound to the
// specified callback function everytime a match is found.
// Note that it can happen that a null pointer is passed
// to the callback function, and that happen when a match
// occured but there was no non-hidden character in the match;
// so it is important for the callback function to make sure
// that its argument is not a null pointer.
// - When the callback function return 0,
// this function stop looking for matches.
// - When the callback function returns 1, this function
// look for the next match starting after the previous match.
// If the match found was a nochar, this function will ignore
// the return value of the callback function and perform
// a slow lookup as described below; this is because
// nochar match have no size.
// - when the callback function returns >1, this function
// look for the next match as if it didn't find a match, meaning
// without positioning itself after the last character of
// the previous match.
// The callback function pointer must be non-null and valid.
// The argument size cannot be null.
// The callback function must not attempt to free
// the pamsynmatchfound object that it receive as it will be
// done by pamsynmatch1() upon the callback function returning.
// TODO: Reimplement the effect of the callback return value
// and update all function that rely on pamsynmatch1().
// When the callback return 0, do a slow lookup.
// When the callback return a value which after being added to curpos,
// 	is less than (start+size) and greater than or equal to start,
// 	continue at that returned offset after the last character
// 	of the previous match.
// For any other return value, stop looking for matches.
// TODO: Make sure to check that the returned offset
// is within start and ((start+size)-1), because
// the returned offset could overflow when added to curpos ...
void pamsynmatch1 (pamsyntokenized t, u8* start, uint size, uint (*callback)(pamsynmatchfound*));

// This function will return the address of the first
// and last non-hidden character of what was matched,
// using the pamsyntokenized on the memory region
// given through the arguments start and size.
// A null pamsynmatched is returned
// if a match could not be found.
pamsynmatched pamsynmatch2 (pamsyntokenized t, u8* start, uint size);

// This function extract from the memory region given
// by the arguments start and size, a match for the
// pamsyntokenized given as argument t and return it.
// Note that only the non-hidden characters
// of what was matched are extracted.
// stringnull is returned if a match could not be found.
string pamsynextract (pamsyntokenized t, u8* start, uint size);

// Free the memory used by the object
// returned by pamsyntokenize().
void pamsynfree (pamsyntokenized t);

#endif
