
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


// All functions within this file are used only
// in the secondpass because instructions
// are generated only in the secondpass and lyricalreg
// are manipulated only in the secondpass.


// Function which return
// the lyricalreg for which
// the id is given as argument
// from the lyricalfunction
// given as argument.
// The register id to search
// must be valid, otherwise
// an infinite looping occur.
lyricalreg* searchreg (lyricalfunction* funcowner, uint id) {
	
	lyricalreg* r = funcowner->gpr;
	
	while (r->id != id) r = r->next;
	
	return r;
}

// This function is used to move a register to the bottom of the linkedlist
// of registers of the owning function. Registers found at the top of the linkedlist are the less
// used registers and will be the ones choosen when I need a register during allocation.
// Moving the register to the bottom of the linkedlist is crucial not only for fast
// register allocation but also necessary to the function flushanddiscardallreg();
// In fact within that function while flushing a bitselected register, flushreg() allocate
// another register for the main value of the bitselected register, and if flushreg() had not moved
// that register to the bottom of the linkedlist it would not have been flushed during
// the working of the loop used within flushanddiscardallreg().
void setregtothebottom (lyricalreg* r) {
	
	if (r != r->funcowner->gpr) {
		// Remove register from linkedlist.
		r->prev->next = r->next;
		r->next->prev = r->prev;
		
		// Set the fields prev and next of the register to move to the bottom of the linkedlist.
		r->next = r->funcowner->gpr;
		r->prev = r->funcowner->gpr->prev;
		
		// Re-insert the register at the bottom of the linkedlist.
		r->funcowner->gpr->prev->next = r;
		r->funcowner->gpr->prev = r;
		
	} else r->funcowner->gpr = r->funcowner->gpr->next;
}

// This function is used to move a register to the top of the linkedlist
// of registers of the owning function. Registers found at the top of the linkedlist are the less
// used registers and will be the ones prefered when I need a register during allocation.
void setregtothetop (lyricalreg* r) {
	
	if (r != r->funcowner->gpr) {
		setregtothebottom(r);
		r->funcowner->gpr = r;
	}
}

// I distinguish two type of registers:
// critical and non-critical registers.
// 
// Critical registers are:
// - Registers which cannot get dirty and never get flushed.
// 	ei: Registers allocated for a constant variable,
// 	or for a pointer to global variable region,
// 	or for a pointer to retvar.
// - Registers allocated to variables which when flushed
// 	do not need to allocate another register.
// 	ei: Register allocated for a local variable (including
// 	locals of a function other than currentfunc for which
// 	the tiny stackframe is contained in the shared region
// 	of currentfunc) which is not a bitselected and not
// 	a dereference variable, or register allocated for
// 	a pointer to a parent function stackframe, or register
// 	holding the return address; such register when flushed,
// 	make use of %0 which is the stack pointer register
// 	and allocreg() never allocate %0.
// - Registers that are not dirty regardless of whether
// 	they are the type of register which when dirty
// 	and flushed need to allocate another register.
// - Free registers, which mean the fields funclevel,
// 	globalregionaddr, stringregionaddr, thisaddr,
// 	retvaraddr and v of the lyricalreg are null.
// 
// Distinguishing two types of registers is necessary
// and there always ought to be enough critical
// register available because those registers do not
// spun another register allocation when flushing them
// if they are flushable.
// A critical register is needed wherever a register
// should be allocated without causing another
// allocation or causing a flushing.
// Not considering the necessity of critical registers
// can cause an endless allocation loop which
// will yield the error "no register left available to use".
// In a scenario where all registers are dirty and allocated
// for global variables, attempting to flush one of
// the dirty register will lead to the locking of
// all registers because I will need to flush and discard
// another register in order to use it to hold the address
// of the global variable region, but attempting to flush
// that other dirty register will recursively lead to
// the flushing of another dirty register until all
// registers are locked, since flushreg() will lock
// a register to flush before calling allocreg()
// to get a register to be used to hold the address
// of the global variable region.

// This function return 1 if the register
// given as argument is a critical register,
// otherwise null is returned.
uint isregcritical (lyricalreg* r) {
	// Here I check whether the register
	// is allocated for the return address, or
	// a pointer to a parent function stackframe,
	// or a pointer to global variable region, or
	// retvar address, or is free, or allocated
	// to a constant variable, or is dirty, or
	// allocated to a local variable(including
	// locals of a function other than currentfunc
	// for which the tiny stackframe is contained
	// in the shared region of currentfunc) which
	// is not retvar, a bitselected and not a dereferenced variable.
	// Keep in mind that when the fields returnaddr,
	// funclevel, globalregionaddr, stringregionaddr, thisaddr
	// retvaraddr and v are null then the register is free.
	if (r->returnaddr || r->funclevel || r->globalregionaddr || r->stringregionaddr || r->thisaddr || r->retvaraddr ||
		!r->v || r->v->name.ptr[0] == '0' || !r->dirty ||
			(r->v != &thisvar && r->v != &returnvar && currentfunc != rootfunc && !r->bitselect && r->v->name.ptr[1] != '*' &&
				// Here below I am checking whether the register
				// is allocated to a local variable (including locals
				// of a function other than currentfunc for which
				// the tiny stackframe is contained in the shared
				// region of currentfunc).
				(r->v->funcowner == currentfunc || (r->v->funcowner->firstpass->stackframeholder &&
					r->v->funcowner->firstpass->stackframeholder->secondpass == currentfunc))))
						return 1;
	
	return 0;
}

// ### GCC wouldn't compile without the use of the keyword auto.
auto void flushreg (lyricalreg* r);

// This function assess whether there are enough critical registers,
// and if not, it will turn as many non-critical registers as needed
// into critical registers.
void insurethereisenoughcriticalreg () {
	
	lyricalreg* r = currentfunc->gpr;
	
	// Assess whether there is a critical
	// register count of at least 2.
	
	uint criticalregcount = 0;
	
	do {
		if (!r->lock && !r->reserved && isregcritical(r)) {
			++criticalregcount;
			if (criticalregcount == 2) return;
		}
		
		r = r->next;
		
	} while (r != currentfunc->gpr);
	
	// If I get here, I don't have enough critical registers;
	// So I need to turn as many non-critical registers as needed
	// into critical registers.
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("begin: critical register count less than minimum"));
	}
	
	// When I get out of the above loop, r == currentfunc->gpr.
	
	// I compute the number of non-critical registers
	// to turn into critical registers.
	uint i = 2 - criticalregcount;
	
	do {
		if (!r->lock && !r->reserved && !isregcritical(r)) {
			
			if (r->dirty) flushreg(r);
			
			if (!--i) break;
		}
		
		r = r->next;
		
	} while (r != currentfunc->gpr);
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}

// Enum used with allocreg().
typedef enum {
	// Use when allocating a register
	// that will never be marked dirty, or
	// will be marked dirty but flushing it
	// will not trigger the allocation
	// of another register.
	CRITICALREG,
	
	// Use when allocating a register that will
	// be marked dirty and flushing it will trigger
	// the allocation of another register.
	NONCRITICALREG
	
} allocregflag;

// This function will return a register to use,
// flushing and discarding a register if necessary.
// The register returned is not associated
// with any variable.
// The fields waszeroextended and wassignextended
// of the register to return are reset to null.
// I need to lock a register being used if
// the function allocreg() is going to be called,
// since it can deallocate a register while
// trying to allocate a new register.
// The argument flag is set depending on whether
// the register to allocate is going to be used
// as a critical or non-critical register.
lyricalreg* allocreg (allocregflag flag) {
	
	lyricalreg* r = currentfunc->gpr;
	
	keeplooking:
	
	// I go through the linkedlist of
	// registers from the less used register
	// which is at the top of the linkedlist,
	// to the most used register which is at
	// the bottom of the linkedlist; skipping
	// any locked or reserved registers.
	// The first available register get selected in r.
	while (r->lock || r->reserved) {
		
		r = r->next;
		
		// I throw an error if I go through
		// the entire linkedlist without finding
		// an unlocked register available.
		// It happen when too many registers
		// are reserved by an asm block.
		if (r == currentfunc->gpr) throwerror("no register left available to use");
	}
	
	// If the register was used to hold the return address or
	// the stackframe address of a function or the address of
	// the global variable region or the address of the string region
	// or the address of retvar, I set the field returnaddr or
	// funclevel or globalregionaddr or stringregionaddr or
	// thisaddr or retvaraddr to zero accordingly.
	// Remember that only one of the fields returnaddr,
	// funclevel, globalregionaddr, stringregionaddr,
	// thisaddr, retvaraddr and v can be non-null at a time.
	if (r->returnaddr) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		// If the register holding the return address
		// was not previously saved, I flush it.
		if (r->dirty) flushreg(r);
		
		r->returnaddr = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->funclevel) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		// If the register for the stackframe pointer
		// has not yet been written to its cache, I flush it.
		if (r->dirty) flushreg(r);
		
		r->funclevel = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->globalregionaddr) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		r->globalregionaddr = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->stringregionaddr) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		r->stringregionaddr = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->thisaddr) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		r->thisaddr = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->retvaraddr) {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
		
		r->retvaraddr = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else if (r->v) {
		// The following test is similar to what is found in isregcritical().
		if (r->v->name.ptr[0] == '0' || !r->dirty || (r->v != &thisvar && r->v != &returnvar &&
			currentfunc != rootfunc && !r->bitselect && r->v->name.ptr[1] != '*' &&
				// Here below I am checking whether the register
				// is allocated to a local variable(including locals
				// of a function other than currentfunc for which
				// the tiny stackframe is contained in the shared
				// region of currentfunc)
				(r->v->funcowner == currentfunc || (r->v->funcowner->firstpass->stackframeholder &&
					r->v->funcowner->firstpass->stackframeholder->secondpass == currentfunc)))) {
			
			// I get here if I have a critical register.
			
			// If the critical register is going to be used
			// as a non-critical register, I insure that
			// I have enough critical registers.
			// I should not be calling insurethereisenoughcriticalreg()
			// when flag == CRITICALREG because, this instance of
			// allocreg() may have been called while trying
			// to flush a non-critical register, and I should
			// only allocate a critical register to prevent
			// another recursive call of this function by flushreg()
			// which could occur when flushreg() need to allocate
			// the register containing the address of the memory
			// region where a variable is to be written; ei:
			// stackframe address, global variable region address,
			// retvar address.
			if (flag == NONCRITICALREG) {
				// I lock the register because if insurethereisenoughcriticalreg()
				// make use of flushreg(), allocreg() could be called and if
				// the register is not locked, allocreg() could use it while trying
				// to allocate a new register.
				r->lock = 1;
				insurethereisenoughcriticalreg();
				r->lock = 0;
			}
			
		} else {
			// I get here if I have a non-critical register.
			
			// When I get here, I can assume that
			// I have enough critical registers.
			
			if (flag == CRITICALREG) {
				// If I get here, I should skip, because
				// when flag == CRITICALREG, this instance of
				// allocreg() may have been called while trying
				// to flush a non-critical register, and I should
				// only allocate a critical register to prevent
				// another recursive call of this function by flushreg()
				// which could occur when flushreg() need to allocate
				// the register containing the address of the memory
				// region where a variable is to be written; ei:
				// stackframe address, global variable region address,
				// retvar address.
				
				r = r->next;
				
				// I throw an error if I go through
				// the entire linkedlist without finding
				// an unlocked register available.
				// It happen when too many registers
				// are reserved by an asm block.
				if (r == currentfunc->gpr) throwerror("no register left available to use");
				
				// If I get here, I jump to keeplooking
				// to try another register.
				goto keeplooking;
			}
		}
		
		// If the register that I am about to use
		// was assigned to a variable, I flush it
		// if it was dirty and discard it.
		// flushreg() will make sure to set
		// the field dirty of the register to null.
		// Note that if the register being flushed
		// is bitselected, flushreg() will allocate another
		// register for the main value of the bitselected
		// register; doing so will not cause any problem
		// with respect to volatility, because if
		// I was working with a volatile variable,
		// the main register will simply end up being
		// reloaded next time it will be requested as volatile.
		// On the other hand if the variable was not volatile
		// I would have gained in speed because
		// next time the main register will
		// be needed, I will not need to reload it.
		// There is no need to lock the register
		// pointed by r because flushreg() do it.
		if (r->dirty) flushreg(r);
		
		r->v = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else {
		// If the critical register is going to be used
		// as a non-critical register, I insure that
		// I have enough critical registers.
		// I should not be calling insurethereisenoughcriticalreg()
		// when flag == CRITICALREG because, this instance of
		// allocreg() may have been called while trying
		// to flush a non-critical register, and I should
		// only allocate a critical register to prevent
		// another recursive call of this function by flushreg()
		// which could occur when flushreg() need to allocate
		// the register containing the address of the memory
		// region where a variable is to be written; ei:
		// stackframe address, global variable region address,
		// retvar address.
		if (flag == NONCRITICALREG) {
			// I lock the register because if insurethereisenoughcriticalreg()
			// make use of flushreg(), allocreg() could be called and if
			// the register is not locked, allocreg() could use it while trying
			// to allocate a new register.
			r->lock = 1;
			insurethereisenoughcriticalreg();
			r->lock = 0;
		}
	}
	
	r->dirty = 0;
	r->bitselect = 0;
	r->offset = 0;
	r->size = 0;
	r->waszeroextended = 0;
	r->wassignextended = 0;
	
	// The register allocated should not
	// be moved to the bottom or top of
	// the linkedlist just yet.
	// It will be the function which used it
	// which will do the work according to its need.
	// As a rule, registers are moved to the bottom
	// of the linkedlist only if they have a variable
	// or funclevel or globalregionaddr or stringregionaddr
	// associated with them, otherwise they get moved
	// to the top of the linkedlist.
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		
		comment(stringfmt("reg %%%d allocated", r->id));
	}
	
	return r;
}


