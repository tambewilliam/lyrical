
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


#ifndef LIBSTRING
#define LIBSTRING

// Library to manipulate array of characters.

// Struct used to manipulate
// an array of characters.
typedef struct {
	// Pointer to a null-terminated
	// array of characters allocated
	// through mmalloc().
	// It must be modified only through
	// string*() functions, and freed
	// only through mmrefdown() as
	// it may be multi-referenced.
	u8* ptr;
	
} string;

// Null value useful for initialization.
#define stringnull ((string){.ptr = 0})

// An argument of type u8* cannot
// be null or an invalid pointer,
// and the string it point to is
// always treated as read-only.

// Return the number of characters
// in the null terminated string.
// The terminating null character
// is not included in the count.
uint stringsz (u8* s);

#define stringmmsz(s) (s.ptr?(mmsz(s.ptr)-1):0)

// These functions return a string duplicate.
string stringduplicate1 (string s);
string stringduplicate2 (u8* s);
string stringduplicate3 (u8* s, uint sz);
string stringduplicate4 (u8 c);

// These functions append s2 to s1.
void stringappend1 (string* s1, string s2);
void stringappend2 (string* s1, u8* s2);
void stringappend3 (string* s1, u8* s2, uint s2sz);
void stringappend4 (string* s, u8 c);

// These functions insert s2 in s1
// at the offset given as argument.
// If offset is greater than
// the size of s1, nothing is done.
void stringinsert1 (string* s1, string s2, uint offset);
void stringinsert2 (string* s1, u8* s2, uint offset);
void stringinsert3 (string* s1, u8* s2, uint s2sz, uint offset);
void stringinsert4 (string* s, u8 c, uint offset);

// These functions overwrite s1 with
// s2 from the offset given as argument,
// increasing s1 as necessary
// until s2 has all been written.
// If offset is greater than
// the size of s1 nothing is done.
void stringoverwrite1 (string* s1, string s2, uint offset);
void stringoverwrite2 (string* s1, u8* s2, uint offset);
void stringoverwrite3 (string* s1, u8* s2, uint s2sz, uint offset);
void stringoverwrite4 (string* s, u8 c, uint offset);

// This function remove from the string given
// as argument, the string portion starting at
// the offset given as argument and which has
// the character count given by the argument sz.
// If the length of the string given as argument
// is null, or if the argument sz is null or
// if the string portion to remove go beyond
// boundary, nothing is done.
void stringremove (string* s, uint offset, uint sz);

// This function truncate the argument
// string s to the character count
// given by the argument sz.
// Nothing is done if the argument sz
// is greater than or equal to
// the size of the argument string s.
void stringtruncate (string* s, uint sz);

// This function remove the character
// count given by the argument sz
// from the end of the argument string s.
// Nothing is done if the argument sz
// is null or greater than the size
// of the argument string s.
void stringchop (string* s, uint sz);

// This function split the string given
// as argument at the offset given as argument,
// and return the bottom part of the split.
// stringnull is returned if the offset
// is greater than or equal to the number of
// characters in the string given as argument.
string stringsplit (string* s, uint offset);

// These functions return 0 if the strings given
// as argument are not equal, otherwise 1 is returned.
// stringiseq() take into account the characters' case
// while stringiseqnocase() do not.
// Functions for which the last argument is sz compare
// the amount of characters given by the argument sz
// ignoring their null-terminating character.
uint stringiseq1 (string s1, string s2);
uint stringiseq2 (string s1, u8* s2);
uint stringiseq3 (u8* s1, u8* s2);
uint stringiseq4 (u8* s1, u8* s2, uint sz);
uint stringiseq5 (u8* s1, uint s1sz, string s2);
uint stringiseq6 (u8* s1, uint s1sz, u8* s2);
uint stringiseq7 (u8* s1, uint s1sz, u8* s2, uint s2sz);
uint stringiseqnocase1 (string s1, string s2);
uint stringiseqnocase2 (string s1, u8* s2);
uint stringiseqnocase3 (u8* s1, u8* s2);
uint stringiseqnocase4 (u8* s1, u8* s2, uint sz);
uint stringiseqnocase5 (u8* s1, uint s1sz, string s2);
uint stringiseqnocase6 (u8* s1, uint s1sz, u8* s2);
uint stringiseqnocase7 (u8* s1, uint s1sz, u8* s2, uint s2sz);

// These functions find the first occurence
// of the string tosearch in the string s, searching
// from left to right, and return the starting address
// within s of the string found, otherwise null is returned.
// The functions stringsearchright() take into account
// the characters' case while the functions
// stringsearchrightignorecase() do not.
u8* stringsearchright1 (string s, string tosearch);
u8* stringsearchright2 (string s, u8* tosearch);
u8* stringsearchright3 (u8* s, string tosearch);
u8* stringsearchright4 (u8* s, u8* tosearch);
u8* stringsearchright5 (u8* s, uint ssz, string tosearch);
u8* stringsearchright6 (u8* s, uint ssz, u8* tosearch);
u8* stringsearchright7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz);
u8* stringsearchrightignorecase1 (string s, string tosearch);
u8* stringsearchrightignorecase2 (string s, u8* tosearch);
u8* stringsearchrightignorecase3 (u8* s, string tosearch);
u8* stringsearchrightignorecase4 (u8* s, u8* tosearch);
u8* stringsearchrightignorecase5 (u8* s, uint ssz, string tosearch);
u8* stringsearchrightignorecase6 (u8* s, uint ssz, u8* tosearch);
u8* stringsearchrightignorecase7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz);

