
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


while (stringiseq4(curpos, "#+e", 3) || stringiseq4(curpos, "#-e", 3)) {
	isexportinferred = (curpos[1] == '+');
	curpos += 3;
	skipspace();
}

if (*curpos == '{') {
	// If I get here, I am entering a new
	// scope created by opening a brace.
	
	++curpos; // Set curpos after '{' .
	skipspace();
	
	// If (*curpos == '}') then
	// there is no block to process.
	if (*curpos != '}') {
		// Save currentswitchblock before setting it
		// to null, because I could be currently within
		// a switch() block. Upon completing the parsing
		// of the switch() block, I will restore it.
		// I set currentswitchblock to null because
		// case and default statements must be
		// within the scope of their corresponding
		// switch() block and not a sub-scope.
		switchblock* savedcurrentswitchblock = currentswitchblock;
		currentswitchblock = 0;
		
		scopeentering();
		
		// parsestatement() is recursively called
		// to process the block; parsestatement()
		// process the entire block and set curpos
		// after the closing brace of the block
		// and do skipspace() before returning.
		parsestatement(PARSEBLOCK);
		
		scopeleaving();
		
		// I restore currentswitchblock.
		currentswitchblock = savedcurrentswitchblock;
		
	} else {
		
		++curpos; // Set curpos after '}' .
		skipspace();
	}
	
	endofstatement:
	
	if (statementparsingflag == PARSESINGLEEXPR) return;
	
	c = *curpos;
	
	if (c == '}') {
		// If I get here I have reached the end
		// of a function or control statement block
		// being processed and I should return.
		
		endofstatementclosingbrace:
		
		++curpos; // Set curpos after '}' .
		skipspace();
		
		// If statementparsingflag == PARSEFUNCTIONBODY
		// then I reached the end of the definition of
		// a function and I will be returning from
		// the processing of the body of a function.
		if (statementparsingflag == PARSEFUNCTIONBODY) goto doneparsingfunctiondefinition;
		
		return;
		
	} else if (c == ';') {
		// Semi-colon is used to separate two statements.
		
		endofstatementsemicolon:
		
		++curpos; // Set curpos after ';' .
		skipspace();
		
		goto endofstatement;
	}
	
	// Jump to startofprocessing to keep
	// processing the current function if
	// not within a struct/pstruct/union,
	// otherwise jump to checkstructpstructunionorenum
	// to keep processing the struct/pstruct/union.
	if (currenttype) goto checkstructpstructunionorenum;
	else goto startofprocessing;
}

savedcurpos = curpos;

