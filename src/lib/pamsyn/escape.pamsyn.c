
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


// This function take as input a null-terminated string
// and return a string in which any characters
// that could be seen as a metacharacter is escaped.
// Useful when given a string containing characters
// that could be seen as metacharacter and which must
// be seen as regular characters before passing it
// to pamsyntokenize().
string pamsynescape (u8* pattern) {
	
	u8 c = *pattern;
	
	if (c) {
		
		string retvar = stringnull;
		
		do {
			// If the value of c is a metachar
			// I escape it by appending '\' .
			if (ismetachar(c)) stringappend4(&retvar, '\\');
			
			stringappend4(&retvar, c);
			
		} while (c = *++pattern);
		
		return retvar;
		
	} else return stringduplicate2("");
}
