
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


// The routines within this file will parse
// an assembly instruction and expect each
// assembly instructions to be terminated with
// a semi-colon. skipspace() is called after
// the parsing of the semi-colon before returning.
// The routines within this file will also
// parse the creation of labels and machine
// code within double quotes.

string s;

if (*curpos == '"') {
	// If I get here, I have a machine code.
	// I do not use flushanddiscardallreg()
	// before a machine code which are written
	// between double-quotes.
	// It will be the responsability
	// of the programmer to make sure
	// that any registers, that was used
	// and not reserved, be restored.
	
	// Instructions are generated
	// only in the secondpass.
	if (compilepass) {
		
		if ((s = readstringconstant(NORMALSTRINGREADING)).ptr) {
			
			lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPMACHINECODE);
			
			i->opmachinecode = s;
		}
		
	} else skipstringconstant(SKIPSPACEAFTERSTRING);
	
	return 0;
}

u8* savedcurpos = curpos;

s = readsymbol(LOWERCASESYMBOL);

if (!s.ptr) {
	curpos = savedcurpos;
	throwerror("expecting a label, an assembly instruction or a machine code string");
}

if (*curpos == ':') {
	
	if (compilepass) {
		// I need to call flushanddiscardallreg()
		// before creating a new label since execution
		// will reach the label from different
		// locations, making it impossible to keep
		// track of which register was allocated;
		// hence all registers need to be un-used
		// when I start parsing the code coming
		// after the label.
		// flushanddiscardallreg() is called only
		// in the secondpass because instructions
		// are not generated in the firstpass.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Since newlabel() do not use curpos,
		// I set curpos to savedcurpos so that
		// if an error is thrown by newlabel(),
		// curpos is correctly set to the location
		// of the error.
		swapvalues(&curpos, &savedcurpos);
		
		// newlabel() is called only in
		// the secondpass because lyricallabel
		// are created only in the secondpass.
		newlabel(s);
		
		// Here I restore curpos.
		swapvalues(&curpos, &savedcurpos);
		
	} else mmrefdown(s.ptr);
	
	++curpos; // Set curpos after ':' .
	skipspace();
	
	return 0;
}

// Here I declare an array of 4 structs
// which are used with the assembly instructions.
struct {
	lyricalreg* reg;
	lyricalargument* arg;
	
} operand[3];

// Enum used with readoperands.
typedef enum {
	COMMAENDING,
	SEMICOLONENDING
	
} readoperandsflag;

// This function is used to read the comma separated
// operands of an assembly instruction and set
// the appropriate element of the array "operand"
// depending on the number of operand to read.
// The argument n determine the number
// of operand to read and cannot be null.
// The argument readoperandsflag determine
// whether a comma or semi-colon should
// be used for ending an expression.
void readoperands (uint n, readoperandsflag flag) {
	// This variable will be used to index the array operand;
	uint i = 0;
	
	evaluatearg:;
	
	u8* savedcurpos = curpos;
	
	if (*curpos == '%') {
		// If I get here, I parse a register id.
		
		++curpos; // Set curpos right after '%'.
		
		readnumberresult numberparsed = readnumber();
		
		if (!numberparsed.wasread || numberparsed.n > nbrofgpr) {
			curpos = savedcurpos;
			throwerror("expecting a valid register id after '%'");
		}
		
		// Register %0 can be used,
		// and cannot and do not
		// need to be reserved.
		if (numberparsed.n == 0) {
			
			operand[i].reg = &rstack;
			operand[i].arg = 0;
			
		} else if (compilepass) {
			// lyricalreg are usable only in the secondpass.
			lyricalreg* r = currentfunc->gpr;
			
			while (1) {
				
				if (r->id == numberparsed.n) {
					
					if (r->reserved) break;
					
					// I reserve the register before the call to
					// insurethereisenoughcriticalreg(), because if
					// insurethereisenoughcriticalreg() is called,
					// it could make use of flushreg() which could make
					// use of allocreg(); and if the register is not locked
					// or reserved, allocreg() could use it while trying
					// to allocate a new register.
					r->reserved = 1;
					
					// If I am reserving a critical register,
					// I need to assess whether I have enough
					// critical registers left.
					// The file regmanipulations should be read
					// to understand the meaning of a critical register.
					if (isregcritical(r)) insurethereisenoughcriticalreg();
					
					// If the register was assigned to a variable,
					// I flush it (if it was dirty) and discard it;
					// If the register was used to hold the return address or
					// the stackframe address of a function or the address of
					// the global variable region or the address of the string
					// region or the address of retvar, I set the field returnaddr
					// or funclevel or globalregionaddr or stringregionaddr or
					// retvaraddr to zero accordingly. Remember that only one
					// of the fields returnaddr, funclevel, globalregionaddr, stringregionaddr,
					// thisaddr, retvaraddr and v can be non-null at a time.
					if (r->returnaddr) {
						// If the register holding the return address
						// was not previously saved, I flush it.
						if (r->dirty) flushreg(r);
						
						r->returnaddr = 0;
						
					} else if (r->funclevel) {
						// If the register for the stackframe pointer
						// has not yet been written to its cache, I flush it.
						if (r->dirty) flushreg(r);
						
						r->funclevel = 0;
						
					} else if (r->globalregionaddr) r->globalregionaddr = 0;
					else if (r->stringregionaddr) r->stringregionaddr = 0;
					else if (r->thisaddr) r->thisaddr = 0;
					else if (r->retvaraddr) r->retvaraddr = 0;
					else if (r->v) {
						
						if (r->dirty) flushreg(r);
						
						r->v = 0;
						
					} else break;
					
					if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
						comment(stringfmt("reg %%%d discarded", r->id));
					}
					
					break;
				}
				
				r = r->next;
				
				if (r == currentfunc->gpr) {
					curpos = savedcurpos;
					throwerror("invalid register");
				}
			}
			
			operand[i].reg = r;
			operand[i].arg = 0;
			
		} else {
			
			operand[i].reg = 0;
			operand[i].arg = 0;
		}
		
	} else {
		// If I get here, I evaluate an expression.
		// There is no need to save and restore
		// the value of linkedlistofpostfixoperatorcall
		// because when this function is called, I am not
		// within the evaluation of an expression.
		
		lyricalvariable* v = evaluateexpression(LOWESTPRECEDENCE);
		
		if (!v || v == EXPRWITHNORETVAL) {
			curpos = savedcurpos;
			reverseskipspace();
			throwerror("expecting a valid expression");
		}
		
		// I set curpos to its saved value where
		// the expression for the argument start.
		// It will be used by pushargument() when setting
		// the argument lyricalargumentflag.id .
		swapvalues(&curpos, &savedcurpos);
		
		// Push the variable resulting from the evaluation
		// of the expression. If the variable had its field bitselect set,
		// pushargument() will use it and set it back to null
		// so that it do not get to be used until set again.
		pushargument(v);
		
		// I restore curpos.
		swapvalues(&curpos, &savedcurpos);
		
		// Note that the last pushed argument
		// is pointed by funcarg.
		
		// Only variables with a pointer type or
		// native type can be used with assembly instructions,
		// because their size can fit in a register.
		if (!pamsynmatch2(isnativeorpointertype, funcarg->typepushed.ptr, stringmmsz(funcarg->typepushed)).start) {
			curpos = savedcurpos;
			throwerror("an argument to an assembly instruction can only be a pointer, a variable with a native type or a reserved register");
		}
		
		operand[i].arg = funcarg;
		operand[i].reg = 0;
	}
	
	// Arguments to an assembly instruction
	// are seperated with a comma.
	if (--n) {
		
		++i;
		
		if (*curpos != ',') {
			reverseskipspace();
			throwerror("expecting ','");
		}
		
		++curpos; // Set curpos after ',' .
		skipspace();
		
		goto evaluatearg;
	}
	
	if (flag == COMMAENDING) {
		
		if (*curpos != ',') {
			
			reverseskipspace();
			throwerror("expecting ','");
		}
		
		++curpos; // Set curpos after ',' .
		skipspace();
	}
	
	if (linkedlistofpostfixoperatorcall) {
		// This call is meant for processing
		// postfix operations if there was any.
		evaluateexpression(DOPOSTFIXOPERATIONS);
	}
}