if (checkforkeyword("goto")) {
	// The goto statement should only be used with
	// a label not with a variable or anything else
	// because I jump to location where flushanddiscardallreg()
	// was called and registers were in a known state.
	
	// Read the label name to which to jump to.
	string labelname = readsymbol(LOWERCASESYMBOL);
	
	if (!labelname.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I flush and discard all registers since I am branching.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I generate the jump instruction.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		// Set curpos to savedcurpos so that
		// resolvelabellater() correctly set
		// the field pos of the newly created
		// lyricallabeledinstructiontoresolve.
		swapvalues(&curpos, &savedcurpos);
		
		// The address where I have to jump to, is resolved later.
		// resolvelabellater() will make a duplicate of the string in labelname and use it.
		resolvelabellater(labelname, &i->imm->i);
		
		// Restore curpos.
		swapvalues(&curpos, &savedcurpos);
	}
	
	mmrefdown(labelname.ptr);
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("return")) {
	// A new line is not allowed
	// right after the keyword "return";
	// it help catch error not easily
	// caught during coding. ei:
	// 
	// 	if (s->level == level) return
	// 	ss = mmalloc(sizeof(lyricalstackframe));
	// 
	// In the above example, the semi-colon
	// was forgotten after the return statement;
	// In C, the error can only be caught
	// if the function containing that piece
	// of code was declared using a void return type
	// or if the type of the returned variable mismatched;
	// otherwise the code will compile just fine because
	// the value of ss will be returned; which is
	// probably not what the programmer intended,
	// and can be very hard to debug; what was
	// most likely intended is:
	// 
	// 	if (s->level == level) return
	// 	ss = mmalloc(sizeof(lyricalstackframe));
	// 
	// The code within this block will
	// insure that a newline character
	// is not found right after
	// the keyword "return".
	{
		u8* c = curpos;
		
		while (1) {
			
			--c;
			
			if (*c == 'n') break;
			else if (*c == '\n') {
				curpos = c;
				throwerror("newline disallowed to prevent coding confusion");
			}
		}
	}
	
	if (currentfunc == rootfunc) {
		curpos = savedcurpos;
		throwerror("\"return\" statement not within the definition of a function or operator");
	}
	
	c = *curpos;
	
	if (c != ';' && c != '}') {
		
		if (stringiseq2(returnvar.type, "void")) {
			curpos = savedcurpos;
			throwerror("\"return\" statement with an expression, used in function returning void");
		}
		
		lyricalvariable* v;
		
		while (1) {
			
			savedcurpos = curpos;
			
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
				
			} else break;
		}
		
		// An incorrect expression error
		// is also thrown when the variable
		// to use with the return statement
		// is retvar; not testing for that
		// condition will cause the memcpy
		// instruction to be used incorrectly
		// where both the source and destination
		// operands are the same registers.
		if (!v || v == EXPRWITHNORETVAL || v == &returnvar) {
			curpos = savedcurpos;
			throwerror("invalid return expression");
		}
		
		// Generate a signature to match
		// against the assign native operator.
		// In fact return work the same as assigning
		// to retvar using the native assign operator.
		string s = stringduplicate2("=");
		stringappend4(&s, '|');
		stringappend1(&s, returnvar.type);
		stringappend4(&s, '|');
		stringappend1(&s, v->cast.ptr ? v->cast : v->type);
		stringappend4(&s, '|');
		
		if (!searchnativeop(s)) {
			curpos = savedcurpos;
			throwerror("invalid return type");
		}
		
		mmrefdown(s.ptr);
		
		// If there was a string allocated in the field cast of returnvar,
		// I free it before using it with getvarduplicate().
		if (returnvar.cast.ptr) mmrefdown(returnvar.cast.ptr);
		returnvar.cast = stringnull;
		
		lyricalvariable* ptrtoreturnvar = &returnvar;
		
		getvarduplicate(&ptrtoreturnvar, v);
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		// If I have a tempvar or a lyricalvariable
		// which depend on a tempvar (dereference variable
		// or variable with its name suffixed with an offset),
		// I should free all those variables because
		// I will no longer need them and it allow
		// the generated code to save stack memory.
		varfreetempvarrelated(v);
	}
	
	// Labels, catchable-labels and instructions are created only in the secondpass.
	if (compilepass) {
		// I call flushanddiscardallreg() with its argument flag
		// set to DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR since
		// I will be generating the instructions returning from currentfunc.
		flushanddiscardallreg(DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR);
		
		if (currentfunc->firstpass->stackframeholder) {
			// I get here if the function for which
			// the lyricalfunction is pointed by currentfunc
			// make use of a stackframe holder.
			
			// I load the return address in register %1.
			loadreturnaddr();
			
			// I make a call to flushanddiscardallreg() in order
			// to discard the remaining register which will be
			// the one used to hold the return address.
			flushanddiscardallreg(DONOTFLUSHREGFORLOCALS);
			
			// Search the lyricalreg for %1.
			lyricalreg* r = searchreg(currentfunc, 1);
			
			// Lock the lyricalreg for %1,
			// to prevent newinstruction()
			// from adding it to the list
			// of unused registers of
			// the lyricalinstruction.
			r->lock = 1;
			
			jr(r);
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
		} else {
			// I get here if the function for which
			// the lyricalfunction is pointed by currentfunc
			// do not make use of a stackframe holder.
			
			// I create the instruction:
			// addi %0, %0, sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
			// It will set the stackpointer register to the location
			// within the stackframe where the return address is located.
			
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPADDI);
			i->r1 = 0;
			i->r2 = 0;
			
			lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMVALUE;
			imm->n = sizeofgpr;
			
			i->imm = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = currentfunc;
			
			i->imm->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSHAREDREGIONSIZE;
			imm->f = currentfunc;
			
			i->imm->next->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMLOCALVARSSIZE;
			imm->f = currentfunc;
			
			i->imm->next->next->next = imm;
			
			// I generate the instruction jpop.
			i = newinstruction(currentfunc, LYRICALOPJPOP);
		}
	}
	
	// Note that because flushanddiscardallreg() was called with
	// its argument flag set to DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR,
	// any following expression that use the variables or stackframes
	// that were discarded without being flushed is dead code which
	// will never be executed.
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("throw")) {
	
	if (currentfunc == rootfunc) {
		curpos = savedcurpos;
		throwerror("throw statement not within the definition of a function or operator");
	}
	
	// Labels, catchable-labels and instructions are created only in the secondpass.
	if (compilepass) {
		
		savedcurpos = curpos;
		
		string s = readsymbol(LOWERCASESYMBOL);
		
		if (!s.ptr) {
			reverseskipspace();
			throwerror("expecting a catchable-label name");
		}
		
		// If I get here, I am jumping out of the current function to a catchable-label.
		
		// Structure used by searchcatchablelabel() to return the result of its search.
		typedef struct {
			// When non-null, this field point to the lyricalcatchablelabel found.
			lyricalcatchablelabel* cl;
			
			// This field should only be used if the field t is set.
			// This field determines in which nth parent of the current function
			// the lyricalcatchablelabel pointed by the field t is in.
			// Hence a lyricalcatchablelabel found in the immediate parent
			// of currentfunc will have this field set to 1; and so on.
			// It work just like the value returned by the function varfunclevel().
			// When a lyricalcatchablelabel is found, this field can never be null,
			// because catchable-labels are used only by nested functions
			// and never used within the function where they are declared.
			uint funclevel;
			
		} searchcatchablelabelresult;
		
		// This function search for a catchable-label
		// having the name given as argument.
		// The search start from the parent function
		// because catchable-labels are used only
		// by nested functions and never used within
		// the function where they are declared.
		// This function is used only in the secondpass because
		// catchable-labels are created only in the secondpass.
		searchcatchablelabelresult searchcatchablelabel (string name) {
			
			searchcatchablelabelresult sclr = {0, 0};
			
			lyricalfunction* f = currentfunc;
			
			while (1) {
				
				lyricalfunction* fparent = f->parent;
				
				// Note that when searching through parent functions,
				// the scope tests need to be done with respect
				// to the scope used with the parent function and not
				// with respect to the scope used with the current function
				// because the variables scopecurrent and scope are reset
				// to null within funcdeclaration() before parsing the body
				// of a new function; hence the variables scopecurrent
				// and scope declared within parsestatement() are not used.
				uint scopedepth;
				uint* scope;
				
				if (fparent) {
					
					scopedepth = f->scopedepth;
					scope = f->scope;
					
				} else return sclr; // sclr.cl will be null.
				
				f = fparent;
				
				++sclr.funclevel;
				
				if (sclr.cl = searchcatchablelabellinkedlist(name, f->cl, scopedepth, scope, INCURRENTFUNCTIONONLY))
					return sclr;
			}
			
			// I will never get here.
		}
		
		searchcatchablelabelresult sclr = searchcatchablelabel(s);
		
		if (!sclr.cl) {
			curpos = savedcurpos;
			throwerror("undeclared catchable-label");
		}
		
		mmrefdown(s.ptr); // I no longer need the string in s.ptr; so I free it.
		
		// Since I will be generating the instruction to jump
		// out of the current function and effectively return to
		// a parent function, I call flushanddiscardallreg() with
		// its argument flag set to
		// DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL;
		// registers holding parent function stackframe addresses
		// will be kept so that they can be reused by
		// setregstackptrtofuncstackframe() which is called below.
		flushanddiscardallreg(DONOTFLUSHREGFORLOCALSKEEPREGFORFUNCLEVEL);
		
		// setregstackptrtofuncstackframe() will adjust
		// the stack pointer to the stackframe of the function in which
		// the catchable-label was declared and where the associated label to which
		// I am jumping-to is located. flushanddiscardallreg() called above
		// would have beforehand flushed all registers used for variables,
		// which is necessary, because the stack pointer can only be moved
		// after those registers have been flushed.
		// Note that setregstackptrtofuncstackframe() will also call
		// flushanddiscardallreg() to discard the remaining registers before
		// generating the instructions that check for excess stackpage to free.
		setregstackptrtofuncstackframe(sclr.funclevel);
		
		// There is no need to call cachestackframe() for
		// the stackframe level of the function containing
		// the catchable-label because the throw statement
		// return from the function and there would be no
		// gain caching the stackframe address of the function
		// to return to since no further instructions of
		// the function will be processed.
		// Furthermore the stackframe level of the function
		// containing the catchable-label can only be obtained
		// in the secondpass since catchable-labels are created
		// only in the secondpass; but cachestackframe()
		// is called only in the firstpass.
		
		// I generate the jump instruction.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		// The address where I have to jump to is resolved later,
		// when the function where the catchable-label was declared
		// is done being parsed.
		resolvecatchablelabellater(sclr.cl, &i->imm->i);
		
	} else skipsymbol(); // In the firstpass, if there is any symbol, it is skipped.
	
	// Note that because flushanddiscardallreg() was called within
	// setregstackptrtofuncstackframe() with its argument flag set
	// to DONOTFLUSHREGFORLOCALSKEEPREGFORRETURNADDR, any following expression
	// that use the variables or stackframes that were discarded without
	// being flushed is dead code which will never be executed.
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("break")) {
	// Labels and instructions are generated only in the secondpass.
	if (compilepass) {
		// I throw an error if labelnameforendofloop.ptr
		// is null which mean that I am not whithin
		// a loop or switch control statement block.
		if (!labelnameforendofloop.ptr) {
			curpos = savedcurpos;
			throwerror("break statement not within a loop or switch() block");
		}
		
		// I flush and discard all registers since I am branching.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		j(labelnameforendofloop);
	}
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("continue")) {
	// Labels and instructions are generated only in the secondpass.
	if (compilepass) {
		// I throw an error if labelnameforcontinuestatement.ptr is null which mean that I am not
		// whithin a loop control statement block.
		if (!labelnameforcontinuestatement.ptr) {
			curpos = savedcurpos;
			throwerror("continue statement not within a loop");
		}
		
		// I flush and discard all registers since I am branching.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I generate the jump instruction.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelnameforcontinuestatement, &i->imm->i);
	}
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("else")) {
	curpos = savedcurpos;
	throwerror("incorrect use of else statement");
}

