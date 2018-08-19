
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


// resolvelabellater() is the only function
// adding elements to the linkedlist pointed
// by this variable; its value is modified
// within funcdeclaration().
// This variable is used only in the secondpass
// because labels are created and used only
// in the secondpass.
lyricallabeledinstructiontoresolve* linkedlistoflabeledinstructiontoresolve = 0;

// parsestatement() process an entire block
// of instruction between '{' and '}' and
// return once it reach the end of the block.
// When processing a while() block for example,
// I sometime need to know where to jump in order
// to exit a while() loop (When break statement is used);
// so to achieve that capability I use labelnameforendofloop
// which contain a string from which a label will be created
// and which will be used with resolvelabellater().
// A similar logic go with labelnameforcontinuestatement
// which contain a string to use with resolvelabellater() when
// using the keywork continue, and it is used only for loops.
// These 2 variables are used only in the secondpass
// because labels are created and used only in the secondpass.
string labelnameforendofloop = stringnull;
string labelnameforcontinuestatement = stringnull;

// This variable point to a linkedlist of arguments
// linked through their fields prevregisteredarg
// and nextregisteredarg. The linkedlist is used by
// propagatevarchange() to find all arguments
// that have been pushed.
// This variable is used only in the secondpass.
lyricalargument* registeredargs = 0;


// When getting into a new scope:
// - scopecurrent is incremented.
// - If scopecurrent is greater than scopedepth,
// the array pointed by scope is increased by
// an additional sizeof(uint), which get initialized
// to 1, and scopedepth get incremented afterward;
// else I simply increment scope[scopecurrent-1].
// 
// The above steps are implemented within scopeentering().
// 
// When leaving a scope, local variables which
// were created in the scope that I am leaving
// are freed and scopecurrent is decremented.
// 
// Newly created variables, functions or types
// have their field scopedepth set to scopecurrent;
// And scopecurrent*sizeof(uint) bytes is copied from
// the array pointed by scope to an array of
// the same size pointed by their field scope.
// With all those informations it is possible to tell
// in which scope exactely a variable was created.
// 
// The scope variables below are saved and reset to null
// within funcdeclaration() before calling parsestatement()
// to parse the definition of a function.
// 
// Getting into a new scope occur when the opening brace
// character '{' is parsed; the opening brace used
// when parsing an initialization block or an union/struct
// definition do not create a new scope.
// 
// There is no need to re-initialized these variables
// to null before going into the secondpass because
// their values are always restored to their initilial
// state when I am done with the firstpass.
uint scopecurrent = 0;
uint scopedepth = 0; // Determine how many uint I have in the array pointed by scope.
uint* scope = 0; // Array of uint to keep the counts of the number of scope at a corresponding depth.


// Structure representing a switch() block
// in which are created switch() cases.
typedef struct {
	// Type of the switch() block expression.
	string exprtype;
	
	// Default switch() case label.
	// The default switch() case do not get added
	// to the binary tree which hold all
	// other switch() case of the switch() block.
	// This field is set and used only in the secondpass.
	string defaultcase;
	
	// Binary tree which will hold all
	// switch() case label of the switch() block.
	// This field is set and used only in the secondpass.
	bintree btree;
	
} switchblock;

// This variable is non-null while within the scope of
// its corresponding switch() block, otherwise it is null.
// This variable is saved across parsing of nested switch() block.
// There is no need to re-initialize this variable to null
// before going into the secondpass because its value is always
// restored to its initilial state when done with the firstpass.
switchblock* currentswitchblock = 0;

// This string is used when statementparsingflag
// is set to PARSEPOINTERTOFUNCTIONTYPE or PARSEFUNCTIONSIGNATURE;
// I call parsestatement() that way when respectively parsing
// a pointer to function type or function signature.
// By recursing into parsestatement(), I am effectively
// re-using codes that are used to declare variable type
// and to verify that they are valids.
string containparsedstring;

// The argument currenttype is used to point
// to the struct/pstruct/union type that I am
// processing when I am parsing a struct/pstruct/union.
// There is no need to re-initialized this variable
// to null before going into the secondpass because
// its value is always restored to its initilial
// state when I am done with the firstpass.
lyricaltype* currenttype = 0;

// currentfunc is used to point to
// the function that I am defining or processing.
// Before going into the secondpass, I will
// re-initialize it to rootfunc which will then
// be pointing to the root function created
// for the secondpass.
lyricalfunction* currentfunc = rootfunc;

// lastchildofcurrentfunc point
// to the last child lyricalfunction
// to currentfunc that was created.
// It will be used to set the field sibling
// of all children of a lyricalfunction
// so that they form a linkedlist
// where the first created children
// has its field sibling set to null.
// Before going into the secondpass,
// I will re-initialize it to null.
lyricalfunction* lastchildofcurrentfunc = 0;

// Structure used to keep offsets,
// within the global variable region,
// of status variables to set null.
// This variable is used only
// in the secondpass.
struct {
	uint* o; // Array of uint.
	uint i;	 // Number of uint in the array.
	
} statusvars = {0,0};


// Possible values of the argument
// statementparsingflag of parsestatement().
typedef enum {
	// Used when parsing the body of
	// the definition of a function.
	PARSEFUNCTIONBODY,
	
	// Used when parsing the declaration
	// of the arguments of a function.
	PARSEFUNCTIONARG,
	
	// Used when parsing a struct.
	PARSESTRUCT,
	
	// Used when parsing a pstruct.
	PARSEPSTRUCT,
	
	// Used when parsing an union.
	PARSEUNION,
	
	// Used when parsing a single expression.
	// Example: when parsing the if() statement followed
	// by a single expression instead of a block.
	PARSESINGLEEXPR,
	
	// Used when parsing code within a scoping-block.
	PARSEBLOCK,
	
	// Used when reading a type symbol with any array
	// or pointer specification coming after it.
	PARSEPOINTERTOFUNCTIONTYPE,
	
	// Used when reading a function signature.
	PARSEFUNCTIONSIGNATURE
	
} parsestatementflag;

