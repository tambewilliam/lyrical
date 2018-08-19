
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


// This function truncate the argument
// string s to the character count
// given by the argument sz.
// Nothing is done if the argument sz
// is greater than or equal to
// the size of the argument string s.
void stringtruncate (string* s, uint sz) {
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	if (sz >= ssz) return;
	
	// +1 account for the null terminating character.
	sptr = mmrealloc(sptr, sz+1);
	
	// Set the null-terminating character.
	sptr[sz] = 0;
	
	s->ptr = sptr;
}