if (checkforkeyword("if")) {
	// This variable will contain the name of the label
	// which will be created at the very end of this
	// if() statement and any of its else statements.
	string labelnameforendofblock;
	
	if (compilepass) {
		// Labels are created only in the secondpass.
		labelnameforendofblock = stringfmt("%d", newgenericlabelid());
	}
	
	evaluateifstatement:
	
	if (*curpos != '(') {
		reverseskipspace();
		throwerror("expecting '('");
	}
	
	++curpos; // Set curpos after '(' .
	
	lyricalvariable* v;
	
	while (1) {
		
		savedcurpos = curpos;
		
		skipspace();
		
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
			
		} else break;
	}
	
	if (*curpos != ')') {
		reverseskipspace();
		throwerror("expecting ')'");
	}
	
	if (!v || v == EXPRWITHNORETVAL) {
		curpos = savedcurpos;
		throwerror("expecting condition expression");
	}
	
	string type;
	
	if (v->cast.ptr) type = v->cast;
	else type = v->type;
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("the condition expression can only be of a native type or be a pointer");
	}
	
	// This variable will contain the name of the label to jump-to
	// in order to evaluate an alternate condition expression,
	// if the current condition expression evaluate to false.
	string labelnameforalternatetest;
	
	// Instructions and labels are generated only in the secondpass.
	if (compilepass) {
		
		lyricalreg* r = getregforvar(v, 0, type, v->bitselect, FORINPUT);
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		// I also lock it so that it is not seen as an unused
		// register when generating lyricalinstruction,
		// if the register is not assigned to anything.
		r->lock = 1;
		
		uint printregdiscardmsg = 0;
		
		// If the register pointed by r is allocated to a tempvar,
		// it is discarded so as to prevent its value from being flushed
		// by flushanddiscardallreg() if it was dirty; in fact if the
		// lyrical variable v is a tempvar, it will be freed soon after
		// the branching instruction making use of the register r
		// has been generated.
		if (r->v->name.ptr[0] == '$') {
			
			r->v = 0;
			
			setregtothetop(r);
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
		}
		
		if (linkedlistofpostfixoperatorcall) {
			// If the register is allocated to a variable which
			// is an argument to a pending postfix operation,
			// its value before postfixing must be used.
			if (r->v && isvarpostfixoperatorarg(r->v)) {
				
				lyricalreg* rr = allocreg(CRITICALREG);
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				rr->lock = 1;
				
				cpy(rr, r);
				
				// Unlock the lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r->lock = 0;
				
				r = rr;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			}
			
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		// I flush all registers without discarding them
		// since I am doing a conditional branching.
		flushanddiscardallreg(DONOTDISCARD);
		
		// Here I generate an instruction which will
		// jump depending on whether the value in the
		// register r is true or false. The jump instruction
		// will be resolved later to a lyricalinstruction
		// coming right after the last lyricalinstruction
		// generated while evaluating the expression or
		// block of this condition statement.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
		i->r1 = r->id;
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		labelnameforalternatetest = stringfmt("%d", newgenericlabelid());
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelnameforalternatetest, &i->imm->i);
		
		// Unlock the lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
		if (printregdiscardmsg) {
			// The lyricalreg pointed by r is not allocated
			// to a lyricalvariable but since I am done using it,
			// I should also produce a comment about it having been
			// discarded to complement the allocation comment that
			// was generated when it was allocated.
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
	} else {
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		// labelnameforalternatetest need to be non-null, so I can later detect an incorrect use of the "else" keyword.
		labelnameforalternatetest.ptr = (u8*)-1;
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(v);
	
	++curpos; // Position curpos after ')' .
	skipspace();
	
	evaluateblockorexpression:
	
	// I evaluate the expression or block;
	
	// I recursively call parsestatement()
	// to process a single line of expression
	// which could end up being a block which
	// will be handled within the recursion.
	// parsestatement() process the expression
	// and set curpos at the end of the expression,
	// and skipspace() is done before returning.
	parsestatement(PARSESINGLEEXPR);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I flush and discard all registers since I am going to branch.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Here I generate the jump instruction which will
		// be resolved later to the end of the if statement
		// and any of its else statement.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelnameforendofblock, &i->imm->i);
		
		if (labelnameforalternatetest.ptr) {
			// I create the label to jump to in order
			// to evaluate an alternate condition expression
			// if the previous condition expression evaluated to false.
			newlabel(labelnameforalternatetest);
		}
	}
	
	savedcurpos = curpos;
	
	if (checkforkeyword("else")) {
		
		if (labelnameforalternatetest.ptr) {
			// Setting labelnameforalternatetest to null
			// allow to determine later if I had a jump to
			// an alternate condition expression; for example
			// when I have the else statement, there is no test,
			// hence no jump to an alternate condition expression.
			labelnameforalternatetest = stringnull;
			
			if (checkforkeyword("if")) goto evaluateifstatement;
			
			goto evaluateblockorexpression;
		}
		
		// If labelnameforalternatetest is null
		// it mean that I already had an else statement
		// and this else statement is either incorrect
		// or for an outer if()else sequence.
		curpos = savedcurpos;
	}
	
	// I get here if the keyword else was not parsed,
	// and if the last character parsed was '}', a newline
	// should be required to prevent confusions such as
	// shown in the example below:
	// 
	// 	if (var) {
	// 		dosomething();
	// 		
	// 	} goto somewhere;
	// 
	// In C, the above code will compile fine, but the result
	// is probably not what the programmer intended, and
	// can be very hard to debug; what was most likely
	// intended is either:
	// 
	// 	if (var) {
	// 		dosomething();
	// 		
	// 	} else goto somewhere;
	// 
	// Or either:
	// 
	// 	if (var) {
	// 		dosomething();
	// 	}
	// 	
	// 	goto somewhere;
	// 
	// The code within this block will insure that a newline
	// character is found after the closing brace of
	// an if() or else block, unless the keyword "else" is
	// used after the closing brace of an if() block.
	if (*curpos) {
		
		u8* c = curpos;
		
		while (1) {
			
			--c;
			
			if (*c == '\n' || *c == ';') break;
			else if (*c == '}') {
				curpos = c + 1;
				throwerror("newline required to prevent coding confusion");
			}
		}
	}
	
	if (compilepass) {
		// Labels are created only in the secondpass.
		
		// It is not necessary to call flushanddiscardallreg()
		// before creating the label because it is done before
		// I get here before the last jump to the label that
		// I am creating here.
		// Hence I would end up having a jump to the label
		// with no meaningful instructions between the jump
		// label and the destination label. Those instructions
		// will be removed later when removing dead-code.
		
		// I create the label which mark the end of
		// the if statement and any of its else statement.
		newlabel(labelnameforendofblock);
	}
	
	goto endofstatement;
}