// This function will search among all registers
// if there is a register already containing the stackframe
// address of the parent function of the current function
// at the level given by the argument funclevel.
// If no register could be found, instructions are generated
// to load into a register to return, the wanted stackframe address.
// This function should not be called with a funclevel
// argument value of 0, because any register will be returned
// since registers allocated to variables for example have
// their field funclevel set to zero.
// Any register currently being used should be locked
// before calling this function because it can use allocreg()
// which can cause an unlocked register to be discarded while
// looking for a new register.
// This function is the only one creating registers having
// their field funclevel set.
lyricalreg* getregptrtofuncstackframe (uint funclevel) {
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringfmt("begin: retrieving stackframe #%d", funclevel));
	}
	
	// Because only functions that are stackframe holders
	// have a cache for stackframe pointers, if currentfunc
	// is not a stackframe holder, I adjust funclevel so as to
	// compute the stackframe pointer as if getregptrtofuncstackframe()
	// had been called while parsing the body of the function
	// that is the stackframe holder of currentfunc.
	// If the function at the level in funclevel is not
	// a stackframe holder, I adjust funclevel so as the stackframe
	// pointer register for its stackframe holder is returned.
	// Hence the field funclevel of the lyricalreg returned
	// may not be the same as the initial value of
	// the argument funclevel.
	
	uint level;
	
	lyricalfunction* f;
	
	lyricalfunction* stackframeholder = currentfunc->firstpass->stackframeholder;
	
	// I check whether currentfunc is a stackframe holder.
	if (stackframeholder) {
		// I get here if currentfunc is not a stackframe holder.
		// I will never get here when getregptrtofuncstackframe()
		// is called from cachestackframepointers() because
		// cachestackframepointers() is always called while parsing
		// the body of functions that can be stackframe holders.
		
		// I get the secondpass equivalent lyricalfunction
		// of the stackframe holder.
		stackframeholder = stackframeholder->secondpass;
		
		level = 1;
		
		f = currentfunc->parent;
		
		while (f != stackframeholder) {
			++level;
			f = f->parent;
		}
		
		// When the function at the level in funclevel is not
		// a stackframe holder, the stackframe pointer register
		// for its stackframe holder is returned, hence if
		// funclevel is less than or equal to the level of
		// the stackframe holder of currentfunc then the stack
		// pointer register %0 already hold the stackframe address
		// needed; so &rstack should be returned.
		if (funclevel <= level) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringduplicate2("end: done"));
			}
			
			return &rstack;
		}
		
		// I adjust funclevel as to compute the stackframe pointer
		// as if getregptrtofuncstackframe() had been called while
		// parsing the body of the function that is the stackframe
		// holder of currentfunc.
		funclevel -= level;
		
	} else stackframeholder = currentfunc;
	
	// I adjust funclevel if the function at
	// the level in funclevel is not a stackframe holder;
	// the stackframe pointer for its stackframe holder
	// is what should be retrieved.
	
	level = funclevel -1;
	
	f = stackframeholder->parent;
	
	while (level) {
		--level;
		f = f->parent;
	}
	
	while (f->firstpass->stackframeholder) {
		++funclevel;
		f = f->parent;
	}
	
	// I search through the registers if there is
	// already a register containing the stackframe address
	// of the parent function of the current function at
	// the level given by the argument funclevel; if yes
	// I use it, otherwise I compute it.
	
	lyricalreg* r = currentfunc->gpr;
	
	// When a register containing the stackframe address
	// for the value given by the argument funclevel cannot
	// be found, this variable get set to the register
	// containing the address of the closest stackframe;
	// this way I save some computation by not starting
	// from the current stackframe.
	lyricalreg* rr = 0;
	
	while (1) {
		// I break from the loop if I found the register.
		if ((level = r->funclevel) == funclevel) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			break;
			
		} else {
			
			if (level && level < funclevel) {
				if (!rr || level > rr->funclevel) rr = r;
			}
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all the registers and I could not
			// find a register containing the stackframe address of the parent function
			// of the current function at the level given by the argument funclevel.
			
			uint savedfunclevel = funclevel;
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			if (rr) rr->lock = 1;
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it will
			// be seen as an unused register
			// when generating lyricalinstruction,
			// since it is not assigned to anything.
			(r = allocreg(CRITICALREG))->lock = 1;
			
			// I also lock the above lyricalreg,
			// otherwise they will be seen as
			// unused registers when generating
			// lyricalinstruction.
			
			// If stackframeholder->stackframepointercachingdone is set,
			// then this function is certainly not being called from
			// cachestackframepointers() and the stackframe pointer
			// should be retrieved from the cache.
			// The stackframe pointer cache cannot be used while
			// cachestackframepointers() is caching stackframe pointers.
			if (stackframeholder->stackframepointercachingdone) {
				
				lyricalcachedstackframe* stackframe = stackframeholder->firstpass->cachedstackframes;
				
				// There is no need to check whether stackframe
				// is null because the stackframe pointer should
				// certainly exist in the cache.
				
				// This variable is initialized to 1 to take
				// into account the space used at the top of a stackframe
				// to store the offset from the start of the stackframe
				// to the field holding the return address.
				uint i = 1;
				
				while (1) {
					
					if (stackframe->level == funclevel) {
						
						ld(r, &rstack, i*sizeofgpr);
						
						goto stackframepointerloaded;
					}
					
					if (stackframe = stackframe->next) ++i;
					else if (funclevel != 1) throwerror(stringfmt("internal error: %s: stackframe #%d pointer missing from cache", __FUNCTION__, funclevel).ptr);
					else break;
				}
			}
			
			// I will get here only when this function is called from
			// cachestackframepointers(); later calls of this function will
			// retrieve the stackframe pointer either from a register
			// or from the cache; unless I was retrieving the address
			// of the stackframe for which funclevel == 1; such stackframe
			// address is not always cached because it can be read from
			// the stackframe of stackframeholder.
			
			f = stackframeholder;
			
			// If rr was set, I use its register as the starting point
			// of the computation to find the stackframe address of the parent
			// function at the level given by the argument funclevel.
			// If rr was not set, I start the computation from the current
			// stackframe for which the address is in the stack pointer register.
			if (rr) {
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringfmt("re-using reg %%%d", rr->id));
				}
				
				// setregtothebottom() should not be called
				// on the lyricalreg pointed by rr.
				
				level = rr->funclevel;
				
				// I decrement funclevel by the amount of
				// iterations that do not need to be done
				// to retrieve the stackframe address.
				funclevel -= level;
				
				// Within this loop I set f to the function
				// for which the stackframe address is held
				// by the lyricalreg pointed by rr.
				do f = f->parent; while (--level);
				
			} else rr = &rstack;
			
			// Here I generate instructions which will load in r
			// the wanted stackframe address.
			
			lyricalinstruction* i;
			lyricalimmval* imm;
			
			if (f->firstpass->itspointerisobtained) {
				// I load the pointer to the previous stackframe.
				// The following instruction is generated:
				// ld r, rr, 2*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
				// stackframepointerscachesize, sharedregionsize and
				// vlocalmaxsize are immediate values generated
				// from the lyricalfunction pointed by f.
				
				i = ld(r, rr, 2*sizeofgpr);
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
				imm->f = f;
				
				i->imm->next = imm;
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = f;
				
				i->imm->next->next = imm;
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMLOCALVARSSIZE;
				imm->f = f;
				
				i->imm->next->next->next = imm;
				
				goto usestackframeid;
			}
			
			// I load the pointer to the parent function stackframe.
			// The following instruction is generated:
			// ld r, rr, 3*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
			// stackframepointerscachesize, sharedregionsize and
			// vlocalmaxsize are immediate values generated
			// from the lyricalfunction pointed by f.
			
			i = ld(r, rr, 3*sizeofgpr);
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = f;
			
			i->imm->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSHAREDREGIONSIZE;
			imm->f = f;
			
			i->imm->next->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMLOCALVARSSIZE;
			imm->f = f;
			
			i->imm->next->next->next = imm;
			
			f = f->parent;
			
			--funclevel;
			
			while (funclevel) {
				// If the function for which the lyricalfunction
				// is pointed by f make use of a stackframe holder,
				// the stackframe address that was loaded in the register
				// for which the lyricalreg is pointed by r is for
				// its stackframe holder; hence I adjust the variables f
				// and funclevel so as to be at the level for
				// that stackframe holder function.
				while (f->firstpass->stackframeholder) {
					
					if (!--funclevel) goto stackframepointerloaded;
					
					f = f->parent;
				}
				
				lyricalcachedstackframe* stackframe = f->firstpass->cachedstackframes;
				
				if (stackframe) {
					// If I get here, I check whether the stackframe
					// pointer for the value of funclevel was cached
					// by the function pointed by f.
					
					// If the stackframe pointer for the value of funclevel
					// could not be found among the cached stackframe pointers
					// of the function pointed by f, this variable contain
					// the level of the closest stackframe as well as
					// the location of its address within the stack of
					// the function pointed by f.
					struct {
						
						uint level;
						uint locofaddrinstack;
						
					} closeststackframe = {
						
						.level = 0,
						.locofaddrinstack = 0,
					};
					
					// This variable is initialized to 1 to take
					// into account the space used at the top of a stackframe
					// to write the offset from the start of the stackframe
					// to the field holding the return address.
					uint i = 1;
					
					while (1) {
						
						if (stackframe->level == funclevel) {
							
							ld(r, r, i*sizeofgpr);
							
							goto stackframepointerloaded;
							
						} else {
							
							level = stackframe->level;
							
							if (level < funclevel) {
								
								if (!closeststackframe.level ||
									level > closeststackframe.level) {
									
									closeststackframe.level = level;
									closeststackframe.locofaddrinstack = i;
								}
							}
						}
						
						if (stackframe = stackframe->next) ++i;
						else break;
					}
					
					// I get here if I could not find the stackframe
					// pointer for the value of funclevel among
					// the cached stackframe pointers of the function
					// pointed by f.
					
					// If closeststackframe.level was set, I use the address
					// of that stackframe as the starting point of the computation
					// to find the stackframe address of the parent function
					// at the level given by the variable funclevel;
					// if closeststackframe.level was not set, I start the computation
					// from the stackframe of the function pointed f for which
					// the stackframe address is in the register pointed by r.
					if (closeststackframe.level) {
						
						ld(r, r, closeststackframe.locofaddrinstack*sizeofgpr);
						
						// I decrement funclevel by the amount of
						// iterations that do not need to be done
						// to retrieve the stackframe address.
						funclevel -= closeststackframe.level;
						
						// Within this loop I set f to the function
						// for which the stackframe address was loaded
						// in the register pointed by r.
						do f = f->parent;
						while (--closeststackframe.level);
					}
				}
				
				if (f->firstpass->itspointerisobtained) {
					// If the function pointed by f had its pointer obtained then
					// it mean that it can be called from within any function.
					// In order to retrieve the stackframe of its parent function,
					// I scan through previous stackframes checking each time
					// the field stackframe-id to identify the correct stackframe.
					
					// I load the pointer to the previous stackframe.
					// The following instruction is generated:
					// ld r, r, 2*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
					// stackframepointerscachesize, sharedregionsize and
					// vlocalmaxsize are immediate values generated
					// from the lyricalfunction pointed by f.
					
					i = ld(r, r, 2*sizeofgpr);
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					i->imm->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					i->imm->next->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMLOCALVARSSIZE;
					imm->f = f;
					
					i->imm->next->next->next = imm;
					
					usestackframeid:;
					
					f = f->parent;
					
					// I adjust the variables f and funclevel
					// so as to use a stackframe holder function,
					// because a stackframe-id exist only
					// in a regular stackframe which is used
					// only by stackframe holder functions.
					while (f->firstpass->stackframeholder) {
						// Within this loop, funclevel
						// will never become null because
						// it become null only after
						// the stackframe pointer of
						// interest has been loaded.
						--funclevel;
						
						f = f->parent;
					}
					
					// Here below are the instructions generated in order
					// to keep going through previous stackframes until
					// the correct stackframe is found.
					// 
					// ---------------------------------
					// #Load in r1, the address of the function
					// #for which the stackframe has to be found.
					// afip r1, OFFSETTOFUNC(f);
					// 
					// stackframenotfound:
					// 
					// #Load the offset from the start of the stackframe
					// #pointed by r to the field holding the return address;
					// #and use that offset to set r2 to the location of
					// #the field holding the return address.
					// ld r2, r, 0;
					// add r2, r2, r;
					// 
					// #Get stackframe-id.
					// ld r3, r2, 3*sizeofgpr;
					// 
					// jeq r1, r3, stackframefound;
					// 
					// #If I get here it mean that I do not have
					// #the correct stackframe in r, so I load the previous
					// #function stackframe in r and redo the checks again.
					// ld r, r2, sizeofgpr;
					// 
					// j stackframenotfound;
					// 
					// stackframefound:
					// --------------------------------
					// 
					// Note that before the jump instructions and the labels used above, it is not necessary to flush and
					// discard registers, because those branching instructions and labels are not generated to
					// branch across code written by the programmer.
					
					// I first allocate 3 registers which will be used
					// for the stackframe lookup.
					
					lyricalreg* r1;
					lyricalreg* r2;
					lyricalreg* r3;
					
					// I lock the below registers because
					// allocreg() could use them while
					// trying to allocate a new register.
					// I also lock them, otherwise they
					// will be seen as unused registers
					// when generating lyricalinstruction.
					(r1 = allocreg(CRITICALREG))->lock = 1;
					(r2 = allocreg(CRITICALREG))->lock = 1;
					(r3 = allocreg(CRITICALREG))->lock = 1;
					
					// I create the name of the labels that will be needed below.
					string labelnameforstackframefound = stringfmt("%d", newgenericlabelid());
					string labelnameforstackframenotfound = stringfmt("%d", newgenericlabelid());
					
					// From here I generate the instructions which will do
					// the stackframe lookup and that I previously commented about above.
					
					// Here I generate the "afip" instruction.
					i = newinstruction(currentfunc, LYRICALOPAFIP);
					i->r1 = r1->id;
					i->imm = mmallocz(sizeof(lyricalimmval));
					i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
					i->imm->f = f;
					
					newlabel(labelnameforstackframenotfound);
					ldr(r2, r);
					add(r2, r2, r);
					ld(r3, r2, 3*sizeofgpr);
					jeq(r1, r3, labelnameforstackframefound);
					ld(r, r2, sizeofgpr);
					j(labelnameforstackframenotfound);
					newlabel(labelnameforstackframefound);
					
					// Unlock lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r3->lock = 0;
					r2->lock = 0;
					r1->lock = 0;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						// The lyricalreg pointed by r1, r2 and r3 were not
						// allocated to a lyricalvariable but since I am done
						// using them, I should also produce a comment about them
						// having been discarded to complement the allocation comment
						// that is generated when they were allocated by allocreg().
						comment(stringfmt("reg %%%d discarded", r1->id));
						comment(stringfmt("reg %%%d discarded", r2->id));
						comment(stringfmt("reg %%%d discarded", r3->id));
					}
					
				} else {
					// I load the pointer to the parent function stackframe.
					// The following instruction is generated:
					// ld r, r, 3*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
					// stackframepointerscachesize, sharedregionsize and
					// vlocalmaxsize are immediate values generated
					// from the lyricalfunction pointed by f.
					
					i = ld(r, r, 3*sizeofgpr);
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					i->imm->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					i->imm->next->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMLOCALVARSSIZE;
					imm->f = f;
					
					i->imm->next->next->next = imm;
					
					f = f->parent;
				}
				
				--funclevel;
			}
			
			stackframepointerloaded:
			
			r->size = sizeofgpr;
			
			// The field funclevel of the register pointed by r
			// is set only here to prevent it from being used during
			// the above processing when calling allocreg() which
			// can call flushreg() that could attempt to retrieve
			// the stackframe register that I am currently computing.
			r->funclevel = savedfunclevel;
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			if (rr) rr->lock = 0;
			
			// Getting here mean that the computation to obtain
			// the wanted stackframe address is completed.
			
			break;
		}
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
	
	setregtothebottom(r);
	
	return r;
}


// This function load in registers,
// pointers to all the stackframes
// used by currentfunc; the registers
// loaded with the stackframe addresses
// are set dirty so that their value get
// cached in the stackframe of currentfunc.
// All stackframe pointers are cached
// at the entry into a function; doing
// otherwise would be complex, because
// due to scoping, it would be hard
// to keep track of whether a stackframe
// pointer was already cached.
void cachestackframepointers () {
	// currentfunc->firstpass->cachedstackframes will
	// always be non-null when this function is called.
	lyricalcachedstackframe* stackframe = currentfunc->firstpass->cachedstackframes;
	
	// Unless the address of the current function was obtained,
	// it would be a waist to cache a stackframe address at
	// a level of 1, since that address can be read from
	// the stackframe of the current function.
	// In addition, if rootfunc do not hold the tiny stackframe
	// of its children in its shared region, the stackframe
	// address at a level of 1 must not be cached.
	// Because lyricalcachedstackframe elements are ordered
	// in the linkedlist from the lowest level value to the highest
	// level value, only the first element of the linkedlist
	// is checked to determine whether its field level == 1.
	if (stackframe->level == 1 &&
		((currentfunc->parent == rootfunc && !rootfunc->firstpass->sharedregions) ||
		!currentfunc->firstpass->itspointerisobtained)) {
		
		currentfunc->firstpass->cachedstackframes = stackframe->next;
		
		mmrefdown(stackframe);
		
		stackframe = currentfunc->firstpass->cachedstackframes;
		
		if (!stackframe) return;
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("begin: caching stackframe pointers"));
	}
	
	// This variable will keep the count of stackframes to cache.
	uint countofstackframestocache = 0;
	
	do {
		++countofstackframestocache;
		
		// I load the stackframe pointer in a register and
		// set the register dirty so that its value get
		// cached in the stack when discarded.
		
		lyricalreg* r = getregptrtofuncstackframe(stackframe->level);
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		r->lock = 1;
		
		r->dirty = 1;
		
		// When compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE
		// is true, all variables are made volatile, and
		// stackframe addresses should be cached to memory
		// as soon as possible.
		// This is useful when debugging because it is easier
		// for a debugger to just read memory in order
		// to retrieve a value instead of having to write code
		// in order to keep track of which register has
		// the value needed.
		if (compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE) flushreg(r);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
	} while (stackframe = stackframe->next);
	
	uint stackframepointerscachesize = countofstackframestocache * sizeofgpr;
	
	if (stackframepointerscachesize > MAXSTACKFRAMEPOINTERSCACHESIZE) {
		// I set curpos where the function was declared,
		// before throwing the error message.
		curpos = currentfunc->startofdeclaration;
		
		throwerror("function nested too deep");
		
	} else currentfunc->stackframepointerscachesize = stackframepointerscachesize;
	
	currentfunc->stackframepointercachingdone = 1;
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}


// This function generate instructions which
// load in register %1 the return address.
// This function is only used while parsing the body
// of a function which make use of a stackframe holder.
void loadreturnaddr () {
	// I search through currentfunc->gpr for the register %1
	// and check whether it is already loaded with
	// the return address; if not, I load it
	// with the return address.
	
	lyricalreg* r = currentfunc->gpr;
	
	while (1) {
		
		if (r->id == 1) {
			
			if (r->returnaddr) {
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("re-using reg %1"));
				}
				
				return;
				
			} else r->returnaddr = 1;
			
			// I generate the following instruction:
			// ld %1, %0, sizeofgpr + stackframepointerscachesize + offsetwithinsharedregion;
			// stackframepointerscachesize and offsetwithinsharedregion are
			// immediate values generated from the stackframe holder.
			
			// Lock the lyricalreg for %1,
			// to prevent newinstruction()
			// from adding it to the list
			// of unused registers of
			// the lyricalinstruction.
			r->lock = 1;
			
			lyricalinstruction* i = ld(r, &rstack, sizeofgpr);
			
			lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = currentfunc->firstpass->stackframeholder->secondpass;
			
			i->imm->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
			imm->sharedregion = currentfunc->firstpass->sharedregiontouse;
			
			i->imm->next->next = imm;
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			return;
		}
		
		r = r->next;
	}
	
	setregtothebottom(r);
}


// This function will search among all registers
// if there is an already allocated register pointing
// to the region containing global variables.
// If no register could be found, I generate an instruction
// to load into a register to return, the address to
// the global variable region.
// Any register currently being used should be locked
// before calling this function because it can use
// allocreg() which will cause any unlocked register
// to be discarded while looking for a new register.
// This function is the only one creating registers
// having their field globalregionaddr set.
lyricalreg* getregptrtoglobalregion () {
	// I search through the registers if there is
	// already a register containing the address
	// to the global variable region. If I find it,
	// I use it, otherwise I compute it.
	
	lyricalreg* r = currentfunc->gpr;
	
	while (1) {
		// I break from the loop if I found the register.
		if (r->globalregionaddr) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			break;
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all the registers
			// and I didn't find anything.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			(r = allocreg(CRITICALREG))->lock = 1;
			
			r->size = sizeofgpr;
			
			r->globalregionaddr = 1;
			
			// Here I generate the instruction which will load
			// in r the pointer to the global variable page;
			// the following instruction is generated:
			// afip r, OFFSETTOGLOBALREGION;
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPAFIP);
			i->r1 = r->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOGLOBALREGION;
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			break;
		}
	}
	
	setregtothebottom(r);
	
	return r;
}


// This function will search among all registers
// if there is an already allocated register pointing
// to the region containing string constants.
// If no register could be found, I generate an instruction
// to load into a register to return, the address
// to the string region.
// Any register currently being used should be locked
// before calling this function because it can use
// allocreg() which will cause any unlocked register
// to be discarded while looking for a new register.
// This function is the only one creating registers
// having their field stringregionaddr set.
lyricalreg* getregptrtostringregion () {
	// I search through the registers if there is
	// already a register containing the address
	// to the global variable region.
	// If I find it, I use it, otherwise I compute it.
	
	lyricalreg* r = currentfunc->gpr;
	
	while (1) {
		// I break from the loop if I found the register.
		if (r->stringregionaddr) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			break;
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all the registers
			// and I didn't find anything.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			(r = allocreg(CRITICALREG))->lock = 1;
			
			r->size = sizeofgpr;
			
			r->stringregionaddr = 1;
			
			// Here I generate the instruction which will load
			// in r the pointer to the string region; the following
			// instruction is generated: afip r, OFFSETTOSTRINGREGION.
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPAFIP);
			i->r1 = r->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOSTRINGREGION;
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			break;
		}
	}
	
	setregtothebottom(r);
	
	return r;
}


// This function will search among all registers
// if there is an already allocated register holding
// the address of "this". If no register could be found,
// I generate instructions to load into a register
// to return, the address of "this".
// Any register currently being used should be locked
// before calling this function because it can use
// allocreg() which will cause any unlocked register
// to be discarded while looking for a new register.
// This function is the only one creating registers
// having their field thisaddr set.
lyricalreg* getregptrtothis () {
	// I search through gprs if there is already a register
	// containing the address of retvar.
	// If I find it, I use it, otherwise I compute it.
	lyricalreg* r = currentfunc->gpr;
	
	while (1) {
		// I break from the loop if I found the register.
		if (r->thisaddr) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			break;
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all gprs
			// and I didn't find anything.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			(r = allocreg(CRITICALREG))->lock = 1;
			
			r->size = sizeofgpr;
			
			r->thisaddr = 1;
			
			// Note that a function that use the keyword "this"
			// is always a function for which the address is obtained;
			// therefore such function never make use
			// of a stackframe holder.
			
			// I generate the following instruction:
			// ld r, %0, 5*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
			
			lyricalinstruction* i = ld(r, &rstack, 5*sizeofgpr);
			
			lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = currentfunc;
			
			i->imm->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSHAREDREGIONSIZE;
			imm->f = currentfunc;
			
			i->imm->next->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMLOCALVARSSIZE;
			imm->f = currentfunc;
			
			i->imm->next->next->next = imm;
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			break;
		}
	}
	
	setregtothebottom(r);
	
	return r;
}


