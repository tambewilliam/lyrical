
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


#include <stdtypes.h>
#include <macros.h>
#include <string.h>
#include <mm.h>
#include <pamsyn.h>
#include <arrayuint.h>
#include <file.h>
#include <byt.h>
#include <bintree.h>

#include "structures.lyrical.c"


// Assumption made by the compiler
// when generating instructions:
// - The global variable region and the stack (which
// contain all local variables), are all located
// in memory pages that are private to the address space
// of the thread running the program.
// In fact the need for the keyword volatile found
// in C/C++ was removed by always assuming that
// the memory pages used by the stack, global variable
// region and the ones from which the program is run
// are always private to threads of a process; and by
// taking advantage of the fact that variables for which
// the address was obtained as well as dereferenced variables,
// must always be treated as volatile to prevent aliasing.


// This function perform the compilation
// and return a lyricalcompileresult;
// if an error occur, the field rootfunc
// of the lyricalcompileresult returned is null.
lyricalcompileresult lyricalcompile (lyricalcompilearg* compilearg) {
	// Catchable-labels to be thrown for an error,
	// and for when a recompile is needed.
	__label__ labelforerror, labelforrecompile;
	
	lyricalcompileresult compileresult;
	
	uint sizeofgpr = compilearg->sizeofgpr;
	uint nbrofgpr = compilearg->nbrofgpr;
	
	// Function used to compute
	// the log2 ceiling of the
	// value given as argument.
	// When the argument is 0 or 1,
	// the value returned is 1.
	uint clog2 (uint a) {
		
		if (a > 1) {
			
			--a;
			
			uint n = 0;
			
			while (a) {
				a >>= 1;
				++n;
			}
			
			return n;
			
		} else return 1;
	}
	
	// This function return the powerof2 amount
	// of the number given as argument if it is
	// a powerof2, otherwise it return null.
	// Null is returned if the argument is null.
	u64 ispowerof2 (u64 n) {
		
		if (!n) return 0;
		
		// Will hold the result.
		u64 retvar = 0;
		
		while (!(n&1)) {
			
			n >>= 1;
			
			++retvar;
		}
		
		if (n == 1) return retvar;
		else return 0;
	}
	
	// The bytesize of gpr must be non-null,
	// a powerof2, and not larger than
	// the highest supported gpr bytesize.
	// The minimum count of gpr needed is 3.
	if (!ispowerof2(sizeofgpr) || (sizeofgpr > sizeof(u64))  || (nbrofgpr < 3)) {
		
		compileresult.rootfunc = 0;
		
		return compileresult;
	}
	
	uint bitsizeofgpr = 8*sizeofgpr;
	
	// Any change to the following constants
	// require any previously compiled code
	// to be recompiled.
	enum {
		// The compiler assume that a pagesize
		// is 4096 bytes when generating
		// instructions that manipulate
		// the stack, as well as when making
		// use of the instructions LYRICALOPPAGE*.
		PAGESIZE = 4096,
		
		// Maximum stackframe size used only
		// with the stackframe of a function
		// for which the address was obtained.
		// It must be less than or equal
		// to (PAGESIZE-sizeofgpr).
		MAXSTACKUSAGE = 1024,
		
		// The sum of MAXARGUSAGE and
		// MAXSTACKFRAMEPOINTERSCACHESIZE
		// described below must be much
		// less than MAXSTACKUSAGE since
		// space in the stack for local
		// variables is also needed.
		
		// Maximum usage size for arguments of a function.
		MAXARGUSAGE = 256,
		
		// Maximum size that the region used
		// to cache stackframe pointers can grow up to.
		// It limit the nesting depth to about
		// 256/sizeof(u32) == 64 for a 32bits target.
		MAXSTACKFRAMEPOINTERSCACHESIZE = 256,
		
		// I distinguish two types of stackframe:
		// regular and tiny stackframes; they are
		// described in doc/stackframe.txt;
		// Tiny stackframes reside inside regular stackframes.
		// Both the regular and tiny stackframe size
		// is limited by the maximum size of a stackpage
		// which is the value of PAGESIZE; but for
		// the stackframe of a function for which
		// the address was obtained, its size is
		// limited to the value of MAXSTACKUSAGE;
		// the limit MAXSTACKUSAGE is set in such
		// a way as to minimize the likelyhood that
		// a new page would need to be allocated
		// when the function is called, which
		// would make the function call slow.
	};
	
	// This structure represent a chunk of source code.
	typedef struct chunk {
		// This field is set if the chunk originated
		// from a macro that was created by the preprocessor
		// directive `define, or from a macro argument
		// of a function-like macro.
		// It contain a string such as either
		// of the two examples below:
		// from macro "MACRONAME"
		// argument "argname" of macro "MACRONAME"
		string origin;
		
		// Path to the file where
		// the chunk originated from.
		string path;
		
		// Offset of the chunk
		// first character
		// within the file.
		uint offset;
		
		// Line number where the chunk
		// start in the file.
		uint linenumber;
		
		// When this chunk is the last chunk
		// of a linkedlist of chunks
		// generated for an included file,
		// this field is set to the address
		// of the first chunk of that linkedlist,
		// otherwise it is set to null.
		struct chunk* first;
		
		union {
			// Content of the chunk of source code.
			string content;
			// Size in bytes of the above string content.
			// At some point the above string content
			// get freed and only its size is used instead.
			uint contentsz;
		};
		
		// The field prev of the first element of
		// the linkedlist point to the last element
		// of the linkedlist while the field next
		// of the last element of the linkedlist point
		// to the first element of the linkedlist.
		struct chunk* prev;
		struct chunk* next;
		
	} chunk;
	
	// This variable point to a linkedlist
	// of chunk used by throwerror()
	// to locate where an error occurred.
	chunk* chunks = 0;
	
	// The variable curpos is used to
	// go through the data to compile.
	u8* curpos;
	
	// I create a new session which will allow to regain
	// any allocated memory block if an error is thrown.
	mmsession compilesession1 = mmsessionnew();
	
	// The variable rootfunc point to the first
	// created lyricalfunction of a circular linkedlist
	// of lyricalfunction which represent all the functions parsed.
	// Its field prev will point to the last
	// created lyricalfunction and its field next will
	// point to the second created lyricalfunction.
	lyricalfunction* rootfunc;
	
	// Path to the file currently being preprocessed.
	// This variable is set within preprocessor.lyrical.c
	// and used by throwerror().
	string currentfilepath;
	
	// During preprocessing this variable get set
	// in turn to the file source to preprocess;
	// after preprocessing, this variable get set
	// to the preprocessed result.
	u8* compileargsource = compilearg->source;
	
	lyricalcompileflag compileargcompileflag =
		compilearg->compileflag;
	
	uint compileargstackpageallocprovision = compilearg->stackpageallocprovision;
	
	#define LYRICALCOMPILE
	// This file is also included within lyricalfree().
	#include "tools.lyrical.c"
	#undef LYRICALCOMPILE
	
	if ((uint)compileargsource < PAGESIZE) {
		// lyricalcompilearg.source must be
		// greater than PAGESIZE to insure
		// that the value of curpos which is used
		// to set lyricalvariable.id will not
		// conflict with the ids used with
		// predeclared variables.
		// It is also desirable for valid
		// addresses to be greater than PAGESIZE
		// in order to be similar in use as
		// addresses such as NULL which are
		// considered invalid.
		curpos = 0;
		throwerror(stringfmt("internal error: lyricalcompilearg.source must be greater than %d\n", PAGESIZE).ptr);
	} else if (!*compileargsource) {
		curpos = 0;
		throwerror("internal error: lyricalcompilearg.source null\n");
	}
	
	compileresult.srcfilepaths = stringnull;
	
	uint compileargfunctioncallargsguardspace =
		compilearg->functioncallargsguardspace;
	
	uint (*compilearginstallmissingmodule)(u8* modulename) =
		compilearg->installmissingmodule;
	
	u8* compilearglyxappend =
		compilearg->lyxappend;
	
	#include "preprocessor.lyrical.c"
	
	// Replace the last '\n'
	// so that the last path
	// be terminated by 0.
	if (compileresult.srcfilepaths.ptr) compileresult.srcfilepaths.ptr[stringmmsz(compileresult.srcfilepaths)-1] = 0;
	
	#include "constants.lyrical.c"
	
	// The register %0 is reserved
	// as the stack pointer.
	
	lyricalreg rstack;
	
	bytsetz(&rstack, sizeof(lyricalreg));
	
	rstack.id = 0;
	
	// %0 is the stack register which
	// will always hold an address.
	rstack.size = sizeofgpr;
	
	// Array that will be used to contain the id of
	// functions that could not get their stackframe
	// held by a stackframe holder function.
	arrayuint couldnotgetastackframeholder = {
		// I initially allocate a memory block of size 0
		// which will be resized as arrayuintappend1() is used.
		// I do so before the creation of the memory session
		// compilesession2 so that the memory block do not
		// get freed when a recompilation is needed and
		// mmsessionfree() is called on compilesession2.
		.ptr = mmalloc(0)
	};
	
	// I jump to this label when I need to do
	// a recompile starting over from the firstpass.
	labelforrecompile:;
	
	// I create a new session which will allow to regain
	// any allocated memory block if a recompilation is needed.
	mmsession compilesession2 = mmsessionnew();
	
	// Here I create the root function which will be
	// parent to all other functions in the source code
	// that I am compiling.
	rootfunc = mmallocz(sizeof(lyricalfunction));
	
	// lyricalfunction structures form a circular
	// linkedlist using their fields prev and next.
	rootfunc->next = rootfunc->prev = rootfunc;
	
	//rootfunc->parent = rootfunc->sibling = 0; // Not needed because mmallocz() was used.
	
	// This variable is set later after completion of
	// the firstpass and is used within funcdeclaration()
	// during the secondpass in order to link functions
	// created in the secondpass to their equivalent
	// function created in the firstpass.
	// I declare it before including parsestatement.lyrical.c
	// so that it can be seen within funcdeclaration()
	// where it is used.
	lyricalfunction* firstpassrootfunc;
	
	// This function will add a new lyricalcachedstackframe to
	// the linkedlist pointed by the field cachedstackframes of
	// the lyricalfunction given as argument if a lyricalcachedstackframe
	// was not already added for the level value given as argument.
	// The lyricalcachedstackframe elements in the linkedlist pointed by
	// the field cachedstackframes of the lyricalfunction given as argument
	// will be ordered from the lowest level value to the highest level value
	// so that cachestackframepointers() re-use already loaded stackframe
	// pointers while going through all stackframe used by a function.
	// This function will be used only in the firstpass.
	// The argument level will always be non-null.
	void cachestackframe (lyricalfunction* f, uint level) {
		
		lyricalcachedstackframe* s = f->cachedstackframes;
		
		if (s) {
			
			lyricalcachedstackframe* ss;
			
			if (s->level < level) {
				
				while (1) {
					
					lyricalcachedstackframe* snext = s->next;
					
					if (snext) {
						
						s = snext;
						
						if (s->level < level) continue;
					}
					
					break;
				}
				
				if (s->level == level) return;
				
				ss = mmalloc(sizeof(lyricalcachedstackframe));
				
				ss->level = level;
				
				ss->next = s->next;
				
				s->next = ss;
				
				return;
			}
			
			if (s->level == level) return;
			
			ss = mmalloc(sizeof(lyricalcachedstackframe));
			
			ss->level = level;
			
			ss->next = s;
			
			f->cachedstackframes = ss;
			
			return;
		}
		
		s = mmalloc(sizeof(lyricalcachedstackframe));
		
		s->level = level;
		
		s->next = 0;
		
		f->cachedstackframes = s;
	}
	
	// Structure representing a string constant.
	typedef struct stringconstant {
		// String value to use with
		// the string constant.
		string value;
		
		// Pointer to the next element
		// of the linkedlist.
		// This field is null for the last
		// element of the linkedlist.
		struct stringconstant* next;
		
	} stringconstant;
	
	// Pointer to the linkedlist of all stringconstant.
	// This variable point to the first stringconstant
	// of the linkedlist.
	stringconstant* stringconstants = 0;
	
	// Total size that will be needed
	// by the string region.
	uint stringregionsz = 0;
	
	// In the firstpass I determine which argument will
	// be passed byref during a function call; I also
	// determine which argument will be used as output,
	// because such argument should never be duplicated
	// when pushed since they get set without
	// their value getting used; I also determine
	// which stackframe should be cached, and
	// which functions will be stackframe holders,
	// in other words, which function will use
	// a tiny or regular stackframe (see doc/stackframe.txt).
	// gpr are not used in the firstpass and are
	// created only in the secondpass.
	// Labels and catchable-labels are created
	// and used only in the secondpass.
	uint compilepass = 0;
	
	// This variable represent the "this" variable
	// "this" is accessed by using its address written
	// in the stackframe of the current function.
	lyricalvariable thisvar;
	bytsetz(&thisvar, sizeof(lyricalvariable));
	thisvar.name = stringduplicate2("this");
	thisvar.type = stringduplicate1(voidptrstr);
	thisvar.size = sizeofgpr;
	thisvar.alwaysvolatile = &thisvar.isalwaysvolatile;
	// Here I initialize the variable
	// pointed by the field alwaysvolatile.
	// When compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE is true,
	// all variables are made volatile, hence
	// their value are never cached in registers.
	// This is useful when debugging because
	// it is easier for a debugger to just read
	// memory in order to retrieve the value
	// of a variable instead of having to write
	// code in order to keep track of register
	// utilization per variable for each instruction.
	if (compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE)
		*thisvar.alwaysvolatile = 1;
	else *thisvar.alwaysvolatile = 0;
	
	// This variable represent the return variable
	// of a function and which is accessed by
	// the programmer when using the symbol retvar.
	// retvar is accessed by using its address written
	// in the stackframe of the current function.
	lyricalvariable returnvar;
	bytsetz(&returnvar, sizeof(lyricalvariable));
	returnvar.name = stringduplicate2("retvar");
	// Instead of (uint)curpos, 4 is used
	// because "retvar" do not have a corresponding
	// curpos location where it is declared.
	returnvar.id = 4;
	returnvar.alwaysvolatile = &returnvar.isalwaysvolatile;
	// Here I initialize the variable
	// pointed by the field alwaysvolatile.
	// When compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE is true,
	// all variables are made volatile, hence
	// their value are never cached in registers.
	// This is useful when debugging because
	// it is easier for a debugger to just read
	// memory in order to retrieve the value
	// of a variable instead of having to write
	// code in order to keep track of register
	// utilization per variable for each instruction.
	if (compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE) *returnvar.alwaysvolatile = 1;
	else *returnvar.alwaysvolatile = 0;
	
	#include "parsestatement.lyrical.c"
	
	curpos = compileargsource;
	
	rootfunc->startofdeclaration = curpos;
	
	skipspace();
	
	parsestatement(PARSEFUNCTIONBODY);
	
	void reviewdatafromfirstpass () {
		// The root function can have variables
		// created in its field vlocal field.
		if (rootfunc->vlocal) freevarlinkedlist(rootfunc->vlocal);
		
		// The linkedlist of lyricaltype has already been freed within parsestatement().
		//if (rootfunc->t) freetypelinkedlist(rootfunc->t);
		
		// Instructions are not generated in the firstpass
		// hence there is no need to check rootfunc->i .
		
		// The linkedlist pointed by the field pushedargflags of the root function
		// should not be freed, since it is used in the secondpass.
		
		lyricalfunction* f = rootfunc;
		
		while ((f = f->next) != rootfunc) {
			
			if (f->scope) mmrefdown(f->scope);
			// Every functions beside the root function,
			// has its fields name, type, typeofaddr, fcall set;
			// so there is no need to check whether they are null.
			mmrefdown(f->name.ptr);
			mmrefdown(f->type.ptr);
			mmrefdown(f->typeofaddr.ptr);
			pamsynfree(f->fcall);
			mmrefdown(f->callsignature.ptr);
			
			// The linkedlist pointed by the field
			// pushedargflags should not be freed,
			// since it is used in the secondpass.
			
			if (f->varg) freevarlinkedlist(f->varg);
			if (f->vlocal) freevarlinkedlist(f->vlocal);
			
			// Instructions are not generated in the firstpass
			// hence there is no need to check f->i .
			
			// The linkedlist of lyricaltype has
			// already been freed within parsestatement().
			//if (f->t) freetypelinkedlist(f->t);
			
			// The linkedlists pointed by the fields l
			// and cl are set and used only in the secondpass,
			// because labels and catchable-labels are
			// created and used only in the secondpass.
			
			// This function is called at the end of the firstpass
			// to go through the linkedlist of propagation elements
			// of a function and recursively resolve the propagation
			// elements of type FUNCTIONTOPROPAGATE to propagation
			// elements of type VARIABLETOPROPAGATE.
			// When this function is called, the argument f must be
			// the same as the argument initialfunc; it allow for
			// determining whether a recursive call of
			// this function is being done.
			void resolvepropagations (lyricalfunction* f, lyricalfunction* initialfunc) {
				
				lyricalpropagation* p = f->p;
				
				while (p) {
					
					if (p->type == FUNCTIONTOPROPAGATE) {
						// If I get here, I have a propagation element
						// of type FUNCTIONTOPROPAGATE and I recursively
						// call resolvepropagations() to copy all propagation
						// elements of type VARIABLETOPROPAGATE of
						// the lyricalfunction pointed by p->f to
						// the linkedlist of propagation elements of
						// the lyricalfunction pointed by initialfunc.
						
						// p->f is null if the propagation element
						// was already processed.
						if (p->f) resolvepropagations(p->f, initialfunc);
						
						// Once a propagation element of type FUNCTIONTOPROPAGATE
						// has been resolved, it is left in its linkedlist; in
						// the secondpass, propagatevarschangedbyfunc() skip it.
						
					} else if (f != initialfunc) {
						// I get here if I have a propagation element
						// of type VARIABLETOPROPAGATE and I am currently
						// recursing to resolve a propagation element of
						// type FUNCTIONTOPROPAGATE.
						// initialfunc hold the address of the lyricalfunction
						// where the propagation element of type
						// VARIABLETOPROPAGATE is to be copied to; so here
						// I copy the propagation element pointed by p
						// to the linkedlist of propagations
						// pointed by initialfunc->p.
						
						// This function will search the linkedlist
						// of propagation elements pointed by initialfunc->p
						// and return 1 if a propagation element similar
						// to the one pointed by p do not exist
						// in that linkedlist.
						uint isnotinthelinkedlist () {
							
							lyricalpropagation* psearch = initialfunc->p;
							
							while (psearch) {
								
								if (psearch->type == VARIABLETOPROPAGATE &&
									psearch->id == p->id &&
									psearch->offset == p->offset &&
									psearch->size == p->size) return 0;
								
								psearch = psearch->next;
							}
							
							return 1;
						}
						
						// If the propagation element of type VARIABLETOPROPAGATE
						// pointed by p is not for a variable that is local to
						// the lyricalfunction pointed by initialfunc and has not
						// yet been added to the linkedlist of propagation elements
						// pointed by initialfunc->p, then it is added.
						// Note that the propagation element will be added at the top
						// of the linkedlist and will not conflict with the fact that
						// I am at the same time going through that linkedlist
						// to resolve propagation element of type FUNCTIONTOPROPAGATE;
						// because I will be going down that linkedlist until I hit
						// the propagation element having its field next null.
						if (p->f != initialfunc && isnotinthelinkedlist()) {
							// If I get here, I create a new propagation element
							// in the linkedlist pointed by initialfunc->p.
							
							lyricalpropagation* pnew = mmalloc(sizeof(lyricalpropagation));
							
							pnew->type = VARIABLETOPROPAGATE;
							
							pnew->f = p->f;
							pnew->id = p->id;
							pnew->offset = p->offset;
							pnew->size = p->size;
							
							pnew->next = initialfunc->p;
							
							initialfunc->p = pnew;
						}
					}
					
					p = p->next;
				}
				
				// If the address of the function pointed by initialfunc
				// was obtained, all its propagation elements are added
				// to the linkedlist pointed by rootfunc->p; that linkedlist
				// will be used when a function is called through a pointer
				// to function in order to propagate all variables that were
				// modified by functions for which the address was obtained;
				// since a function called through a pointer to function could
				// be any one of the function for which the address was obtained.
				// Populating the linkedlist pointed by rootfunc->p is done
				// only when f == initialfunc, otherwise it would be done
				// while in the recursion of resolvepropagations() and
				// the linkedlist of propagation elements pointed by
				// initialfunc->p would not have been completely populated.
				if (initialfunc == f && initialfunc->itspointerisobtained) {
					
					p = initialfunc->p;
					
					while (p) {
						// This function will search the linkedlist
						// of propagation elements pointed by rootfunc->p
						// and return 1 if a propagation element similar
						// to the one pointed by p do not exist
						// in that linkedlist.
						uint isnotinthelinkedlist () {
							
							lyricalpropagation* psearch = rootfunc->p;
							
							while (psearch) {
								// Propagation elements in the linkedlist
								// pointed by rootfunc->p will always be of type
								// VARIABLETOPROPAGATE; so there is no need to test
								// whether psearch->type == VARIABLETOPROPAGATE
								if (psearch->id == p->id &&
									psearch->offset == p->offset &&
									psearch->size == p->size) return 0;
								
								psearch = psearch->next;
							}
							
							return 1;
						}
						
						// If the propagation element pointed by p is
						// of type VARIABLETOPROPAGATE and has not yet been
						// added to the linkedlist of propagation elements
						// pointed by rootfunc->p, then it is added.
						if (p->type == VARIABLETOPROPAGATE && isnotinthelinkedlist()) {
							// If I get here, I create a new propagation element
							// in the linkedlist pointed by rootfunc->p.
							
							lyricalpropagation* pnew = mmalloc(sizeof(lyricalpropagation));
							
							pnew->type = VARIABLETOPROPAGATE;
							
							pnew->f = p->f;
							pnew->id = p->id;
							pnew->offset = p->offset;
							pnew->size = p->size;
							
							pnew->next = rootfunc->p;
							
							rootfunc->p = pnew;
						}
						
						p = p->next;
					}
				}
			}
			
			// The linkedlist of propagation elements
			// pointed by f->p is kept because it is used
			// in the secondpass through f->firstpass;
			// I instead resolve all propagation elements
			// of type FUNCTIONTOPROPAGATE of
			// the lyricalfunction pointed by f.
			resolvepropagations(f, f);
			
			// If the function was not set recursive,
			// check whether it is recursive because it called
			// a sibling function that called it back again.
			if (!f->isrecursive) {
				// Note that the linkedlist also contain
				// calledfunctions made by subfunctions.
				lyricalcalledfunction* calledfunction1 = f->calledfunctions;
				
				while (calledfunction1) {
					// I check whether the called function
					// is a sibling of the function for which
					// the lyricalfunction is pointed by f.
					if (calledfunction1->f->parent == f->parent) {
						// Note that the linkedlist also contain
						// calledfunctions made by subfunctions.
						lyricalcalledfunction* calledfunction2 = calledfunction1->f->calledfunctions;
						
						while (calledfunction2) {
							
							if (calledfunction2->f == f) {
								
								f->isrecursive = 1;
								
								// Decrement f->wasused
								// by the number of times
								// a call to the function
								// was made; since the function
								// was using itself through
								// another function.
								f->wasused -= calledfunction2->count;
								// I also set calledfunction2->count null,
								// since its value was substracted above.
								calledfunction2->count = 0;
								
								break;
							}
							
							calledfunction2 = calledfunction2->next;
						}
					}
					
					calledfunction1 = calledfunction1->next;
				}
			}
		}
		
		// When I get here, f == rootfunc;
		
		while ((f = f->next) != rootfunc) {
			// Variable set to 1,
			// if a redo from
			// rootfunc->next
			// need to be done.
			uint redo = 0;
			
			// I free functions that
			// were found to never be used,
			// along with their children;
			// as they should no longer
			// be taken into account.
			if (!f->wasused) {
				// Free ressources allocated to a function.
				void freefunction (lyricalfunction* df) {
					// This function is used to free
					// a linkedlist of lyricalcalledfunction,
					// decrementing the field wasused
					// of each associated lyricalfunction.
					// The linkedlist should be valid because
					// there is no check on whether it is valid.
					void freecalledfunctionlinkedlist (lyricalcalledfunction* linkedlist) {
						
						do {
							// If another function is found
							// to never be used (ei: its field
							// wasused becoming null), then I need
							// to restart from the first lyricalfunction
							// of the linkedlist pointed by rootfunc
							// in order to also go through the children
							// of the function that was just
							// found to never be used.
							if (!(linkedlist->f->wasused -= linkedlist->count)) redo = 1;
							
							lyricalcalledfunction* savedptr = linkedlist;
							
							linkedlist = linkedlist->next;
							
							mmrefdown(savedptr);
							
						} while (linkedlist);
					}
					
					if (df->calledfunctions) freecalledfunctionlinkedlist(df->calledfunctions);
					
					// When the field wasused is
					// non-null, the lyricalfunction
					// is still being referenced
					// by a lyricalcalledfunction
					// and must not yet be freed.
					if (df->wasused) df->calledfunctions = 0;
					else {
						// I detach the lyricalfunction
						// from the circular linkedlist
						// of lyricalfunction.
						df->prev->next = df->next;
						df->next->prev = df->prev;
						
						if (df->p) freepropagationlinkedlist(df->p);
						
						if (df->pushedargflags) freeargumentflaglinkedlist(df->pushedargflags);
						
						if (df->cachedstackframes) freestackframelinkedlist(df->cachedstackframes);
						
						// df->sharedregions is certainly null,
						// because it has not yet been set.
						
						mmrefdown(df);
					}
				}
				
				// Free ressources allocated to children functions.
				void freechildfunctions (lyricalfunction* cf) {
					
					if (cf->children) freechildfunctions(cf->children);
					
					lyricalfunction* nextcf;
					
					do {
						nextcf = cf->sibling;
						
						freefunction(cf);
						
					} while (cf = nextcf);
				}
				
				if (f->children) freechildfunctions(f->children);
				
				f = f->prev;
				
				freefunction(f->next);
				
				if (redo) f = rootfunc;
				
			// The field sibling of
			// firstpass lyricalfunction
			// should no longer be used;
			// and to be sure to catch
			// any of its use by mistake,
			// I set it to null.
			} else f->sibling = 0;
		}
		
		// This function is used to check whether
		// the function associated with the lyricalfunction
		// given as argument is a stackframe holder.
		// A function that is a strackframe holder
		// has to create its own stackframe because
		// it cannot use another function stackframe
		// to hold its arguments and local variables.
		uint isstackframeholder (lyricalfunction* f) {
			return (f == rootfunc) || f->isrecursive || f->itspointerisobtained || f->couldnotgetastackframeholder;
		}
		
		// When I get here, f == rootfunc;
		
		while ((f = f->next) != rootfunc) {
			// If a function that is a stackframe holder
			// call a function that is not its child
			// and is not a stackframe holder, that
			// called function must become a function
			// that is a stackframe holder if it call
			// a function that is a stackframe holder,
			// otherwise the stack would get corrupted,
			// because the stack pointer register would
			// not point to the top of the stack, as
			// it would have been backtracked to the stackframe
			// holding the tiny-stackframe of interest.
			if (isstackframeholder(f)) {
				// Note that the linkedlist pointed by
				// f->calledfunctions also contain
				// calledfunctions made by subfunctions.
				lyricalcalledfunction* calledfunction1 = f->calledfunctions;
				
				while (calledfunction1) {
					// Return 1 if calledfunction1->f
					// is not a subfunction of f.
					uint calledfunction1isnotfsubfunction() {
						
						lyricalfunction* ff = calledfunction1->f->parent;
						
						while (ff != f) {
							if (!(ff = ff->parent))
								return 1;
						}
						
						return 0;
					}
					
					if (!isstackframeholder(calledfunction1->f) && calledfunction1isnotfsubfunction()) {
						// This function recurse through
						// all lyricalcalledfunction, and
						// return 1 if a lyricalcalledfunction->f
						// in any of the recursions was
						// a stackframe holder.
						uint calledfunction1fcalledfunctions (lyricalcalledfunction* calledfunction2) {
							
							uint retvar = 0;
							
							while (calledfunction2) {
								
								if (isstackframeholder(calledfunction2->f))
									retvar = 1;
								else retvar |= calledfunction2->f->couldnotgetastackframeholder =
									// Note that the linkedlist pointed by
									// calledfunction2->f->calledfunctions also
									// contain calledfunctions made by subfunctions.
									calledfunction1fcalledfunctions(calledfunction2->f->calledfunctions);
								
								calledfunction2 = calledfunction2->next;
							}
							
							return retvar;
						}
						
						calledfunction1->f->couldnotgetastackframeholder =
							// Note that the linkedlist pointed by
							// calledfunction1->f->calledfunctions also
							// contain calledfunctions made by subfunctions.
							calledfunction1fcalledfunctions(calledfunction1->f->calledfunctions);
					}
					
					calledfunction1 = calledfunction1->next;
				}
			}
		}
		
		// When I get here, f == rootfunc;
		
		while ((f = f->next) != rootfunc) {
			
			if (!isstackframeholder(f)) {
				// I find the closest parent function that will be
				// the stackframe holder of the function for which
				// the lyricalfunction is pointed by f.
				
				lyricalfunction* stackframeholder = f->parent;
				
				// This variable will be used to count how many
				// nesting level there is between the lyricalfunction
				// pointed by stackframeholder and the lyricalfunction
				// pointed by f.
				uint level = 1;
				
				while (!isstackframeholder(stackframeholder)) {
					
					stackframeholder = stackframeholder->parent;
					
					++level;
				}
				
				f->stackframeholder = stackframeholder;
				
				lyricalcachedstackframe* cachedstackframe = f->cachedstackframes;
				
				if (cachedstackframe) {
					// I merge all lyricalcachedstackframe of the lyricalfunction
					// pointed by f into the lyricalfunction pointed by stackframeholder,
					// and free the linkedlist of lyricalcachedstackframe of
					// the lyricalfunction pointed by f.
					
					do {
						if (cachedstackframe->level > level)
							cachestackframe(stackframeholder,
								cachedstackframe->level - level);
						
					} while (cachedstackframe = cachedstackframe->next);
					
					freestackframelinkedlist(f->cachedstackframes);
					
					f->cachedstackframes = 0;
				}
				
				// This function create a shared region
				// for the lyricalfunction pointed by f
				// in the stackframe holder lyricalfunction
				// pointed by stackframeholder.
				void createsharedregion () {
					
					lyricalsharedregion* sharedregion;
					
					// This function add the lyricalfunction pointed by f
					// to the lyricalsharedregion pointed by sharedregion.
					// It also set the field sharedregiontouse of
					// the lyricalfunction pointed by f.
					void addfunctiontosharedregion () {
						
						lyricalsharedregionelement* e = mmalloc(sizeof(lyricalsharedregionelement));
						
						e->f = f;
						
						e->next = sharedregion->e;
						
						sharedregion->e = e;
						
						f->sharedregiontouse = sharedregion;
					}
					
					sharedregion = stackframeholder->sharedregions;
					
					while (sharedregion) {
						// Two functions cannot share the same stackframe region
						// if one of the function is calling the other.
						// This function check whether the functions for which
						// the lyricalfunction pointed by f and e->f are calling
						// each other; and return 1 if they are not calling
						// each other otherwise return 0.
						uint noconflictfound () {
							// catchable-label to be thrown when
							// a conflict is found.
							__label__ labelforconflictfound;
							
							lyricalsharedregionelement* e = sharedregion->e;
							
							do {
								lyricalfunction* ff;
								
								// This function check whether
								// a lyricalcalledfunction called
								// the lyricalfunction pointed by ff.
								// It recursively go through lyricalcalledfunction
								// of a lyricalcalledfunction being processed.
								void recursivelycheck (lyricalcalledfunction* calledfunction) {
									
									if (calledfunction) {
										// This array is used to prevent recursively
										// checking the same lyricalcalledfunction,
										// otherwise infinite recursive calls
										// of recursivelycheck() will occur.
										static arrayuint beingchecked = arrayuintnull;
										
										if (beingchecked.ptr) {
											
											uint i = 0;
											
											while (i < arrayuintsz(beingchecked)) {
												// If the lyricalcalledfunction
												// pointed by calledfunction
												// is already recursively
												// being checked, I return.
												if (beingchecked.ptr[i] == (uint)calledfunction) return;
												
												++i;
											}
										}
										
										// I save calledfunction value,
										// so that a recursive call
										// of recursivelycheck() skip
										// its value if encountered again.
										*arrayuintappend1(&beingchecked) = (uint)calledfunction;
										
										do {
											if (calledfunction->f == ff) {
												
												mmrefdown(beingchecked.ptr);
												
												beingchecked = arrayuintnull;
												
												goto labelforconflictfound;
											}
											
											recursivelycheck(calledfunction->f->calledfunctions);
											
										} while (calledfunction = calledfunction->next);
										
										// I remove calledfunction value
										// from the array where it was
										// saved; I free the array if
										// I am removing its last element.
										if (arrayuintsz(beingchecked) == 1) {
											
											mmrefdown(beingchecked.ptr);
											
											beingchecked = arrayuintnull;
											
										} else arrayuintchop(&beingchecked, 1);
									}
								}
								
								// I first check whether any function
								// called by the lyricalfunction f
								// called the lyricalfunction e->f.
								
								ff = e->f;
								
								recursivelycheck(f->calledfunctions);
								
								// I now check whether any function
								// called by the lyricalfunction e->f
								// called the lyricalfunction f.
								
								ff = f;
								
								recursivelycheck(e->f->calledfunctions);
								
							} while (e = e->next);
							
							return 1;
							
							// I jump here if a conflict was found.
							labelforconflictfound:
							
							return 0;
						}
						
						if (noconflictfound()) {
							
							addfunctiontosharedregion();
							
							return;
						}
						
						sharedregion = sharedregion->next;
					}
					
					// If I get here, I couldn't find an already created
					// shared stackframe region useable by the function
					// for which the lyricalfunction is pointed by f.
					
					sharedregion = mmalloc(sizeof(lyricalsharedregion));
					
					sharedregion->e = 0;
					
					sharedregion->next = stackframeholder->sharedregions;
					
					stackframeholder->sharedregions = sharedregion;
					
					addfunctiontosharedregion();
				}
				
				createsharedregion();
			}
		}
		
		// When I get here, f == rootfunc;
		
		while ((f = f->next) != rootfunc) {
			// This function is used to free
			// a linkedlist of lyricalcalledfunction.
			// The linkedlist should be valid because
			// there is no check on whether it is valid.
			void freecalledfunctionlinkedlist (lyricalcalledfunction* linkedlist) {
				
				do {
					lyricalcalledfunction* savedptr = linkedlist;
					
					linkedlist = linkedlist->next;
					
					mmrefdown(savedptr);
					
				} while (linkedlist);
			}
			
			// Free the linkedlist pointed by the field calledfunctions
			// of all lyricalfunction; it is no longer needed.
			if (f->calledfunctions) freecalledfunctionlinkedlist(f->calledfunctions);
			
			// If the lyricalfunction is a stackframe holder,
			// I go through its linkedlist of lyricalcachedstackframe
			// and remove the ones that refer to lyricalfunction that
			// make use of a stackframe holder because such lyricalfunction
			// cannot cache stackframe pointers; the removed lyricalcachedstackframe
			// are replaced by lyricalcachedstackframe for their respective
			// stackframe holder.
			if (!f->stackframeholder) {
				
				lyricalcachedstackframe* cachedstackframe = f->cachedstackframes;
				
				if (cachedstackframe) {
					
					lyricalcachedstackframe* prevcachedstackframe = 0;
					
					do {
						uint level = cachedstackframe->level -1;
						
						lyricalfunction* ff = f->parent;
						
						while (level) {
							
							ff = ff->parent;
							
							--level;
						}
						
						if (ff->stackframeholder) {
							// I remove the lyricalcachedstackframe pointed
							// by cachedstackframe from its linkedlist
							// after I have added a lyricalcachedstackframe
							// for the stackframe holder of
							// the lyricalcachedstackframe to remove.
							
							level = cachedstackframe->level;
							
							do {
								++level;
								
								ff = ff->parent;
								
							} while (ff->stackframeholder);
							
							cachestackframe(f, level);
							
							if (prevcachedstackframe) {
								
								prevcachedstackframe->next = cachedstackframe->next;
								
								mmrefdown(cachedstackframe);
								
								cachedstackframe = prevcachedstackframe->next;
								
							} else {
								
								f->cachedstackframes = cachedstackframe->next;
								
								mmrefdown(cachedstackframe);
								
								cachedstackframe = f->cachedstackframes;
							}
							
						} else {
							
							prevcachedstackframe = cachedstackframe;
							
							cachedstackframe = cachedstackframe->next;
						}
						
					} while (cachedstackframe);
				}
			}
		}
	}
	
	reviewdatafromfirstpass();
	
	// Here I go into the secondpass.
	++compilepass;
	
	firstpassrootfunc = rootfunc;
	
	// Here I allocate a new root function for the secondpass.
	rootfunc = mmallocz(sizeof(lyricalfunction));
	rootfunc->next = rootfunc->prev = rootfunc;
	// rootfunc->parent = rootfunc->sibling = 0; // Not needed because mmallocz() was used.
	
	rootfunc->firstpass = firstpassrootfunc;
	
	firstpassrootfunc->secondpass = rootfunc;
	
	// Reset curpos.
	curpos = compileargsource;
	
	rootfunc->startofdeclaration = curpos;
	
	skipspace();
	
	// Before going into the secondpass,
	// I need to re-initialize currentfunc
	// and lastchildofcurrentfunc which
	// are declared within the file
	// "parsestatement.lyrical.c" .
	currentfunc = rootfunc;
	lastchildofcurrentfunc = 0;
	
	// Create the general purpose registers for
	// the function; gpr are made available only
	// in the secondpass because instructions
	// are generated only in the secondpass.
	// Each function has to have its own linkedlist
	// of GPRs because register allocation is a process
	// unique to each function; so here I create
	// a new linkedlist of GPRs for the processing
	// of the function that I am defining.
	creategpr(rootfunc);
	
	parsestatement(PARSEFUNCTIONBODY);
	
	void reviewdatafromsecondpass () {
		
		lyricalfunction* f = rootfunc;
		
		// This loop compute
		// the stackframe usage
		// of all functions.
		do {
			// I check the stack memory usage of the function
			// for which the lyricalfunction is pointed by f.
			// In doing the check, I assume that the function is
			// always a stackframe holder and I ommit the shared region.
			// Note that I distinguish two types of stackframe:
			// regular and tiny stackframes; functions making use
			// of a regular stackframe can be stackframe holders
			// and hold other function stackframes in their
			// shared memory region.
			
			// Align the value of f->vlocalmaxsize to sizeofgpr,
			// otherwise, during runtime, the address in
			// the stack pointer register may become unaligned.
			f->vlocalmaxsize = ROUNDUPTOPOWEROFTWO(f->vlocalmaxsize, sizeofgpr);
			
			// This variable will hold
			// the stack memory usage of
			// the function without the
			// shared region memory usage.
			uint u;
			
			// The stack space for rootfunc
			// exist only to hold shared regions
			// so that a function created within
			// rootfunc can use a tiny stackframe.
			// rootfunc->stackframepointerscachesize
			// will always be null because rootfunc
			// has no parent function for which the
			// stackframe pointer need to be cached.
			// rootfunc->varg is always null, and
			// rootfunc->vlocal are global variables
			// which reside in the global region.
			if (f == rootfunc) u = 0;
			else {
				u = (7*sizeofgpr) + f->stackframepointerscachesize + f->vlocalmaxsize;
				
				// If the function is variadic,
				// MAXARGUSAGE is taken as
				// the amount of memory that
				// will be used by its arguments.
				if (f->isvariadic) u += MAXARGUSAGE;
				else {
					lyricalvariable* v = f->varg;
					
					if (v) u += v->offset + v->size;
				}
				
				// Align the value of u to sizeofgpr.
				u = ROUNDUPTOPOWEROFTWO(u, sizeofgpr);
			}
			
			// This variable will hold the amount of stack memory
			// that is available for the shared region.
			uint availablesharedregionsize;
			
			// The stackframe size of a function that has
			// its address obtained is limited to MAXSTACKUSAGE,
			// otherwise it is limited to (PAGESIZE-sizeofgpr)
			// where sizeofgpr account for the space at the bottom
			// of a stackpage to store an address and which is
			// used to link all stackpages in a list.
			// The limitation to MAXSTACKUSAGE is used because
			// code that call a function through a pointer to
			// function do not know how much memory it will use
			// for its stack and will assume that its stack usage
			// cannot be greater than MAXSTACKUSAGE.
			if (f->firstpass->itspointerisobtained) {
				
				if (u > MAXSTACKUSAGE) {
					
					u -= MAXSTACKUSAGE;
					
					curpos = f->startofdeclaration;
					
					throwerror(stringfmt("function stack usage exceed limit by %d bytes", u).ptr);
				}
				
				availablesharedregionsize = MAXSTACKUSAGE - u;
				
			} else {
				
				if (u > (PAGESIZE-sizeofgpr)) {
					
					u -= (PAGESIZE-sizeofgpr);
					
					curpos = f->startofdeclaration;
					
					throwerror(stringfmt("function stack usage exceed limit by %d bytes", u).ptr);
				}
				
				availablesharedregionsize = (PAGESIZE-sizeofgpr) - u;
			}
			
			if (!f->firstpass->stackframeholder) f->stackusage = u;
			
			// I now compute how much memory will be needed
			// for the shared region if the function has any.
			
			lyricalsharedregion* sharedregion = f->firstpass->sharedregions;
			
			if (sharedregion) {
				// Variable used to keep track of which function
				// has the largest stackframe size among the functions
				// using the shared region of the function for which
				// the lyricalfunction is pointed by f.
				struct {
					
					lyricalfunction* f;
					
					uint size;
					
				} funcwithlargeststackframe = {
					
					.size = 0,
				};
				
				// Within this loop, I will be setting the fields
				// offset and size of all lyricalsharedregion of the function;
				// I will also be setting the field sharedregionsize of
				// the lyricalfunction for which the lyricalsharedregion
				// were created.
				do {
					uint sharedregionsize = 0;
					
					lyricalsharedregionelement* e = sharedregion->e;
					
					do {
						lyricalfunction* ff = e->f->secondpass;
						
						// I compute the stackframe size that will be needed by
						// the function for which the lyricalfunction is pointed by ff.
						
						// 2*sizeofgpr account for the size of the fields
						// at the beginning of the tiny stackframe,
						// which are used to hold the return address
						// and pointer to the previous stackframe.
						uint stackframesize = ff->vlocalmaxsize + (2*sizeofgpr);
						
						// If the function for which the lyricalfunction is pointed
						// by ff is variadic, MAXARGUSAGE is taken as the amount of
						// memory that will be used by its arguments.
						if (ff->isvariadic) stackframesize += MAXARGUSAGE;
						else {
							// When non-null the field ff->varg point to
							// the lyricalvariable for the last created argument.
							lyricalvariable* v = ff->varg;
							
							// I increment stackframesize by the size
							// occupied by all its arguments.
							if (v) stackframesize += v->offset + v->size;
						}
						
						// If the function return a value, I increment
						// stackframesize by the space needed by
						// the return variable pointer.
						if (!stringiseq2(ff->type, "void"))
							stackframesize += sizeofgpr;
						
						ff->stackusage = stackframesize;
						
						// I update sharedregionsize with
						// the largest stackframe size.
						if (stackframesize > sharedregionsize)
							sharedregionsize = stackframesize;
						
						// I update funcwithlargeststackframe with
						// the function having the largest stackframe.
						if (stackframesize > funcwithlargeststackframe.size) {
							funcwithlargeststackframe.size = stackframesize;
							funcwithlargeststackframe.f = ff;
						}
						
					} while (e = e->next);
					
					// Align the value of sharedregionsize to sizeofgpr.
					sharedregionsize = ROUNDUPTOPOWEROFTWO(sharedregionsize, sizeofgpr);
					
					sharedregion->offset = f->sharedregionsize;
					
					f->sharedregionsize += sharedregionsize;
					
				} while (sharedregion = sharedregion->next);
				
				// If the shared region usage of the function for which
				// the lyricalfunction is pointed by f is greater than what
				// is available, I do a recompilation preventing the function
				// that was the largest user of the shared region from
				// having its stackframe held when recompiling.
				if (f->sharedregionsize > availablesharedregionsize) {
					// I save the id of the function which should not
					// have its stackframe held when recompiling.
					// I do this one function at a time between recompilation
					// attempts in order to minimize the number of functions
					// that would need to be prevented from getting
					// their stackframe held by a stackframe holder, since
					// a smaller and a completely different shared region
					// arrangement can result from preventing only a
					// single function; in fact multiple regions are created
					// within a shared region to accomodate functions
					// which cannot share the same region because
					// they call each other.
					*arrayuintappend1(&couldnotgetastackframeholder) =
						funcwithlargeststackframe.f->firstpass->id;
					
					// I free any memory block that has been allocated within
					// the memory session that I created for regaining allocated
					// memory when a recompilation is needed.
					mmsessionfree(compilesession2, MMDOSUBSESSIONS);
					
					goto labelforrecompile;
				}
			}
			
		} while ((f = f->next) != rootfunc);
		
		mmrefdown(couldnotgetastackframeholder.ptr);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			// Prepend the instructions of each lyricalfunction
			// with comments about its strackframe usage.
			do {
				if (!f->firstpass->stackframeholder) {
					// I save what was the last instruction
					// generated in the lyricalfunction, so as
					// to restore it once the comment instruction
					// has been generated; and since the linkedlist
					// pointed by f->i is circular, it make first
					// the generated instruction.
					lyricalinstruction* savedi = f->i;
					
					comment(f, stringfmt("stackframe size: %d+(sharedregion: %d)", f->stackusage, f->sharedregionsize));
					
					// Restore f->i, making first
					// the comment instruction.
					f->i = savedi;
				}
				
				lyricalsharedregion* sharedregion = f->firstpass->sharedregions;
				
				if (sharedregion) {
					
					do {
						lyricalsharedregionelement* e = sharedregion->e;
						
						do {
							lyricalfunction* ff = e->f->secondpass;
							
							// I save what was the last instruction
							// generated in the lyricalfunction, so as
							// to restore it once the comment instructions
							// have been generated; and since the linkedlist
							// pointed by ff->i is circular, it make first
							// the generated instructions.
							lyricalinstruction* savedi = ff->i;
							
							comment(ff, stringfmt("stackframe size: %d", ff->stackusage));
							comment(ff, stringfmt("stackframe holder: function_%08x:%s", f, f->linkingsignature.ptr));
							comment(ff, stringfmt("stackframe offset within sharedregion: %d", sharedregion->offset));
							
							// Restore ff->i, making first
							// the comment instructions.
							ff->i = savedi;
							
						} while (e = e->next);
						
					} while (sharedregion = sharedregion->next);
				}
				
			} while ((f = f->next) != rootfunc);
		}
		
		// I cancel the session that I created for regaining
		// any allocated memory when a recompilation is needed.
		mmsessioncancel(compilesession2, MMDOSUBSESSIONS);
		
		// When I get here, f == rootfunc;
		
		// This block of code is disabled
		// to speed-up compilation because
		// a backend should be able
		// to do it more efficiently;
		// and it can be desirable to see
		// the branching instructions that
		// this block of code is removing,
		// when reviewing the lyricalbackendtext
		// output of the lyricalcompileresult.
		#if 0
		// Loop that remove un-necessary
		// branching instructions.
		// Note that it is not a dead-code
		// removal optimization; it simply
		// remove lyricalinstruction that branch
		// to a LYRICALOPNOP that immediately
		// follow them with LYRICALOPCOMMENT and
		// other LYRICALOPNOP in between ignored.
		// At this point, the target
		// of all branching instructions
		// is an LYRICALOPNOP, and
		// this optimization must
		// come before the reviewing
		// of immediate values of type
		// LYRICALIMMOFFSETTOINSTRUCTION
		// connecting them to the next
		// closest lyricalinstruction
		// that is not an LYRICALOPNOP.
		do {
			// Note that f->i is always non-null,
			// so there is no need to check it.
			
			lyricalinstruction* i = f->i;
			
			do {
				if (i->op == LYRICALOPNOP) {
					// If a lyricalinstruction
					// that is not LYRICALOPCOMMENT
					// or LYRICALOPNOP precede the
					// lyricalinstruction pointed by i,
					// and is a branching instruction
					// to the lyricalinstruction pointed
					// by i, then it can be safely removed,
					// since there is no instructions
					// between that branching instruction
					// and the lyricalinstruction pointed by i.
					
					lyricalinstruction* ii = i->prev;
					
					while (ii != f->i) {
						
						if (ii->op != LYRICALOPCOMMENT && ii->op != LYRICALOPNOP) {
							// All branching lyricalop are between
							// LYRICALOPJEQ and LYRICALOPJPOP .
							if ((ii->op >= LYRICALOPJEQ && ii->op <= LYRICALOPJPOP) &&
								ii->imm->i == i) {
								
								// I detach the lyricalinstruction
								// pointed by ii from its linkedlist
								// and free it.
								LINKEDLISTCIRCULARREMOVE_(prev, next, ii);
								// The lyricalinstruction pointed
								// by ii is sure not the only
								// or last lyricalinstruction
								// generated within a lyricalfunction;
								// hence f->i would not need to be updated.
								
								// I free the linkedlist of immediate
								// values attached to the instruction.
								
								lyricalimmval* imm = ii->imm;
								
								do {
									lyricalimmval* savedimm = imm;
									imm = imm->next;
									mmrefdown(savedimm);
									
								} while (imm);
								
								if (ii->unusedregs) mmrefdown(ii->unusedregs);
								
								if (ii->dbginfo.filepath.ptr) mmrefdown(ii->dbginfo.filepath.ptr);
								
								lyricalinstruction* savedii = ii;
								ii = ii->prev;
								mmrefdown(savedii);
								
								continue;
								
							} else break;
						}
						
						ii = ii->prev;
					}
				}
				
			} while ((i = i->next) != f->i);
			
		} while ((f = f->next) != rootfunc);
		#endif
		
		// When I get here, f == rootfunc;
		
		// This loop resolve all immediate
		// values of type LYRICALIMMLOCALVARSSIZE,
		// LYRICALIMMNEGATIVELOCALVARSSIZE,
		// LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE,
		// LYRICALIMMSHAREDREGIONSIZE and
		// LYRICALIMMOFFSETWITHINSHAREDREGION to immediate
		// values of type LYRICALIMMVALUE.
		// It also review immediate values
		// of type LYRICALIMMOFFSETTOINSTRUCTION
		// connecting them to the next
		// closest lyricalinstruction
		// that is not an LYRICALOPNOP,
		// ignoring any LYRICALOPCOMMENT.
		// In fact, an LYRICALOPNOP in
		// the result of the compilation
		// is there to give a cue to the
		// backend that branching occur
		// to the instruction following
		// the LYRICALOPNOP.
		do {
			if (f->i) {
				
				lyricalinstruction* i = f->i;
				
				do {
					if (i->imm) {
						
						lyricalimmval* imm = i->imm;
						
						// This loop go through all the immediate values
						// of the lyricalinstruction.
						do {
							switch(imm->type) {
								
								case LYRICALIMMOFFSETTOINSTRUCTION:
									// The field i of the immediate
									// is set to the next closest
									// lyricalinstruction that
									// is not an LYRICALOPNOP,
									// ignoring any LYRICALOPCOMMENT.
									// Note that an LYRICALOPNOP can never
									// be the only or last lyricalinstruction
									// generated within a lyricalfunction;
									// hence imm->i->next will
									// always be valid.
									while (imm->i->op == LYRICALOPNOP || imm->i->op == LYRICALOPCOMMENT)
										imm->i = imm->i->next;
									
									break;
									
								case LYRICALIMMLOCALVARSSIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = imm->f->vlocalmaxsize;
									
									break;
									
								case LYRICALIMMNEGATIVELOCALVARSSIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = -imm->f->vlocalmaxsize;
									
									break;
									
								case LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = imm->f->stackframepointerscachesize;
									
									break;
									
								case LYRICALIMMNEGATIVESTACKFRAMEPOINTERSCACHESIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = -imm->f->stackframepointerscachesize;
									
									break;
									
								case LYRICALIMMSHAREDREGIONSIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = imm->f->sharedregionsize;
									
									break;
									
								case LYRICALIMMNEGATIVESHAREDREGIONSIZE:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = -imm->f->sharedregionsize;
									
									break;
									
								case LYRICALIMMOFFSETWITHINSHAREDREGION:
									
									imm->type = LYRICALIMMVALUE;
									imm->n = imm->sharedregion->offset;
									
									break;
									
								case LYRICALIMMVALUE:
									
									;s64 n = imm->n;
									
									// Sign-extend immediate value
									// to help backends properly use it.
									n <<= ((8*sizeof(u64))-bitsizeofgpr);
									n >>= ((8*sizeof(u64))-bitsizeofgpr);
									
									imm->n = n;
									
									break;
							}
							
						} while (imm = imm->next);
					}
					
				} while ((i = i->next) != f->i);
			}
			
		} while ((f = f->next) != rootfunc);
		
		// When I get here, f == rootfunc;
		
		// This optimization is commented out
		// because a backend should be able
		// to handle more than one LYRICALOPNOP,
		// following each other or separated
		// by one or more LYRICALOPCOMMENT;
		// and it is faster.
		#if 0
		// Loop that replace more
		// than one LYRICALOPNOP,
		// following each other
		// or separated by one or
		// more LYRICALOPCOMMENT,
		// with a single LYRICALOPNOP.
		// This optimization should
		// always come after the reviewing
		// of immediate values of type
		// LYRICALIMMOFFSETTOINSTRUCTION
		// connecting them to the next
		// closest lyricalinstruction
		// that is not an LYRICALOPNOP.
		do {
			// Note that f->i is always non-null,
			// so there is no need to check it.
			
			lyricalinstruction* i = f->i;
			
			// Variable set to 1 to determine
			// that duplicate are being removed.
			uint removingduplicate = 0;
			
			do {
				if (i->op == LYRICALOPNOP) {
					
					if (removingduplicate) {
						// If I get here, I detach
						// the instruction from
						// its linkedlist and
						// I free it.
						
						i->prev->next = i->next;
						i->next->prev = i->prev;
						
						// An LYRICALOPNOP can never be
						// the only or last lyricalinstruction
						// generated within a lyricalfunction,
						// hence there is no need to check
						// whether i == f->i in order to prevent
						// f->i from being set to a lyricalinstruction
						// that has been freed.
						
						if (i->unusedregs) mmrefdown(i->unusedregs);
						
						if (i->dbginfo.filepath.ptr) mmrefdown(i->dbginfo.filepath.ptr);
						
						lyricalinstruction* savedi = i;
						i = i->next;
						mmrefdown(savedi);
						
					} else {
						// If get here, I set
						// removingduplicate
						// to 1, so that the following
						// instructions get removed
						// if they were duplicate.
						
						removingduplicate = 1;
						
						i = i->next;
					}
					
				} else {
					
					if (removingduplicate && i->op != LYRICALOPCOMMENT) {
						// Note that when I get here,
						// i is set to a lyricalinstruction
						// to which branching occur since
						// an LYRICALOPNOP precede it.
						
						removingduplicate = 0;
					}
					
					i = i->next;
				}
				
			} while (i != f->i);
			
		} while ((f = f->next) != rootfunc);
		#endif
		
		// When I get here, f == rootfunc;
		
		while ((f = f->next) != rootfunc) {
			// I free the general purpose registers
			// that were created for the function
			// since I will no longer need them.
			destroygpr(f);
			
			if (f->scope) mmrefdown(f->scope);
			// Every functions beside the root function,
			// has its fields name, type, typeofaddr, fcall set;
			// so there is no need to check whether they are null.
			mmrefdown(f->name.ptr);
			mmrefdown(f->type.ptr);
			mmrefdown(f->typeofaddr.ptr);
			pamsynfree(f->fcall);
			mmrefdown(f->callsignature.ptr);
			
			if (f->varg) freevarlinkedlist(f->varg);
			if (f->vlocal) freevarlinkedlist(f->vlocal);
			
			// The linkedlist pointed by f->i is freed by lyricalfree().
			
			// The linkedlist of lyricaltype has
			// already been freed within parsestatement().
			//if (f->t) freetypelinkedlist(f->t);
			
			// The linkedlist pointed by f->p is set only
			// for lyricalfunction created in the firstpass,
			// and is used in the secondpass through f->firstpass.
			
			// The linkedlists pointed by the fields l
			// and cl are freed respectively by resolvelabelsnow()
			// and resolvecatchablelabelsnow() once a function
			// is done being parsed.
			
			// The freeing of the lyricalfunction is done by lyricalfree().
		}
		
		// I free the general purpose registers
		// that were created for the function
		// since I will no longer need them.
		destroygpr(rootfunc);
		
		if (rootfunc->vlocal) freevarlinkedlist(rootfunc->vlocal);
		
		// The linkedlist of lyricaltype has
		// already been freed within parsestatement().
		//if (rootfunc->t) freetypelinkedlist(rootfunc->t);
		
		// if (rootfunc->i) freeinstructionlinkedlist(rootfunc->i); // Done by lyricalfree();
		
		// mmrefdown(rootfunc); // Done by lyricalfree();
		
		// Here I free the linkedlist of lyricalfunction
		// that was created in the firstpass as well as
		// any other associated data that was not freed
		// by reviewdatafromfirstpass() because it was
		// needed in the secondpass.
		f = firstpassrootfunc->next;
		while (f != firstpassrootfunc) {
			
			if (f->p) freepropagationlinkedlist(f->p);
			
			if (f->pushedargflags) freeargumentflaglinkedlist(f->pushedargflags);
			
			if (f->cachedstackframes) freestackframelinkedlist(f->cachedstackframes);
			
			if (f->sharedregions) freesharedregionlinkedlist(f->sharedregions);
			
			f = f->next;
			
			mmrefdown(f->prev);
		}
		
		// I take care of the fields of the firstpass
		// root function which contain data to free.
		
		if (firstpassrootfunc->p) freepropagationlinkedlist(firstpassrootfunc->p);
		
		if (firstpassrootfunc->pushedargflags) freeargumentflaglinkedlist(firstpassrootfunc->pushedargflags);
		
		if (firstpassrootfunc->sharedregions) freesharedregionlinkedlist(firstpassrootfunc->sharedregions);
		
		// I free the firstpass root function.
		mmrefdown(firstpassrootfunc);
	}
	
	reviewdatafromsecondpass();
	
	mmrefdown(returnvar.name.ptr);
	mmrefdown(thisvar.type.ptr);
	mmrefdown(thisvar.name.ptr);
	
	mmrefdown(u8ptrstr.ptr);
	mmrefdown(voidptrstr.ptr);
	mmrefdown(voidfncstr.ptr);
	
	// I free the strings that were
	// allocated for the native type names;
	// since I will no longer need them.
	
	// This get the number of elements
	// in the nativetype array.
	uint nativetypecount = sizeof(nativetype)/sizeof(lyricaltype);
	
	do {
		--nativetypecount;
		
		mmrefdown(nativetype[nativetypecount].name.ptr);
	
	} while (nativetypecount);
	
	// I free all the pamsyntokenized that I created,
	// since I will no longer need them.
	
	// This will get the number of elements in
	// the array of pamsyntokenized that was created
	// for native operators.
	uint nativefcallcount = sizeof(nativefcall)/sizeof(pamsyntokenized);
	
	do {
		--nativefcallcount;
		
		pamsynfree(nativefcall[nativefcallcount]);
		
	} while (nativefcallcount);
	
	pamsynfree(iscorrectopdeclaration);
	pamsynfree(overloadableop);
	pamsynfree(isnativeorpointertype);
	pamsynfree(isnativetype);
	pamsynfree(iskeyword);
	pamsynfree(matchoffsetifvarfield);
	pamsynfree(matchtempvarname);
	
	// I free the linkedlist of
	// chunk pointed by chunks.
	freechunklinkedlist(chunks);
	
	// I free the null terminated string that
	// contained the source code to compile.
	// That string was generated by postpreprocessing().
	mmrefdown(compileargsource);
	
	if (stringregionsz) {
		// I allocate memory for the data
		// which is to be used to initialize
		// the string region.
		compileresult.stringregion.ptr = mmalloc(stringregionsz);
		
	} else compileresult.stringregion = arrayu8null;
	
	// This function generate the data that should
	// be used to initialize the string region.
	void generatestringregion () {
		// Variable that will be used as an offset
		// within the string region being generated.
		uint o = 0;
		
		stringconstant* sc;
		
		// I generate the data which is to be used
		// to initialize the string region.
		// I also free the linkedlist of stringconstant
		// in the process since it will no longer be needed.
		do {
			void* ptr = stringconstants->value.ptr;
			
			// The string null-terminating byte
			// will be included in the size.
			uint sz = mmsz(stringconstants->value.ptr);
			
			bytcpy(compileresult.stringregion.ptr + o, ptr, sz);
			
			o += sz;
			
			mmrefdown(ptr);
			
			sc = stringconstants->next;
			
			mmrefdown(stringconstants);
			
		} while ((stringconstants = sc));
	}
	
	if (stringconstants) generatestringregion();
	
	compileresult.globalregionsz = rootfunc->vlocalmaxsize;
	
	// I cancel the session that I created for regaining
	// any allocated memory in the event of a thrown error.
	mmsessioncancel(compilesession1, MMDOSUBSESSIONS);
	
	compileresult.rootfunc = rootfunc;
	
	return compileresult;
	
	// I jump here in the event of an error.
	labelforerror:
	
	// Since an error was thrown, I free any memory block
	// that may have been allocated within the memory session
	// that I created before making any allocation.
	mmsessionfree(compilesession1, MMDOSUBSESSIONS);
	
	compileresult.rootfunc = 0;
	
	return compileresult;
}


