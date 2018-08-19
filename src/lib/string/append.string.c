
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


// These functions append s2 to s1.

void stringappend1 (string* s1, string s2) {
	
	if (!s2.ptr) return;
	
	uint s2sz = (mmsz(s2.ptr)-1);
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	// +1 account for the null terminating character.
	s1ptr = mmrealloc(s1ptr, (s1sz+s2sz)+1);
	
	// Copy the characters to append.
	// +1 account for the null terminating character.
	bytcpy(s1ptr+s1sz, s2.ptr, s2sz+1);
	
	s1->ptr = s1ptr;
}

void stringappend2 (string* s1, u8* s2) {
	
	uint s2sz = stringsz(s2);
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	// +1 account for the null terminating character.
	s1ptr = mmrealloc(s1ptr, (s1sz+s2sz)+1);
	
	// Copy the characters to append.
	// +1 account for the null terminating character.
	bytcpy(s1ptr+s1sz, s2, s2sz+1);
	
	s1->ptr = s1ptr;
}

void stringappend3 (string* s1, u8* s2, uint s2sz) {
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	uint sz = (s1sz+s2sz);
	
	// +1 account for the null terminating character.
	s1ptr = mmrealloc(s1ptr, sz+1);
	
	// Copy the characters to append.
	bytcpy(s1ptr+s1sz, s2, s2sz);
	
	// Set the null-terminating character.
	s1ptr[sz] = 0;
	
	s1->ptr = s1ptr;
}

void stringappend4 (string* s, u8 c) {
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	// +2 account for the size of
	// the character to append and the size
	// of the null-terminating character.
	sptr = mmrealloc(sptr, ssz+2);
	
	sptr[ssz] = c;
	
	// Set the null-terminating character.
	sptr[ssz+1] = 0;
	
	s->ptr = sptr;
}