// Process opcodes for branching
// where the only argument is: label.
void opcodejlabel (lyricalinstruction*(* opcode)(string)) {
	
	u8* savedcurpos = curpos;
	
	// I read the label name to which
	// I am supposed to jump to.
	string label = readsymbol(LOWERCASESYMBOL);
	
	if (!label.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	// Instructions are not generated in the firstpass.
	if (!compilepass) {
		mmrefdown(label.ptr);
		return;
	}
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	// I set curpos to savedcurpos so that
	// resolvelabellater(), used by opcode(),
	// correctly set the field pos of the newly
	// created lyricallabeledinstructiontoresolve.
	swapvalues(&curpos, &savedcurpos);
	
	opcode(label);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The string label is duplicated by j();
	// so here I free the string in label
	// since I will no longer need it.
	mmrefdown(label.ptr);
}

// Process opcodes for branching
// where the only argument is: immediate.
void opcodejimm (lyricalinstruction*(* opcode)(u64)) {
	
	readoperands(1, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		if (!operand[0].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	opcode(ifnativetypedosignorzeroextend(operand[0].arg->v->numbervalue, operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)));
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes for branching
// where the only argument is: input.
void opcodejin (lyricalinstruction*(* opcode)(lyricalreg*)) {
	
	readoperands(1, SEMICOLONENDING);
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the only operand
	// which is an input.
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	opcode(operand[0].reg);
	
	// I unlock the register that was allocated
	// and locked for the input operand.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: input-output, immediate.
void opcodeinoutimm (lyricalinstruction*(* opcode)(lyricalreg*, u64)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[1].arg) {
		
		if (!operand[1].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input-output operand.
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar4 () {
				// I search among registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar4() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	opcode(operand[0].reg, ifnativetypedosignorzeroextend(operand[1].arg->v->numbervalue, operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)));
	
	// If the input-output operand was
	// a variable(meaning its field arg was set)
	// and was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the register that was allocated
	// and locked for the input-output operand.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes where the arguments
// in order are: input, immediate.
void opcodeinimm (lyricalinstruction*(* opcode)(lyricalreg*, u64)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[1].arg) {
		
		if (!operand[1].arg->v->isnumber) {
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand.
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
	}
	
	opcode(operand[0].reg, ifnativetypedosignorzeroextend(operand[1].arg->v->numbervalue, operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)));
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes where the arguments
// in order are: output, immediate.
void opcodeoutimm (lyricalinstruction*(* opcode)(lyricalreg*, u64)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (operand[1].arg) {
		
		if (!operand[1].arg->v->isnumber) {
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the output operand.
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, ifnativetypedosignorzeroextend(operand[1].arg->v->numbervalue, operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)));
	
	// If the output operand was a variable
	// (meaning its field arg was set) and
	// was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes where the arguments
// in order are: input, input.
void opcodeinin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operands.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: output, input.
void opcodeoutin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I process the output operand.
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg);
	
	// If the output operand was a variable
	// (meaning its field arg was set) and
	// was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: input-output, input.
void opcodeinoutin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I process the input-output operand.
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar4 () {
				// I first check whether the tempvar
				// is shared with the second argument.
				// Note that if operand[1].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange().
				if (operand[1].arg && operand[1].arg->v == operand[0].arg->v)
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar4() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg);
	
	// If the input-output operand was
	// a variable(meaning its field arg was set)
	// and was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, label.
void opcodejcondinlabel (lyricalinstruction*(* opcode)(lyricalreg*, string)) {
	
	readoperands(1, COMMAENDING);
	
	u8* savedcurpos = curpos;
	
	// I read the label name to which
	// I am supposed to jump to.
	string label = readsymbol(LOWERCASESYMBOL);
	
	if (!label.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		mmrefdown(label.ptr);
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand.
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	// I set curpos to savedcurpos so that
	// resolvelabellater(), used by opcode(),
	// correctly set the field pos of the newly
	// created lyricallabeledinstructiontoresolve.
	swapvalues(&curpos, &savedcurpos);
	
	opcode(operand[0].reg, label);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The string label is duplicated by opcode()
	// So here I free the string in label
	// since I will no longer need it.
	mmrefdown(label.ptr);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, immediate.
void opcodejcondinimm (lyricalinstruction*(* opcode)(lyricalreg*, u64)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[1].arg) {
		
		if (!operand[1].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand.
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	opcode(operand[0].reg, ifnativetypedosignorzeroextend(operand[1].arg->v->numbervalue, operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)));
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, input.
void opcodejcondinin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*)) {
	
	readoperands(2, SEMICOLONENDING);
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operands.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	opcode(operand[0].reg, operand[1].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: output, input, input.
void opcodeoutinin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, lyricalreg*)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[2].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
	}
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I process the output operand.
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, operand[2].reg);
	
	// If the output operand was a variable
	// (meaning its field arg was set) and
	// was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	operand[2].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: output, input, immediate.
void opcodeoutinimm (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, u64)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I process the output operand.
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// If the output operand was a variable
	// (meaning its field arg was set) and
	// was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes where the arguments
// in order are: input, input, immediate.
void opcodeininimm (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, u64)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operands.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes where the arguments
// in order are: input-output, input, immediate.
void opcodeinoutinimm (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, u64)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I process the input-output operand.
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar4 () {
				// I first check whether the tempvar
				// is shared with the second argument.
				// Note that if operand[1].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange().
				if (operand[1].arg && operand[1].arg->v == operand[0].arg->v)
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar4() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// If the input-output operand was
	// a variable(meaning its field arg was set)
	// and was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, input, label.
void opcodejcondininlabel (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, string)) {
	
	readoperands(2, COMMAENDING);
	
	u8* savedcurpos = curpos;
	
	// I read the label name to which
	// I am supposed to jump to.
	string label = readsymbol(LOWERCASESYMBOL);
	
	if (!label.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		mmrefdown(label.ptr);
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	// I set curpos to savedcurpos so that
	// resolvelabellater(), used by opcode(),
	// correctly set the field pos of the newly
	// created lyricallabeledinstructiontoresolve.
	swapvalues(&curpos, &savedcurpos);
	
	opcode(operand[0].reg, operand[1].reg, label);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The string label is duplicated by opcode()
	// So here I free the string in label
	// since I will no longer need it.
	mmrefdown(label.ptr);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, input, immediate.
void opcodejcondininimm (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, u64)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	opcode(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}

// Process opcodes for conditional
// branching where the arguments
// in order are: input, input, input.
void opcodejcondininin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, lyricalreg*)) {
	
	readoperands(3, SEMICOLONENDING);
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operands.
	
	if (operand[2].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
	}
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I flush all registers without discarding them
	// since I am doing a conditional branching.
	flushanddiscardallreg(DONOTDISCARD);
	
	opcode(operand[0].reg, operand[1].reg, operand[2].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	operand[2].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

#if 0
// ###: The following two functions
// are no longer needed, but they are
// kept for documentation reasons.
// They were used when I considered
// implementing assembly instructions
// that returned two values.

// Process opcodes where the arguments
// in order are: output, output, input, input.
void opcodeoutoutinin (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, lyricalreg*, lyricalreg*)) {
	
	readoperands(4, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the output operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (operand[1].arg) {
		
		lyricalvariable* v = operand[1].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the output operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is
		// no need to duplicate its value, because
		// it is not used by the instruction;
		// the instruction only set its value; so
		// in the firstpass, I notify the secondpass,
		// that the output operand should never
		// be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[1].arg->flag->istobeoutput = 1;
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[3].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[3].reg = getregforvar(operand[3].arg->v, 0, operand[3].arg->typepushed, operand[3].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[3].reg->lock = 1;
	}
	
	if (operand[2].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
	}
	
	// I process the output operands.
	
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	if (operand[1].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[1].reg = getregforvar(operand[1].arg->varpushed, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FOROUTPUT);
		
		// If while obtaining the operand register
		// operand[1].reg, I find that the operand
		// register operand[0].reg has been discarded,
		// then it mean that both output operand were
		// overlapping each other, and I should
		// throw an error.
		if (operand[0].arg && !operand[0].reg->v) {
			curpos = savedcurpos;
			throwerror("the output operands cannot overlap in memory");
		}
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, operand[2].reg, operand[3].reg);
	
	// If the output operands are volatile, I flush them.
	if (operand[1].arg && *operand[1].arg->varpushed->alwaysvolatile) flushreg(operand[1].reg);
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	operand[2].reg->lock = 0;
	operand[3].reg->lock = 0;
	
	if (funcarg) freefuncarg();
}

// Process opcodes where the arguments
// in order are: output, output, input, immediate.
void opcodeoutoutinimm (lyricalinstruction*(* opcode)(lyricalreg*, lyricalreg*, lyricalreg*, u64)) {
	
	readoperands(4, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the output operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	if (operand[1].arg) {
		
		lyricalvariable* v = operand[1].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the output operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is
		// no need to duplicate its value, because
		// it is not used by the instruction;
		// the instruction only set its value; so
		// in the firstpass, I notify the secondpass,
		// that the output operand should never
		// be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[1].arg->flag->istobeoutput = 1;
	}
	
	if (operand[3].arg) {
		
		if (!operand[3].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return;
	}
	
	// I process the input operand first because
	// for the output operand, I will discard any
	// overlapping register in order to follow
	// the rule about the loading of registers; so
	// I make use of any loaded register before
	// they get discarded if they were overlapping
	// the output operand.
	
	if (operand[2].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
	}
	
	// I process the output operands.
	
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	if (operand[1].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[1].reg = getregforvar(operand[1].arg->varpushed, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FOROUTPUT);
		
		// If while obtaining the operand register
		// operand[1].reg, I find that the operand
		// register operand[0].reg has been discarded,
		// then it mean that both output operand were
		// overlapping each other, and I should
		// throw an error.
		if (operand[0].arg && !operand[0].reg->v) {
			curpos = savedcurpos;
			throwerror("the output operands cannot overlap in memory");
		}
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	opcode(operand[0].reg, operand[1].reg, operand[2].reg, ifnativetypedosignorzeroextend(operand[3].arg->v->numbervalue, operand[3].arg->typepushed.ptr, stringmmsz(operand[3].arg->typepushed)));
	
	// If the output operands are volatile, I flush them.
	if (operand[1].arg && *operand[1].arg->varpushed->alwaysvolatile) flushreg(operand[1].reg);
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	operand[2].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
}
#endif


if (stringiseq2(s, "add")) {
	mmrefdown(s.ptr);
	opcodeoutinin(add);
	return 0;
}

if (stringiseq2(s, "addi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(addi);
	return 0;
}

if (stringiseq2(s, "sub")) {
	mmrefdown(s.ptr);
	opcodeoutinin(sub);
	return 0;
}

if (stringiseq2(s, "neg")) {
	mmrefdown(s.ptr);
	opcodeoutin(neg);
	return 0;
}

if (stringiseq2(s, "mul")) {
	mmrefdown(s.ptr);
	opcodeoutinin(mul);
	return 0;
}

if (stringiseq2(s, "mulh")) {
	mmrefdown(s.ptr);
	opcodeoutinin(mulh);
	return 0;
}

if (stringiseq2(s, "div")) {
	mmrefdown(s.ptr);
	opcodeoutinin(div);
	return 0;
}

if (stringiseq2(s, "mod")) {
	mmrefdown(s.ptr);
	opcodeoutinin(mod);
	return 0;
}

if (stringiseq2(s, "mulhu")) {
	mmrefdown(s.ptr);
	opcodeoutinin(mulhu);
	return 0;
}

if (stringiseq2(s, "divu")) {
	mmrefdown(s.ptr);
	opcodeoutinin(divu);
	return 0;
}

if (stringiseq2(s, "modu")) {
	mmrefdown(s.ptr);
	opcodeoutinin(modu);
	return 0;
}

if (stringiseq2(s, "muli")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(muli);
	return 0;
}

if (stringiseq2(s, "mulhi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(mulhi);
	return 0;
}

if (stringiseq2(s, "divi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(divi);
	return 0;
}

if (stringiseq2(s, "modi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(modi);
	return 0;
}

if (stringiseq2(s, "divi2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(divi2);
	return 0;
}

if (stringiseq2(s, "modi2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(modi2);
	return 0;
}

if (stringiseq2(s, "mulhui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(mulhui);
	return 0;
}

if (stringiseq2(s, "divui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(divui);
	return 0;
}

if (stringiseq2(s, "modui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(modui);
	return 0;
}

if (stringiseq2(s, "divui2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(divui2);
	return 0;
}

if (stringiseq2(s, "modui2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(modui2);
	return 0;
}

if (stringiseq2(s, "and")) {
	mmrefdown(s.ptr);
	opcodeoutinin(and);
	return 0;
}

if (stringiseq2(s, "andi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(andi);
	return 0;
}

if (stringiseq2(s, "or")) {
	mmrefdown(s.ptr);
	opcodeoutinin(or);
	return 0;
}

if (stringiseq2(s, "ori")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ori);
	return 0;
}

if (stringiseq2(s, "xor")) {
	mmrefdown(s.ptr);
	opcodeoutinin(xor);
	return 0;
}

if (stringiseq2(s, "xori")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(xori);
	return 0;
}

if (stringiseq2(s, "not")) {
	mmrefdown(s.ptr);
	opcodeoutin(not);
	return 0;
}

if (stringiseq2(s, "cpy")) {
	mmrefdown(s.ptr);
	opcodeoutin(cpy);
	return 0;
}

if (stringiseq2(s, "sll")) {
	mmrefdown(s.ptr);
	opcodeoutinin(sll);
	return 0;
}

if (stringiseq2(s, "slli")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(slli);
	return 0;
}

if (stringiseq2(s, "slli2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(slli2);
	return 0;
}

if (stringiseq2(s, "srl")) {
	mmrefdown(s.ptr);
	opcodeoutinin(srl);
	return 0;
}

if (stringiseq2(s, "srli")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(srli);
	return 0;
}

if (stringiseq2(s, "srli2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(srli2);
	return 0;
}

if (stringiseq2(s, "sra")) {
	mmrefdown(s.ptr);
	opcodeoutinin(sra);
	return 0;
}

if (stringiseq2(s, "srai")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(srai);
	return 0;
}

if (stringiseq2(s, "srai2")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(srai2);
	return 0;
}

if (stringiseq2(s, "zxt")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(zxt);
	return 0;
}

if (stringiseq2(s, "sxt")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sxt);
	return 0;
}

#if 0
// ###: The following function
// is no longer needed, but it is
// kept for documentation reasons.
// It was used when I considered
// implementing an instruction that
// swap the value of two registers.
if (stringiseq2(s, "swap")) {
	
	mmrefdown(s.ptr);
	
	readoperands(2, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[1].arg) {
		
		lyricalvariable* v = operand[1].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable I throw an error.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			curpos = savedcurpos;
			throwerror("the operand cannot be readonly");
		}
		
		if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return 0;
	}
	
	// I process the input-output operand.
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar5 () {
				// I first check whether the tempvar
				// is shared with the second argument.
				// Note that if operand[1].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange().
				if (operand[1].arg && operand[1].arg->v == operand[0].arg->v)
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar5() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed))),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	// I process the input-output operand.
	if (operand[1].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
		
		if (operand[1].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[1].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar6 () {
				// I search among registered arguments.
				// Unlike what is done in issharedtempvar5(),
				// I do not check whether operand[0].arg->v == operand[1].arg->v
				// because operand[0].arg->v has already been processed.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[1].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar6() && operand[1].reg->dirty)
				flushreg(operand[1].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[1].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[1].arg->varpushed,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[1].arg->varpushed since the variable
			// pointed by operand[1].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[1].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[1].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[1].reg->v = operand[1].arg->varpushed;
			operand[1].reg->offset = processvaroffsetifany(&operand[1].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[1].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[1].arg->v,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// If while obtaining the operand register
		// operand[1].reg, I find that the operand
		// register operand[0].reg has been discarded,
		// then it mean that both input-output operands
		// were overlapping each other, and
		// I should throw an error.
		// Note that, there can never be an overlap when
		// one of the operand has been duplicated during
		// pushing, because it will be a tempvar and
		// a tempvar created during pushing is only used
		// for that argument until freed once the operator,
		// function or assembly call has been made.
		if (operand[0].arg && !operand[0].reg->v) {
			curpos = savedcurpos;
			throwerror("The operands cannot overlap in memory");
		}
		
		// Since I am going to modify the value
		// of the register operand[1].reg,
		// I need to set it dirty.
		operand[1].reg->dirty = 1;
	}
	
	swap(operand[0].reg, operand[1].reg);
	
	// If the operands are volatile, I flush them.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	if (operand[1].arg && *operand[1].arg->varpushed->alwaysvolatile) flushreg(operand[1].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
	
	return 0;
}
#endif

if (stringiseq2(s, "seq")) {
	mmrefdown(s.ptr);
	opcodeoutinin(seq);
	return 0;
}

if (stringiseq2(s, "sne")) {
	mmrefdown(s.ptr);
	opcodeoutinin(sne);
	return 0;
}

if (stringiseq2(s, "seqi")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(seqi);
	return 0;
}

if (stringiseq2(s, "snei")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(snei);
	return 0;
}

if (stringiseq2(s, "slt")) {
	mmrefdown(s.ptr);
	opcodeoutinin(slt);
	return 0;
}

if (stringiseq2(s, "slte")) {
	mmrefdown(s.ptr);
	opcodeoutinin(slte);
	return 0;
}

if (stringiseq2(s, "sltu")) {
	mmrefdown(s.ptr);
	opcodeoutinin(sltu);
	return 0;
}

if (stringiseq2(s, "slteu")) {
	mmrefdown(s.ptr);
	opcodeoutinin(slteu);
	return 0;
}

if (stringiseq2(s, "slti")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(slti);
	return 0;
}

if (stringiseq2(s, "sltei")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sltei);
	return 0;
}

if (stringiseq2(s, "sltui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sltui);
	return 0;
}

if (stringiseq2(s, "slteui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(slteui);
	return 0;
}

if (stringiseq2(s, "sgti")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sgti);
	return 0;
}

if (stringiseq2(s, "sgtei")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sgtei);
	return 0;
}

if (stringiseq2(s, "sgtui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sgtui);
	return 0;
}

if (stringiseq2(s, "sgteui")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(sgteui);
	return 0;
}

if (stringiseq2(s, "sz")) {
	mmrefdown(s.ptr);
	opcodeoutin(sz);
	return 0;
}

if (stringiseq2(s, "snz")) {
	mmrefdown(s.ptr);
	opcodeoutin(snz);
	return 0;
}

if (stringiseq2(s, "jeq")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jeq);
	return 0;
}

if (stringiseq2(s, "jeqi")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jeqi);
	return 0;
}

if (stringiseq2(s, "jeqr")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jeqr);
	return 0;
}

if (stringiseq2(s, "jne")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jne);
	return 0;
}

if (stringiseq2(s, "jnei")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jnei);
	return 0;
}

if (stringiseq2(s, "jner")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jner);
	return 0;
}

if (stringiseq2(s, "jlt")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jlt);
	return 0;
}

if (stringiseq2(s, "jlti")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jlti);
	return 0;
}

if (stringiseq2(s, "jltr")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jltr);
	return 0;
}

if (stringiseq2(s, "jlte")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jlte);
	return 0;
}

if (stringiseq2(s, "jltei")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jltei);
	return 0;
}

if (stringiseq2(s, "jlter")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jlter);
	return 0;
}

if (stringiseq2(s, "jltu")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jltu);
	return 0;
}

if (stringiseq2(s, "jltui")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jltui);
	return 0;
}

if (stringiseq2(s, "jltur")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jltur);
	return 0;
}

if (stringiseq2(s, "jlteu")) {
	mmrefdown(s.ptr);
	opcodejcondininlabel(jlteu);
	return 0;
}

if (stringiseq2(s, "jlteui")) {
	mmrefdown(s.ptr);
	opcodejcondininimm(jlteui);
	return 0;
}

if (stringiseq2(s, "jlteur")) {
	mmrefdown(s.ptr);
	opcodejcondininin(jlteur);
	return 0;
}

if (stringiseq2(s, "jz")) {
	mmrefdown(s.ptr);
	opcodejcondinlabel(jz);
	return 0;
}

if (stringiseq2(s, "jzi")) {
	mmrefdown(s.ptr);
	opcodejcondinimm(jzi);
	return 0;
}

if (stringiseq2(s, "jzr")) {
	mmrefdown(s.ptr);
	opcodejcondinin(jzr);
	return 0;
}

if (stringiseq2(s, "jnz")) {
	mmrefdown(s.ptr);
	opcodejcondinlabel(jnz);
	return 0;
}

if (stringiseq2(s, "jnzi")) {
	mmrefdown(s.ptr);
	opcodejcondinimm(jnzi);
	return 0;
}

if (stringiseq2(s, "jnzr")) {
	mmrefdown(s.ptr);
	opcodejcondinin(jnzr);
	return 0;
}

if (stringiseq2(s, "j")) {
	mmrefdown(s.ptr);
	opcodejlabel(j);
	return 0;
}

if (stringiseq2(s, "ji")) {
	mmrefdown(s.ptr);
	opcodejimm(ji);
	return 0;
}

if (stringiseq2(s, "jr")) {
	mmrefdown(s.ptr);
	opcodejin(jr);
	return 0;
}

if (stringiseq2(s, "jl")) {
	
	mmrefdown(s.ptr);
	
	readoperands(1, COMMAENDING);
	
	// The output operand cannot be
	// a variable because its value
	// cannot be flushed before branching;
	// the destination of a branching
	// is a location where no register
	// has been allocated.
	if (operand[0].arg) {
		curpos = savedcurpos;
		throwerror("the output operand must be a register");
	}
	
	u8* savedcurpos = curpos;
	
	// I read the label name to which
	// I am supposed to jump to.
	string label = readsymbol(LOWERCASESYMBOL);
	
	if (!label.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	// Instructions are not generated in the firstpass.
	if (!compilepass) {
		
		mmrefdown(label.ptr);
		
		// There is no need to call freefuncarg(),
		// since there was no pushed argument.
		
		return 0;
	}
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	// I set curpos to savedcurpos so that
	// resolvelabellater(), used by jl(),
	// correctly set the field pos of the newly
	// created lyricallabeledinstructiontoresolve.
	swapvalues(&curpos, &savedcurpos);
	
	jl(operand[0].reg, label);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The string label is duplicated by jl();
	// so here I free the string in label
	// since I will no longer need it.
	mmrefdown(label.ptr);
	
	// There is no need to call freefuncarg(),
	// since there was no pushed argument.
	
	return 0;
}

if (stringiseq2(s, "jli")) {
	
	mmrefdown(s.ptr);
	
	readoperands(2, SEMICOLONENDING);
	
	// The output operand cannot be
	// a variable because its value
	// cannot be flushed before branching;
	// the destination of a branching
	// is a location where no register
	// has been allocated.
	if (operand[0].arg) {
		curpos = savedcurpos;
		throwerror("the output operand must be a register");
	}
	
	if (operand[1].arg) {
		
		if (!operand[1].arg->v->isnumber) {
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return 0;
	}
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	jli(operand[0].reg, ifnativetypedosignorzeroextend(operand[1].arg->v->numbervalue, operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)));
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
	
	return 0;
}

if (stringiseq2(s, "jlr")) {
	
	mmrefdown(s.ptr);
	
	readoperands(2, SEMICOLONENDING);
	
	// The output operand cannot be
	// a variable because its value
	// cannot be flushed before branching;
	// the destination of a branching
	// is a location where no register
	// has been allocated.
	if (operand[0].arg) {
		curpos = savedcurpos;
		throwerror("the output operand must be a register");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return 0;
	}
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
	}
	
	// I lock the register to prevent
	// a call of allocreg() from using it.
	operand[0].reg->lock = 1;
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	jlr(operand[0].reg, operand[1].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	if (funcarg) freefuncarg();
	
	return 0;
}

if (stringiseq2(s, "jpush")) {
	mmrefdown(s.ptr);
	opcodejlabel(jpush);
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
	}
	return 0;
}

if (stringiseq2(s, "jpushi")) {
	mmrefdown(s.ptr);
	opcodejimm(jpushi);
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
	}
	return 0;
}

if (stringiseq2(s, "jpushr")) {
	mmrefdown(s.ptr);
	opcodejin(jpushr);
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
	}
	return 0;
}

if (stringiseq2(s, "jpop")) {
	
	mmrefdown(s.ptr);
	
	// Instructions are not generated in the firstpass.
	if (!compilepass) return 0;
	
	// I flush and discard all registers
	// since I am branching.
	flushanddiscardallreg(FLUSHANDDISCARDALL);
	
	jpop();
	
	return 0;
}

if (stringiseq2(s, "afip")) {
	
	mmrefdown(s.ptr);
	
	readoperands(1, COMMAENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
		
		// When an operand is an output there is no need
		// to duplicate its value, because it is not
		// used by the instruction; the instruction only
		// set its value; so in the firstpass, I notify
		// the secondpass, that the output operand should
		// never be duplicated and that information
		// is used by pushargument().
		if (!compilepass) operand[0].arg->flag->istobeoutput = 1;
	}
	
	u8* savedcurpos = curpos;
	
	// I read the label.
	string label = readsymbol(LOWERCASESYMBOL);
	
	if (!label.ptr) {
		reverseskipspace();
		throwerror("expecting a label name");
	}
	
	// Instructions are not generated in the firstpass.
	if (!compilepass) {
		
		mmrefdown(label.ptr);
		
		if (funcarg) freefuncarg();
		
		return 0;
	}
	
	// I process the output operand.
	if (operand[0].arg) {
		// Note that an output operand cannot be
		// a readonly variable; also, the argument for
		// an output operand is never duplicated because
		// there is no need to do so since it is not used
		// by the instruction; the assembly instruction
		// only set its value. The register obtained
		// will be appropriately set dirty.
		operand[0].reg = getregforvar(operand[0].arg->varpushed, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FOROUTPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
	}
	
	// I set curpos to savedcurpos so that
	// resolvelabellater(), used by afip(),
	// correctly set the field pos of the newly
	// created lyricallabeledinstructiontoresolve.
	swapvalues(&curpos, &savedcurpos);
	
	afip(operand[0].reg, label);
	
	// Here I restore curpos.
	swapvalues(&curpos, &savedcurpos);
	
	// The string label is duplicated by afip().
	// So here I free the string in label since
	// I will no longer need it.
	mmrefdown(label.ptr);
	
	// If the output operand was a variable
	// (meaning its field arg was set) and
	// was volatile, I should flush it.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	
	if (funcarg) freefuncarg();
	
	return 0;
}

if (stringiseq2(s, "li")) {
	mmrefdown(s.ptr);
	opcodeoutimm(li);
	return 0;
}

if (stringiseq2(s, "ld8")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ld8);
	return 0;
}

if (stringiseq2(s, "ld8r")) {
	mmrefdown(s.ptr);
	opcodeoutin(ld8r);
	return 0;
}

if (stringiseq2(s, "ld8i")) {
	mmrefdown(s.ptr);
	opcodeoutimm(ld8i);
	return 0;
}

if (stringiseq2(s, "ld16")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ld16);
	return 0;
}

if (stringiseq2(s, "ld16r")) {
	mmrefdown(s.ptr);
	opcodeoutin(ld16r);
	return 0;
}

if (stringiseq2(s, "ld16i")) {
	mmrefdown(s.ptr);
	opcodeoutimm(ld16i);
	return 0;
}

if (stringiseq2(s, "ld32")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ld32);
	return 0;
}

if (stringiseq2(s, "ld32r")) {
	mmrefdown(s.ptr);
	opcodeoutin(ld32r);
	return 0;
}

if (stringiseq2(s, "ld32i")) {
	mmrefdown(s.ptr);
	opcodeoutimm(ld32i);
	return 0;
}

if (stringiseq2(s, "ld64")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ld64);
	return 0;
}

if (stringiseq2(s, "ld64r")) {
	mmrefdown(s.ptr);
	opcodeoutin(ld64r);
	return 0;
}

if (stringiseq2(s, "ld64i")) {
	mmrefdown(s.ptr);
	opcodeoutimm(ld64i);
	return 0;
}

if (stringiseq2(s, "ld")) {
	mmrefdown(s.ptr);
	opcodeoutinimm(ld);
	return 0;
}

if (stringiseq2(s, "ldr")) {
	mmrefdown(s.ptr);
	opcodeoutin(ldr);
	return 0;
}

if (stringiseq2(s, "ldi")) {
	mmrefdown(s.ptr);
	opcodeoutimm(ldi);
	return 0;
}

if (stringiseq2(s, "st8")) {
	mmrefdown(s.ptr);
	opcodeininimm(st8);
	return 0;
}

if (stringiseq2(s, "st8r")) {
	mmrefdown(s.ptr);
	opcodeinin(st8r);
	return 0;
}

if (stringiseq2(s, "st8i")) {
	mmrefdown(s.ptr);
	opcodeinimm(st8i);
	return 0;
}

if (stringiseq2(s, "st16")) {
	mmrefdown(s.ptr);
	opcodeininimm(st16);
	return 0;
}

if (stringiseq2(s, "st16r")) {
	mmrefdown(s.ptr);
	opcodeinin(st16r);
	return 0;
}

if (stringiseq2(s, "st16i")) {
	mmrefdown(s.ptr);
	opcodeinimm(st16i);
	return 0;
}

if (stringiseq2(s, "st32")) {
	mmrefdown(s.ptr);
	opcodeininimm(st32);
	return 0;
}

if (stringiseq2(s, "st32r")) {
	mmrefdown(s.ptr);
	opcodeinin(st32r);
	return 0;
}

if (stringiseq2(s, "st32i")) {
	mmrefdown(s.ptr);
	opcodeinimm(st32i);
	return 0;
}

if (stringiseq2(s, "st64")) {
	mmrefdown(s.ptr);
	opcodeininimm(st64);
	return 0;
}

if (stringiseq2(s, "st64r")) {
	mmrefdown(s.ptr);
	opcodeinin(st64r);
	return 0;
}

if (stringiseq2(s, "st64i")) {
	mmrefdown(s.ptr);
	opcodeinimm(st64i);
	return 0;
}

if (stringiseq2(s, "st")) {
	mmrefdown(s.ptr);
	opcodeininimm(st);
	return 0;
}

if (stringiseq2(s, "str")) {
	mmrefdown(s.ptr);
	opcodeinin(str);
	return 0;
}

if (stringiseq2(s, "sti")) {
	mmrefdown(s.ptr);
	opcodeinimm(sti);
	return 0;
}

if (stringiseq2(s, "ldst8")) {
	mmrefdown(s.ptr);
	opcodeinoutinimm(ldst8);
	return 0;
}

if (stringiseq2(s, "ldst8r")) {
	mmrefdown(s.ptr);
	opcodeinoutin(ldst8r);
	return 0;
}

if (stringiseq2(s, "ldst8i")) {
	mmrefdown(s.ptr);
	opcodeinoutimm(ldst8i);
	return 0;
}

if (stringiseq2(s, "ldst16")) {
	mmrefdown(s.ptr);
	opcodeinoutinimm(ldst16);
	return 0;
}

if (stringiseq2(s, "ldst16r")) {
	mmrefdown(s.ptr);
	opcodeinoutin(ldst16r);
	return 0;
}

if (stringiseq2(s, "ldst16i")) {
	mmrefdown(s.ptr);
	opcodeinoutimm(ldst16i);
	return 0;
}

if (stringiseq2(s, "ldst32")) {
	mmrefdown(s.ptr);
	opcodeinoutinimm(ldst32);
	return 0;
}

if (stringiseq2(s, "ldst32r")) {
	mmrefdown(s.ptr);
	opcodeinoutin(ldst32r);
	return 0;
}

if (stringiseq2(s, "ldst32i")) {
	mmrefdown(s.ptr);
	opcodeinoutimm(ldst32i);
	return 0;
}

if (stringiseq2(s, "ldst64")) {
	mmrefdown(s.ptr);
	opcodeinoutinimm(ldst64);
	return 0;
}

if (stringiseq2(s, "ldst64r")) {
	mmrefdown(s.ptr);
	opcodeinoutin(ldst64r);
	return 0;
}

if (stringiseq2(s, "ldst64i")) {
	mmrefdown(s.ptr);
	opcodeinoutimm(ldst64i);
	return 0;
}

if (stringiseq2(s, "ldst")) {
	mmrefdown(s.ptr);
	opcodeinoutinimm(ldst);
	return 0;
}

if (stringiseq2(s, "ldstr")) {
	mmrefdown(s.ptr);
	opcodeinoutin(ldstr);
	return 0;
}

if (stringiseq2(s, "ldsti")) {
	mmrefdown(s.ptr);
	opcodeinoutimm(ldsti);
	return 0;
}

#if 0
// ### Old implementations that did not update
// operands values unless they were registers.
// It is kept for reference.

// These instructions modify the value
// of the registers used with them.
// When using the associated asm instruction
// with variables, for the programmer convenience,
// the variable operands are not modified; but
// if the instruction is used with a register,
// the register value get modified just
// like the generated lyricalop behave.
// Hence argument variables to these instructions
// are treated as input-only where the value
// of the variable is used and remain the same
// after the instruction has completed, while
// argument registers are treated as input-output
// where the value of the register is used and
// modified after the instruction has completed.
uint ismem8cpy = stringiseq2(s, "mem8cpy") || ((sizeofgpr == 1) && stringiseq2(s, "memcpy"));
uint ismem8cpy2 = stringiseq2(s, "mem8cpy2") || ((sizeofgpr == 1) && stringiseq2(s, "memcpy2"));
uint ismem16cpy = stringiseq2(s, "mem16cpy") || ((sizeofgpr == 2) && stringiseq2(s, "memcpy"));;
uint ismem16cpy2 = stringiseq2(s, "mem16cpy2") || ((sizeofgpr == 2) && stringiseq2(s, "memcpy2"));
uint ismem32cpy = stringiseq2(s, "mem32cpy") || ((sizeofgpr == 4) && stringiseq2(s, "memcpy"));
uint ismem32cpy2 = stringiseq2(s, "mem32cpy2") || ((sizeofgpr == 4) && stringiseq2(s, "memcpy2"));
uint ismem64cpy = stringiseq2(s, "mem64cpy") || ((sizeofgpr == 8) && stringiseq2(s, "memcpy"));
uint ismem64cpy2 = stringiseq2(s, "mem64cpy2") || ((sizeofgpr == 8) && stringiseq2(s, "memcpy2"));
if (ismem8cpy || ismem8cpy2 || ismem16cpy || ismem16cpy2 || ismem32cpy || ismem32cpy2 || ismem64cpy || ismem64cpy2) {
	
	mmrefdown(s.ptr);
	
	readoperands(3, SEMICOLONENDING);
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return 0;
	}
	
	// I process the input operands.
	
	if (operand[2].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
		
		// Because the instruction will modify
		// the value of its input registers,
		// I allocate a register that will
		// hold a copy of the operand value.
		lyricalreg* r = allocreg(CRITICALREG);
		// Since the register is not assigned
		// to anything, I lock it, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		// I also lock the register to prevent
		// a call of allocreg() from using it.
		r->lock = 1;
		
		cpy(r, operand[2].reg);
		
		operand[2].reg->lock = 0;
		
		operand[2].reg = r;
	}
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
		
		// Because the instruction will modify
		// the value of its input registers,
		// I allocate a register that will
		// hold a copy of the operand value.
		lyricalreg* r = allocreg(CRITICALREG);
		// Since the register is not assigned
		// to anything, I lock it, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		// I also lock the register to prevent
		// a call of allocreg() from using it.
		r->lock = 1;
		
		cpy(r, operand[1].reg);
		
		operand[1].reg->lock = 0;
		
		operand[1].reg = r;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		// Because the instruction will modify
		// the value of its input registers,
		// I allocate a register that will
		// hold a copy of the operand value.
		lyricalreg* r = allocreg(CRITICALREG);
		// Since the register is not assigned
		// to anything, I lock it, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		// I also lock the register to prevent
		// a call of allocreg() from using it.
		r->lock = 1;
		
		cpy(r, operand[0].reg);
		
		operand[0].reg->lock = 0;
		
		operand[0].reg = r;
	}
	
	if (ismem8cpy) mem8cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem8cpy2) mem8cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem16cpy) mem16cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem16cpy2) mem16cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem32cpy) mem32cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem32cpy2) mem32cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem64cpy) mem64cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem64cpy2) mem64cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	
	if (operand[2].arg) {
		
		operand[2].reg->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", operand[2].reg->id));
		}
	}
	
	if (operand[1].arg) {
		
		operand[1].reg->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", operand[1].reg->id));
		}
	}
	
	if (operand[0].arg) {
		
		operand[0].reg->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", operand[0].reg->id));
		}
	}
	
	if (funcarg) freefuncarg();
	
	return 0;
}

