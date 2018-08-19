
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


// Free the memory used by the object
// returned by pamsyntokenize().
void pamsynfree (pamsyntokenized t) {
	
	if (!t.tokenlist) return;
	
	uint i = (mmsz(t.tokenlist)/sizeof(token*));
	
	do {
		token* ttokenlisti = t.tokenlist[--i];
		
		u8* ttokenlististrptr = ttokenlisti->str.ptr;
		
		if (ttokenlististrptr) mmfree(ttokenlististrptr);
		
		uint* ttokenlistirepeatseq = ttokenlisti->repeat.seq;
		
		if (ttokenlistirepeatseq) mmfree(ttokenlistirepeatseq);
		
		mmfree(ttokenlisti);
		
	} while (i);
	
	mmfree(t.tokenlist);
}
