
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


// This function return how much memory in bytes has
// been allocated within the session given as argument.
// If flag == MMDOSUBSESSIONS, subsessions are
// included in the computation.
uint mmsessionusage (mmsession s, mmsessionflag flag) {
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in s.s set it at the beginning of the block.
	mmblock* b = (mmblock*)((void*)s.s - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	#ifdef MMCHECKSIGNATURE
	// If b point to a memory location that is not a block
	// which belong to this instance of the memory manager,
	// mmbadsignature() is called.
	if (b->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	uint usage = 0;
	
	if (flag == MMDOSUBSESSIONS) {
		#ifdef MMTHREADSAFE
		mutexlock(&mmsessionmutex);
		#endif
		
		mmsessioninternal* ss = mmcurrentsession;
		
		while (1) {
			#ifdef MMTHREADSAFE
			mutexlock(&ss->mutex);
			#endif
			
			b = (mmblock*)ss;
			
			while ((b = b->nextsessionblock) != (mmblock*)ss)
				usage += (b->size - (sizeof(mmblock) - 2*sizeof(mmblock*)));
			
			#ifdef MMTHREADSAFE
			mutexunlock(&ss->mutex);
			#endif
			
			if (ss == s.s) break;
			
			ss = ss->prevsession;
		}
		
		#ifdef MMTHREADSAFE
		mutexunlock(&mmsessionmutex);
		#endif
		
	} else {
		#ifdef MMTHREADSAFE
		mutexlock(&s.s->mutex);
		#endif
		
		b = (mmblock*)s.s;
		
		while ((b = b->nextsessionblock) != (mmblock*)s.s)
			usage += (b->size - (sizeof(mmblock) - 2*sizeof(mmblock*)));
		
		#ifdef MMTHREADSAFE
		mutexunlock(&s.s->mutex);
		#endif
	}
	
	return usage;
}