if (checkforkeyword("while")) {
	
	if (*curpos != '(') {
		reverseskipspace();
		throwerror("expecting '('");
	}
	
	// Used to save the value of labelnameforcontinuestatement.
	string savedlabelnameforcontinuestatement;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg()
		// before creating a new label since
		// jump-instructions will jump to the label
		// and all registers need to be un-used when
		// I start parsing the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I save whatever is currently in labelnameforcontinuestatement
		// before changing it because I could be currently within a loop.
		// Upon completing the parsing of this loop, I will restore it.
		savedlabelnameforcontinuestatement = labelnameforcontinuestatement;
		labelnameforcontinuestatement = stringfmt("%d", newgenericlabelid());
		
		// I create the label where continue statements will jump to.
		newlabel(labelnameforcontinuestatement);
	}
	
	++curpos; // Set curpos after '(' .
	
	lyricalvariable* v;
	
	while (1) {
		
		savedcurpos = curpos;
		
		skipspace();
		
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
			
		} else break;
	}
	
	if (*curpos != ')') {
		reverseskipspace();
		throwerror("expecting ')'");
	}
	
	if (!v || v == EXPRWITHNORETVAL) {
		curpos = savedcurpos;
		throwerror("expecting condition expression");
	}
	
	string type;
	
	if (v->cast.ptr) type = v->cast;
	else type = v->type;
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("the condition expression can only be of a native type or be a pointer");
	}
	
	// Used to save the value of labelnameforendofloop.
	string savedlabelnameforendofloop;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I save whatever is currently in labelnameforendofloop
		// before changing it because I could be currently within a loop.
		// Upon completing the parsing of this loop, I will restore it.
		savedlabelnameforendofloop = labelnameforendofloop;
		labelnameforendofloop = stringfmt("%d", newgenericlabelid());
		
		// If the result of the evaluation of the condition expression
		// is a lyricalvariable for a number, there is no need to generate
		// a conditional jump instruction because the result value
		// of the condition expression is known; it result in a faster loop.
		if (v->isnumber) {
			
			if (linkedlistofpostfixoperatorcall) {
				// This call is meant for processing
				// postfix operations if there was any.
				evaluateexpression(DOPOSTFIXOPERATIONS);
			}
			
			// Getting here mean that the condition expression
			// used only constant variables.
			if (!v->numbervalue) {
				// There is no need to call flushanddiscardallreg(FLUSHANDDISCARDALL)
				// because it was already called before evaluating
				// the condition expression, and since the condition expression
				// used only constant variables, I am sure that no registers
				// was allocated while evaluating the condition expression.
				
				// I generate the jump instruction.
				lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
				i->imm = mmallocz(sizeof(lyricalimmval));
				i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
				
				// The address where I have to jump to is resolved later.
				resolvelabellater(labelnameforendofloop, &i->imm->i);
			}
			
		} else {
			
			lyricalreg* r = getregforvar(v, 0, type, v->bitselect, FORINPUT);
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			// I also lock it so that it is not seen as an unused
			// register when generating lyricalinstruction,
			// if the register is not assigned to anything.
			r->lock = 1;
			
			uint printregdiscardmsg = 0;
			
			// If the register pointed by r is allocated to a tempvar,
			// it is discarded so as to prevent its value from being flushed
			// by flushanddiscardallreg() if it was dirty; in fact if the
			// lyrical variable v is a tempvar, it will be freed soon after
			// the branching instruction making use of the register r
			// has been generated.
			if (r->v->name.ptr[0] == '$') {
				
				r->v = 0;
				
				setregtothetop(r);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			}
			
			if (linkedlistofpostfixoperatorcall) {
				// If the register is allocated to a variable which
				// is an argument to a pending postfix operation,
				// its value before postfixing must be used.
				if (r->v && isvarpostfixoperatorarg(r->v)) {
					
					lyricalreg* rr = allocreg(CRITICALREG);
					// I lock the allocated register to prevent
					// another call to allocreg() from using it.
					// I also lock it, otherwise it could be lost
					// when insureenoughunusedregisters() is called
					// while creating a new lyricalinstruction.
					// I also lock it so that it is not seen
					// as an unused register when generating
					// lyricalinstruction, since the register
					// is not assigned to anything.
					rr->lock = 1;
					
					cpy(rr, r);
					
					// Unlock the lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r->lock = 0;
					
					r = rr;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
				}
				
				// This call is meant for processing
				// postfix operations if there was any.
				evaluateexpression(DOPOSTFIXOPERATIONS);
			}
			
			// I flush all registers without discarding them since I am doing a conditional branching.
			flushanddiscardallreg(DONOTDISCARD);
			
			// Here I generate an instruction which will jump
			// depending on whether the value in the register r
			// is true or false. The jump instruction will be
			// resolved later to a lyricalinstruction coming right
			// after the last lyricalinstruction generated while evaluating
			// the expression or block of this condition statement.
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
			i->r1 = r->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
			
			// The address where I have to jump to is resolved later.
			resolvelabellater(labelnameforendofloop, &i->imm->i);
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			if (printregdiscardmsg) {
				// The lyricalreg pointed by r is not allocated
				// to a lyricalvariable but since I am done using it,
				// I should also produce a comment about it having been
				// discarded to complement the allocation comment that
				// was generated when it was allocated.
				comment(stringfmt("reg %%%d discarded", r->id));
			}
		}
		
	} else {
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(v);
	
	++curpos; // Set curpos after ')' .
	skipspace();
	
	// I evaluate the expression or block;
	
	// I recursively call parsestatement()
	// to process a single line of expression
	// which could end up being a block which
	// will be handled within the recursion.
	// parsestatement() process the expression
	// and set curpos at the end of the expression,
	// and skipspace() is done before returning.
	parsestatement(PARSESINGLEEXPR);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// Adjust curpos so to log correct location.
		reverseskipspace();
		
		// I need to call flushanddiscardallreg()
		// since I am going to use a jump instruction.
		// I also need to call flushanddiscardallreg()
		// before creating the new label after the
		// jump instruction, since jump-instructions will jump
		// to the label and all registers need to be un-used
		// when I start parsing the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Here I generate the jump instruction to
		// loop back to the beginning of the loop.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelnameforcontinuestatement, &i->imm->i);
		
		// I create the label which marks the end of the loop statement.
		newlabel(labelnameforendofloop);
		
		// I restore labelnameforendofloop.
		labelnameforendofloop = savedlabelnameforendofloop;
		
		// I restore labelnameforcontinuestatement.
		labelnameforcontinuestatement = savedlabelnameforcontinuestatement;
		
		skipspace();
	}
	
	goto endofstatement;
}

