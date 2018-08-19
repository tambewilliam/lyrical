
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


// This function create a new argument and
// add it to the linkedlist pointed by funcarg.
// This function can never be called twice for
// the same value of curpos, because the value
// of curpos is used in the firstpass to set
// the field id of lyricalargumentflag created
// for each argument; and each lyricalargumentflag
// must have a unique id.
// This function also register the new argument
// in order for propagatevarchange() to use it.
void pushargument (lyricalvariable* v) {
	
	if (compilepass && compileargcompileflag&LYRICALCOMPILECOMMENT) {
		comment(stringduplicate2("begin: pushing argument"));
	}
	
	uint preservetempattr = v->preservetempattr;
	
	// I clear the field preservetempattr of
	// the lyricalvariable that I am pushing.
	// It will prevent that field from being
	// used anywhere else until set again.
	v->preservetempattr = 0;
	
	// This function is used to register
	// an argument in order for
	// propagatevarchange() to use it.
	// This function is used
	// only in the secondpass.
	void registerarg (lyricalargument* arg) {
		
		arg->prevregisteredarg = 0;
		
		if (registeredargs) {
			
			registeredargs->prevregisteredarg = arg;
			
			arg->nextregisteredarg = registeredargs;
			
		} else arg->nextregisteredarg = 0;
		
		registeredargs = arg;	
	}
	
	lyricalargument* arg = mmallocz(sizeof(lyricalargument));
	
	if (v->cast.ptr) arg->typepushed = stringduplicate1(v->cast);
	else arg->typepushed = stringduplicate1(v->type);
	
	if (funcarg) {
		// Remember that funcarg->next point to
		// the first argument of the linkedlist,
		// and I want the field next of the last
		// created argument to point to that first
		// argument of the linkedlist.
		arg->next = funcarg->next;
		
		// I set the field next of the previously
		// last created argument to the argument
		// that I just created.
		funcarg->next = arg;
		
	} else arg->next = arg;
	
	// Set funcarg to the last created argument.
	funcarg = arg;
	
	arg->bitselect = v->bitselect;
	
	if (!preservetempattr) {
		// I set to zero the field bitselect of
		// the lyricalvariable that I am pushing
		// if it was set.
		// It will prevent that field from being
		// used anywhere else until set again.
		// Note that if the variable that I am
		// pushing is to be duplicated, it is done
		// without applying its bitselect; it is
		// only when the argument need to be used
		// that the bitselect saved
		// in arg->bitselect is applied.
		v->bitselect = 0;
	}
	
	// This function allocate memory for
	// a new lyricalargumentflag structure and
	// add it to the linkedlist pointed by
	// the field pushedargflags of currentfunc.
	// The fields id and next of the newly
	// created lyricalargumentflag are set.
	// A pointer to the lyricalargumentflag
	// created is returned.
	// This function is only used in the firstpass.
	lyricalargumentflag* newargumentflag () {
		// I allocate a zeroed memory block
		// for a new lyricalargumentflag structure.
		lyricalargumentflag* argflag = mmallocz(sizeof(lyricalargumentflag));
		
		// I use the value of curpos and set it
		// in the field id of the newly created argumentflag.
		// Because arguments are parsed the same way
		// in the first and second pass, the id value
		// will be the same both in the first and second pass.
		argflag->id = (uint)curpos;
		
		if (currentfunc->pushedargflags) {
			
			argflag->next = currentfunc->pushedargflags->next;
			
			currentfunc->pushedargflags->next = argflag;
			
		} else argflag->next = argflag;
		
		currentfunc->pushedargflags = argflag;
		
		return argflag;
	}
	
	// This function is used to search for
	// a lyricalargumentflag matching its field id.
	// This function is only used in the secondpass.
	lyricalargumentflag* lookupargflagbyid (uint id) {
		
		lyricalargumentflag* argflag;
		
		argflag = currentfunc->firstpass->pushedargflags;
		
		if (argflag) {
			
			lyricalargumentflag* a = argflag;
			
			do {
				if (a->id == id) return a;
				
				a = a->next;
				
			} while (a != argflag);
		}
		
		throwerror(stringfmt("internal error: %s: could not retrieve firstpass lyricalargumentflag", __FUNCTION__).ptr);
	}
	
	if (compilepass) {
		// Here I search the flag for the argument
		// using its id. That flag was created in the
		// firstpass; in the firstpass the field id of
		// a lyricalargumentflag is set using curpos.
		// I use the value of curpos because it will be
		// the same both in the first and secondpass.
		arg->flag = lookupargflagbyid((uint)curpos);
		
	} else {
		// I come here only during the firstpass;
		// in the firstpass, the linkedlist of
		// argumentflag is built.
		arg->flag = newargumentflag();
		
		// In the firstpass, the linkedlist
		// of lyricalcachedstackframe is built.
		// If the lyricalvariable pointed by v is external
		// to the lyricalfunction pointed by currentfunc,
		// I create a lyricalcachedstackframe for it.
		if (v != &thisvar && v != &returnvar && v->funcowner != rootfunc &&
			v->funcowner != currentfunc)
				cachestackframe(currentfunc, varfunclevel(v));
	}
	
	arg->varpushed = v;
	
	// Here I check if the variable
	// is to be passed by reference.
	if (arg->flag->istobepassedbyref) {
		// I only get here in the secondpass.
		// The field flag->istobepassedbyref is set
		// in the firstpass, and it is in
		// the secondpass that I can see it set.
		
		// Note that when passing an argument
		// by reference, it must be set as always
		// volatile because the function called
		// can pass that address elsewhere creating
		// an aliasing issue.
		// getvaraddr() will always be successful
		// because arg->flag->istobepassedbyref is only
		// set for arguments which can be passed byref.
		v = getvaraddr(v, MAKEALWAYSVOLATILE);
	}
	
	// Arguments are registered only in
	// the secondpass, and propagatevarchange()
	// go through registered arguments only
	// in the secondpass.
	// I register the argument at the very end
	// because propagatevarchange() can be called
	// if I make use of getvaraddr() while pushing
	// a byref variable; and it would cause
	// propagatevarchange() to go through all registered
	// arguments and I don't want it to go through
	// a registered argument which do not have all
	// its fields correctly set.
	if (compilepass) registerarg(arg);
	
	// For speed, duplication of a pushed argument
	// is only done in the secondpass; also, a pushed
	// argument is duplicated if it is volatile and
	// it is not going to be used as the output of
	// an operation, and if it is not a readonly variable.
	// Note that a readonly variable can never be volatile.
	if (compilepass && !arg->flag->istobeoutput &&
		!isvarreadonly(v) && *v->alwaysvolatile) getvarduplicate(&arg->v, v);
	else arg->v = v;
	
	if (!preservetempattr) {
		
		if (arg->varpushed->cast.ptr) {
			// I clear the field cast of the
			// lyricalvariable that I am pushing.
			// It will prevent that field from being
			// used anywhere else until set again.
			mmrefdown(arg->varpushed->cast.ptr);
			arg->varpushed->cast = stringnull;
		}
		
		if (arg->v != arg->varpushed && arg->v->cast.ptr) {
			// I clear the field cast of the
			// lyricalvariable that I am pushing.
			// It will prevent that field from being
			// used anywhere else until set again.
			mmrefdown(arg->v->cast.ptr);
			arg->v->cast = stringnull;
		}
	}
	
	if (compilepass && compileargcompileflag&LYRICALCOMPILECOMMENT)
		comment(stringduplicate2("end: done"));
}


