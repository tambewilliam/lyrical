
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


// This function split the string given
// as argument at the offset given as argument,
// and return the bottom part of the split.
// stringnull is returned if the offset
// is greater than or equal to the number of
// characters in the string given as argument.
string stringsplit (string* s, uint offset) {
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	if (offset >= ssz) return stringnull;
	
	uint retvarsz = (ssz-offset);
	
	// +1 account for the null-terminating character.
	string retvar = {.ptr = mmalloc(retvarsz+1)};
	
	// Copy the bottom split to its new location.
	// +1 account for the null-terminating character.
	bytcpy(retvar.ptr, sptr+offset, retvarsz+1);
	
	// Truncate the top split.
	// +1 account for the null-terminating character.
	sptr = mmrealloc(sptr, offset+1);
	
	// Set the null terminating character.
	sptr[offset] = 0;
	
	s->ptr = sptr;
	
	return retvar;
}