if (checkforkeyword("do")) {
	
	string labelnameforstartofloop;
	string savedlabelnameforendofloop;
	string savedlabelnameforcontinuestatement;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		
		labelnameforstartofloop = stringfmt("%d", newgenericlabelid());
		
		// I need to call flushanddiscardallreg() before creating
		// a new label since jump-instructions will jump to the label
		// and all registers need to be un-used when I start parsing
		// the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label where jumps will be made to keep looping.
		newlabel(labelnameforstartofloop);
		
		// I save whatever is currently in labelnameforendofloop
		// before changing it because I could be currently within a loop.
		// Upon completing the parsing of this loop, I will restore it.
		// Note that I only need to do this if I have block, because
		// if I simply have an expression, the keyword break,
		// which use the label name, will not be used.
		savedlabelnameforendofloop = labelnameforendofloop;
		labelnameforendofloop = stringfmt("%d", newgenericlabelid());
		
		// I save whatever is currently in labelnameforcontinuestatement
		// before changing it because I could be currently within a loop.
		// Upon completing the parsing of this block, I will restore it.
		// Note that I only need to do this if I have a block, because
		// if I simply have an expression, the keyword continue,
		// which uses the label name, will not be used.
		savedlabelnameforcontinuestatement = labelnameforcontinuestatement;
		labelnameforcontinuestatement = stringfmt("%d", newgenericlabelid());
	}
	
	// I evaluate the expression or block;
	
	// I recursively call parsestatement()
	// to process a single line of expression
	// which could end up being a block which
	// will be handled within the recursion.
	// parsestatement() process the expression
	// and set curpos at the end of the expression,
	// and skipspace() is done before returning.
	parsestatement(PARSESINGLEEXPR);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg() before creating
		// a new label since jump-instructions will jump to the label
		// and all registers need to be un-used when I start parsing
		// the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label where continue statements will jump to.
		newlabel(labelnameforcontinuestatement);
		
		// I restore labelnameforcontinuestatement;
		labelnameforcontinuestatement = savedlabelnameforcontinuestatement;
	}
	
	if (!checkforkeyword("while")) {
		curpos = savedcurpos;
		throwerror("incorrect use of do statement");
	}
	
	if (*curpos != '(') {
		reverseskipspace();
		throwerror("expecting '('");
	}
	
	++curpos; // Set curpos after '(' .
	
	lyricalreg* r;
	
	lyricalvariable* v;
	
	while (1) {
		
		savedcurpos = curpos;
		
		skipspace();
		
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
			
		} else break;
	}
	
	if (*curpos != ')') {
		reverseskipspace();
		throwerror("expecting ')'");
	}
	
	if (!v || v == EXPRWITHNORETVAL) {
		curpos = savedcurpos;
		throwerror("expecting condition expression");
	}
	
	string type;
	
	if (v->cast.ptr) type = v->cast;
	else type = v->type;
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("the condition expression can only be of a native type or be a pointer");
	}
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// If the result of the evaluation of the condition expression
		// is a lyricalvariable for a number, there is no need to generate
		// a conditional jump instruction because the result value
		// of the condition expression is known; it result in a faster loop.
		if (v->isnumber) {
			
			if (linkedlistofpostfixoperatorcall) {
				// This call is meant for processing
				// postfix operations if there was any.
				evaluateexpression(DOPOSTFIXOPERATIONS);
			}
			
			// Getting here mean that the condition expression
			// used only constant variables.
			if (v->numbervalue) {
				// There is no need to call flushanddiscardallreg(FLUSHANDDISCARDALL)
				// because it was already called before evaluating
				// the condition expression, and since the condition expression
				// used only constant variables, I am sure that no registers
				// was allocated while evaluating the condition expression.
				
				// I generate the jump instruction.
				lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
				i->imm = mmallocz(sizeof(lyricalimmval));
				i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
				
				// The address where I have to jump to is resolved later.
				resolvelabellater(labelnameforstartofloop, &i->imm->i);
			}
			
		} else {
			
			r = getregforvar(v, 0, type, v->bitselect, FORINPUT);
			// I lock the allocated register to prevent
			// another call to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			// I also lock it so that it is not seen as an unused
			// register when generating lyricalinstruction,
			// if the register is not assigned to anything.
			r->lock = 1;
			
			uint printregdiscardmsg = 0;
			
			// If the register pointed by r is allocated to a tempvar,
			// it is discarded so as to prevent its value from being flushed
			// by flushanddiscardallreg() if it was dirty; in fact if the
			// lyrical variable v is a tempvar, it will be freed soon after
			// the branching instruction making use of the register r
			// has been generated.
			if (r->v->name.ptr[0] == '$') {
				
				r->v = 0;
				
				setregtothetop(r);
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			}
			
			if (linkedlistofpostfixoperatorcall) {
				// If the register is allocated to a variable which
				// is an argument to a pending postfix operation,
				// its value before postfixing must be used.
				if (r->v && isvarpostfixoperatorarg(r->v)) {
					
					lyricalreg* rr = allocreg(CRITICALREG);
					// I lock the allocated register to prevent
					// another call to allocreg() from using it.
					// I also lock it, otherwise it could be lost
					// when insureenoughunusedregisters() is called
					// while creating a new lyricalinstruction.
					// I also lock it so that it is not seen
					// as an unused register when generating
					// lyricalinstruction, since the register
					// is not assigned to anything.
					rr->lock = 1;
					
					cpy(rr, r);
					
					// Unlock the lyricalreg.
					// Locked registers must be unlocked only after
					// the instructions using them have been generated;
					// otherwise they could be lost when insureenoughunusedregisters()
					// is called while creating a new lyricalinstruction.
					r->lock = 0;
					
					r = rr;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
				}
				
				// This call is meant for processing
				// postfix operations if there was any.
				evaluateexpression(DOPOSTFIXOPERATIONS);
			}
			
			// I flush all registers without discarding them since I am doing a conditional branching.
			flushanddiscardallreg(DONOTDISCARD);
			
			// Here I generate an instruction which will jump
			// depending on whether the value in the register r
			// is true or false. The jump instruction will be
			// resolved later to a lyricalinstruction coming right
			// after the last lyricalinstruction generated while evaluating
			// the expression or block of this condition statement.
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJNZ);
			i->r1 = r->id;
			i->imm = mmallocz(sizeof(lyricalimmval));
			i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
			
			// The address where I have to jump to is resolved later.
			resolvelabellater(labelnameforstartofloop, &i->imm->i);
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
			if (printregdiscardmsg) {
				// The lyricalreg pointed by r is not allocated
				// to a lyricalvariable but since I am done using it,
				// I should also produce a comment about it having been
				// discarded to complement the allocation comment that
				// was generated when it was allocated.
				comment(stringfmt("reg %%%d discarded", r->id));
			}
		}
		
	} else {
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(v);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg()
		// before creating a new label since jump-instructions
		// will jump to the label and all registers need to be un-used
		// when I start parsing the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label which marks the end of the loop statement.
		newlabel(labelnameforendofloop);
		
		// I restore labelnameforendofloop;
		labelnameforendofloop = savedlabelnameforendofloop;
	}
	
	++curpos; // Position curpos after ')' .
	skipspace();
	
	c = *curpos;
	
	if (c == '}') goto endofstatementclosingbrace;
	else if (c == ';') goto endofstatementsemicolon;
	
	reverseskipspace();
	throwerror("expecting ';' or '}'");
}

