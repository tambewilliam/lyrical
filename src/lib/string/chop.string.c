
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


// This function remove the character
// count given by the argument sz
// from the end of the argument string s.
// Nothing is done if the argument sz
// is null or greater than the size
// of the argument string s.
void stringchop (string* s, uint sz) {
	
	u8* sptr = s->ptr;
	
	uint ssz = sptr ? (mmsz(sptr)-1) : 0;
	
	if (!sz || sz > ssz) return;
	
	ssz -= sz;
	
	// +1 account for the null terminating character.
	sptr = mmrealloc(sptr, ssz+1);
	
	// Set the null-terminating character.
	sptr[ssz] = 0;
	
	s->ptr = sptr;
}
