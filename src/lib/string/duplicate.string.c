
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


// These functions return a string duplicate.


string stringduplicate3 (u8* s, uint sz) {
	// +1 account for the null-terminating character.
	u8* ptr = mmalloc(sz+1);
	
	bytcpy(ptr, s, sz);
	
	ptr[sz] = 0;
	
	return ((string){.ptr = ptr});
}


string stringduplicate1 (string s) {
	
	if (!s.ptr) return stringnull;
	
	#ifdef STRINGNOREFCNT
	s = stringduplicate3(s.ptr, mmsz(s.ptr)-1);
	#else
	mmrefup(s.ptr);
	#endif
	
	return s;
}


string stringduplicate2 (u8* s) {
	// +1 account for the null-terminating character.
	uint ssz = stringsz(s)+1;
	
	u8* ptr = mmalloc(ssz);
	
	bytcpy(ptr, s, ssz);
	
	return ((string){.ptr = ptr});
}


string stringduplicate4 (u8 c) {
	// +1 account for the null-terminating character.
	u8* ptr = mmalloc(2);
	
	ptr[0] = c;
	
	ptr[1] = 0;
	
	return ((string){.ptr = ptr});
}
