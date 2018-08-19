
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

#include <string.h>

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
void parseargshort1 (u8** arg, void(*callback)(u8 option, u8* value), u8* optionswithvalue) {
	// Remove *arg from the array of arguments.
	// There must be at least one argument in the array.
	void detacharg () {
		
		uint i = 0;
		
		do {
			arg[i] = arg[i+1];
			
			++i;
			
		} while (arg[i]);
	}
	
	parseargument:
	
	if (*arg) {
		
		uint i = 0;
		
		// I assume that the shell removed
		// unnecessary spaces at the beginning
		// of the argument.
		if ((*arg)[i] == '-') { // Check for a short option which start with "-".
			
			++i;
			
			u8 o = (*arg)[i]; // Save the option name.
			
			// Ignore an argument that is "-".
			if (!o) {
				
				++arg; // Select the next argument.
				
				goto parseargument;
			}
			
			do {
				uint j = 0;
				
				// I check if the option is not among
				// options which can take a value.
				while (optionswithvalue[j]) {
					// If the test below is true then
					// this short option take an argument.
					if (o == optionswithvalue[j]) {
						
						++i;
						
						u8* v;
						
						if ((*arg)[i]) {
							// If I get here then the value of the
							// short option is the rest of the argument.
							
							v = &(*arg)[i]; // Save the address where the value start.
							
							detacharg();
							
						} else {
							// If I get here then the value of the
							// short option is the next argument.
							
							detacharg();
							
							// After detaching the current argument, arg will point
							// to the next argument which could be a null pointer; but
							// that is ok; if needed the programmer should check that
							// a value was given.
							v = *arg;
							
							// If *arg was really an argument
							// and not a null pointer to mark
							// the end of the array of strings,
							// I need to detach it.
							if (v) detacharg();
						}
						
						(*callback)(o, v);
						
						goto parseargument;
					}
					
					++j;
				}
				
				// If I get here, then the option do not take a value.
				
				(*callback)(o, (u8*)0);
				
				++i;
				
			} while (o = (*arg)[i]);
			
			detacharg();
			
		} else ++arg; // Select the next argument.
		
		goto parseargument;
	}
}


// Same as parseargshort1(), but stop at
// the first argument that is not an option,
// returning the pointer at the argument
// where parsing stopped.
u8** parseargshort2 (u8** arg, void(*callback)(u8 option, u8* value), u8* optionswithvalue) {
	// Remove *arg from the array of arguments.
	// There must be at least one argument in the array.
	void detacharg () {
		
		uint i = 0;
		
		do {
			arg[i] = arg[i+1];
			
			++i;
			
		} while (arg[i]);
	}
	
	parseargument:
	
	if (*arg) {
		
		uint i = 0;
		
		// I assume that the shell removed
		// unnecessary spaces at the beginning
		// of the argument.
		if ((*arg)[i] == '-') { // Check for a short option which start with "-".
			
			++i;
			
			u8 o = (*arg)[i]; // Save the option name.
			
			// Ignore an argument that is "-".
			if (!o) return arg;
			
			do {
				uint j = 0;
				
				// I check if the option is not among
				// options which can take a value.
				while (optionswithvalue[j]) {
					// If the test below is true then
					// this short option take an argument.
					if (o == optionswithvalue[j]) {
						
						++i;
						
						u8* v;
						
						if ((*arg)[i]) {
							// If I get here then the value of the
							// short option is the rest of the argument.
							
							v = &(*arg)[i]; // Save the address where the value start.
							
							detacharg();
							
						} else {
							// If I get here then the value of the
							// short option is the next argument.
							
							detacharg();
							
							// After detaching the current argument, arg will point
							// to the next argument which could be a null pointer; but
							// that is ok; if needed the programmer should check that
							// a value was given.
							v = *arg;
							
							// If *arg was really an argument
							// and not a null pointer to mark
							// the end of the array of strings,
							// I need to detach it.
							if (v) detacharg();
						}
						
						(*callback)(o, v);
						
						goto parseargument;
					}
					
					++j;
				}
				
				// If I get here, then the option do not take a value.
				
				(*callback)(o, (u8*)0);
				
				++i;
				
			} while (o = (*arg)[i]);
			
			detacharg();
			
		} else return arg;
		
		goto parseargument;
		
	} else return arg;
}


