
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


#include <stdtypes.h>

// Library implementing mutex.

// A mutex is used to serialize
// access to a section of code that
// cannot be executed concurrently
// by more than one thread.
// A mutex object only allow
// one thread into a controlled section,
// forcing other threads which attempt
// to gain access to that section
// to wait until the first thread
// has exited from that section.

// Threads get a lock in the order
// in which they called lock().

// Struct representing a mutex.
// Before use, the mutex should
// be initialized using mutexnull.
typedef struct {
	// This field is used to prevent more
	// than one threads from incrementing
	// the field ticket at the same time.
	uint lock;
	
	// The fields ticket and current are
	// used to insure that each thread having
	// called mutexlock() on the same mutex,
	// get a lock on the mutex in the order
	// they called mutexlock().
	// Each thread calling mutexlock()
	// on the mutex, save the value from
	// the field ticket, increment it and
	// enter a loop until the field current
	// become equal to the value previously
	// saved from the field ticket.
	// The field current is incremented
	// everytime a thread call mutexunlock().
	uint ticket;
	uint current;
	
} mutex;

// This function enter a tight loop
// until any other thread having called
// this function on the mutex, call mutexunlock()
// on the same mutex. Each thread having
// called this function on the same mutex,
// get out of the tight loop in the same
// order they made the call.
void mutexlock (volatile mutex* s) {
	// This lock function only work with x86 processors.
	// If *state has a null value then it will be set to 1
	// and this function will return without looping around.
	// If *state has a value of 1, it mean that a lock is present
	// and I will be in a loop until a null value is externally set
	// in *state to mark it as no lock present and stop the loop.
	// The idea here is that, when *state is externally set to 0
	// by another thread, I only want a single thread to read
	// the 0 and set *state to 1. If more than one thread
	// was allowed to read that 0, then all those threads
	// having read that 0 would assume that there is no lock
	// and I all set *state to 1, and I will no longer have
	// a single thread holding the lock but multiple
	// thread thinking that they hold the lock.
	// So the atomic operation of reading *state and
	// setting it to 1 at once is what make mutex work.
	#if 0
	void lock (uint* state) { ### Replaced by portable built-in GCC function.
		// (%0) dereference the variable "state"
		asm (	"mov $1, %%eax;" // Load a value of 1 in register eax.
			"loop:;"
			"xchg %%eax, (%0);" // Atomically exchange
			"and %%eax, %%eax;" // This is used to set the processor flag for the instruction jnz.
			"jnz loop;"
			:
			:"r"(state)
			:"memory", "%eax"
		);
	}
	#endif
	
	// Only one thread can have access
	// to the field mutex.ticket .
	// Spinlock until the lock is removed
	// if there was already a lock.
	// lock(&s->lock); ### Replaced by portable built-in GCC function.
	while (__sync_lock_test_and_set(&s->lock, 1));
	
	// If there is not enough tickets,
	// I loop around until a ticket
	// is made available.
	// In fact if many ticket are being
	// requested, the value of the field
	// mutex.ticket may wrap around
	// and catch up with the value of
	// the field mutex.current .
	// -1 is the maximum value of a uint.
	while ((s->ticket - s->current) == -1);
	
	// Get the ticket value.
	uint i = s->ticket;
	
	// Update ticket to its next value.
	++s->ticket;
	
	// Remove the lock.
	// s->lock = 0; ### Replaced by portable built-in GCC function.
	__sync_lock_release(&s->lock);
	
	// Loop around until it is my turn.
	while (s->current != i);
}

// This function return the lock
// obtained on the mutex.
void mutexunlock (volatile mutex* s) {
	++s->current;
}