// This function will unregister and free the arguments
// in the linkedlist pointed by funcarg, then
// set it to null. For speed, there is no check
// on whether funcarg is null; that check should
// be done before calling this function.
void freefuncarg () {
	// This function unregister all lyricalarguments
	// of the linkedlist pointed by funcarg.
	// For speed, there is no check on whether
	// funcarg is null; that check should
	// be done before calling this function.
	// This function is used only in the secondpass
	// because arguments are registered only
	// in the secondpass.
	void unregisterargs () {
		
		lyricalargument* arg = funcarg;
		
		do {
			if (arg->prevregisteredarg)
				arg->prevregisteredarg->nextregisteredarg = arg->nextregisteredarg;
			else registeredargs = arg->nextregisteredarg;
			
			if (arg->nextregisteredarg)
				arg->nextregisteredarg->prevregisteredarg = arg->prevregisteredarg;
			
			arg = arg->next;
			
		} while (arg != funcarg);
		
		return;
	}
	
	// Arguments are registered only in the secondpass.
	if (compilepass) unregisterargs();
	
	lyricalargument* arg = funcarg;
	
	do {
		// The field type of pushed arguments
		// is always set by pushargument().
		mmrefdown(arg->typepushed.ptr);
		
		// If arg->tobeusedasreturnvariable is true,
		// I do not attempt to check whether arg->varpushed
		// and arg->v are tempvar or variable which
		// depend on a tempvar.
		if (!arg->tobeusedasreturnvariable) {
			// If I have a tempvar or a lyricalvariable
			// which depend on a tempvar (dereference variable
			// or variable with its name suffixed with an offset),
			// I should free all those variables because
			// I will no longer need them and it allow
			// the generated code to save stack memory.
			
			// If arg->v point to the lyricalvariable
			// for the address of arg->varpushed, and it was not
			// a tempvar, the lyricalvariable pointed by arg->v
			// get freed by varfreetempvarrelated(); hence
			// prior to calling varfreetempvarrelated(),
			// I save whether arg->v was a tempvar.
			uint isargvtempvar = (arg->v != arg->varpushed) && !!arg->v->type.ptr;
			
			varfreetempvarrelated(arg->varpushed);
			
			if (isargvtempvar && pamsynmatch2(matchtempvarname, arg->v->name.ptr, stringmmsz(arg->v->name)).start) {
				// If I get here, arg->v point to a tempvar.
				// I free the tempvar if it is not shared
				// with another registered argument.
				
				// This function check whether the lyricalargument
				// pointed by arg is sharing a tempvar with
				// a registered argument.
				// If a match is found, the lyricalvariable pointed
				// by the field v of the lyricalargument is certainly
				// a tempvar for a duplicate variable which was
				// created by propagatevarchange().
				uint issharedtempvar3 () {
					
					lyricalargument* argsearch = registeredargs;
					
					while (argsearch) {
						
						if (argsearch->v == arg->v) return 1;
						
						argsearch = argsearch->nextregisteredarg;
					}
					
					return 0;
				}
				
				if (!issharedtempvar3()) {
					// Before freeing the tempvar, I make sure that
					// it is not set in the field v of the remaining
					// lyricalargument to process.
					
					lyricalvariable* v = arg->v;
					
					lyricalargument* a = arg->next;
					
					while (a != funcarg) {
						
						if (v == a->v) {
							// If I get here, the lyricalargument pointed
							// by a and arg where sharing the same tempvar.
							// I set a->v to a->varpushed to prevent
							// the tempvar from getting freed twice.
							a->v = a->varpushed;
						}
						
						a = a->next;
					}
					
					varfree(v);
				}
			}
		}
		
		lyricalargument* argnext = arg->next;
		mmrefdown(arg);
		arg = argnext;
		
	// Because the linkedlist of pushed argument
	// is a circular linkedlist, if arg == funcarg then
	// I came back to the first argument variable which was
	// already processed and I should stop going through
	// the pushed argument variables.
	} while (arg != funcarg);
	
	funcarg = 0;
	
	return;
}


