
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


// Normal operators prepended by a byte
// for which the value denote their precedence.
// The lower the precedence value, the higher
// the precedence of the corresponding operator.
// The precedence value must be less than 0x0f
// as expected by readoperator() .
// Note that "<=" is specified before "<" to insure
// that "<=" get matched first otherwise "<" would
// be matched incorrectly when "<=" is encountered;
// the same logic is applied to other operators when
// they are similar to the way another operator begin.
// The enums following must be updated
// if the position of "=" or "+" change.
// ### This variable was declared static due to
// ### pagefault that would occur for unknown reasons.
static u8* normalop[] = {
	"\x07!=", "\x07==", "\x0e=", "\x0e+=", "\x04+", "\x0e-=", "\x04-",
	"\x0e%=", "\x03%", "\x0e^=", "\x09^", "\x0e/=", "\x03/", "\x0e*=", "\x03*",
	"\x0e<<=", "\x05<<", "\x06<=", "\x06<", "\x0e>>=", "\x05>>", "\x06>=", "\x06>",
	"\x0b&&", "\x0e&=", "\x08&", "\x0c||", "\x0e|=", "\x0a|", "\x0d?", 0
};

// Enum to retrieve the index
// of "=" from normalop.
enum {NORMALOPASSIGN = 2};

// Enum to retrieve the index
// of "+" from normalop.
enum {NORMALOPPLUS = 4};

// Prefix operators.
// ### This variable was declared static due to
// ### pagefault that would occur for unknown reasons.
static u8* prefixop[] = {
	// '&' is the operator used to
	// obtain the address of a variable.
	// '*' is the operator used
	// to dereference a variable.
	"&", "*", "++", "--", "-", "!", "?", "~", 0
};

// Postfix operators.
// The enum following must be updated
// if the position of "." change.
// ### This variable was declared static due to
// ### pagefault that would occur for unknown reasons.
static u8* postfixop[] = {
	".", "->", "[", "++", "--", 0
};

// Enum to retrieve the index
// of "." from postfixop.
enum {POSTFIXOPDOT = 0};


// I declare the array nativefcall which will
// contain the patterns used to match native operations.
// 
// Among the native types, "void" can only be used
// as the return type of a function or operator which
// do not return a value, or as a pointer type.
// 
// _______________________________
// Overloadable operators
// -------------------------------
// One argument operators:
// ++ -- ~ ? ! -
// 
// Two arguments operators:
// |= | ^= ^ &= & -= - += + %= % /= / *= * <<= << <= < >>= >> >= > != == =
// 
// I surround all the pattern with the metachar < and >;
// because I am matching against a finite string; and
// the entire string has to be correct and match.
// 
// Pattern used to match enum types which
// are not pointer nor array:
// "\#+0-*[a-z0-9]"
// 
// Pattern used to match named/unamed types which can be non-pointer,
// pointer (Pointer to function included) or array:
// "[a-z0-9]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}"
// 
// From the above pattern I derive the pattern used to match
// types which can be enum, non-pointer, pointer (Pointer
// to function included) or array:
// "[\#a-z0-9]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}"
// 
// I also derive the pattern used to match types which
// are only pointer (Pointer to function included):
// "[\#a-z0-9]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}[)\*]"
// 
// Keep in mind that in the above three patterns, the characters
// , . ( ) & are used only in pointer to function types.
// 
// Note that in the pattern string, the name of the operator or
// function and its arguments are separated by '|' instead of
// commas, opening and closing paranthesis which would create
// conflict since those same characters can be used in the portion
// of the pattern used to match a type, ei: pointer to function type.
// Note also that in all pattern string the escaping metachar '\'
// is itself escaped because the compiler uses it as well for escaping.
pamsyntokenized nativefcall[14];

// Increment and decrement operators for native types and pointers.
nativefcall[0] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\+\\+,\\-\\-}"
		"|"
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\+\\+,\\-\\-}"
		"|"
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\+\\+,\\-\\-}"
		"|"
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\+\\+,\\-\\-}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	""
);

// Negate operator for native types.
nativefcall[1] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"\\-"
		"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"\\-"
		"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"\\-"
		"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"\\-"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Operator "not" and "istrue" for native types and pointers.
nativefcall[2] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\!,\\?}"
		"|"
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\!,\\?}"
		"|"
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\!,\\?}"
		"|"
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\!,\\?}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"|"
		">" :
	""
);

// Bitwise "not" for native types and enum types.
nativefcall[3] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"~"
		"|"
			"{s8,u8,\\#+0-*[a-z0-9]}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"~"
		"|"
			"{s8,s16,u8,u16,\\#+0-*[a-z0-9]}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"~"
		"|"
			"{s8,s16,s32,u8,u16,u32,\\#+0-*[a-z0-9]}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"~"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,\\#+0-*[a-z0-9]}"
		"|"
		">" :
	""
);