// These instructions modify the value
// of the registers used with them.
// When using the associated asm instruction
// with variables, for the programmer convenience,
// the variable operands are not modified; but
// if the instruction is used with a register,
// the register value get modified just
// like the generated lyricalop behave.
// Hence argument variables to these instructions
// are treated as input-only where the value
// of the variable is used and remain the same
// after the instruction has completed, while
// argument registers are treated as input-output
// where the value of the register is used and
// modified after the instruction has completed.
uint ismem8cpyi = stringiseq2(s, "mem8cpyi") || ((sizeofgpr == 1) && stringiseq2(s, "memcpyi"));
uint ismem8cpyi2 = stringiseq2(s, "mem8cpyi2") || ((sizeofgpr == 1) && stringiseq2(s, "memcpyi2"));
uint ismem16cpyi = stringiseq2(s, "mem16cpyi") || ((sizeofgpr == 2) && stringiseq2(s, "memcpyi"));;
uint ismem16cpyi2 = stringiseq2(s, "mem16cpyi2") || ((sizeofgpr == 2) && stringiseq2(s, "memcpyi2"));
uint ismem32cpyi = stringiseq2(s, "mem32cpyi") || ((sizeofgpr == 4) && stringiseq2(s, "memcpyi"));
uint ismem32cpyi2 = stringiseq2(s, "mem32cpyi2") || ((sizeofgpr == 4) && stringiseq2(s, "memcpyi2"));
uint ismem64cpyi = stringiseq2(s, "mem64cpyi") || ((sizeofgpr == 8) && stringiseq2(s, "memcpyi"));
uint ismem64cpyi2 = stringiseq2(s, "mem64cpyi2") || ((sizeofgpr == 8) && stringiseq2(s, "memcpyi2"));
if (ismem8cpyi || ismem8cpyi2 || ismem16cpyi || ismem16cpyi2 || ismem32cpyi || ismem32cpyi2 || ismem64cpyi || ismem64cpyi2) {
	
	mmrefdown(s.ptr);
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return 0;
	}
	
	// I process the input operands.
	
	if (operand[1].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
		
		// Because the instruction will modify
		// the value of its input registers,
		// I allocate a register that will
		// hold a copy of the operand value.
		lyricalreg* r = allocreg(CRITICALREG);
		// Since the register is not assigned
		// to anything, I lock it, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		// I also lock the register to prevent
		// a call of allocreg() from using it.
		r->lock = 1;
		
		cpy(r, operand[1].reg);
		
		operand[1].reg->lock = 0;
		
		operand[1].reg = r;
	}
	
	if (operand[0].arg) {
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		// Because the instruction will modify
		// the value of its input registers,
		// I allocate a register that will
		// hold a copy of the operand value.
		lyricalreg* r = allocreg(CRITICALREG);
		// Since the register is not assigned
		// to anything, I lock it, otherwise
		// it will be seen as an unused register
		// when generating lyricalinstruction.
		// I also lock the register to prevent
		// a call of allocreg() from using it.
		r->lock = 1;
		
		cpy(r, operand[0].reg);
		
		operand[0].reg->lock = 0;
		
		operand[0].reg = r;
	}
	
	if (ismem8cpyi) mem8cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem8cpyi2) mem8cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem16cpyi) mem16cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem16cpyi2) mem16cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem32cpyi) mem32cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem32cpyi2) mem32cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem64cpyi) mem64cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem64cpyi2) mem64cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	
	if (operand[1].arg) {
		
		operand[1].reg->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", operand[1].reg->id));
		}
	}
	
	if (operand[0].arg) {
		
		operand[0].reg->lock = 0;
		
		if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
			
			comment(stringfmt("reg %%%d discarded", operand[0].reg->id));
		}
	}
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
	
	return 0;
}
#endif

