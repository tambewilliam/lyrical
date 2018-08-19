
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


// This function lower the case
// of the string given as argument.
void stringlowercase (string* s) {
	// mmrealloc() is used so that
	// the memory block get duplicated
	// if it was multi-referenced.
	u8* sptr = mmrealloc(s->ptr, -1);
	
	u8 c;
	
	while (c = *sptr) {
		
		if (c >= 'A' && c <= 'Z') *sptr += ('a'-'A');
		
		++sptr;
	}
	
	s->ptr = sptr;
}
