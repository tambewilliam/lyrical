
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


void mmsessioncancel (mmsession s, mmsessionflag flag) {
	
	mmblock* b;
	
	// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
	// the address in s.s set it at the beginning of the block.
	b = (mmblock*)((void*)s.s - (sizeof(mmblock) - 2*sizeof(mmblock*)));
	
	#ifdef MMCHECKSIGNATURE
	// If b point to a memory location that is not a block
	// which belong to this instance of the memory manager,
	// mmbadsignature() is called.
	if (b->signature != (uint)&mmarrayofpointerstolinkedlistoffreeblocks)
		mmbadsignature();
	#endif
	
	#ifdef MMTHREADSAFE
	mutexlock(&mmsessionmutex);
	#endif
	
	if (flag == MMDOSUBSESSIONS) {
		// s.s is set to the parent session of
		// the session that will be canceled.
		s.s = s.s->prevsession;
		
		#ifdef MMTHREADSAFE
		mutexlock(&s.s->mutex);
		#endif
		
		// This loop move all blocks out of the current session
		// all the way up to the session pointed by s.s .
		// Locking is not used on each session to cancel,
		// because they should no longer be in use.
		while (mmcurrentsession != s.s) {
			// I move all blocks of the current session
			// to the parent session of the session that
			// will be canceled.
			
			b = mmcurrentsession->nextsessionblock;
			
			while (b != (mmblock*)mmcurrentsession) {
				// Since I am going to remove the block pointed
				// by b from the current session, I need to get
				// the address of the next block within the current
				// session before I loose it.
				mmblock* bsaved = b->nextsessionblock;
				
				// I don't need this code since I am moving
				// all blocks of the current session to the parent
				// session of the session that will be canceled.
				//LINKEDLISTCIRCULARREMOVE(prevsessionblock, nextsessionblock, b, ((mmblock*){(mmblock*)0}));
				
				#ifdef MMTHREADSAFE
				b->session = s.s;
				#endif
				
				// Add the block to the linkedlist of the parent
				// session of the session that will be canceled.
				// Note that LINKEDLISTCIRCULARADDTOBOTTOM()
				// will not modify s.s which must not be modified.
				LINKEDLISTCIRCULARADDTOBOTTOM(prevsessionblock, nextsessionblock, b, s.s);
				
				b = bsaved;
			}
			
			// I set mmcurrentsession to the parent session.
			mmcurrentsession = mmcurrentsession->prevsession;
			
			// Now that I have set mmcurrentsession to the parent
			// session, I can free the session that was previously current.
			// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
			// the address in sessiontofree set it at the beginning
			// of the block to free.
			// Note that memory allocated for a session is not added
			// to any session, hence mmfreeblock() is called with
			// its argument flag set to null.
			mmfreeblock((mmblock*)((void*)mmcurrentsession->nextsession - (sizeof(mmblock) - 2*sizeof(mmblock*))), 0);
		}
		
		#ifdef MMTHREADSAFE
		mutexunlock(&s.s->mutex);
		#endif
		
		mmcurrentsession->nextsession = 0;
		
	} else {
		// If the session being canceled is the current session,
		// mmcurrentsession is set to the parent session of s.s .
		if (mmcurrentsession == s.s) mmcurrentsession = s.s->prevsession;
		
		#ifdef MMTHREADSAFE
		mutexlock(&s.s->prevsession->mutex);
		#endif
		
		// I move all blocks of the current session to
		// the parent session of the session that will be canceled.
		
		b = s.s->nextsessionblock;
		
		while (b != (mmblock*)s.s) {
			// Since I am going to remove the block pointed by b
			// from the session pointed by s.s, I need to get the address
			// of the next block within that session before I loose it.
			mmblock* bsaved = b->nextsessionblock;
			
			// I don't need this code since I am moving
			// all blocks of the current session to the parent
			// session of the session that will be canceled.
			//LINKEDLISTCIRCULARREMOVE(prevsessionblock, nextsessionblock, b, ((mmblock*){(mmblock*)0}));
			
			#ifdef MMTHREADSAFE
			b->session = s.s->prevsession;
			#endif
			
			// Add the block to the linkedlist of the parent
			// session of the session that will be canceled.
			// Note that LINKEDLISTCIRCULARADDTOBOTTOM()
			// will not modify s.s which must not be modified.
			LINKEDLISTCIRCULARADDTOBOTTOM(prevsessionblock, nextsessionblock, b, s.s->prevsession);
			
			b = bsaved;
		}
		
		#ifdef MMTHREADSAFE
		mutexunlock(&s.s->prevsession->mutex);
		#endif
		
		// I detach the session from the linkedlist
		// of sessions. The field prevsession of a session created
		// through mmsessionnew() is never null because it is either
		// set to mmfirstsession or set to a session created
		// through mmsessionnew().
		s.s->prevsession->nextsession = s.s->nextsession;
		if (s.s->nextsession) s.s->nextsession->prevsession = s.s->prevsession;
		
		// I free the session from which I moved all the blocks.
		// Substracting (sizeof(mmblock) - 2*sizeof(mmblock*)) from
		// the address in sessiontofree set it at the beginning of
		// the block to free.
		// Note that memory allocated for a session is not added to
		// any session, hence mmfreeblock() is called with its argument
		// flag set to null.
		mmfreeblock((mmblock*)((void*)s.s - (sizeof(mmblock) - 2*sizeof(mmblock*))), 0);
	}
	
	#ifdef MMTHREADSAFE
	mutexunlock(&mmsessionmutex);
	#endif
}