// This function will search among all registers
// if there is an already allocated register holding
// the address of retvar. If no register could be found,
// I generate instructions to load into a register
// to return, the address of retvar.
// Any register currently being used should be locked
// before calling this function because it can use
// allocreg() which will cause any unlocked register
// to be discarded while looking for a new register.
// This function will never be called when parsing
// a function which do not return a variable.
// This function is the only one creating registers
// having their field retvaraddr set.
lyricalreg* getregptrtoretvar () {
	// I search through gprs if there is already a register
	// containing the address of retvar.
	// If I find it, I use it, otherwise I compute it.
	lyricalreg* r = currentfunc->gpr;
	
	while (1) {
		// I break from the loop if I found the register.
		if (r->retvaraddr) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			break;
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all gprs
			// and I didn't find anything.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			(r = allocreg(CRITICALREG))->lock = 1;
			
			r->size = sizeofgpr;
			
			r->retvaraddr = 1;
			
			// I generate instructions which will load in r
			// the address of retvar from the current stackframe.
			if (currentfunc->firstpass->stackframeholder) {
				// I get here if currentfunc make use
				// of a stackframe holder.
				
				// I generate the following instruction:
				// ld r, %0, 3*sizeofgpr + stackframepointerscachesize + offsetwithinsharedregion;
				// stackframepointerscachesize and offsetwithinsharedregion are
				// immediate values generated from the stackframe holder.
				
				lyricalinstruction* i = ld(r, &rstack, 3*sizeofgpr);
				
				lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
				imm->f = currentfunc->firstpass->stackframeholder->secondpass;
				
				i->imm->next = imm;
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
				imm->sharedregion = currentfunc->firstpass->sharedregiontouse;
				
				i->imm->next->next = imm;
				
			} else {
				// I get here if currentfunc is a stackframe holder.
				
				// I generate the following instruction:
				// ld r, %0, 6*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
				
				lyricalinstruction* i = ld(r, &rstack, 6*sizeofgpr);
				
				lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
				imm->f = currentfunc;
				
				i->imm->next = imm;
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = currentfunc;
				
				i->imm->next->next = imm;
				
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMLOCALVARSSIZE;
				imm->f = currentfunc;
				
				i->imm->next->next->next = imm;
			}
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			break;
		}
	}
	
	setregtothebottom(r);
	
	return r;
}


// Enum used with getregforvar().
typedef enum {
	// When used, if a register was
	// already allocated, return it, otherwise
	// allocate a register and generate instructions
	// to load the register with its value.
	// If the lyricalvariable is volatile,
	// a register is allocated if it had not yet
	// been allocated and is always reloaded with
	// its value even if it was already allocated.
	FORINPUT,
	
	// When used, if a register was
	// already allocated, return it, otherwise
	// allocate a register and return it without
	// loading it with its value.
	// The register returned is set dirty since
	// it is to be used for an output.
	FOROUTPUT
	
	// To resume, FOROUTPUT is used when
	// I want to obtain a register to be used
	// as an output to an instruction, while FORINPUT
	// is used when I want to obtain a register
	// to be used as an input to an instruction.
	
} getregforvarregpurpose;

// I declare getregforvar() here since it is used by generateloadinstr().
// ### GCC wouldn't compile without the use of the keyword auto.
auto lyricalreg* getregforvar (lyricalvariable* v, uint offset, string type, u64 bitselect, getregforvarregpurpose regpurpose);


// Enum used with generateloadinstr()
// and loadregwithderefvar().
typedef enum {
	// When used the value of the variable
	// is loaded in the register.
	LOADVALUE,
	
	// When used the address of the variable
	// is loaded in the register.
	LOADADDR
	
} loadflag;

// This function will generate instructions
// which will load the register pointed by r
// with a value from memory. It is only used with
// the lyricalvariable thisvar, the lyricalvariable returnvar,
// lyricalvariable created for predeclared variables and
// lyricalvariable which have their field type set (Variables
// which take memory space in the stack or global variable region),
// hence the argument v should never be a lyricalvariable
// for which the value is generated by the compiler,
// or a dereference variable, or a lyricalvariable which
// has its name suffixed with an offset.
// When the argument flag is LOADADDR, the address of
// the variable is loaded otherwise its value is loaded.
// The argument size can only be a powerof2
// less than or equal to sizeofgpr.
// When the argument flag is LOADADDR,
// the argument size is not used.
// The fields of the register pointed by r
// are not modified and that register should
// have been locked before calling this function,
// since this function can make use of functions which
// call allocreg() and which could use that register
// while trying to look for a new register.
// When the argument flag is not LOADADDR,
// r->size is used by this function and must
// have been correctly set.
void generateloadinstr (lyricalreg* r, lyricalvariable* v, uint size, uint offset, loadflag flag) {
	// The algoritm used to retrieve
	// the value of a variable or its address
	// is divided in three parts.
	
	// The first part of the algorithm is
	// to determine the address of the region
	// where the variable is located.
	
	// This variable will point to
	// the lyricalreg which hold
	// the address of the region containing
	// the variable to load.
	lyricalreg* regptrtovarregion;
	
	// This variable will be used
	// to save the value returned
	// from varfunclevel().
	uint funclevel;
	
	// Pseudo offset which is used along
	// the actual offset within the variable,
	// when determining the correct aligned
	// memory access instruction that will
	// not caused a misaligned memory access.
	// In fact the actual offset within
	// the variable assume that the base address
	// of the variable is aligned to sizeofgpr.
	// The pseudo offset allow to take into account
	// that the base address of the variable
	// may not be aligned to sizeofgpr,
	// and can only be computed when
	// the base address is a known constant.
	uint pseudooffset = 0;
	
	if (v->predeclared) {
		// I call getregforvar() in order to re-use any
		// register that already has the address value
		// of the predeclared variable.
		regptrtovarregion = getregforvar(getvarnumber((uint)v->predeclared->varaddr, voidptrstr),
			0, voidptrstr, 0, FORINPUT);
		
		pseudooffset = (uint)v->predeclared->varaddr % sizeofgpr;
		
	} else if (v == &thisvar) {
		// If I get here the lyricalvariable is thisvar
		// and I need to set in regptrtovarregion
		// the lyricalreg holding the address of "this".
		// There is no need to lock the lyricalreg
		// pointed by r if I am going to call
		// getregptrtothis(), because it should
		// have already been locked before
		// calling this function.
		regptrtovarregion = getregptrtothis();
		
	} else if (v == &returnvar) {
		// If I get here the lyricalvariable is returnvar
		// and I need to set in regptrtovarregion
		// the lyricalreg holding the address of retvar.
		// There is no need to lock the lyricalreg
		// pointed by r if I am going to call
		// getregptrtoretvar(), because it should
		// have already been locked before
		// calling this function.
		regptrtovarregion = getregptrtoretvar();
		
	} else if (v->funcowner == rootfunc) {
		// If I get here the lyricalvariable is
		// a global variable and I need to set
		// in regptrtovarregion a lyricalreg holding
		// the address of the region containing
		// global variables.
		// There is no need to lock the lyricalreg
		// pointed by r if I am going to call
		// getregptrtoglobalregion(), because it should
		// have already been locked before
		// calling this function.
		regptrtovarregion = getregptrtoglobalregion();
		
	} else if (funclevel = varfunclevel(v)) {
		// I get here if the result of
		// the above assignment is non-null and
		// it mean that the variable is not
		// within the current function.
		// If the variable do not belong to
		// the root function (in other words if
		// the variable is not a global variable)
		// and is not in the current function,
		// I generate instructions to compute
		// the stackframe address of the parent
		// function to which it belong.
		// There is no need to lock the lyricalreg
		// pointed by r if I am going to call
		// getregptrtofuncstackframe(), because
		// it should have already been locked
		// before calling this function.
		regptrtovarregion = getregptrtofuncstackframe(funclevel);
		
	} else {
		// If I get here the lyricalvariable is
		// within the current function and
		// the stackframe address of the current
		// lyricalfunction is in the stack pointer register.
		regptrtovarregion = &rstack;
	}
	
	// I lock the allocated register to prevent
	// another call to allocreg() from using it.
	// I also lock it, otherwise it could be lost
	// when insureenoughunusedregisters() is called
	// while creating a new lyricalinstruction.
	regptrtovarregion->lock = 1;
	
	
	// The second part of the algorithm
	// is to generate the immediate value
	// for the offset within the region
	// where the variable is located.
	
	// Increment the offset given as argument
	// to this function (which is the offset within
	// the variable) by the offset of the variable.
	// If v point to thisvar, returnvar or a lyricalvariable
	// for a predeclared variable, v->offset will always be 0.
	offset += v->offset;
	
	lyricalimmval* generateimmvalue (uint loadoffset) {
		
		lyricalimmval* immvalue = mmallocz(sizeof(lyricalimmval));
		immvalue->type = LYRICALIMMVALUE;
		
		lyricalfunction* f = v->funcowner;
		
		if (f != rootfunc) {
			// I get here only for variables which
			// reside in the stack.
			
			if (v->argorlocal == &f->varg) {
				// I get here if the variable is a function argument.
				
				if (f->firstpass->stackframeholder) {
					// I get here if f make use
					// of a stackframe holder.
					
					immvalue->n = 3*sizeofgpr;
					
					if (!stringiseq2(f->type, "void"))
						immvalue->n += sizeofgpr;
					
					lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f->firstpass->stackframeholder->secondpass;
					
					immvalue->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
					imm->sharedregion = f->firstpass->sharedregiontouse;
					
					immvalue->next->next = imm;
					
				} else {
					// I get here if f is a stackframe holder.
					
					immvalue->n = 7*sizeofgpr;
					
					lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					immvalue->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					immvalue->next->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMLOCALVARSSIZE;
					imm->f = f;
					
					immvalue->next->next->next = imm;
				}
				
			} else if (v->argorlocal == &f->vlocal) {
				// I get here if the variable is a local variable.
				
				if (f->firstpass->stackframeholder) {
					// I get here if f make use
					// of a stackframe holder.
					
					immvalue->n = 3*sizeofgpr;
					
					if (!stringiseq2(f->type, "void"))
						immvalue->n += sizeofgpr;
					
					// If the function for which the lyricalfunction is pointed
					// by f is variadic, MAXARGUSAGE is taken as the amount
					// of memory that will be used by its arguments.
					if (f->isvariadic) immvalue->n += MAXARGUSAGE;
					else {
						// When non-null the field f->varg point to
						// the lyricalvariable for the last created argument.
						lyricalvariable* varg = f->varg;
						
						// I increment immvalue->n by the size
						// occupied by all its arguments.
						if (varg) immvalue->n += (varg->offset + varg->size);
					}
					
					lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f->firstpass->stackframeholder->secondpass;
					
					immvalue->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
					imm->sharedregion = f->firstpass->sharedregiontouse;
					
					immvalue->next->next = imm;
					
				} else {
					// I get here if f is a stackframe holder.
					
					immvalue->n = sizeofgpr;
					
					lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					immvalue->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					immvalue->next->next = imm;
				}
			}
		}
		
		// I increment the immediate value
		// by the value computed in offset,
		// and the offset given as argument
		// to this function.
		immvalue->n += (offset + loadoffset);
		
		// If the immediate value is null,
		// set immvalue to null after freeing
		// the previously allocated lyricalimmval.
		if (!(immvalue->next || immvalue->n)) {
			mmrefdown(immvalue);
			immvalue = 0;
		}
		
		return immvalue;
	}
	
	
	// The third part of the algorithm is
	// to generate instructions to load into
	// the register, the desired value; either
	// the address of the variable or its value.
	
	lyricalinstruction* i;
	
	lyricalimmval* immvalue;
	
	if (flag == LOADADDR) {
		
		if (immvalue = generateimmvalue(0)) {
			// The following instruction is created:
			// addi r, regptrtovarregion, immvalue;
			i = newinstruction(currentfunc, LYRICALOPADDI);
			
			i->imm = immvalue;
			
		} else {
			// The following instruction is created:
			// cpy r, regptrtovarregion;
			i = newinstruction(currentfunc, LYRICALOPCPY);
		}
		
		i->r1 = r->id;
		i->r2 = regptrtovarregion->id;
		
		r->waszeroextended = 0;
		
	} else {
		// Determine the correct and optimized
		// aligned load instructions to use.
		
		uint loadsize = sizeofgpr;
		
		while ((offset+pseudooffset) % loadsize) loadsize /= 2;
		
		uint nbrofloads;
		
		if (loadsize > size) {
			
			loadsize = size;
			
			nbrofloads = 1;
			
		} else nbrofloads = size/loadsize;
		// The value of size and loadsize
		// will always be set to a powerof2.
		
		lyricalreg* regtoload = r;
		
		uint count = 0;
		
		// Issue as many load instructions as needed.
		while (1) {
			
			uint loadoffset = count*loadsize;
			
			if (loadsize == 1) {
				
				if (immvalue = generateimmvalue(loadoffset)) {
					// The following instruction is created:
					// ld8 regtoload, regptrtovarregion, immvalue;
					i = newinstruction(currentfunc, LYRICALOPLD8);
					
					i->imm = immvalue;
					
				} else {
					// The following instruction is created:
					// ld8r regtoload, regptrtovarregion;
					i = newinstruction(currentfunc, LYRICALOPLD8R);
				}
				
			} else if (loadsize == 2) {
				
				if (immvalue = generateimmvalue(loadoffset)) {
					// The following instruction is created:
					// ld16 regtoload, regptrtovarregion, immvalue;
					i = newinstruction(currentfunc, LYRICALOPLD16);
					
					i->imm = immvalue;
					
				} else {
					// The following instruction is created:
					// ld16r regtoload, regptrtovarregion;
					i = newinstruction(currentfunc, LYRICALOPLD16R);
				}
				
			} else if (loadsize == 4) {
				
				if (immvalue = generateimmvalue(loadoffset)) {
					// The following instruction is created:
					// ld32 regtoload, regptrtovarregion, immvalue;
					i = newinstruction(currentfunc, LYRICALOPLD32);
					
					i->imm = immvalue;
					
				} else {
					// The following instruction is created:
					// ld32r regtoload, regptrtovarregion;
					i = newinstruction(currentfunc, LYRICALOPLD32R);
				}
				
			} else if (loadsize == 8) {
				
				if (immvalue = generateimmvalue(loadoffset)) {
					// The following instruction is created:
					// ld64 regtoload, regptrtovarregion, immvalue;
					i = newinstruction(currentfunc, LYRICALOPLD64);
					
					i->imm = immvalue;
					
				} else {
					// The following instruction is created:
					// ld64r regtoload, regptrtovarregion;
					i = newinstruction(currentfunc, LYRICALOPLD64R);
				}
				
			} else throwerror(stringfmt("internal error: %s: loadsize invalid", __FUNCTION__).ptr);
			
			i->r1 = regtoload->id;
			i->r2 = regptrtovarregion->id;
			
			if (regtoload != r) {
				
				slli(regtoload, regtoload, 8*loadoffset);
				
				or(r, r, regtoload);
			}
			
			if (++count < nbrofloads) {
				
				if (regtoload == r) {
					// I lock the allocated register, otherwise
					// it will be seen as an unused register
					// when generating lyricalinstruction,
					// since it is not assigned to anything.
					(regtoload = allocreg(CRITICALREG))->lock = 1;
				}
				
			} else break;
		}
		
		if (regtoload != r) {
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			regtoload->lock = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("reg %%%d discarded", regtoload->id));
			}
		}
		
		// Loading a value from memory into a register
		// zero extend it if the register value size
		// is greater than or equal to the value size
		// loaded from memory.
		r->waszeroextended = (r->size >= size);
	}
	
	r->wassignextended = 0;
	
	// Unlock lyricalreg.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	regptrtovarregion->lock = 0;
	
	// The register pointed by r should not
	// be moved to the bottom or top of
	// the linkedlist of lyricalreg just yet.
	// It will be the function using this function
	// which will do the work according to its need.
	// The register pointed by regptrtovarregion
	// has already been moved to the bottom of
	// the linkedlist when it was obtained either
	// through getregptrtoretvar() or getregptrtoglobalregion()
	// or getregptrtofuncstackframe(); if I instead
	// used the stack register, then there was no need
	// to move it to the bottom of the linkedlist,
	// since the stack register do not exist
	// in the linkedlist of lyricalreg
	// pointed by currentfunc->gpr.
}


