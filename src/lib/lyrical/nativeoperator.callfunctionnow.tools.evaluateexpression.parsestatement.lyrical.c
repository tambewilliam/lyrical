
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


// rresult point to the register
// used for the result variable.
// r1 and r2 point to the register used for
// the first and second variable respectively.
lyricalreg* rresult; lyricalreg* r1; lyricalreg* r2;

// immresult is used when
// the result variable is
// an immedediate value.
// imm1 and imm2 are used
// when the first and second
// arguments respectively
// are immediate values.
u64 immresult, imm1, imm2;

// This function return the stride of a pointer.
// The argument type must be a valid string
// and must be a pointer type.
uint stride (string type) {
	
	uint i = stringmmsz(type)-1;
	
	// If the type is a pointer to function,
	// the stride is always sizeofgpr.
	if (type.ptr[i] == ')') return sizeofgpr;
	
	// For any other pointer type which is
	// terminated with '*', I simply remove the last
	// asterix and get the size of the resulting type.
	return sizeoftype(type.ptr, i);
}

// This function is used to determine whether
// the tempvar pointed by funcarg->v is
// shared with another lyricalargument.
// This function is to be called only by operators
// for which their first lyricalargument
// is an input-output argument.
uint issharedtempvar1 () {
	// I first do the search among the other
	// lyricalargument in the linkedlist pointed
	// by funcarg beside the first lyricalargument
	// of the linkedlist which is the input-output
	// argument.
	
	lyricalargument* arg = funcarg->next;
	
	while (arg != funcarg) {
		// Note that if arg->v == funcarg->v,
		// the lyricalvariable pointed by arg->v
		// is always a tempvar that was created
		// by propagatevarchange().
		if (arg->v == funcarg->v) return 1;
		
		arg = arg->next;
	}
	
	// I get here if I couldn't find a match.
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

// There can never be an argument
// with its field byref set when
// used with a native operator.
// For all native operator which produce
// a result variable, the type of
// the result variable is the type
// of the first argument. Except for
// comparison and "not" native operators
// which produce a result variable having
// a type uint; the reason for choosing
// a type uint for the result variable
// is because it generate faster code since
// there can never be instructions generated
// to sign or zero extend it.

if (compilepass) {
	// I get here in the secondpass.
	// Instructions are created only in the secondpass.
	if (compileargcompileflag&LYRICALCOMPILECOMMENT)
		comment(stringduplicate2("begin: native operation"));
}

switch(sfr.nativeop) {
	
	case 1: // I get here for the operators ++ and --
		#include "incdec.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 2: // I get here for the operator -
		#include "neg.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 3: // I get here for the operators ! and ?
		// The type of the result variable is uint;
		// The reason for choosing a uint for
		// the result variable is because it generate
		// faster code since there can never be
		// instructions generated to keep it
		// within boundary or to sign extend it.
		#include "notistrue.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 4: // I get here for the operator ~
		#include "bitwisenot.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 5: // I get here for the operators << and >>
		#include "leftrightshift.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 6: // I get here for the operators * / %
		#include "muldiv.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 7: // I get here for the operators + -
		#include "plusminus.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 8: // I get here for the operators & ^ |
		#include "bitwiseandxoror.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 9: // I get here for the operators < <= > >= == !=
		// The type of the result variable is uint;
		// The reason for choosing a uint for the result variable
		// is because it generate faster code since there can
		// never be instructions generated to keep it within
		// boundary or to zero or sign extend it.
		#include "comparison.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 10: // I get here for the operator =
		#include "assign.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 11: // I get here for the operators <<= >>=
		#include "selfleftrightshift.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 12: // I get here for the operators *= /= %=
		#include "selfmuldiv.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 13: // I get here for the operators += -=
		#include "selfplusminus.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
		
	case 14: // I get here for the operators &= ^= |=
		#include "selfbitwiseandxoror.nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		break;
}

if (compilepass) {
	// I get here in the secondpass.
	// Instructions are created only in the secondpass.
	if (compileargcompileflag&LYRICALCOMPILECOMMENT)
		comment(stringduplicate2("end: done"));
}