// Bitwise shift operators for native types
// and enum types, where the second argument
// is always a native type.
nativefcall[4] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\<\\<,\\>\\>}"
		"|"
			"{s8,u8,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\<\\<,\\>\\>}"
		"|"
			"{s8,s16,u8,u16,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\<\\<,\\>\\>}"
		"|"
			"{s8,s16,s32,u8,u16,u32,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\<\\<,\\>\\>}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Multiply, divide and modulo for native types.
nativefcall[5] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\*,/,\\%}"
		"|"
			"{s8,u8}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\*,/,\\%}"
		"|"
			"{s8,s16,u8,u16}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\*,/,\\%}"
		"|"
			"{s8,s16,s32,u8,u16,u32}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\*,/,\\%}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Plus and minus operators for native types and pointers.
// Only the first argument can be a pointer.
nativefcall[6] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\+,\\-}"
		"|"
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\+,\\-}"
		"|"
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\+,\\-}"
		"|"
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\+,\\-}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Bitwise operators for native types and enum types.
nativefcall[7] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{&,^,|}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,u8}"
			"|"
			"{s8,u8}"
		"}|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{&,^,|}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,u8,u16}"
			"|"
			"{s8,s16,u8,u16}"
		"}|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{&,^,|}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,u8,u16,u32}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"}|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{&,^,|}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"}|"
		">" :
	""
);

// Comparison operators for native types, enum and pointers.
// Comparison can be done between two pointers, or between
// a pointer and an integer, or between an integer
// and a pointer, or between two integers, or between
// two identical enum types.
nativefcall[8] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\<,\\<=,\\>,\\>=,==,\\!=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"}|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\<,\\<=,\\>,\\>=,==,\\!=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"}|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\<,\\<=,\\>,\\>=,==,\\!=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"}|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\<,\\<=,\\>,\\>=,==,\\!=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
		"}|"
		">" :
	""
);

// Enum to specify the index
// of the assignment operator
// from nativefcall.
enum {NATIVEFCALLASSIGN = 9};

// Assignment operator.
// All native integer types are compatible with each other.
// An assignment is allowed between non pointer types that
// are not native types only if their name match exactely.
// An assignment can also be done from a native
// integer type to a pointer type.
// Assignment between incompatible pointers cannot be done.
// "void*" is compatible with any type of pointer.
// Example of compatible pointers are:
// "void*" is compatible with any pointer type such as "uint*" or "uint(u8)";
// "uint(void*)" is compatible only with another "uint(void*)", and
// the "void*" type used within the paranthesis of the pointer
// to function type specification has no effect.
nativefcall[NATIVEFCALLASSIGN] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"="
		"|{"
			"{s8,u8}"
			"|"
			"{s8,u8}"
		","
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
			"|"
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
		","
			"void\\*"
			"|"
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
		","
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
			"|"
			"{void\\*,s8,u8}"
		"}|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"="
		"|{"
			"{s8,s16,u8,u16}"
			"|"
			"{s8,s16,u8,u16}"
		","
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
			"|"
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
		","
			"void\\*"
			"|"
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
		","
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
			"|"
			"{void\\*,s8,s16,u8,u16}"
		"}|"
		">" :
	
	(sizeofgpr == 4) ?
		"<"
		"="
		"|{"
			"{s8,s16,s32,u8,u16,u32}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		","
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
			"|"
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
		","
			"void\\*"
			"|"
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
		","
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
			"|"
			"{void\\*,s8,s16,s32,u8,u16,u32}"
		"}|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"="
		"|{"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		","
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
			"|"
			"%{[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}}"
		","
			"void\\*"
			"|"
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
		","
			"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]"
			"|"
			"{void\\*,s8,s16,s32,s64,u8,u16,u32,u64}"
		"}|"
		">" :
	""
);

// Self bitwise shift operators for native types and enum types.
nativefcall[10] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\<\\<=,\\>\\>=}"
		"|"
			"{s8,u8,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\<\\<=,\\>\\>=}"
		"|"
			"{s8,s16,u8,u16,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\<\\<=,\\>\\>=}"
		"|"
			"{s8,s16,s32,u8,u16,u32,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\<\\<=,\\>\\>=}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,\\#+0-*[a-z0-9]}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Self multiply, divide and modulo for native types.
nativefcall[11] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\*=,/=,\\%=}"
		"|"
			"{s8,u8}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\*=,/=,\\%=}"
		"|"
			"{s8,s16,u8,u16}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\*=,/=,\\%=}"
		"|"
			"{s8,s16,s32,u8,u16,u32}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\*=,/=,\\%=}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Self plus and minus operators for native types and pointers.
