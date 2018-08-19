
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


// If the only argument is a number,
// the return variable should be a number.
if (!r1) {
	
	if (*name == '!') immresult = !imm1;
	else immresult = !!imm1;
	
	resultvar = getvarnumber(immresult, largeenoughunsignednativetype(sizeofgpr));
	
	break;
}


// Instructions are not generated in the firstpass.
// So here, only the result variable is created.
if (!compilepass) {
	// I create the result variable.
	resultvar = varalloc(sizeofgpr, LOOKFORAHOLE);
	
	resultvar->name = generatetempvarname(resultvar);
	
	// Set the type of the result variable.
	resultvar->type = stringduplicate1(largeenoughunsignednativetype(sizeofgpr));
	
	break;
}


// I use the type and bitselect with
// which the variable was pushed.
r1 = getregforvar(funcarg->v, 0, funcarg->typepushed, funcarg->bitselect, FORINPUT);

// I lock the register to prevent
// a call of allocreg() from using it.
r1->lock = 1;


// I compute the result value.

// I create the result variable.
resultvar = varalloc(sizeofgpr, LOOKFORAHOLE);

resultvar->name = generatetempvarname(resultvar);

// Set the type of the result variable.
resultvar->type = stringduplicate1(largeenoughunsignednativetype(sizeofgpr));

// The register obtained will be appropriately
// set dirty and setregtothebottom() will
// be called on it before getregforvar() return.
rresult = getregforvar(resultvar, 0, largeenoughunsignednativetype(sizeofgpr), 0, FOROUTPUT);

// I lock the register to prevent
// a call of allocreg() from using it.
rresult->lock = 1;

if (*name == '!') sz(rresult, r1);
else snz(rresult, r1);


// I unlock the registers that were allocated
// and locked for the operands.
// Locked registers must be unlocked only after
// the instructions using them have been generated;
// otherwise they could be lost when insureenoughunusedregisters()
// is called while creating a new lyricalinstruction.
r1->lock = 0;
rresult->lock = 0;