// This function register the lyricalfunction
// given as argument as having been
// called by currentfunc.
// The work done by this function is
// later used by reviewdatafromfirstpass()
// to find functions that are recursives
// because they called a sibling function
// that called them back again, and to determine
// whether two functions can share the same
// memory region for their stackframes.
// The work done by this function is also
// later used by reviewdatafromfirstpass()
// to find lyricalfunction that must be
// stackframe holder because they called
// a function that is a stackframe holder while
// being called by a function that is a stackframe holder.
// This function is used only in the firstpass.
lyricalcalledfunction* registercalledfunction (lyricalfunction* cf) {
	// currentfunc lyricalcalledfunction to return.
	lyricalcalledfunction* retvar;
	
	++cf->wasused;
	
	lyricalfunction* f;
	
	// This function add the called lyricalfunction
	// pointed by cf to the linkedlist pointed
	// by f->calledfunctions.
	void addcalledfunctiontolist () {
		
		lyricalcalledfunction* calledfunction;
		
		// f->calledfunctions is always
		// non-null when this function
		// is called.
		uint isfunctionalreadyinlist () {
			
			do {
				if (calledfunction->f == cf) {
					// calledfunction->count is incremented
					// only for the currentfunc lyricalcalledfunction,
					// because currentfunc is the lyricalfunction
					// actually calling the lyricalfunction
					// pointed by cf.
					if (f == currentfunc) {
						
						++calledfunction->count;
						
						retvar = calledfunction;
					}
					
					return 1;
				}
				
			} while (calledfunction = calledfunction->next);
			
			return 0;
		}
		
		calledfunction = f->calledfunctions;
		
		if (calledfunction && isfunctionalreadyinlist()) return;
		
		calledfunction = mmalloc(sizeof(lyricalcalledfunction));
		
		calledfunction->f = cf;
		
		// calledfunction->count is set to 1 only
		// for the currentfunc lyricalcalledfunction,
		// because currentfunc is the lyricalfunction
		// actually calling the lyricalfunction
		// pointed by cf.
		if (f == currentfunc) {
			
			calledfunction->count = 1;
			
			retvar = calledfunction;
			
		} else calledfunction->count = 0;
		
		calledfunction->next = f->calledfunctions;
		
		f->calledfunctions = calledfunction;
	}
	
	if (currentfunc != rootfunc) {
		
		f = currentfunc;
		
		do {
			addcalledfunctiontolist();
			
			// I process parent functions as well
			// because when a function is calling
			// another function, it is also as if
			// its parent functions made the call.
			f = f->parent;
			
		} while (f != rootfunc);
	}
	
	return retvar;
}