// These functions find the first occurence
// of the string tosearch in the string s, searching
// from right to left, and return the starting address
// within s of the string found, otherwise null is returned.
// The functions stringsearchright() take into account
// the characters' case while the functions
// stringsearchrightignorecase() do not.
u8* stringsearchleft1 (string s, string tosearch);
u8* stringsearchleft2 (string s, u8* tosearch);
u8* stringsearchleft3 (u8* s, string tosearch);
u8* stringsearchleft4 (u8* s, u8* tosearch);
u8* stringsearchleft5 (u8* s, uint ssz, string tosearch);
u8* stringsearchleft6 (u8* s, uint ssz, u8* tosearch);
u8* stringsearchleft7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz);
u8* stringsearchleftignorecase1 (string s, string tosearch);
u8* stringsearchleftignorecase2 (string s, u8* tosearch);
u8* stringsearchleftignorecase3 (u8* s, string tosearch);
u8* stringsearchleftignorecase4 (u8* s, u8* tosearch);
u8* stringsearchleftignorecase5 (u8* s, uint ssz, string tosearch);
u8* stringsearchleftignorecase6 (u8* s, uint ssz, u8* tosearch);
u8* stringsearchleftignorecase7 (u8* s, uint ssz, u8* tosearch, uint tosearchsz);

// This function lower the case
// of the string given as argument.
void stringlowercase (string* s);

// This function upper the case
// of the string given as argument.
void stringuppercase (string* s);

// The stringconverttoint() functions convert
// a string having an unsigned decimal, hexadecimal,
// octale or binary notation, to an integer.
// They stop at the first invalid character, or
// at the limit set by the argument sz when used.
// When u8** s for the string to convert is used,
// the address where the conversion stopped
// get updated in *s.
// A decimal notation number cannot start with 0.
// Overflow is ignored.
// Here below are examples of strings accepted:
// Decimal: 234
// Hexadecimal: 0xaa13
// Octale: 0o1234
// Binary: 0b101110
uint stringconverttoint1 (u8* s);
uint stringconverttoint2 (u8* s, uint sz);
uint stringconverttoint3 (u8** s);
uint stringconverttoint4 (u8** s, uint sz);

// Macro resolving to the number of digits
// needed for the encoding of the largest
// uint value, respectively to its decimal,
// hexadecimal, octale and binary notations.
#define STRINGMAXDECDIGITS ((sizeof(uint)*8)/4)
#define STRINGMAXHEXDIGITS ((sizeof(uint)*8)/4)
#define STRINGMAXOCTDIGITS ((sizeof(uint)*8)/3)
#define STRINGMAXBINDIGITS (sizeof(uint)*8)

// Functions converting their uint argument
// to their corresponding encoding. ei:
// respectively decimal, hexadecimal,
// octale and binary.
// Their argument buf must point to a buffer
// large enough for the targetted encoding.
// They return the number of digits
// that resulted from the conversion.
uint stringconverttodec_ (uint n, u8* buf); // The buffer size must be (STRINGMAXDECDIGITS+1).
uint stringconverttohex_ (uint n, u8* buf); // The buffer size must be (STRINGMAXHEXDIGITS+1).
uint stringconverttooct_ (uint n, u8* buf); // The buffer size must be (STRINGMAXOCTDIGITS+1).
uint stringconverttobin_ (uint n, u8* buf); // The buffer size must be (STRINGMAXBINDIGITS+1).

// Generate a string from the string
// given by the argument fmt, substituting
// anywhere format specifiers are used.
// For each format specifier used,
// other than %%, there must be
// corresponding arguments in
// the order in which they are used
// in the string argument fmt.
// A format specifier syntax is
// %[|][pad][width]specifier where
// items in brackets are optionals,
// and "specifier" is either
// i for a signed decimal number,
// d for an unsigned decimal number,
// x for an unsigned hexadecimal number,
// o for an unsigned octal number,
// b for an unsigned binary number,
// c for a character,
// s for a null-terminated string.
// "[width]" when used, is a decimal value
// which is the minimum number
// of characters for the substitute
// of the format specifier;
// if the value to substitute has
// less characters, pading is done.
// ei: "Value is :%10d".stringfmt(22);
// yield "Value is :        22";
// "[pad]" when used, is one or more characters,
// that cannot be 1 thru 9 or any of the specifier
// characters, unless escaped using '\';
// the first character is used for padding instead
// of the default space character, and the subsequent
// characters are padding overwrites between
// paddings and the value to substitute. ei:
// "Value is %-> [10d]".stringfmt(22);
// yield "Value is -----> [22]";
// Another example where escaping must be used:
// ei: "Value is %-> 0\\x10x]".stringfmt(22);
// yield "Value is ----> 0x16]";
// The backslash character itself is used
// by escaping it using backslash as well;
// ei: "Value is %-> \\\\10d]".stringfmt(22);
// yield "Value is -----> \22]";
// [|] when used, the value that substitute
// the format specifier is left aligned
// (instead of right aligned) within
// the given field [width];
// ei: "value is : %10d".stringfmt(22);
// yield "value is : 22        ";
// when [pad] is used, it is applied to the right;
// ei: "value is : [>%|-<]10d".stringfmt(22);
// yield "value is : [>22<]------";
string stringfmt (u8* fmt, ...);

// NOTE: To debug where functions returning a string were used,
// through MMDEBUG, a macro similar to the following can be used:
#define stringdbgduplicate1(str) ((string){.ptr = mmrealloc(stringduplicate1(str).ptr, -1)})
#define stringdbgduplicate2(str) ((string){.ptr = mmrealloc(stringduplicate2(str).ptr, -1)})
#define stringdbgduplicate3(str,sz) ((string){.ptr = mmrealloc(stringduplicate3(str,sz).ptr, -1)})
// All usage of "stringduplicate" must then be renamed to their corresponding "stringdbgduplicate".

#endif
