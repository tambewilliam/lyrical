
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


// Enum used with the argument flag of funcdeclaration().
typedef enum {
	
	OPERATORDECLARATION, // Used when declaring or defining an operator.
	FUNCTIONDECLARATION, // Used when declaring or defining a named function.
	
} funcdeclarationflag;

// This function create a new lyricalfunction and
// recursively call parsestatement() to read the declaration
// of arguments of the function and its definition if any.
// It will also generate the necessary instructions to adjust
// the stack when entering the function and leaving the function.
// The argument type is the return type of the function that will be parsed.
// The argument name is the function name or operator symbol
// which was parsed before calling funcdeclaration().
// curpos has to be set at the opening paranthesis of
// the arguments when this function is called, and curpos
// will be set after ");" or after '}' depeding on whether
// I was just declaring or defining the function.
void funcdeclaration (string type, string name, funcdeclarationflag flag) {
	// This function allocate memory for a new lyricalfunction.
	// The newly created lyricalfunction is added to
	// a circular linkedlist formed using its fields
	// prev and next and it is added to the hierarchy
	// of created lyricalfunction.
	// Only the fields prev, next, parent,
	// sibling, scope and scopedepth of the newly
	// created lyricalfunction are set.
	lyricalfunction* newfunction () {
		// I allocate memory for a new lyricalfunction.
		lyricalfunction* f = mmallocz(sizeof(lyricalfunction));
		
		// I attach the newly created lyricalfunction
		// to the circular linkedlist formed using
		// the fields prev and next.
		f->next = rootfunc;
		f->prev = rootfunc->prev;
		rootfunc->prev->next = f;
		rootfunc->prev = f; // This should come last because I use rootfunc->prev above.
		
		// I attach the newly created lyricalfunction
		// to the hierarchy of created lyricalfunction.
		f->parent = currentfunc;
		f->sibling = lastchildofcurrentfunc;
		lastchildofcurrentfunc = f;
		
		scopesnapshotforfunc(f);
		
		return f;
	}
	
	lyricalfunction* f;
	
	// This function create the
	// pamsyntokenized to set in f->fcall .
	// Special care is taken when
	// the lyricalfunction pointed
	// by f is variadic.
	// The variable f is valid when
	// this function is called.
	pamsyntokenized createfcallmatchtokensignature () {
		
		string s1 = pamsynescape(name.ptr);
		
		// Note that in the pattern, the name of
		// the operator or function and its arguments are
		// separated by '|' instead of commas, opening and
		// closing paranthesis which would create conflict
		// since those same characters can be used in
		// the portion of the pattern used to match a type.
		// ei: pointer to function type.
		string s2 = stringfmt("<%s|", s1.ptr);
		
		mmrefdown(s1.ptr);
		
		if (f->varg) {
			// f->varg point to the last argument variable
			// of the lyricalfunction pointed by f.
			// f->varg->next will give me the address
			// of the first argument variable.
			lyricalvariable* v = f->varg->next;
			
			do {
				if (stringiseq2(v->type, "void*")) {
					// If the argument is "void*",
					// I append the pattern that will be
					// used to match any type of pointer.
					// Here below I made sure to escape
					// the escaping character '\' itself which
					// is used by C as well to do escaping.
					stringappend2(&s2, "[\\#a-z0-9]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*[0-9]\\]}[)\\*]");
					
				} else {
					// Appended to s2 after
					// escaping any metacharacters.
					string s3 = pamsynescape(v->type.ptr);
					stringappend1(&s2, s3);
					mmrefdown(s3.ptr);
				}
				
				// If the argument is a byref variable, to find the type
				// that the argument accept, I need to chop-off the last
				// two characters from the pattern in s2, which will effectively
				// remove an asterix and its escaping character since any '*'
				// is escaped in order for it not to be seen as metacharacter.
				// ei: void myfunc(uint& arg); The variable argument arg
				// will have the type "uint*" and accept arguments of type "uint".
				// The compiler when seeing a byref argument,
				// automatically find the address of the argument before
				// passing it to the function; and within the function,
				// the byref argument variable is automatically
				// dereferenced before getting used.
				// Note that I can never have a variable having
				// a type "void*" and be a byref variable because
				// a byref variable cannot be declared as void;
				// parsestatement() insure that it never happen.
				if (v->isbyref) stringchop(&s2, 2);
				
				v = v->next;
				
				// Because the linkedlist of argument
				// is a circular linkedlist,
				// if v == f->varg->next then I came back
				// to the first argument variable which was
				// already processed and I should stop
				// going through the argument variables.
				if (v == f->varg->next) {
					// Testing whether the function is variadic here
					// simply re-iterate the fact that I need at least
					// one argument before using "..." to specify a variadic
					// function. The requirement that I need to have
					// at least one variable argument declared before "..."
					// is implemented within parsestatement().
					if (f->isvariadic) {
						// If the lyricalfunction pointed by f
						// is a variadic function I append the
						// pattern that will be used to match
						// any number of arguments given to
						// the function.
						// Here below I made sure to escape
						// the escaping character '\' itself which
						// is used by C as well to do escaping.
						stringappend2(&s2, "+0-*{|[\\#a-z]+0-*{[\\,\\.()&\\*a-z0-9],\\[[1-9]+0-*{[0-9]}\\]}}");
					}
					
					break;
				}
				
				// If I get here I append the character '|' which
				// seperate the arguments variable of a function.
				stringappend4(&s2, '|');
				
			} while (1);
		}
		
		stringappend2(&s2, "|>");
		
		// Create the pamsyntokenized.
		pamsyntokenized fcall = pamsyntokenize(s2.ptr);
		
		// Free the string pointed by s2.ptr since I won't need it.
		mmrefdown(s2.ptr);
		
		return fcall;
	}
	
	// This function will create the string to set in f->typeofaddr;
	// The variable f is valid when this function is called.
	void createtypeofaddrstring () {
		
		f->typeofaddr = stringduplicate1(f->type);
		
		stringappend4(&f->typeofaddr, '(');
		
		if (f->varg) {
			// f->varg point to the last argument variable of the lyricalfunction pointed by f.
			// f->varg->next will give me the address of the first argument variable.
			lyricalvariable* v = f->varg->next;
			
			while (1) {
				
				stringappend1(&f->typeofaddr, v->type);
				
				// If the argument variable is byref, I append '&';
				if (v->isbyref) {
					// Before appending '&', I chop-off
					// an asterix from the type.
					stringchop(&f->typeofaddr, 1);
					
					stringappend4(&f->typeofaddr, '&');
				}
				
				v = v->next;
				
				// Because the linkedlist of
				// argument is a circular linkedlist,
				// if v == f->varg->next then I came back
				// to the first argument variable which was
				// already processed and I should stop
				// going through the argument variables.
				if (v == f->varg->next) {
					// If the function is variadic, I append ',' and "...";
					if (f->isvariadic) stringappend2(&f->typeofaddr, ",...");
					
					break;
				}
				
				// If I get here, I add comma before
				// processing the next variable argument.
				stringappend4(&f->typeofaddr, ',');
			}
		}
		
		stringappend4(&f->typeofaddr, ')');
	}
	
	// This function generate a call signature.
	// ei: "funcname|uint|void()|", "++|uint*|", "+|uint|";
	// The variable f is valid when this function is called.
	// Note that in the signature string, the name of
	// the operator or function and its arguments
	// are separated by '|' instead of commas,
	// opening and closing paranthesis which would
	// create conflict since those same characters
	// are used in a pointer to function type.
	string generatecallsignature () {
		
		string s = stringduplicate1(name);
		
		stringappend4(&s, '|');
		
		if (f->varg) {
			// f->varg point to the last argument variable
			// of the lyricalfunction pointed by f.
			// f->varg->next will give me the address
			// of the first argument variable.
			lyricalvariable* v = f->varg->next;
			
			do {
				stringappend1(&s, v->type);
				
				// If the argument variable is by reference,
				// to find the type that the argument accept,
				// I need to decrease the size of the string
				// which will effectively remove an asterix
				// from the type string in order to obtain
				// the type that is accepted by the function.
				// ei: void myfunc(uint& arg); The variable
				// argument arg will have the type "uint*"
				// and accept arguments of type "uint".
				// The compiler when seeing a byref argument,
				// automatically find the address of the argument
				// before passing it to the function.
				// And within the function, the argument
				// is automatically dereferenced before use.
				if (v->isbyref) stringchop(&s, 1);
				
				v = v->next;
				
				stringappend4(&s, '|');
				
			// Because the linkedlist of argument
			// is a circular linkedlist,
			// if v == f->varg->next then I came back
			// to the first argument variable which was
			// already processed and I should stop
			// going through the argument variables.
			} while (v != f->varg->next);
			
		} else stringappend4(&s, '|');
		
		return s;
	}
	
	// This function generate a linking signature string used to identify a function for the
	// purpose of linking. Its format is similar to the way a pointer to function type is specified,
	// with the difference that the name of the function is used instead of its return type. ei:
	// "functionname(uint&,uint(void*),...)" This is a linking signature string for a variadic function
	// 	called functionname; its first argument is a byref uint and its second argument is
	// 	a pointer to function which return a uint and take a void* as argument.
	// "+(uint&,uint(void*))" This is a linking signature string for the operator +.
	// 	its first argument is a byref uint and its second argument is a pointer to function which
	// 	return a uint and take a void* as argument.
	// This function is only used in the secondpass.
	// The variable f is valid when this function is called.
	void generatelinkingsignature () {
		
		string s = stringduplicate1(name);
		
		stringappend4(&s, '(');
		
		if (f->varg) {
			// f->varg point to the last argument variable of the lyricalfunction pointed by f.
			// f->varg->next will give me the address of the first argument variable.
			lyricalvariable* v = f->varg->next;
			
			while (1) {
				
				stringappend1(&s, v->type);
				
				// If the argument variable is by reference, to find the type that
				// the argument accept I need to decrease the size of the string which will
				// effectively remove an asterix from the type string in order to obtain the type that
				// is accepted by the function.
				// ei: void myfunc(uint& arg); The variable argument arg will have the type "uint*"
				// and accept arguments of type "uint". The compiler when seeing a byref argument,
				// automatically finds the address of the argument before passing it to the function.
				// And within the function, the argument is automatically dereferenced before use.
				if (v->isbyref) {
					stringchop(&s, 1);
					stringappend4(&s, '&');
				}
				
				v = v->next;
				
				// Because the linkedlist of argument is a circular linkedlist,
				// if v == f->varg->next then I came back to the first argument variable which
				// was already processed and I should stop going through the argument variables.
				if (v == f->varg->next) {
					// If the function is variadic, I append ",..."
					if (f->isvariadic) stringappend2(&s, ",...");
					
					break;
				}
				
				// If I get here, I add comma before processing the next variable argument.
				stringappend4(&s, ',');
			}
		}
		
		stringappend4(&s, ')');
		
		f->linkingsignature = s;
	}
	
	// I create a new lyricalfunction.
	// newfunction() will correctly set
	// the newly created lyricalfunction
	// as a child of the lyricalfunction
	// pointed by currentfunc.
	f = newfunction();
	
	f->name = name;
	f->type = type;
	
	++curpos; // Set curpos after '(' .
	skipspace();
	
	u8* savedcurpos = curpos;
	
	// I recursively call parsestatement()
	// to read the arguments of the newly
	// created lyricalfunction pointed by f.
	// If (*curpos == ')') then there is no arguments
	// to read and parsestatement() should not be
	// invoked otherwise it will return incorrect errors.
	// parsestatement() should be called only
	// if there is something to read.
	if (*curpos != ')') {
		// I save scopedepth and scopecount and set them
		// to null before recursively calling parsestatement()
		// to process the declaration of the arguments of the function.
		// Once I return, I restore those fields before
		// going any further; because they get used before
		// I recurse again to process the body of the function.
		uint savedscopecurrent = scopecurrent;
		uint savedscopedepth = scopedepth;
		uint* savedscope = scope;
		
		scopecurrent = 0; scopedepth = 0; scope = 0;
		
		lyricalfunction* savedcurrentfunc = currentfunc;
		currentfunc = f;
		
		lyricalfunction* savedlastchildofcurrentfunc = lastchildofcurrentfunc;
		lastchildofcurrentfunc = 0;
		
		parsestatement(PARSEFUNCTIONARG);
		
		lastchildofcurrentfunc = savedlastchildofcurrentfunc;
		
		currentfunc = savedcurrentfunc;
		
		// There is no need to check whether scope is non-null
		// to free it because scopeentering() will not be called
		// while parsing the declaration of the arguments of
		// the newly created lyricalfunction.
		
		scopecurrent = savedscopecurrent;
		scopedepth = savedscopedepth;
		scope = savedscope;
		
		if (!compilepass) {
			// It is only necessary to
			// get here in the firstpass.
			
			// f->varg point to the lyricalvariable
			// of the last argument that was created.
			lyricalvariable* v = f->varg;
			
			// The memory space used by all arguments
			// cannot be greater than MAXARGUSAGE.
			if ((v->offset + v->size) > MAXARGUSAGE) {
				curpos = startofdeclaration;
				throwerror("function arguments usage exceed limit");
			}
		}
		
	} else {
		
		++curpos; // Set curpos after ')'
		skipspace();
	}
	
	// generatecallsignature() will generate
	// a call signature that is used to determine
	// if there is any function which respond
	// to that function call within the current scope.
	// The argument of the functions declared below are similar;
	// they will not be allowed to be declared in
	// the same scope since they will respond to
	// the same call signature (It also apply to operators):
	// void f(mytype*& arg);
	// void f(mytype* argg);
	// u8 f(void* aargg);
	// uint f(mytype* aarggg, ...);
	string callsignature = generatecallsignature();
	
	// If I am declaring an operator,
	// I use the call signature to determine
	// if the declaration was correct
	// (Whether it was variadic; whether
	// it had the correct number of arguments).
	// These tests are necessary only in the firstpass.
	if (!compilepass && flag == OPERATORDECLARATION && (f->isvariadic || // The declaration of an operator cannot be a variadic function.
		!pamsynmatch2(iscorrectopdeclaration, callsignature.ptr, stringmmsz(callsignature)).start)) {
		curpos = startofdeclaration;
		throwerror("incorrect operator declaration");
	}
	
	// This function return 1 if no arguments
	// or if all arguments are native types
	// or pointer types, otherwise return 0.
	uint allnativeandpointertypeargs () {
		
		lyricalvariable* varg = f->varg;
		
		if (varg) {
			
			do {
				string s = varg->type;
				
				if (!pamsynmatch2(isnativeorpointertype, s.ptr, stringmmsz(s)).start)
					return 0;
				
			// Because the linkedlist of arguments is a circular linkedlist,
			// if ((varg = varg->next) == f->varg) then all elements
			// of the circular linkedlist have been visited.
			} while ((varg = varg->next) != f->varg);
		}
		
		return 1;
	}
	
	// I check whether a native operation is being overloaded.
	if (allnativeandpointertypeargs() && searchnativeop(callsignature)) {
		curpos = startofdeclaration;
		throwerror("native operations that exclusively use native types or pointer types as argument cannot be overloaded");
	}
	
	pamsyntokenized fcall = createfcallmatchtokensignature();
	
	lyricalfunction* searchedf = lastchildofcurrentfunc;
	
	// Search wether a similar function is
	// already defined within this function.
	while (searchedf) {
		
		if (searchedf->scopedepth <= scopecurrent &&
			(pamsynmatch2(searchedf->fcall, callsignature.ptr, stringmmsz(callsignature)).start ||
			pamsynmatch2(fcall, searchedf->callsignature.ptr, stringmmsz(searchedf->callsignature)).start) &&
			scopeiseq(searchedf->scope, scope, searchedf->scopedepth)) break;
		
		searchedf = searchedf->sibling;
	}
	
	if (searchedf) {
		// If (isexportinferred || declarationprefix.isexport)
		// is true, I can define a previously declared function
		// existing either in the current scope
		// or in parent scopes within currentfunc.
		// If (isexportinferred || declarationprefix.isexport)
		// is not true, I can only define a previously declared
		// function existing in the current scope.
		if (!(isexportinferred || declarationprefix.isexport) &&
			searchedf->scopedepth != scopecurrent)
			goto funcnotfound;
		
		if (!compilepass) {
			// The tests within this clause need
			// to be done only in the firstpass.
			
			// Instructions are generated only in the secondpass,
			// but in the firstpass, the field i of a lyricalfunction
			// is set to (lyricalinstruction*)-1 in order to determine
			// whether the function was defined.
			if (searchedf->i) {
				
				curpos = startofdeclaration;
				
				if (isexportinferred || declarationprefix.isexport)
					throwerror("function with similar call signature already defined within this function");
				else throwerror("function with similar call signature already defined within this scope");
			}
			
			// If I get here, I expect to see '{' to
			// define the previously declared function.
			if (*curpos != '{') {
				
				reverseskipspace();
				
				if (isexportinferred || declarationprefix.isexport)
					throwerror("expecting '{' in order to define function that was already declared within this function");
				else throwerror("expecting '{' in order to define function that was already declared within this scope");
			}
			
			// Now I need to make sure that I will be defining a function matching
			// exactly the function that I searched as having the same call signature.
			// The search that I previously did was just matching against the arguments.
			if (searchedf->isvariadic != f->isvariadic || // Their variadicity must be identical.
				!stringiseq1(searchedf->type, f->type)) {// They must have the same return type.
				
				curpos = startofdeclaration;
				
				if (isexportinferred || declarationprefix.isexport)
					throwerror("function signature do not match similar function previously declared within this function");
				else throwerror("function signature do not match similar function previously declared within this scope");
			}
		}
		
		// If I get here a function with a similar call signature
		// has already been declared but not defined.
		
		// There is no need to check if both f->varg and searchedf->varg are null or both are
		// non-null because if f->varg is null or non-null then searchedf->varg will be respectively
		// null or non-null; it is guarantied by previous signature call checks and variadicity
		// check. So I only need to do the check either on f->varg or on searchedf->varg;
		if (f->varg) {
			// Using a loop I check that the type
			// of the arguments are in fact identicals,
			// because the use of byref arguments make it
			// impossible for searchfunc() to determine
			// if the type of the arguments were
			// identical because searchfunc() simply find
			// a function which can accept a particular
			// function call without providing a way to check
			// if the variable arguments type were identical.
			// 	In addition I free the variable arguments
			// of the lyricalfunction pointed by f after checks,
			// since the lyricalfunction pointed by f will be discarded.
			// 	I discard the newly created function
			// and not the previously declared function because
			// if there is part of the code already parsed
			// using the function, they have been pointing
			// to the declaring function.
			
			lyricalvariable* vargofdeclaringfunc = searchedf->varg;
			lyricalvariable* vargofdefiningfunc = f->varg;
			
			do {
				if (!stringiseq1(vargofdeclaringfunc->type, vargofdefiningfunc->type)
					|| !stringiseq1(vargofdeclaringfunc->name, vargofdefiningfunc->name)) {
					
					curpos = startofdeclaration;
					
					if (isexportinferred || declarationprefix.isexport)
						throwerror("function signature do not match similar function previously declared within this function");
					else throwerror("function signature do not match similar function previously declared within this scope");
				}
				
				// Here I free the argument variable of
				// the function that I just created and which
				// is pointed by f. I could have made use of
				// varfree() to do the job, but it is faster and
				// easier to just do it manually. Furthermore varfree()
				// is only meant to free variables created in
				// the linkedlist pointed by the field vlocal
				// of a lyricalfunction.
				// At this point, only the fields name and type of
				// the lyricalvariable argument are set;
				// I make sure to free those data before freeing
				// the variable itself.
				// The field cast is surely not set, since
				// the variable was just recently created
				// by parsestatement() while reading arguments
				// and has not been used.
				
				mmrefdown(vargofdefiningfunc->name.ptr);
				mmrefdown(vargofdefiningfunc->type.ptr);
				
				// Here I save the value of vargofdefiningfunc->next
				// before freeing the memory block pointed by vargofdefiningfunc.
				lyricalvariable* v = vargofdefiningfunc->next;
				mmrefdown(vargofdefiningfunc);
				vargofdefiningfunc = v;
				
				vargofdeclaringfunc = vargofdeclaringfunc->next;
				
			// f->varg point to a circular linkedlist of variables.
			// So if vargofdefiningfunc == f->varg then I went through
			// all the argument variable of the function.
			} while (vargofdefiningfunc != f->varg);
		}
		
		// Discard the lyricalfunction pointed by f and set f with searchedf .
		// I discard the newly created function and not the previously
		// declared function because the fields firstpass and pushedargflags
		// of a lyricalfunction are set only when declaring a function;
		// furthermore if there is part of the code using the function,
		// they have been pointing to the declaration. Hence I simply need
		// to fill the field i of the previously declared function. Also,
		// it is much easier to discard the newly created defining function
		// than the previously declared function because the previously
		// declared function is likely not to be at the end of the sibling
		// linkedlist pointed by lastchildofcurrentfunc where it can be easily
		// removed and there must be instructions already referring to it.
		
		if (f->scope) mmrefdown(f->scope);
		// Every functions beside the root function,
		// has its fields name, type set;
		// so there is no need to check whether they are null.
		mmrefdown(f->name.ptr);
		mmrefdown(f->type.ptr);
		
		// I detach the lyricalfunction pointed by f from
		// the circular linkedlist of lyricalfunction.
		f->prev->next = f->next;
		f->next->prev = f->prev;
		
		// I restore lastchildofcurrentfunc.
		// Since the lyricalfunction pointed by f
		// was just created, the field sibling of
		// the lyricalfunction pointed by f still point
		// to the value of lastchildofcurrentfunc before
		// the lyricalfunction pointed by f was created.
		lastchildofcurrentfunc = f->sibling;
		
		mmrefdown(f);
		
		if (searchedf != lastchildofcurrentfunc) {
			// I move the declaring function
			// to the end of its sibling linkedlist
			// so that functions declared/defined
			// before its definition can be found
			// by searchfunc() and searchsymbol(),
			// as they only find previously
			// declared/defined functions.
			
			// Remove searchedf from its
			// sibling linkedlist.
			f = lastchildofcurrentfunc;
			while (f->sibling != searchedf) f = f->sibling;
			f->sibling = searchedf->sibling;
			
			// Add searchedf to the end of
			// its sibling linkedlist.
			searchedf->sibling = lastchildofcurrentfunc;
			lastchildofcurrentfunc = searchedf;
		}
		
		// Set f to the declaring function.
		f = searchedf;
		
		// It is not necessary to set the fields
		// f->callsignature, f->fcall, f->typeofaddr
		// since they were set when the declaring
		// function was created.
		
		mmrefdown(callsignature.ptr);
		
		pamsynfree(fcall);
		
	} else {
		// I get here if no function that respond
		// to the call signature was found within
		// the current scope.
		
		/*
		// ###: Error disabled, because, if the declaration within
		// the .lyh file was found to never be used, it would get
		// freed and cause this error to be incorrectly triggered.
		if ((isexportinferred || declarationprefix.isexport) && (currentfunc != rootfunc || scopecurrent)) {
			curpos = startofdeclaration;
			throwerror("missing declaration from corresponding .lyh file");
		}
		*/
		
		funcnotfound:
		
		if (compilepass) {
			// The routine within this clause is only executed in the secondpass.
			
			// This function is used to search
			// a function matching its field id.
			// It is only used in the secondpass.
			lyricalfunction* lookupfuncbyid (uint id) {
				
				lyricalfunction* f = firstpassrootfunc;
				
				do {
					if (f->id == id) return f;
					
					f = f->next;
					
				} while (f != firstpassrootfunc);
				
				// If I get here, I couldn't find anything.
				return 0;
			}
			
			// Here I search the firstpass equivalent
			// function using its id.
			// In the firstpass, the field id of a function
			// is set using startofdeclaration.
			// I use the value of startofdeclaration,
			// because function declaration is parsed
			// the same way in the first and secondpass,
			// hence the id value will be the same both
			// in the first and secondpass.
			// If null is returned, it mean that
			// the firstpass equivalent of the function
			// was freed because it was found to never be used.
			if (f->firstpass = lookupfuncbyid((uint)startofdeclaration)) {
				
				f->firstpass->secondpass = f;
				
				// I set the field linkingsignature
				// only in the secondpass.
				generatelinkingsignature();
			}
			
		} else {
			// I use the value of startofdeclaration and set it
			// in the field id of the newly created function.
			// Because function declaration is parsed
			// the same way in the first and second pass,
			// the id value will be the same both
			// in the first and second pass.
			f->id = (uint)startofdeclaration;
			
			// if (compileargcompileflag&LYRICALCOMPILENOSTACKFRAMESHARING)
			// is false I check whether the function is to be prevented from
			// getting its stackframe held by a stackframe holder, otherwise
			// I always prevent the function from getting its stackframe
			// held by a stackframe holder.
			
			if (compileargcompileflag&LYRICALCOMPILENOSTACKFRAMESHARING)
				f->couldnotgetastackframeholder = 1;
			else {
				uint i = arrayuintsz(couldnotgetastackframeholder);
				
				while (i) {
					
					--i;
					
					if (startofdeclaration == (u8*)couldnotgetastackframeholder.ptr[i]) {
						f->couldnotgetastackframeholder = 1;
						break;
					}
				}
			}
		}
		
		// Generate the string to set
		// in the field typeofaddr.
		createtypeofaddrstring();
		
		f->callsignature = callsignature;
		
		f->fcall = fcall;
	}
	
	// If the function was found
	// to never being used, I free
	// the ressources allocated for it.
	if (compilepass && !f->firstpass) {
		
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
		
		// I detach the lyricalfunction
		// from the circular linkedlist
		// of lyricalfunction.
		f->prev->next = f->next;
		f->next->prev = f->prev;
		
		// Remove the lyricalfunction
		// from the linkedlist pointed
		// by lastchildofcurrentfunc.
		if (f == lastchildofcurrentfunc) lastchildofcurrentfunc = f->sibling;
		else {
			lyricalfunction* ff = lastchildofcurrentfunc;
			
			while (ff->sibling != f) ff = ff->sibling;
			
			ff->sibling = f->sibling;
		}
		
		// Free the lyricalfunction.
		mmrefdown(f);
		
		// I set f to null in order
		// to later determine whether
		// to skip the function definition.
		f = 0;
		
	// If the function was being defined after a previous declaration,
	// its field startofdeclaration will correctly get updated here.
	} else f->startofdeclaration = startofdeclaration;
	
	if (*curpos != '{') {
		// If I get here, I am declaring the function.
		
		if (isexportinferred || declarationprefix.isexport) {
			
			curpos = startofdeclaration;
			
			if (currentfunc != rootfunc || scopecurrent)
				throwerror("expecting function/operator definition");
			else if (isexportinferred)
				throwerror(stringfmt("internal error: %s: invalid export inference", __FUNCTION__).ptr);
			else throwerror("keyword \"export\" can only be used with function/operator definition");
		}
		
		return;
	}
	
	// If I get here I am defining the function.
	
	// If the function was found
	// to never being used,
	// I skip its definition.
	if (!f) {
		// Skip the definition of the function.
		// It will set curpos after the closing
		// brace of the function definition
		// and do skipspace().
		skipbraceblock();
		
		return;
	}
	
	uint isfscopesaved = 0;
	uint savedfscopedepth;
	uint* savedfscope;
	
	if (isexportinferred || declarationprefix.isexport) {
		
		if (currentfunc == rootfunc && !scopecurrent) {
			
			if (compilepass) f->toexport = 1;
			else {
				if (isexportinferred) {
					curpos = startofdeclaration;
					throwerror(stringfmt("internal error: %s: invalid export inference", __FUNCTION__).ptr);
				} else if (declarationprefix.isexport && compileargcompileflag&LYRICALCOMPILENOFUNCTIONEXPORT) {
					curpos = startofdeclaration;
					throwerror("runtime exporting disabled");
				}
				
				// Functions to export must be treated as
				// function for which the address is obtained.
				f->itspointerisobtained = 1;
				
				++f->wasused;
			}
			
		} else {
			// I get here when
			// (isexportinferred || declarationprefix.isexport)
			// is true for a scope-nested function
			// in order to define a function declared
			// within currentfunc, in the current
			// or parent scope.
			// I save f->scopedepth and f->scope,
			// and I rescope the function using
			// the scope where it is being defined,
			// so that symbols declared/defined
			// before its definition can be found;
			// but after its definition, I restore it
			// so that it can be reached when searched
			// from outside the scope where it is being
			// defined, since its declaration is outside
			// the scope where it is being defined.
			
			savedfscopedepth = f->scopedepth;
			savedfscope = f->scope;
			
			isfscopesaved = 1;
			
			scopesnapshotforfunc(f);
		}
	}
	
	// Create the general purpose registers for
	// the function; gpr are made available only
	// in the secondpass because instructions
	// are generated only in the secondpass.
	// Each function has to have its own linkedlist
	// of GPRs because register allocation is a process
	// unique to each function; so here I create
	// a new linkedlist of GPRs for the processing
	// of the function that I am defining.
	if (compilepass) creategpr(f);
	
	// Instructions are generated only in the secondpass.
	if (compilepass && !f->firstpass->stackframeholder) {
		// I get here only if the function for which
		// the lyricalfunction is pointed by f do not
		// make use of a stackframe holder.
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			lyricalinstruction* i = newinstruction(f, LYRICALOPCOMMENT);
			i->comment = stringduplicate2("begin: entering function");
		}
		
		// I generate the instruction:
		// addi %0, %0, -sizeofgpr + -stackframepointerscachesize + -sharedregionsize + -vlocalmaxsize;
		
		lyricalinstruction* i = newinstruction(f, LYRICALOPADDI);
		i->r1 = 0;
		i->r2 = 0;
		
		lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		imm->n = -sizeofgpr;
		
		i->imm = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMNEGATIVESTACKFRAMEPOINTERSCACHESIZE;
		imm->f = f;
		
		i->imm->next = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMNEGATIVESHAREDREGIONSIZE;
		imm->f = f;
		
		i->imm->next->next = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMNEGATIVELOCALVARSSIZE;
		imm->f = f;
		
		i->imm->next->next->next = imm;
		
		// I generate the instructions that will write
		// the offset from the start of the stackframe
		// to the field holding the return address.
		// I can safely assume that register %1 is not
		// being used within the function to define.
		
		// I generate the instructions:
		// li %1, sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
		// str %1, %0;
		
		// Search the lyricalreg for %1.
		lyricalreg* r = searchreg(f, 1);
		
		// Lock the lyricalreg for %1,
		// to prevent newinstruction()
		// from adding it to the list
		// of unused registers of
		// the lyricalinstruction.
		r->lock = 1;
		
		i = newinstruction(f, LYRICALOPLI);
		i->r1 = 1;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMVALUE;
		imm->n = sizeofgpr;
		
		i->imm = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
		imm->f = f;
		
		i->imm->next = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMSHAREDREGIONSIZE;
		imm->f = f;
		
		i->imm->next->next = imm;
		
		imm = mmallocz(sizeof(lyricalimmval));
		imm->type = LYRICALIMMLOCALVARSSIZE;
		imm->f = f;
		
		i->imm->next->next->next = imm;
		
		// There is no need to set to 0
		// the fields waszeroextended and
		// wassignextended of register %1 since
		// it is temporarily being used.
		
		i = newinstruction(f, lyricalopstr());
		i->r1 = 1;
		i->r2 = 0;
		
		// Unlock the lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			lyricalinstruction* i = newinstruction(f, LYRICALOPCOMMENT);
			i->comment = stringduplicate2("end: done");
		}
	}
	
	++curpos; // Set curpos after '{' .
	skipspace();
	
	// I call parsestatement() to process
	// the definition of the function.
	// If (*curpos == '}') then there is no expression
	// or declaration to read, and parsestatement()
	// is not called; but the function will still
	// be correctly seen as having being defined
	// already (Because I used the opening and closing brace)
	// because the field i will not be null since instructions
	// adjusting the stack would have been generated.
	if (*curpos != '}') {
		
		uint savedvalueofalwaysvolatileforthisvar = *thisvar.alwaysvolatile;
		
		// Initialize the variable pointed by the field alwaysvolatile.
		// When compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE is true,
		// all variables are made volatile, hence
		// their value are never cached in registers.
		// This is useful when debugging because it is easier
		// for a debugger to just read memory in order
		// to retrieve the value of a variable instead of
		// having to write code in order to keep track of
		// register utilization per variable for each instruction.
		if (compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE) *thisvar.alwaysvolatile = 1;
		else *thisvar.alwaysvolatile = 0;
		
		// Since I am going to process another function,
		// I need to save the type and size of
		// the current retvar and restore it later.
		// The function that I am going to process
		// will set its own retvar type and size.
		string savedreturnvartype = returnvar.type;
		uint savedreturnvarsize = returnvar.size;
		returnvar.type = stringnull;
		
		uint savedvalueofalwaysvolatileforreturnvar = *returnvar.alwaysvolatile;
		
		// Here I initialize the variable pointed
		// by the field alwaysvolatile.
		// When compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE
		// is true, all variables are made volatile, hence
		// their value are never cached in registers.
		// This is useful when debugging because it is easier
		// for a debugger to just read memory in order
		// to retrieve the value of a variable instead of
		// having to write code in order to keep track of
		// register utilization per variable for each instruction.
		if (compileargcompileflag&LYRICALCOMPILEALLVARVOLATILE) *returnvar.alwaysvolatile = 1;
		else *returnvar.alwaysvolatile = 0;
		
		// I save scopedepth and scopecount
		// and set them to null before recursively
		// calling parsestatement() to process
		// the definition of the function.
		uint savedscopecurrent = scopecurrent;
		uint savedscopedepth = scopedepth;
		uint* savedscope = scope;
		scopecurrent = 0; scopedepth = 0; scope = 0;
		
		// It is necessary to save currentswitchblock,
		// labelnameforendofloop and labelnameforcontinuestatement
		// because I can be within a loop or switch block
		// and declare or define a function.
		
		switchblock* savedcurrentswitchblock = currentswitchblock;
		currentswitchblock = 0;
		
		// There is no need to save and restore
		// currenttype and containparsedstring because
		// I cannot be processing a type or processing
		// the parsing of a type string and
		// declare or define a function.
		
		lyricalfunction* savedcurrentfunc = currentfunc;
		currentfunc = f;
		
		lyricalfunction* savedlastchildofcurrentfunc = lastchildofcurrentfunc;
		lastchildofcurrentfunc = 0;
		
		lyricallabeledinstructiontoresolve* savedlinkedlistoflabeledinstructiontoresolve;
		
		string savedlabelnameforendofloop;
		string savedlabelnameforcontinuestatement;
		
		// Labels and gpr are created only in the secondpass.
		if (compilepass) {
			
			savedlinkedlistoflabeledinstructiontoresolve = linkedlistoflabeledinstructiontoresolve;
			linkedlistoflabeledinstructiontoresolve = 0;
			
			savedlabelnameforendofloop = labelnameforendofloop;
			labelnameforendofloop = stringnull;
			
			savedlabelnameforcontinuestatement = labelnameforcontinuestatement;
			labelnameforcontinuestatement = stringnull;
			
			// If the function for which I am about to parse
			// the body make use of a stackframe holder, I set
			// the field dirty and returnaddr of the lyricalreg
			// for the register %1 since it contain
			// the return address.
			if (currentfunc->firstpass->stackframeholder) {
				
				lyricalreg* r = searchreg(currentfunc, 1);
				
				r->dirty = 1;
				r->returnaddr = 1;
				r->size = sizeofgpr; // Set only so that flushreg() can pick the correct store instruction.
				
				setregtothebottom(r);
			}
		}
		
		parsestatement(PARSEFUNCTIONBODY);
		
		// Labels and gpr are created only in the secondpass.
		if (compilepass) {
			// If the function for which I am done parsing
			// the body make use of a stackframe holder, I call
			// loadreturnaddr() to load in the register %1
			// the return address.
			if (currentfunc->firstpass->stackframeholder) loadreturnaddr();
			
			labelnameforcontinuestatement = savedlabelnameforcontinuestatement;
			
			labelnameforendofloop = savedlabelnameforendofloop;
			
			linkedlistoflabeledinstructiontoresolve = savedlinkedlistoflabeledinstructiontoresolve;
		}
		
		lastchildofcurrentfunc = savedlastchildofcurrentfunc;
		
		currentfunc = savedcurrentfunc;
		
		currentswitchblock = savedcurrentswitchblock;
		
		if (scope) mmrefdown(scope);
		
		scopecurrent = savedscopecurrent;
		scopedepth = savedscopedepth;
		scope = savedscope;
		
		*returnvar.alwaysvolatile = savedvalueofalwaysvolatileforreturnvar;
		
		// I restore the field type and size of returnvar.
		if (returnvar.type.ptr) mmrefdown(returnvar.type.ptr);
		returnvar.type = savedreturnvartype;
		returnvar.size = savedreturnvarsize;
		
		// If there was a string allocated in the field cast, I free it.
		if (returnvar.cast.ptr) {
			mmrefdown(returnvar.cast.ptr);
			returnvar.cast = stringnull;
		}
		
		*thisvar.alwaysvolatile = savedvalueofalwaysvolatileforthisvar;
		
	} else {
		
		++curpos; // Set curpos after '}' .
		skipspace();
	}
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			lyricalinstruction* i = newinstruction(f, LYRICALOPCOMMENT);
			i->comment = stringduplicate2("begin: leaving function");
		}
		
		if (f->firstpass->stackframeholder) {
			// I get here if the function for which
			// the lyricalfunction is pointed by f
			// make use of a stackframe holder.
			// The return address is in register %1.
			
			// Search the lyricalreg for %1.
			lyricalreg* r = searchreg(f, 1);
			
			// Lock the lyricalreg for %1,
			// to prevent newinstruction()
			// from adding it to the list
			// of unused registers of
			// the lyricalinstruction.
			r->lock = 1;
			
			lyricalinstruction* i = newinstruction(f, LYRICALOPJR);
			i->r1 = 1;
			
			// Unlock the lyricalreg.
			// Locked registers must be unlocked only after
			// the instructions using them have been generated;
			// otherwise they could be lost when insureenoughunusedregisters()
			// is called while creating a new lyricalinstruction.
			r->lock = 0;
			
		} else {
			// I get here if the function for which
			// the lyricalfunction is pointed by f
			// do not make use of a stackframe holder.
			
			// I create the instruction:
			// addi %0, %0, sizeofgpr + stackframepointerscachesize + sharedregionsize + vlocalmaxsize;
			// It will set the stackpointer register to the location
			// within the stackframe where the return address is located.
			
			lyricalinstruction* i = newinstruction(f, LYRICALOPADDI);
			i->r1 = 0;
			i->r2 = 0;
			
			lyricalimmval* imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMVALUE;
			imm->n = sizeofgpr;
			
			i->imm = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSTACKFRAMEPOINTERSCACHESIZE;
			imm->f = f;
			
			i->imm->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMSHAREDREGIONSIZE;
			imm->f = f;
			
			i->imm->next->next = imm;
			
			imm = mmallocz(sizeof(lyricalimmval));
			imm->type = LYRICALIMMLOCALVARSSIZE;
			imm->f = f;
			
			i->imm->next->next->next = imm;
			
			// I generate the instruction jpop.
			i = newinstruction(f, LYRICALOPJPOP);
		}
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			lyricalinstruction* i = newinstruction(f, LYRICALOPCOMMENT);
			i->comment = stringduplicate2("end: done");
		}
		
	// In the firstpass, the field i
	// of the lyricalfunction need to be
	// set so as to know when it has already
	// been defined.
	} else f->i = (lyricalinstruction*)-1;
	
	// If f->scopedepth and f->scope
	// were saved, I restore them.
	if (isfscopesaved) {
		
		f->scopedepth = savedfscopedepth;
		
		if (f->scope) mmrefdown(f->scope);
		
		f->scope = savedfscope;
	}
	
	// reverseskipspace() is called at the end
	// of the function definition parsing in order
	// to adjust curpos so to log correct location.
	skipspace();
	
	// curpos is positioned after the closing brace.
	
	return;
}
