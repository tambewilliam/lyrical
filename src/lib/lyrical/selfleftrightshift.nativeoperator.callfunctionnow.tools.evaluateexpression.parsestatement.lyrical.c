
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


// When using a signed native type
// and shifting right, I generate the instruction
// sra or srai(Arithmetic shift) instead of
// the instruction srl or srli(Logical shift).

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
	if (*name == '<') throwerror("incorrect left argument to the native operator <<=");
	else throwerror("incorrect left argument to the native operator >>=");
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


// I process the second argument first.
// For native operators which modify
// their first argument, I use getregforvar()
// on the second argument first, because for
// the first argument, I will discard any
// overlapping register in order to follow
// the rule about the loading of registers.
// So I make use of any loaded register before
// they get discarded if they were overlapping
// the first argument.

if (funcarg->next->v->isnumber) {
	// If I get here, the argument is a number
	// which should translate to an immediate value
	// when generating the instruction.
	// Note that constant variables cannot be bitselected
	// because they do not reside in memory.
	
	imm2 = ifnativetypedosignorzeroextend(funcarg->next->v->numbervalue, funcarg->next->typepushed.ptr, stringmmsz(funcarg->next->typepushed));
	
	// I set r2 to null in order to be able
	// to determine later whether the argument
	// is an immediate value.
	r2 = 0;
	
} else {
	// I use the type and bitselect with
	// which the variable was pushed.
	r2 = getregforvar(funcarg->next->v, 0, funcarg->next->typepushed, funcarg->next->bitselect, FORINPUT);
	
	// I lock the register to prevent
	// a call of allocreg() from using it.
	r2->lock = 1;
}


// I process the first argument.
// Note that I do the loading of
// the first argument last because
// it will discard any overlapping register
// in order to follow the rule about
// the use of registers. So I make use
// of any loaded registers before
// they get discarded.

// Note that the first argument cannot be
// an immediate value since it cannot be
// a readonly variable.

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

if (r2) {
	// I get here if the second argument
	// was not an immediate value.
	
	// I check whether I have a left shift.
	if (*name == '<') sll(r1, r1, r2);
	else {
		// When shifting right, I have to make
		// the difference between the arithmetic
		// shift and logical shift.
		if (*funcarg->typepushed.ptr == 'u' || *funcarg->typepushed.ptr == '#')
			srl(r1, r1, r2);
		else sra(r1, r1, r2);
	}
	
} else {
	// I get here if the second argument
	// was an immediate value.
	
	// I check whether I have a left shift.
	if (*name == '<') slli(r1, r1, imm2);
	else {
		// When shifting right, I have to make
		// the difference between the arithmetic
		// shift and logical shift.
		if (*funcarg->typepushed.ptr == 'u' || *funcarg->typepushed.ptr == '#')
			srli(r1, r1, imm2);
		else srai(r1, r1, imm2);
	}
}


// When the left argument is volatile,
// it need to be flushed after computation.
if (*funcarg->varpushed->alwaysvolatile) flushreg(r1);


// I unlock the registers that were allocated
// and locked for the operands.
// Locked registers must be unlocked only after
// the instructions using them have been generated;
// otherwise they could be lost when insureenoughunusedregisters()
// is called while creating a new lyricalinstruction.
r1->lock = 0;
if (r2) r2->lock = 0;