if (checkforkeyword("case")) {
	// I throw an error if currentswitchblock is null
	// which mean that I am not whithin a switch() block.
	if (!currentswitchblock) {
		curpos = savedcurpos;
		throwerror("case block not within a switch() block");
	}
	
	keywordcasechecked:;
	
	string caselabel;
	
	// Labels and instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg() before creating
		// the label for the switch() case since jumps will be made
		// to this label and all registers need to be un-used
		// when I start parsing the code within the case block.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		newlabel(caselabel = stringfmt("%d", newgenericlabelid()));
	}
	
	// Within this loop, I parse each
	// of the comma separated case value.
	while (1) {
		
		savedcurpos = curpos;
		
		lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
		
		if (!v || v == EXPRWITHNORETVAL || !v->isnumber) {
			curpos = savedcurpos;
			throwerror("expecting an expression that result in a constant");
		}
		
		// Postfix operations are not allowed
		// because the case value must be a known
		// constant value; furthermore the instructions
		// generated for the post-evaluation would
		// not appear in the correct execution flow.
		if (linkedlistofpostfixoperatorcall) {
			curpos = savedcurpos;
			throwerror("expecting an expression with no postfix operations");
		}
		
		// A lyricalvariable for a constant
		// always has its field cast set.
		string casetype = v->cast;
		
		// The type string of an enum always start
		// with the character '#'.
		if (currentswitchblock->exprtype.ptr[0] == '#') {
			if (!stringiseq1(casetype, currentswitchblock->exprtype)) {
				curpos = savedcurpos;
				throwerror("switch() case type mismatch");
			}
			
		} else if (!pamsynmatch2(isnativetype, casetype.ptr, stringmmsz(casetype)).end) {
			curpos = savedcurpos;
			throwerror("switch() case type mismatch");
		}
		
		if (v->numbervalue > maxhostuintvalue) {
			curpos = savedcurpos;
			throwerror("switch() case value beyond host capability");
		}
		
		uint casevalue = v->numbervalue;
		
		if (bintreefind(currentswitchblock->btree, casevalue)) {
			curpos = savedcurpos;
			throwerror("duplicate switch() case");
		}
		
		// Labels are generated only in the secondpass.
		if (compilepass) {
			bintreeadd(&currentswitchblock->btree, casevalue, caselabel.ptr);
		}
		
		if (*curpos == ',') {
			
			++curpos;
			skipspace();
			
		} else break;
	}
	
	if (*curpos != '{') {
		reverseskipspace();
		throwerror("expecting '{'");
	}
	
	// parsestatement(PARSESINGLEEXPR) will do
	// necessary prerequisites before calling
	// parsestatement(PARSEBLOCK).
	parsestatement(PARSESINGLEEXPR);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I generate a jump instruction to labelnameforendofloop.
		
		// I flush and discard all registers since I am branching.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		j(labelnameforendofloop);
	}
	
	if (*curpos == '}') goto endofstatementclosingbrace;
	
	// If '}' was not parsed, I should expect
	// either of the statement "case" or "default".
	
	if (checkforkeyword("case")) goto keywordcasechecked;
	else if (checkforkeyword("default")) goto keyworddefaultchecked;
	
	// There is no need to check whether
	// statementparsingflag == PARSESINGLEEXPR
	// in order to return because when a case statement
	// is used I am certainly within a switch() block.
	
	reverseskipspace();
	throwerror("expecting a case or default statement");
}