// This function free the memory used by
// the compile result returned by lyricalcompile().
// The field rootfunc of the compileresult should
// be non-null to be used with this function.
void lyricalfree (lyricalcompileresult compileresult) {
	#define LYRICALFREE
	// This file is also included within lyricalcompile().
	#include "tools.lyrical.c"
	#undef LYRICALFREE
	
	lyricalfunction* f = compileresult.rootfunc->next;
	
	while (f != compileresult.rootfunc) {
		// There is no need to check whether
		// f->i is non-null because every function
		// will have at least a single instruction.
		freeinstructionlinkedlist(f->i);
		
		// The linking signature of a declared or
		// defined function is always generated;
		// so there is no need to check whether
		// f->linkingsignature.ptr is non-null.
		mmrefdown(f->linkingsignature.ptr);
		
		f = f->next;
		
		mmrefdown(f->prev);
	}
	
	// There is no need to check whether
	// rootfunc->i is non-null because
	// every function will have at least
	// a single instruction.
	freeinstructionlinkedlist(compileresult.rootfunc->i);
	
	mmrefdown(compileresult.rootfunc);
	
	if (compileresult.stringregion.ptr) mmrefdown(compileresult.stringregion.ptr);
	
	if (compileresult.srcfilepaths.ptr) mmrefdown(compileresult.srcfilepaths.ptr);
}