// This function return a register associated
// with the variable given by the argument v.
// The argument offset give the location
// to load within the variable.
// The argument type should not reduce
// to a size greater than sizeofgpr,
// otherwise an error is thrown.
// The argument bitselect is the integer value
// to use for the bitselect.
// Any register currently being used should be locked
// before calling this function because it can use
// allocreg() which can cause any unlocked register
// to be discarded while looking for a new register.
// When regpurpose == FOROUTPUT, this function
// do not check whether the register to return
// should be sign or zero extended.
// Note that the field cast or type of
// the lyricalvariable given through the argument v
// must be correctly set regardless of what
// was given through the argument type; the argument
// type is used to compute the size that I wish
// to load; and it may not necessarily match
// the field cast or type of the lyricalvariable
// given through the argument v; the argument type
// is also used to determine whether to sign or
// zero extend the value loaded in the register.
lyricalreg* getregforvar (lyricalvariable* v, uint offset, string type, u64 bitselect, getregforvarregpurpose regpurpose) {
	// Variable used to save and restore
	// v->cast if v is a dereference variable.
	string savedcast = stringnull;
	
	// If I have a variable such as "(*(cast*)var)"
	// where "cast" is the name of the type or cast
	// used when dereferencing, with no field offset
	// suffixed to the variable name, ei: (*(cast*)var).8,
	// I use the cast embedded in the name of the variable,
	// in order to make the correct memory access when loading
	// a register with the dereference variable value. ei:
	// In an expression such as ((u16)(*(u8*)var)),
	// I am making an 8bits memory access, but the cast
	// of the dereference variable value will be a u16.
	if (v->name.ptr[1] == '*' && v->name.ptr[stringmmsz(v->name)-1] == ')') {
		
		savedcast = v->cast;
		
		// This function is used to obtain the address location
		// of the closing paranthesis which end the cast
		// in the name of a dereference variable.
		// Its argument should be set to the location right after "(*(".
		// This function should never fail because it go through
		// a type string that is supposed to have been correctly parsed.
		u8* findwherecastend(u8* c) {
			
			uint openingparanthesisfound = 0;
			
			while (1) {
				
				if (*c == ')') {
					
					if (!openingparanthesisfound) break;
					
					--openingparanthesisfound;
					
				} else if (*c == '(') ++openingparanthesisfound;
				
				++c;
			}
			
			return c;
		}
		
		// The following instructions
		// set s to the cast used
		// when dereferencing, stripping
		// the last asterix, since
		// the cast should certainly
		// be a pointer.
		u8* ptr = (v->name.ptr +3);
		uint sz = ((uint)findwherecastend(ptr) - (uint)ptr) -1;
		
		type = stringduplicate3(ptr, sz);
		
		v->cast = type;
		
		// It is ok for v->cast and type
		// to share the same string.
	}
	
	// This variable will point
	// to the register to return.
	lyricalreg* r;
	
	uint size;
	
	enum {
		SIGNEXTEND,
		ZEROEXTEND,
		NOSIGNORZEROEXTEND
		
	} signorzeroextend;
	
	// This function is called before
	// returning and generate the sign or zero
	// extension instruction on the register
	// pointed by r depending on the value
	// of signorzeroextend.
	// This function is never called
	// if regpurpose == FOROUTPUT or
	// if signorzeroextend == NOSIGNORZEROEXTEND or
	// if the variable pointed by v is a number.
	void dosignorzeroextension () {
		// There is no need to do sign/zero
		// extending if "size" is null.
		if (!size) return;
		
		if (signorzeroextend == SIGNEXTEND) {
			// There is no need to sign extend
			// the register value if it was
			// already sign extended.
			if (r->wassignextended) return;
			
			uint sxtamount;
			
			if (bitselect)
				sxtamount = countoflsbones(bitselecttoboundary(bitselect));
			else sxtamount = countoflsbones(sizetoboundary(size));
			
			// There is also no need to sign
			// extend the register if the
			// sign extension amount is 0
			// or bitsizeofgpr.
			if (!(sxtamount % bitsizeofgpr)) return;
			
			sxt(r, r, sxtamount);
			
		} else if (signorzeroextend == ZEROEXTEND) {
			// There is no need to zero extend
			// the register value if it was
			// already zero extended.
			if (r->waszeroextended) return;
			
			uint zxtamount;
			
			if (bitselect)
				zxtamount = countoflsbones(bitselecttoboundary(bitselect));
			else zxtamount = countoflsbones(sizetoboundary(size));
			
			// There is also no need to zero
			// extend the register if the
			// zero extension amount is 0
			// or bitsizeofgpr.
			if (!(zxtamount % bitsizeofgpr)) return;
			
			zxt(r, r, zxtamount);
		}
		
		// For a register associated to a variable
		// generated by the compiler, it should be
		// deassociated with that variable because
		// it would no longer reflect the value
		// of that variable.
		// The deassociation is done only
		// if size != sizeofgpr, because a sign
		// or zero extension would have been
		// certainly done.
		if (size != sizeofgpr && isvarreadonly(r->v)) {
			
			r->v = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("reg %%%d discarded", r->id));
			}
			
			// The deassociated register is moved
			// to the top of the linkedlist of registers
			// so as to be readily available after having
			// been used by the routine which requested it
			// through getregforvar().
			setregtothetop(r);
		}
	}
	
	// This function generate instructions
	// to load the register pointed by r with
	// the value of the variable pointed by v,
	// making the difference between
	// the different type of variables.
	// The register pointed by r should have been
	// locked before calling this function, since
	// this function can call functions which
	// may make use of allocreg().
	// If the register pointed by r is associated
	// to a variable, meaning its field v is set,
	// the register must not be dirty, because when
	// this function process a dereference variable,
	// it discard the register without doing any flushing.
	// This function also set the field size
	// of the lyricalreg pointed by r.
	void loadregwithvarvalue () {
		// The field size of the register is set
		// to the amount to load in it.
		r->size = size;
		
		// This function set in the lyricalreg
		// pointed by r, the value or address of
		// a dereferenced lyricalvariable given
		// through the argument v.
		// The argument offset is added to the address
		// of the dereferenced variable; and
		// the dereference lyricalvariable given
		// through the argument v must never be one
		// that has its name suffixed with an offset.
		// The argument flag determine whether
		// the value or address of the dereferenced
		// variable is to be loaded.
		void loadregwithderefvar (lyricalvariable* v, uint offset, loadflag flag) {
			// The register pointed by r was locked,
			// it is unlocked and discarded before
			// recursing inside getregforvar().
			// The reason for discarding the register is because,
			// if I have a lot of dereference associated with
			// the variable, not freeing the register will
			// cause the compiler to run out of available
			// registers when recursing to resolve all
			// the dereferencing.
			// If the register pointed by r is associated
			// to a variable, meaning, its field v is set,
			// the register would have already been flushed
			// if it was dirty before calling loadregwithvarvalue();
			// so I can safely discard it here.
			r->lock = 0;
			
			r->v = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("reg %%%d discarded", r->id));
			}
			
			setregtothetop(r);
			
			// I recursively call getregforvar()
			// to load into raddr the address of the
			// dereference variable which is
			// the variable which name is
			// between "(*(cast)" and ")" .
			lyricalvariable* vaddr = getvaraddr(v, DONOTMAKEALWAYSVOLATILE);
			lyricalreg* raddr = getregforvar(vaddr, 0, voidptrstr, 0, FORINPUT);
			
			raddr->lock = 1;
			
			if (flag == LOADADDR) {
				// I get here if I am loading the address
				// of the dereferenced variable.
				
				// I add the offset to the address
				// of the dereference variable.
				addi(raddr, raddr, offset);
				
				r = raddr;
				
				// The fields v, size and offset of the lyricalreg
				// pointed by r will be reset upon returning
				// from this function.
				// The fields waszeroextended and wassignextended
				// of the register pointed by r would have
				// been correctly set.
				
			} else {
				// I get here if I am loading the value
				// of the dereferenced variable.
				
				// getvaraddr() will cast the variable
				// that it return using the type with which
				// the dereference was done.
				// ei: obtaining the address of a lyricalvariable
				// named "(*(u8*)var)" will yield a variable
				// having the cast u8* .
				// It is necessary to consider the type with
				// which the dereference was done in order
				// to make the correct memory size access when
				// reading the value of a dereferenced variable.
				// In an expression such as ((u16)(*(u8*)var)),
				// I am making an 8bits memory access, but
				// the type of the dereference variable value
				// will be a u16 since a cast was done.
				
				// I allocate a new register which will
				// contain the value of the dereference variable.
				// Since the allocated register is not
				// assigned to anything, I lock it,
				// otherwise it will be seen as an unused
				// register when generating lyricalinstruction.
				// It will be unlocked if it end up being discarded
				// or when loadregwithvarvalue() return.
				// setregtothebottom() is called by
				// the routine which called this function.
				(r = allocreg(NONCRITICALREG))->lock = 1;
				
				// Since the initial lyricalreg pointed by r
				// was discarded and a new lyricalreg was allocated,
				// the field size of the new lyricalreg need to be set.
				// The field size of the register is set to the amount
				// to load in it, so that when using opcode instructions
				// within opcodes.tools.parsestatement, the sign or
				// zero extended state get preserved or set correctly.
				r->size = size;
				
				// I chop the last character of vaddr->cast
				// in order to remove the last asterix from
				// the type with which the dereference was done
				// yielding the load type.
				// I am guaranteed that there will always
				// be an ending asterix.
				stringchop(&vaddr->cast, 1);
				
				// I generate instructions to load
				// the dereference variable using the size
				// with which the variable was dereferenced.
				// I also use the argument offset.
				
				uint loadsize = sizeoftype(vaddr->cast.ptr, stringmmsz(vaddr->cast));
				
				if (loadsize > sizeofgpr) loadsize = sizeofgpr;
				
				// Determine the correct and optimized
				// aligned load instructions to use.
				
				// Pseudo offset which is used along
				// the actual offset within the variable,
				// when determining the correct aligned
				// memory access instruction that will
				// not caused a misaligned memory access.
				// In fact the actual offset within
				// the variable assume that the base address
				// of the variable is aligned to sizeofgpr.
				// The pseudo offset allow to take into account
				// that the base address of the variable
				// may not be aligned to sizeofgpr,
				// and can only be computed when
				// the base address is a known constant.
				uint pseudooffset = (vaddr->isnumber ? (vaddr->numbervalue % sizeofgpr) : 0);
				
				while ((offset+pseudooffset) % loadsize)
					loadsize /= 2;
				
				uint nbrofloads;
				
				if (loadsize > size) {
					
					loadsize = size;
					
					nbrofloads = 1;
					
				} else nbrofloads = size/loadsize;
				// The value of size and loadsize
				// will always be set to a powerof2.
				
				lyricalreg* regtoload = r;
				
				uint count = 0;
				
				// Issue as many load instructions as needed.
				while (1) {
					
					uint loadoffset = count*loadsize;
					
					if 	(loadsize == 1)  ld8(regtoload, raddr, offset+loadoffset);
					else if (loadsize == 2) ld16(regtoload, raddr, offset+loadoffset);
					else if (loadsize == 4) ld32(regtoload, raddr, offset+loadoffset);
					else if (loadsize == 8) ld64(regtoload, raddr, offset+loadoffset);
					else throwerror(stringfmt("internal error: %s: loadsize invalid", __FUNCTION__).ptr);
					
					if (regtoload != r) {
						
						slli(regtoload, regtoload, 8*loadoffset);
						
						or(r, r, regtoload);
					}
					
					if (++count < nbrofloads) {
						
						if (regtoload == r) {
							// I lock the allocated register, otherwise
							// it will be seen as an unused register
							// when generating lyricalinstruction,
							// since it is not assigned to anything.
							(regtoload = allocreg(CRITICALREG))->lock = 1;
						}
						
					} else break;
				}
				
				if (regtoload != r) {
					// Unlock lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					regtoload->lock = 0;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						comment(stringfmt("reg %%%d discarded", regtoload->id));
					}
				}
				
				// The ld() functions would have correctly
				// set the fields waszeroextended and wassignextended
				// of the register pointed by r.
			}
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			raddr->lock = 0;
		}
		
		if (v->isfuncaddr) {
			// I generate the instruction
			// that will load the address of
			// the function in the register.
			
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPAFIP);
			i->r1 = r->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			
			i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
			i->imm->f = v->isfuncaddr;
			
			// The register pointed by r will be
			// moved to the bottom of the linkedlist of
			// registers, right before I exit getregforvar().
			
		} else if (v->isnumber) {
			// I set the register pointed
			// by r with the number value.
			li(r, v->numbervalue);
			// li() would have correctly set
			// the fields waszeroextended and wassignextended
			// of the register pointed by r.
			
			// The register pointed by r will be moved
			// to the bottom of the linkedlist of
			// registers right before I exit getregforvar().
			
		} else if (v->isstring) {
			// I generate the instructions that will
			// load in the register pointed by r
			// the address of the constant string.
			
			// Offset within the string region.
			uint o = v->isstring -1;
			
			// getregptrtostringregion() could make use of
			// allocreg() which can discard unlocked registers,
			// but note that the lyricalreg pointed by r
			// get locked before calling this function.
			lyricalreg* regptrtostringregion = getregptrtostringregion();
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			regptrtostringregion->lock = 1;
			
			if (o) addi(r, regptrtostringregion, o);
			else cpy(r, regptrtostringregion);
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			regptrtostringregion->lock = 0;
			
			// addi() would have correctly set the fields
			// waszeroextended and wassignextended of
			// the register pointed by r.
			
			// The register pointed by r will be moved
			// to the bottom of the linkedlist of
			// registers right before I exit getregforvar().
			
		} else if (v->name.ptr[1] == '&') {
			// To obtain the value of an address variable
			// I need to dereference the variable in order to
			// get the variable which name is between "&(" and ")".
			// Note that I cannot have a lyricalvariable
			// with v->name == "(&(*(cast)var))" because
			// getvaraddr() which generate lyricalvariable
			// for address variables would always return
			// the lyricalvariable v->name == "var" for a given
			// lyricalvariable v->name == "(*(cast)var)";
			// but in a situation where a lyricalvariable
			// with v->name == "(*(cast)var).8" is given
			// to getvaraddr(), the lyricalvariable returned
			// would be v->name == "(&(*(cast)var).8)" .
			
			// The cast of the lyricalvariable pointed by v is
			// set to a valid type that can be dereferenced; ei:
			// The following expression would cause dereferencevar()
			// to generate the error message "incorrect dereference"
			// because the variable for which the address is requested
			// get casted before the assignment: var1 = (uint)&var2;
			
			if (v->cast.ptr) mmrefdown(v->cast.ptr);
			
			v->cast = stringduplicate2("void**"); // "void**" is used because its stride is sizeofgpr.
			
			lyricalvariable* vderef = dereferencevar(v);
			
			// processvaroffsetifany() will make sure
			// that the result of the above dereference
			// is not a lyricalvariable which has an offset
			// suffixed to its name because such lyricalvariable
			// must not be used with loadregwithderefvar()
			// or generateloadinstr().
			uint o = processvaroffsetifany(&vderef);
			
			// If the result of dereferencevar() and
			// processvaroffsetifany() is a dereferenced variable,
			// I cannot use it with generateloadinstr() (Which
			// only process variables which take memory space
			// in the stack or global variable region).
			if (vderef->name.ptr[1] == '*') {
				// I load in the lyricalreg pointed by r,
				// the value of the dereferenced lyricalvariable
				// pointed by vderef.
				loadregwithderefvar(vderef, o, LOADADDR);
				
				// I set the field size of the lyricalreg
				// pointed by r to sizeofgpr, since the variable
				// is an address variable.
				r->size = sizeofgpr;
				
			} else generateloadinstr(r, vderef, 0, o, LOADADDR);
			
			// loadregwithderefvar() or generateloadinstr()
			// would have correctly set the fields waszeroextended
			// and wassignextended of the register pointed by r.
			
			// The register pointed by r will be moved to
			// the bottom of the linkedlist of registers
			// right before I exit getregforvar().
			
		} else if (bitselect) {
			// I get the unbitselected value of the variable.
			lyricalreg* regforunbitselectedvar = getregforvar(v, offset, type, 0, FORINPUT);
			
			regforunbitselectedvar->lock = 1;
			
			// Here I shift the unbitselected value to the right
			// in order to have the bitselected value correctly
			// formed in the least significant bits of the register.
			// The field bitselect as well as the other fields of
			// the register are set later after this function return.
			
			srli(r, regforunbitselectedvar, countoflsbzeros(bitselect));
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			regforunbitselectedvar->lock = 0;
			
			// The bitselected value need
			// to be sign/zero extended.
			r->waszeroextended = 0;
			r->wassignextended = 0;
			
			// getregforvar() used to obtain the register
			// pointed by regforunbitselectedvar has called setregtothebottom().
			// The register pointed by r will be moved
			// to the bottom of the linkedlist of registers
			// right before exiting getregforvar().
			
		} else if (v->name.ptr[1] == '*') {
			// I load in the lyricalreg pointed by r,
			// the value of the dereferenced lyricalvariable
			// pointed by v.
			loadregwithderefvar(v, offset, LOADVALUE);
			
			// The register pointed by r will be moved to
			// the bottom of the linkedlist of registers
			// right before I exit getregforvar().
			
		} else {
			// If I get here the variable pointed by
			// the argument v is a regular variable or
			// an intermediate variable result of function
			// or operator; so I call generateloadinstr()
			// to generate instructions to load its value.
			// The register pointed by r should have
			// already been locked.
			generateloadinstr(r, v, size, offset, LOADVALUE);
			
			// generateloadinstr() would have correctly
			// set the fields waszeroextended and wassignextended
			// of the register pointed by r.
			
			// The register pointed by r will be moved to
			// the bottom of the linkedlist of registers
			// right before I exit getregforvar().
		}
	}
	
	// This function check if the register
	// pointed by r is associated with a variable
	// which overlap the data region specified
	// by v, size, offset and bitselect.
	// Note that a bitselected variable and its main
	// variable are not considered as being the same,
	// but rather as overlapping variables.
	// This function is never called if the variable
	// pointed by v is a dereferenced variable
	// otherwise it can give wrong results, because
	// a dereferenced variable would be seen as
	// a local variable whereas its field size is null.
	uint isregoverlapping(lyricalreg* r) {
		// If the variable pointed by v is the same as
		// the variabale associated with the register,
		// I should return false because I am only
		// trying to check whether the variable
		// associated with the register is overlapping
		// the variable pointed by v.
		if ((r->v == v) && (r->offset == offset) && (r->size == size) && (r->bitselect == bitselect))
			return 0;
		
		if (!isvarreadonly(r->v)) {
			// If I get here, the variable associated with
			// the register pointed by r reside either in
			// the stack or global variable region; and
			// in order for the 2 variables being compared
			// to overlap, they have to belong to the same
			// function(Implying to the same stack or
			// be both global variables) and be both either
			// arguments or local variables.
			if ((r->v->funcowner == v->funcowner) && (r->v->argorlocal == v->argorlocal)) {
				// If I get here, using the offset of
				// the variables within the stack or global
				// variable region, and the offset from
				// the beginning of those variables,
				// I determine if the variables overlap.
				if ((r->v->offset + r->offset == v->offset + offset) ||
					((r->v->offset + r->offset < v->offset + offset) &&
					(r->v->offset + r->offset + r->size > v->offset + offset)) ||
					((r->v->offset + r->offset > v->offset + offset) &&
					(r->v->offset + r->offset < v->offset + offset + size)))
					return 1;
			}
		}
		
		return 0;
	}
	
	if (regpurpose != FOROUTPUT)
		signorzeroextend = (!pamsynmatch2(isnativetype, type.ptr, stringmmsz(type)).start) ? NOSIGNORZEROEXTEND:
			type.ptr[0] == 's' ? SIGNEXTEND : ZEROEXTEND;
	
	size = sizeoftype(type.ptr, stringmmsz(type));
	
	if (isvarreadonly(v)) {
		// If signorzeroextend != NOSIGNORZEROEXTEND and
		// the variable pointed by v is a variable number,
		// I replace it with its equivalent sign or zero
		// extended variable number.
		if (signorzeroextend != NOSIGNORZEROEXTEND && v->isnumber) {
			
			u64 value = v->numbervalue;
			
			if (signorzeroextend == SIGNEXTEND) {
				
				u64 msbzeros = countofmsbzeros(sizetoboundary(size));
				
				if (msbzeros) {
					value <<= msbzeros;
					value = ((s64)value >> msbzeros);
				}
				
			} else {
				// I apply the correct boundary to ensure
				// that the immediate value contain a value
				// within the correct boundaries.
				value &= sizetoboundary(size);
			}
			
			// Note that getvarnumber() will duplicate
			// the string pointed by type.ptr
			// before using it.
			v = getvarnumber(value, type);
		}
		
		// The variable size must be set to null
		// because it is used by loadregwithvarvalue()
		// to set the field size of the lyricalreg to return;
		// in fact registers for which the value is generated
		// by the compiler must have their field size null.
		size = 0;
		
	} else {
		// I call processvaroffsetifany() to check
		// if the variable pointed by v had an offset
		// suffixed to its name. If yes, it will find
		// the main variable and adjust the variable
		// offset which give the location of the variable
		// from the beginning of the main variable.
		// Note that an offset is never suffixed
		// to the name of variables which do not
		// reside in memory; also the field v of
		// a lyricalreg should never be set to a variable
		// which has a name suffixed with an offset,
		// instead it should be set to its main variable
		// and the field offset of the lyricalreg
		// set appropriatly.
		// For a variable such as "(*var.8).4",
		// the variable with an offset which is between
		// the paranthesis is resolved recursively when
		// this function is called again
		// by loadregwithvarvalue().
		offset += processvaroffsetifany(&v);
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		
		string s = stringduplicate2("begin: allocating register");
		
		string strnum;
		
		if (v->isnumber) {
			
			strnum = stringfmt("; for %d", v->numbervalue);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
		} else {
			
			stringappend2(&s, "; for \"");
			stringappend1(&s, v->name);
			stringappend4(&s, '"');
		}
		
		if (isvarreadonly(v)) stringappend2(&s, "; readonly");
		else {
			stringappend2(&s, "; offset within variable ");
			
			strnum = stringfmt("%d", offset);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
			stringappend2(&s, "; offset within region ");
			
			strnum = stringfmt("%d", v->offset);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
			stringappend2(&s, "; size ");
			
			strnum = stringfmt("%d", size);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
		}
		
		if (bitselect) {
			
			stringappend2(&s, "; bitselect 0x");
			
			strnum = stringfmt("%x", bitselect);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
		}
		
		if (regpurpose == FORINPUT) {
			
			if (*v->alwaysvolatile) stringappend2(&s, "; volatile input");
			else stringappend2(&s, "; input");
			
		} else {
			
			if (*v->alwaysvolatile) stringappend2(&s, "; volatile output");
			else stringappend2(&s, "; output");
		}
		
		comment(s);
	}
	
	void genendcomment () {
		comment(stringfmt("end: done: allocated %%%d", r->id));
	}
	
	if (size > sizeofgpr) size = sizeofgpr;
	
	if (v->size) {
		// The check on whether offset >= v->size is
		// not necessary because evaluateexpression()
		// insure that an offset to use with a variable
		// be always valid; but for future reference
		// the check is there.
		if (offset >= v->size) {
			// I get here if the offset is outside
			// of the boundary of the variable in memory.
			// It would be illegal to try to read beyond
			// the size of the variable.
			
			throwerror(stringfmt("internal error: %s: invalid variable access", __FUNCTION__).ptr);
			
		} else if (size > (v->size - offset)) {
			// I get here if the size requested
			// is greater than the size of the variable
			// pointed by v; and it would be illegal for
			// the programmer to be allowed to access
			// data beyond the size of the variable.
			
			throwerror(stringfmt("internal error: %s: invalid variable access", __FUNCTION__).ptr);
		}
	}
	
	// *** Rule about the loading of registers ***
	// 
	// There can only be one dirty register
	// for a variable bitselected or non-bitselected
	// spanning a specific region of stack or
	// global variable region, with no other
	// register allocated to a variable overlapping
	// that region of memory.
	// On the other hand I can have more than one
	// clean register bitselected or non-bitselected
	// spanning a specific region of stack or
	// global variable region, with many other
	// non-dirty register allocated to variables
	// overlapping that same region of memory.
	// 
	// Before loading:
	// - If there is a register overlapping
	// the region from where I am loading my value:
	// 	- The variable being loaded is volatile:
	// 		- If the register overlapping is dirty,
	// 		it is flushed and discarded.
	// 		- If the register overlapping is clean,
	// 		I simply discard it.
	// 	- The variable being loaded is non-volatile:
	// 		- If the register overlapping is dirty,
	// 		it is flushed without being discarded.
	// 		- If the register overlapping is clean,
	// 		I leave it alone.
	// - If there is no register overlapping,
	// I simply continue to loading.
	// 
	// Loading:
	// - If I am loading an already loaded value:
	// 	- Volatile:
	// 		- If the already loaded value is clean,
	// 		I simply reload it.
	// 		- If the already loaded value is dirty,
	// 		I flush it and reload it.
	// 	- Non-Volatile:
	// 		- Whether the already loaded value is clean
	// 		or dirty, I use that loaded value.
	// - If there is not an already loaded value,
	// I simply load it.
	
	r = currentfunc->gpr;
	
	// This loop below is used to ensure that
	// registers associated with overlapping variables
	// are in sync with the variable that I am loading.
	// Note that there can be aliasing issues
	// that may happen between a variable and
	// a dereference variable which refer to
	// the same location; and the solution for
	// that type of situation is to make always volatile,
	// dereference variables and also make always volatile
	// variables for which the programmer code
	// obtained the address.
	// That solution is implemented within getvaraddr()
	// and dereferencevar() by setting
	// the field *alwaysvolatile of variables;
	// The loop is skipped for address and
	// constant variables because their value
	// is generated by the compiler.
	// The loop is also skipped for dereference variables;
	// I should not let isregoverlapping() work with
	// dereference variables otherwise it can give
	// wrong results, because a dereference variable
	// would be seen as a local variable whereas
	// its field size is null.
	if (!isvarreadonly(v) && v->name.ptr[1] != '*') {
		
		do {
			// I first check whether r->v is true,
			// because it can be null when the register is not
			// in used or when one of its following field
			// is set: returnaddr, funclevel, globalregionaddr,
			// stringregionaddr, thisaddr, retvaraddr.
			if (r->v && isregoverlapping(r)) {
				
				if (r->dirty) flushreg(r);
				
				// If getregforvar() is called to obtain
				// a register for an output or volatile variable,
				// any overlapping register should be discarded.
				if (regpurpose == FOROUTPUT || *v->alwaysvolatile) {
					
					r->v = 0;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						// The lyricalreg pointed by r is not allocated
						// to a lyricalvariable but since I am done using it,
						// I should also produce a comment about it having been
						// discarded to complement the allocation comment that
						// was generated when it was allocated.
						comment(stringfmt("reg %%%d discarded", r->id));
					}
					
					setregtothetop(r);
				}
			}
			
			r = r->next;
			
		} while (r != currentfunc->gpr); // When I exit the loop, r == currentfunc->gpr;
	}
	
	// Here I do the work necessary for
	// the loading of the register, as stated
	// in the comments about how registers are loaded.
	
	do {
		// If r->v is a variable generated by the compiler,
		// the fields size, offset and bitselect of the register
		// pointed by r are not tested because they are null.
		if (r->v == v && (isvarreadonly(v) || (r->size == size && r->offset == offset && r->bitselect == bitselect))) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			if (regpurpose == FOROUTPUT) {
				// If the register pointed by r is already dirty,
				// it should not be flushed since when
				// regpurpose == FOROUTPUT, the register obtained
				// is to be dirty; so here I set the field dirty to 1
				// because registers obtained using
				// regpurpose == FOROUTPUT are always dirty.
				r->dirty = 1;
				
			// If the variable is always volatile, its value
			// must be read again from memory.
			// v->name can never be "(&(*(cast)var))" because
			// getvaraddr() which generate lyricalvariable for
			// address variables would always return
			// a lyricalvariable with v->name == "(*(cast)var)";
			// but in a situation where a lyricalvariable
			// with v->name == "(*(cast)var).8" is given
			// to getvaraddr(), the lyricalvariable returned
			// has v->name == "(&(*(cast)var).8)" and when that
			// lyricalvariable is used with this function, instructions
			// to load the address variable value should be
			// generated again since the address is the value
			// of the dereference variable (which is always volatile)
			// to which the offset 8 should be added.
			} else if (*v->alwaysvolatile || (v->name.ptr[1] == '&' && v->name.ptr[3] == '*')) {
				
				if (r->dirty) flushreg(r);
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				r->lock = 1;
				
				loadregwithvarvalue();
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r->lock = 0;
				
				// The fields of the lyricalreg are set below
				// to avoid loosing what was previously set,
				// because when for example loadregwithvarvalue()
				// process a dereferenced variable, it discard
				// the lyricalreg pointed by r and allocate
				// a new lyricalreg.
				
				r->v = v;
				
				// The field size, offset and bitselect are null
				// for variables generated by the compiler;
				// but here, it is not necessary to check whether
				// the lyricalvariable pointed by v is generated by
				// the compiler because it will never be the case
				// for an always volatile variable.
				
				// Note that r->size is set by loadregwithvarvalue().
				
				r->offset = offset;
				r->bitselect = bitselect;
			}
			
			setregtothebottom(r);
			
			// dosignorzeroextension() is not to be called
			// for a register allocated for an output; it is
			// also not to be called if there is no sign or
			// zero extension to do, or if the variable pointed
			// by v is a number which would have already been
			// sign or zero extended.
			if (regpurpose != FOROUTPUT &&
				signorzeroextend != NOSIGNORZEROEXTEND &&
				!v->isnumber) {
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				r->lock = 1;
				
				dosignorzeroextension();
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r->lock = 0;
			}
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) genendcomment();
			
			// Restore v->cast if it was modified.
			if (savedcast.ptr) {
				mmrefdown(v->cast.ptr);
				v->cast = savedcast;
			}
			
			return r;
		}
		
		r = r->next;
		
	} while (r != currentfunc->gpr);
	
	// I get here if no register previously allocated
	// for the variable pointed by v was found.
	
	// I call allocreg() with the proper argument depending
	// on whether the lyricalvariable pointed by v need
	// a critical or non-critical register.
	// The following test is similar to what is found in isregcritical().
	if (v != &thisvar && v != &returnvar && currentfunc != rootfunc && !bitselect && v->name.ptr[1] != '*' &&
		// Here below I am checking whether I have
		// a local variable (including locals of a function
		// other than currentfunc for which the tiny stackframe
		// is contained in the shared region of currentfunc).
		(v->funcowner == currentfunc && (v->funcowner->firstpass->stackframeholder &&
			v->funcowner->firstpass->stackframeholder->secondpass == currentfunc)))
				r = allocreg(CRITICALREG);
	else r = allocreg(NONCRITICALREG);
	
	if (regpurpose != FOROUTPUT) {
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		r->lock = 1;
		
		loadregwithvarvalue();
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
	// Register obtained using
	// regpurpose == FOROUTPUT
	// are always dirty.
	} else r->dirty = 1;
	
	// The fields of the lyricalreg
	// are set only here to avoid loosing
	// what is set, because when loadregwithvarvalue()
	// process a dereferenced variable, it discard
	// the lyricalreg pointed by r and
	// allocate a new lyricalreg.
	
	r->v = v;
	
	// If v is a variable generated by the compiler,
	// the fields size, offset and bitselect are not
	// set and remain null.
	if (!isvarreadonly(v)) {
		// Setting the field size of the lyricalreg
		// is redundant if regpurpose != FOROUTPUT
		// because loadregwithvarvalue() would have
		// already done it; but if regpurpose == FOROUTPUT
		// it is necessary to set the field size of
		// the lyricalreg because it would not have been
		// done anywhere else.
		r->size = size;
		r->offset = offset;
		r->bitselect = bitselect;
	}
	
	setregtothebottom(r);
	
	// dosignorzeroextension() is not to be called
	// for a register allocated for an output; it is
	// also not to be called if there is no sign or
	// zero extension to do, or if the variable pointed
	// by v is a number which would have already been
	// sign or zero extended.
	if (regpurpose != FOROUTPUT &&
		signorzeroextend != NOSIGNORZEROEXTEND &&
		!v->isnumber) {
		
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		r->lock = 1;
		
		dosignorzeroextension();
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) genendcomment();
	
	// Restore v->cast if it was modified.
	if (savedcast.ptr) {
		mmrefdown(v->cast.ptr);
		v->cast = savedcast;
	}
	
	return r;
}


