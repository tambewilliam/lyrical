
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


// This function return how much memory in bytes
// has been allocated accross all sessions.
uint mmusage () {
	
	uint usage = 0;
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmsessionmutex);
	#endif
	
	mmsessioninternal* s = mmcurrentsession;
	
	do {
		#ifdef MMTHREADSAFE
		mutexlock(&s->mutex);
		#endif
		
		mmblock* b = (mmblock*)s;
		
		while ((b = b->nextsessionblock) != (mmblock*)s)
			usage += (b->size - (sizeof(mmblock) - 2*sizeof(mmblock*)));
		
		#ifdef MMTHREADSAFE
		mutexunlock(&s->mutex);
		#endif
		
	} while (s = s->prevsession);
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmsessionmutex);
	#endif
	
	return usage;
}
