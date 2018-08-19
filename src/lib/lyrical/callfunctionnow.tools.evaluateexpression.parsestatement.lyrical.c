
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


typedef enum {
	// Used when calling a previously
	// declared function or operator or
	// when calling a function through
	// a pointer to function.
	CALLDEFAULT,
	
	// Used when forcing the calling
	// of a native operator.
	CALLNATIVEOP
	
} callfunctionnowflag;

// This function generate instructions to call
// a function or operator and return the resulting
// variable. EXPRWITHNORETVAL is returned if
// the function called did not produce a compileresult.
// This function never return null.
// When I am calling a function through a pointer
// to function, the argument varptrtofunc is expected
// to be set to a valid variable pointer to function
// and the arguments name is not used.
lyricalvariable* callfunctionnow (lyricalvariable* varptrtofunc, u8* name, callfunctionnowflag flag) {
	// This variable will only be set to a string
	// or portion of a string; so the string
	// it is set to must not be freed.
	string typeofresultvar;
	
	// This includes the file containing
	// functions used within this function.
	#include "tools.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
	
	searchfuncresult sfr = {0, 0, 0};
	
	// Will contain the string used with searchfunc()
	// to determine which function to call.
	// It is also used to generate the error message
	// when I couldn't find a matching function to call.
	string callsignature;
	
	// Variable which will be set to 1
	// if a method call can be done.
	uint domethodcall = !!funcarg;
	
	if (varptrtofunc) {
		
		if (!compilepass) {
			// currentfunc must become a stackframe holder
			// otherwise the function that it is calling
			// through a pointer could corrupt the stack
			// because the stack pointer register may not
			// point to the top of the stack, as it could
			// have been backtracked to the stackframe
			// holding the tiny-stackframe of currentfunc
			// if it was not a stackframe holder.
			currentfunc->couldnotgetastackframeholder = 1;
			
			// The lyricalcachedstackframe linkedlist
			// is built in the firstpass.
			// If the lyricalvariable pointed by varptrtofunc
			// is external to the lyricalfunction pointed by currentfunc,
			// I create a lyricalcachedstackframe for it.
			if (varptrtofunc != &thisvar &&
				varptrtofunc != &returnvar &&
				varptrtofunc->funcowner != rootfunc &&
				varptrtofunc->funcowner != currentfunc)
				cachestackframe(currentfunc, varfunclevel(varptrtofunc));
		}
		
		// The call to parsetypeofptrtofunc() will set
		// typeofresultvar and push a null argument if
		// the cast or type of the lyricalvariable pointed
		// by varptrtofunc represent a variadic function.
		parsetypeofptrtofunc();
		
	} else {
		// Generate the function call signature.
		callsignature = generatecallsignature(name);
		
		// I use the call signature generated above
		// to search for the function to call.
		if (flag == CALLNATIVEOP) sfr.nativeop = searchnativeop(callsignature);
		else if (flag == CALLDEFAULT) {
			
			lyricalargument* firstarg;
			
			lyricaltype* t;
			
			// If a method call can be done,
			// I retrieve the object type, which
			// will later be used to search
			// inheritance through basetype.
			if (domethodcall) {
				// When calling a function as a method, the first
				// argument of the function to call is the object.
				// funcarg point to the last pushed argument.
				// funcarg->next will give me the address of
				// the first pushed argument.
				
				string s = (firstarg = funcarg->next)->typepushed;
				
				t = searchtype(s.ptr, stringmmsz(s), INCURRENTANDPARENTFUNCTIONS);
				
			} else t = 0;
			
			string s = stringduplicate1(callsignature);
			
			while (1) {
				
				sfr = searchfunc(s, INCURRENTANDPARENTFUNCTIONS);
				
				mmrefdown(s.ptr);
				
				if (sfr.f) {
					// If the function is a variadic function
					// I add an additional argument which is
					// a null pointer which will mark
					// the end of the variadic arguments.
					if (sfr.f->isvariadic) {
						// Note that curpos has been set at '(' .
						// It will be used by pushargument() when setting
						// the argument lyricalargumentflag.id .
						pushargument(getvarnumber(0, largeenoughunsignednativetype(sizeofgpr)));
					}
					
					typeofresultvar = stringduplicate1(sfr.f->type);
					
					// Note that the field type of a lyricalfunction
					// is only set in the firstpass.
					if (!compilepass) {
						// In the firstpass, the linkedlist
						// of lyricalcachedstackframe is built.
						// Within generatefunctioncall() when generating
						// instructions calling a function, the address
						// of its parent function stackframe is needed
						// by currentfunc; hence the reason why I create
						// a lyricalcachedstackframe for it.
						if (sfr.funclevel) cachestackframe(currentfunc, sfr.funclevel);
						
						lyricalcalledfunction* calledfunction = registercalledfunction(sfr.f);
						
						// I check whether I am doing a recursive call.
						// This check is done only in the firstpass.
						if (sfr.funclevel) {
							
							lyricalfunction* f = currentfunc->parent;
							
							uint i = sfr.funclevel -1;
							
							while (i) {
								
								f = f->parent;
								
								--i;
							}
							
							if (f == sfr.f) {
								
								sfr.f->isrecursive = 1;
								
								// Decrement sfr.f->wasused
								// since the function
								// was using itself.
								--sfr.f->wasused;
								// I also decrement calledfunction->count
								// for the associated lyricalcalledfunction
								// created in currentfunc.
								--calledfunction->count;
							}
						}
					}
					
				} else if (t && (t = t->basetype)) {
					
					mmrefdown(firstarg->typepushed.ptr);
					
					firstarg->typepushed = stringduplicate1(t->name);
					
					s = generatecallsignature(name);
					
					continue;
				}
				
				break;
			}
			
		} else throwerror(stringfmt("internal error: %s: invalid flag", __FUNCTION__).ptr);
	}
	
	// When calling this function, funcarg always
	// point to the last pushed argument;
	// I set funcarg to the first pushed argument.
	if (funcarg) funcarg = funcarg->next;
	
	lyricalvariable* resultvar;
	
	if (sfr.nativeop) {
		// I get here to process calls to native operators.
		
		mmrefdown(callsignature.ptr);
		
		#include "nativeoperator.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
		
		freefuncarg();
		
		// The appropriate result variable is set
		// by the routines within the file included above.
		return resultvar;
		
	} else {
		// I get here for a defined operator or
		// function to call or a function to call
		// through a pointer to function.
		
		if (varptrtofunc) {
			// Only in the secondpass that
			// propagatevarschangedbyfunc() can
			// be called with its argument null.
			if (compilepass) propagatevarschangedbyfunc((lyricalfunction*)0);
			
		} else {
			// I check if there was any function found.
			if (!sfr.f) {
				
				string s = stringduplicate2("nothing defined to handle the call \"");
				
				// Here in the call signature, the characters '|'
				// which separate the function or operator name and
				// its arguments are replaced by the commas, opening
				// and closing paranthesis appropriately, in order
				// to display a suitable human readable error.
				
				callsignature.ptr[stringmmsz(callsignature)-1] = ')';
				*stringsearchright2(callsignature, "|") = '('; // The very first '|' is for the opening paranthesis.
				
				// Any other '|' are for the commas.
				u8* c; while (c = stringsearchright2(callsignature, "|")) *c = ',';
				
				stringappend1(&s, callsignature);
				stringappend2(&s, "\"");
				
				// The string pointed by s.ptr will be freed by
				// mmsessionfree() upon returning to lyricalcompile();
				throwerror(s.ptr);
			}
			
			mmrefdown(callsignature.ptr);
			
			propagatevarschangedbyfunc(sfr.f);
		}
		
		// When calling a function within
		// an asm block, the reserved
		// registers must be saved and
		// restored after the function
		// return, because their value
		// can be modified within
		// the function called.
		
		// Variable that will point to the lyricalvariable
		// used to save the reserved register values.
		lyricalvariable* varforsavedreservedgpr;
		
		// Array that will be used to gather
		// the list of reserved registers.
		arrayuint arrayofreservedgpr;
		
		if (compilepass) {
			
			arrayofreservedgpr = arrayuintnull;
			
			lyricalreg* r = currentfunc->gpr;
			// Gather the list of reserved registers.
			do {
				if (r->reserved) *arrayuintappend1(&arrayofreservedgpr) = (uint)r;
				
				r = r->next;
				
			} while (r != currentfunc->gpr);
			
			// I save the value of reserved
			// registers if there was any.
			if (arrayofreservedgpr.ptr) {
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT)
					comment(stringduplicate2("begin: saving reserved registers"));
				
				uint arrayofreservedgprsz = arrayuintsz(arrayofreservedgpr);
				
				// Here I create a variable to save
				// the reserved register values.
				varforsavedreservedgpr = varalloc(arrayofreservedgprsz * sizeofgpr, LOOKFORAHOLE);
				
				// I allocate the register
				// which will hold the address
				// of the variable to which
				// reserved register values
				// will be saved.
				// I lock the register because generateloadinstr()
				// could make use of functions which use allocreg()
				// and if the register is not locked, allocreg() could
				// use it while trying to allocate a new register.
				// I also lock it, otherwise it will
				// be seen as an unused register
				// when generating lyricalinstruction.
				(r = allocreg(CRITICALREG))->lock = 1;
				
				// I load the address of the variable
				// to which reserved register values
				// will be saved.
				// Note that generateloadinstr() only work with variables
				// which reside in the stack or global variable region.
				// Since the lyricalvariable pointed by varforsavedreservedgpr
				// reside there, I am perfectly fine to use generateloadinstr().
				generateloadinstr(r, varforsavedreservedgpr, 0, 0, LOADADDR);
				
				// Loop saving all reserved registers.
				do {
					--arrayofreservedgprsz;
					
					st((lyricalreg*)arrayofreservedgpr.ptr[arrayofreservedgprsz], r, arrayofreservedgprsz*sizeofgpr);
					
				} while (arrayofreservedgprsz);
				
				r->lock = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					// The lyricalreg pointed by r is not allocated
					// to a lyricalvariable but since I am done using it,
					// I should also produce a comment about it having been
					// discarded to complement the allocation comment that
					// was generated when it was allocated.
					comment(stringfmt("reg %%%d discarded", r->id));
					
					comment(stringduplicate2("end: done"));
				}
			}
		}
		
		// This function perform the actual function call.
		void docall () {
			// This include the file containing
			// the function used to generate
			// function call instructions.
			#include "generatefunctioncall.callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"
			
			string savedcast;
			u64 savedbitselect;
			
			// resultvar->cast and resultvar->bitselect
			// are saved so to restore them if they changed.
			if (resultvar != EXPRWITHNORETVAL) {
				
				savedcast = stringduplicate1(resultvar->cast);
				
				savedbitselect = resultvar->bitselect;
			}
			
			generatefunctioncall();
			
			// resultvar->cast get restored if it changed.
			if (resultvar != EXPRWITHNORETVAL) {
				
				if (resultvar->cast.ptr) mmrefdown(resultvar->cast.ptr);
				
				resultvar->cast = savedcast;
				
				resultvar->bitselect = savedbitselect;
			}
			
			if (varptrtofunc) {
				// If I have a tempvar or a lyricalvariable
				// which depend on a tempvar (dereference variable
				// or variable with its name suffixed with an offset),
				// I should free all those variables because
				// I will no longer need them and it allow
				// the generated code to save stack memory.
				varfreetempvarrelated(varptrtofunc);
			}
		}
		
		// I check if the function to call return a variable.
		if (!stringiseq2(typeofresultvar, "void")) {
			// Here I create a tempvar for the result variable.
			resultvar = varalloc(sizeoftype(typeofresultvar.ptr, stringmmsz(typeofresultvar)), LOOKFORAHOLE);
			resultvar->name = generatetempvarname(resultvar);
			
			// Set the type of the result variable.
			// I make sure to duplicate the string
			// in typeofresultvar because it is set
			// to a string which belong somewhere else.
			resultvar->type = stringduplicate1(typeofresultvar);
			
			docall();
			
			goto donecallingfunction;
		}
		
		// I get here if the function or operator
		// to call do not return a variable.
		// I set resultvar to the first argument otherwise
		// I set resultvar to EXPRWITHNORETVAL
		if (domethodcall) {
			
			resultvar = funcarg->varpushed;
			
			// The field bitselect of the variable to
			// return is restored since it was unset from
			// the variable when used with pushargument().
			resultvar->bitselect = funcarg->bitselect;
			
			// The cast of the variable to return
			// is restored since it was cleared from
			// the variable when used with pushargument().
			if (resultvar->cast.ptr) mmrefdown(resultvar->cast.ptr);
			resultvar->cast = stringduplicate1(funcarg->typepushed);
			
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
			
		} else resultvar = EXPRWITHNORETVAL;
		
		docall();
		
		donecallingfunction:
		
		if (funcarg) freefuncarg();
		
		if (compilepass) {
			// I restore reserved registers
			// if there was any saved.
			if (arrayofreservedgpr.ptr) {
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT)
					comment(stringduplicate2("begin: restoring reserved registers"));
				
				lyricalreg* r;
				// I allocate the register
				// which will hold the address
				// of the variable from which
				// reserved register values
				// will be restored.
				// I lock the register because generateloadinstr()
				// could make use of functions which use allocreg()
				// and if the register is not locked, allocreg() could
				// use it while trying to allocate a new register.
				// I also lock it, otherwise it will
				// be seen as an unused register
				// when generating lyricalinstruction.
				(r = allocreg(CRITICALREG))->lock = 1;
				
				// I load the address of the variable
				// from which reserved register values
				// will be restored.
				// Note that generateloadinstr() only work with variables
				// which reside in the stack or global variable region.
				// Since the lyricalvariable pointed by varforsavedreservedgpr
				// reside there, I am perfectly fine to use generateloadinstr().
				generateloadinstr(r, varforsavedreservedgpr, 0, 0, LOADADDR);
				
				uint arrayofreservedgprsz = arrayuintsz(arrayofreservedgpr);
				
				// Loop restoring all reserved registers.
				do {
					--arrayofreservedgprsz;
					
					ld((lyricalreg*)arrayofreservedgpr.ptr[arrayofreservedgprsz], r, arrayofreservedgprsz*sizeofgpr);
					
				} while (arrayofreservedgprsz);
				
				r->lock = 0;
				
				mmrefdown(arrayofreservedgpr.ptr);
				
				varfree(varforsavedreservedgpr);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					// The lyricalreg pointed by r is not allocated
					// to a lyricalvariable but since I am done using it,
					// I should also produce a comment about it having been
					// discarded to complement the allocation comment that
					// was generated when it was allocated.
					comment(stringfmt("reg %%%d discarded", r->id));
					
					comment(stringduplicate2("end: done"));
				}
			}
		}
		
		mmrefdown(typeofresultvar.ptr);
		
		return resultvar;
	}
}
