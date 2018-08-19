
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


if (funcarg->v->isnumber) {
	// If I get here, the argument is a number
	// which should translate to an immediate value
	// when generating the instruction.
	// Note that constant variables cannot be bitselected
	// because they do not reside in memory.
	
	imm1 = ifnativetypedosignorzeroextend(funcarg->v->numbervalue, funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed));
	
	r1 = 0;
	
} else r1 = (lyricalreg*)-1;


if (funcarg->next->v->isnumber) {
	// If I get here, the argument is a number
	// which should translate to an immediate value
	// when generating the instruction.
	// Note that constant variables cannot be bitselected
	// because they do not reside in memory.
	
	imm2 = ifnativetypedosignorzeroextend(funcarg->next->v->numbervalue, funcarg->next->typepushed.ptr, stringmmsz(funcarg->next->typepushed));
	
	r2 = 0;
	
} else r2 = (lyricalreg*)-1;


// If the first and second arguments are numbers,
// the return variable should be a number.
if (!(r1 || r2)) {
	
	if (*name == '&') immresult = imm1 & imm2;
	else if (*name == '^') immresult = imm1 ^ imm2;
	else immresult = imm1 | imm2;
	
	// Note that getvarnumber() will duplicate
	// the string pointed by funcarg->typepushed.ptr
	// before using it, so I don't need to
	// give it a duplicated string.
	resultvar = getvarnumber(immresult, funcarg->typepushed);
	
	break;
}


// Instructions are not generated in the firstpass.
// So here, only the result variable is created.
if (!compilepass) {
	// I create the result variable.
	resultvar = varalloc(sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)), LOOKFORAHOLE);
	
	resultvar->name = generatetempvarname(resultvar);
	
	// Set the type of the result variable.
	resultvar->type = stringduplicate1(funcarg->typepushed);
	
	break;
}


if (r1) {
	// I use the type and bitselect with
	// which the variable was pushed.
	r1 = getregforvar(funcarg->v, 0, funcarg->typepushed, funcarg->bitselect, FORINPUT);
	
	// I lock the register to prevent
	// a call of allocreg() from using it.
	r1->lock = 1;
}


if (r2) {
	// I use the type and bitselect with
	// which the variable was pushed.
	r2 = getregforvar(funcarg->next->v, 0, funcarg->next->typepushed, funcarg->next->bitselect, FORINPUT);
	
	// I lock the register to prevent
	// a call of allocreg() from using it.
	r2->lock = 1;
}


// I compute the result value.

// I create the result variable.
resultvar = varalloc(sizeoftype(funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)), LOOKFORAHOLE);

resultvar->name = generatetempvarname(resultvar);

// Set the type of the result variable.
resultvar->type = stringduplicate1(funcarg->typepushed);

// The register obtained will be appropriately
// set dirty and setregtothebottom() will
// be called on it before getregforvar() return.
rresult = getregforvar(resultvar, 0, resultvar->type, 0, FOROUTPUT);

// I lock the register to prevent
// a call of allocreg() from using it.
rresult->lock = 1;

if (r1) {
	
	if (r2) {
		// I get here if both the first and
		// second argument are not immediate values.
		
		if (*name == '&') and(rresult, r1, r2);
		else if (*name == '^') xor(rresult, r1, r2);
		else or(rresult, r1, r2);
		
	} else {
		// I get here if the second argument is
		// an immediate value but the first argument is not.
		
		if (*name == '&') andi(rresult, r1, imm2);
		else if (*name == '^') xori(rresult, r1, imm2);
		else ori(rresult, r1, imm2);
	}
	
} else if (r2) {
	// I get here if the first argument is
	// an immediate value but the second argument is not.
	
	if (*name == '&') andi(rresult, r2, imm1);
	else if (*name == '^') xori(rresult, r2, imm1);
	else ori(rresult, r2, imm1);
}


// I unlock the registers that were allocated
// and locked for the operands.
// Locked registers must be unlocked only after
// the instructions using them have been generated;
// otherwise they could be lost when insureenoughunusedregisters()
// is called while creating a new lyricalinstruction.
if (r1) r1->lock = 0;
if (r2) r2->lock = 0;
rresult->lock = 0;
