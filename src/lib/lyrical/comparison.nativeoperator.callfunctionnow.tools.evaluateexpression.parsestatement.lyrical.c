
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


// Comparison can be done between two pointers,
// or between a pointer and an integer, or between
// an integer and a pointer, or between two integers,
// or between two identical enum types.


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
	
	if (*name == '=') {
		// I get here for the operator ==;
		
		immresult = (imm1 == imm2);
		
	} else if (*name == '!') {
		// I get here for the operator !=;
		
		immresult = (imm1 != imm2);
		
	// The first argument determine whether
	// I will use an unsigned comparison, and
	// an unsigned comparison is done if the first
	// argument is a pointer or an unsigned native type.
	// I only need to check the first character of
	// the type string to determine whether I have
	// an unsigned native type because there will not
	// be any other type starting with 'u' after
	// I have tested for a pointer type which could
	// have had a name starting with 'u';
	// the fcall pattern insure that.
	} else if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
		funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')' ||
			*funcarg->typepushed.ptr == 'u') {
		
		if (*name == '<') {
			
			if (name[1] == '=') {
				// I get here for the operator <=;
				
				immresult = (imm1 <= imm2);
				
			} else {
				// I get here for the operator <;
				
				immresult = (imm1 < imm2);
			}
			
		} else {
			
			if (name[1] == '=') {
				// I get here for the operator >=;
				
				immresult = (imm1 >= imm2);
				
			} else {
				// I get here for the operator >;
				
				immresult = (imm1 > imm2);
			}
		}
		
	} else {
		
		if (*name == '<') {
			
			if (name[1] == '=') {
				// I get here for the operator <=;
				
				immresult = ((s64)imm1 <= (s64)imm2);
				
			} else {
				// I get here for the operator <;
				
				immresult = ((s64)imm1 < (s64)imm2);
			}
			
		} else {
			
			if (name[1] == '=') {
				// I get here for the operator >=;
				
				immresult = ((s64)imm1 >= (s64)imm2);
				
			} else {
				// I get here for the operator >;
				
				immresult = ((s64)imm1 > (s64)imm2);
			}
		}
	}
	
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