// Include the file containing the function callfunctionnow();
#include "callfunctionnow.tools.evaluateexpression.parsestatement.lyrical.c"


// This create a new postfixoperatorcall
// and set funcarg to null. It is used
// with postfix operators ++ and -- .
void processpostfixoperatorlater (u8* op, u8* savedcurpos) {
	
	postfixoperatorcall* postfixopcall = mmalloc(sizeof(postfixoperatorcall));
	
	if (linkedlistofpostfixoperatorcall) {
		
		postfixopcall->next = linkedlistofpostfixoperatorcall->next;
		
		linkedlistofpostfixoperatorcall->next = postfixopcall;
		
	} else postfixopcall->next = postfixopcall;
	
	// Set linkedlistofpostfixoperatorcall to
	// the last created postfixoperatorcall.
	linkedlistofpostfixoperatorcall = postfixopcall;
	
	postfixopcall->op = op;
	
	postfixopcall->savedcurpos = savedcurpos;
	
	postfixopcall->arg = funcarg;
	
	// Set funcarg to null since I just used
	// the arguments which were created by pushargument().
	funcarg = 0;
}

// This function is only used within
// evaluationexpression() to process
// postfix operations.
// It will also free the linkedlist
// of postfixoperatorcall pointed by
// linkedlistofpostfixoperatorcall
// and set it back to null.
// The linkedlist of arguments used
// by each postfixoperatorcall structure
// is freed by callfunctionnow()
// as funcarg get set with it.
// linkedlistofpostfixoperatorcall
// must be non-null.
void processpostfixoperatornow () {
	// linkedlistofpostfixoperatorcall point
	// the last postfixoperatorcall added to
	// the linkedlist. The last postfixoperatorcall
	// created will have its field next pointing
	// to the first postfixoperatorcall of the linkedlist.
	// So I select the first added postfixoperatorcall
	// of the linkedlist since I want to process
	// the postix operations in the order
	// they were added in the linkedlist
	// of postfix operation.
	postfixoperatorcall* firstpostfixopcall = linkedlistofpostfixoperatorcall->next;
	postfixoperatorcall* postfixopcall = firstpostfixopcall;
	
	do {
		// The pointer saved in funcarg
		// will be freed by callfunctionnow().
		funcarg = postfixopcall->arg;
		
		// funcarg point to the last pushed argument, while
		// funcarg->next point to the first pushed argument.
		lyricalvariable* firstargv = funcarg->next->varpushed;
		
		// I set curpos to postfixopcall->savedcurpos so that
		// if an error is thrown by callfunctionnow(),
		// curpos is correctly set to the location
		// of the error.
		swapvalues(&curpos, &postfixopcall->savedcurpos);
		
		lyricalvariable* v = callfunctionnow((lyricalvariable*)0, postfixopcall->op, CALLDEFAULT);
		
		if (v && v != EXPRWITHNORETVAL && v != firstargv) {
			// If I have a tempvar or a lyricalvariable
			// which depend on a tempvar (dereference variable
			// or variable with its name suffixed with an offset),
			// I should free all those variables because
			// I will no longer need them and it allow
			// the generated code to save stack memory.
			varfreetempvarrelated(v);
		}
		
		// Here I restore curpos.
		swapvalues(&curpos, &postfixopcall->savedcurpos);
		
		postfixoperatorcall* savedfcall = postfixopcall;
		
		postfixopcall = postfixopcall->next;
		
		// Free the previous value of postfixopcall.
		mmrefdown(savedfcall);
		
	} while (postfixopcall != firstpostfixopcall);
	
	linkedlistofpostfixoperatorcall = 0;
}


// This function match each operator string
// in the array of pointer to strings passed as argument
// against what is at the location pointed by curpos;
// if a match is found, the pointer to the matching
// operator string from the array is returned.
// This function must only be used with
// the arrays normalop, prefixop and postfixop.
// Null is returned if no matching operator was found.
u8* readoperator (u8** oplist) {
	
	do {
		u8* op = *oplist;
		
		// Skip precedence byte if any.
		// A precedence byte value is always less than 0x0f.
		if (*op < 0x0f) ++op;
		
		uint sz = stringsz(op);
		
		if (stringiseq4(curpos, op, sz)) {
			// If I get here I have a match.
			
			// I set curpos after the operator matched.
			curpos += sz;
			
			skipspace();
			
			return op;
		}
		
	} while (*++oplist);
	
	// If I get here there was no matching operator found.
	return 0;
}