// Enum used with flushanddiscardallreg().
typedef enum {
	// When used, all registers are flushed if dirty and then discarded.
	FLUSHANDDISCARDALL,
	
	// When used, all registers are flushed if dirty but are not discarded.
	DONOTDISCARD,
	
	// This flag do the same thing as FLUSHANDDISCARDALL except that if
	// the register is associated with a local variable or used to hold
	// a stackframe pointer, or used to hold the return address, it is
	// discarded without being flushed.
	DONOTFLUSHREGFORLOCALS,
	
	// This flag do the same thing as FLUSHANDDISCARDALL except that if
	// the register is associated with a local variable or used to hold
	// a stackframe pointer, it is discarded without being flushed.
	// The register used to hold the return address is not discarded
	// and do not get flushed.
	// This flag is used before generating the instructions
	// to return from a function.
	DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR,
	
	// This flag do the same thing as FLUSHANDDISCARDALL except that if
	// the register is associated with a local variable or used to hold
	// the return address, it is discarded without being flushed.
	// A register used to hold a stackframe pointer address is not discarded
	// and do not get flushed.
	// This flag is used before calling setregstackptrtofuncstackframe().
	DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL
	
} flushanddiscardallregflag;

// I declared flushanddiscardallreg() here because it is used by flushreg().
// ### GCC wouldn't compile without the use of the keyword auto.
auto void flushanddiscardallreg (flushanddiscardallregflag flag);

