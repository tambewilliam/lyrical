
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


// This function will modify curpos.
// It must be called when curpos is positioned
// at the beginning of the expression to parse.
// This function will be called recursively until
// an expression has been completely parsed.
// When there is no expression parsed, or when
// the argument larg == DOASM, null is returned.
// If an expression is read and evaluated but is not
// returning a value, EXPRWITHNORETVAL is returned;
// When the argument larg is set to an actual lyricalvariable,
// it become the left argument of a normal operator.
lyricalvariable* evaluateexpression (lyricalvariable* larg) {
	// This variable point to the last created
	// argument in the linkedlist of arguments
	// which is created using pushargument().
	// The field next of the last created element
	// pointed by funcarg point to the first
	// argument in the linkedlist.
	// After use, the memory used by
	// the linkedlist is freed and funcarg
	// is set back to null by freefuncarg();
	// When using processpostfixoperatorlater()
	// a new postfixoperatorcall is allocated
	// and the value of funcarg is saved in
	// the created postfixoperatorcall and
	// funcarg is set back to null.
	// Note that the linkedlist pointed by
	// funcarg contain only the pushed arguments
	// that will be processed by this
	// call instance of evaluateexpression().
	lyricalargument* funcarg = 0;
	
	// This includes the file containing functions used within this function.
	#include "tools.evaluateexpression.parsestatement.lyrical.c"
	
	// Used to save the value of curpos when necessary.
	u8* savedcurpos;
	
	// This variable is set to 1 when initializing a variable, otherwise it is set to 0.
	uint initializingvar;
	
	// This variable get set to the string
	// for the operator to evaluate.
	u8* op;
	
	// This variable hold the precedence value
	// of the operator for which the argument
	// is about to be parsed.
	uint prevopprecedence;
	
	// Will hold the operator precedence value
	// computed using op which must be pointing
	// to within a constant string from
	// the array normalop.
	uint opprecedence;
	
	if (larg == DOPOSTFIXOPERATIONS) {
		// Process postfix operations.
		// linkedlistofpostfixoperatorcall
		// is assumed non-null.
		processpostfixoperatornow();
		
		return 0;
		
	} else if (larg == DOASM) {
		// This includes the file containing the routine used to evaluate assembly instructions.
		#include "assembly.evaluateexpression.parsestatement.lyrical.c"
		
		// I will never get here as returning
		// is done in the above include.
		
	} else if (larg <= (lyricalvariable*)0x0f) {
		// If I get here, larg hold a precedence value
		// to be compared against the precedence value
		// of the subsequent normal operator to be parsed.
		// The above check assume that the first page
		// of a process address space is never mapped,
		// therefore it is safe to assume that
		// an actual lyricalvariable will never be
		// allocated within that page.
		
		initializingvar = 0;
		
		prevopprecedence = (uint)larg;
		
		larg = 0;
		
	} else {
		// I get here if evaluateexpression() was called
		// to process the initialization of a variable.
		
		initializingvar = 1;
		
		savedcurpos = curpos;
		
		if (*curpos == '.') {
			
			++curpos;
			
			// It is necessary to set op because
			// when jumping to parsemethodormemberfield
			// and an error occur, it is used
			// to select the correct error message.
			op = postfixop[POSTFIXOPDOT];
			
			prevopprecedence = (uint)HIGHESTPRECEDENCE;
			
			skipspace();
			
			goto parsemethodormemberfield;
			
		} else {
			// If I get here, I am initializing
			// a variable using the operator '=', and
			// (*curpos == '=') is guaranteed to be true.
			
			++curpos;
			
			// It is necessary to set op because
			// when jumping to initializingvarwithassign
			// the string pointed by op is used
			// as argument to callfunctionnow().
			// +1 account for the precedence byte
			// value prepended to the string.
			op = normalop[NORMALOPASSIGN]+1;
			
			opprecedence = *(op-1);
			
			skipspace();
			
			goto initializingvarwithassign;
		}
	}
	
	string s; // Used for multipurposes.
	
	// I start the evaluation of an expression by checking if I have an opening paranthesis
	// which can be used to enclose another expression or for doing casting.
	if (*curpos == '(') {
		
		++curpos; // Set curpos after the opening paranthesis.
		skipspace();
		
		// If I get here I check whether the opening paranthesis
		// is either for a casting or for an expression.
		
		savedcurpos = curpos; // This will save curpos so that I can restore it later.
		
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
			
			goto typewasread;
		}
		
		// The test below show that the name of a type cannot be the same as the name of
		// a variable, otherwise a variable having the same name as a type could be thought
		// to be used for casting whereas it was simply being part of an expression.
		if (!s.ptr || !searchtype(s.ptr, stringmmsz(s), INCURRENTANDPARENTFUNCTIONS)) {
			// If I get here, then I didn't have a valid type,
			// so whatever is in the paranthesis is an expression.
			
			if (s.ptr) mmrefdown(s.ptr);
			
			// Set curpos back to what it was
			// before calling readsymbol().
			curpos = savedcurpos;
			
			while (1) {
				
				larg = evaluateexpression(LOWESTPRECEDENCE);
				
				if (*curpos == ',') {
					
					if (larg && larg != EXPRWITHNORETVAL) {
						// If I have a tempvar or a lyricalvariable
						// which depend on a tempvar (dereference variable
						// or variable with its name suffixed with an offset),
						// I should free all those variables because
						// I will no longer need them and it allow
						// the generated code to save stack memory.
						varfreetempvarrelated(larg);
					}
					
					++curpos; // Set curpos after ',' .
					skipspace();
					
				} else break;
			}
			
			// evaluateexpression() return null if nothing
			// like an expression was evaluated, but will
			// return EXPRWITHNORETVAL if an expression
			// was evaluated but no return variable was produced
			// or will return a non-null address containing a resulting
			// lyricalvariable from the evaluation of the expression.
			if (!larg) {
				
				reverseskipspace();
				
				// The error message below say expecting casting.
				// In fact I got here because what I found was not
				// a cast and should have been an expression.
				// Which is not the case.
				throwerror("expecting a type or an expression");
			}
			
			// When evaluateexpression() return,
			// I should expect the closing paranthesis.
			if (*curpos != ')') {
				reverseskipspace();
				throwerror("expecting a closing paranthesis");
			}
			
			++curpos; // Set curpos after the closing paranthesis.
			skipspace();
			
			// I jump where postfix operators are evaluated because
			// I could have had an expression such as: (a+b)->field;
			goto parsepostfixop;
		}
		
		typewasread:;
		
		// If I get here, I was able to read a valid type.
		
		// readsymbol() has already done skipspace()
		// before returning, so no need to do it again.
		// If the type read is void, I read the type specification
		// unless it is a pointer (ei: void*[3]), because
		// a variable type such as void[3] is incorrect.
		if (!stringiseq2(s, "void") || *curpos == '*') parsetypespec(&s);
		
		// Here I parse a pointer to function type if any and append it to s;
		if (*curpos == '(') {
			// When I get here, I am parsing a type
			// that is a place holder for the return
			// type since it is before '(';
			
			containparsedstring = s;
			
			// I recursively call parsestatement() with
			// its argument statementparsingflag set
			// to PARSEPOINTERTOFUNCTIONTYPE in order
			// to completely parse the pointer to function type.
			// The parsed type will be appended.
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
			
		} else if (stringiseq2(s, "void")) throwerror("invalid use of void");
		
		if (*curpos != ')') {
			// If I get here, I create a tempvar
			// which is to be initialized.
			
			larg = varalloc(sizeoftype(s.ptr, stringmmsz(s)), LOOKFORAHOLE);
			
			larg->name = generatetempvarname(larg);
			
			// The string in s was allocated
			// and will not be used elsewhere,
			// so I am ok to set it as the type
			// string instead of duplicating it.
			larg->type = s;
			
			readinit(larg);
		}
		
		if (*curpos != ')') {
			reverseskipspace();
			throwerror("expecting ')'");
		}
		
		++curpos; // Set curpos after ')'.
		
		savedcurpos = curpos;
		
		skipspace();
		
		// If a variable to initialize was created,
		// I continue with parsing for a postfix
		// operator instead of casting.
		if (larg) goto parsepostfixop;
		
		larg = evaluateexpression(HIGHESTPRECEDENCE);
		
		if (!larg || larg == EXPRWITHNORETVAL) {
			
			curpos = savedcurpos;
			
			throwerror("no operand to cast");
			
		} else if (larg->bitselect) {
			// A cast cannot be applied to a bitselected variable
			// because it will cause the wrong size to be reported
			// to getregforvar() and the bitselect when used will not
			// match the size for which it was initially created;
			// hence, I duplicate it.
			
			lyricalvariable* largbitselected = larg;
			
			larg = 0;
			
			getvarduplicate(&larg, largbitselected);
			
			// If I have a tempvar or a lyricalvariable
			// which depend on a tempvar (dereference variable
			// or variable with its name suffixed with an offset),
			// I should free all those variables because
			// I will no longer need them and it allow
			// the generated code to save stack memory.
			varfreetempvarrelated(largbitselected);
		}
		
		// I cast the resulting variable
		// in larg to the type in s.
		if (larg->cast.ptr) mmrefdown(larg->cast.ptr);
		// The string in s was allocated
		// and will not be used elsewhere,
		// so I am ok to set it as the cast
		// string instead of duplicating it.
		larg->cast = s;
		
		// When evaluateexpression() return,
		// I expect a normal operator or the end
		// of the expression. Prefix and postfix operators
		// are evaluated in the recursive call
		// to evaluateexpression() that I did above.
		
		goto parsenormalop;
	}
	
	// If I get here, there was no paranthesis,
	// so I try parsing for prefix operators.
	
	savedcurpos = curpos;
	
	op = readoperator(prefixop);
	
	if (op) {
		
		u8* savedcurpos2 = curpos;
		
		larg = evaluateexpression(HIGHESTPRECEDENCE);
		
		if (!larg || larg == EXPRWITHNORETVAL) {
			curpos = savedcurpos;
			throwerror("no operand to use with prefix operator");
		}
		
		// Using the previously read operator pointed by op,
		// I process the variable pointed by larg.
		
		if (*op == '*') {
			// There is no need to check whether
			// the variable pointed by larg has a bitselect set.
			// In fact a bitselected variable come from
			// a non-pointer variable member of a struct
			// and cannot be casted and will never get
			// a pointer type; hence if the type is not
			// a valid pointer type, dereferencevar() will
			// throw an error.
			
			// Since dereferencevar() do not use curpos,
			// I set curpos to savedcurpos so that
			// if an error is thrown by dereferencevar(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos);
			
			larg = dereferencevar(larg);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos);
			
		} else if (*op == '&') {
			// I check if the variable pointed
			// by larg has a bitselect set; if yes,
			// I throw an error, because it is impossible
			// to represent the address of individual bits.
			// For a similar reason, the operator '&'
			// cannot be used with readonly variables.
			if (larg->bitselect || isvarreadonly(larg)) {
				curpos = savedcurpos;
				throwerror("this variable address cannot be obtained");
			}
			
			// Since getvaraddr() do not use curpos,
			// I set curpos to savedcurpos so that
			// if an error is thrown by getvaraddr(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos);
			
			larg = getvaraddr(larg, MAKEALWAYSVOLATILE);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos);
			
		} else {
			// I get here for any other prefix operator.
			
			// I set curpos to its saved value where
			// the expression for the argument start.
			// It will be used by pushargument() when setting
			// the argument lyricalargumentflag.id .
			swapvalues(&curpos, &savedcurpos2);
			
			// If the variable pointed by larg had a bitselect,
			// pushargument() will use it and set the field
			// bitselect of the variable pointed by larg
			// to null so that it do not get to be used
			// anywhere else until set again.
			pushargument(larg);
			
			// I restore curpos.
			swapvalues(&curpos, &savedcurpos2);
			
			// I set curpos to savedcurpos so that
			// if an error is thrown by callfunctionnow(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos);
			
			// Call the operator function.
			// callfunctionnow() never return a null pointer
			// and return EXPRWITHNORETVAL if there was
			// no resulting variable.
			larg = callfunctionnow((lyricalvariable*)0, op, CALLDEFAULT);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos);
		}
		
		// When I get here I jump to parsenormalop.
		// Any other prefix or postfix operators
		// were processed in the recursive call of
		// evaluateexpression() that I did before
		// checking which prefix operator was used.
		
		goto parsenormalop;
	}
	
	// If I get here, there was no prefix operator
	// to read, so I expect to read a constant,
	// variable or function. If I am also unable
	// to read that, I should return null which mean
	// that there was no expression to evaluate.
	
	// If readvarorfunc() find a variable,
	// it will return the variable; if it find
	// a function it will evaluate it and return
	// any resulting variable or EXPRWITHNORETVAL
	// if an expression was evaluated but no return variable
	// was produced. If there was no variable to read or
	// function to evaluate, null is returned.
	larg = readvarorfunc();
	
	// I return null if readvarorfunc() returned null; it mean
	// that there was no expressions to read or to evaluate.
	if (!larg) return 0;
	
	// If I get here a variable was read or
	// a function was evaluated, so I need
	// to check if a postfix operator was used.
	
	parsepostfixop:;
	
	savedcurpos = curpos;
	
	op = readoperator(postfixop);
	
	if (op) {
		
		lyricalvariable* v;
		
		// Here I make sure that larg is not EXPRWITHNORETVAL
		// which mean that there is no variable to apply
		// the postfix operator to it.
		// Note that larg cannot be found null when
		// I get here because there is always a test that
		// prevent it to be null by the time I get here.
		if (larg == EXPRWITHNORETVAL) {
			curpos = savedcurpos;
			throwerror("no operand to use with postfix operator");
		}
		
		if (*op == '[') {
			// There is no need to check whether the
			// variable pointed by larg has a bitselect set.
			// In fact bitselected variables come from
			// a variable member of struct which is not
			// an array and a bitselected variable cannot
			// be casted and will not get an array type;
			// hence an array type can never be bitselected.
			
			// Array manipulations is implemented as follow:
			// 
			// uint[3][7] var1;
			// var1[index] implementation depend on whether
			// the index expression is a constant.
			// If it is a constant, it yield a lyricalvariable
			// with its name suffixed with an offset. ei: "var1.8"
			// If it is not a constant, it is implemented doing:
			// *(&(uint[3])var1 + index);
			// 
			// uint* var2;
			// var2[index] is implemented doing: *(var2 + index);
			
			string vartype;
			
			// I save the cast of the lyricalvariable pointed by larg
			// before evaluating the index which is the expression within
			// the brackets '[' and ']'; because it can get modified
			// while processing the index.
			if (larg->cast.ptr) vartype = stringduplicate1(larg->cast);
			else vartype = stringduplicate1(larg->type);
			
			// I evaluate the index which is the expression
			// within the brackets '[' and ']';
			
			v = evaluateexpression(LOWESTPRECEDENCE);
			
			if (*curpos != ']') {
				reverseskipspace();
				throwerror("expecting ']'");
			}
			
			if (!v || v == EXPRWITHNORETVAL) {
				curpos = savedcurpos;
				throwerror("no operand to use as array index");
			}
			
			if (v->cast.ptr) s = v->cast;
			else s = v->type;
			
			if (!pamsynmatch2(isnativetype, s.ptr, stringmmsz(s)).start) {
				curpos = savedcurpos;
				throwerror("the expression for the index can only be of a native type");
			}
			
			// I process the variable in larg.
			
			if (vartype.ptr[stringmmsz(vartype)-1] == ']') {
				// If I have an address variable or constant variable
				// and their type is an array, I cannot do array manipulation
				// on them, because they do not reside in memory
				// and I cannot obtain their address.
				if (isvarreadonly(larg)) {
					curpos = savedcurpos;
					throwerror("array manipulations cannot be done on constant variables unless they are pointers");
				}
				
				// I find the type to which the last array spec is stripped.
				// ei: "uint[3][7]" become "uint[3]";
				
				u8* sptr = vartype.ptr;
				uint ssz = stringmmsz(vartype);
				
				do --ssz; while (sptr[ssz-1] != '[');
				
				--ssz;
				
				if (v->isnumber) {
					// Since the index is a constant integer value,
					// I generate a lyricalvariable having its name
					// suffixed with a offset using the index value.
					
					if (v->numbervalue > maxhostuintvalue) {
						curpos = savedcurpos;
						throwerror("index value beyond host capability");
					}
					
					uint vnumbervalue = v->numbervalue;
					
					// Note that the number of elements of an array is never zero.
					uint arraysize = stringconverttoint2(sptr+ssz+1, (stringmmsz(vartype)-ssz)-2);
					
					if (vnumbervalue >= arraysize) {
						curpos = savedcurpos;
						throwerror("index value beyond the size of the array");
					}
					
					uint sizeoftypesptrssz = sizeoftype(sptr, ssz);
					
					// This variable will contain the numerical value of the offset
					// of the string to use in the name of the variable to produce.
					// The stride of the array is taken into account.
					uint fieldoffset = vnumbervalue*sizeoftypesptrssz;
					
					// Check whether the above multiplication overflowed.
					if (vnumbervalue && (fieldoffset/vnumbervalue) != sizeoftypesptrssz) {
						curpos = savedcurpos;
						throwerror("indexed element beyond host capability");
					}
					
					// processvaroffsetifany() find the main variable and set it
					// in larg; I use its returned value to update fieldoffset which
					// will give me the overall offset from the main variable that
					// I need to use with the name of the variable to produce.
					fieldoffset += processvaroffsetifany(&larg);
					
					// If the variable is one that is explicitly
					// declared by the programmer as oppose to
					// a dereference variable, I make sure that
					// the region located at the offset computed
					// in fieldoffset is within the valid size
					// of the variable.
					// It would be illegal to try to make an access
					// beyond the size of the variable.
					// There is no need to check whether
					// sizeoftype(sptr, ssz) > (larg->size - fieldoffset)
					// because getregforvar() adjust the size to read
					// if it spill outside the variable boundary.
					if (larg->size && fieldoffset >= larg->size) {
						curpos = savedcurpos;
						throwerror("illegal access beyond the size of the variable");
					}
					
					// This variable will contain the uint pointer pointed by
					// the field alwaysvolatile of the main variable of the variable
					// having its name suffixed with an offset that I am going to generate.
					uint* fieldalwaysvolatile = larg->alwaysvolatile;
					
					// I duplicate the name of the main variable
					// to which I suffix the offset starting with the dot.
					string name = stringfmt("%s.%d", larg->name, fieldoffset);
					
					// I search for the variable name that I generated in name
					// if it was already created within the current scope.
					if (larg = searchvar(name.ptr, stringmmsz(name), INCURRENTSCOPEONLY)) {
						// If I get here, then the variable had previously been created.
						
						// The string pointed by name.ptr need to be deallocated
						// to avoid memory leak, since I am not creating a variable,
						// where it would have been used to set the name of the variable.
						mmrefdown(name.ptr);
						
						// Variable with suffixed offset only have a cast and
						// their cast is always set to something; so I do not need
						// to check if larg->cast.ptr is null before freeing it.
						// I free it here in order to set a new cast later.
						if (larg->cast.ptr) mmrefdown(larg->cast.ptr);
						
					} else {
						// If I get here, then I need to create the variable.
						// Note that because the name of the variable is suffixed
						// with an offset, newvarlocal() should not use the argument type,
						// so I use a null string for the argument type. The cast of
						// the new variable is set later.
						larg = newvarlocal(name, stringnull);
					}
					
					// I need to remember that variables with suffixed offset
					// do not have a type; so I only set their cast.
					larg->cast = stringduplicate3(sptr, ssz);
					
					// The field alwaysvolatile is a pointer to a uint shared by
					// a main variable and variables having their name suffixed with
					// an offset which refer to that main variable; so here I point
					// the field alwaysvolatile to the same uint that was set in
					// the field alwaysvolatile of its main variable.
					// There can be a situation where within a function, I could be using
					// an external main variable up to a point where a main variable
					// with the same name as the external main variable is created locally;
					// and if a variable having its name suffixed with an offset was created
					// before the main variable having the same name as the external
					// main variable, the field alwaysvolatile of the variable having
					// its name suffixed with an offset will point instead to the uint used
					// with the external main variable. Hence it is necessary to everytime
					// set the field alwaysvolatile of the variable having
					// its name suffixed with an offset.
					larg->alwaysvolatile = fieldalwaysvolatile;
					
				} else {
					
					if (larg->cast.ptr) mmrefdown(larg->cast.ptr);
					
					larg->cast = stringduplicate3(sptr, ssz);
					
					// I set curpos to its saved value at '['.
					// It will be used by pushargument() when setting
					// the argument lyricalargumentflag.id .
					swapvalues(&curpos, &savedcurpos);
					
					// Here because the address of the variable is obtained
					// and its content will be accessed through dereferencing
					// when manipulating the array, it is necessary to mark
					// the variable as always volatile to prevent aliasing.
					pushargument(getvaraddr(larg, MAKEALWAYSVOLATILE));
					
					// I restore curpos.
					swapvalues(&curpos, &savedcurpos);
					
					// curpos is at ']' and will be used by pushargument()
					// when setting the argument lyricalargumentflag.id .
					
					// Push the variable resulting from the evaluation of
					// the index expression. If the variable has its field bitselect set,
					// pushargument() will use it and set it back to null so that
					// it do not get to be used anywhere else until set again.
					pushargument(v);
					
					larg = dereferencevar(callfunctionnow(
						(lyricalvariable*)0,
						// +1 account for the precedence byte
						// value prepended to the string.
						normalop[NORMALOPPLUS]+1,
						CALLNATIVEOP));
				}
				
				mmrefdown(vartype.ptr);
				
			} else if (vartype.ptr[stringmmsz(vartype)-1] == '*') {
				
				if (larg->cast.ptr) mmrefdown(larg->cast.ptr);
				
				// I restore the type of the lyricalvariable pointed by larg
				// in case it was modified while processing the index expression.
				larg->cast = vartype;
				
				// I set curpos to its saved value at '['.
				// It will be used by pushargument() when setting
				// the argument lyricalargumentflag.id .
				swapvalues(&curpos, &savedcurpos);
				
				pushargument(larg);
				
				// I restore curpos.
				swapvalues(&curpos, &savedcurpos);
				
				// curpos is at ']' and will be used by pushargument()
				// when setting the argument lyricalargumentflag.id .
				
				// Push the variable resulting from the evaluation of
				// the index expression. If the variable has its field bitselect set,
				// pushargument() will use it and set it back to null so that
				// it do not get to be used anywhere else until set again.
				pushargument(v);
				
				larg = dereferencevar(callfunctionnow(
					(lyricalvariable*)0,
					// +1 account for the precedence byte
					// value prepended to the string.
					normalop[NORMALOPPLUS]+1,
					CALLNATIVEOP));
				
			} else {
				curpos = savedcurpos;
				throwerror("array manipulations can only be done on array type or pointers");
			}
			
			++curpos; // Set curpos after ']' .
			skipspace();
			
		} else if (stringiseq3(op, "->")) {
			// If I get here, I need to dereference larg since
			// var->field == (*var).field;
			// Hence it will generate a variable such as "(*var).2";
			// If I had a case such as:
			// v.field->field; It would generate
			// a variable such as "(*v.2).2" instead.
			
			// There is no need to check whether
			// the variable pointed by larg has a bitselect set.
			// In fact a bitselected variable come from
			// a non-pointer variable member of a struct
			// and cannot be casted and will never get
			// a pointer type, hence if the type is not
			// a valid pointer type, dereferencevar()
			// will throw an error.
			
			// Since dereferencevar() do not use curpos,
			// I set curpos to savedcurpos so that
			// if an error is thrown by dereferencevar(),
			// curpos is correctly set to the location
			// of the error.
			swapvalues(&curpos, &savedcurpos);
			
			larg = dereferencevar(larg);
			
			// Here I restore curpos.
			swapvalues(&curpos, &savedcurpos);
			
			// Now that I have dereferenced the variable, I can
			// treat it as if I had the operator '.' instead of "->";
			goto parsemethodormemberfield;
			
		} else if (*op == '.') {
			
			parsemethodormemberfield:
			
			// Read the symbol after the operator '.' or "->" .
			s = readsymbol(LOWERCASESYMBOL);
			
			if (!s.ptr) {
				
				reverseskipspace();
				
				if (*op == '.') throwerror("expecting a symbol name after the operator '.'");
				else throwerror("expecting a symbol name after the operator '->'");
			}
			
			// If I get here then the symbol name read in s can be a member of
			// the cast or type of the variable pointed by larg or a function name.
			
			// This variable get set to 1 if the symbol name
			// readed in s will be used to call a function.
			uint domethodcall;
			
			// If the variable is being initialized
			// using the postfix operator '.' ,
			// I expect a method call.
			if (initializingvar) {
				
				if (*curpos == '(') {
					domethodcall = 1;
					goto methodcall;
				}
				
				reverseskipspace();
				throwerror("expecting a method call");
			}
			
			// Make sure that the variable is not generated by the compiler because
			// the operator '.' can only be applied to variables which were not generated
			// by the compiler and which resides in memory.
			// Note that I only considered the operator '.' in the error message because when
			// the operator -> is used the variable is dereferenced, hence the resulting variable
			// is always able to have a field if the appropriate type is used.
			if (isvarreadonly(larg)) {
				// If I am making a method call, I should not throw an error.
				if (*curpos == '(') {
					domethodcall = 1;
					goto methodcall;
				}
				
				curpos = savedcurpos;
				throwerror("incorrect use of the operator '.'");
			}
			
			// There is no need to check whether the variable pointed by larg has a bitselect set.
			// In fact bitselected variables come from a variable member of structs having a native
			// type and cannot be casted. Hence if the variable is bitselected I will not be able
			// to used the operator '.' to select a field because native type do not have member
			// variable.
			
			string vartype;
			
			if (larg->cast.ptr) vartype = larg->cast;
			else vartype = larg->type;
			
			// Pointer types do not have member variables. Arrays need to have an element
			// completely indexed first before I can look if it has members.
			if (vartype.ptr[stringmmsz(vartype)-1] == '*' ||
			vartype.ptr[stringmmsz(vartype)-1] == ')' ||
			vartype.ptr[stringmmsz(vartype)-1] == ']') {
				// If I am making a method call, I should not throw an error.
				if (*curpos == '(') {
					domethodcall = 1;
					goto methodcall;
				}
				
				curpos = savedcurpos;
				throwerror("array and pointer types do not have members");
			}
			
			// I retrieve the type, which will be used to obtain
			// the offset and the bitselect if any. Remember that searchtype() cannot
			// retrieve a type if it is a pointer or an array, but I am safe because I made sure
			// that it was not a pointer or array before getting here.
			// If the field v of the structure type is null the type has no member variables
			// and I cannot use it.
			// 	I do not need to check whether t is null because searchtype()
			// will always successfully find a type because previous checks insured that
			// larg was casted with a valid type.
			lyricaltype* t;
			if (!(t = searchtype(vartype.ptr, stringmmsz(vartype), INCURRENTANDPARENTFUNCTIONS))->v) {
				// If I am making a method call, I should not throw an error.
				if (*curpos == '(') {
					domethodcall = 1;
					goto methodcall;
				}
				
				curpos = savedcurpos;
				throwerror("use of a type which do not have any member");
			}
			
			// I now try to obtain the member which name
			// is saved in s, from the type pointed by t.
			if (!(v = searchtypemember(t, s))) {
				// If I am making a method call, I should not throw an error.
				if (*curpos == '(') {
					domethodcall = 1;
					goto methodcall;
				}
				
				curpos = savedcurpos;
				throwerror("unknown type member");
			}
			
			// I free the memory used by the string pointed by s.ptr
			// to avoid memory leaks, since I only needed it to search
			// for the type member.
			// Note that the string s will not be freed if above I made
			// a jump to methodcall where s would contain the name of
			// the function to call.
			mmrefdown(s.ptr);
			
			// This variable will contain the numerical value of the offset
			// of the string to use in the name of the variable to produce.
			uint fieldoffset = v->offset;
			
			// processvaroffsetifany() finds the main variable and set it in larg; I use its
			// returned value to update fieldoffset which will give me the overall offset from
			// the main variable that I need to use with the name of the variable to produce.
			fieldoffset += processvaroffsetifany(&larg);
			
			// If the variable is one that is explicitly
			// declared by the programmer as oppose to
			// a dereference variable, I make sure that
			// the region located at the offset computed
			// in fieldoffset is within the valid size
			// of the variable.
			// It would be illegal to try to make an access
			// beyond the size of the variable.
			// There is no need to check whether
			// sizeoftype(v->type) > (larg->size - fieldoffset)
			// because getregforvar() adjust the size to read
			// if it spill outside the variable boundary.
			if (larg->size && fieldoffset >= larg->size) {
				curpos = savedcurpos;
				throwerror("illegal access beyond the size of the variable");
			}
			
			// This variable will contain the uint pointer pointed by
			// the field alwaysvolatile of the main variable of the variable
			// having its name suffixed with an offset that I am going to generate.
			uint* fieldalwaysvolatile = larg->alwaysvolatile;
			
			// I duplicate the name of the main variable
			// to which I suffix the offset starting with the dot.
			s = stringfmt("%s.%d", larg->name, fieldoffset);
			
			// I search for the variable name that I generated in s
			// if it was already created within the current scope.
			if (larg = searchvar(s.ptr, stringmmsz(s), INCURRENTSCOPEONLY)) {
				// If I get here, then the variable had previously been created.
				
				// The string pointed by s.ptr need to be deallocated
				// to avoid memory leak, since I am not creating a variable,
				// where it would have been used to set the name of the variable.
				mmrefdown(s.ptr);
				
				// Variable with suffixed offset only have a cast and
				// their cast is always set to something; so I do not need
				// to check if larg->cast.ptr is null before freeing it.
				// I free it here in order to set a new cast later.
				if (larg->cast.ptr) mmrefdown(larg->cast.ptr);
				
			} else {
				// If I get here, then I need to create the variable.
				// Note that because the name of the variable is suffixed
				// with an offset, newvarlocal() should not use the argument type,
				// so I use a null string for the argument type. The cast of
				// the new variable is set later.
				larg = newvarlocal(s, stringnull);
			}
			
			// I need to remember that variables with suffixed offset
			// do not have a type; so I only set their cast.
			larg->cast = stringduplicate1(v->type);
			
			// Here I set the field bitselect of the variable pointed by larg;
			// note that it is temporarily set and the variable will see
			// its field bitselect set back to null after it has been used.
			// ei: by pushargument().
			larg->bitselect = v->bitselect;
			
			// The field alwaysvolatile is a pointer to a uint shared by
			// a main variable and variables having their name suffixed with
			// an offset which refer to that main variable; so here I point
			// the field alwaysvolatile to the same uint that was set in
			// the field alwaysvolatile of its main variable.
			// There can be a situation where within a function, I could be using
			// an external main variable up to a point where a main variable
			// with the same name as the external main variable is created locally;
			// and if a variable having its name suffixed with an offset was created
			// before the main variable having the same name as the external
			// main variable, the field alwaysvolatile of the variable having
			// its name suffixed with an offset will point instead to the uint used
			// with the external main variable. Hence it is necessary to everytime
			// set the field alwaysvolatile of the variable having
			// its name suffixed with an offset.
			larg->alwaysvolatile = fieldalwaysvolatile;
			
			// I check whether I am calling a function.
			if (*curpos == '(') {
				
				domethodcall = 0;
				
				// I make sure that larg is a valid pointer to function.
				if (larg->cast.ptr[stringmmsz(larg->cast)-1] != ')') throwerror("the type member is not a pointer to function");
				
				// I jump to this label if I am making
				// a method call and s contain the
				// name of the function to call.
				methodcall:
				
				// When doing a method call, the first argument
				// of the function to call is the object.
				// Remember that the method of an object is
				// a function for which the first argument
				// is of the same type as the object.
				if (domethodcall) {
					// I set curpos to its saved value
					// where the postfix operator start.
					// It will be used by pushargument() when setting
					// the argument lyricalargumentflag.id .
					swapvalues(&curpos, &savedcurpos);
					
					pushargument(larg);
					
					// I restore curpos.
					swapvalues(&curpos, &savedcurpos);
				}
				
				savedcurpos = curpos;
				
				++curpos; // Set curpos after '(' .
				skipspace();
				
				if (*curpos != ')') {
					// If there is no closing paranthesis
					// I process the arguments of the function call.
					
					evaluatefuncarg:;
					
					u8* savedcurpos2 = curpos;
					
					// evaluateexpression() will run until
					// an unknown symbol which should be
					// ',' or ')' is found.
					v = evaluateexpression(LOWESTPRECEDENCE);
					
					if (!v || v == EXPRWITHNORETVAL) {
						curpos = savedcurpos2;
						reverseskipspace();
						throwerror("no operand to pass as function argument");
					}
					
					// I set curpos to its saved value where
					// the expression for the argument start.
					// It will be used by pushargument() when setting
					// the argument lyricalargumentflag.id .
					swapvalues(&curpos, &savedcurpos2);
					
					// Push the variable resulting from the evaluation of the expression
					// as an argument. If the variable had its field bitselect set,
					// pushargument() will use it and set it back to null so that
					// it do not get to be used anywhere else until set again.
					pushargument(v);
					
					// I restore curpos.
					swapvalues(&curpos, &savedcurpos2);
					
					// Arguments to a function are seperated with a comma.
					if (*curpos == ',') {
						
						++curpos; // Set curpos after ',' .
						skipspace();
						
						if (*curpos == ')') throwerror("incorrect function calling");
						
						goto evaluatefuncarg;
						
					} else if (*curpos != ')') {
						
						reverseskipspace();
						throwerror("incorrect function calling");
					}
				}
				
				++curpos; // Set curpos after ')' .
				skipspace();
				
				// I set curpos to savedcurpos so that
				// if an error is thrown by callfunctionnow(),
				// curpos is correctly set to the location
				// of the error.
				swapvalues(&curpos, &savedcurpos);
				
				// I make the appropriate function call
				// which could be using a pointer to function
				// or a previously declared function.
				// callfunctionnow() never return a null pointer
				// and return EXPRWITHNORETVAL if there
				// was no resulting variable.
				if (domethodcall) {
					// The string in s contain the name of the function to call.
					larg = callfunctionnow((lyricalvariable*)0, s.ptr, CALLDEFAULT);
					
					mmrefdown(s.ptr);
					
				} else {
					// I get here if larg is a lyricalvariable pointer to function.
					// Note that they never have their field bitselect set.
					larg = callfunctionnow(larg, (u8*)0, CALLDEFAULT);
				}
				
				// Here I restore curpos.
				swapvalues(&curpos, &savedcurpos);
			}
			
		} else if (stringiseq3(op, "++") || stringiseq3(op, "--")) {
			// larg stay untouched; I setup
			// the function operator to be processed
			// later by processpostfixoperatornow().
			
			// larg temporary attributes need
			// to be preserved as it is not going
			// to be immediately processed by callfunctionnow()
			// after it has been pushed.
			larg->preservetempattr = 1;
			
			// I set curpos to its saved value
			// where the postfix operator start.
			// It will be used by pushargument() when setting
			// the argument lyricalargumentflag.id .
			swapvalues(&curpos, &savedcurpos);
			
			pushargument(larg);
			
			// I restore curpos.
			swapvalues(&curpos, &savedcurpos);
			
			processpostfixoperatorlater(op, savedcurpos);
		}
		
		// I jump to parsepostfixop to process
		// the next postfix operator.
		goto parsepostfixop;
	}
	
	parsenormalop:
	
	// Note that larg cannot be found null when I get here
	// because there is always a test that prevent it to be
	// null by the time I get here. Although it can be found
	// to be equal to EXPRWITHNORETVAL;
	
	// I look for a normalop.
	// As a side note, funcarg is
	// always null when I get here.
	
	savedcurpos = curpos;
	
	op = readoperator(normalop);
	
	// I try parsing a normal operator.
	if (!op || prevopprecedence < (opprecedence = *(op-1))) {
		// If I get here, I return larg, as there was
		// either not a normal operator parsed, or
		// the precedence of the operator parsed was
		// lower than the precedence of the operator
		// for which the argument was parsed in larg.
		// If the field bitselect of the variable pointed
		// by larg was set, it is left untouched so that
		// upper recursion of this function can use it.
		// If this function was not recursively called
		// by evaluateexpression(), that field will be
		// unset within parsestatement() upon returning,
		// to prevent it from being used until set again.
		
		curpos = savedcurpos;
		
		return larg;
	}
	
	// Before reading the expression on the right of
	// the normal operator, I need check that larg is valid
	// by making sure that it is not EXPRWITHNORETVAL.
	// Note that larg cannot be found null when I get here
	// because there is always a test that prevent
	// it to be null by the time I get here.
	if (larg == EXPRWITHNORETVAL) {
		
		curpos = savedcurpos;
		
		throwerror("void found on the left argument of the operator");
	}
	
	// Point to the right argument of the normal operator.
	lyricalvariable* rarg;
	
	// Value to use as evaluateexpression() argument.
	// It set the precedence to compare against
	// subsequent normal operator to parse.
	#define OPPRECEDENCE ((lyricalvariable*)opprecedence)
	
	// Before evaluating the right side of the normal operator,
	// I check whether the normal operator was either "&&" "||" "?:" .
	// Those are short-circuit operators and I do not push
	// the left and right argument using pushargument(),
	// and do not make use of callfunctionnow().
	// The processing of short-circuit operators
	// is done within the file included below.
	#include "shortcircuiteval.evaluateexpression.parsestatement.lyrical.c"
	
	// I get here if a short-circuit operator was not used.
	
	initializingvarwithassign:
	
	// I set curpos to its saved value
	// where the normal operator start.
	// It will be used by pushargument() when setting
	// the argument lyricalargumentflag.id .
	swapvalues(&curpos, &savedcurpos);
	
	// I push the left argument of the normal operator.
	// If the variable had its field bitselect set, pushargument()
	// will use it and set it back to null so that it do not
	// get to be used anywhere else until set again.
	pushargument(larg);
	
	// I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	u8* savedcurpos2 = curpos;
	
	// If I get here, I evaluate
	// the right side of the normal operator.
	// Any normal operator on the right side of
	// the normal operator, with a higher precedence,
	// will be processed as well in the recursion.
	rarg = evaluateexpression(OPPRECEDENCE);
	
	if (!rarg || rarg == EXPRWITHNORETVAL) {
		
		curpos = savedcurpos;
		
		throwerror("no operand for the operator right argument");
	}
	
	// I set curpos to its saved value where
	// the expression for the argument start.
	// It will be used by pushargument() when setting
	// the argument lyricalargumentflag.id .
	swapvalues(&curpos, &savedcurpos2);
	
	// I push the right argument argument of the normal operator.
	// If the variable had its field bitselect set,
	// pushargument() will use it and set it back to null so that
	// it do not get to be used anywhere else until set again.
	pushargument(rarg);
	
	// I restore curpos.
	swapvalues(&curpos, &savedcurpos2);
	
	// I set curpos to savedcurpos so that
	// if an error is thrown by callfunctionnow(),
	// curpos is correctly set to the location
	// of the error.
	swapvalues(&curpos, &savedcurpos);
	
	// Call the operator function.
	larg = callfunctionnow((lyricalvariable*)0, op, CALLDEFAULT);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// I jump to parsenormalop to process
	// the next normal operator if any.
	// If there is any normal operator,
	// that operator will be a short-circuit
	// operator, because every other normal operator
	// in the expression would have been
	// evaluated in the recursive call of
	// evaluateexpression() used to evaluate
	// the right side of the normal operator.
	goto parsenormalop;
}