// Only the first argument can be a pointer.
nativefcall[12] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{\\+=,\\-=}"
		"|"
			"{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,u8}"
		"|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{\\+=,\\-=}"
		"|"
			"{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,u8,u16}"
		"|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{\\+=,\\-=}"
		"|"
			"{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{\\+=,\\-=}"
		"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"|"
		">" :
	""
);

// Self bitwise operators for native types and enum types.
nativefcall[13] = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<"
		"{&=,^=,|=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,u8}"
			"|"
			"{s8,u8}"
		"}|"
		">" :
	(sizeofgpr == 2) ?
		"<"
		"{&=,^=,|=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,u8,u16}"
			"|"
			"{s8,s16,u8,u16}"
		"}|"
		">" :
	(sizeofgpr == 4) ?
		"<"
		"{&=,^=,|=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,u8,u16,u32}"
			"|"
			"{s8,s16,s32,u8,u16,u32}"
		"}|"
		">" :
	(sizeofgpr == 8) ?
		"<"
		"{&=,^=,|=}"
		"|{"
			"%{\\#+0-*[a-z0-9]}"
			"|"
			"%{\\#+0-*[a-z0-9]}"
		","
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
			"|"
			"{s8,s16,s32,s64,u8,u16,u32,u64}"
		"}|"
		">" :
	""
);


// Pattern used to determine if the declaration of an operator function was correct; The pattern,
// depending on the operator determine whether the right number of arguments was given.
// The pattern created is similar to the pattern found in the field fcall of a lyricalfunction.
// Not all operators are overloadable. The operators not overloadable are reserved for exclusive use of the
// language and should not have their meaning changed. Those operators are: [] . -> * & && || ?:
// 
// When overloading a function or operator, the return type is not part of the signature.
// In other words, it do not matter whether the return types are the same or differents, as long
// as their arguments count are differents or the type of their arguments are differents in order
// to distinguish which function to call; hence the signature of a function or operator
// only contain the name of the function or operator followed by the arguments types.
// 
// To make it easier to understand how the entire pattern is constructed, here below are the
// different portions of the final pattern pointed by iscorrectopdeclaration.
// 
// Here below is what I used to match types which
// can be enums, non-pointers, pointers or arrays.
// "[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}"
// 
// Operators taking one argument:
// ++ -- ~ ? ! -
// Their names have the form: "op|type|"
// pattern match: "<{\+\+,\-\-,~,\?,\!}|[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}|>"
// 
// Operator taking two arguments which are basically all the normal operators available:
// |= | ^= ^ &= & -= - += + %= % /= / *= * <<= << <= < >>= >> >= > != == =
// Their names have the form: "op|type|type|"
// pattern match: "<{\<\<,\>\>,\*,/,\%,\+,\-,&,^,|,\<,\<=,\>,\>=,==,\!=,=,\<\<=,\>\>=,\*=,/=,\%=,\+=,\-=,&=,^=,|=}|[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}|[\#a-z]+0-*{[\,\.()&\*a-z0-9],\[[1-9]+0-*[0-9]\]}|>"
// 
// Note that "<=" is specified before "<" to insure that "<=" get matched first
// otherwise "<" would be matched incorrectly when "<=" is encountered; the same logic
// is applied to other operators when they are similar to the way another operator begin.
// 
// Note that in the pattern string, the name of the operator or function and its arguments are
// separated by '|' instead of commas, opening and closing paranthesis which would create
// conflict since those same characters can be used in the portion of the pattern used
// to match a type, ei: pointer to function type.
// Note also that in the final pattern assigned to iscorrectopdeclaration below, I made sure
// to escape the escaping character '\' itself which is used by C as well to do escaping.
// This pattern is only used within funcdeclaration();
pamsyntokenized iscorrectopdeclaration = pamsyntokenize(
"{"
	"<"
	"{\\+\\+,\\-\\-,~,\\?,\\!,\\-}"
	"|"
	"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}"
	"|"
	">"
","
	"<"
	"{|=,|,^=,^,&=,&,\\-=,\\-,\\+=,\\+,\\%=,\\%,/=,/,*=,*,\\<\\<=,\\<\\<,\\<=,\\<,\\>\\>=,\\>\\>,\\>=,\\>,\\!=,==,=}"
	"|"
	"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}"
	"|"
	"[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}"
	"|"
	">"
"}"
);

