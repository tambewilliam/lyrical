
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


#ifndef LIBPARSEARG
#define LIBPARSEARG

// parseargshort() only parse short options.
// ei: myprogram -o -b value1 -xvalue2 -c;
// parsearglong() parse both short and long options.
// ei: myprogram --longoption value --anotherlongoption -o;
// With both functions, no memory allocation is used and
// the pointers to string passed to the callback functions
// come directly from the arguments themselves.
// Arguments that are "-" or "--" are not treated as options
// and are ignored by both parseargshort() and parsearglong().

// Take as argument:
// - A pointer to an array of strings. ei: arguments passed to the process.
// - A pointer to a function taking a u8 and a pointer to string as arguments.
// 	The u8 argument is the option and the string is
// 	the value of the option if the option could take a value.
// - A pointer to a string of short options which take a value.
void parseargshort1 (u8** arg, void(*callback)(u8 option, u8* value), u8* optionswithvalue);
// Same as parseargshort1(), but stop at
// the first argument that is not an option,
// returning the pointer at the argument
// where parsing stopped.
u8** parseargshort2 (u8** arg, void(*callback)(u8 option, u8* value), u8* optionswithvalue);

// Take as argument:
// - A pointer to an array of strings. ei: arguments passed to the process.
// - A pointer to a function taking 2 pointers to string as arguments.
// 	The first string is the option and the second string is
// 	the value of the option if the option could take a value.
// - A pointer to an array of strings, which are options which take a value.
void parsearglong1 (u8** arg, void(*callback)(u8* option, u8* value), u8** optionswithvalue);
// Same as parsearglong1(), but stop at
// the first argument that is not an option,
// returning the pointer at the argument
// where parsing stopped.
u8** parsearglong2 (u8** arg, void(*callback)(u8* option, u8* value), u8** optionswithvalue);

#endif