// This function read a variable name or read and evaluate
// a function being called through a declared function or
// a pointer to function variable.
// This function will also recognize sizeof() and offsetof().
// When the name of a function without the paranthesis is read,
// a variable which represent its address is returned.
// This function is called with curpos positioned at
// the beginning of what is to be read.
// This function modify curpos.
// This function is used only within evaluateexpression().
lyricalvariable* readvarorfunc () {
	
	string s;
	
	lyricalvariable* v;
	
	u8* savedcurpos = curpos;
	
	if (*curpos == '"') {
		// If I get here, I am reading a string constant.
		
		// I get the lyricalvariable for the string constant
		// using the string returned by readstringconstant().
		// getvarforstringconstant() will do the work of checking
		// whether there was an already created lyricalvariable
		// for the same string constant; if yes there will be
		// no need of creating a new lyricalvariable.
		// Remember that variable for string constant do not
		// have a type, but are only casted to "u8*".
		// The string returned by readstringconstant() will
		// either get used or freed by getvarforstringconstant().
		return getvarforstringconstant(readstringconstant(NORMALSTRINGREADING));
		
	} else if (*curpos == '\'') {
		// If I get here, I am reading a character or
		// multi-character constant. ei: 'a', 'abcd';
		
		readcharconstantresult r = readcharconstant();
		
		// I get the variable number using
		// the value returned by readcharconstant().
		// getvarnumber() will do the work of checking
		// whether there was an already created variable
		// number of the same value, if yes there will
		// be no need of creating a new variable.
		// Remember that variable number do not
		// have a type, but are only casted.
		return getvarnumber(r.n, largeenoughunsignednativetype(r.sz));
		
	} else if (*curpos >= '0' && *curpos <= '9') {
		// If I get here, I am reading a number. ei: 0b10101
		// The resulting variable type is uint.
		
		// There is no need to check whether
		// readnumber().wasread is true,
		// because I certainly read a number
		// since I get here only if I have
		// at least a single digit of a number.
		return getvarnumber(readnumber().n, largeenoughunsignednativetype(sizeofgpr));
		
	} else if ((s = readsymbol(UPPERCASESYMBOL)).ptr) {
		// If I get here, I readed an enum element
		// which is always uppercase.
		
		v = searchvar(s.ptr, stringmmsz(s), INCURRENTANDPARENTFUNCTIONS);
		
		if (!v) {
			
			curpos = savedcurpos;
			
			// The string pointed by s.ptr will be freed by
			// mmsessionfree() upon returning to lyricalcompile();
			throwerror("symbol not usable from this scope");
		}
		
		mmrefdown(s.ptr);
		
		// An enum element value is a constant that is never
		// to occupy memory; hence a lyricalvariable created for
		// an enum element has its field cast set at the time of
		// its creation and is never to change until the lyricalvariable
		// itself is freed; and to prevent the lost of the enum
		// element type set in its field cast, a lyricalvariable
		// created for an enum element is never to be used with
		// pushargument() nor casted; hence here, an equivalent
		// lyricalvariable for a constant number having the same
		// value as the enum element is returned.
		return getvarnumber(v->numbervalue, v->cast);
	}
	
	// If I get here I should expect to read
	// a symbol for a variable name or function name,
	// otherwise I return null.
	if (!(s = readsymbol(LOWERCASESYMBOL)).ptr) return 0;
	
	if (stringiseq2(s, "sizeof")) {
		
		if (*curpos != '(') {
			reverseskipspace();
			throwerror("expecting an opening paranthesis");
		}
		
		++curpos; // Set curpos after '(' .
		skipspace();
		
		u8* savedcurpos3 = curpos;
		
		mmrefdown(s.ptr);
		
		s = readsymbol(LOWERCASESYMBOL);
		
		if (s.ptr && stringiseq2(s, "typeof")) {
			
			if (*curpos != '(') {
				reverseskipspace();
				throwerror("expecting an opening paranthesis");
			}
			
			++curpos; // Set curpos after '(' .
			skipspace();
			
			u8* savedcurpos2 = curpos;
			
			mmrefdown(s.ptr);
			
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
			
			s = v->cast.ptr ?
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
		}
		
		if (!s.ptr || !searchtype(s.ptr, stringmmsz(s), INCURRENTANDPARENTFUNCTIONS)) {
			// If I get here, then I didn't have a valid type,
			// so whatever is in the paranthesis is an expression.
			
			if (s.ptr) mmrefdown(s.ptr);
			
			// Set curpos back to what it was
			// before calling readsymbol().
			curpos = savedcurpos3;
			
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
					
					savedcurpos3 = curpos;
					
				} else break;
			}
			
			// evaluateexpression() return null if nothing
			// like an expression was evaluated, but will
			// return EXPRWITHNORETVAL if an expression
			// was evaluated but no return variable was produced
			// or will return a non-null address containing a resulting
			// lyricalvariable from the evaluation of the expression.
			if (!v || v == EXPRWITHNORETVAL) {
				
				curpos = savedcurpos3;
				
				reverseskipspace();
				
				// The error message below say expecting casting.
				// In fact I got here because what I found was not
				// a cast and should have been an expression.
				// Which is not the case.
				throwerror("expecting a type or an expression");
			}
			
			s = v->cast.ptr ?
				stringduplicate1(v->cast) :
				stringduplicate1(v->type);
			
			// If I have a tempvar or a lyricalvariable
			// which depend on a tempvar (dereference variable
			// or variable with its name suffixed with an offset),
			// I should free all those variables because
			// I will no longer need them and it allow
			// the generated code to save stack memory.
			varfreetempvarrelated(v);
			
			goto readytocomputesizeof;
		}
		
		// If I get here, I was able to read a valid type.
		
		// readsymbol() has already done skipspace()
		// before returning, so no need to do it again.
		// If the type read is void, I read the type
		// specification unless it is a pointer (ei: void*[3]),
		// because a variable type such as void[3] is incorrect.
		if (!stringiseq2(s, "void") || *curpos == '*') parsetypespec(&s);
		
		// Here I parse a pointer to function type if any and append it to s;
		if (*curpos == '(') {
			// When I get here I parsed a type that is a placeholder
			// for the return type since it is before '(';
			
			containparsedstring = s;
			
			// I recursively call parsestatement() with
			// its argument statementparsingflag set
			// to PARSEPOINTERTOFUNCTIONTYPE in order
			// to completely parse the pointer to function type.
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
			
			s = containparsedstring;
			
		} else if (stringiseq2(s, "void")) {
			curpos = savedcurpos;
			throwerror("sizeof(void) cannot be used");
		}
		
		readytocomputesizeof:
		
		if (*curpos != ')') {
			reverseskipspace();
			throwerror("expecting a closing paranthesis");
		}
		
		++curpos; // Set curpos after ')'.
		skipspace();
		
		v = getvarnumber(sizeoftype(s.ptr, stringmmsz(s)), largeenoughunsignednativetype(sizeofgpr));
		
		mmrefdown(s.ptr);
		
		return v;
		
	} else if (stringiseq2(s, "offsetof")) {
		
		if (*curpos != '(') {
			reverseskipspace();
			throwerror("expecting an opening paranthesis");
		}
		
		++curpos; // Set curpos after '(' .
		skipspace();
		
		u8* savedcurpos3 = curpos;
		
		mmrefdown(s.ptr);
		
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
				
				savedcurpos3 = curpos;
				
			} else break;
		}
		
		// evaluateexpression() return null if nothing
		// like an expression was evaluated, but will
		// return EXPRWITHNORETVAL if an expression
		// was evaluated but no return variable was produced
		// or will return a non-null address containing a resulting
		// lyricalvariable from the evaluation of the expression.
		if (!v || v == EXPRWITHNORETVAL) {
			
			curpos = savedcurpos3;
			
			reverseskipspace();
			
			throwerror("expecting an expression");
		}
		
		lyricalvariable* savedv = v;
		
		uint vfieldoffset = processvaroffsetifany(&v);
		
		if (v == savedv) {
			
			curpos = savedcurpos3;
			
			reverseskipspace();
			
			throwerror("expecting an expression that result in a selected struct/pstruct/union member");
		}
		
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
		
		v = getvarnumber(vfieldoffset, largeenoughunsignednativetype(sizeofgpr));
		
		return v;
	}
	
	if (!stringiseq2(s, "retvar") && pamsynmatch2(iskeyword, s.ptr, stringmmsz(s)).start) {
		
		curpos = savedcurpos;
		
		reverseskipspace();
		throwerror("expecting a valid statement");
	}
	
	// If I get here, s contain either the name
	// of a variable or a function. Remember that
	// a symbol name cannot be shared by a type,
	// variable or function within the same scope.
	
	searchsymbolresult r;
	
	// This variable is set to 1 if the symbol
	// was followed by an opening paranthesis for
	// a function call, otherwise it is set to 0.
	uint dofunctioncall;
	
	u8* savedcurpos2 = curpos;
	
	// If the symbol was followed by an opening
	// paranthesis, it is either for a function calling,
	// or for specifying a function signature
	// in order to obtain a pointer to function.
	if (*curpos == '(') {
		
		dofunctioncall = 1;
		
		++curpos; // Set curpos after '(' .
		skipspace();
		
		if (*curpos != ')') {
			// I try reading a string;
			// if that string is a type,
			// then restore curpos and do
			// a pointer to function type parsing.
			
			u8* savedcurpos3 = curpos;
			
			string ss = readsymbol(LOWERCASESYMBOL);
			
			if (ss.ptr) {
				
				if (stringiseq2(ss, "void") && *curpos == ')') {
					
					mmrefdown(ss.ptr);
					
					stringappend2(&s, "||");
					
					++curpos; // Set curpos after ')' .
					skipspace();
					
					goto searchfunctionsignature;
				}
				
				r = searchsymbol(ss, INCURRENTANDPARENTFUNCTIONS);
				
				mmrefdown(ss.ptr);
				
			} else r.s = SYMBOLNOTFOUND;
			
			curpos = savedcurpos3;
			
			if (r.s == SYMBOLISTYPE) {
				// If I get here, the opening paranthesis
				// is for specifying a function signature
				// in order to obtain a pointer to function.
				
				containparsedstring = s;
				
				// I recursively call parsestatement() with
				// its argument statementparsingflag set
				// to PARSEFUNCTIONSIGNATURE in order to
				// completely parse the function signature.
				// The recursive call to parsestatement() below
				// will return after a closing paranthesis and
				// will do a skipspace() before returning.
				
				stringappend4(&containparsedstring, '|');
				
				parsestatement(PARSEFUNCTIONSIGNATURE);
				
				s = containparsedstring;
				
				searchfunctionsignature:
				
				if (r.f = searchfunc(s, INCURRENTANDPARENTFUNCTIONS).f) {
					// It is not necessary to set r.s
					// because only r.f will be used.
					// r.s = SYMBOLISFUNCTION;
					
					goto getpointertofunction;
				}
				
				curpos = savedcurpos;
				
				throwerror("no matching function for the signature");
				
			} else {
				// If I get here, the opening paranthesis
				// is for a function calling.
				// I parse the arguments of the function call.
				
				evaluatefuncarg:;
				
				savedcurpos3 = curpos;
				
				v = evaluateexpression(LOWESTPRECEDENCE);
				
				if (!v || v == EXPRWITHNORETVAL) {
					curpos = savedcurpos3;
					reverseskipspace();
					throwerror("no operand to pass as argument");
				}
				
				// I set curpos to its saved value where
				// the expression for the argument start.
				// It will be used by pushargument() when setting
				// the argument lyricalargumentflag.id .
				swapvalues(&curpos, &savedcurpos3);
				
				// Push as argument the variable resulting
				// from the evaluation of the expression.
				// If the variable had its field bitselect set,
				// pushargument() will use it and set it back
				// to null so that it do not get to be used
				// until set again.
				pushargument(v);
				
				// I restore curpos.
				swapvalues(&curpos, &savedcurpos3);
				
				// Arguments to a function-call
				// are seperated with a comma.
				if (*curpos == ',') {
					
					++curpos; // Set curpos after ',' .
					skipspace();
					
					if (*curpos == ')') throwerror("unexpected");
					
					goto evaluatefuncarg;
					
				} else if (*curpos != ')') throwerror("expecting ')'");
			}
		}
		
		++curpos; // Set curpos after ')' .
		skipspace();
		
	} else dofunctioncall = 0;
	
	r = searchsymbol(s, INCURRENTANDPARENTFUNCTIONS);
	
	switch (r.s) {
		
		case SYMBOLISVARIABLE:
			// If I get here, it mean that
			// s contain the name of a variable.
			
			v = r.v;
			
			// The cast should be discarded for
			// subsequent computations if it was set.
			if (v->cast.ptr) {
				mmrefdown(v->cast.ptr);
				v->cast = stringnull;
			}
			
			// If the variable retrieved is a predeclared variable,
			// I set the field cast; note that prediclared variables
			// do not have their field type set.
			if (v->predeclared) v->cast = stringduplicate2(v->predeclared->type);
			
			// If the variable retrieved is a byref variable,
			// it should be automatically dereferenced.
			// Dereferencing will always be successful because
			// the type of a byref variable is always a pointer.
			if (v->isbyref) v = dereferencevar(v);
			
			if (dofunctioncall) {
				// If I get here, I am calling a function
				// through a variable pointer to function.
				
				// I make sure that the type of the variable
				// is a valid pointer to function.
				if (v->cast.ptr) {
					
					if (v->cast.ptr[stringmmsz(v->cast)-1] != ')') {
						
						curpos = savedcurpos;
						
						// The string pointed by s.ptr will be freed by
						// mmsessionfree() upon returning to lyricalcompile();
						throwerror("expecting a pointer to function");
					}
					
				} else {
					
					if (v->type.ptr[stringmmsz(v->type)-1] != ')') {
						
						curpos = savedcurpos;
						
						// The string pointed by s.ptr will be freed by
						// mmsessionfree() upon returning to lyricalcompile();
						throwerror("expecting a pointer to function");
					}
				}
				
				// I set curpos to savedcurpos2 so that
				// if an error is thrown by callfunctionnow(),
				// curpos is correctly set to the location
				// of the error.
				swapvalues(&curpos, &savedcurpos2);
				
				// callfunctionnow() never return a null pointer
				// and return EXPRWITHNORETVAL if there
				// was no resulting variable.
				v = callfunctionnow(v, (u8*)0, CALLDEFAULT);
				
				// Here I restore curpos.
				swapvalues(&curpos, &savedcurpos2);
			}
			
			break;
			
		case SYMBOLISFUNCTION:
			// If I get here, it mean that
			// s contain the name of a function.
			
			if (dofunctioncall) {
				// If I get here, I am calling a function.
				
				// I set curpos to savedcurpos2 so that
				// if an error is thrown by callfunctionnow(),
				// curpos is correctly set to the location
				// of the error.
				swapvalues(&curpos, &savedcurpos2);
				
				// callfunctionnow() never return
				// a null pointer and return EXPRWITHNORETVAL
				// if there was no resulting variable.
				v = callfunctionnow((lyricalvariable*)0, s.ptr, CALLDEFAULT);
				
				// Here I restore curpos.
				swapvalues(&curpos, &savedcurpos2);
				
			} else {
				// I set in v, a pointer to a variable
				// representing the address of the function
				// pointed by r.f.
				
				getpointertofunction:
				
				if (!rootfunc->vlocal) goto createnewvar;
				
				// I search if the variable address for the
				// function pointed by r.f was already created.
				
				v = rootfunc->vlocal;
				
				while (1) {
					
					if (v->isfuncaddr == r.f) {
						// Constant variable do not have
						// a type and always have a cast.
						// I free the cast that is set because
						// it could have been set to another type
						// different from what it was initially set to.
						// I will freshly set it later.
						if (v->cast.ptr) mmrefdown(v->cast.ptr);
						
						break;
					}
					
					v = v->next;
					
					if (v == rootfunc->vlocal) {
						
						createnewvar:
						
						// I get here if I couldn't find
						// the variable address for the function
						// pointed by r.f, so I need to create
						// a new variable for it.
						v = newvarglobal(stringnull, stringnull);
						
						// I set the field that make the lyricalvariable
						// an address variable for the function pointed by r.f .
						v->isfuncaddr = r.f;
						
						break;
					}
				}
				
				v->cast = stringduplicate1(r.f->typeofaddr);
				
				if (!compilepass) {
					// I could have set r.f->itspointerisobtained
					// within getregforvar() when the lyricalvariable
					// address of function that will be created get
					// actually used; but getregforvar() is called
					// only in the secondpass and the field itspointerisobtained
					// of a lyricalfunction need to be set in the firstpass.
					// Furthermore, it is the responsability
					// of the programmer to use a function name symbol
					// in a program only when it is going to be used.
					r.f->itspointerisobtained = 1;
					
					// Since the address of a function
					// is being obtained, it is probably
					// to be called; hence I register it
					// as having been called.
					registercalledfunction(r.f);
				}
			}
			
			break;
			
		case SYMBOLISTYPE:
			// If I get here, it mean that
			// s contain the name of a type.
			
			curpos = savedcurpos;
			
			// The string pointed by s.ptr will be freed by
			// mmsessionfree() upon returning to lyricalcompile().
			throwerror("invalid use of type symbol");
			
			break;
			
		default:
			
			curpos = savedcurpos;
			
			// The string pointed by s.ptr will be freed by
			// mmsessionfree() upon returning to lyricalcompile().
			throwerror("symbol not usable from this scope");
	}
	
	mmrefdown(s.ptr);
	
	return v;
}