if (checkforkeyword("default")) {
	// I throw an error if currentswitchblock is null
	// which mean that I am not whithin a switch() block.
	if (!currentswitchblock) {
		curpos = savedcurpos;
		throwerror("default block not within a switch() block");
	}
	
	keyworddefaultchecked:;
	
	// There is no need to check whether the default block
	// is a duplicate, because no other case block can be
	// parsed after a default block.
	
	// Label and instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg() before creating
		// the label for the switch() default since jumps will be made
		// to this label and all registers need to be un-used
		// when I start parsing the code within the default block.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		newlabel(currentswitchblock->defaultcase = stringfmt("%d", newgenericlabelid()));
	}
	
	if (*curpos != '{') {
		reverseskipspace();
		throwerror("expecting '{'");
	}
	
	// parsestatement(PARSESINGLEEXPR) will do
	// necessary prerequisites before calling
	// parsestatement(PARSEBLOCK).
	parsestatement(PARSESINGLEEXPR);
	
	// There is no need to generate a jump instruction
	// to labelnameforendofloop once a default block has
	// been parsed, because a default block is always
	// to be found at the end of a switch() block.
	
	if (*curpos == '}') goto endofstatementclosingbrace;
	
	// If I get here, I throw an error,
	// because the closing brace marking
	// the end of the switch() block
	// should have been parsed.
	
	// There is no need to check whether
	// statementparsingflag == PARSESINGLEEXPR
	// in order to return because when a default statement
	// is used I am certainly within a switch() block.
	
	reverseskipspace();
	throwerror("expecting end of switch() block after default block");
}

if (checkforkeyword("switch")) {
	
	if (*curpos != '(') {
		reverseskipspace();
		throwerror("expecting '('");
	}
	
	++curpos; // Set curpos after '(' .
	
	lyricalvariable* v;
	
	u8* savedcurpos2;
	
	while (1) {
		
		savedcurpos2 = curpos;
		
		skipspace();
		
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
			
		} else break;
	}
	
	if (!v || v == EXPRWITHNORETVAL) {
		curpos = savedcurpos2;
		throwerror("expecting condition expression");
	}
	
	string currentswitchblockexprtype =
		v->cast.ptr ? v->cast : v->type;
	
	if (currentswitchblockexprtype.ptr[0] != '#' &&
		!pamsynmatch2(isnativetype,
			currentswitchblockexprtype.ptr,
			stringmmsz(currentswitchblockexprtype)).start) {
		curpos = savedcurpos2;
		throwerror("the condition expression can only be of an enum or native integer type");
	}
	
	if (*curpos != ')') {
		reverseskipspace();
		throwerror("expecting ')'");
	}
	
	++curpos; // Position curpos after ')' .
	
	savedcurpos2 = curpos;
	
	skipspace();
	
	if (*curpos != '{') {
		curpos = savedcurpos2;
		throwerror("expecting '{'");
	}
	
	++curpos; // Set curpos after '{' .
	savedcurpos = curpos;
	skipspace();
	
	if (*curpos == '}') {
		
		++curpos; // Set curpos after '}' .
		skipspace();
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		// If I have a tempvar or a lyricalvariable
		// which depend on a tempvar (dereference variable
		// or variable with its name suffixed with an offset),
		// I should free all those variables because
		// I will no longer need them and it allow
		// the generated code to save stack memory.
		varfreetempvarrelated(v);
		
		goto doneparsingswitchblock;
	}
	
	savedcurpos2 = curpos;
	
	// The body of a switch() should only
	// have case and default statements.
	if (!checkforkeyword("case") && !checkforkeyword("default")) {
		curpos = savedcurpos2;
		throwerror("expecting a case or default statement");
	}
	
	curpos = savedcurpos2;
	
	// I save currentswitchblock before
	// changing it, because I could be
	// currently within a switch() block.
	// I will restore it upon completing
	// the parsing of the switch() block.
	switchblock* savedcurrentswitchblock = currentswitchblock;
	
	currentswitchblock = mmallocz(sizeof(switchblock));
	
	currentswitchblock->exprtype = stringduplicate1(currentswitchblockexprtype);
	
	lyricalreg* r;
	lyricalinstruction* i1;
	
	uint printregdiscardmsg = 0;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		
		r = getregforvar(v, 0, currentswitchblock->exprtype, v->bitselect, FORINPUT);
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		// I also lock it so that it is not seen as an unused
		// register when generating lyricalinstruction,
		// if the register is not assigned to anything.
		r->lock = 1;
		
		// If the register pointed by r is allocated to a tempvar,
		// it is discarded so as to prevent its value from being flushed
		// by flushanddiscardallreg() if it was dirty; in fact if the
		// lyrical variable v is a tempvar, it will be freed soon after
		// the branching instruction making use of the register r
		// has been generated.
		if (r->v->name.ptr[0] == '$') {
			
			r->v = 0;
			
			setregtothetop(r);
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
		}
		
		if (linkedlistofpostfixoperatorcall) {
			// If the register is allocated to a variable which
			// is an argument to a pending postfix operation,
			// its value before postfixing must be used.
			if (r->v && isvarpostfixoperatorarg(r->v)) {
				
				lyricalreg* rr = allocreg(CRITICALREG);
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				rr->lock = 1;
				
				cpy(rr, r);
				
				// Unlock the lyricalreg.
				// Locked registers must be unlocked only after
				// the instructions using them have been generated;
				// otherwise they could be lost when insureenoughunusedregisters()
				// is called while creating a new lyricalinstruction.
				r->lock = 0;
				
				r = rr;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			}
			
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
		
		// I flush and discard all registers since
		// jumptable instructions will be coming next.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Although the register pointed by r has lost to which variable it was
		// associated-to, its value has been preserved since it was locked.
		
		// I save the instruction address
		// where to later insert the jumptable.
		i1 = currentfunc->i;
		
		// Note that right after the instructions
		// flushing all registers, the jumptable instructions
		// will be generated using the lyricalreg pointed by r.
		// Using the saved lyricalinstruction address where
		// to later insert instructions, the jumptable instructions
		// will be generated knowing what register to use
		// since the value of r would not have changed.
		
		// Unlock the lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
	} else {
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(v);
	
	string savedlabelnameforendofloop;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I save whatever is currently in
		// labelnameforendofloop before changing it
		// because I could be currently within a loop.
		// I will restore it upon completing
		// the parsing of the switch() block.
		savedlabelnameforendofloop = labelnameforendofloop;
		labelnameforendofloop = stringfmt("%d", newgenericlabelid());
	}
	
	scopeentering();
	
	// parsestatement() is recursively
	// called to process the block;
	// parsestatement() process the entire block
	// and set curpos after the closing brace of
	// the block and do skipspace() before returning.
	parsestatement(PARSEBLOCK);
	
	scopeleaving();
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I need to call flushanddiscardallreg() before creating
		// a new label since jump-instructions will jump to the label
		// and all registers need to be un-used when I start parsing
		// the code coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label which mark
		// the end of the switch statement.
		newlabel(labelnameforendofloop);
		
		// I set curpos to savedcurpos so that
		// debugging information get generated
		// with the correct location.
		swapvalues(&curpos, &savedcurpos);
		
		// I save the current last instruction created
		// and set currentfunc->i to i1 in which
		// was previously saved the instruction
		// after which the jumptable is to be inserted,
		// allowing for the transparent use of newinstruction()
		// to insert the jumptable instructions.
		lyricalinstruction* savedi = currentfunc->i;
		currentfunc->i = i1;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("begin: generating switch() jumptable"));
		}
		
		string defaultsc = currentswitchblock->defaultcase;
		
		if (currentswitchblock->btree.ptr) {
			// Prior to saving in i1 the instruction address
			// where to later insert the jumptable,
			// it was insured that the value of the register
			// pointed by r had been preserved for use
			// by the jumptable instructions.
			// I lock it to prevent another call
			// to allocreg() from using it.
			// I also lock it, otherwise it could be lost
			// when insureenoughunusedregisters() is called
			// while creating a new lyricalinstruction.
			// I also lock it so that it is not seen as an unused
			// register when generating lyricalinstruction,
			// if the register is not assigned to anything.
			r->lock = 1;
			
			lyricalreg* r1 = allocreg(CRITICALREG);
			// Since the register is not assigned
			// to anything, I lock it, otherwise
			// it will be seen as an unused register
			// when generating lyricalinstruction.
			r1->lock = 1;
			
			uint currentswitchblocklowestcasevalue;
			// Retrieve lowest switch() case value.
			uint bintreescanupcallback (uint value, void** data) {
				currentswitchblocklowestcasevalue = value;
				return 0;
			}
			bintreescanup(currentswitchblock->btree, bintreescanupcallback);
			
			if (currentswitchblocklowestcasevalue) {
				li(r1, currentswitchblocklowestcasevalue);
				sub(r, r, r1); // Substract lowest case value to ease comparison.
			}
			
			uint currentswitchblockhighestcasevalue;
			// Retrieve highest switch() case value.
			uint bintreescandowncallback (uint value, void** data) {
				currentswitchblockhighestcasevalue = value;
				return 0;
			}
			bintreescandown(currentswitchblock->btree, bintreescandowncallback);
			
			li(r1, currentswitchblockhighestcasevalue-currentswitchblocklowestcasevalue);
			
			// Jump to the switch() default if it exist,
			// otherwise jump to the end of the switch().
			if (defaultsc.ptr) jltu(r1, r, defaultsc);
			else jltu(r1, r, labelnameforendofloop);
			
			string labelnameforjumpcasearray = stringfmt("%d", newgenericlabelid());
			
			afip(r1, labelnameforjumpcasearray); // Get the address of the jumpcase array.
			
			uint jumpcaseclog2sz = compilearg->jumpcaseclog2sz;
			
			slli(r, r, jumpcaseclog2sz); // Compute offset within the jumpcase array.
			add(r1, r1, r); // Compute the address to jump to within the jumpcase array.
			jr(r1);
			
			// Create the label which mark
			// the start of the jumpcase array.
			newlabel(labelnameforjumpcasearray);
			
			// Generate the jumpcase array.
			uint jumpcasebinsz = (1<<jumpcaseclog2sz);
			uint n = currentswitchblocklowestcasevalue;
			do {
				string sc = {.ptr = bintreefind(currentswitchblock->btree, n)};
				j(sc.ptr ? sc : labelnameforendofloop)->binsz = jumpcasebinsz;
			} while (++n <= currentswitchblockhighestcasevalue);
			
			// Destroy currentswitchblock->btree .
			bintreeempty(&currentswitchblock->btree);
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r1->lock = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				// The lyricalreg pointed by r1 is not allocated
				// to a lyricalvariable but since I am done using it,
				// I should also produce a comment about it having been
				// discarded to complement the allocation comment that
				// was generated when it was allocated.
				comment(stringfmt("reg %%%d discarded", r1->id));
			}
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
		// There would at least be
		// the switch() default label
		// when there was no switch() case.
		} else j(defaultsc);
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			comment(stringduplicate2("end: done"));
		}
		
		if (printregdiscardmsg) {
			// The lyricalreg pointed by r is not allocated
			// to a lyricalvariable but since I am done using it,
			// I should also produce a comment about it having been
			// discarded to complement the allocation comment that
			// was generated when it was allocated.
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
		// Restore currentfunc->i;
		currentfunc->i = savedi;
		
		// Restore curpos.
		swapvalues(&curpos, &savedcurpos);
		
		// Restore labelnameforendofloop.
		labelnameforendofloop = savedlabelnameforendofloop;
	}
	
	// Destroy currentswitchblock.
	mmrefdown(currentswitchblock->exprtype.ptr);
	mmrefdown(currentswitchblock);
	
	// Restore the saved currentswitchblock.
	currentswitchblock = savedcurrentswitchblock;
	
	doneparsingswitchblock:
	
	goto endofstatement;
}

