
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


// The native operator do not return a value;
// its first argument is returned instead.
resultvar = funcarg->varpushed;

// Note that the field v of the lyricalargument
// is not used to determine whether it is
// a readonly variable, because the field
// flag->istobeoutput is not set, since it is
// an input-output argument, and pushargument()
// can duplicate the variable which it would set
// in the field v of the lyricalargument, and which
// would prevent me from correctly determining
// whether I have a readonly variable.
// Note also that this argument cannot be passed
// by reference, which would have caused
// the lyricalvariable in the field v to be
// the address of the lyricalvariable
// in the field varpushed.
if (isvarreadonly(resultvar)) {
	if (*name == '+') throwerror("argument to the native operator ++ cannot be readonly");
	else throwerror("argument to the native operator -- cannot be readonly");
}

// The field tobeusedasreturnvariable is
// set to prevent freefuncarg() from attempting
// to free the variable in the field varpushed
// of the argument, since that lyricalvariable
// will be the result of this function calling.
if (funcarg->v == funcarg->varpushed ||
	// This prevent freefuncarg() from
	// attempting to free the variable
	// in the field varpushed of the argument
	// if it is a dereference variable
	// which depend on a tempvar.
	pamsynmatch2(matchtempvarname, funcarg->varpushed->name.ptr, stringmmsz(funcarg->varpushed->name)).start)
		funcarg->tobeusedasreturnvariable = 1;

if (resultvar->name.ptr[1] != '*') {
	// Only lyricalvariable for variables explicitly
	// declared by the programmer should be used
	// with propagatevarchange; hence it should never be
	// a tempvar, a readonly variable or a dereference variable;
	// the lyricalvariable must have been used
	// with processvaroffsetifany() to insure that
	// there is no offset suffixed to its name;
	// the field id of a lyricalvariable is non-null
	// only for such lyricalvariable.
	
	lyricalvariable* v = resultvar;
	
	// I call processvaroffsetifany() to check
	// if the variable pointed by v had an offset
	// suffixed to its name. If yes, it will find
	// the main variable and set it in v.
	uint offset = processvaroffsetifany(&v);
	
	if (v->id) {
		
		uint size = sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed));
		
		// There is no need to check whether the offset is outside
		// of the boundary of the variable in memory; because
		// evaluateexpression(), when parsing the postfix operator '.',
		// prevent the use of an offset that can result in an illegal
		// access beyond the size of the variable.
		
		// If the portion to modify on the variable
		// is greater than the size of the variable,
		// the size of the portion to modify is recomputed
		// to the maximum allowable size so that the portion
		// of the variable to modify is within its boundaries.
		// Keep in mind that getregforvar() do a similar
		// work to prevent access beyond the size of a variable.
		if (size > (v->size - offset))
			size = v->size - offset;
		
		propagatevarchange(v, offset, size);
	}
}

// I cast the lyricalvariable pointed by resultvar
// with the type with which it was pushed.
// It is necessary to do so since the variable
// will become the result variable.
if (resultvar->cast.ptr) mmrefdown(resultvar->cast.ptr);
resultvar->cast = stringduplicate1(funcarg->typepushed);

// The field bitselect of the variable to return
// is restored since it was unset from
// the variable when used with pushargument().
resultvar->bitselect = funcarg->bitselect;

// Instructions are not generated in the firstpass.
if (!compilepass) break;


// I process the only argument.

// I use the type and bitselect with
// which the variable was pushed.
r1 = getregforvar(funcarg->v, 0, funcarg->typepushed, funcarg->bitselect, FORINPUT);

// I lock the register to prevent
// a call of allocreg() from using it.
r1->lock = 1;

if (funcarg->v->name.ptr[0] == '$') {
	// If the tempvar from which the register
	// pointed by r1 was loaded is shared with
	// another lyricalargument, its value
	// should be flushed if it is dirty before
	// the register reassignment; otherwise the value
	// of the tempvar will be lost whereas it is
	// still needed by another argument.
	if (issharedtempvar1() && r1->dirty)
		flushreg(r1);
	
	// I get here if getregforvar() was used
	// with the duplicate of the variable pushed;
	// nothing was done to flush(If dirty) and discard
	// any register overlapping the memory region of
	// the variable funcarg->varpushed that I am going
	// to modify; so I should discard those overlapping
	// registers in order to follow the rule about
	// the use of registers.
	// Before reassignment, I also discard any register
	// associated with the memory region for which
	// I am trying to discard overlaps, otherwise after
	// the register reassignment made below, I can have
	// 2 registers associated with the same memory location,
	// violating the rule about the use of registers; hence
	// I call discardoverlappingreg(), setting
	// its argument flag to DISCARDALLOVERLAP.
	discardoverlappingreg(
		funcarg->varpushed,
		sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)),
		funcarg->bitselect,
		DISCARDALLOVERLAP);
	
	// I manually reassign the register
	// to funcarg->varpushed since the variable
	// pointed by funcarg->varpushed is the one
	// getting modified through the register and
	// the dirty value of the register should be
	// flushed to funcarg->varpushed; I have to do
	// this because the field v of the register
	// is set to the duplicate of funcarg->varpushed.
	// Reassigning the register instead of creating
	// a new register and copying its value is faster.
	r1->v = funcarg->varpushed;
	r1->offset = processvaroffsetifany(&r1->v);
	
	// Note that the value set in the fields offset
	// and size of the register could be wrong to where
	// the register represent a location beyond the boundary
	// of the variable; and that can only occur if
	// the programmer confused the compiler by using
	// incorrect casting type.
	
} else {
	// I get here if getregforvar() was not used
	// with the duplicate of the variable pushed.
	// Since funcarg->v which was used with getregforvar()
	// was certainly not a volatile variable(Pushed
	// volatile variables get duplicated), the registers
	// overlapping the memory region of the variable
	// associated with the register that I am going to dirty
	// were only flushed without getting discarded; so
	// I should discard those overlapping registers
	// in order to follow the rule about
	// the use of registers.
	discardoverlappingreg(
		funcarg->v,
		sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)),
		funcarg->bitselect,
		DISCARDALLOVERLAPEXCEPTREGFORVAR);
}

// Since I am going to modify the value
// of the register r1, I need to set it dirty.
r1->dirty = 1;


// I perform the computation.

if (name[0] == '+') {
	
	if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
		funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')') {
		// I get here if I have a pointer
		// to variable or a pointer to function;
		// so the value of the variable pointer
		// is incremented by its stride.
		addi(r1, r1, stride(funcarg->typepushed));
		
	} else {
		// I get here for any other native type.
		addi(r1, r1, 1);
	}
	
} else {
	
	if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
		funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')') {
		// I get here if I have a pointer
		// to variable or a pointer to function;
		// so the value of the variable pointer
		// is decremented by its stride.
		addi(r1, r1, -stride(funcarg->typepushed));
		
	} else {
		// I get here for any other native type.
		addi(r1, r1, -1);
	}
}


// When the argument is volatile,
// it need to be flushed after computation.
if (*funcarg->varpushed->alwaysvolatile) flushreg(r1);


// I unlock the registers that were allocated
// and locked for the operands.
// Locked registers must be unlocked only after
// the instructions using them have been generated;
// otherwise they could be lost when insureenoughunusedregisters()
// is called while creating a new lyricalinstruction.
r1->lock = 0;
