
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


#ifndef LIBMUTEX
#define LIBMUTEX

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
	uint _[3];
} mutex;

// Null value useful for initialization.
#define mutexnull ((mutex){0, 0, 0})

// This function enter a tight loop
// until any other thread having called
// this function on the mutex, call mutexunlock()
// on the same mutex. Each thread having
// called this function on the same mutex,
// get out of the tight loop in the same
// order they made the call.
void mutexlock (mutex* s);

// This function return the lock
// obtained on the mutex.
void mutexunlock (mutex* s);

#endif
