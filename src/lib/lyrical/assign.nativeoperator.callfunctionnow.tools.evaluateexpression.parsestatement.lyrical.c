
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

if (isvarreadonly(resultvar)) throwerror("the left argument of the native operator '=' cannot be readonly");

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
// It is necessary to do so for the call
// to getvarduplicate(), and the fact that
// the variable will become the result variable.
if (resultvar->cast.ptr) mmrefdown(resultvar->cast.ptr);
resultvar->cast = stringduplicate1(funcarg->typepushed);

// The field bitselect of the variable to return
// is restored since it was unset from
// the variable when used with pushargument().
resultvar->bitselect = funcarg->bitselect;

// When an argument is used for the left operand
// of the native operator =, there is no need
// to duplicate its value, because it is not used by
// the operator; the operator only set its value.
// So in the firstpass, I notify the secondpass,
// that the left argument should never be duplicated
// and that information is used by pushargument().
if (!compilepass) {
	
	funcarg->flag->istobeoutput = 1;
	
	// Instructions are not generated
	// in the firstpass.
	break;
}

// The cast of the right argument is restored
// in order to have it used by getvarduplicate().
if (funcarg->next->v->cast.ptr) mmrefdown(funcarg->next->v->cast.ptr);
funcarg->next->v->cast = stringduplicate1(funcarg->next->typepushed);

// The bitselect of the right argument is restored
// in order to have it used by getvarduplicate().
funcarg->next->v->bitselect = funcarg->next->bitselect;

// Here I generate instructions to copy data
// from the right argument to the left argument.
// If the right argument is an immediate value,
// getvarduplicate() will correctly generate
// instructions that use immediate values instead of
// allocating a register for the immediate value.
// getvarduplicate() will take into consideration
// the bitselect and volatility of the left and right argument.
getvarduplicate(&funcarg->varpushed, funcarg->next->v);

// I set to zero the field bitselect
// of the lyricalvariable from
// the right argument if it was set.
// It will prevent that field from being
// used anywhere else until set again.
funcarg->next->v->bitselect = 0;