uint ismem8cpy = stringiseq2(s, "mem8cpy");
uint ismem8cpy2 = stringiseq2(s, "mem8cpy2");
uint ismem16cpy = stringiseq2(s, "mem16cpy");
uint ismem16cpy2 = stringiseq2(s, "mem16cpy2");
uint ismem32cpy = stringiseq2(s, "mem32cpy");
uint ismem32cpy2 = stringiseq2(s, "mem32cpy2");
uint ismem64cpy = stringiseq2(s, "mem64cpy");
uint ismem64cpy2 = stringiseq2(s, "mem64cpy2");
uint ismemcpy = stringiseq2(s, "memcpy");
uint ismemcpy2 = stringiseq2(s, "memcpy2");
if (ismem8cpy || ismem8cpy2 || ismem16cpy || ismem16cpy2 || ismem32cpy || ismem32cpy2 || ismem64cpy || ismem64cpy2 || ismemcpy || ismemcpy2) {
	
	mmrefdown(s.ptr);
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[1].arg) {
		
		lyricalvariable* v = operand[1].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[1].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[1].reg->lock = 1;
				
				operand[1].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[2].arg) {
		
		lyricalvariable* v = operand[2].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[2].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[2].reg->lock = 1;
				
				operand[2].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		if (funcarg) freefuncarg();
		
		return 0;
	}
	
	// I process the input-output operands.
	
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v is
			// shared with another lyricalargument.
			uint issharedtempvar6 () {
				// I first check whether the tempvar
				// is shared with the second or third argument.
				// Note that if operand[1].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange();
				// similarly if operand[2].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[2].arg->v
				// is always a tempvar that was created by propagatevarchange().
				if ((operand[1].arg && operand[1].arg->v == operand[0].arg->v) ||
					(operand[2].arg && operand[2].arg->v == operand[0].arg->v))
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar6() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	if (operand[1].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
		
		if (operand[1].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[1].arg->v is
			// shared with another lyricalargument.
			uint issharedtempvar7 () {
				// I first check whether the tempvar
				// is shared with the second argument.
				// Note that if operand[2].arg->v == operand[1].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange().
				// Unlike what is done in issharedtempvar6(),
				// I do not check whether operand[0].arg->v == operand[1].arg->v
				// because operand[0].arg->v has already been processed.
				if (operand[2].arg && operand[2].arg->v == operand[1].arg->v)
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[1].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar7() && operand[1].reg->dirty)
				flushreg(operand[1].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[1].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[1].arg->varpushed,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[1].arg->varpushed since the variable
			// pointed by operand[1].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[1].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[1].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[1].reg->v = operand[1].arg->varpushed;
			operand[1].reg->offset = processvaroffsetifany(&operand[1].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[1].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[1].arg->v,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// If while obtaining the operand register
		// operand[1].reg, I find that the operand
		// register operand[0].reg has been discarded,
		// then it mean that both input-output operands
		// were overlapping each other, and
		// I should throw an error.
		// Note that, there can never be an overlap when
		// one of the operand has been duplicated during
		// pushing, because it will be a tempvar and
		// a tempvar created during pushing is only used
		// for that argument until freed once the operator,
		// function or assembly call has been made.
		if (operand[0].arg && !operand[0].reg->v) {
			curpos = savedcurpos;
			throwerror("The operands cannot overlap in memory");
		}
		
		// Since I am going to modify
		// the value of the register operand[1].reg,
		// I need to set it dirty.
		operand[1].reg->dirty = 1;
	}
	
	if (operand[2].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[2].reg = getregforvar(operand[2].arg->v, 0, operand[2].arg->typepushed, operand[2].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[2].reg->lock = 1;
		
		if (operand[2].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[2].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar8 () {
				// I search among registered arguments.
				// Unlike what is done in issharedtempvar6()
				// and issharedtempvar7(), I do not check whether
				// operand[0].arg->v == operand[2].arg->v or
				// operand[1].arg->v == operand[2].arg->v
				// because operand[0].arg->v and operand[1].arg->v
				// have already been processed.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[2].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar8() && operand[2].reg->dirty)
				flushreg(operand[2].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[2].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[2].arg->varpushed,
				sizeoftype(operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)),
				operand[2].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[2].arg->varpushed since the variable
			// pointed by operand[2].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[2].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[2].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[2].reg->v = operand[2].arg->varpushed;
			operand[2].reg->offset = processvaroffsetifany(&operand[2].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[2].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[2].arg->v,
				sizeoftype(operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)),
				operand[2].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// If while obtaining the operand register
		// operand[2].reg, I find that the operand
		// register operand[0].reg or operand[1].reg
		// have been discarded, then it mean that
		// both input-output operands were overlapping
		// each other, and I should throw an error.
		// Note that, there can never be an overlap when
		// one of the operand has been duplicated during
		// pushing, because it will be a tempvar and
		// a tempvar created during pushing is only used
		// for that argument until freed once the operator,
		// function or assembly call has been made.
		if ((operand[0].arg && !operand[0].reg->v) ||
			(operand[1].arg && !operand[1].reg->v)) {
			curpos = savedcurpos;
			throwerror("The operands cannot overlap in memory");
		}
		
		// Since I am going to modify
		// the value of the register operand[2].reg,
		// I need to set it dirty.
		operand[2].reg->dirty = 1;
	}
	
	if (ismem8cpy) mem8cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem8cpy2) mem8cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem16cpy) mem16cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem16cpy2) mem16cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem32cpy) mem32cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem32cpy2) mem32cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem64cpy) mem64cpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismem64cpy2) mem64cpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismemcpy) memcpy(operand[0].reg, operand[1].reg, operand[2].reg);
	else if (ismemcpy2) memcpy2(operand[0].reg, operand[1].reg, operand[2].reg);
	
	// If the operands are volatile, I flush them.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	if (operand[1].arg && *operand[1].arg->varpushed->alwaysvolatile) flushreg(operand[1].reg);
	if (operand[2].arg && *operand[2].arg->varpushed->alwaysvolatile) flushreg(operand[2].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	operand[2].reg->lock = 0;
	
	if (funcarg) freefuncarg();
	
	return 0;
}