// This function write the dirty register passed as argument
// to memory and set its field dirty to null.
// The register is not discarded nor is it moved to the top
// or bottom of the linkedlist of currentfunc->gpr.
// This function should only be used with a dirty register.
// Registers currently being used should be locked before
// calling this function because it can use allocreg()
// which could cause any unlocked register to be discarded
// while looking for a new register.
void flushreg (lyricalreg* r) {
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		
		string s = stringfmt("begin: flush reg %%%d", r->id);
		
		// Note that only registers used for a variable or
		// the return address or a stackframe pointer
		// can be flushed.
		
		if (r->v) {
			
			stringappend2(&s, "; for \"");
			stringappend1(&s, r->v->name);
			stringappend4(&s, '"');
			
			stringappend2(&s, "; offset within variable ");
			
			string strnum = stringfmt("%d", r->offset);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
			stringappend2(&s, "; offset within region ");
			
			strnum = stringfmt("%d", r->v->offset);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
			stringappend2(&s, "; size ");
			
			strnum = stringfmt("%d", r->size);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
			
			if (r->bitselect) {
				
				stringappend2(&s, "; bitselect 0x");
				
				strnum = stringfmt("%x", r->bitselect);
				stringappend1(&s, strnum);
				mmrefdown(strnum.ptr);
			}
			
		} else if (r->returnaddr) stringappend2(&s, "; for return address");
		else {
			
			stringappend2(&s, "; for stackframe #");
			
			string strnum = stringfmt("%d", r->funclevel);
			stringappend1(&s, strnum);
			mmrefdown(strnum.ptr);
		}
		
		comment(s);
	}
	
	// This variable will only be used if
	// the register to flush is bitselected.
	lyricalreg* runbitselected;
	
	// This function will generate instructions to merge
	// the value of the bitselected variable associated with the
	// register pointed by r, with the value of its main variable
	// in memory(The value of its main variable would not have
	// been loaded in any register since the bitselected variable
	// overlap its main variable in memory).
	// None of the fields of the register pointed by r are modified;
	// also the register pointed by r would have been locked
	// before calling this function.
	void mergebitselectedregwithunbitselectedreg () {
		
		lyricalreg* rtmp = allocreg(CRITICALREG);
		
		// I use a temporary register to compute
		// the bitselected value that need to be merged
		// into the main variable; what I am doing is:
		// rtmp = (r<<countoflsbzeros(r->bitselect)) & r->bitselect;
		
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		// I also lock it, otherwise it will
		// be seen as an unused register
		// when generating lyricalinstruction,
		// since it is not assigned to anything.
		rtmp->lock = 1;
		
		slli(rtmp, r, countoflsbzeros(r->bitselect));
		
		andi(rtmp, rtmp, r->bitselect);
		
		// Here I load into a register the value
		// of the main variable.
		// The value of the main variable would not
		// have been loaded in any register, so getregforvar()
		// will certainly generate instructions to load the value.
		// The field dirty of the lyricalreg pointed by r to flush
		// was set to clean before calling this function so that
		// getregforvar() when called below, do not call flushreg()
		// on it while I am still trying to flush it,
		// causing an infinite loop.
		// getregforvar() will make use of isregoverlapping() on
		// the lyricalreg pointed by r and test whether to discard it;
		// but to prevent getregforvar() from discarding that
		// lyricalregister which I am currently flushing,
		// I temporarily set non-volatile the lyricalvariable
		// associated with that register if it was volatile.
		
		uint savedvolatilestate = *r->v->alwaysvolatile;
		*r->v->alwaysvolatile = 0;
		
		if (r->size > sizeofgpr) throwerror(stringfmt("internal error: %s: r->size invalid", __FUNCTION__).ptr);
		
		(runbitselected = getregforvar(r->v, r->offset,
			largeenoughunsignednativetype(r->size),
			0, FORINPUT))->lock = 1;
		
		*r->v->alwaysvolatile = savedvolatilestate;
		
		// The bits in runbitselected that will receive
		// the bits values computed in rtmp are set to null.
		andi(runbitselected, runbitselected, ~r->bitselect);
		
		// Finally I "or" the original register with
		// whatever was computed in rtmp to do the merging.
		or(runbitselected, runbitselected, rtmp);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		runbitselected->lock = 0;
		rtmp->lock = 0;
		
		setregtothetop(rtmp);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			// The lyricalreg pointed by rtmp is not allocated
			// to a lyricalvariable but since I am done using it,
			// I should also produce a comment about it having been
			// discarded to complement the allocation comment that
			// was generated when it was allocated.
			comment(stringfmt("reg %%%d discarded", rtmp->id));
		}
		
		// getregforvar() used to obtain the register
		// pointed by runbitselected has already called
		// setregtothebottom() so there is no need to
		// do that here again.
	}
	
	// There can only be one dirty register for
	// a variable bitselected or non-bitselected spanning
	// a specific region of stack or global variable
	// region, with no other register allocated to
	// a variable overlapping that region of memory.
	// On the other hand I can have more than one clean
	// register bitselected or non-bitselected spanning a specific
	// region of stack or global variable region, with
	// many other non-dirty register allocated to
	// variables overlapping that same region of memory.
	// 
	// I assume that the register is effectively dirty,
	// and as I wrote above there should not be any
	// variable allocated to variables which overlap
	// the region used by the variable associated with
	// the register pointed by r.
	// 
	// - If the register is bitselected, I load its main value
	// from memory(Its main variable will always be in memory
	// and not in a register while it is using a register,
	// as I mentioned above about dirty registers and
	// clean registers), do merging, flush the register
	// containing the result of the merging and mark
	// the bitselected register as clean. At this point I would
	// have the register allocated for the bitselected variable
	// and the register allocated for the main variable both clean.
	// 
	// - If the register is not bitselected I simply flush
	// the register to memory and mark it as clean.
	
	// This variable will point to the register to flush.
	// In fact if I have a bitselected register, the register
	// to flush will be the unbitselected register obtained using
	// mergebitselectedregwithunbitselectedreg(). If there is no bitselect
	// I simply flush the unbitselected register pointed by r.
	lyricalreg* regtoflush;
	
	uint savedrlock = r->lock;
	
	// I lock the register to prevent
	// another call to allocreg() from using it.
	// I also lock it, otherwise it could be lost
	// when insureenoughunusedregisters() is called
	// while creating a new lyricalinstruction.
	r->lock = 1;
	
	// Here the register to flush is set to clean early
	// before flushing it because if the register to flush
	// is bitselected, I will call mergebitselectedregwithunbitselectedreg()
	// below and that function will call getregforvar()
	// to allocate a register for the main variable; but
	// in doing so, if I do not set the register to flush
	// to clean, getregforvar() will try flushing the dirty register
	// that I am still trying to flush because it is overlapping
	// the variable containing the main value that I am trying
	// to obtain. I will end up with an endless calling of
	// flushreg() which will result in allocreg() throwing
	// an error because I will not have anymore unlocked register.
	// To prevent that from happening, I set to clean
	// the register pointed by r.
	r->dirty = 0;
	
	// If the register to flush was a bitselected register,
	// I simply merge it to the original register.
	if (r->bitselect) {
		
		mergebitselectedregwithunbitselectedreg();
		
		regtoflush = runbitselected;
		
		// Locking the register will insure that
		// it do not get used while trying to flush
		// the same register.
		regtoflush->lock = 1;
		
	} else regtoflush = r;
	
	// I can now flush the register to memory.
	// The algorithm used here is similar to
	// the algorithm used in generateloadinstr()
	// except that I am writting the register
	// value to memory.
	
	// The first part of the algoritm is to determine
	// the address of the region where the variable
	// is located.
	
	// Will point to the register which will hold
	// the address of the region containing
	// the variable to flush.
	lyricalreg* regptrtovarregion;
	
	// This variable will be used
	// to save the value returned
	// from varfunclevel().
	uint funclevel;
	
	// Pseudo offset which is used along
	// the actual offset within the variable,
	// when determining the correct aligned
	// memory access instruction that will
	// not caused a misaligned memory access.
	// In fact the actual offset within
	// the variable assume that the base address
	// of the variable is aligned to sizeofgpr.
	// The pseudo offset allow to take into account
	// that the base address of the variable
	// may not be aligned to sizeofgpr,
	// and can only be computed when
	// the base address is a known constant.
	uint pseudooffset = 0;
	
	lyricalvariable* v = regtoflush->v;
	
	if (regtoflush->returnaddr) {
		// If I get here if the register to flush
		// hold the return address.
		regptrtovarregion = &rstack;
		
	} else if (regtoflush->funclevel) {
		// If I get here if the register to flush
		// hold a stackframe address to cache within
		// the stackframe of currentfunc.
		regptrtovarregion = &rstack;
		
	} else if (v->predeclared) {
		// I call getregforvar() in order to re-use any
		// register that already has the address value
		// of the predeclared variable.
		regptrtovarregion = getregforvar(getvarnumber((uint)v->predeclared->varaddr, voidptrstr),
			0, voidptrstr, 0, FORINPUT);
		
		pseudooffset = (uint)v->predeclared->varaddr % sizeofgpr;
		
	} else if (v == &thisvar) {
		// If I get here the variable is thisvar and
		// I need to set in regptrtovarregion a register
		// holding the address of retvar.
		regptrtovarregion = getregptrtothis();
		
	} else if (v == &returnvar) {
		// If I get here the variable is returnvar and
		// I need to set in regptrtovarregion a register
		// holding the address of retvar.
		regptrtovarregion = getregptrtoretvar();
		
	} else if (v->name.ptr[1] == '*') {
		// If I get here, the variable is a dereference
		// variable which do not take memory space neither
		// in the stack nor in the global variable region.
		// I call getregforvar() to load into regptrtovarregion
		// the address of the dereference variable
		// which is the variable for which the name
		// is between "(*(cast)" and ")" .
		lyricalvariable* vaddr = getvaraddr(v, DONOTMAKEALWAYSVOLATILE);
		regptrtovarregion = getregforvar(vaddr, 0, voidptrstr, 0, FORINPUT);
		
		pseudooffset = (vaddr->isnumber ? (vaddr->numbervalue % sizeofgpr) : 0);
		
	} else if (v->funcowner == rootfunc) {
		// If I get here the variable is a global variable
		// and I need to set in regptrtovarregion a register
		// pointing to the region containing global variables.
		regptrtovarregion = getregptrtoglobalregion();
		
	} else if (funclevel = varfunclevel(v)) {
		// If the variable do not belong to the root
		// function (in other words if the variable is not
		// a global variable) and is not in the current function,
		// I generate instructions to compute the address of
		// the stackframe to which it belong.
		regptrtovarregion = getregptrtofuncstackframe(funclevel);
		
	} else {
		// If I get here the variable is within
		// the current function and its stackframe
		// address is in the stack register.
		regptrtovarregion = &rstack;
	}
	
	// I lock the allocated register to prevent
	// another call to allocreg() from using it.
	// I also lock it, otherwise it could be lost
	// when insureenoughunusedregisters() is called
	// while creating a new lyricalinstruction.
	regptrtovarregion->lock = 1;
	
	
	// The second part of the algorithm
	// is to generate the immediate value for
	// the offset within the region where
	// the variable is located.
	
	lyricalimmval* generateimmvalue (uint flushoffset) {
		
		lyricalimmval* immvalue = mmallocz(sizeof(lyricalimmval));
		immvalue->type = LYRICALIMMVALUE;
		
		if (regtoflush->returnaddr) {
			// I get here if the register to flush
			// hold the return address.
			
			// When I get here, currentfunc->firstpass->stackframeholder
			// is always set because the register used to hold
			// the return address is only used by functions
			// which make use of a stackframe holder.
			
			immvalue->n = sizeofgpr;
			
			lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = currentfunc->firstpass->stackframeholder->secondpass;
			
			immvalue->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
			imm->sharedregion = currentfunc->firstpass->sharedregiontouse;
			
			immvalue->next->next = imm;
			
		} else if (regtoflush->funclevel) {
			// I get here if the register to flush hold
			// a stackframe address to cache within
			// the stackframe of currentfunc.
			
			// I compute the immediate value which is the offset
			// within the space reserved in the stack of currentfunc
			// for the caching of stackframe pointers.
			
			uint funclevel = regtoflush->funclevel;
			
			// currentfunc->firstpass->cachedstackframes will
			// certainly be non-null when getting here.
			lyricalcachedstackframe* stackframe = currentfunc->firstpass->cachedstackframes;
			
			// This variable is initialized to 1 to take
			// into account the space used at the top of a stackframe
			// to write the offset from the start of the stackframe
			// to the field holding the return address.
			uint i = 1;
			
			// The funclevel value will certainly be found
			// in the linkedlist of lyricalcachedstackframe pointed
			// by currentfunc->firstpass->cachedstackframes; since a register
			// for a stackframe pointer is set dirty only if
			// a lyricalcachedstackframe was created for it.
			while (stackframe->level != funclevel) {
				
				++i;
				
				stackframe = stackframe->next;
			}
			
			immvalue->n = i*sizeofgpr;
			
		} else if (v == &thisvar) {
			// If I get here, the variable is thisvar.
			// I increment the immediate value by
			// the offset within thisvar saved in
			// the register structure.
			immvalue->n = regtoflush->offset;
			
		} else if (v == &returnvar) {
			// If I get here, the variable is returnvar.
			// I increment the immediate value by
			// the offset within returnvar saved in
			// the register structure.
			immvalue->n = regtoflush->offset;
			
		} else if (v->name.ptr[1] == '*') {
			// If I get here, the variable is
			// a dereference variable.
			immvalue->n = regtoflush->offset;
			
		} else {
			
			lyricalfunction* f = v->funcowner;
			
			if (f != rootfunc) {
				// I get here only for variables which
				// reside in the stack.
				
				if (v->argorlocal == &f->varg) {
					// I get here if the variable is a function argument.
					
					if (f->firstpass->stackframeholder) {
						// I get here if f make use
						// of a stackframe holder.
						
						immvalue->n = 3*sizeofgpr;
						
						if (!stringiseq2(f->type, "void"))
							immvalue->n += sizeofgpr;
						
						lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
						imm->f = f->firstpass->stackframeholder->secondpass;
						
						immvalue->next = imm;
						
						imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
						imm->sharedregion = f->firstpass->sharedregiontouse;
						
						immvalue->next->next = imm;
						
					} else {
						// I get here if f is a stackframe holder.
						
						immvalue->n = 7*sizeofgpr;
						
						lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
						imm->f = f;
						
						immvalue->next = imm;
						
						imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSHAREDREGIONSIZE;
						imm->f = f;
						
						immvalue->next->next = imm;
						
						imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMLOCALVARSSIZE;
						imm->f = f;
						
						immvalue->next->next->next = imm;
					}
					
				} else if (v->argorlocal == &f->vlocal) {
					// I get here if the variable is a local variable.
					
					if (f->firstpass->stackframeholder) {
						// I get here if f make use
						// of a stackframe holder.
						
						immvalue->n = 3*sizeofgpr;
						
						if (!stringiseq2(f->type, "void"))
							immvalue->n += sizeofgpr;
						
						// If the function for which the lyricalfunction is pointed
						// by f is variadic, MAXARGUSAGE is taken as the amount
						// of memory that will be used by its arguments.
						if (f->isvariadic) immvalue->n += MAXARGUSAGE;
						else {
							// When non-null the field f->varg point to
							// the lyricalvariable for the last created argument.
							lyricalvariable* varg = f->varg;
							
							// I increment immvalue->n by the size
							// occupied by all its arguments.
							if (varg) immvalue->n += (varg->offset + varg->size);
						}
						
						lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
						imm->f = f->firstpass->stackframeholder->secondpass;
						
						immvalue->next = imm;
						
						imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
						imm->sharedregion = f->firstpass->sharedregiontouse;
						
						immvalue->next->next = imm;
						
					} else {
						// I get here if f is a stackframe holder.
						
						immvalue->n = sizeofgpr;
						
						lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
						imm->f = f;
						
						immvalue->next = imm;
						
						imm = mmallocz(sizeof(lyricalimmval));
						imm->type = LYRICALIMMSHAREDREGIONSIZE;
						imm->f = f;
						
						immvalue->next->next = imm;
					}
				}
			}
			
			// I increment the immediate value by
			// the total of the offset of the variable
			// associated with the register and the offset
			// within the variable saved in the lyricalreg struct.
			// If the register is allocated for a predeclared
			// variable, v->offset will always be 0.
			immvalue->n += (v->offset + regtoflush->offset);
		}
		
		// I increment the immediate value
		// by the offset given as argument
		// to this function.
		immvalue->n += flushoffset;
		
		// If the immediate value is null,
		// set immvalue to null after freeing
		// the previously allocated lyricalimmval.
		if (!(immvalue->next || immvalue->n)) {
			mmrefdown(immvalue);
			immvalue = 0;
		}
		
		return immvalue;
	}
	
	
	// The third part of the algorithm is
	// to generate instructions to write
	// to memory the register content.
	
	lyricalinstruction* i;
	
	lyricalimmval* immvalue = generateimmvalue(0);
	
	// Determine the correct and optimized
	// aligned flush instructions to use.
	
	uint offset = (immvalue ? immvalue->n : 0);
	
	uint flushsize = sizeofgpr;
	
	while ((offset+pseudooffset) % flushsize)
		flushsize /= 2;
	
	uint regtoflushsize = regtoflush->size;
	
	uint nbrofflushs;
	
	if (flushsize > regtoflushsize) {
		
		flushsize = regtoflushsize;
		
		nbrofflushs = 1;
		
	} else nbrofflushs = regtoflushsize/flushsize;
	// The value of regtoflushsize and flushsize
	// will always be set to a powerof2.
	
	lyricalreg* regtoflush2 = regtoflush;
	
	uint count = 0;
	
	// Issue as many flush instructions as needed.
	while (1) {
		
		if (flushsize == 1) {
			
			if (immvalue) {
				// The following instruction is created:
				// st8 regtoflush2, regptrtovarregion, immvalue;
				i = newinstruction(currentfunc, LYRICALOPST8);
				
				i->imm = immvalue;
				
			} else {
				// The following instruction is created:
				// st8r regtoflush2, regptrtovarregion;
				i = newinstruction(currentfunc, LYRICALOPST8R);
			}
			
		} else if (flushsize == 2) {
			
			if (immvalue) {
				// The following instruction is created:
				// st16 regtoflush2, regptrtovarregion, immvalue;
				i = newinstruction(currentfunc, LYRICALOPST16);
				
				i->imm = immvalue;
				
			} else {
				// The following instruction is created:
				// st16r regtoflush2, regptrtovarregion;
				i = newinstruction(currentfunc, LYRICALOPST16R);
			}
			
		} else if (flushsize == 4) {
			
			if (immvalue) {
				// The following instruction is created:
				// st32 regtoflush2, regptrtovarregion, immvalue;
				i = newinstruction(currentfunc, LYRICALOPST32);
				
				i->imm = immvalue;
				
			} else {
				// The following instruction is created:
				// st32r regtoflush2, regptrtovarregion;
				i = newinstruction(currentfunc, LYRICALOPST32R);
			}
			
		} else if (flushsize == 8) {
			
			if (immvalue) {
				// The following instruction is created:
				// st64 regtoflush2, regptrtovarregion, immvalue;
				i = newinstruction(currentfunc, LYRICALOPST64);
				
				i->imm = immvalue;
				
			} else {
				// The following instruction is created:
				// st64r regtoflush2, regptrtovarregion;
				i = newinstruction(currentfunc, LYRICALOPST64R);
			}
			
		} else throwerror(stringfmt("internal error: %s: flushsize invalid", __FUNCTION__).ptr);
		
		i->r1 = regtoflush2->id;
		i->r2 = regptrtovarregion->id;
		
		if (++count < nbrofflushs) {
			
			if (regtoflush2 == regtoflush) {
				// I lock the allocated register, otherwise
				// it will be seen as an unused register
				// when generating lyricalinstruction,
				// since it is not assigned to anything.
				(regtoflush2 = allocreg(CRITICALREG))->lock = 1;
			}
			
			uint flushoffset = count*flushsize;
			
			srli(regtoflush2, regtoflush, 8*flushoffset);
			
			immvalue = generateimmvalue(flushoffset);
			
		} else break;
	}
	
	if (regtoflush2 != regtoflush) {
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		regtoflush2->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringfmt("reg %%%d discarded", regtoflush2->id));
		}
	}
	
	// Unlock lyricalreg.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	regptrtovarregion->lock = 0;
	
	// This is useless if runbitselected
	// is not used, meaning that regtoflush is
	// set to r; but if runbitselected is used,
	// it will effectively unlock it because
	// regtoflush will be set to runbitselected.
	regtoflush->lock = 0;
	
	// Restore the register lock status.
	r->lock = savedrlock;
	
	// The register pointed by regtoflush
	// has already been set to clean.
	
	// If the register to flush was allocated for
	// a predeclared variable, I generate instructions
	// to call the callback function associated with
	// the predeclared variable if there was any set.
	if (v && v->predeclared && v->predeclared->callback) {
		// It would have been simpler to call callfunctionnow()
		// from here, but callfunctionnow() is declared within
		// evaluateexpression() and cannot be called from here.
		// Furthermore propagatevarschangedbyfunc() which is used
		// within callfunctionnow() should not be called as
		// it will propagate the variables changed by all functions
		// for which the address was obtained and that is not what
		// I want when calling the callback function associated with
		// the predeclared variable.
		
		// The code within this block is similar to what
		// is used in generatefunctioncall().
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: calling callback function of predeclared variable"));
		}
		
		lyricalreg* r1; lyricalreg* r2;
		
		lyricalvariable* varptrtofunc = getvarnumber((uint)v->predeclared->callback, voidfncstr);
		
		// I allocate and lock a register that will
		// be used to hold the address where the stackframe
		// of the function to call is located.
		// I lock the register because I will be calling functions
		// which make use of allocreg() and if the register
		// is not locked, allocreg() could use it while
		// trying to allocate a new register.
		// I also lock it, otherwise it will
		// be seen as an unused register
		// when generating lyricalinstruction,
		// since it is not assigned to anything.
		(r1 = allocreg(CRITICALREG))->lock = 1;
		
		r1->size = sizeofgpr;
		
		// I generate instructions which check whether
		// there is enough stack left; and if not allocate
		// a new stackpage.
		// 
		// The following instructions are generated:
		// 
		// -------------------------
		// #I compute the size left in the stack page.
		// andi r1, %0, PAGESIZE-1;
		// 
		// #I check whether the amount of stack left
		// #is large enough to hold the stackframe
		// #of the function to call.
		// sltui r1, r1, stackneededvalue;
		// 
		// #If the result of the above test is false,
		// #then there is no need to allocate
		// #a new stack page.
		// jz r1, enoughstackleft;
		// 
		// #I allocate a new page.
		// stackpagealloc r1;
		// 
		// #I set r1 to the bottom of the stack page
		// #where the current value of the stack
		// #pointer register will be written.
		// addi r1, r1, PAGESIZE-sizeofgpr;
		// 
		// #I store the current value of the stack
		// #pointer register; that allow for keeping
		// #all stack page allocated in a linkedlist.
		// st %0, r1, 0;
		// 
		// #I generate the instruction that compute
		// #the stack pointer for the function to call.
		// #Remember that the stack grow toward
		// #lower addresses.
		// addi r1, r1, stackframeadjustvalue;
		// 
		// j doneallocatingstack;
		// 
		// enoughstackleft:
		// 
		// #I generate the instruction that compute
		// #the stack pointer for the function to call.
		// #Remember that the stack grow toward
		// #lower addresses.
		// addi r1, %0, stackframeadjustvalue;
		// 
		// doneallocatingstack:
		// -------------------------
		// 
		// Note that before the jump instruction and
		// the label used above, it is not necessary to flush and
		// discard registers, because the branching instruction
		// and label are not used to branch across code written
		// by the programmer.
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: checking for enough stack"));
		}
		
		andi(r1, &rstack, PAGESIZE-1);
		
		// The stackframe usage of a function called
		// through a pointer to function cannot be determined
		// at compiled time because its locals usage cannot
		// be estimated, hence the reason why I use MAXSTACKUSAGE
		// as the stackframe usage of the function
		// that I am calling.
		sltui(r1, r1, MAXSTACKUSAGE);
		lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		// To the stack needed amount,
		// I add a value which represent
		// the additional stack space needed
		// by LYRICALOPSTACKPAGEALLOC when
		// allocating a new stack page.
		imm->n = compileargstackpageallocprovision;
		imm->next = currentfunc->i->imm;
		currentfunc->i->imm = imm;
		
		// I create the name of the label needed.
		string labelnameforenoughstackleft = stringfmt("%d", newgenericlabelid());
		
		jz(r1, labelnameforenoughstackleft);
		
		stackpagealloc(r1);
		
		addi(r1, r1, PAGESIZE-sizeofgpr);
		
		str(&rstack, r1);
		
		// Remember that the stack grow toward
		// lower addresses; 5*sizeofgpr account for
		// the space used by the address of the previous
		// function stackframe, the address of the parent
		// function stackframe, the stackframe-id,
		// the "this" pointer and the address
		// of the return variable.
		addi(r1, r1, -((5*sizeofgpr)+(compileargfunctioncallargsguardspace*sizeofgpr)));
		
		// I create the name of the label needed.
		string labelnamefordoneallocatingstack = stringfmt("%d", newgenericlabelid());
		
		j(labelnamefordoneallocatingstack);
		
		newlabel(labelnameforenoughstackleft);
		
		// Remember that the stack grow toward
		// lower addresses; 5*sizeofgpr account for
		// the space used by the address of the previous
		// function stackframe, the address of the parent
		// function stackframe, the stackframe-id,
		// the "this" pointer and the address
		// of the return variable.
		addi(r1, &rstack, -((5*sizeofgpr)+(compileargfunctioncallargsguardspace*sizeofgpr)));
		
		newlabel(labelnamefordoneallocatingstack);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to previous stackframe"));
		}
		
		// Here I write the field which hold the pointer
		// to the previous stackframe.
		// the stackframe address to write is
		// the current value of the stack pointer register.
		str(&rstack, r1);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: no need to write pointer to stackframe of function parent to function being called"));
		}
		
		// When calling a function through a pointer to function,
		// there is no need to write the field for the address
		// to the parent function stackframe; in fact a function
		// which was called through a variable pointer to function
		// retrieve its parent function stackframe scanning through
		// previous stackframes.
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write null in the fields used for \"this\" variable pointer and result variable pointer"));
		}
		
		// I generate instructions to write null
		// to the field used for the address of the return variable
		// and the field used for the "this" pointer.
		// Callback functions associated with prediclared
		// variables do not return a value are called
		// through a constant address.
		
		// I lock the register, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		(r2 = allocreg(CRITICALREG))->lock = 1;
		
		// I set the register value null;
		// which is then used to write null.
		li(r2, 0);
		
		// I write the null value of the register
		// in the fields used for the address of
		// the return variable and the "this" pointer.
		st(r2, r1, 4*sizeofgpr);
		st(r2, r1, 3*sizeofgpr);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r2->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			// The lyricalreg pointed by r2 is not allocated
			// to a lyricalvariable but since I am done using it,
			// I should also produce a comment about it having been
			// discarded to complement the allocation comment that
			// was generated when it was allocated.
			comment(stringfmt("reg %%%d discarded", r2->id));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write stackframe-id"));
		}
		
		// I generate instructions to write
		// the stackframe-id of the function to call.
		// The stackframe-id value is the starting
		// address of the function to call.
		
		// I retrieve the address of the function
		// that is being called through a pointer and lock it
		// since flushanddiscardallreg() will be called and
		// it could make use of allocreg() which could use
		// the register while allocating a new register.
		// The lyricalvariable varptrtofunc certainly has
		// a pointer to function cast or type, but here
		// for faster result I use "void*".
		// An lyricalvariable which has a pointer to function type
		// can never be bitselected, hence there is no need to consider
		// the bitselect of the lyricalvariable varptrtofunc.
		(r2 = getregforvar(varptrtofunc, 0, voidptrstr, 0, FORINPUT))->lock = 1;
		
		// The value of the register obtained above is written.
		st(r2, r1, 2*sizeofgpr);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		// Since I am going to do a branching,
		// I flush and discard all registers.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// This part can only come after flushanddiscardallreg(),
		// since flushing still need the address in the stack
		// pointer register unchanged.
		// So here, I set the stack pointer
		// register to the value of r1.
		cpy(&rstack, r1);
		
		jpushr(r2);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r2->lock = 0;
		
		// Once the function return, I restore
		// the stack pointer register. Remember that
		// the stack grow toward lower addresses; so
		// to restore it, I need to increment it.
		// 5*sizeofgpr account for the space used by
		// the address of the previous function stackframe,
		// the address of the parent function stackframe,
		// the stackframe-id, the "this" pointer
		// and the address of the return variable.
		addi(&rstack, &rstack, (5*sizeofgpr)+(compileargfunctioncallargsguardspace*sizeofgpr));
		
		// I generate instructions which check whether
		// the stack page pointed by the stack pointer
		// register is an excess page which can be freed.
		// 
		// The following instructions are generated:
		// 
		// -------------------------
		// #I compute the size left in the stack page.
		// andi r1, %0, PAGESIZE-1;
		// 
		// #I check whether the size left in
		// #the stack page is (PAGESIZE-sizeofgpr).
		// #If true, it mean that the stack pointer
		// #register is at the bottom of the stack page
		// #where the address to the previous stack page
		// #is written.
		// seqi r1, r1, PAGESIZE-sizeofgpr;
		// 
		// #If the result of the above test is false,
		// #then there is no page to free.
		// jz r1, noneedtofreestack;
		// 
		// #I save the address, which
		// #is within the page to free.
		// cpy r1, %0;
		// 
		// #I load the pointer to the stack page
		// #previous to the one that will be freed.
		// #That pointer is written at the bottom
		// #of the page that will be freed.
		// #Note that when this instruction is executed
		// #the stack pointer register is at the bottom
		// #of the stack page which will be freed.
		// ld %0, %0, 0;
		// 
		// #I free the page.
		// stackpagefree r1;
		// 
		// noneedtofreestack:
		// -------------------------
		// 
		// Note that before the jump instruction and
		// the label used above, it is not necessary to flush and
		// discard registers, because the branching instruction
		// and label are not used to branch across code written
		// by the programmer; furthermore, when returning
		// from a call, there is no registers to flush
		// and discard.
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: checking for excess stack"));
		}
		
		andi(r1, &rstack, PAGESIZE-1);
		
		seqi(r1, r1, PAGESIZE-sizeofgpr);
		
		// I create the name of the label needed.
		string labelnamefornoneedtofreestack = stringfmt("%d", newgenericlabelid());
		
		jz(r1, labelnamefornoneedtofreestack);
		
		cpy(r1, &rstack);
		
		ldr(&rstack, &rstack);
		
		stackpagefree(r1);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r1->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			// The lyricalreg pointed by r1 is not allocated
			// to a lyricalvariable but since I am done using it,
			// I should also produce a comment about it having been
			// discarded to complement the allocation comment that
			// was generated when it was allocated.
			comment(stringfmt("reg %%%d discarded", r1->id));
		}
		
		newlabel(labelnamefornoneedtofreestack);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}


