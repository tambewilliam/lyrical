
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


// This function extract from the memory region given
// by the arguments start and size, a match for the
// pamsyntokenized given as argument t and return it.
// Note that only the non-hidden characters
// of what was matched are extracted.
// stringnull is returned if a match could not be found.
string pamsynextract (pamsyntokenized t, u8* start, uint size) {
	
	if (!t.tokenlist) return stringnull;
	
	string s = stringnull;
	
	// Callback function used with pamsynmatch1().
	uint matchcallback (pamsynmatchfound* m) {
		// I check if a pamsynmatchfound
		// was given; if not I return 2 to tell
		// pamsynmatch1() to do a slow lookup.
		if (!m) return 2;
		
		// I remove all pamsynmatchfound
		// that are nochar from the linkedlist
		// given as argument, because they are only
		// useful to applications that do substitution.
		
		pamsynmatchfound* p; // Used to save the pointer of the previous pamsynmatchfound.
		
		pamsynmatchfound* n = m;
		
		while (1) {
			
			if (n->nochar) {
				// If n == m then I am the very first
				// pamsynmatchfound of the linkedlist.
				// p is not yet set.
				if (n == m) {
					
					if (n->next) m = n->next;
					// I get here if there is no pamsynmatchfound
					// left in the linkedlist.
					// I return 0 to tell pamsynmatch1()
					// to stop looking for matches.
					else return 0;
					
				// I update the value of the field next
				// of the previous match to the field next
				// of the current match since the current
				// match block pointed by n will be removed.
				// I do not have to check whether p is null
				// (pagefault if it is null) because this code
				// will surely always be executed after p is set.
				} else p->next = n->next;
				
				if (n = n->next) continue;
				else break;
				
			} else {
				
				if (n->next) {
					
					p = n; // Save the address of the current pamsynmatchfound.
					
					n = n->next;
					
					continue;
					
				} else break;
			}
		}
		
		// Concatenate all matches into s.
		do {
			u8* mstart = m->start;
			
			stringappend3(&s, mstart, ((uint)m->end - (uint)mstart) +1);
			
		} while ((m = m->next));
		
		// I return 0 to tell pamsynmatch1()
		// to stop looking for matches.
		return 0;
	}
	
	pamsynmatch1(t, start, size, matchcallback);
	
	return s;
}
