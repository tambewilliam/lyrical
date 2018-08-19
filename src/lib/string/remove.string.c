
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


// This function remove from the string given
// as argument, the string portion starting at
// the offset given as argument and which has
// the character count given by the argument sz.
// If the length of the string given as argument
// is null, or if the argument sz is null or
// if the string portion to remove go beyond
// boundary, nothing is done.
void stringremove (string* s, uint offset, uint sz) {
	
	if (!sz) return;
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	uint offsetplussz = offset+sz;
	
	if (!ssz || ((offsetplussz) > ssz)) return;
	
	u8* oldsptr = sptr;
	
	// mmrealloc() is used so that
	// the memory block get duplicated
	// if it was multi-referenced.
	sptr = mmrealloc(sptr, -1);
	
	if (sptr != oldsptr && offset) bytcpy(sptr, oldsptr, offset);
	
	uint newssz = ssz-sz;
	
	bytcpy(sptr+offset, oldsptr+offsetplussz, newssz-offset);
	
	// Set the null terminating character.
	sptr[newssz] = 0;
	
	// +1 account for the null-terminating character.
	s->ptr = mmrealloc(sptr, newssz+1);
}