// Register allocation must be unique between labels which
// are destinations of branching instructions.
// In other words, right before a label, all registers must be
// flushed and discarded.
// In fact a label is a location to which execution from at leat
// 2 different locations can branch to, and there is no way
// to conclude which register should be seen as used or unused
// when branching to the instructions coming after the label.
// Hence it is expected that all registers be unused at the starting
// of the code of a function or right before a label.
// So this function is called right before a label is created or
// right before a function return in order to process
// the registers that should be flushed.
// In addition, this function is also called right before
// a conditional or non-conditional branching instruction;
// in fact before a conditional branching, all registers must be
// flushed but should not be discarded, because the flushing and
// discarding instructions that are before the label to which
// the branching will be done will not have any instruction to flush
// the registers dirtied before the conditional branching instruction;
// and those registers are not discarded so that they can be
// reused right after the conditional branching instruction
// if no branching occured.
// For a non-conditional branching, all registers can be flushed
// and discarding since the instructions coming right after
// the non-conditional branching instruction will not get used.
void flushanddiscardallreg (flushanddiscardallregflag flag) {
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		
		string s;
		
		if (flag == DONOTDISCARD) s = stringduplicate2("begin: flushing all registers");
		else if (flag == DONOTFLUSHREGFORLOCALS) {
			
			if (currentfunc != rootfunc) s = stringduplicate2("begin: flushing and discarding all registers; do not flush registers for local variables, stackframe pointers and return address");
			else s = stringduplicate2("begin: flushing and discarding all registers; do not flush registers for global variables");
			
		} else if (flag == DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR) {
			
			s = stringduplicate2("begin: flushing and discarding all registers; do not flush registers for local variables, stackframe pointers and return address");
			
			// A return address register is used only with
			// functions which make use of a stackframe holder.
			if (currentfunc->firstpass->stackframeholder) stringappend2(&s, "; keep register for return address");
			
		} else if (flag == DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL) s = stringduplicate2("begin: flushing and discarding all registers; do not flush registers for local variables, stackframe pointers and return address; keep registers for stackframe pointers");
		else s = stringduplicate2("begin: flushing and discarding all registers");
		
		comment(s);
	}
	
	// I do not look at the field lock of registers although
	// while going through the registers there could be locked
	// registers; some of those locked registers can be registers
	// reserved for an asm block and there are not associated
	// with a variable. If the locked register was associated
	// to a variable I can still safely flush and discard
	// the register; the lock on it simply insure that allocreg()
	// do not make that register available to any other routines.
	
	// When flushing the registers, I first start by flushing
	// registers that are assigned to variables, registers
	// which hold stackframe pointers, and the register which
	// hold the return address; then I discard registers that
	// are used for funclevel, globalregionaddr, stringregionaddr,
	// thisaddr and retvaraddr; that allow for re-using those registers
	// while I am flushing registers that are assigned to variables,
	// registers which hold stackframe pointers, and the register
	// which hold the return address.
	
	lyricalreg* r = currentfunc->gpr;
	
	do {
		// Note that I cannot find registers associated with
		// a dereferenced variable because dereferenced variables
		// are always volatile to prevent aliasing; hence
		// their value is never cached in registers.
		if (r->v) {
			// I get here if the register was assigned to
			// a variable; I flush it (if it was dirty) and
			// discard it. If the register being flushed is bitselected,
			// flushreg() will allocate another register for the main
			// value of the bitselected register and because registers
			// always get moved to the bottom of the linkedlist,
			// those registers that have been allocated during
			// the flushing of another register will be moved to
			// the bottom of the linkedlist and will also get discarded,
			// since this loop will not stop until I have reached
			// the bottom of the linkedlist.
			// If the register is associated with thisvar
			// or returnvar, it will always be flushed.
			if (r->dirty) {
				
				if (r->v != &thisvar && r->v != &returnvar && (
					flag == DONOTFLUSHREGFORLOCALS ||
					flag == DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR ||
					flag == DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL)) {
					
					if (r->v->funcowner != currentfunc) flushreg(r);
					
				} else flushreg(r);
			}
			
			if (flag != DONOTDISCARD) {
				
				r->v = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					
					comment(stringfmt("reg %%%d discarded", r->id));
				}
			}
			
		// If the register was used to store the return address
		// or used to store a stackframe pointer, I flush it if
		// it was dirty and the argument flag was not set
		// to a value preventing it from being flushed.
		} else if ((r->returnaddr || r->funclevel) && r->dirty &&
			flag != DONOTFLUSHREGFORLOCALS &&
			flag != DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR &&
			flag != DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL) flushreg(r);
		
	} while ((r = r->next) != currentfunc->gpr);
	
	// When I get here, r == currentfunc->gpr;
	
	if (flag != DONOTDISCARD) {
		
		do {
			uint isdiscarded = 1;
			
			// If the register was used to hold the return address or
			// a parent function stackframe address or the address of
			// the global variable region or the address of the string region
			// or the address of retvar, I set the field returnaddr or
			// funclevel or globalregionaddr or stringregionaddr or
			// thisaddr or retvaraddr to zero accordingly.
			// Remember that only one of the fields returnaddr, funclevel,
			// globalregionaddr, stringregionaddr, thisaddr, retvaraddr
			// and v can be non-null at a time.
			if (r->returnaddr) {
				// If flag == DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR,
				// the register used to hold the return address is not discarded.
				if (flag != DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR) r->returnaddr = 0;
				else isdiscarded = 0;
				
			} else if (r->funclevel) {
				// If flag == DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL,
				// registers used to hold stackframe pointers are not discarded.
				if (flag != DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL) r->funclevel = 0;
				else isdiscarded = 0;
				
			} else if (r->globalregionaddr) r->globalregionaddr = 0;
			else if (r->stringregionaddr) r->stringregionaddr = 0;
			else if (r->thisaddr) r->thisaddr = 0;
			else if (r->retvaraddr) r->retvaraddr = 0;
			else continue;
			
			// setregtothetop() should not be used on the discarded
			// register because I am going through all registers.
			
			if (isdiscarded && compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("reg %%%d discarded", r->id));
			}
			
		} while ((r = r->next) != currentfunc->gpr);
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}