// This function is used to parse a statement.
// skipspace() must have been called before calling this function.
void parsestatement (parsestatementflag statementparsingflag) {
	
	union {
		// Bitfield which must fit within a u8,
		// as it is unionized with a uint which
		// could very well be a u8.
		struct {
			// This field is set to 1 when the keyword
			// export is parsed, otherwise it is set to 0.
			uint isexport :1;

			// This field is set to 1 when the keyword
			// static is parsed, otherwise it is set to 0.
			uint isstatic :1;
		};
		
		// If any of isexport or isstatic is set,
		// this field will be non-null. It is used to check
		// if any of the declaration prefix was parsed.
		uint isset;
		
	} declarationprefix;
	
	// Value that evaluateexpression() return
	// when an expression was parsed and evaluated
	// but was not returning a value.
	#define EXPRWITHNORETVAL ((lyricalvariable*)-1)
	
	// Value to use as evaluateexpression() argument.
	// It instruct evaluateexpression() to process
	// pending postfix operations.
	#define DOPOSTFIXOPERATIONS ((lyricalvariable*)0)
	
	// Value to use as evaluateexpression() argument.
	// It instruct evaluateexpression() to parse
	// an assembly statement used only within asm{} .
	#define DOASM ((lyricalvariable*)1)
	
	// Value to use as evaluateexpression() argument.
	// It set to the highest, the precedence to compare
	// against the subsequent normal operator to parse.
	#define HIGHESTPRECEDENCE ((lyricalvariable*)0x02)
	
	// Value to use as evaluateexpression() argument.
	// It set to the lowest, the precedence to compare
	// against the subsequent normal operator to parse.
	#define LOWESTPRECEDENCE ((lyricalvariable*)0x0f)
	
	// Here I simply declare evaluateexpression() because parsetypespec() need it;
	// it is also needed within the file controlstatements.parsestatement.
	// ### GCC wouldn't compile without the use of the keyword auto.
	auto lyricalvariable* evaluateexpression (lyricalvariable* larg);
	
	#include "tools.parsestatement.lyrical.c"
	
	// This structure is used to save
	// post operations which are postfix
	// operators "++" or "--".
	// It is used by processpostfixoperatorlater().
	typedef struct postfixoperatorcall {
		// This string is always the
		// constant string "++" or "--" .
		u8* op;
		
		// Address where the postfix
		// operator "++" or "--" is located
		// in the source code being compiled.
		u8* savedcurpos;
		
		lyricalargument* arg;
		
		// A pointer to the previous
		// postfixoperatorcall is not needed.
		// The last postfixoperatorcall created
		// will have its field next pointing to
		// the first postfixoperatorcall of
		// the linkedlist.
		// Hence postfixoperatorcall will be later
		// executed by callfunctionnow() in
		// the order they were created.
		struct postfixoperatorcall* next;
		
	} postfixoperatorcall;
	
	// Pointer to a linkedlist of postfixoperatorcall.
	// The first added element to the linkedlist is the
	// first function call to make. It is crucial to keep
	// that ordering since it is in that order that
	// postfix operations are specified in an expression.
	// ei: a++ + b++;
	// In the example above, it would be syntatically
	// wrong to execute the post-operation b++
	// before the post-operation a++ .
	postfixoperatorcall* linkedlistofpostfixoperatorcall = 0;
	
	// This function check whether a variable is
	// an argument to a pending postfix operation.
	// linkedlistofpostfixoperatorcall must be non-null.
	uint isvarpostfixoperatorarg (lyricalvariable* v) {
		
		postfixoperatorcall* fcall = linkedlistofpostfixoperatorcall;
		
		do {
			lyricalargument* fcallarg = fcall->arg;
			
			lyricalargument* arg = fcallarg;
			
			do {
				if (arg->varpushed == v) return 1;
				
			} while ((arg = arg->next) != fcallarg);
			
		} while ((fcall = fcall->next) != linkedlistofpostfixoperatorcall);
		
		return 0;
	}
	
	// This function will read the type specifications
	// that are asterix and array specifications, and
	// append it to the string pointed by the argument s.
	void parsetypespec (string* s) {
		// In the loop below, I keep reading the
		// type spec as long as there exist any.
		while (1) {
			
			if (*curpos == '*') {
				
				++curpos; // Set curpos after '*'.
				
				stringappend4(s, '*');
				
			} else if (*curpos == '[') {
				
				++curpos; // Set curpos after '['.
				
				u8* savedcurpos = curpos;
				
				skipspace();
				
				// currenttype need to be null before
				// evaluating the expression within brackets,
				// so that any call of varalloc() would create
				// lyricalvariable in the correct linkedlist.
				lyricaltype* savedcurrenttype = currenttype;
				
				currenttype = 0;
				
				// I evaluate the expression
				// within brackets, which must
				// result in a non-null constant.
				lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
				
				if (linkedlistofpostfixoperatorcall) {
					curpos = savedcurpos;
					throwerror("expecting an expression with no postfix operations");
				}
				
				if (!v || v == EXPRWITHNORETVAL || !v->isnumber || !v->numbervalue) {
					curpos = savedcurpos;
					throwerror("expecting an expression that result in a non-null constant");
				}
				
				// I restore currenttype.
				currenttype = savedcurrenttype;
				
				stringappend4(s, '[');
				
				string str = stringfmt("%d", v->numbervalue);
				stringappend1(s, str);
				mmrefdown(str.ptr);
				
				stringappend4(s, ']');
				
				if (*curpos != ']') {
					reverseskipspace();
					throwerror("expecting ']'");
				}
				
				++curpos; // Position curpos after ']' .
				
			} else return;
			
			skipspace();
		}
	}
	
	#include "initialization.parsestatement.lyrical.c"
	
	#include "evaluateexpression.parsestatement.lyrical.c"
	
	// This variable is set at the start
	// of a declaration using curpos.
	// It is declared here so that
	// it can be used within funcdeclaration().
	u8* startofdeclaration;
	
	// Variable set depending on whether
	// function export was inferred
	// through characters' sequence that
	// were injected by the preprocessor.
	static uint isexportinferred = 0;
	
	#include "funcdeclaration.parsestatement.lyrical.c"
	
	if (statementparsingflag == PARSEFUNCTIONBODY) {
		// I get here when starting the parsing of a function.
		
		if (currentfunc == rootfunc) {
			
			if (rootfunc->vlocal) {
				// The global variable region
				// is expected to be empty
				// at this point,
				throwerror(stringfmt("internal error: %s: invalid global variable region", __FUNCTION__).ptr);
			}
			
			lyricalvariable* v;
			
			v = varalloc(sizeofgpr, DONOTLOOKFORAHOLE);
			v->name = stringduplicate2("arg");
			v->type = stringduplicate2("u8***");
			v->id = 1;
			v->isbyref = 1;
			
			v = varalloc(sizeofgpr, DONOTLOOKFORAHOLE);
			v->name = stringduplicate2("env");
			v->type = stringduplicate2("u8***");
			v->id = 2;
			v->isbyref = 1;
			
			// [0] is used to store the lyrical syscall function pointer.
			// [1] is used to store the pointer to http request buffer.
			v = varalloc(2*sizeofgpr, DONOTLOOKFORAHOLE);
			v->id = 3;
			
			if (compilearg->predeclaredvars) {
				// I create the predeclared variables.
				
				uint i = 0;
				
				while (1) {
					
					lyricalpredeclaredvar* p = &compilearg->predeclaredvars[i];
					
					u8* name = p->name;
					
					if (!name) break;
					
					lyricalvariable* v = varalloc(0, LOOKFORAHOLE);
					
					// Instead of (uint)curpos, i is used
					// because predeclared variables do not have
					// a corresponding curpos location where it was declared;
					// and they would all have the same curpos value,
					// causing conflict in their id value.
					// Also i+5 is used because id values
					// 1, 2, 3 and 4 have already been used.
					uint id = i+5;
					
					if (id > PAGESIZE) {
						// lyricalcompilearg.source is inforced
						// to be greater than PAGESIZE to insure
						// that the value of curpos which is used
						// to set lyricalvariable.id will not
						// conflict with the ids used with
						// predeclared variables.
						throwerror(stringfmt("internal error: %s: too many predeclared variables", __FUNCTION__).ptr);
					}
					
					v->id = id;
					
					v->name = stringduplicate2(name);
					
					v->isbyref = p->isbyref;
					
					v->predeclared = p;
					
					// The predeclared variable is set volatile if
					// a callback function is provided so as to insure
					// that the modified value of the predeclared variable
					// be written out to memory before the callback
					// function (which may read it) get called.
					if (p->callback) *v->alwaysvolatile = 1;
					
					// The field cast is set by readvarorfunc()
					// when the variable is needed.
					
					++i;
				}
			}
		}
		
		// I set the fields type and size of returnvar.
		// I do not try to check and free the field type.ptr
		// of returnvar here; that field will always be null
		// when I get here, because before lyricalcompile() call
		// parsestatement() the field type is zeroed, and
		// within funcdeclaration() before using parsestatement()
		// I save the current value of returnvar.type and
		// returnvar.size (Since I am modifying them here),
		// and set returnvar.type to null; and once I return
		// from this function I restore those fields.
		
		if (currentfunc != rootfunc) {
			returnvar.type = stringduplicate1(currentfunc->type);
			returnvar.size = sizeoftype(returnvar.type.ptr, stringmmsz(returnvar.type));
		}
		
		if (compilepass) {
			// I call cachestackframepointers() here because
			// instructions are generated only in the secondpass.
			if (currentfunc->firstpass->cachedstackframes) cachestackframepointers();
		}
	}
	
	startofprocessing:;
	
	// If I was the very first call to parsestatement()
	// with currentfunc == rootfunc which is the root function,
	// I should check if (*curpos == 0) after I jump
	// to startofprocessing because the root function do not
	// end with a closing brace just like regular functions.
	if (!*curpos && currentfunc == rootfunc && statementparsingflag == PARSEFUNCTIONBODY) {
		// Adjust curpos so to log correct location.
		reverseskipspace();
		
		lyricalfunction* f;
		
		if (compilepass) {
			// I get here in the secondpass.
			// Instructions, catchable-labels and
			// labels are created only in the secondpass.
			
			// I need to flush to variables
			// any dirty register once
			// the instructions of the root
			// function are done executing.
			flushanddiscardallreg(DONOTFLUSHREGFORLOCALS);
			
			lyricalinstruction* i;
			lyricalimmval* imm;
			
			// I generate the instructions of import functions, if any.
			f = lastchildofcurrentfunc;
			while (f) { // The first element of the linkedlist of siblings has its field sibling null.
				
				if (!f->i) { // At this point, the import function has no instructions.
					// Space is made at the bottom of the string region
					// for the field from which is to be retrieved
					// the address of the function to import.
					uint o = stringregionsz;
					stringregionsz += sizeofgpr;
					
					// I save the offset+1 within the string region
					// for the field from which is to be retrieved
					// the address of the function to import.
					f->toimport = o+1;
					
					// I generate instructions that retrieve
					// from the string region the offset
					// to the function to import,
					// compute its address and jump to it.
					
					// I set curpos so that the location where
					// the import function was declared be
					// the linenumber used with debug information.
					curpos = f->startofdeclaration;
					
					// Since the import function do not yet
					// have any instructions, I can safely assume
					// that gpr %1 and %2 are not allocated.
					
					// Create the general purpose registers for
					// the function; gpr are made available only
					// in the secondpass because instructions
					// are generated only in the secondpass.
					// Each function has to have its own linkedlist
					// of GPRs because register allocation is a process
					// unique to each function; so here I create
					// a new linkedlist of GPRs for the function.
					creategpr(f);
					
					// Search lyricalreg %1 and %2.
					lyricalreg* r1 = searchreg(f, 1);
					lyricalreg* r2 = searchreg(f, 2);
					
					// Lock lyricalreg %1 and %2
					// to prevent newinstruction()
					// from adding it to the list
					// of unused registers of
					// lyricalinstruction.
					r1->lock = 1;
					r2->lock = 1;
					
					// Here I generate:
					// afip %1, OFFSETTOSTRINGREGION + o;
					// which compute in %1 the address of the field
					// holding the address of the function to import.
					i = newinstruction(f, LYRICALOPAFIP);
					i->r1 = 1;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMOFFSETTOSTRINGREGION;
					
					i->imm = imm;
					
					imm = mmallocz(sizeof(lyricalimmval));
					imm->type = LYRICALIMMVALUE;
					imm->n = o;
					
					i->imm->next = imm;
					
					// Here I generate:
					// ld %2, %1; which load in %2
					// the offset to the function to import.
					i = newinstruction(f, lyricalopldr());
					i->r1 = 2;
					i->r2 = 1;
					
					// Here I generate:
					// add %1, %1, %2; which compute in %1
					// the address of the function to import.
					i = newinstruction(f, LYRICALOPADD);
					i->r1 = 1;
					i->r2 = 1;
					i->r3 = 2;
					
					// Here I generate:
					// jr %1; which jump to the function to import.
					i = newinstruction(f, LYRICALOPJR);
					i->r1 = 1;
					
					// Unlock lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r1->lock = 0;
					r2->lock = 0;
				}
				
				f = f->sibling;
			}
			
			// Below I generate the instructions
			// that adjust the stack pointer
			// register so as to make space in
			// the stack for its shared region;
			// I also generate instructions
			// that set status variables null,
			// and I generate them first so that
			// when restoring rootfunc->i,
			// they come after the instructions
			// decrementing the stackframe
			// pointer register.
			
			// I set curpos null so that
			// no debug information get
			// generated.
			curpos = 0;
			
			if (statusvars.o) {
				// I get here if there is any
				// status variable to set null.
				
				// I save what was the last instruction
				// generated in rootfunc, so as to
				// restore it once all the instructions
				// setting status variables to null
				// have been generated; and since
				// the linkedlist pointed by rootfunc->i
				// is circular, it make first
				// the generated instructions.
				lyricalinstruction* savedi = rootfunc->i;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("begin: setting status variables null"));
				}
				
				lyricalreg* r1; lyricalreg* r2;
				
				// Since the instructions setting
				// status variables to null will
				// be first in rootfunc, I can safely
				// assume that the gpr %1 and %2
				// are not allocated.
				
				// Lock the lyricalreg for %1 and %2,
				// to prevent newinstruction()
				// from adding them to the list
				// of unused registers of
				// lyricalinstruction.
				(r1 = searchreg(rootfunc, 1))->lock = 1;
				(r2 = searchreg(rootfunc, 2))->lock = 1;
				
				// Here I generate the instruction which will load
				// in %1 the pointer to the global variable page;
				// the following instruction is generated:
				// afip %1, OFFSETTOGLOBALREGION;
				i = newinstruction(rootfunc, LYRICALOPAFIP);
				i->r1 = 1;
				i->imm = mmallocz(sizeof(lyricalimmval));
				i->imm->type = LYRICALIMMOFFSETTOGLOBALREGION;
				
				// I set %2 null; which is then
				// used to set status variables null.
				li(r2, 0);
				
				// Generate as many st instructions as
				// there are status variables to set null.
				while (statusvars.i) st(r2, r1, statusvars.o[--statusvars.i]);
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r1->lock = 0;
				r2->lock = 0;
				
				mmrefdown(statusvars.o);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("end: done"));
				}
				
				// Restore rootfunc->i, making
				// first the instructions setting
				// status variables to null.
				rootfunc->i = savedi;
			}
			
			if (rootfunc->firstpass->sharedregions) {
				// I get here if rootfunc
				// hold the tiny stackframe
				// of its children in
				// its shared region.
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("begin: destroy rootfunc shared region"));
				}
				
				// I generate the instruction:
				// addi %0, %0, 2*sizeofgpr + sharedregionsize;
				// which increment the stack pointer register
				// by the size of the rootfunc stackframe.
				// 2*sizeofgpr account for the offset
				// and stackframe-id fields.
				
				i = newinstruction(rootfunc, LYRICALOPADDI);
				i->r1 = 0;
				i->r2 = 0;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMVALUE;
				imm->n = 2*sizeofgpr;
				i->imm = imm;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = rootfunc;
				i->imm->next = imm;
				
				// Generate instructions which check whether
				// the stack page pointed by the stack pointer
				// register is an excess page which can be freed.
				// 
				// The following instructions are generated:
				// 
				// -------------------------
				// # Compute the size left in the stack page.
				// andi r1, %0, PAGESIZE-1;
				// 
				// # Check whether the size left in
				// # the stack page is (PAGESIZE-sizeofgpr).
				// # If true, it mean that the stack pointer
				// # register is at the bottom of the stack page
				// # where the address to the previous stack page
				// # is written.
				// seqi r1, r1, PAGESIZE-sizeofgpr;
				// 
				// # If the result of the above test is false,
				// # then there is no page to free.
				// jz r1, noneedtofreestack;
				// 
				// # Save the address, which
				// # is within the page to free.
				// cpy r1, %0;
				// 
				// # Load the pointer to the stack page
				// # previous to the one which will be freed.
				// # That pointer is written at the bottom
				// # of the page which will be freed.
				// # Note that when this instruction is executed
				// # the stack pointer register is at the bottom
				// # of the stack page which will be freed.
				// ldr %0, %0;
				// 
				// # Free the page.
				// stackpagefree r1;
				// 
				// noneedtofreestack:
				// -------------------------
				// 
				// Note that before the jump instruction and
				// the label used above, it is not necessary to flush and
				// discard registers, because the branching instruction
				// and label are not used to branch across code
				// written by the programmer.
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("begin: checking for excess stack"));
				}
				
				lyricalreg* r1;
				
				// Since these instructions will be
				// last in rootfunc, I can safely assume
				// that the gpr %1 is not allocated.
				
				// Lock the lyricalreg for %1,
				// to prevent newinstruction()
				// from adding it to the list
				// of unused registers of
				// lyricalinstruction.
				(r1 = searchreg(rootfunc, 1))->lock = 1;
				
				andi(r1, &rstack, PAGESIZE-1);
				
				seqi(r1, r1, PAGESIZE-sizeofgpr);
				
				string labelnamefornoneedtofreestack = stringfmt("%d", newgenericlabelid());
				
				jz(r1, labelnamefornoneedtofreestack);
				
				cpy(r1, &rstack);
				
				ldr(&rstack, &rstack);
				
				stackpagefree(r1);
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r1->lock = 0;
				
				newlabel(labelnamefornoneedtofreestack);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("end: done"));
				}
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("end: done"));
				}
				
				// Save what was the last instruction
				// generated in rootfunc, so as to
				// restore it once all the instructions
				// decrementing the stack pointer
				// register have been generated; and since
				// the linkedlist pointed by rootfunc->i
				// is circular, it make first
				// the generated instructions.
				lyricalinstruction* savedi = rootfunc->i;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("begin: create rootfunc shared region"));
				}
				
				// Generate instructions which check whether
				// there is enough stack left; and if not
				// allocate a new stackpage.
				// 
				// The following instructions are generated:
				// 
				// -------------------------
				// # Compute the size left in the stack page.
				// andi r1, %0, PAGESIZE -1;
				// 
				// # Check whether there is enough stack left.
				// sltui r1, r1, stackneededvalue;
				// 
				// # If the result of the above test is false,
				// # then there is no need to allocate
				// # a new stack page.
				// jz r1, enoughstackleft;
				// 
				// # Allocate a new stack page.
				// stackpagealloc r1;
				// 
				// # Set r1 to the bottom of the stack page
				// # where the current value of the stack
				// # pointer register will be written.
				// addi r1, r1, PAGESIZE-sizeofgpr;
				// 
				// # Store the current value of the stack
				// # pointer register; that allow for keeping
				// # all allocated stack page in a linkedlist.
				// str %0, r1;
				// 
				// # Update the stack pointer register
				// # to the newly allocated stack page.
				// cpy %0, r1;
				// 
				// enoughstackleft:
				// -------------------------
				// 
				// Note that before the jump instruction and
				// the label used above, it is not necessary to flush and
				// discard registers, because the branching instruction
				// and label are not used to branch across code
				// written by the programmer.
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("begin: checking for enough stack"));
				}
				
				// Since these instructions will be
				// first in rootfunc, I can safely assume
				// that the gpr %1 is not allocated.
				
				// Lock the lyricalreg for %1,
				// to prevent newinstruction()
				// from adding it to the list
				// of unused registers of
				// lyricalinstruction.
				(r1 = searchreg(rootfunc, 1))->lock = 1;
				
				andi(r1, &rstack, PAGESIZE-1);
				
				// Check whether there is enough stack left:
				// (2*sizeofgpr + sharedregionsize + stackpageallocprovision)
				i = sltui(r1, r1, 2*sizeofgpr);
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = rootfunc;
				i->imm->next = imm;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMVALUE;
				// To the stack needed amount,
				// I add a value which represent
				// the additional stack space needed
				// by LYRICALOPSTACKPAGEALLOC when
				// allocating a new stack page.
				imm->n = compileargstackpageallocprovision;
				i->imm->next->next = imm;
				
				string labelnameforenoughstackleft = stringfmt("%d", newgenericlabelid());
				
				jz(r1, labelnameforenoughstackleft);
				
				stackpagealloc(r1);
				
				addi(r1, r1, PAGESIZE-sizeofgpr);
				
				str(&rstack, r1);
				
				cpy(&rstack, r1);
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r1->lock = 0;
				
				newlabel(labelnameforenoughstackleft);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("end: done"));
				}
				
				// I generate the instruction:
				// addi %0, %0, -(2*sizeofgpr + sharedregionsize);
				// which decrement the stack pointer register
				// by the size of the rootfunc stackframe.
				// 2*sizeofgpr account for the offset
				// and stackframe-id fields.
				
				i = newinstruction(rootfunc, LYRICALOPADDI);
				i->r1 = 0;
				i->r2 = 0;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMVALUE;
				imm->n = -(u64)2*sizeofgpr;
				i->imm = imm;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMNEGATIVESHAREDREGIONSIZE;
				imm->f = rootfunc;
				i->imm->next = imm;
				
				// In order for the stack space used
				// by the rootfunc shared region to be
				// retrieved from a function for which
				// the address was obtained, a stackframe-id
				// need to be written; note that rootfunc
				// do not quite have a regular stackframe,
				// because the rootfunc stackframe-id is
				// instead set right after the shared region;
				// the offset field right before the shared region
				// is set so that the stackframe-id can be
				// seemlessly retrieved by the instructions
				// that read a stackframe-id from
				// a regular stackframe.
				
				// I can safely assume that gpr %1 is not
				// allocated since no registers are in use.
				
				// Lock the lyricalreg for %1,
				// to prevent newinstruction()
				// from adding them to the list
				// of unused registers of
				// lyricalinstruction.
				(r1 = searchreg(rootfunc, 1))->lock = 1;
				
				// I generate the instructions:
				// li %1, ((sizeofgpr + sharedregionsize) -3*sizeofgpr);
				// str %1, %0;
				// afip %1, OFFSETTOFUNC(rootfunc);
				// st %1, %0, sizeofgpr + sharedregionsize;
				// 
				// Note that -3*sizeofgpr in the first instruction
				// allow the stackframe-id to be seemlessly retrieved
				// by the instructions that read a stackframe-id
				// from a regular stackframe.
				
				i = newinstruction(rootfunc, LYRICALOPLI);
				i->r1 = 1;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMVALUE;
				imm->n = -(u64)2*sizeofgpr;
				i->imm = imm;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = rootfunc;
				i->imm->next = imm;
				
				str(r1, &rstack);
				
				i = newinstruction(rootfunc, LYRICALOPAFIP);
				i->r1 = 1;
				i->imm = mmallocz(sizeof(lyricalimmval));
				i->imm->type = LYRICALIMMOFFSETTOFUNCTION;
				i->imm->f = rootfunc;
				
				i = newinstruction(rootfunc, lyricalopst());
				i->r1 = 1;
				i->r2 = 0;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMVALUE;
				imm->n = sizeofgpr;
				i->imm = imm;
				imm = mmallocz(sizeof(lyricalimmval));
				imm->type = LYRICALIMMSHAREDREGIONSIZE;
				imm->f = rootfunc;
				i->imm->next = imm;
				
				// Unlock lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r1->lock = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					comment(stringduplicate2("end: done"));
				}
				
				// Restore rootfunc->i, making
				// first the instructions decrementing
				// the stackframe pointer register.
				rootfunc->i = savedi;
			}
			
			// When the entire program start,
			// a jpush should be done to the root function
			// which is the entry point of the program;
			// and the jpop instruction created here allow
			// the program to return from the execution
			// of the root function.
			i = newinstruction(rootfunc, LYRICALOPJPOP);
			
			// Here I resolve catchable-labels and
			// labels created within the root function.
			// It is necessary to resolve all the
			// catchable-labels before resolving all the labels.
			// resolvecatchablelabelsnow() and resolvelabelsnow()
			// will free all the lyricalcatchablelabel and lyricallabel
			// of rootfunc since those will no longer be needed.
			
			if (rootfunc->cl) resolvecatchablelabelsnow();
			
			resolvelabelsnow();
			
		} else {
			
			if (compileargcompileflag&LYRICALCOMPILENOFUNCTIONIMPORT) {
				// I make sure that all functions
				// declared within rootfunc were defined.
				f = lastchildofcurrentfunc;
				while (f) { // The first element of the linkedlist of siblings has its field sibling null.
					
					if (!f->i) {
						// I set curpos where the function was declared,
						// before throwing the error message.
						curpos = f->startofdeclaration;
						
						throwerror("function declaration missing definition");
					}
					
					f = f->sibling;
				}
				
			} else {
				// Declared but undefined functions within
				// the root function are set for import,
				// and must be treated as function for which
				// the address is obtained.
				f = lastchildofcurrentfunc;
				while (f) { // The first element of the linkedlist of siblings has its field sibling null.
					
					if (!f->i) {
						
						f->itspointerisobtained = 1;
						
						// In the secondpass, this field
						// will be set to an offset within
						// the global variable region from
						// which is to be retrieved the address
						// of the function being imported.
						// In the firstpass, this field is set
						// to 1, so that within generatefunctioncall(),
						// I call the function to import the same way
						// a function get called through a pointer
						// to function where the stackframe usage
						// cannot be determined at compiled time
						// because its locals usage cannot be estimated.
						f->toimport = 1;
					}
					
					f = f->sibling;
				}
			}
		}
		
		if (rootfunc->t) freetypelinkedlist(rootfunc->t);
		
		rootfunc->children = lastchildofcurrentfunc;
		
		if (scope) {
			
			mmrefdown(scope);
			
			scopecurrent = 0;
			scopedepth = 0;
			scope = 0;
		}
		
		return;
	}
	
	declarationprefix.isset = 0;
	
	// The order in which the tests below are done is crucial.
	if (statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE) goto readvartype;
	else if (currenttype) goto checkstructpstructunionorenum;
	else if (statementparsingflag == PARSEFUNCTIONARG) goto readvartype;
	// Unlike C/C++, a struct/pstruct/union cannot
	// be declared/defined while declaring
	// the arguments of a function.
	
	// Used in this file and within
	// controlstatements.parsestatement.lyrical.c
	// to save the address in curpos.
	u8* savedcurpos;
	
	// Used in this file and within
	// controlstatements.parsestatement.lyrical.c
	// to save the value of *curpos .
	u8 c;
	
	// Control statements are parsed within the file included below.
	#include "controlstatements.parsestatement.lyrical.c"
	
	// If statementparsingflag == PARSESINGLEEXPR,
	// I jump directly where expressions are evaluated,
	// effectively avoiding the tests on declarative statements;
	// in fact declarations should not occur unless
	// a new scope get created; and that would have happened
	// within the file controlstatements.parsestatement.lyrical.c
	// when the opening brace is parsed.
	if (statementparsingflag == PARSESINGLEEXPR) goto evalexpr;
	
	checkdeclarationprefix:
	
	startofdeclaration = curpos;
	
	// Note that a declaration prefix
	// will only be checked if I am not
	// declaring arguments of a function
	// or variable member of a struct
	// or union, because right above
	// there are checks which make jumps
	// skipping this section of the code.
	
	if (checkforkeyword("export")) {
		
		declarationprefix.isexport = 1;
		
		goto checkstructpstructunionorenum;
	}
	
	if (checkforkeyword("static")) {
		
		declarationprefix.isstatic = 1;
		
		goto checkstructpstructunionorenum;
	}
	
	if (checkforkeyword("catch")) {
		// I read the comma separated name of
		// labels and create the catchable-labels.
		while (1) {
			
			if (compilepass) {
				// Labels and catchable-labels are
				// created only in the secondpass.
				
				u8* savedcurpos2 = curpos;
				
				string s = readsymbol(LOWERCASESYMBOL);
				
				if (!s.ptr) {
					reverseskipspace();
					throwerror("expecting a label name");
				}
				
				// Since newcatchablelabel() do not use curpos,
				// I set curpos to savedcurpos2 so that
				// if an error is thrown by newcatchablelabel(),
				// curpos is correctly set to the location
				// of the error.
				swapvalues(&curpos, &savedcurpos2);
				
				newcatchablelabel(s);
				
				// Here I restore curpos.
				swapvalues(&curpos, &savedcurpos2);
				
			} else skipsymbol();
			
			if (*curpos != ',') break;
			
			++curpos; // Set curpos after ',' .
			skipspace();
		}
		
		c = *curpos;
		
		if (c == '}') goto endofstatementclosingbrace;
		else if (c == ';') goto endofstatementsemicolon;
		
		reverseskipspace();
		throwerror("expecting ';' or '}'");
	}
	
	// If I get here, I check if I have a struct/pstruct/union.
	// If I am within the declaration of a struct/pstruct/union,
	// there is always a jump here from startofprocessing
	// to continue the processing of the struct/pstruct/union.
	checkstructpstructunionorenum:
	
	savedcurpos = curpos;
	
	// This variable will point to the type structure of the
	// type to use to declare a new variable or a new function
	// (Where the type pointed by t is the return type).
	lyricaltype* t = 0;
	
	string vartype;
	
	if (checkforkeyword("struct") || checkforkeyword("pstruct") || checkforkeyword("union") || checkforkeyword("enum")) {
		// Variable set to 1
		// if parsing an enum.
		uint parsingenum = (*savedcurpos == 'e');
		
		// Variable set to 1
		// if parsing a struct.
		uint parsingstruct = (*savedcurpos == 's');
		
		// Variable set to 1
		// if parsing a pstruct.
		uint parsingpstruct = (*savedcurpos == 'p');
		
		u8* savedcurpos2 = curpos;
		
		// I read the name of the struct/pstruct/union
		// or enum if there was any name given.
		vartype = readsymbol(LOWERCASESYMBOL);
		
		lyricaltype* bt = 0;
		
		// If not parsing an enum, check if
		// the newly created struct/pstruct/union
		// is being based-off of another type.
		if (!parsingenum && *curpos == ':') {
			
			++curpos;
			
			skipspace();
			
			u8* savedcurpos3 = curpos;
			
			// I read the name of the basetype.
			string basetype = readsymbol(LOWERCASESYMBOL);
			
			if (basetype.ptr) {
				
				if (!(bt = searchtype(basetype.ptr, stringmmsz(basetype), INCURRENTANDPARENTFUNCTIONS))) {
					
					curpos = savedcurpos3;
					
					throwerror("basetype not reachable from this scope");
				}
				
				mmrefdown(basetype.ptr);
				
				if (!bt->size) {
					
					curpos = savedcurpos3;
					
					throwerror("basetype is declared but undefined");
				}
				
			} else {
				
				reverseskipspace();
				
				throwerror("expecting a basetype name");
			}
		}
		
		c = *curpos;
		
		// If I was able to read a type name, I check whether
		// the type was declared or defined. A type that has been
		// declared but not defined has its field size null.
		// Note that an enum type will always need
		// to be defined when declared.
		if (vartype.ptr && (t = searchtype(vartype.ptr, stringmmsz(vartype), INCURRENTSCOPEONLY))) {
			
			if (currenttype) {
				// If I was defining a struct/pstruct/union, and
				// within its definition I create a struct/pstruct/union
				// with the same name as a struct/pstruct/union already
				// declared/defined, I should throw an error.
				
				curpos = savedcurpos2;
				
				throwerror("already in used within this scope");
			}
			
			if (t->size) {
				
				curpos = savedcurpos;
				
				throwerror("a struct/pstruct/union or enum cannot be defined twice within the same scope");
				
			}
			
			if (c != '{') {
				
				curpos = savedcurpos;
				
				// If I do not see '{', it mean that I am just declaring without defining.
				// Note that an enum is always defined when declared.
				throwerror("definition needed because a struct/pstruct/union cannot be declared twice within the same scope");
			}
			
			// The field vartype.ptr is left non-null
			// because later it is tested to determine
			// whether the struct/pstruct/union was unnamed,
			// but the address there is not used.
			mmrefdown(vartype.ptr);
			
		} else {
			
			if (!vartype.ptr && c == '{') {
				// I check whether I am defining
				// an unnamed enum, in which case
				// I check whether it will be
				// declaring something, and
				// if it is not, the type of
				// its elements are to be "uint".
				if (parsingenum) {
					
					u8* savedcurpos3 = curpos;
					
					skipbraceblock();
					
					u8 c = *curpos;
					// Variable set to 1 if
					// the unnamed enum was
					// not declaring anything.
					uint anonymousenum = (c == ';' || c == '}');
					
					curpos = savedcurpos3;
					
					if (anonymousenum) {
						
						string s = largeenoughunsignednativetype(sizeofgpr);
						
						t = searchtype(s.ptr, stringmmsz(s), INCURRENTANDPARENTFUNCTIONS);
						
						goto enumisanonymous;
					}
				}
			}
			
			// Since newtype() do not use curpos,
			// I set curpos to savedcurpos2 so that
			// if an error is thrown by newtype(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos2);
			
			// newtype() will appropriately create a type even if readsymbol()
			// did not read anything and had set vartype to null.
			// When vartype is null, it mean that I am creating an un-named type.
			// The newly created type will be given a name which is the value
			// of curpos at '{' converted to a string.
			t = newtype(vartype);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos2);
			
			// If I created a lyricaltype for an enum,
			// I prefix its name with '#'.
			if (parsingenum) {
				
				stringinsert4(&t->name, '#', 0);
				
				// The size of an enum type is always sizeofgpr.
				t->size = sizeofgpr;
			}
			
			// I jump here when the enum
			// was found to be anonymous.
			enumisanonymous:;
		}
		
		// If I see ';' or '}', I am simply declaring a type, otherwise
		// I expect to see an opening brace sign to define the type.
		// Only struct/pstruct/union can be declared without definition,
		// but a definition is always needed when declaring an enum.
		if (!parsingenum && (c == ';' || c == '}')) {
			
			if (currenttype) throwerror("expecting '{'");
			
			if (c == '}') goto endofstatementclosingbrace;
			
			// When I get here, c == ';' .
			goto endofstatementsemicolon;
			
		} else if (c != '{') {
			
			reverseskipspace();
			throwerror("expecting a valid enum name or '{'");
		}
		
		if (!parsingenum) {
			// If I get here, I parse the definition
			// of a struct/pstruct/union.
			
			// This static variable is used to remember
			// what is the owner of an anonymous struct/pstruct/union
			// when recursively calling parsestatement()
			// to parse nested anonymous struct/pstruct/union.
			static lyricaltype* ownerofanonymoustype;
			
			// Here I save the current value of curpos and
			// skip till after the closing brace of
			// the definition of the struct/pstruct/union in order
			// to check whether it is terminated by a semi-colon.
			
			savedcurpos2 = curpos;
			
			// Skip the struct/pstruct/union definition
			// till its closing brace; it will
			// set curpos after the closing brace
			// of the struct/pstruct/union definition
			// and do skipspace().
			skipbraceblock();
			
			c = *curpos;
			
			if (!vartype.ptr && (c == ';' || c == '}')) {
				// If I get here, I am about to parse the definition
				// of an anonymous struct/pstruct/union.
				t->ownerofanonymoustype = ownerofanonymoustype;
				
			} else {
				// If I get here, I am about to parse the definition
				// of a struct/pstruct/union that is not anonymous.
				ownerofanonymoustype = t;
			}
			
			// Here I restore curpos.
			curpos = savedcurpos2;
			
			++curpos; // Set curpos after '{'.
			skipspace();
			
			lyricaltype* savedcurrenttype = currenttype;
			
			currenttype = t;
			
			// If a basetype was specified make it
			// the first element of the struct/union.
			if (bt) {
				
				currenttype->basetype = bt;
				
				parsestatementflag savedstatementparsingflag = statementparsingflag;
				
				// statementparsingflag is set so that varalloc()
				// create a lyricalvariable member in currenttype.
				statementparsingflag = (parsingstruct ? PARSESTRUCT : parsingpstruct ? PARSEPSTRUCT : PARSEUNION);
				
				// Here, a lyricalvariable member of the struct/union
				// is created as hidden and given the basetype.
				// A lyricalvariable member of a struct/union
				// that is hidden has its field name set to "." .
				// Note that newvarlocal() is not used here (ei:
				// newvarlocal(stringduplicate2("."), stringduplicate1(bt->name));)
				// because if bt->name is the same as the name
				// of the inheriting type pointed by currenttype,
				// the type pointed by currenttype will endup being used,
				// whereas what is wanted as basetype is the type pointed by bt.
				// Another side-effect from not using newvarlocal()
				// (which has a check that prevent creating
				// a variable with the type "void") is that
				// it is possible to inherit from "void" which
				// is useful to create an empty struct/union;
				// since a variable/argument cannot be declared using
				// the type "void", there is certainly no methods
				// to inherit from "void".
				lyricalvariable* v = varalloc(bt->size, DONOTLOOKFORAHOLE);
				v->type = stringduplicate1(bt->name);
				v->name = stringduplicate2(".");
				
				statementparsingflag = savedstatementparsingflag;
			}
			
			// Before calling parsestatement(), I need
			// to make sure that the struct/pstruct/union definition
			// is not an empty definition, otherwise an incorrect
			// error will be thrown.
			if (*curpos == '}') {
				
				++curpos;
				skipspace();
				
			} else {
				// I recursively call parsestatement() and
				// appropriately set the argument statementparsingflag
				// depending on whether I am going to parse a struct/pstruct/union.
				// currenttype has been set to t and contain
				// the address of the type which will represent
				// the struct/pstruct/union I am going to process.
				parsestatement(parsingstruct ? PARSESTRUCT : parsingpstruct ? PARSEPSTRUCT : PARSEUNION);
				// parsestatement() process the entire
				// struct/pstruct/union and set curpos after
				// the closing brace of the struct/pstruct/union
				// and do skipspace() before returning.
			}
			
			// I restore currenttype.
			currenttype = savedcurrenttype;
			
			// This function will compute the size that the linkedlist
			// of variable passed as argument will occupy in memory.
			// The argument passed should be the last element of the linkedlist.
			uint sizeofvarlinkedlist (lyricalvariable* linkedlist) {
				
				lyricalvariable* v = linkedlist;
				
				// This variable is used in order to select the variable member
				// of an union set having the largest size.
				lyricalvariable* savedv = v;
				
				// This loop will go through the variables of
				// the linkedlist pointed by linkedlist.
				while (v->prev != linkedlist) {
					// If while going through the linkedlist, I find a variable
					// member which is at a different offset, it mean that I am going
					// through the variable member of a struct/pstruct, and the size that
					// will be occupied by all the variable member is the offset of
					// the last element of the linkedlist to which its size is added.
					if (v->offset != v->prev->offset) return linkedlist->offset + linkedlist->size;
					
					v = v->prev;
					
					// If I am going through the linkedlist of variable member
					// of an union, everytime that a variable member is found with
					// a larger size, I save it in savedv.
					if (v->size > savedv->size) savedv = v;
				}
				
				// I return the offset plus the size of the largest variable member of
				// an union; or it could also be the only variable member of a struct/pstruct.
				return savedv->offset + savedv->size;
				
				// This function will also return a correct result
				// in a situation where I have a struct/pstruct where
				// all the variable member are part of the same bitfield,
				// which mean that they will all be at the same offset.
			}
			
			// Compute the size that the linkedlist
			// of variables pointed by t->v, created by
			// the recursive call to parsestatement()
			// will occupy in memory.
			if (t->v) t->size = sizeofvarlinkedlist(t->v);
			else {
				curpos = savedcurpos;
				throwerror("at least one member is needed when defining a struct/pstruct/union");
			}
			
			// In order for the alignment done below to work,
			// the value to align-to has to be a multiple of
			// the native type sizes; but the size of a type
			// may not always be a multiple of the native type
			// sizes, because it could contain a 4bytes and 1byte
			// member which would yield the size of the type
			// as 5bytes and it would give incorrect results
			// if I try to use the size 5 to do alignment.
			// Hence I find the next multiple of the native
			// type sizes greater than the size of the type
			// and align to it. Here below is an illustration:
			// struct mytype {
			// 	u32 a;
			// 	u8 b;
			// 	// So here I need a padding that will make it
			// 	// well aligned when used within an array.
			// }[2] myvar;	// Without alignment, the second element
			// 		// of the array will be misalligned.
			// Depend on the highest supported sizeofgpr value.
			if (t->size > 4 && sizeofgpr >= 8) t->size = ROUNDUPTOPOWEROFTWO(t->size, 8);
			else if (t->size > 2 && sizeofgpr >= 4) t->size = ROUNDUPTOPOWEROFTWO(t->size, 4);
			// "else" not needed because it would be evaluated
			// if t->size == 2, which is already a power-of-2;
			// but it is there to show the trend when adding
			// support for higher values of sizeofgpr.
			else if (t->size > 1 && sizeofgpr >= 2) t->size = ROUNDUPTOPOWEROFTWO(t->size, 2);
			
			// When a struct/pstruct/union contain
			// an anonymous struct/pstruct/union, the offset
			// of the lyricalvariable members of an anonymous
			// lyricaltype can be incorrect because they will
			// not be with respect to the lyricaltype containing
			// the anonymous type; hence this function will
			// insure that all member variables
			// have correct offsets.
			void adjustoffsetoftypemembers (lyricaltype* t, uint offset) {
				
				lyricalvariable* v = t->v;
				
				if (!v) return;
				
				// Remember that the field v of a lyricaltype
				// point to its last created variable member.
				do {
					v->offset += offset;
					
					// Here I check whether the lyricalvariable member
					// was created for an anonymous struct/pstruct/union.
					// The name of a lyricalvariable member created for
					// an anonymous struct/pstruct/union is an empty
					// null terminated string.
					// If the lyricalvariable member was created for
					// an anonymous struct/pstruct/union, I recursively
					// call this function passing the offset to be used
					// for adjusting the member variable offsets of
					// the anonymous type.
					// Note that the name of a lyricalvariable member
					// can also be an empty null terminated string
					// if it was a lyricalvariable member used to
					// hold the offset for the next variable member,
					// and it differ by the fact that it always
					// use a native type.
					if (!v->name.ptr[0]) adjustoffsetoftypemembers(searchtype(v->type.ptr, stringmmsz(v->type), INCURRENTANDPARENTFUNCTIONS), v->offset);
					
				} while ((v = v->next) != t->v);
			}
			
			// The offset adjustment of member variables
			// start from a lyricaltype that is not anonymous.
			if (!t->ownerofanonymoustype) adjustoffsetoftypemembers(t, 0);
			
		} else {
			// If I get here, I parse the definition of an enum.
			
			++curpos; // Set curpos after '{'.
			skipspace();
			
			// The definition of an enum cannot be empty.
			if (*curpos == '}') {
				curpos = savedcurpos;
				throwerror("at least one element is needed when defining an enum");
			}
			
			lyricalvariable* v;
			
			lyricalvariable* savedv = 0;
			
			parsestatementflag savedstatementparsingflag;
			lyricaltype* savedcurrenttype;
			
			// When within a struct/pstruct/union, the variable currenttype
			// should be set to null and the variable statementparsingflag
			// should be set to PARSEFUNCTIONBODY to prevent the lyricalvariable
			// for the enum element from being created within the lyricaltype
			// of the struct/pstruct/union by varalloc(); in fact the lyricalvariable
			// for the enum element should be created within the lyricalfunction
			// pointed by currentfunc.
			
			savedstatementparsingflag = statementparsingflag;
			savedcurrenttype = currenttype;
			
			statementparsingflag = PARSEFUNCTIONBODY;
			currenttype = 0;
			
			keepparsingenum:;
			
			savedcurpos2 = curpos;
			
			// I read the name of
			// an enum element which
			// is always uppercase.
			string s = readsymbol(UPPERCASESYMBOL);
			
			if (s.ptr) {
				
				u8* savedcurpos3 = curpos;
				
				// currenttype need to be null before
				// evaluating the enum value expression,
				// so that any call of varalloc() would create
				// lyricalvariable in the correct linkedlist.
				lyricaltype* savedcurrenttype = currenttype;
				
				currenttype = 0;
				
				// I evaluate the expression
				// for the enum value, which
				// must result to an integer.
				lyricalvariable* enumvalue = evaluateexpression(LOWESTPRECEDENCE);
				
				if (linkedlistofpostfixoperatorcall) {
					curpos = savedcurpos3;
					throwerror("expecting an expression with no postfix operations");
				}
				
				// I restore currenttype.
				currenttype = savedcurrenttype;
				
				// Since newvarlocal() do not use curpos,
				// I set curpos to savedcurpos2 so that
				// if an error is thrown by newvarlocal(),
				// curpos is correctly set to the location
				// of the error.
				swapvalues(&curpos, &savedcurpos2);
				
				v = newvarlocal(s, stringnull);
				
				// Here I restore curpos.
				swapvalues(&curpos, &savedcurpos2);
				
				// An enum element value is a constant that
				// is never to occupy memory; hence the field cast
				// of its lyricalvariable get set as opposed to
				// the field type. Note that the cast is never
				// to be changed until the lyricalvariable is freed,
				// otherwise, the enum type to which the enum element
				// is associated to would be lost.
				// readvarorfunc() always return an equivalement
				// lyricalvariable for a constant integer number
				// whenever a lyricalvariable for an enum
				// element is found; hence a lyricalvariable
				// created for an enum element never get used
				// with pushargument() nor casted.
				v->cast = stringduplicate1(t->name);
				
				// Beside the field name and cast of the lyricalvariable
				// created for an enum element, the field numbervalue
				// is set with the integer value of the enum element;
				// that value will be used by readvarorfunc() to find
				// an equivalent lyricalvariable for an integer number.
				
				if (enumvalue) {
					
					if (enumvalue == EXPRWITHNORETVAL || !enumvalue->isnumber) {
						curpos = savedcurpos3;
						throwerror("expecting an expression that result to a constant");
					}
					
					v->numbervalue = enumvalue->numbervalue;
					
				// If there was no expression given
				// for the enum value, the value of
				// the enum element is the incremented
				// value of the previous enum element.
				} else if (savedv) v->numbervalue = (savedv->numbervalue+1);
				
			} else throwerror("expecting a valid uppercase enum element name");
			
			c = *curpos;
			
			if (c == ',') {
				
				++curpos; // Set curpos after ',' .
				skipspace();
				
				if (*curpos != '}') {
					
					savedv = v;
					
					goto keepparsingenum;
				}
				
			} else if (c != '}') {
				
				reverseskipspace();
				throwerror("expecting ',' or '}'");
			}
			
			++curpos; // Set curpos after '}' .
			skipspace();
			
			// I restore the variables currenttype and statementparsingflag.
			currenttype = savedcurrenttype;
			statementparsingflag = savedstatementparsingflag;
		}
		
		c = *curpos;
		
		if (c == ';' || c == '}') {
			// I get here if the newly created and parsed type
			// is not given a name for what it declare.
			
			if (vartype.ptr) {
				// If the newly created and parsed type
				// was named and defined within a struct/pstruct/union,
				// but no name for what it declare is given,
				// a hidden member is created.
				if (currenttype) newvarlocal(stringduplicate2("."), stringduplicate1(t->name));
				
			} else {
				// I get here if an unnamed type was defined.
				if (currenttype) {
					// Here, a lyricalvariable member of the struct/pstruct/union
					// is created and given the type of the anonymous struct/pstruct/union.
					// An lyricalvariable member of a struct/pstruct/union created for
					// an anonymous struct/pstruct/union has its field name
					// set to an empty string.
					newvarlocal(stringduplicate2(""), stringduplicate1(t->name));
					
				} else if (!parsingenum) {
					
					curpos = savedcurpos;
					
					throwerror("an anonymous struct/pstruct/union can only be created within another struct/pstruct/union");
				}
			}
			
			if (c == '}') goto endofstatementclosingbrace;
			
			// When I get here, c == ';' .
			goto endofstatementsemicolon;
		}
		
		// When I get here, t point to the type to use;
		// I can directly jump to readtypespec to read the type specification
		// and have variables or functions defined. ei:
		// struct type { uint var;}* var;	# Is similar to
		// type* var;	# With the only difference that I created a type
		// 		# during declaration when using the keyword struct/pstruct/union.
		// This below is also possible:
		// struct mytype {uint somevar;}[4] myfunc(uint arg) {return;};
		// Which is similar to:
		// mytype[4] myfunc(uint arg) {return;}
		
		goto readtypespec;
	}
	
	// Getting here mean that no other keywords was found
	// and I now check for a written type or label.
	readvartype:
	
	// Save curpos so that I can restore it
	// in case there was no variable or function
	// being declared but simply an expression,
	// since the use of readsymbol() below
	// will modify the value of curpos.
	savedcurpos = curpos;
	
	// I read the type of a declaration
	// which could also happen to be a label.
	vartype = readsymbol(LOWERCASESYMBOL);
	
	if (stringiseq2(vartype, "typeof")) {
		
		if (*curpos != '(') {
			reverseskipspace();
			throwerror("expecting an opening paranthesis");
		}
		
		++curpos; // Set curpos after '(' .
		skipspace();
		
		u8* savedcurpos2 = curpos;
		
		mmrefdown(vartype.ptr);
		
		lyricalvariable* v;
		
		while (1) {
			
			v = evaluateexpression(LOWESTPRECEDENCE);
			
			if (*curpos == ',') {
				
				if (v && v != EXPRWITHNORETVAL) {
					// If I have a tempvar or a lyricalvariable
					// which depend on a tempvar (dereference variable
					// or variable with its name suffixed with an offset),
					// I should free all those variables because
					// I will no longer need them and it allow
					// the generated code to save stack memory.
					varfreetempvarrelated(v);
				}
				
				++curpos; // Set curpos after ',' .
				skipspace();
				
				savedcurpos2 = curpos;
				
			} else break;
		}
		
		// evaluateexpression() return null if nothing
		// like an expression was evaluated, but will
		// return EXPRWITHNORETVAL if an expression
		// was evaluated but no return variable was produced
		// or will return a non-null address containing a resulting
		// lyricalvariable from the evaluation of the expression.
		if (!v || v == EXPRWITHNORETVAL) {
			
			curpos = savedcurpos2;
			
			reverseskipspace();
			
			// The error message below say expecting casting.
			// In fact I got here because what I found was not
			// a cast and should have been an expression.
			// Which is not the case.
			throwerror("expecting an expression");
		}
		
		vartype = v->cast.ptr ?
			stringduplicate1(v->cast) :
			stringduplicate1(v->type);
		
		// If I have a tempvar or a lyricalvariable
		// which depend on a tempvar (dereference variable
		// or variable with its name suffixed with an offset),
		// I should free all those variables because
		// I will no longer need them and it allow
		// the generated code to save stack memory.
		varfreetempvarrelated(v);
		
		if (*curpos != ')') {
			reverseskipspace();
			throwerror("expecting a closing paranthesis");
		}
		
		++curpos; // Set curpos after ')'.
		skipspace();
		
		goto typespecparsed;
		
	// Here I check whether I have a label.
	} else if (statementparsingflag != PARSEPOINTERTOFUNCTIONTYPE &&
		statementparsingflag != PARSEFUNCTIONSIGNATURE &&
		statementparsingflag != PARSEFUNCTIONARG &&
		statementparsingflag != PARSESTRUCT &&
		statementparsingflag != PARSEPSTRUCT &&
		statementparsingflag != PARSEUNION &&
		!declarationprefix.isset && vartype.ptr && *curpos == ':') {
		
		// Instructions, labels and catchable-labels
		// are created only in the secondpass.
		if (compilepass) {
			// I need to call flushanddiscardallreg() before creating
			// a new label since jump-instructions will jump to the label
			// and all registers need to be un-used when I start parsing
			// the code coming after the label.
			flushanddiscardallreg(FLUSHANDDISCARDALL);
			
			// Since newlabel() do not use curpos,
			// I set curpos to savedcurpos so that
			// if an error is thrown by newlabel(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos);
			
			newlabel(vartype);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos);
			
		} else mmrefdown(vartype.ptr);
		
		++curpos; // Set curpos after ':'
		skipspace();
		
		c = *curpos;
		
		if (c == '}') goto endofstatementclosingbrace;
		else if (c == ';') goto endofstatementsemicolon;
		
		goto startofprocessing;
	}
	
	if (vartype.ptr) {
		
		searchsymbolresult r = searchsymbol(vartype, INCURRENTANDPARENTFUNCTIONS);
		
		mmrefdown(vartype.ptr);
		
		if (r.s == SYMBOLISTYPE) t = r.t;
		else t = 0;
		
	} else t = 0;
	
	if (!t) {
		// If I get here then either a valid symbol
		// which should be a type could not be read,
		// or a symbol was read but it was not
		// a known or defined type.
		
		// I restore curpos to what it was
		// before calling readsymbol().
		curpos = savedcurpos;
		
		// If I parsed a declaration prefix or was parsing a type
		// or was within a struct/pstruct/union or parsing function arguments,
		// I should throw an error because a valid type is expected.
		if (declarationprefix.isset ||
			statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE ||
			statementparsingflag == PARSEFUNCTIONSIGNATURE ||
			statementparsingflag == PARSEFUNCTIONARG ||
			statementparsingflag == PARSESTRUCT ||
			statementparsingflag == PARSEPSTRUCT ||
			statementparsingflag == PARSEUNION) throwerror("expecting a valid type name");
		
		// If I get here there should be an expression
		// which can start by a number or paranthesis; it is
		// an explaination to why vartype.ptr can be null.
		
		evalexpr:;
		
		while (1) {
			
			lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
			
			if (v && v != EXPRWITHNORETVAL) {
				// If I have a tempvar or a lyricalvariable
				// which depend on a tempvar (dereference variable
				// or variable with its name suffixed with an offset),
				// I should free all those variables because
				// I will no longer need them and it allow
				// the generated code to save stack memory.
				varfreetempvarrelated(v);
			}
			
			if (*curpos == ',') {
				
				++curpos; // Set curpos after ',' .
				skipspace();
				
			} else break;
		}
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		c = *curpos;
		
		if (c == '}') goto endofstatementclosingbrace;
		else if (c == ';') goto endofstatementsemicolon;
		
		// If curpos == savedcurpos, then
		// evaluateexpression() did not parse anything.
		if (curpos == savedcurpos) throwerror("unexpected character");
		
		goto startofprocessing;
	}
	
	readtypespec:
	
	// When I get here I am guaranteed that
	// the type name in vartype is valid.
	
	// If the type in vartype is an enum,
	// its name would be prefixed by '#';
	// so here, I insure that I use the
	// correct type name from the lyricaltype.
	vartype = stringduplicate1(t->name);
	
	if (stringiseq2(vartype, "void")) {
		// If the type read is "void" I read the type specification
		// only if it is a pointer (ei: void*[3]),
		// because a type such as void[3] is incorrect.
		if (*curpos == '*') parsetypespec(&vartype);
		
	} else if (t->size) parsetypespec(&vartype);
	else {
		// When I get here, the type in vartype has only
		// been declared but not defined and can only
		// be used as a pointer to declare another variable
		// or a member field of a struct/pstruct/union.
		if (*curpos == '*') parsetypespec(&vartype);
		else {
			reverseskipspace();
			throwerror("a declared but undefined struct/pstruct/union can only be used as a pointer");
		}
	}
	
	typespecparsed:;
	
	// Here I parse a pointer to function
	// if any and append it to vartype.
	if (*curpos == '(') {
		// If I get here, I have parsed a type which
		// represent a return type since it is before '(';
		
		if (statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE ||
			statementparsingflag == PARSEFUNCTIONSIGNATURE) {
			
			stringappend1(&containparsedstring, vartype);
			
			// Since I used the string in vartype I can free it.
			mmrefdown(vartype.ptr);
			// There is no need to set vartype to null.
			
			// I recurse into parsestatement using
			// the flag PARSEPOINTERTOFUNCTIONTYPE in order
			// to keep appending to containparsedstring,
			// the type string that I parse.
			// The recursive call to parsestatement() below
			// will return after a closing paranthesis and will
			// do a skipspace() before returning.
			// And if I see another '(', I recursively
			// call parsestatement() again.
			do {
				++curpos; // Set curpos after '(' .
				skipspace();
				
				stringappend4(&containparsedstring, '(');
				
				// I should call parsestatement() only
				// if there is something to parse.
				if (*curpos != ')') parsestatement(PARSEPOINTERTOFUNCTIONTYPE);
				else {
					stringappend4(&containparsedstring, ')');
					
					++curpos; // Set curpos after ')'.
					skipspace();
				}
				
				parsetypespec(&containparsedstring);
				
			} while (*curpos == '(');
			
			goto keepparsingtype;
			
		} else {
			
			containparsedstring = vartype;
			
			// The recursive call to parsestatement() below
			// will return after a closing paranthesis and
			// will do a skipspace() before returning.
			// And if I see another '(', I recursively
			// call parsestatement() again.
			do {
				++curpos; // Set curpos after '(' .
				skipspace();
				
				stringappend4(&containparsedstring, '(');
				
				// I should call parsestatement() only
				// if there is something to parse.
				if (*curpos != ')') parsestatement(PARSEPOINTERTOFUNCTIONTYPE);
				else {
					stringappend4(&containparsedstring, ')');
					
					++curpos; // Set curpos after ')'.
					skipspace();
				}
				
				parsetypespec(&containparsedstring);
				
			} while (*curpos == '(');
			
			vartype = containparsedstring;
			
			// From here I am done parsing for the pointer to function type.
		}
		
	} else if (statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE ||
		statementparsingflag == PARSEFUNCTIONSIGNATURE) {
		
		if (stringiseq2(vartype, "void")) {
			curpos = savedcurpos;
			throwerror("an argument to function cannot be void");
		}
		
		stringappend1(&containparsedstring, vartype);
		
		// Since I used the string in vartype I can free it.
		mmrefdown(vartype.ptr);
		// There is no need to set vartype to null.
		
		// Parse for '&' only here make it so that
		// it be always used for an argument and never
		// for the return variable. It work because
		// when I get here, I expect to see a comma,
		// "..." or a closing paranthesis, and all of
		// those can only be found between the paranthesis
		// for the arguments of a function.
		if (*curpos == '&') {
			
			++curpos; // Set curpos after '&'.
			skipspace();
			
			// Using '&' while parsing a function signature is ignored.
			if (statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE)
				stringappend4(&containparsedstring, '&');
		}
		
		keepparsingtype:
		
		// When writting the specification of a type
		// for a pointer to function, I allow for symbol name
		// to be written after the type of the argument.
		// It allow code to be more clear.
		// ei: Here is an example written in C of a function
		// which takes as argument a pointer to function
		// to be used as callback function:
		// void func(u8* arg1, uint arg2, void (*error)(u8* filename, uint offset, u8* msg));
		// Note that the code is more clear because I have
		// an idea of the purpose of the arguments expected
		// by the callback function.
		// So in Lyrical I am able to do the same:
		// ei: Here is the same declaration written in Lyrical.
		// void func(u8* arg1, uint arg2, void(u8* filename, uint offset, u8* msg) error);
		// The symbol strings written after the type is
		// ignored and will only be used by the programmer
		// for clarification; And the type of the argument
		// pointer to function will yield "(u8*,uint,u8*)"
		// So the code below is meant to ignore the symbol
		// string coming after the type; I used a modified
		// version of the code found in readsymbol().
		skipsymbol();
		
		if (*curpos == ',') {
			
			++curpos; // Set curpos after ','.
			skipspace();
			
			stringappend4(&containparsedstring,
				(statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE) ?
				',' : '|');
			
			// Here I check if I have "..." for variadic arguments,
			// after I parse a comma; Which will guaranty that
			// I see "..." after a comma as it should.
			if (curpos[0] == '.' && curpos[1] == '.' && curpos[2] == '.') {
				
				if (statementparsingflag == PARSEFUNCTIONSIGNATURE)
					throwerror("expecting a valid type name");
				
				curpos += 3; // Set curpos after "...".
				
				savedcurpos = curpos;
				
				skipspace();
				
				if (*curpos != ')') {
					curpos = savedcurpos;
					throwerror("expecting a closing paranthesis after \"...\"");
				}
				
				stringappend2(&containparsedstring, "...)");
				
				++curpos; // Set curpos after ')'.
				skipspace();
				
				return;
				
			} else goto readvartype; // I jump where the next type is to be read.
			
		} else if (*curpos == ')') {
			
			++curpos; // Set curpos after ')'.
			skipspace();
			
			stringappend4(&containparsedstring,
				(statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE) ?
				')' : '|');
			
			return;
			
		} else {
			reverseskipspace();
			throwerror("expecting a comma or closing paranthesis");
		}
	}
	
	if (linkedlistofpostfixoperatorcall) {
		// This call is meant for processing
		// postfix operations if there was any.
		evaluateexpression(DOPOSTFIXOPERATIONS);
	}
	
	// Keep in mind that, I can never get here
	// with statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE.
	// Only the code between the label readvartype
	// and here, worry about whether
	// statementparsingflag == PARSEPOINTERTOFUNCTIONTYPE;
	// I do not have to worry about it anywhere else.
	
	savedcurpos = curpos;
	
	if (checkforkeyword("operator")) {
		// The keyword static cannot be used when declaring an operator.
		if (declarationprefix.isstatic) {
			curpos = startofdeclaration;
			throwerror("incorrect use of the keyword static");
		}
		
		// Operator declaration is not allowed within
		// a struct/pstruct/union or while declaring
		// arguments to a function.
		if (statementparsingflag == PARSEFUNCTIONARG ||
			statementparsingflag == PARSESTRUCT ||
			statementparsingflag == PARSEPSTRUCT ||
			statementparsingflag == PARSEUNION) {
			
			curpos = savedcurpos;
			
			throwerror("incorrect use of the keyword operator");
		}
		
		pamsynmatched msr;
		
		// The argument size of pamsynmatch2() is set
		// to 3 because it is the size of the largest operator
		// which is "<<=" or ">>=".
		// The code being parsed is always terminated
		// by null characters, so pamsynmatch2() will never cause
		// a pagefault if curpos was already at the null character.
		if (!(msr = pamsynmatch2(overloadableop, curpos, 3)).start) {
			// +8 account for the size of
			// the keyword "operator".
			curpos = savedcurpos +8;
			throwerror("expecting an overloadable operator");
		}
		
		// This set curpos after the last character
		// of the operator because the field end of
		// the pamsynmatch2() result is set to
		// the last non-hidden character of what was matched.
		curpos = msr.end + 1;
		
		skipspace();
		
		if (*curpos != '(') {
			reverseskipspace();
			throwerror("expecting '(' in order to declare the arguments of the overloadable operator");
		}
		
		// I call funcdeclaration() to read the
		// entire operator declaration or definition.
		// funcdeclaration() return after setting curpos
		// after the closing brace sign '}', or the
		// arguments declaration closing paranthesis ')'
		// if it was just a function declaration.
		funcdeclaration(vartype, stringduplicate3(msr.start, (uint)msr.end - (uint)msr.start +1), OPERATORDECLARATION);
		
		c = *curpos;
		
		if (c == '}') goto endofstatementclosingbrace;
		else if (c == ';') goto endofstatementsemicolon;
		
		goto startofprocessing;
	}
	
	// If I get here, then I expect to read a variable name
	// or it could also be a function name.
	
	// This variable will be set to 1 if I am declaring
	// a byref variable otherwise it will be set to 0.
	uint isbyref;
	
	// Before reading the variable or function name,
	// I check if I have '&' for the declaration
	// of a byref variable.
	if (*curpos == '&') {
		
		++curpos; // Set curpos after '&'.
		
		skipspace();
		
		// The creation of a byref element of
		// a struct/pstruct/union is not allowed.
		// It will prevent the creation of byref bitfields
		// since bitfiels are variable members of a struct.
		if (currenttype) {
			curpos = savedcurpos;
			throwerror("declaration of a byref element of struct/pstruct/union is incorrect");
		}
		
		// A variable declared as byref cannot
		// be declared as void, because automatically
		// dereferencing "void*" will yield "void"
		// which is an incorrect type.
		if (stringiseq2(vartype, "void")) {
			curpos = savedcurpos;
			throwerror("a byref variable cannot be declared as void");
		}
		
		// Because I am declaring a byref variable
		// I need to add an asterix to its type so that
		// after it get automatically dereferenced by
		// the compiler whenever it is read, I obtain
		// the type with which it was declared.
		stringappend4(&vartype, '*');
		
		isbyref = 1;
		
	} else isbyref = 0;
	
	savedcurpos = curpos;
	
	// I read a variable name or
	// it could also be a function name.
	string varname = readsymbol(LOWERCASESYMBOL);
	
	if (!varname.ptr) {
		// If the type was used within a struct/pstruct/union,
		// but no name for what it declare is given,
		// a hidden member is created.
		if (currenttype) varname = stringduplicate4('.');
		else {
			reverseskipspace();
			
			throwerror("expecting a declaration name");
		}
	}
	
	// When I get here, I check if
	// I was declaring a function.
	if (*curpos == '(') {
		// If isbyref is set, it mean that
		// the return type of the function
		// to declared was written as a byref type,
		// which is wrong because it do not make sens
		// for a function to return a value byref.
		if (isbyref) {
			curpos = savedcurpos;
			throwerror("the return type of a function cannot be byref");
		}
		
		// The keyword static cannot be used
		// when declaring a function.
		if (declarationprefix.isstatic) {
			curpos = startofdeclaration;
			throwerror("incorrect use of the keyword static");
		}
		
		// Function declaration is not allowed within
		// a struct/pstruct/union or while declaring
		// arguments to a function.
		if (statementparsingflag == PARSEFUNCTIONARG ||
			statementparsingflag == PARSESTRUCT ||
			statementparsingflag == PARSEPSTRUCT ||
			statementparsingflag == PARSEUNION) throwerror("incorrect location for declaring a function");
		
		if (!compilepass) {
			// It is only necessary to get here in the firstpass.
			
			// I check if the name of the function conflict with a keyword.
			if (pamsynmatch2(iskeyword, varname.ptr, stringmmsz(varname)).start) {
				curpos = savedcurpos;
				throwerror("function name conflicting with a keyword");
			}
			
			searchsymbolresult r = searchsymbol(varname, INCURRENTSCOPEONLY);
			
			// I check if a type or variable was not
			// already declared within the current scope.
			if (r.s == SYMBOLISTYPE || r.s == SYMBOLISVARIABLE) {
				curpos = savedcurpos;
				throwerror("symbol already declared within this scope");
			}
		}
		
		// I call funcdeclaration() to read the
		// entire function declaration or definition.
		// funcdeclaration() return after setting curpos
		// after the closing brace sign '}', or the
		// arguments declaration closing paranthesis ')'
		// if it was just a function declaration.
		funcdeclaration(vartype, varname, FUNCTIONDECLARATION);
		
		c = *curpos;
		
		if (c == '}') goto endofstatementclosingbrace;
		else if (c == ';') goto endofstatementsemicolon;
		
		goto startofprocessing;
		
	} else if (declarationprefix.isexport) {
		// The keyword export can only be used
		// when defining a function/operator.
		curpos = startofdeclaration;
		throwerror("keyword \"export\" can only be used with function/operator definition");
	}
	
	createvar:;
	
	// If there is a bitfield parsed,
	// bitfieldvalueread will contain
	// the decimal value specified after ':'.
	// bitfieldoffset will contain an offset which
	// is where the bitfield is located.
	// ei: A bitselect of 0b1111000 for a u8; which has 4 ones
	// starting at the 6th digit from the msb, had
	// an offset of 6 and the value of bitfieldvalueread was 4.
	// bitfieldtypesize is the total number of bits that
	// the native type used to declare the bitfield can contain.
	// bitfieldbitselect will contain the final bitselect value.
	u64 bitfieldvalueread, bitfieldoffset, bitfieldtypesize, bitfieldbitselect;
	
	// I check if I have a colon used
	// when declaring a bitfield.
	if (*curpos == ':') {
		// There is no need to check whether
		// isbyref was set because I previously
		// already made that check and insured that
		// a byref variable element of a struct
		// or union do not get allowed.
		
		if (statementparsingflag != PARSESTRUCT && statementparsingflag != PARSEPSTRUCT)
			throwerror("bitfields can only be used within a struct/pstruct");
		
		if (!pamsynmatch2(isnativetype, vartype.ptr, stringmmsz(vartype)).start)
			throwerror("bitfields can only be used with native types");
		
		++curpos; // Set curpos after ':' .
		
		u8* savedcurpos2 = curpos;
		
		skipspace();
		
		// currenttype need to be null before
		// evaluating the bitfield size expression,
		// so that any call of varalloc() would create
		// lyricalvariable in the correct linkedlist.
		lyricaltype* savedcurrenttype = currenttype;
		
		currenttype = 0;
		
		// I evaluate the bitfield size
		// expression, which must result
		// in a non-null constant.
		lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
		
		if (linkedlistofpostfixoperatorcall) {
			curpos = savedcurpos2;
			throwerror("expecting an expression with no postfix operations");
		}
		
		if (!v || v == EXPRWITHNORETVAL || !v->isnumber || !v->numbervalue) {
			curpos = savedcurpos2;
			throwerror("expecting an expression that result in a non-null constant");
		}
		
		// I restore currenttype.
		currenttype = savedcurrenttype;
		
		bitfieldvalueread = v->numbervalue;
		
		// bitfieldtypesize and bitfieldoffset
		// are reset if I am starting a new bitfield,
		// or if the bitfield that I am about to
		// create cannot fit inside the native type
		// that was being filled with bitfields.
		// Keep in mind that currenttype->v point
		// to the last created member variable.
		if (!currenttype->v || !currenttype->v->bitselect || !stringiseq1(currenttype->v->type, vartype) ||
			bitfieldvalueread > bitfieldoffset) {
			
			// Note that searchtype() only work
			// with type name that do not contain type specifications
			// such as arrays or '*' for pointers; but since
			// the type of a bitfield is always a native type
			// with no array specification, it is much faster
			// to use searchtype() instead of sizeoftype().
			// The size of a type is in byte, so to find
			// the total number of bits I multiply by 8.
			bitfieldtypesize = 8*searchtype(vartype.ptr, stringmmsz(vartype), INCURRENTANDPARENTFUNCTIONS)->size;
			
			// Bitfields are allocated within
			// an integer from the most-significant
			// to the least-significant bit.
			bitfieldoffset = bitfieldtypesize;
			
			// I need to created a lyricalvariable member which
			// will hold the offset for the first variable
			// member of the newly started bitfield; otherwise
			// the first variable member of the newly
			// started bitfield will be created at the same
			// offset as a previously created variable member
			// because bitfield members are created setting
			// statementparsingflag to PARSEUNION.
			// The name of a lyricalvariable member which is used
			// to hold the offset for the next variable
			// member has an empty null terminated string
			// and the value in the field size of such
			// lyricalvariable is the same as the value in
			// the field size of the lyricalvariable member
			// for which it is holding the offset.
			newvarlocal(stringduplicate2(""), stringduplicate1(vartype));
		}
		
		// The test below has to come after
		// the above if() clause to insure that
		// bitfieldtypesize hold or is initialized
		// with a valid value.
		if (bitfieldvalueread >= bitfieldtypesize) {
			
			curpos = savedcurpos2;
			
			throwerror("a bitfield value cannot be greater than or equal to the number of bits in the type used to declare it");
		}
		
		// bitfieldoffset is updated to the offset
		// where the bitfield is to be created
		// within the integer.
		// Keep in mind that bitfields are allocated
		// within an integer from the most-significant
		// to the least-significant bit.
		bitfieldoffset -= bitfieldvalueread;
		
		// The bitfieldbitselect is created here and
		// shifted to its appropriate offset within
		// the variable bitselect that I will create.
		bitfieldbitselect = (1<<bitfieldvalueread)-1;
		bitfieldbitselect <<= bitfieldoffset;
		
	// It is necessary to set bitfieldbitselect
	// to null in case it had been set by a previous
	// reading of a bitfield size from a previous
	// reading of a declaration.
	} else bitfieldbitselect = 0;
	
	// I create the variable.
	
	lyricalvariable* v;
	
	// Since newvararg(), newvarlocal() or newvarglobal()
	// do not use curpos, I set curpos to savedcurpos so that
	// if an error is thrown within those functions,
	// curpos is correctly set to the location of the error.
	swapvalues(&curpos, &savedcurpos);
	
	if (statementparsingflag == PARSEFUNCTIONARG) v = newvararg(varname, vartype);
	else if (declarationprefix.isstatic) {
		// Static variables are created within
		// the root function, only useable within
		// the scope where they were declared,
		// and their initialization instructions,
		// if any, executed only once for
		// the duration of the program.
		v = newvarglobal(varname, vartype);
		
		v->isstatic = 1;
		
	} else {
		// If bitfieldbitselect is set,
		// I am certainly within a struct/pstruct.
		// If bitfieldbitselect is not set,
		// then I am either within
		// a struct/pstruct/union or function.
		if (bitfieldbitselect) {
			// When creating a bitfield variable member
			// of a struct/pstruct, I temporarily change
			// the statementparsingflag to PARSEUNION
			// in order to have the bitfield variable members
			// start at the same offset when calling newvarlocal().
			
			parsestatementflag savedstatementparsingflag = statementparsingflag;
			
			statementparsingflag = PARSEUNION;
			
			v = newvarlocal(varname, vartype);
			
			statementparsingflag = savedstatementparsingflag;
			
			v->bitselect = bitfieldbitselect;
			
		} else v = newvarlocal(varname, vartype);
	}
	
	// I use the value of curpos and set it
	// in the field id of the newly created variable.
	// Because variable declaration is parsed
	// the same way in the first and second pass,
	// the id value will be the same both
	// in the first and second pass.
	v->id = (uint)curpos;
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The field byref of the newly created variable
	// is set to whatever the variable isbyref was set to.
	// Remember that isbyref will not be set if
	// I am within a struct/pstruct/union.
	v->isbyref = isbyref;
	
	// I now parse for initialization if any.
	// If I was declaring an argument to function or
	// a struct/pstruct/union member, this will be skipped
	// since that type of variable cannot be initialized.
	if (statementparsingflag != PARSEFUNCTIONARG &&
		statementparsingflag != PARSESTRUCT &&
		statementparsingflag != PARSEPSTRUCT &&
		statementparsingflag != PARSEUNION &&
		(*curpos == '=' || *curpos == '.' || *curpos == '{'))
		readinit(v);
	
	// After the declaration (and initialization
	// if there was any) of the variable,
	// I determine what to do next.
	
	c = *curpos;
	
	if (c == ',') {
		
		++curpos; // Set curpos after ',' .
		skipspace();
		
		// If I was declaring arguments to a function,
		// I jump to readvartype to read the next argument
		// to declare unless the variadic symbol is found.
		// If the variadic symbol is found, I mark currentfunc
		// as variadic and I expect to see a closing paranthesis.
		// Because I am doing the check for "..." here after
		// the check for ',' , it enforce the fact that variadic
		// arguments need at least one real argument,
		// because ',' is found only after an argument
		// has already been defined.
		if (statementparsingflag == PARSEFUNCTIONARG) {
			
			if (curpos[0] == '.' && curpos[1] == '.' && curpos[2] == '.') {
				
				currentfunc->isvariadic = 1;
				
				curpos += 3; // Set curpos after "..." .
				
				savedcurpos = curpos;
				
				skipspace();
				
				if (*curpos != ')') {
					curpos = savedcurpos;
					throwerror("expecting a closing paranthesis after \"...\"");
				}
				
				++curpos; // Set curpos after ')' .
				skipspace();
				
				return;
				
			} else goto readvartype;
		}
		
		// If I get here I don't need to read the type again,
		// I just read the next variable name.
		
		savedcurpos = curpos;
		
		varname = readsymbol(LOWERCASESYMBOL);
		
		if (!varname.ptr) {
			// If the type was used within a struct/pstruct/union,
			// but no name for what it declare is given,
			// a hidden member is created.
			if (currenttype) varname = stringduplicate4('.');
			else {
				reverseskipspace();
				
				throwerror("expecting a declaration name");
			}
		}
		
		// I duplicate the string in vartype since
		// it get used by newvar*() functions without
		// being duplicated and it is necessary for
		// variables created to have a unique string
		// in their field type otherwise I could end up
		// freeing that string twice when for example
		// I am freeing variables which were used
		// within a scope that I am exiting.
		vartype = stringduplicate1(vartype);
		
		goto createvar;
		
	} else if (c == ';' || c == '}') {
		// The character ';' or '}' should not be
		// found while I am reading function arguments,
		// which should be separated by ',' .
		if (statementparsingflag == PARSEFUNCTIONARG) throwerror("expecting ',' or ')'");
		
		if (c == '}') goto endofstatementclosingbrace;
		
		// When I get here, c == ';' .
		goto endofstatementsemicolon;
		
	} else if (statementparsingflag == PARSEFUNCTIONARG && c == ')') {
		// If I get here I have reached the end of
		// the declaration of arguments of a function.
		++curpos; // Set curpos after ')' .
		skipspace();
		
		// There is no need to check if
		// statementparsingflag == PARSEFUNCTIONBODY
		// in order to jump to doneparsingfunctiondefinition
		// because statementparsingflag is certainly
		// not set to PARSEFUNCTIONBODY since I got here
		// because statementparsingflag == PARSEFUNCTIONARG.
		
		return;
	}
	
	// If I get here it is an error.
	
	// If I was reading arguments of a function,
	// I was expecting to see a comma separating
	// the argument or a closing ')' to end
	// the declaration of arguments.
	// If I was within the definition of
	// a struct/pstruct/union or a function,
	// I was expecting a comma or semi-colon.
	// If I was within the definition of a function,
	// I was expecting a comma, semi-colon or
	// an initialization.
	
	reverseskipspace();
	
	if (statementparsingflag == PARSEFUNCTIONARG) throwerror("expecting ',' or ')'");
	else throwerror("expecting ',' or ';' or '}'");
	
	
	doneparsingfunctiondefinition:
	// I get here only after '}' has been parsed
	// and statementparsingflag == PARSEFUNCTIONBODY;
	// and I should get here only when I reached the end
	// of the definition of a function and I will be returning
	// from the processing of the body of a function.
	// Among other things, I do the following:
	// I resolve all catchable-labels if currentfunc->cl is non-null.
	// I resolve all labels if linkedlistoflabeledinstructiontoresolve is non-null.
	// I call flushanddiscardallreg().
	// I free the linkedlist of lyricaltype created while parsing the function.
	// I set the field children to the last child lyricalfunction created.
	
	// If I am within the root function,
	// then '}' was unexpectedly used.
	if (currentfunc == rootfunc) {
		reverseskipspace();
		--curpos;
		throwerror("unexpected");
	}
	
	// Adjust curpos so to log correct location.
	reverseskipspace();
	
	// Labels, catchable-labels and instructions are created only in the secondpass.
	if (compilepass) {
		// I call flushanddiscardallreg() with its argument flag
		// set to DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR since
		// I will be generating the instructions returning from currentfunc.
		// Those instructions are created within funcdeclaration().
		flushanddiscardallreg(DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR);
		
		// It is necessary to resolve all the catchable-labels
		// before resolving all the labels.
		// resolvecatchablelabelsnow() and resolvelabelsnow() will free
		// all the lyricalcatchablelabel and lyricallabel of currentfunc
		// since those will no longer be needed.
		
		if (currentfunc->cl) resolvecatchablelabelsnow();
		
		resolvelabelsnow();
		
	} else {
		// I need to make sure that all
		// functions declared within the function
		// that I parsed were defined.
		lyricalfunction* f = lastchildofcurrentfunc;
		while (f) { // The first element of the linkedlist of siblings has its field sibling null.
			
			if (!f->i) {
				// I set curpos where the function was declared,
				// before throwing the error message.
				curpos = f->startofdeclaration;
				
				throwerror("function declaration missing definition");
			}
			
			f = f->sibling;
		}
	}
	
	if (currentfunc->t) freetypelinkedlist(currentfunc->t);
	
	currentfunc->children = lastchildofcurrentfunc;
}
