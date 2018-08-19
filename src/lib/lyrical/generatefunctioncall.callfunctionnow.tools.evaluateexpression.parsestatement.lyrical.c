
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


// Stack and stackframe.
// 
// A stack is a collection of stackframes which keep track of
// the point to which each active subroutine should return
// control when it finish execution.
// An active subroutine is a function or operator that has
// been called but has yet to complete execution.
// 
// A stackframe is used to hold the arguments and
// local variables of the instance of a called function;
// in fact, the same function can be called again before
// it get to return; which lead to multiple active instances
// of a called function; and calling the same function before
// it return from its previous call is a recursion.
// 
// In order to implement the recursive call of functions,
// the stackframe of each function also hold a pointer to
// the stackframe of its immediate parent function; and
// that pointer is used when traversing the stack in search of
// a particular stackframe.
// In fact, calling searchfunc() return the depth from
// a parent function of interest; and the depth value is used
// to determine how many stackframes should be traversed
// to reach the stackframe of the parent function of interest.
// 
// When calling a function through a pointer to function,
// there is no way to determine which function is being
// called in order to compute the depth from any of its parent
// functions so as to be able to obtain the stackframe adress
// of any of its parent functions; and it is always assumed that
// a function called through a pointer to function has been called
// after its immediate parent function has been called and
// has not yet returned.
// So in order to retrieve the stackframe address of
// the instance of the function immediately parent to a function
// called through a pointer to function, each stackframe also
// hold a stackframe-id which is the memory address of the first
// instruction of the function owning the stackframe.
// During the retrieval of the stackframe address, while
// traversing each stackframe, the stackframe-id is compared
// against the memory address of the parent function until
// a match is found.
// Note that a parent function could have been recursively
// called more than once, which would mean that more than
// one stackframe with the same stackframe-id exist in
// the stack; and the stackframe of the correct instance of
// the parent function would be the first match while
// traversing the stack.
// So a function called through a pointer should have been
// called from within an instance of its immediate parent function,
// or from within any subfunction of its immediate parent function;
// otherwise a pagefault will likely occur because the stack
// will be traversed with no match.
// 
// In conclusion, each stackframe keep the stackframe address
// of their immediate parent function; and that allow
// for the nesting of functions.
// Each stackframe also keep a stackframe-id which is the address
// of the function for which the stackframe was created; and
// that allow for calling a function through a pointer and still
// be able to reach to the stackframe of any of its parent functions.
// The stack layout is also described in doc/stackframe.txt;
// and within regmanipulations.tools.parsestatement.lyrical.c,
// the functions setregstackptrtofuncstackframe() and
// getregptrtofuncstackframe() implement
// the traversing of the stack.