// This function generate instructions to adjust
// the stack pointer register to the address of
// the stackframe specified by the argument funclevel.
// This function will search among all registers if
// there is a register already containing the stackframe
// address of the parent function of the current function at
// the level given as argument.
// If the register could be found, the stack pointer register
// is set with the value of that register found.
// If no register could be found, instructions are generated
// to load into the stack pointer register, the stackframe address
// of the parent function of the current function at
// the level given as argument.
// This function should not be called with a funclevel
// argument value of 0, because any register will be selected
// during the search, since registers allocated to variables
// for example have their field funclevel set to zero.
// This function also generate the instructions to free
// any excess stackpage.
void setregstackptrtofuncstackframe (uint funclevel) {
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringfmt("begin: restoring stackframe #%d", funclevel));
	}
	
	lyricalreg* r = currentfunc->gpr;
	
	// This function is always called after flushanddiscardallreg()
	// has been called with its argument flag set to
	// DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL;
	// and since I will be discarding all remaining registers
	// which should only be registers holding stackframe addresses,
	// it is no longer needed to flush any dirty register holding
	// a stackframe address; so here I set all registers clean
	// to prevent any register flushing from occuring.
	do r->dirty = 0; while ((r = r->next) != currentfunc->gpr);
	
	// When I get here, r == currentfunc->gpr.
	
	// Because only functions that are stackframe holders
	// have a cache for stackframe pointers, if currentfunc
	// is not a stackframe holder, I adjust funclevel so as to
	// compute the stackframe pointer as if setregstackptrtofuncstackframe()
	// had been called while parsing the body of the function
	// that is the stackframe holder of currentfunc.
	// If the function at the level in funclevel is not
	// a stackframe holder, I adjust funclevel so as the stackframe
	// pointer register for its stackframe holder is returned.
	// Hence the field funclevel of the lyricalreg returned
	// may not be the same as the initial value of
	// the argument funclevel.
	
	uint level;
	
	lyricalfunction* f;
	
	lyricalfunction* stackframeholder = currentfunc->firstpass->stackframeholder;
	
	// I check whether currentfunc is a stackframe holder.
	if (stackframeholder) {
		// I get here if currentfunc is not a stackframe holder.
		
		// I get the secondpass equivalent lyricalfunction
		// of the stackframe holder.
		stackframeholder = stackframeholder->secondpass;
		
		level = 1;
		
		f = currentfunc->parent;
		
		while (f != stackframeholder) {
			++level;
			f = f->parent;
		}
		
		// When the function at the level in funclevel is not
		// a stackframe holder, the stackframe pointer register
		// for its stackframe holder is used, hence if
		// funclevel is less than or equal to the level of
		// the stackframe holder of currentfunc then the stack
		// pointer register %0 already hold the stackframe address
		// needed; so no additional computation need to be done.
		if (funclevel <= level) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringduplicate2("end: done"));
			}
			
			return;
		}
		
		// I adjust funclevel as to compute the stackframe pointer
		// as if getregptrtofuncstackframe() had been called while
		// parsing the body of the function that is the stackframe
		// holder of currentfunc.
		funclevel -= level;
		
	} else stackframeholder = currentfunc;
	
	// I further adjust funclevel if the function at
	// the level in funclevel is not a stackframe holder;
	// the stackframe pointer for its stackframe holder
	// is what should be retrieved.
	
	level = funclevel -1;
	
	f = stackframeholder->parent;
	
	while (level) {
		--level;
		f = f->parent;
	}
	
	while (f->firstpass->stackframeholder) {
		++funclevel;
		f = f->parent;
	}
	
	// I search through the registers if there is
	// already a register containing the stackframe address
	// of the parent function of the current function at
	// the level given by the argument funclevel; if yes
	// I use it, otherwise I compute it.
	
	// Variable that will point to the lyricalreg used
	// for saving the current value of the stack pointer register.
	// That saved value will later be used to determine
	// whether there is any excess stackpage to free.
	lyricalreg* rstacksaved;
	
	// When a register containing the stackframe address
	// for the value given by the argument funclevel cannot
	// be found, this variable get set to the register
	// containing the address of the closest stackframe;
	// this way I save some computation by not starting
	// from the current stackframe.
	lyricalreg* rr = 0;
	
	while (1) {
		
		if (r->funclevel == funclevel) {
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringfmt("re-using reg %%%d", r->id));
			}
			
			// I get here if I found a register already containing
			// the stackframe address of the parent function of
			// the current function at the level given by the argument
			// funclevel. Before assigning its value to %0, I lock it
			// and generate the instruction that save the current value
			// of the stack pointer register; that saved value will later
			// be used to determine whether there is any excess
			// stackpage to free.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			r->lock = 1;
			
			// Note that the register used to save the current value
			// of the stack pointer register is allocated after the register
			// for the stackframe pointer needed has been found;
			// otherwise, the register for the stackframe needed could
			// be discarded and allocated for the saving of the stack pointer.
			// I lock the allocated register to prevent another call
			// to allocreg() from using it.
			// I also lock it, otherwise it will
			// be seen as an unused register
			// when generating lyricalinstruction,
			// since it is not assigned to anything.
			(rstacksaved = allocreg(CRITICALREG))->lock = 1;
			
			cpy(rstacksaved, &rstack);
			
			cpy(&rstack, r);
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			// Note that I do not need to use setregtothebottom() on
			// the register pointed by r because flushanddiscardallreg()
			// will be called to discard all remaining registers
			// before generating the instructions that check for
			// excess stackpage to free.
			
			break;
			
		} else {
			
			level = r->funclevel;
			
			if (level && level < funclevel) {
				if (!rr || level > rr->funclevel) rr = r;
			}
		}
		
		r = r->next;
		
		if (r == currentfunc->gpr) {
			// If I get here I went through all the registers and
			// I could not find a register containing the stackframe address
			// of the parent function of the current function at the level
			// given by the argument funclevel; so here I compute it.
			
			// I generate the instruction that save the current value of
			// the stack pointer register; that saved value will later
			// be used to determine whether there is any excess
			// stackpage to free.
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			if (rr) rr->lock = 1;
			
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it will
			// be seen as an unused register
			// when generating lyricalinstruction,
			// since it is not assigned to anything.
			(rstacksaved = allocreg(CRITICALREG))->lock = 1;
			
			cpy(rstacksaved, &rstack);
			
			f = stackframeholder;
			
			while (1) {
				
				lyricalcachedstackframe* stackframe = f->firstpass->cachedstackframes;
				
				if (stackframe) {
					// If I get here, I check whether the stackframe
					// pointer for the value of funclevel was cached
					// by the function pointed by f.
					
					// If the stackframe pointer for the value of funclevel
					// could not be found among the cached stackframe pointers
					// of the function pointed by f, this variable contain
					// the level of the closest stackframe as well as
					// the location of its address within the stack of
					// the function pointed by f.
					struct {
						
						uint level;
						uint locofaddrinstack;
						
					} closeststackframe = {
						
						.level = 0,
						.locofaddrinstack = 0,
					};
					
					// This variable is initialized to 1 to take
					// into account the space used at the top of a stackframe
					// to write the offset from the start of the stackframe
					// to the field holding the return address.
					uint i = 1;
					
					while (1) {
						
						if (stackframe->level == funclevel) {
							
							ld(&rstack, &rstack, i*sizeofgpr);
							
							goto stackframepointerloaded;
							
						} else {
							
							level = stackframe->level;
							
							if (level < funclevel) {
								if (!closeststackframe.level ||
									level > closeststackframe.level) {
									
									closeststackframe.level = level;
									closeststackframe.locofaddrinstack = i;
								}
							}
						}
						
						if (stackframe = stackframe->next) ++i;
						else break;
					}
					
					// I get here if I could not find the stackframe
					// pointer for the value of funclevel among
					// the cached stackframe pointers of the function
					// pointed by f.
					
					// If closeststackframe.level was set, I use the address
					// of that stackframe as the starting point of the computation
					// to find the stackframe address of the parent function
					// at the level given by the variable funclevel;
					// if closeststackframe.level was not set, I start the computation
					// from the stackframe of the function pointed f for which
					// the stackframe address is in the stack pointer register.
					if (closeststackframe.level) {
						// If rr is set, then it mean that I am in the first
						// iteration of this loop, and I should use the value
						// of the register that it point to as the starting
						// point of the computation to find the stackframe address
						// of the parent function at the level given by
						// the variable funclevel, unless the stackframe level
						// for which it is holding the address is less than
						// closeststackframe.level .
						if (rr) {
							if (rr->funclevel >= closeststackframe.level) goto usereginrr;
							else rr = 0;
						}
						
						ld(&rstack, &rstack, closeststackframe.locofaddrinstack*sizeofgpr);
						
						// I decrement funclevel by the amount of
						// iterations that do not need to be done
						// to retrieve the stackframe address.
						funclevel -= closeststackframe.level;
						
						// Within this loop I set f to the function
						// for which the stackframe address was loaded
						// in the stack pointer register.
						do f = f->parent;
						while (--closeststackframe.level);
						
					// If rr is set, then it mean that I am in the first
					// iteration of this loop, and I should use the value
					// of the register that it point to as the starting
					// point of the computation to find the stackframe address
					// of the parent function at the level given by
					// the variable funclevel.
					} else if (rr) goto usereginrr;
					
				// If rr was set, I use the value of its register as
				// the starting point of the computation to find the stackframe
				// address of the parent function at the level given by
				// the argument funclevel.
				// If rr was not set, I start the computation from
				// the current stackframe.
				} else if (rr) {
					
					usereginrr:
					
					// Note that I get here only in the first
					// iteration of this loop.
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						comment(stringfmt("re-using reg %%%d", rr->id));
					}
					
					// setregtothebottom() should not be called
					// on the lyricalreg pointed by rr.
					
					cpy(&rstack, rr);
					
					level = rr->funclevel;
					
					// I decrement funclevel by the amount of
					// iterations that do not need to be done
					// to retrieve the stackframe address.
					funclevel -= level;
					
					// Within this loop I set f to the function
					// for which the stackframe address is held
					// by the lyricalreg pointed by rr.
					do f = f->parent; while (--level);
					
					// I set rr to null so that this block
					// get executed only in the first iteration.
					rr = 0;
				}
				
				lyricalinstruction* i;
				lyricalimmval* imm;
				
				if (f->firstpass->itspointerisobtained) {
					// If the function pointed by f had its pointer obtained then
					// it mean that it can be called from within any function.
					// In order to retrieve the stackframe of its parent function,
					// I scan through previous stackframes checking each time
					// the field stackframe-id to identify the correct stackframe.
					
					// I load the pointer to the previous stackframe.
					// The following instruction is generated:
					// ld %0, %0, 2*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
					// stackframepointerscachesize, sharedregionsize and
					// vlocalmaxsize are immediate values generated
					// from the lyricalfunction pointed by f.
					
					i = ld(&rstack, &rstack, 2*sizeofgpr);
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					i->imm->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					i->imm->next->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMLOCALVARSSIZE;
					imm->f = f;
					
					i->imm->next->next->next = imm;
					
					f = f->parent;
					
					// I adjust the variables f and funclevel
					// so as to use a stackframe holder function,
					// because a stackframe-id exist only
					// in a regular stackframe which is used
					// only by stackframe holder functions.
					while (f->firstpass->stackframeholder) {
						// Within this loop, funclevel
						// will never become null because
						// it become null only after
						// the stackframe pointer of
						// interest has been loaded.
						--funclevel;
						
						f = f->parent;
					}
					
					// Here below are the instructions generated in order
					// to keep going through previous stackframes until
					// the correct stackframe is found.
					// 
					// ---------------------------------
					// #Load in r1, the address of the function
					// #for which the stackframe has to be found.
					// afip r1, OFFSETTOFUNC(f);
					// 
					// stackframenotfound:
					// 
					// #Load the offset from the start of the stackframe
					// #pointed by %0 to the field holding the return address;
					// #and use that offset to set r2 to the location of
					// #the field holding the return address.
					// ld r2, %0, 0;
					// add r2, r2, %0;
					// 
					// #Get stackframe-id.
					// ld r3, r2, 3*sizeofgpr;
					// 
					// jeq r1, r3, stackframefound;
					// 
					// #If I get here it mean that I do not have
					// #the correct stackframe in %0, so I load the previous
					// #function stackframe in %0 and redo the checks again.
					// ld %0, r2, sizeofgpr;
					// 
					// j stackframenotfound;
					// 
					// stackframefound:
					// --------------------------------
					// 
					// Note that before the jump instructions and
					// the labels used above, it is not necessary to flush
					// and discard registers, because those branching instructions
					// and labels are not generated to branch across code
					// written by the programmer.
					
					// Here I first allocate 3 registers which will
					// be used for the stackframe lookup.
					
					lyricalreg* r1;
					lyricalreg* r2;
					lyricalreg* r3;
					
					// I lock the below registers because
					// allocreg() could use them while
					// trying to allocate a new register.
					// I also lock them, otherwise they
					// will be seen as unused registers
					// when generating lyricalinstruction.
					(r1 = allocreg(CRITICALREG))->lock = 1;
					(r2 = allocreg(CRITICALREG))->lock = 1;
					(r3 = allocreg(CRITICALREG))->lock = 1;
					
					// I create the name of the labels
					// that will be needed below.
					string labelnameforstackframefound = stringfmt("%d", newgenericlabelid());
					string labelnameforstackframenotfound = stringfmt("%d", newgenericlabelid());
					
					// From here I generate the instructions
					// which will do the stackframe lookup that
					// I previously commented about above.
					
					// Here I generate the "afip" instruction.
					i = newinstruction(currentfunc, LYRICALOPAFIP);
					i->r1 = r1->id;
					i->imm = mmallocz(sizeof(lyricalimmval));
					i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
					i->imm->f = f;
					
					newlabel(labelnameforstackframenotfound);
					ldr(r2, &rstack);
					add(r2, r2, &rstack);
					ld(r3, r2, 3*sizeofgpr);
					jeq(r1, r3, labelnameforstackframefound);
					ld(&rstack, r2, sizeofgpr);
					j(labelnameforstackframenotfound);
					newlabel(labelnameforstackframefound);
					
					// Unlock lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r3->lock = 0;
					r2->lock = 0;
					r1->lock = 0;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						// The lyricalreg pointed by r1, r2 and r3 were not
						// allocated to a lyricalvariable but since I am done
						// using them, I should also produce a comment about them
						// having been discarded to complement the allocation comment
						// that is generated when they were allocated by allocreg().
						comment(stringfmt("reg %%%d discarded", r1->id));
						comment(stringfmt("reg %%%d discarded", r2->id));
						comment(stringfmt("reg %%%d discarded", r3->id));
					}
					
				} else {
					// I load the pointer to the parent function stackframe.
					// The following instruction is generated:
					// ld %0, %0, 3*sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
					// stackframepointerscachesize, sharedregionsize and
					// vlocalmaxsize are immediate values generated
					// from the lyricalfunction pointed by f.
					
					i = ld(&rstack, &rstack, 3*sizeofgpr);
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
					imm->f = f;
					
					i->imm->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMSHAREDREGIONSIZE;
					imm->f = f;
					
					i->imm->next->next = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMLOCALVARSSIZE;
					imm->f = f;
					
					i->imm->next->next->next = imm;
					
					f = f->parent;
				}
				
				if (--funclevel) {
					// If the function for which the lyricalfunction
					// is pointed by f make use of a stackframe holder,
					// the stackframe address that was loaded in the stack
					// pointer register %0 is for its stackframe holder;
					// hence I adjust the variables f and funclevel so as
					// to be at the level for that stackframe holder function.
					while (f->firstpass->stackframeholder) {
						
						if (!--funclevel) goto stackframepointerloaded;
						
						f = f->parent;
					}
					
				} else break;
			}
			
			stackframepointerloaded:
			
			// Unlock lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			if (rr) rr->lock = 0;
			
			// Getting here mean that the computation to obtain
			// the wanted stackframe address is completed.
			
			break;
		}
	}
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
	
	// Here I make a call to flushanddiscardallreg() in order
	// to discard any remaining registers which will be the ones
	// used to hold stackframe pointers; since before calling
	// this function, flushanddiscardallreg() is called to flush
	// all registers except regsiters for stackframe pointers.
	flushanddiscardallreg(DONOTFLUSHREGFORLOCALS);
	
	// The necessary instructions adjusting
	// the stack pointer register have been generated,
	// and I now need to generate instructions that
	// free any excess stackpage.
	// 
	// The following instructions are generated:
	// 
	// ----------------------------------
	// keepchecking:
	// 
	// #Bits that are the same in r and rstacksaved
	// #will yield a 0 bit value in the result.
	// #So if the addresses in r and rstacksaved
	// #point to locations within the same page,
	// #all bits above bit11 in the result of xor
	// #will be null and will yield a result value
	// #less than PAGESIZE.
	// xor r, rstacksaved, %0;
	// 
	// #I check whether the result of xor yielded
	// #a value greater than (PAGESIZE-1); which would
	// #mean that the stack pointer register was adjusted
	// #to a different page than where it was initially;
	// #and the page pointed by rstacksaved should
	// #be freed.
	// sgtui r, r, PAGESIZE-1;
	// 
	// #If the result of the above test is false,
	// #then there is no page to free.
	// jz r, noneedtokeepchecking;
	// 
	// #I compute the address start of
	// #the page to free.
	// andi r, rstacksaved, ~(PAGESIZE-1);
	// 
	// #I load the pointer to the stack page
	// #previous to the one that will be freed.
	// #That pointer is written at the bottom
	// #of the page that will be freed.
	// ld rstacksaved, r, (PAGESIZE-sizeofgpr);
	// 
	// #I free the page.
	// stackpagefree r;
	// 
	// #I jump to the label keepchecking to keep
	// #freeing page if rstacksaved and %0 do not
	// #point to the same page.
	// j keepchecking;
	// 
	// noneedtokeepchecking:
	// ----------------------------------
	// 
	// Note that before the branching instructions and
	// labels used above, it is not necessary to flush and
	// discard registers, because the branching instruction
	// and label are not used to branch across code written
	// by the programmer; furthermore flushanddiscardallreg()
	// is called before calling this function.
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("begin: checking for excess stack"));
	}
	
	// I lock the allocated register,
	// otherwise it will be seen as
	// an unused register when generating
	// lyricalinstruction, since it is not
	// assigned to anything.
	(r = allocreg(CRITICALREG))->lock = 1;
	
	// I create the name of the label needed.
	string labelnameforkeepchecking = stringfmt("%d", newgenericlabelid());
	
	newlabel(labelnameforkeepchecking);
	
	xor(r, rstacksaved, &rstack);
	
	sgtui(r, r, PAGESIZE-1);
	
	// I create the name of the label needed.
	string labelnamefornoneedtokeepchecking = stringfmt("%d", newgenericlabelid());
	
	jz(r, labelnamefornoneedtokeepchecking);
	
	andi(r, rstacksaved, PAGESIZE-1);
	
	ld(rstacksaved, r, PAGESIZE-sizeofgpr);
	
	// Unlock lyricalreg.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	rstacksaved->lock = 0;
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		// The lyricalreg pointed by rstacksaved is not allocated
		// to a lyricalvariable but since I am done using it,
		// I should also produce a comment about it having been
		// discarded to complement the allocation comment that
		// was generated when it was allocated.
		comment(stringfmt("reg %%%d discarded", rstacksaved->id));
	}
	
	stackpagefree(r);
	
	// Unlock lyricalreg.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	r->lock = 0;
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		// The lyricalreg pointed by r is not allocated
		// to a lyricalvariable but since I am done using it,
		// I should also produce a comment about it having been
		// discarded to complement the allocation comment that
		// was generated when it was allocated.
		comment(stringfmt("reg %%%d discarded", r->id));
	}
	
	j(labelnameforkeepchecking);
	
	newlabel(labelnamefornoneedtokeepchecking);
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}


// Enum used with discardoverlappingreg().
typedef enum {
	// When used, a register associated with the memory region for which I am trying to
	// discard the overlaps will be discarded without being flushed if it was dirty.
	DISCARDALLOVERLAP,
	
	// When used, a register associated with the memory region for which I am trying to
	// discard the overlaps will not be discarded nor flushed if it was dirty.
	DISCARDALLOVERLAPEXCEPTREGFORVAR,
	
	// When used, any dirty register overlapping the memory region given by
	// the arguments v and size is flushed without being discarded.
	FLUSHALLOVERLAPWITHOUTDISCARDINGTHEM
	
} discardoverlappingregflag;

// This function will discard all registers
// associated with variables which overlap
// the memory region of v, and v should only
// be a variable residing in the stack or
// global variable region. If I have a dereferenced
// variable, it is ignored as I explain later.
// This function can make use of flushreg()
// which can use allocreg() so it is necessary
// to lock any used register before using
// this function, otherwise they can get discarded
// when allocreg() try to allocate a new register.
void discardoverlappingreg (lyricalvariable* v, uint size, u64 bitselect, discardoverlappingregflag flag) {
	// Dereferenced variables are ignored;
	// it is necessary not to let isregoverlapping()
	// work with dereferenced variables otherwise
	// it will give wrong results, because
	// a dereferenced variable would be
	// seen as a local variable.
	if (v->name.ptr[1] == '*') return;
	
	// Will hold the offset within the variable
	// pointed by v, and will be extracted
	// using processvaroffsetifany();
	uint offset;
	
	// This function check if the register
	// pointed by r is associated with a variable
	// which overlap the data region specified
	// by v, size, offset and bitselect.
	// Note that a bitselected variable and
	// its main variable are not considered
	// as being the same, but rather
	// as overlapping variables.
	uint isregoverlapping(lyricalreg* r) {
		// If the variable pointed by v is the same
		// as the variable associated with the register,
		// I should return false because I should
		// return true only if the variable associated
		// with the register is overlapping
		// the variable pointed by v.
		if (r->v == v && r->offset == offset && r->size == size && r->bitselect == bitselect && flag != FLUSHALLOVERLAPWITHOUTDISCARDINGTHEM) {
			// If flag == DISCARDALLOVERLAP, a register
			// associated with the memory region for which
			// I am trying to discard the overlaps, get
			// discarded without being flushed if it was dirty.
			if (flag == DISCARDALLOVERLAP) {
				
				r->v = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					
					comment(stringfmt("reg %%%d discarded", r->id));
				}
				
				setregtothetop(r);
			}
			
			return 0;
		}
		
		if (!isvarreadonly(r->v)) {
			// If I get here, the variable associated
			// with the register pointed by r reside either
			// in the stack or global variable region;
			// and in order for the 2 variables being
			// compared to overlap, they have to belong
			// to the same function (Implying to the same
			// stack or be both global variables) and be
			// both either arguments or local variables.
			if ((r->v->funcowner == v->funcowner) &&
				(r->v->argorlocal == v->argorlocal)) {
				// If I get here, using the offset
				// of the variables within the stack
				// or global variable region, and
				// the offset from the beginning
				// of those variables, I determine
				// if the variables overlap.
				if ((r->v->offset + r->offset == v->offset + offset) ||
				((r->v->offset + r->offset < v->offset + offset) &&
				(r->v->offset + r->offset + r->size > v->offset + offset)) ||
				((r->v->offset + r->offset > v->offset + offset) &&
				(r->v->offset + r->offset < v->offset + offset + size))) return 1;
			}
		}
		
		return 0;
	}
	
	
	// Calling processvaroffsetifany() will set
	// offset and will replace the address in v
	// with the address of the main variable
	// if v was pointing to a variable which
	// had its name suffixed with an offset.
	// I need to remember that the field v
	// of registers is never set to a variable
	// which has a name suffixed with an offset.
	offset = processvaroffsetifany(&v);
	
	lyricalreg* r = currentfunc->gpr;
	
	// This function will never be called on
	// a constant variable or a variable address;
	// Hence there is no need to check whether
	// v point to those type of variables
	// in order to skip this loop.
	do {
		// I first check whether r->v is true,
		// because it can be null when the register
		// is not in used or when one of its following
		// field is set: funclevel, globalregionaddr,
		// stringregionaddr, thisaddr, retvaraddr;
		if (r->v && isregoverlapping(r)) {
			
			if (r->dirty) flushreg(r);
			
			// The register is discarded only if flag != FLUSHALLOVERLAPWITHOUTDISCARDINGTHEM.
			if (flag != FLUSHALLOVERLAPWITHOUTDISCARDINGTHEM) {
				
				r->v = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					
					comment(stringfmt("reg %%%d discarded", r->id));
				}
				
				setregtothetop(r);
			}
		}
		
		r = r->next;
		
	} while (r != currentfunc->gpr);
}