// Take as argument:
// - A pointer to an array of strings. ei: arguments passed to the process.
// - A pointer to a function taking 2 pointers to string as arguments.
// 	The first string is the option and the second string is
// 	the value of the option if the option could take a value.
// - A pointer to an array of strings, which are options which take a value.
void parsearglong1 (u8** arg, void(*callback)(u8* option, u8* value), u8** optionswithvalue) {
	// Remove *arg from the array of arguments.
	// There must be at least one argument in the array.
	void detacharg () {
		
		uint i = 0;
		
		do {
			arg[i] = arg[i+1];
			
			++i;
			
		} while (arg[i]);
	}
	
	parseargument:
	
	if (*arg) {
		
		uint i = 0;
		
		// Long options must be checked first,
		// because short options start the same way.
		// I assume that the shell removed
		// unnecessary spaces at the beginning
		// of the argument.
		if ((*arg)[i] == '-' && (*arg)[i+1] == '-') { // Check for a long option which start with "--".
			
			i += 2;
			
			u8* o = &(*arg)[i]; // Save the option name.
			
			// Ignore an argument that is "--".
			if (!*o) {
				
				++arg; // Select the next argument.
				
				goto parseargument;
			}
			
			uint j = 0;
			
			// I check if the option is not among
			// options which can take a value.
			while (optionswithvalue[j]) {
				// If the test below is true then
				// this long option take an argument.
				if (stringiseq3(o, optionswithvalue[j])) {
					// The short option value is the next argument.
					
					detacharg();
					
					// After detaching the current argument, arg will point
					// to the next argument which could be a null pointer; but
					// that is ok; if needed the programmer should check that
					// a value was given.
					u8* v = *arg;
					
					// If *arg was really an argument
					// and not a null pointer to mark
					// the end of the array of strings,
					// I need to detach it.
					if (v) detacharg();
					
					(*callback)(o, v);
					
					goto parseargument;
				}
				
				++j;
			}
			
			// If I get here, then the option do not take a value.
			
			detacharg();
			
			(*callback)(o, (u8*)0);
			
			goto parseargument;
			
		} else if ((*arg)[i] == '-') { // Check for a short option which start with "-".
			
			++i;
			
			u8 o[2] = {[1] = 0}; // Used to save the option name.
			
			// Ignore an argument that is "-".
			if (!(o[0] = (*arg)[i])) {
				
				++arg; // Select the next argument.
				
				goto parseargument;
			}
			
			do {
				uint j = 0;
				
				// I check if the option is not among
				// options which can take a value.
				while (optionswithvalue[j]) {
					// If the test below is true then
					// this short option take an argument.
					if (stringiseq3(o, optionswithvalue[j])) {
						
						++i;
						
						u8* v;
						
						if ((*arg)[i]) {
							// If I get here then the value of the
							// short option is the rest of the argument.
							
							v = &(*arg)[i]; // Save the address where the value start.
							
							detacharg();
							
						} else {
							// If I get here then the value of the
							// short option is the next argument.
							
							detacharg();
							
							// After detaching the current argument, arg will point
							// to the next argument which could be a null pointer; but
							// that is ok; if needed the programmer should check that
							// a value was given.
							v = *arg;
							
							// If *arg was really an argument
							// and not a null pointer to mark
							// the end of the array of strings,
							// I need to detach it.
							if (v) detacharg();
						}
						
						(*callback)(o, v);
						
						goto parseargument;
					}
					
					++j;
				}
				
				// If I get here, then the option do not take a value.
				
				(*callback)(o, (u8*)0);
				
				++i;
				
			} while (o[0] = (*arg)[i]);
			
			detacharg();
			
		} else ++arg; // Select the next argument.
		
		goto parseargument;
	}
}


// Same as parsearglong1(), but stop at
// the first argument that is not an option,
// returning the pointer at the argument
// where parsing stopped.
u8** parsearglong2 (u8** arg, void(*callback)(u8* option, u8* value), u8** optionswithvalue) {
	// Remove *arg from the array of arguments.
	// There must be at least one argument in the array.
	void detacharg () {
		
		uint i = 0;
		
		do {
			arg[i] = arg[i+1];
			
			++i;
			
		} while (arg[i]);
	}
	
	parseargument:
	
	if (*arg) {
		
		uint i = 0;
		
		// Long options must be checked first,
		// because short options start the same way.
		// I assume that the shell removed
		// unnecessary spaces at the beginning
		// of the argument.
		if ((*arg)[i] == '-' && (*arg)[i+1] == '-') { // Check for a long option which start with "--".
			
			i += 2;
			
			u8* o = &(*arg)[i]; // Save the option name.
			
			// Ignore an argument that is "--".
			if (!*o) return arg;
			
			uint j = 0;
			
			// I check if the option is not among
			// options which can take a value.
			while (optionswithvalue[j]) {
				// If the test below is true then
				// this long option take an argument.
				if (stringiseq3(o, optionswithvalue[j])) {
					// The short option value is the next argument.
					
					detacharg();
					
					// After detaching the current argument, arg will point
					// to the next argument which could be a null pointer; but
					// that is ok; if needed the programmer should check that
					// a value was given.
					u8* v = *arg;
					
					// If *arg was really an argument
					// and not a null pointer to mark
					// the end of the array of strings,
					// I need to detach it.
					if (v) detacharg();
					
					(*callback)(o, v);
					
					goto parseargument;
				}
				
				++j;
			}
			
			// If I get here, then the option do not take a value.
			
			detacharg();
			
			(*callback)(o, (u8*)0);
			
			goto parseargument;
			
		} else if ((*arg)[i] == '-') { // Check for a short option which start with "-".
			
			++i;
			
			u8 o[2] = {[1] = 0}; // Used to save the option name.
			
			// Ignore an argument that is "-".
			if (!(o[0] = (*arg)[i])) return arg;
			
			do {
				uint j = 0;
				
				// I check if the option is not among
				// options which can take a value.
				while (optionswithvalue[j]) {
					// If the test below is true then
					// this short option take an argument.
					if (stringiseq3(o, optionswithvalue[j])) {
						
						++i;
						
						u8* v;
						
						if ((*arg)[i]) {
							// If I get here then the value of the
							// short option is the rest of the argument.
							
							v = &(*arg)[i]; // Save the address where the value start.
							
							detacharg();
							
						} else {
							// If I get here then the value of the
							// short option is the next argument.
							
							detacharg();
							
							// After detaching the current argument, arg will point
							// to the next argument which could be a null pointer; but
							// that is ok; if needed the programmer should check that
							// a value was given.
							v = *arg;
							
							// If *arg was really an argument
							// and not a null pointer to mark
							// the end of the array of strings,
							// I need to detach it.
							if (v) detacharg();
						}
						
						(*callback)(o, v);
						
						goto parseargument;
					}
					
					++j;
				}
				
				// If I get here, then the option do not take a value.
				
				(*callback)(o, (u8*)0);
				
				++i;
				
			} while (o[0] = (*arg)[i]);
			
			detacharg();
			
		} else return arg;
		
		goto parseargument;
		
	} else return arg;
}
