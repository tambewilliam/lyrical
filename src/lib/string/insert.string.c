
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


// These functions insert s2 in s1
// at the offset given as argument.
// If offset is greater than
// the size of s1, nothing is done.

void stringinsert1 (string* s1, string s2, uint offset) {
	
	if (!s2.ptr) return;
	
	uint s2sz = (mmsz(s2.ptr)-1);
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	if (offset == s1sz) {
		stringappend1(s1, s2);
		return;
	}
	
	if (offset > s1sz) return;
	
	// +1 account for the null-terminating character.
	s1ptr = mmrealloc(s1ptr, (s1sz+s2sz)+1);
	
	u8* s1ptroffsetted = s1ptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	// +1 account for the null-terminating character.
	bytcpy(s1ptroffsetted+s2sz, s1ptroffsetted, (s1sz-offset)+1);
	
	// Do the insertion.
	bytcpy(s1ptroffsetted, s2.ptr, s2sz);
	
	s1->ptr = s1ptr;
}

void stringinsert2 (string* s1, u8* s2, uint offset) {
	
	uint s2sz = stringsz(s2);
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	if (offset == s1sz) {
		stringappend2(s1, s2);
		return;
	}
	
	if (offset > s1sz) return;
	
	// +1 account for the null-terminating character.
	s1ptr = mmrealloc(s1ptr, (s1sz+s2sz)+1);
	
	u8* s1ptroffsetted = s1ptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	// +1 account for the null-terminating character.
	bytcpy(s1ptroffsetted+s2sz, s1ptroffsetted, (s1sz-offset)+1);
	
	// Do the insertion.
	bytcpy(s1ptroffsetted, s2, s2sz);
	
	s1->ptr = s1ptr;
}

void stringinsert3 (string* s1, u8* s2, uint s2sz, uint offset) {
	
	if (!s2sz) return;
	
	u8* s1ptr = s1->ptr;
	
	uint s1sz = s1ptr ? (mmsz(s1ptr)-1) : 0;
	
	if (offset == s1sz) {
		stringappend3(s1, s2, s2sz);
		return;
	}
	
	if (offset > s1sz) return;
	
	// +1 account for the null-terminating character.
	s1ptr = mmrealloc(s1ptr, (s1sz+s2sz)+1);
	
	u8* s1ptroffsetted = s1ptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	// +1 account for the null-terminating character.
	bytcpy(s1ptroffsetted+s2sz, s1ptroffsetted, (s1sz-offset)+1);
	
	// Do the insertion.
	bytcpy(s1ptroffsetted, s2, s2sz);
	
	s1->ptr = s1ptr;
}

void stringinsert4 (string* s, u8 c, uint offset) {
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	if (offset == ssz) {
		stringappend4(s, c);
		return;
	}
	
	if (offset > ssz) return;
	
	// +2 account for the character to insert
	// and the null-terminating character.
	sptr = mmrealloc(sptr, ssz+2);
	
	u8* sptroffsetted = sptr+offset;
	
	// Move to the right, the block on the right
	// of the position where insertion will be done.
	bytcpy(sptroffsetted+1, sptroffsetted, ssz-offset);
	
	// Do the insertion.
	*sptroffsetted = c;
	
	// Set the null terminating character.
	sptr[ssz+1] = 0;
	
	s->ptr = sptr;
}