// Pattern used to check if an operator read
// is an overloadable operator; it is used
// when declaring an operator.
// Note that "<=" is specified before "<"
// to insure that "<=" get matched first
// otherwise "<" would be matched incorrectly
// when "<=" is encountered; the same logic
// is applied to other operators when they are
// similar to the way another operator begin.
pamsyntokenized overloadableop = pamsyntokenize("<{|=,|,^=,^,&=,&,\\-=,\\-\\-,\\-,\\+=,\\+\\+,\\+,\\%=,\\%,/=,/,\\*=,\\*,\\<\\<=,\\<\\<,\\<=,\\<,\\>\\>=,\\>\\>,\\>=,\\>,\\!=,\\!,==,=,~,\\?}");

// Pattern used to check the type of variables used
// with the operators "&&" "||" "?:" and the type of variables
// used with control statements if(), while(), do while();
// The type can only be a native type or pointer (pointer to function included).
pamsyntokenized isnativeorpointertype = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<{s8,u8,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}>" :
	(sizeofgpr == 2) ?
		"<{s8,s16,u8,u16,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}>" :
	(sizeofgpr == 4) ?
		"<{s8,s16,s32,u8,u16,u32,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}>" :
	(sizeofgpr == 8) ?
		"<{s8,s16,s32,s64,u8,u16,u32,u64,[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]}>" :
	""
);

// Pattern used to check whether I have a native type.
pamsyntokenized isnativetype = pamsyntokenize(
	(sizeofgpr == 1) ?
		"<{s8,u8}>" :
	(sizeofgpr == 2) ?
		"<{s8,s16,u8,u16}>" :
	(sizeofgpr == 4) ?
		"<{s8,s16,s32,u8,u16,u32}>" :
	(sizeofgpr == 8) ?
		"<{s8,s16,s32,s64,u8,u16,u32,u64}>" :
	""
);

// Pattern used to check if a symbol is a keyword.
pamsyntokenized iskeyword = pamsyntokenize("<{this,retvar,return,catch,throw,sizeof,offsetof,typeof,switch,case,default,break,continue,while,do,if,else,goto,operator,struct,pstruct,union,enum,static,asm,export,void,{s,u}{8,16,32,64}}>");

// This pattern is used to extract the field offset suffixed to a variable if any.
pamsyntokenized matchoffsetifvarfield = pamsyntokenize("#{\\.}+[0-9]>");

// This pattern is used to determine whether
// a variable is a tempvar or is a variable which
// depend on the tempvar (dereference variable,
// address variable or variable with
// its name suffixed with an offset).
pamsyntokenized matchtempvarname = pamsyntokenize("$+[0-9]$");

// Native types.
// I only define their name, size and field v.
// Note that the type void is given a size of 1,
// although void can never be used to declare
// a variable, only a void pointer can be used.
// Setting its size to 1 allow functions
// such as stride() to work correctly.
lyricaltype nativetype[] = {
	// The enum down below must be
	// updated when this array is modified.
	// largeenoughunsignednativetype()
	// must always be updated.
	{.name = stringduplicate2("void"), .size = 1, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("s8"), .size = 1, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("s16"), .size = 2, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("s32"), .size = 4, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("s64"), .size = 8, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("u8"), .size = 1, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("u16"), .size = 2, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("u32"), .size = 4, .v = 0, .scopedepth = 0, .scope = 0},
	{.name = stringduplicate2("u64"), .size = 8, .v = 0, .scopedepth = 0, .scope = 0}
};

// Enum for which the value is used
// to index the array nativetype.
enum {
	NATIVETYPEVOID,
	NATIVETYPES8,
	NATIVETYPES16,
	NATIVETYPES32,
	NATIVETYPES64,
	NATIVETYPEU8,
	NATIVETYPEU16,
	NATIVETYPEU32,
	NATIVETYPEU64
};

string u8ptrstr = stringduplicate2("u8*");
string voidptrstr = stringduplicate2("void*");
string voidfncstr = stringduplicate2("void()");

// Mask used to extract the least significant bits
// that matter in the shift amount used with
// the target instructions sll, srl and sra.
u64 targetshiftamountmask = (((u64)1<<clog2(bitsizeofgpr))-1);

// Max value that the host uint can have.
u64 maxhostuintvalue = (((sizeof(uint) < sizeof(u64)) ? ((u64)1<<(8*sizeof(uint))) : (u64)0) -(u64)1);

// Max value that the target uint can have.
u64 maxtargetuintvalue = (((bitsizeofgpr < (8*sizeof(u64))) ? ((u64)1<<bitsizeofgpr) : (u64)0) -(u64)1);

// ((u64)1<<((sizeofgpr*8)-1)) is a u64
// with its most significant bit 1.
u64 msbone = ((u64)1<<((sizeofgpr*8)-1));