// In the firstpass this function check
// for arguments to be passed byref and
// set their field flag->istobepassedbyref
// so that argument to be passed byref
// get correctly handled in the secondpass.
// In the secondpass this function generate
// instructions to call a function by creating
// and writing its stackframe and generating
// the instruction jump-and-link to
// the address of the function.
// Note that instructions to call a function
// are also generated within flushreg()
// for predeclared variables with a callback.
void generatefunctioncall () {
	
	if (!compilepass) {
		
		if (funcarg && !varptrtofunc) {
			// sfr.f->varg point to the last variable
			// of the linkedlist. I set it to the first
			// variable of the linkedlist. It will be set
			// back to the last variable of the linkedlist
			// upon completion of the loop below.
			sfr.f->varg = sfr.f->varg->next;
			
			// Note that funcarg was already set to
			// the first argument of the linkedlist.
			lyricalargument* arg = funcarg;
			
			// Variable used to keep track
			// of the argument position.
			uint argpos = 1;
			
			while (1) {
				// I get here if I am in the firstpass
				// and I am not calling the function
				// through a pointer to function.
				// I only come here when I am not
				// calling the function through a pointer
				// to function because I use sfr.f which
				// is only used for a declared function;
				// similar work is done in parsetypeofptrtofunc()
				// for pointers to function.
				
				// I check if the argument is to be passed by reference.
				if (sfr.f->varg->isbyref) {
					// bitselected, address or constant
					// variables cannot be passed byref.
					if (arg->bitselect || isvarreadonly(arg->v)) {
						throwerror(stringfmt("incorrect %dth argument passed by reference", argpos).ptr);
					}
					
					// I notify the secondpass that the argument
					// will be passed byref. That information is
					// used in the secondpass within pushargument();
					arg->flag->istobepassedbyref = 1;
				}
				
				// If I am calling a variadic function,
				// there can be more argument to pass than
				// variable argument of the function that
				// I am calling.
				// To determine if I went through all
				// the variable argument, I use the fact
				// that variable arguments are a circular
				// linkedlist and I use the fact that
				// the first variable argument of a function
				// always has its field offset null and
				// is the only one in the linkedlist
				// to have its field offset null.
				// So if the test below is false, it mean
				// that sfr.f->varg point to the last
				// argument of the function and I should
				// no longer keep going through the arguments
				// of the function; I can return because
				// there is no other byref argument to process.
				if (sfr.f->varg->next->offset) sfr.f->varg = sfr.f->varg->next;
				else return;
				
				++argpos;
				
				arg = arg->next; // Select the next argument.
			}
		}
		
		return;
	}
	
	// I get here only in the secondpass.
	// In the firstpass, instructions for
	// calling the function are not generated.
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		
		string s = stringduplicate2("begin: calling function ");
		
		if (varptrtofunc) stringappend2(&s, "through a pointer");
		
		comment(s);
	}
	
	// This function is only used when calling
	// a function which use a stackframe holder,
	// and it is only used after the generation
	// of an instruction which used the address
	// to the stackframe holder.
	// It will add additional lyricalimmval
	// to the last lyricalinstruction
	// generated in currentfunc.
	// Those lyricalimmval are offsets to
	// the tiny stackframe contained within
	// the shared region of the stackframe holder.
	void addoffsetstotinystackframe () {
		
		lyricalimmval* imm1 = mmallocz(sizeof(lyricalimmval));
		imm1->type = LYRICALIMMVALUE;
		imm1->n = sizeofgpr;
		
		lyricalimmval* imm2 = imm1;
		
		imm1 = mmallocz(sizeof(lyricalimmval));
		imm1->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
		imm1->f = sfr.f->firstpass->stackframeholder->secondpass;
		
		imm2->next = imm1;
		
		imm1 = mmallocz(sizeof(lyricalimmval));
		imm1->type = LYRICALIMMOFFSETWITHINSHAREDREGION;
		imm1->sharedregion = sfr.f->firstpass->sharedregiontouse;
		
		imm2->next->next = imm1;
		
		imm2->next->next->next = currentfunc->i->imm;
		
		currentfunc->i->imm = imm2;
	}
	
	lyricalreg* r1;
	
	lyricalreg* r2;
	
	lyricalimmval** stackneededvalue;
	
	u64* stackframeadjustvalue1;
	
	u64* stackframeadjustvalue2;
	
	uint stackframeoffset;
	
	// I check whether I am calling a function
	// which use a stackframe holder.
	if (varptrtofunc || !sfr.f->firstpass->stackframeholder) {
		// I allocate and lock a register that will
		// be used to hold the address where the stackframe
		// of the function to call is located and where
		// arguments of the function to call will be written.
		// I lock the register because I will be calling functions
		// which make use of allocreg() and if the register
		// is not locked, allocreg() could use it while
		// trying to allocate a new register.
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
		// andi r1, %0, PAGESIZE -1;
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
		// #I allocate a new stack page.
		// stackpagealloc r1;
		// 
		// #I set r1 to the bottom of the stack page
		// #where the current value of the stack
		// #pointer register will be written.
		// addi r1, r1, PAGESIZE-sizeofgpr;
		// 
		// #I store the current value of the stack
		// #pointer register; that allow for keeping
		// #all allocated stack page in a linkedlist.
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
		
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPSLTUI);
		i->r1 = r1->id;
		i->r2 = r1->id;
		
		// Here I save the address where to set
		// the amount of stack needed by the stackframe
		// of the function to call so that later
		// it can be set.
		stackneededvalue = &i->imm;
		
		// I create the name of the label needed.
		string labelnameforenoughstackleft = stringfmt("%d", newgenericlabelid());
		
		jz(r1, labelnameforenoughstackleft);
		
		stackpagealloc(r1);
		
		addi(r1, r1, PAGESIZE-sizeofgpr);
		
		str(&rstack, r1);
		
		i = newinstruction(currentfunc, LYRICALOPADDI);
		i->r1 = r1->id;
		i->r2 = r1->id;
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMVALUE;
		
		// Here I save the address of the variable
		// which contain the amount by which the stack
		// is to be adjusted, so that later it can be set.
		stackframeadjustvalue1 = &i->imm->n;
		
		// I create the name of the label needed.
		string labelnamefordoneallocatingstack = stringfmt("%d", newgenericlabelid());
		
		j(labelnamefordoneallocatingstack);
		
		newlabel(labelnameforenoughstackleft);
		
		i = newinstruction(currentfunc, LYRICALOPADDI);
		i->r1 = r1->id;
		i->r2 = 0;
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMVALUE;
		
		// Here I save the address of the variable
		// which contain the amount by which the stack
		// is to be adjusted, so that later it can be set.
		stackframeadjustvalue2 = &i->imm->n;
		
		newlabel(labelnamefordoneallocatingstack);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to previous stackframe"));
		}
		
		// Here I write the field which hold
		// the pointer to the previous stackframe.
		// the stackframe address to write is the
		// current value of the stack pointer register.
		str(&rstack, r1);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to stackframe of function parent to function being called"));
		}
		
		if (sfr.f && !sfr.f->firstpass->itspointerisobtained) {
			// Here I write the pointer to the
			// stackframe of the parent function.
			if (sfr.funclevel) {
				// The value of sfr.funclevel is used with getregptrtofuncstackframe()
				// to obtain the address of the stackframe of the function which is
				// parent to the function that I am calling.
				// So calling getregptrtofuncstackframe() return a register
				// containing the address to write.
				// If sfr.f->parent == rootfunc, there is no need to write
				// the pointer to the parent function stackframe (because
				// there is no parent function stackframe), unless rootfunc
				// hold the tiny stackframe of its children in its shared region.
				if (sfr.f->parent != rootfunc || rootfunc->firstpass->sharedregions) {
					// I lock the allocated register to prevent
					// another call to allocreg() from using it.
					// I also lock it, otherwise it could be lost
					// when insureenoughunusedregisters() is called
					// while creating a new lyricalinstruction.
					(r2 = getregptrtofuncstackframe(sfr.funclevel))->lock = 1;
					
					// The value of the register obtained above is written.
					st(r2, r1, sizeofgpr);
					
					// Unlock the lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r2->lock = 0;
				}
				
			} else {
				// If I get here, I am calling a function that is a child
				// of currentfunc; the stackframe address to write is
				// the current value of the stack pointer register.
				// If sfr.f->parent == rootfunc, there is no need to write
				// the pointer to the parent function stackframe (because
				// there is no parent function), unless rootfunc hold
				// the tiny stackframe of its children in its shared region.
				// I will also get here if I am calling a function through
				// a variable pointer to function, and when that is the case
				// there is no need to write the pointer to the parent
				// function stackframe; in fact a function which was called
				// through a variable pointer to function retrieve its parent
				// function stackframe scanning through previous stackframes.
				if (!varptrtofunc && (sfr.f->parent != rootfunc || rootfunc->firstpass->sharedregions))
					st(&rstack, r1, sizeofgpr);
			}
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write stackframe-id"));
		}
		
		// Here I generate instructions to write
		// the stackframe-id of the function to call.
		// The stackframe-id value is the starting
		// address of the function to call.
		if (varptrtofunc) {
			// The lyricalvariable varptrtofunc certainly has
			// a pointer to function cast or type, but here
			// for faster result I use "void*".
			// A lyricalvariable which has a pointer to function type
			// can never be bitselected, hence there is no need to consider
			// the bitselect of the lyricalvariable varptrtofunc.
			(r2 = getregforvar(varptrtofunc, 0, voidptrstr, 0, FORINPUT))->lock = 1;
			
			// The value of the register obtained
			// above is written after the location
			// where the stackframe address of
			// the parent function is written.
			st(r2, r1, 2*sizeofgpr);
			
			// If the lyricalvariable varptrtofunc is volatile, its value
			// may end up being different when used with the instruction jlr;
			// hence the value which was used for the stackframe-id wouldn't
			// be the same value used with the instruction jlr.
			// Such discrepancy can be prevented by locking the register
			// allocated for the lyricalvariable varptrtofunc, and re-using
			// that same register with the instruction jlr, but locking
			// the register would increase the minimum count of gpr to 5 from 4;
			// because when calling a function through a pointer to function
			// and one of the argument has a size greater than sizeofgpr,
			// a total of 5 registers would be needed at once.
			// It is the responsability of the programmer not
			// to call a function through a volatile variable.
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r2->lock = 0;
			
		} else {
			// Since the register is not assigned
			// to anything, I lock it, otherwise
			// it will be seen as an unused register
			// when generating lyricalinstruction.
			(r2 = allocreg(CRITICALREG))->lock = 1;
			
			// Here I generate the "afip" instruction.
			i = newinstruction(currentfunc, LYRICALOPAFIP);
			i->r1 = r2->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
			i->imm->f = sfr.f;
			
			// The value of the register obtained above
			// is written after the location where
			// the stackframe address of the parent
			// function is written.
			st(r2, r1, 2*sizeofgpr);
			
			// Unlock the lyricalreg.
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
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write \"this\" variable pointer"));
		}
		
		// If the function is being called
		// through a variable ponter to function,
		// the address of that variable get written
		// otherwise null get written only if it is
		// a function that used the keyword "this".
		if (varptrtofunc && !isvarreadonly(varptrtofunc)) {
			// The lyricalvariable resulting
			// from getvaraddr(varptrtofunc) certainly
			// has a valid cast or type, but here
			// for faster result I use "void*" .
			(r2 = getregforvar(
				getvaraddr(varptrtofunc, DONOTMAKEALWAYSVOLATILE),
				0, voidptrstr, 0, FORINPUT))->lock = 1;
			
			// I write the value of the register obtained above
			// after the location where the stackframe-id of
			// the function to call is written.
			st(r2, r1, 3*sizeofgpr);
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r2->lock = 0;
			
		} else if (sfr.f && sfr.f->firstpass->usethisvar) {
			// I lock the register, otherwise
			// it will be seen as an unused register
			// when generating lyricalinstruction.
			(r2 = allocreg(CRITICALREG))->lock = 1;
			
			// I set the register value null;
			// which is then used to write null.
			li(r2, 0);
			
			// I write the value of the register
			// after the location where the stackframe-id
			// of the function to call is written.
			st(r2, r1, 3*sizeofgpr);
			
			// Unlock the lyricalreg.
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
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to result variable"));
		}
		
		// If the function return a variable, I write
		// the pointer to the result variable.
		// There is no need to write the pointer to the result
		// variable if the function is not returning a variable
		// because I know where the function is coming from and
		// it will not use that field.
		// When using pointers to function, the compiler is not
		// responsible for codes written by a programmer fooling
		// the compiler by using a pointer to function which
		// was declared for functions not returning a variable,
		// with a function which is returning a variable.
		if (!stringiseq2(typeofresultvar, "void")) {
			// I lock the register because generateloadinstr()
			// could make use of functions which use allocreg()
			// and if the register is not locked, allocreg() could
			// use it while trying to allocate a new register.
			// I also lock it, otherwise it will
			// be seen as an unused register
			// when generating lyricalinstruction.
			(r2 = allocreg(CRITICALREG))->lock = 1;
			
			// Note that generateloadinstr() only work with variables
			// which reside in the stack or global variable region.
			// Since the result variable pointed by resultvar resides there,
			// I am perfectly fine to use generateloadinstr().
			generateloadinstr(r2, resultvar, 0, 0, LOADADDR);
			
			// The reason why I use generateloadinstr() instead
			// of getregforvar() is that it is faster; furthermore
			// there is no need to call getregforvar() on the result
			// variable because there is no chance to find an already
			// allocated register since the result variable was just
			// created within callfunctionnow(), and there is no need
			// to have a register associated with the result variable
			// at this point, since a jump-and-link will be done and
			// all registers will be discarded.
			
			// I write the value of the register after
			// the location where the "this" pointer
			// of the function to call is written.
			st(r2, r1, 4*sizeofgpr);
			
			// Unlock the lyricalreg.
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
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		// I set this variable with the offset
		// where arguments are to be written.
		// 5*sizeofgpr account for the space used by
		// the address of the previous function stackframe,
		// the address of the parent function stackframe,
		// the stackframe-id, the "this" pointer
		// and the address of the return variable.
		stackframeoffset = 5*sizeofgpr;
		
	} else {
		// I get here if I am calling a function
		// which make use of a stackframe holder.
		
		// Note that null cannot be used with getregptrtofuncstackframe();
		if (sfr.funclevel) r1 = getregptrtofuncstackframe(sfr.funclevel);
		else r1 = &rstack;
		// I lock the register which contain
		// the address of where to write in the stack.
		// I lock the register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		r1->lock = 1;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to previous stackframe"));
		}
		
		if (r1 != &rstack) {
			// I write the field which hold the pointer
			// to the previous stackframe.
			// the stackframe address to write is
			// the current value of the stack pointer register.
			st(&rstack, r1, sizeofgpr);
			
			// To the last instruction generated above, I add
			// the offsets to the tiny stackframe contained
			// within the shared region.
			addoffsetstotinystackframe();
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: write pointer to result variable"));
		}
		
		// If the function return a variable, I write
		// the pointer to the result variable.
		// Note that for functions that do not return a value
		// and make use of a stackframe holder, there is
		// no space allocated in their tiny stackframe to write
		// the return variable address, as oppose to
		// a regular stackframe where that space is always
		// allocated regardless of whether the function
		// that I am calling return a value.
		if (!stringiseq2(typeofresultvar, "void")) {
			// I lock the register because generateloadinstr()
			// could make use of functions which use allocreg()
			// and if the register is not locked, allocreg() could
			// use it while trying to allocate a new register.
			// I also lock it, otherwise it will
			// be seen as an unused register
			// when generating lyricalinstruction.
			(r2 = allocreg(CRITICALREG))->lock = 1;
			
			// Note that generateloadinstr() only work with variables
			// which reside in the stack or global variable region.
			// Since the result variable pointed by resultvar reside there,
			// I am perfectly fine to use generateloadinstr().
			generateloadinstr(r2, resultvar, 0, 0, LOADADDR);
			
			// The reason why I use generateloadinstr() instead
			// of getregforvar() is that it is faster; furthermore
			// there is no need to call getregforvar() on the result
			// variable because there is no chance to find an already
			// allocated register since the result variable was just
			// created within callfunctionnow(), and there is no need
			// to have a register associated with the result variable
			// at this point, since a jump-and-link will be done and
			// all registers will be discarded.
			
			// I write the value of the register obtained above.
			st(r2, r1, 2*sizeofgpr);
			
			// To the last instruction generated above, I add
			// the offsets to the tiny stackframe contained
			// within the shared region.
			addoffsetstotinystackframe();
			
			// Unlock the lyricalreg.
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
			
			stackframeoffset = 3*sizeofgpr;
			
		} else stackframeoffset = 2*sizeofgpr;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
	}
	
	// If I have arguments, I write them to the stack.
	if (funcarg) {
		// funcarg was already set to the first
		// argument of the linkedlist by callfunctionnow().
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: writting function arguments"));
		}
		
		// Here I save the address of the first
		// lyricalargument in the linkedlist pointed by funcarg.
		// It will allow me to determine whether
		// all arguments have been processed.
		lyricalargument* firstarg = funcarg;
		
		do {
			// Let's consider a function defined as:
			// void myfunc(type var);
			// Where the type has a size of 16 bytes.
			// If the myfunc() is called this way: myfunc((type)0); the compiler should be smart enough
			// to write the constant value only once in the stack of the function to call.
			// Similarly if I use as argument a variable which was declared as such; u16 var;
			// and call myfunc() this way: myfunc((type)var); the compiler should be smart enough
			// to write only the bytes belonging to var otherwise it will end up going pass
			// the boundaries of var which can cause a pagefault.
			// If myfunc() is called using a dereference variable all the 16bytes can be copied
			// from the dereference variable because dereference variable do not have a type but only a
			// cast. So the real size of the dereference variable can be anything; hence I am ok to
			// copy all the 16bytes from the dereference variables. It will be the task of the programmer
			// to insure that no pagefault occur.
			// All those features are implemented by appropriately setting argvarsize.
			
			uint argvarsize;
			uint typepushedisnative;
			
			if (funcarg->flag->istobepassedbyref) {
				// When writting an argument byref,
				// I set typepushedisnative to null;
				// this way, funcarg->typepushed will
				// not be used since it contain the type
				// of the variable that was pushed and
				// not the type of the address variable.
				
				argvarsize = sizeofgpr;
				typepushedisnative = 0;
				
			} else {
				
				argvarsize = sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed));
				typepushedisnative = (uint)pamsynmatch2(isnativetype, funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)).start;
			}
			
			// If the variable pushed has its field type set,
			// I check whether its size in memory is smaller
			// than what was previously set in argvarsize.
			if (funcarg->v->type.ptr) {
				// If the variable pushed has a memory size smaller than the size
				// of the type used when it was pushed, I use its memory size.
				if (funcarg->v->size < argvarsize) argvarsize = funcarg->v->size;
			}
			
			// Here I copy the arguments value to
			// the stackframe of the function to call.
			
			if (isvarreadonly(funcarg->v) || argvarsize <= sizeofgpr) {
				// I get here if I am copying from
				// a variable generated by the compiler,
				// or if argvarsize <= sizeofgpr;
				
				string type;
				
				if (typepushedisnative && funcarg->typepushed.ptr[0] == 's')
					type = largeenoughsignednativetype(argvarsize);
				else type = largeenoughunsignednativetype(argvarsize);
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				(r2 = getregforvar(funcarg->v, 0, type, funcarg->bitselect, FORINPUT))->lock = 1;
				
				// Note that if the variable was pushed using a native type,
				// its register value is written to the stack without truncation
				// and occupy a sizeofgpr memory space.
				// Similarly, for any other argument which do not use a native type,
				// every argument is aligned to sizeofgpr, hence the instruction st
				// can safely be used to write the argument to the stack.
				// Keep in mind that getregforvar(), when appropriate,
				// apply sign or zero extension to the value
				// of the register that it return.
				st(r2, r1, stackframeoffset);
				
				// I check whether I am calling a function
				// which use a stackframe holder.
				if (!varptrtofunc && sfr.f->firstpass->stackframeholder) {
					// To the last instruction generated above, I add
					// the offsets to the tiny stackframe contained
					// within the shared region.
					addoffsetstotinystackframe();
				}
				
				// This function is used to determine whether
				// the tempvar pointed by funcarg->v is
				// shared with another lyricalargument.
				uint issharedtempvar2 () {
					// I first do the search among the remaining
					// lyricalargument to write in the stack.
					
					lyricalargument* arg = funcarg->next;
					
					while (arg != firstarg) {
						// Note that if arg->v == funcarg->v,
						// the lyricalvariable pointed by arg->v
						// is always a tempvar that was created
						// by propagatevarchange().
						if (arg->v == funcarg->v) return 1;
						
						arg = arg->next;
					}
					
					// I get here if I couldn't find a match
					// among the remaining lyricalargument
					// to write in the stack.
					// I now search among registered arguments.
					
					arg = registeredargs;
					
					while (arg) {
						// Note that if arg->v == funcarg->v,
						// the lyricalvariable pointed by arg->v
						// is always a tempvar that was created
						// by propagatevarchange().
						if (arg->v == funcarg->v) return 1;
						
						arg = arg->nextregisteredarg;
					}
					
					return 0;
				}
				
				// Unlock the lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r2->lock = 0;
				
				// If the register pointed by r2 is allocated to
				// a tempvar not to be used as the return variable,
				// and if the tempvar is not being shared with
				// another lyricalargument, it is discarded.
				// Discarding the register prevent its value, if dirty,
				// from being flushed by flushanddiscardallreg() which
				// is called before generating the branching instruction.
				if (!funcarg->tobeusedasreturnvariable &&
					funcarg->v->name.ptr[0] == '$' &&
						!issharedtempvar2()) {
					
					r2->v = 0;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						comment(stringfmt("reg %%%d discarded", r2->id));
					}
					
					setregtothetop(r2);
				}
				
			} else {
				// If I get here, the argument certainly reside in memory
				// and can safely be used with discardoverlappingreg().
				// Registers overlapping the source memory region of
				// the copy should be flushed without being discarded.
				discardoverlappingreg(funcarg->v, argvarsize, 0, FLUSHALLOVERLAPWITHOUTDISCARDINGTHEM);
				
				// I obtain the address variable of funcarg->v.
				
				// I insure that the type or cast
				// of funcarg->v is correctly set.
				// I cast the variable pointed by
				// funcarg->v with the type with
				// which it was pushed.
				// It is necessary to do so since the
				// variable will be used with getvaraddr().
				
				if (funcarg->v->cast.ptr) mmrefdown(funcarg->v->cast.ptr);
				
				funcarg->v->cast = stringduplicate1(funcarg->typepushed);
				
				lyricalvariable* varaddrofarg = getvaraddr(funcarg->v, DONOTMAKEALWAYSVOLATILE);
				
				// I load in registers, the destination and
				// source addresses; I also lock r2 and r3
				// to prevent a call of allocreg() from using them.
				// Note that because the memcpy instruction will
				// modify the value of its input registers, the registers
				// used with that instruction should be flushed if dirty
				// and discarded in order to de-associate them from
				// their variable.
				
				// Note that the lyricalvariable pointed by varaddrofarg
				// will certainly not have a bitselect.
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				(r2 = getregforvar(varaddrofarg, 0, varaddrofarg->cast, 0, FORINPUT))->lock = 1;
				
				if (r2->dirty) flushreg(r2);
				
				r2->v = 0;
				
				lyricalreg* r3 = allocreg(CRITICALREG);
				// Since the register is not assigned
				// to anything, I lock it, otherwise
				// it will be seen as an unused register
				// when generating the lyricalinstruction
				// for memcpy.
				r3->lock = 1;
				
				// I generate the instruction that compute the address
				// of the destination memory region in the stackframe.
				addi(r3, r1, stackframeoffset);
				
				// I check whether I am calling a function
				// which use a stackframe holder.
				if (!varptrtofunc && sfr.f->firstpass->stackframeholder) {
					// To the last instruction generated above, I add
					// the offsets to the tiny stackframe contained
					// within the shared region.
					addoffsetstotinystackframe();
				}
				
				// Note that the source and destination memory region
				// of the copy will never overlap, so there is no need
				// to compare the source and destination addresses
				// to determine whether the copy should be done from
				// the bottom of the memory regions involved
				// in the copy.
				memcpyi(r3, r2, argvarsize/sizeofgpr);
				
				// Unlock the lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r2->lock = 0;
				r3->lock = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					// The lyricalreg pointed by r2 and r3 were not allocated
					// to a lyricalvariable but since I am done using them,
					// I should also produce a comment about them having been
					// discarded to complement the allocation comment that
					// were generated when they were allocated.
					comment(stringfmt("reg %%%d discarded", r2->id));
					comment(stringfmt("reg %%%d discarded", r3->id));
				}
				
				setregtothetop(r3);
				setregtothetop(r2);
			}
			
			stackframeoffset += argvarsize;
			
			// Every stack argument is aligned to sizeofgpr;
			// so here I make sure that stackframeoffset
			// is aligned to sizeofgpr.
			stackframeoffset = ROUNDUPTOPOWEROFTWO(stackframeoffset, sizeofgpr);
			
		} while ((funcarg = funcarg->next) != firstarg);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
	}
	
	// I use the value of stackframeoffset to insure that
	// the call arguments usage did not exceed its limit.
	// If I am calling a function which use a stackframe holder
	// stackframeoffset was initialized to 2*sizeofgpr or 3*sizeofgpr
	// prior to writting the arguments depending on whether
	// the function returned a value, otherwise it was initialized
	// to 5*sizeofgpr prior to writting the arguments.
	if (varptrtofunc || !sfr.f->firstpass->stackframeholder) {
		
		if (stackframeoffset > (MAXARGUSAGE + (5*sizeofgpr)))
			throwerror("call arguments usage exceed limit");
		
		stackframeoffset += compileargfunctioncallargsguardspace*sizeofgpr;
		
		// I can now set the value by which the stackframe
		// is adjusted before writting its arguments; that
		// value was being computed in stackframeoffset.
		*stackframeadjustvalue1 = -stackframeoffset;
		*stackframeadjustvalue2 = -stackframeoffset;
		
	} else {
		
		if (stringiseq2(typeofresultvar, "void")) {
			
			if (stackframeoffset > (MAXARGUSAGE + (2*sizeofgpr)))
				throwerror("call arguments usage exceed limit");
				
		} else {
			
			if (stackframeoffset > (MAXARGUSAGE + (3*sizeofgpr)))
				throwerror("call arguments usage exceed limit");
		}
	}
	
	if (varptrtofunc) {
		// I get here if I am calling the function
		// through a pointer to function.
		
		// The stackframe usage of a function called
		// through a pointer to function cannot be determined
		// at compiled time because its locals usage cannot
		// be estimated, hence the reason why I use MAXSTACKUSAGE
		// as the stackframe usage of the function
		// that I am calling.
		
		lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		imm->n = MAXSTACKUSAGE;
		
		(*stackneededvalue) = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		// To the stack needed amount,
		// I add a value which represent
		// the additional stack space needed
		// by LYRICALOPSTACKPAGEALLOC when
		// allocating a new stack page.
		imm->n = compileargstackpageallocprovision;
		imm->next = (*stackneededvalue);
		
		(*stackneededvalue) = imm;
		
		// I retrieve the address of the function
		// that is being called through a pointer and lock it
		// since flushanddiscardallreg() will be called and
		// it could make use allocreg() which could use
		// the register while allocating a new register.
		
		// The lyricalvariable varptrtofunc certainly has
		// a pointer to function cast or type, but here
		// for faster result I use "void*".
		// An lyricalvariable which has a pointer to function type
		// can never be bitselected, hence there is no need to consider
		// the bitselect of the lyricalvariable varptrtofunc. 
		// I lock the register because flushanddiscardallreg()
		// could make use of functions which use allocreg()
		// and if the register is not locked, allocreg() could
		// use it while trying to allocate a new register.
		// I also lock it, otherwise it will
		// be seen as an unused register
		// when generating lyricalinstruction.
		(r2 = getregforvar(varptrtofunc, 0, voidptrstr, 0, FORINPUT))->lock = 1;
		
	} else if (!sfr.f->firstpass->stackframeholder) {
		// If I get here, I am calling a function
		// which do not use a stackframe holder.
		
		lyricalimmval* imm;
		
		if (sfr.f->firstpass->toimport) {
			// The stackframe usage of an imported function
			// cannot be determined at compiled time because
			// its locals usage cannot be estimated,
			// hence the reason why I use MAXSTACKUSAGE
			// as the stackframe usage of the function
			// that I am calling.
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMVALUE;
			imm->n = MAXSTACKUSAGE;
			
		} else {
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMLOCALVARSSIZE;
			imm->f = sfr.f;
			
			(*stackneededvalue) = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSHAREDREGIONSIZE;
			imm->f = sfr.f;
			imm->next = (*stackneededvalue);
			
			(*stackneededvalue) = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = sfr.f;
			imm->next = (*stackneededvalue);
			
			(*stackneededvalue) = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMVALUE;
			// The following expression was simplified:
			// (7*sizeofgpr) + (stackframeoffset - (5*sizeofgpr));
			// It account for 5*sizeofgpr which was used
			// to initialize stackframeoffset.
			// 7*sizeofgpr account for the uint fields of
			// a regular stackframe; refer to doc/stackframe.txt;
			imm->n = (2*sizeofgpr)+stackframeoffset;
			imm->next = (*stackneededvalue);
		}
		
		(*stackneededvalue) = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		// To the stack needed amount,
		// I add a value which represent
		// the additional stack space needed
		// by LYRICALOPSTACKPAGEALLOC when
		// allocating a new stack page.
		imm->n = compileargstackpageallocprovision;
		imm->next = (*stackneededvalue);
		
		(*stackneededvalue) = imm;
	}
	
	// Since I am going to do a branching,
	// I flush and discard all registers.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	// I generate the instruction that push in the stack or store
	// in %1 the return address and jump to the function to call.
	if (varptrtofunc) {
		// This part can only come after flushanddiscardallreg(),
		// since flushing still need the address in the stack
		// pointer register unchanged.
		// So here, I set the stack pointer
		// register to the value of r1.
		cpy(&rstack, r1);
		
		jpushr(r2);
		
		// Unlock the lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r2->lock = 0;
		
		goto uponreturning;
		
	} else {
		// If I am calling a function which use
		// a stackframe holder, the return address
		// get stored in %1, instead of getting
		// pushed in the stack.
		if (sfr.f->firstpass->stackframeholder) {
			
			if (r1 != &rstack) {
				// This part can only come after flushanddiscardallreg(),
				// since flushing still need the address in the stack
				// pointer register unchanged.
				// So here, I set the stack pointer
				// register to the value of r1.
				cpy(&rstack, r1);
			}
			
			// Search the lyricalreg for %1.
			lyricalreg* r = searchreg(currentfunc, 1);
			
			// Lock the lyricalreg for %1,
			// to prevent newinstruction()
			// from adding it to the list
			// of unused registers of
			// the lyricalinstruction.
			r->lock = 1;
			
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJL);
			i->r1 = 1;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
			i->imm->f = sfr.f;
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			if (r1 != &rstack) {
				// Once the function return,
				// I restore the stack
				// pointer register.
				
				ld(&rstack, &rstack, sizeofgpr);
				
				// To the last instruction generated above, I add
				// the offsets to the tiny stackframe contained
				// within the shared region.
				addoffsetstotinystackframe();
			}
			
		} else {
			// This part can only come after flushanddiscardallreg(),
			// since flushing still need the address in the stack
			// pointer register unchanged.
			// So here, I set the stack pointer
			// register to the value of r1.
			cpy(&rstack, r1);
			
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJPUSH);
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
			i->imm->f = sfr.f;
			
			uponreturning:
			
			// Once the function return, I restore
			// the stack pointer register. Remember that
			// the stack grow toward lower addresses; so
			// to restore it, I need to increment it.
			addi(&rstack, &rstack, stackframeoffset);
			
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
			// #of the stack page.
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
			
			newlabel(labelnamefornoneedtofreestack);
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				comment(stringduplicate2("end: done"));
			}
			
			// This should come right after where r1 get unlocked,
			// but this comment is to be used only when r1 was allocated
			// by allocreg() and was not associated to something; and that
			// is the case when (varptrtofunc || !sfr.f->firstpass->stackframeholder).
			// The unlocking of r1 come right after this comment, so it is still ok.
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				// The lyricalreg pointed by r1 is not allocated
				// to a lyricalvariable but since I am done using it,
				// I should also produce a comment about it having been
				// discarded to complement the allocation comment that
				// was generated when it was allocated.
				comment(stringfmt("reg %%%d discarded", r1->id));
			}
		}
	}
	
	// Unlock the lyricalreg.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	r1->lock = 0;
	
	// There is no need to call setregtothetop()
	// on the lyricalreg pointed by r1 or r2(if used)
	// since I flushed and discarded all registers
	// when using flushanddiscardallreg() above.
	
	if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("end: done"));
	}
}