uint ismem8cpyi = stringiseq2(s, "mem8cpyi");
uint ismem8cpyi2 = stringiseq2(s, "mem8cpyi2");
uint ismem16cpyi = stringiseq2(s, "mem16cpyi");
uint ismem16cpyi2 = stringiseq2(s, "mem16cpyi2");
uint ismem32cpyi = stringiseq2(s, "mem32cpyi");
uint ismem32cpyi2 = stringiseq2(s, "mem32cpyi2");
uint ismem64cpyi = stringiseq2(s, "mem64cpyi");
uint ismem64cpyi2 = stringiseq2(s, "mem64cpyi2");
uint ismemcpyi = stringiseq2(s, "memcpyi");
uint ismemcpyi2 = stringiseq2(s, "memcpyi2");
if (ismem8cpyi || ismem8cpyi2 || ismem16cpyi || ismem16cpyi2 || ismem32cpyi || ismem32cpyi2 || ismem64cpyi || ismem64cpyi2 || ismemcpyi || ismemcpyi2) {
	
	mmrefdown(s.ptr);
	
	readoperands(3, SEMICOLONENDING);
	
	if (operand[0].arg) {
		
		lyricalvariable* v = operand[0].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[0].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[0].reg->lock = 1;
				
				operand[0].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[1].arg) {
		
		lyricalvariable* v = operand[1].arg->varpushed;
		
		// If the output operand of the instruction
		// is a readonly variable, I load its value
		// in a lyricalreg and dis-associate the lyricalreg
		// from its lyricalvariable since the value
		// of a readonly variable cannot be changed,
		// in other words a dirty lyricalreg cannot
		// be flushed to a readonly variable.
		// Note that the field v of the lyricalargument
		// is not used to determine whether it is
		// a readonly variable, because the field
		// flag->istobeoutput is not set, since it is
		// an input-output argument, and pushargument()
		// can duplicate the variable which it would set
		// in the field v of the lyricalargument, and which
		// would prevent me from correctly determining
		// whether I have a readonly variable.
		// Note also that this argument cannot be passed
		// by reference, which would have caused
		// the lyricalvariable in the field v to be
		// the address of the lyricalvariable
		// in the field varpushed.
		if (isvarreadonly(v)) {
			// lyricalreg are usable only in the secondpass.
			if (compilepass) {
				// I use the type and bitselect with
				// which the variable was pushed.
				operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
				
				// I dis-associate the lyricalreg from
				// the lyricalvariable since the value
				// of a readonly variable cannot be changed,
				// in other words a dirty lyricalreg cannot
				// be flushed to a readonly variable.
				operand[1].reg->v = 0;
				
				// I lock the allocated register to prevent
				// another call to allocreg() from using it.
				// I also lock it, otherwise it could be lost
				// when insureenoughunusedregisters() is called
				// while creating a new lyricalinstruction.
				// I also lock it so that it is not seen
				// as an unused register when generating
				// lyricalinstruction, since the register
				// is not assigned to anything.
				operand[1].reg->lock = 1;
				
				operand[1].arg = 0;
			}
			
		} else if (v->name.ptr[1] != '*') {
			// Only lyricalvariable for variables explicitly
			// declared by the programmer should be used
			// with propagatevarchange; hence it should never be
			// a tempvar, a readonly variable or a dereference variable;
			// the lyricalvariable must have been used
			// with processvaroffsetifany() to insure that
			// there is no offset suffixed to its name;
			// the field id of a lyricalvariable is non-null
			// only for such lyricalvariable.
			
			// I call processvaroffsetifany() to check
			// if the variable pointed by v had an offset
			// suffixed to its name. If yes, it will find
			// the main variable and set it in v.
			uint offset = processvaroffsetifany(&v);
			
			if (v->id) {
				
				uint size = sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed));
				
				// There is no need to check whether the offset is outside
				// of the boundary of the variable in memory; because
				// evaluateexpression(), when parsing the postfix operator '.',
				// prevent the use of an offset that can result in an illegal
				// access beyond the size of the variable.
				
				// If the portion to modify on the variable
				// is greater than the size of the variable,
				// the size of the portion to modify is recomputed
				// to the maximum allowable size so that the portion
				// of the variable to modify is within its boundaries.
				// Keep in mind that getregforvar() do a similar
				// work to prevent access beyond the size of a variable.
				if (size > (v->size - offset))
					size = v->size - offset;
				
				propagatevarchange(v, offset, size);
			}
		}
	}
	
	if (operand[2].arg) {
		
		if (!operand[2].arg->v->isnumber) {
			
			curpos = savedcurpos;
			throwerror("incorrect immediate operand");
		}
		
	} else {
		
		curpos = savedcurpos;
		throwerror("immediate operand expected");
	}
	
	if (!compilepass) {
		// Instructions are not generated in the firstpass.
		
		// There is no need to check whether
		// there was any pushed argument by checking
		// if funcarg is null, because funcarg will
		// always be non-null since I am required
		// to have an immediate value.
		freefuncarg();
		
		return 0;
	}
	
	// I process the input-output operands.
	
	if (operand[0].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[0].reg = getregforvar(operand[0].arg->v, 0, operand[0].arg->typepushed, operand[0].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[0].reg->lock = 1;
		
		if (operand[0].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[0].arg->v is
			// shared with another lyricalargument.
			uint issharedtempvar9 () {
				// I first check whether the tempvar
				// is shared with the second argument.
				// Note that if operand[1].arg->v == operand[0].arg->v,
				// the lyricalvariable pointed by operand[1].arg->v
				// is always a tempvar that was created by propagatevarchange().
				if (operand[1].arg && operand[1].arg->v == operand[0].arg->v)
					return 1;
				
				// If I get here, I search among
				// registered arguments.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[0].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar9() && operand[0].reg->dirty)
				flushreg(operand[0].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[0].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[0].arg->varpushed,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[0].arg->varpushed since the variable
			// pointed by operand[0].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[0].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[0].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[0].reg->v = operand[0].arg->varpushed;
			operand[0].reg->offset = processvaroffsetifany(&operand[0].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[0].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[0].arg->v,
				sizeoftype(operand[0].arg->typepushed.ptr, stringmmsz(operand[0].arg->typepushed)),
				operand[0].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// Since I am going to modify the value
		// of the register operand[0].reg,
		// I need to set it dirty.
		operand[0].reg->dirty = 1;
	}
	
	if (operand[1].arg) {
		// Note that the input-output operand
		// cannot be a readonly variable.
		
		// I use the type and bitselect with
		// which the variable was pushed.
		operand[1].reg = getregforvar(operand[1].arg->v, 0, operand[1].arg->typepushed, operand[1].arg->bitselect, FORINPUT);
		
		// I lock the register to prevent
		// a call of allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		operand[1].reg->lock = 1;
		
		if (operand[1].arg->v->name.ptr[0] == '$') {
			// This function is used to determine whether
			// the tempvar pointed by operand[1].arg->v
			// is shared with another lyricalargument.
			uint issharedtempvar10 () {
				// I search among registered arguments.
				// Unlike what is done in issharedtempvar9(),
				// I do not check whether operand[0].arg->v == operand[1].arg->v
				// because operand[0].arg->v has already been processed.
				
				lyricalargument* arg = registeredargs;
				
				while (arg) {
					// Note that if arg->v == operand[0].arg->v,
					// the lyricalvariable pointed by arg->v
					// is always a tempvar that was created
					// by propagatevarchange().
					if (arg->v == operand[0].arg->v) return 1;
					
					arg = arg->nextregisteredarg;
				}
				
				return 0;
			}
			
			// If the tempvar from which the register
			// pointed by operand[1].reg was loaded is shared
			// with another lyricalargument, its value
			// should be flushed if it is dirty before
			// the register reassignment; otherwise the value
			// of the tempvar will be lost whereas it is
			// still needed by another argument.
			if (issharedtempvar10() && operand[1].reg->dirty)
				flushreg(operand[1].reg);
			
			// I get here if getregforvar() was used
			// with the duplicate of the variable pushed;
			// nothing was done to flush(If dirty) and discard
			// any register overlapping the memory region of
			// the variable operand[1].arg->varpushed that
			// I am going to modify; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			// Before reassignment, I also discard any register
			// associated with the memory region for which
			// I am trying to discard overlaps, otherwise
			// after the register reassignment made below,
			// I can have 2 registers associated with
			// the same memory location, violating the rule
			// about the use of registers; hence I call
			// discardoverlappingreg(), setting its argument
			// flag to DISCARDALLOVERLAP.
			discardoverlappingreg(
				operand[1].arg->varpushed,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAP);
			
			// I manually reassign the register
			// to operand[1].arg->varpushed since the variable
			// pointed by operand[1].arg->varpushed is the one
			// getting modified through the register and
			// the dirty value of the register should
			// be flushed to operand[1].arg->varpushed; I have
			// to do this because the field v of the register
			// is set to the duplicate of operand[1].arg->varpushed.
			// Reassigning the register instead of creating
			// a new register and copying its value is faster.
			operand[1].reg->v = operand[1].arg->varpushed;
			operand[1].reg->offset = processvaroffsetifany(&operand[1].reg->v);
			
			// Note that the value set in the fields offset
			// and size of the register could be wrong to where
			// the register represent a location beyond the boundary
			// of the variable; and that can only occur if
			// the programmer confused the compiler by using
			// incorrect casting type.
			
		} else {
			// I get here if getregforvar() was not used
			// with the duplicate of the variable pushed.
			// Since operand[1].arg->v which was used with
			// getregforvar() was certainly not a volatile
			// variable(Pushed volatile variables get duplicated),
			// the registers overlapping the memory region of
			// the variable associated with the register that
			// I am going to dirty were only flushed without
			// getting discarded; so I should discard
			// those overlapping registers in order to follow
			// the rule about the use of registers.
			discardoverlappingreg(
				operand[1].arg->v,
				sizeoftype(operand[1].arg->typepushed.ptr, stringmmsz(operand[1].arg->typepushed)),
				operand[1].arg->bitselect,
				DISCARDALLOVERLAPEXCEPTREGFORVAR);
		}
		
		// If while obtaining the operand register
		// operand[1].reg, I find that the operand
		// register operand[0].reg has been discarded,
		// then it mean that both input-output operands
		// were overlapping each other, and
		// I should throw an error.
		// Note that, there can never be an overlap when
		// one of the operand has been duplicated during
		// pushing, because it will be a tempvar and
		// a tempvar created during pushing is only used
		// for that argument until freed once the operator,
		// function or assembly call has been made.
		if (operand[0].arg && !operand[0].reg->v) {
			curpos = savedcurpos;
			throwerror("The operands cannot overlap in memory");
		}
		
		// Since I am going to modify
		// the value of the register operand[1].reg,
		// I need to set it dirty.
		operand[1].reg->dirty = 1;
	}
	
	if (ismem8cpyi) mem8cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem8cpyi2) mem8cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem16cpyi) mem16cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem16cpyi2) mem16cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem32cpyi) mem32cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem32cpyi2) mem32cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem64cpyi) mem64cpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismem64cpyi2) mem64cpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismemcpyi) memcpyi(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	else if (ismemcpyi2) memcpyi2(operand[0].reg, operand[1].reg, ifnativetypedosignorzeroextend(operand[2].arg->v->numbervalue, operand[2].arg->typepushed.ptr, stringmmsz(operand[2].arg->typepushed)));
	
	// If the operands are volatile, I flush them.
	if (operand[0].arg && *operand[0].arg->varpushed->alwaysvolatile) flushreg(operand[0].reg);
	if (operand[1].arg && *operand[1].arg->varpushed->alwaysvolatile) flushreg(operand[1].reg);
	
	// I unlock the registers that were allocated
	// and locked for the operands.
	// Locked registers must be unlocked only after
	// the instructions using them have been generated;
	// otherwise they could be lost when insureenoughunusedregisters()
	// is called while creating a new lyricalinstruction.
	operand[0].reg->lock = 0;
	operand[1].reg->lock = 0;
	
	// There is no need to check whether
	// there was any pushed argument by checking
	// if funcarg is null, because funcarg will
	// always be non-null since I am required
	// to have an immediate value.
	freefuncarg();
	
	return 0;
}

if (stringiseq2(s, "pagealloc")) {
	mmrefdown(s.ptr);
	opcodeoutin(pagealloc);
	return 0;
}

if (stringiseq2(s, "pagealloci")) {
	mmrefdown(s.ptr);
	opcodeoutimm(pagealloci);
	return 0;
}

if (stringiseq2(s, "pagefree")) {
	mmrefdown(s.ptr);
	opcodeinin(pagefree);
	return 0;
}

if (stringiseq2(s, "pagefreei")) {
	mmrefdown(s.ptr);
	opcodeinimm(pagefreei);
	return 0;
}

// If I get here, it is an error.
curpos = savedcurpos;
throwerror("invalid assembly instruction");