if (r1) {
	
	if (r2) {
		// I get here if both the first and
		// second argument are not immediate values.
		
		if (*name == '=') {
			// I get here for the operator ==;
			
			seq(rresult, r1, r2);
			
		} else if (*name == '!') {
			// I get here for the operator !=;
			
			sne(rresult, r1, r2);
			
		// The first argument determine whether
		// I will use an unsigned comparison, and
		// an unsigned comparison is done if the first
		// argument is a pointer or an unsigned native type.
		// I only need to check the first character of
		// the type string to determine whether I have
		// an unsigned native type because there will not
		// be any other type starting with 'u' after
		// I have tested for a pointer type which could
		// have had a name starting with 'u';
		// the fcall pattern insure that.
		} else if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
			funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')' ||
				*funcarg->typepushed.ptr == 'u') {
			
			if (*name == '<') {
				
				if (name[1] == '=') {
					// I get here for the operator <=;
					
					slteu(rresult, r1, r2);
					
				} else {
					// I get here for the operator <;
					
					sltu(rresult, r1, r2);
				}
				
			} else {
				
				if (name[1] == '=') {
					// I get here for the operator >=;
					
					slteu(rresult, r2, r1);
					
				} else {
					// I get here for the operator >;
					
					sltu(rresult, r2, r1);
				}
			}
			
		} else {
			
			if (*name == '<') {
				
				if (name[1] == '=') {
					// I get here for the operator <=;
					
					slte(rresult, r1, r2);
					
				} else {
					// I get here for the operator <;
					
					slt(rresult, r1, r2);
				}
				
			} else {
				
				if (name[1] == '=') {
					// I get here for the operator >=;
					
					slte(rresult, r2, r1);
					
				} else {
					// I get here for the operator >;
					
					slt(rresult, r2, r1);
				}
			}
		}
		
	} else {
		// I get here if the second argument is
		// an immediate value but the first argument is not.
		
		if (*name == '=') {
			// I get here for the operator ==;
			
			seqi(rresult, r1, imm2);
			
		} else if (*name == '!') {
			// I get here for the operator !=;
			
			snei(rresult, r1, imm2);
			
		// The first argument determine whether
		// I will use an unsigned comparison, and
		// an unsigned comparison is done if the first
		// argument is a pointer or an unsigned native type.
		// I only need to check the first character of
		// the type string to determine whether I have
		// an unsigned native type because there will not
		// be any other type starting with 'u' after
		// I have tested for a pointer type which could
		// have had a name starting with 'u';
		// the fcall pattern insure that.
		} else if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
			funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')' ||
				*funcarg->typepushed.ptr == 'u') {
			
			if (*name == '<') {
				
				if (name[1] == '=') {
					// I get here for the operator <=;
					
					slteui(rresult, r1, imm2);
					
				} else {
					// I get here for the operator <;
					
					sltui(rresult, r1, imm2);
				}
				
			} else {
				
				if (name[1] == '=') {
					// I get here for the operator >=;
					
					sgteui(rresult, r1, imm2);
					
				} else {
					// I get here for the operator >;
					
					sgtui(rresult, r1, imm2);
				}
			}
			
		} else {
			
			if (*name == '<') {
				
				if (name[1] == '=') {
					// I get here for the operator <=;
					
					sltei(rresult, r1, imm2);
					
				} else {
					// I get here for the operator <;
					
					slti(rresult, r1, imm2);
				}
				
			} else {
				
				if (name[1] == '=') {
					// I get here for the operator >=;
					
					sgtei(rresult, r1, imm2);
					
				} else {
					// I get here for the operator >;
					
					sgti(rresult, r1, imm2);
				}
			}
		}
	}
	
} else if (r2) {
	// I get here if the first argument is
	// an immediate value but the second argument is not.
	
	if (*name == '=') {
		// I get here for the operator ==;
		
		seqi(rresult, r2, imm1);
		
	} else if (*name == '!') {
		// I get here for the operator !=;
		
		snei(rresult, r2, imm1);
		
	// The first argument determine whether
	// I will use an unsigned comparison, and
	// an unsigned comparison is done if the first
	// argument is a pointer or an unsigned native type.
	// I only need to check the first character of
	// the type string to determine whether I have
	// an unsigned native type because there will not
	// be any other type starting with 'u' after
	// I have tested for a pointer type which could
	// have had a name starting with 'u';
	// the fcall pattern insure that.
	} else if (funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == '*' ||
		funcarg->typepushed.ptr[stringmmsz(funcarg->typepushed)-1] == ')' ||
			*funcarg->typepushed.ptr == 'u') {
		
		if (*name == '<') {
			
			if (name[1] == '=') {
				// I get here for the operator <=;
				
				sgteui(rresult, r2, imm1);
				
			} else {
				// I get here for the operator <;
				
				sgtui(rresult, r2, imm1);
			}
			
		} else {
			
			if (name[1] == '=') {
				// I get here for the operator >=;
				
				slteui(rresult, r2, imm1);
				
			} else {
				// I get here for the operator >;
				
				sltui(rresult, r2, imm1);
			}
		}
		
	} else {
		
		if (*name == '<') {
			
			if (name[1] == '=') {
				// I get here for the operator <=;
				
				sgtei(rresult, r2, imm1);
				
			} else {
				// I get here for the operator <;
				
				sgti(rresult, r2, imm1);
			}
			
		} else {
			
			if (name[1] == '=') {
				// I get here for the operator >=;
				
				sltei(rresult, r2, imm1);
				
			} else {
				// I get here for the operator >;
				
				slti(rresult, r2, imm1);
			}
		}
	}
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