if (checkforkeyword("asm")) {
	// Prior to entering an asm block,
	// there should not be any register locked,
	// because registers get locked while
	// evaluating an expression. There should
	// also not be any register reserved.
	
	if (*curpos == '{') {
		// If I get here, I read a block of instructions.
		
		++curpos; // Set curpos after '{' .
		skipspace();
		
		while (1) {
			
			c = *curpos;
			
			if (c == '}')  break;
			else if (c == ';') {
				
				++curpos; // Set curpos after ';' .
				skipspace();
				
				if (*curpos == '}') break;
				
				continue;
			}
			
			evaluateexpression(DOASM);
		}
		
		++curpos; // Set curpos after '}' .
		skipspace();
		
	} else {
		
		skipspace();
		
		// If I get here, I read a single
		// instruction. ei: asm roli n, n, 1;
		evaluateexpression(DOASM);
		
		if (*curpos != ';') {
			reverseskipspace();
			throwerror("expecting ';'");
		}
		
		++curpos; // This set curpos after ';'.
		skipspace();
	}
	
	// Because instructions are generated only in the secondpass,
	// gpr are made available only in the secondpass.
	if (compilepass) {
		// I un-reserve any reserved registers.
		
		lyricalreg* r = currentfunc->gpr;
		
		do {
			if (r->reserved) {
				
				r->reserved = 0;
				
				if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
					
					comment(stringfmt("reg %%%d unreserved", r->id));
				}
			}
			
			r = r->next;
			
		} while (r != currentfunc->gpr);
	}
	
	goto endofstatement;
}
