
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


// Function used to parse initialization.
// When this function is called,
// curpos is at '.', '=' or '{'
void readinit (lyricalvariable* vartoinit) {
	
	string labelafterinitializationcode;
	
	if (compilepass && vartoinit->isstatic) {
		// I get here if the variable
		// to initialize is a static variable.
		// A status variable is created
		// for the static variable within
		// the global variable region.
		// Instructions will be generated
		// to set it null at program start.
		// It will be set to 1 the first time
		// the code initializing the static variable
		// is run; preventing that same code from running again.
		// All the above work is done only in the secondpass
		// because labels, catchable-labels and instructions are
		// created only in the secondpass.
		
		// I first create the status variable for
		// the static variable to initialize.
		// The name of the status variable is the address
		// of the static variable converted to a string
		// and to which necessary prefixing will be
		// done by newvarglobal().
		
		// Instructions, labels and catchable-labels are
		// generated only in the secondpass.
		
		string s = stringduplicate1(largeenoughunsignednativetype(sizeofgpr));
		
		lyricalvariable* statusvar = newvarglobal(stringfmt("%d", (uint)vartoinit), s);
		
		// isstatic for the status variable 
		// need to be set to 1 so that
		// it do not get deleted by
		// scopeleaving() if created
		// within a scope.
		statusvar->isstatic = 1;
		
		// I save the offset of the newly
		// created status variable so as
		// to retrieve it later in order
		// to generate instructions that
		// will set it null at program start.
		statusvars.o = mmrealloc(statusvars.o, (statusvars.i+1)*sizeofgpr);
		statusvars.o[statusvars.i] = statusvar->offset;
		++statusvars.i;
		
		// Here the value of the status variable
		// is loaded into a register and instructions
		// will be generated to test whether it is non-null.
		lyricalreg* r = getregforvar(statusvar, 0, s, 0, FORINPUT);
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		r->lock = 1;
		
		// Here I create the name of the label that
		// will be created after the initialization code.
		labelafterinitializationcode = stringfmt("%d", newgenericlabelid());
		
		// I flush all registers without discarding them
		// since I am doing a conditional branching.
		flushanddiscardallreg(DONOTDISCARD);
		
		// The instruction jnz is generated to check whether
		// the register for the status variable is non-null.
		// The code initializing the static variable is skipped
		// if the value of the register is non-null.
		jnz(r, labelafterinitializationcode);
		
		// The register used for the status variable of
		// the static variable is set to 1 and set dirty.
		li(r, 1);
		r->dirty = 1;
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
	}
	
	// When this function is called,
	// curpos is at '.' or '='
	// or within the initialization
	// field of an array element
	// or single variable.
	void readinitexpr (lyricalvariable* vartoinit) {
		
		if (*curpos == '.' || *curpos == '=') {
			
			lyricalvariable* v = evaluateexpression(vartoinit);
			
			if (v && v != EXPRWITHNORETVAL && v != vartoinit) {
				// If I have a tempvar or a lyricalvariable
				// which depend on a tempvar (dereference variable
				// or variable with its name suffixed with an offset),
				// I should free all those variables because
				// I will no longer need them and it allow
				// the generated code to save stack memory.
				varfreetempvarrelated(v);
			}
			
		} else {
			
			u8* savedcurpos = curpos;
			
			skipspace();
			
			lyricalvariable* srcvar = evaluateexpression(LOWESTPRECEDENCE);
			
			if (!srcvar || srcvar == EXPRWITHNORETVAL) {
				
				curpos = savedcurpos;
				
				throwerror("invalid initialization expression");
			}
			
			string vartoinittype = vartoinit->cast;
			
			if (!vartoinittype.ptr) vartoinittype = vartoinit->type;
			
			string srcvartype = srcvar->cast;
			
			if (!srcvartype.ptr) srcvartype = srcvar->type;
			
			string callsignature = stringfmt("=|%s|%s|", vartoinittype, srcvartype);
			
			if (!pamsynmatch2(nativefcall[NATIVEFCALLASSIGN], callsignature.ptr, stringmmsz(callsignature)).start) {
				
				curpos = savedcurpos;
				
				throwerror(stringfmt("initialization type mismatch; expecting \"%s\", not \"%s\"", vartoinittype, srcvartype).ptr);
			}
			
			mmrefdown(callsignature.ptr);
			
			// I generate the instructions that assign
			// to the variable to initialize, the result
			// of the initialization expression.
			getvarduplicate(&vartoinit, srcvar);
			
			// If I have a tempvar or a lyricalvariable
			// which depend on a tempvar (dereference variable
			// or variable with its name suffixed with an offset),
			// I should free all those variables because
			// I will no longer need them and it allow
			// the generated code to save stack memory.
			varfreetempvarrelated(srcvar);
		}
		
		if (linkedlistofpostfixoperatorcall) {
			// This call is meant for processing
			// postfix operations if there was any.
			evaluateexpression(DOPOSTFIXOPERATIONS);
		}
	}
	
	if (*curpos != '{') {
		// Note that if the variable declared
		// was a byref variable, it is passed as-is,
		// to evaluateexpression() for initialization and
		// it is the intended behavior since a byref var
		// is meant to be initialized and not automatically
		// dereferenced at this point.
		
		readinitexpr(vartoinit);
		
	} else {
		
		void readinitblock (lyricalvariable* vartoinit) {
			// This function will generate a variable with
			// its name suffixed with an offset using
			// the variable pointed by vartoinit.
			lyricalvariable* getportionofvartoinit (uint offset) {
				
				lyricalvariable* v = vartoinit;
				
				// processvaroffsetifany() find the main variable
				// and set it in v; I use its returned value to update
				// offset which will give me the overall offset from
				// the main variable that I need to use with the name
				// of the variable to produce.
				offset += processvaroffsetifany(&v);
				
				// This variable will contain the uint pointer pointed
				// by the field alwaysvolatile of the main variable
				// of the variable having its name suffixed with
				// an offset that I am going to generate.
				uint* fieldalwaysvolatile = v->alwaysvolatile;
				
				// I duplicate the name of the main variable to initialize,
				// to which I suffix the offset starting with the dot.
				
				string name = stringfmt("%s.%d", v->name, offset);
				
				// There is no need to check whether
				// sizeoftype(v->type) > (v->size - offset)
				// or whether offset >= v->size; because
				// it will always be valid since I am initializing
				// the un-casted variable that I just declared.
				
				// I search for the variable name that I generated
				// in the variable name if it was already created
				// within the current scope.
				if (v = searchvar(name.ptr, stringmmsz(name), INCURRENTSCOPEONLY)) {
					// If I get here, then the variable had
					// previously been created.
					
					// The string pointed by s.ptr need to be deallocated
					// to avoid memory leak, since I am not creating a variable,
					// where it would have been used to set the name of the variable.
					mmrefdown(name.ptr);
					
					// Variable which have their name suffixed with
					// an offset only have a cast as oppose to a type.
					// I free it here in order to set a new cast later.
					if (v->cast.ptr) mmrefdown(v->cast.ptr);
					
				} else {
					// If I get here, then I need to create the variable.
					// Note that because the name of the variable is suffixed
					// with an offset, newvarlocal() should not use the argument
					// type, so I use a null string for the argument type.
					// The cast of the new variable is set later.
					v = newvarlocal(name, stringnull);
					
					// The field alwaysvolatile is a pointer to a uint shared
					// by a main variable and variables having their name suffixed
					// with an offset which refer to that main variable; so here
					// I point the field alwaysvolatile to the same uint that was
					// set in the field alwaysvolatile of its main variable.
					v->alwaysvolatile = fieldalwaysvolatile;
				}
				
				return v;
			}
			
			lyricaltype* t;
			
			++curpos; // Set curpos after '{' .
			skipspace();
			
			if (*curpos == '}') goto emptyinitblock;
			
			string type = vartoinit->cast;
			
			if (!type.ptr) type = vartoinit->type;
			
			u8* s = (type.ptr + stringmmsz(type)) -1;
			
			if (*s == ']') {
				// I get here if the variable to initialize is an array.
				
				// I first retrieve the number
				// of elements of the array.
				
				do --s; while (*s != '[');
				
				// Note that the number of elements
				// of an array is never zero.
				uint arraysize = stringconverttoint1(s+1);
				
				uint arrayelementtypestrsz = (uint)s - (uint)type.ptr;
				
				uint arraystride = sizeoftype(type.ptr, arrayelementtypestrsz);
				
				uint i = 0;
				
				uint readinitarrayelement (u8 c) {
					// I get the portion of the variable to initialize.
					// It will be a variable with its name suffixed with an offset.
					lyricalvariable* dstvar = getportionofvartoinit(i*arraystride);
					
					dstvar->cast = stringduplicate3(type.ptr, arrayelementtypestrsz);
					
					if (c == '{') readinitblock(dstvar);
					else readinitexpr(dstvar);
					
					c = *curpos;
					
					if (c == '}') return 0;
					else if (c != ',') {
						reverseskipspace();
						throwerror("expecting ',' or '}'");
					}
					
					return 1;
				}
				
				while (1) {
					
					if (*curpos == '}') break;
					
					if (*curpos == '{') {
						
						if (i == arraysize) {
							reverseskipspace();
							throwerror("out-of-bounds initialization");
						}
						
						if (!readinitarrayelement(*curpos)) break;
						
					} else if (*curpos != ',') {
						
						u8 c;
						
						if ((c = *curpos) == '[') {
							
							++curpos; // Set curpos after '[' .
							
							u8* savedcurpos = curpos;
							
							skipspace();
							
							// I evaluate the index expression,
							// which must result to a constant.
							lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
							
							if (linkedlistofpostfixoperatorcall) {
								curpos = savedcurpos;
								throwerror("expecting an expression with no postfix operations");
							}
							
							if (!v || v == EXPRWITHNORETVAL || !v->isnumber) {
								curpos = savedcurpos;
								throwerror("expecting an expression that result to a constant");
							}
							
							if (v->numbervalue > maxhostuintvalue) {
								curpos = savedcurpos;
								throwerror("index value beyond host capability");
							}
							
							uint ii = v->numbervalue;
							
							if (ii < i) {
								curpos = savedcurpos;
								throwerror("out-of-order initialization");
							}
							
							i = ii;
							
							if (i >= arraysize) {
								curpos = savedcurpos;
								throwerror("out-of-bounds initialization");
							}
							
							if (*curpos != ']') {
								reverseskipspace();
								throwerror("expecting ']'");
							}
							
							++curpos; // Set curpos after ']' .
							
							skipspace();
							
							c = *curpos;
							
							if (c != '.' && c != '=' && c != '{') {
								reverseskipspace();
								throwerror("expecting '.' or '=' or '{'");
							}
							
						} else if (i == arraysize) {
							reverseskipspace();
							throwerror("out-of-bounds initialization");
						}
						
						if (!readinitarrayelement(*curpos)) break;
						
					} else if (i == arraysize) {
						reverseskipspace();
						throwerror("out-of-bounds initialization");
					}
					
					++curpos; // Set curpos after ',' .
					
					skipspace();
					
					++i;
				}
				
			} else if ((t = searchtype(type.ptr, stringmmsz(type), INCURRENTANDPARENTFUNCTIONS)) && t->v) {
				// I get here if the variable to initialize
				// is a struct/pstruct/union.
				
				while (1) {
					
					if (*curpos == '}') break;
					else if (*curpos != '.') {
						reverseskipspace();
						throwerror("expecting '.' to select a member field to initialize");
					}
					
					++curpos; // Set curpos after '.' .
					
					u8* savedcurpos = curpos;
					
					skipspace();
					
					// I read the member field name.
					string s = readsymbol(LOWERCASESYMBOL);
					
					if (!s.ptr) {
						reverseskipspace();
						throwerror("expecting a member field name");
					}
					
					// I now try to obtain the member which name
					// is saved in s, from the type pointed by t.
					lyricalvariable* member = searchtypemember(t, s);
					
					if (!member) {
						curpos = savedcurpos;
						throwerror("unknown type member");
					}
					
					mmrefdown(s.ptr);
					
					// I get the portion of the variable to initialize.
					// It will be a variable with its name suffixed with an offset.
					lyricalvariable* dstvar = getportionofvartoinit(member->offset);
					
					// A struct/pstruct/union member is initialized using either
					// an initialization block or using a native assign operator
					// or using a method call.
					
					// Note that the lyricalvariable pointed by dstvar
					// has its name suffixed with an offset; and such
					// lyricalvariable do not have a type.
					dstvar->cast = stringduplicate1(member->type);
					
					if (*curpos == '{') readinitblock(dstvar);
					else if (*curpos == '=' || *curpos == '.') {
						// The bitselect will be unset by the call
						// to pushargument() within evaluateexpression().
						// The bitselect is not set when *curpos == '{'
						// because dstvar would be expected to be
						// of an array, struct/pstruct/union type.
						dstvar->bitselect = member->bitselect;
						
						readinitexpr(dstvar);
						
					} else {
						reverseskipspace();
						throwerror("expecting '=', '.' or '{'");
					}
					
					if (*curpos == '}') break;
					else if (*curpos != ',') {
						reverseskipspace();
						throwerror("expecting ',' or '}'");
					}
					
					++curpos; // Set curpos after ',' .
					skipspace();
				}
				
			} else {
				
				readinitexpr(vartoinit);
				
				if (*curpos != '}') {
					reverseskipspace();
					throwerror("expecting '}'");
				}
			}
			
			emptyinitblock:
			
			++curpos; // Set curpos after '}' .
			skipspace();
		}
		
		readinitblock(vartoinit);
	}
	
	if (compilepass && vartoinit->isstatic) {
		// I get here if I was initializing a static variable.
		// Instructions, labels and catchable-labels are generated
		// only in the secondpass.
		
		// I need to call flushanddiscardallreg() before
		// creating a new label since jump-instructions
		// will jump to the label and all registers need
		// to be un-used when I start parsing the code
		// coming after the label.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Here I create the label where to jump to
		// in order to skip the code initializing
		// the static variable.
		newlabel(labelafterinitializationcode);
	}
}
