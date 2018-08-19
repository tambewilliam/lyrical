
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


string type;
lyricalreg* r;
lyricalvariable* resultofshortcircuiteval;

if (*op == '?') {
	// I get here for the ternary operator
	// which look like this: var ? true : false;
	
	// These variable will contain the name of the string used for the labels located
	// after the last instruction generated for the true and false target respectively.
	string labelaftertruetargetinstructions;
	string labelafterfalsetargetinstructions;
	
	if (larg->cast.ptr) type = larg->cast;
	else type = larg->type;
	
	type = stringduplicate1(type);
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("the ternary operator test argument can only be of a native type or be a pointer");
	}
	
	uint printregdiscardmsg;
	
	// Labels and instructions are generated only in the secondpass.
	if (compilepass) {
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		(r = getregforvar(larg, 0, type, larg->bitselect, FORINPUT))->lock = 1;
		
		// If the register pointed by r is allocated to a tempvar,
		// it is discarded so as to prevent its value from being flushed
		// by flushanddiscardallreg() if it was dirty; in fact if the
		// lyrical variable larg is a tempvar, it will be freed soon
		// after the branching instruction making use of the register r
		// has been generated.
		if (r->v->name.ptr[0] == '$') {
			
			r->v = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			
			setregtothetop(r);
			
		} else printregdiscardmsg = 0;
		
		// I flush all registers without discarding them
		// since I am doing a conditional branching.
		flushanddiscardallreg(DONOTDISCARD);
		
		// Here I generate an instruction which will
		// jump depending on whether the value in
		// the register r is true or false.
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJZ);
		i->r1 = r->id;
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		labelaftertruetargetinstructions = stringfmt("%d", newgenericlabelid());
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelaftertruetargetinstructions, &i->imm->i);
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(larg);
	
	
	// I evaluate the true target.
	
	// I recursively call evaluateexpression() to
	// evaluate the true target of the ternary operator.
	// It is parsed as if paranthesized; its precedence
	// relative to the operator "?:" is ignored.
	rarg = evaluateexpression(LOWESTPRECEDENCE);
	
	if (rarg == EXPRWITHNORETVAL) {
		curpos = savedcurpos;
		throwerror("invalid expression to use for the true target of the ternary operator");
	}
	
	// The middle operand in the short-circuit operator ?: expression
	// may be omitted and if the first operand is nonzero, the result
	// is the value of the condition expression, otherwise
	// if the first operand is zero, the result is the value
	// of the third operand which is the false target expression.
	if (!rarg) {
		// I create a tempvar and set it in resultofshortcircuiteval.
		
		resultofshortcircuiteval = varalloc(sizeoftype(type.ptr, stringmmsz(type)), LOOKFORAHOLE);
		
		resultofshortcircuiteval->name = generatetempvarname(resultofshortcircuiteval);
		
		// Note that the field type will be null when obtained from varalloc(); also
		// the field type of variables allocated with varalloc() get freed by varfree().
		resultofshortcircuiteval->type = type;
		
		// Labels and instructions are generated only in the secondpass.
		if (compilepass) {
			// I manually reassign the register (containing
			// the value of the condition expression)
			// to resultofshortcircuiteval.
			r->v = resultofshortcircuiteval;
			r->offset = 0;
			r->bitselect = 0;
			r->dirty = 1;
		}
		
	} else {
		
		mmrefdown(type.ptr);
		
		// Labels and instructions are generated only in the secondpass.
		if (compilepass) {
			// Unlock lyricalreg.
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
		
		// Setting resultofshortcircuiteval to null will make getvarduplicate()
		// create a tempvar for the duplicate and set it in resultofshortcircuiteval.
		resultofshortcircuiteval = 0;
		getvarduplicate(&resultofshortcircuiteval, rarg);
		
		// If I have a tempvar or a lyricalvariable
		// which depend on a tempvar (dereference variable
		// or variable with its name suffixed with an offset),
		// I should free all those variables because
		// I will no longer need them and it allow
		// the generated code to save stack memory.
		varfreetempvarrelated(rarg);
	}
	
	// Labels and instructions are generated only in the secondpass.
	if (compilepass) {
		// I flush and discard all registers since I am going to branch.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// Here I generate a jump instruction
		lyricalinstruction* i = newinstruction(currentfunc, LYRICALOPJ);
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		labelafterfalsetargetinstructions = stringfmt("%d", newgenericlabelid());
		
		// The address where I have to jump to is resolved later.
		resolvelabellater(labelafterfalsetargetinstructions, &i->imm->i);
		
		// I create the label which mark the location
		// after the last instruction that is created
		// for the true target of the ternary operator.
		newlabel(labelaftertruetargetinstructions);
	}
	
	// I evaluate the false target.
	
	u8* savedcurpos2 = curpos;
	
	// True and false targets of a ternary operator are separated by a colon.
	if (*curpos == ':') {
		
		++curpos; // Position curpos after ':' .
		
		skipspace();
		
	} else {
		
		reverseskipspace();
		
		throwerror("expecting a colon after the expression used for the true target of the ternary operator");
	}
	
	// I recursively call evaluateexpression() to
	// evaluate the false target of the ternary operator.
	rarg = evaluateexpression(OPPRECEDENCE);
	
	if (!rarg || rarg == EXPRWITHNORETVAL) {
		curpos = savedcurpos2;
		throwerror("no expression to use for the false target of the ternary operator");
	}
	
	// The true and false target of the ternary operator need to have the same type since an operator
	// can only return a single variable with a specific type.
	if (!stringiseq1(rarg->cast.ptr ? rarg->cast : rarg->type, resultofshortcircuiteval->type)) {
		curpos = savedcurpos2;
		throwerror("the expressions for true and false target of the ternary operator have to be of the same type");
	}
	
	// resultofshortcircuiteval is non-null,
	// hence getvarduplicate() will use that variable
	// for the duplicate instead of creating
	// a new variable for the duplicate.
	// I should only have a single variable that
	// is the return variable of the ternary operator.
	getvarduplicate(&resultofshortcircuiteval, rarg);
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(rarg);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I flush and discard all registers.
		// This is similar to cases where I have to call
		// flushanddiscardallreg() when encountering a closing
		// bracket for the if(), while(), etc... clauses.
		// Whatever come next has to start with un-used registers.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label which mark the location
		// after the last instruction that is created
		// for the false target of the ternary operator.
		newlabel(labelafterfalsetargetinstructions);
	}
	
	larg = resultofshortcircuiteval;
	
	// I jump to parsenormalop to process the next normal operator.
	goto parsenormalop;
	
} else if (stringiseq3(op, "&&") || stringiseq3(op, "||")) {
	// This variable will contain the name of
	// the string used for the label located after
	// the last instruction created for the right
	// operand of the short-circuit operator.
	string labelnameafterrightoperand;
	
	if (larg->cast.ptr) type = larg->cast;
	else type = larg->type;
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("arguments to the operator && or || can only be pointers or have native types");
	}
	
	// This variable will point to the result variable of the short-circuit evaluation.
	// Note that I create my own result variable and do not use callfunctionnow().
	// ei: c+(a&&b); Here I would need a result variable for (a&&b) to use with c.
	resultofshortcircuiteval = varalloc(sizeofgpr, LOOKFORAHOLE);
	resultofshortcircuiteval->name = generatetempvarname(resultofshortcircuiteval);
	
	// The field type of the variable allocated is set to uint.
	// The reason for choosing uint for the result variable is because it generate
	// faster code since there can never be instructions generated to keep it within boundary.
	// I use the field type of the variable instead of its field cast because it
	// represents the real size of the variable in memory.
	// The field type of a variable allocated with varalloc() get freed by varfree().
	resultofshortcircuiteval->type = stringduplicate1(largeenoughunsignednativetype(sizeofgpr));
	
	lyricalreg* rresult;
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		(rresult = getregforvar(resultofshortcircuiteval, 0, largeenoughunsignednativetype(sizeofgpr), 0, FOROUTPUT))->lock = 1;
		
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		(r = getregforvar(larg, 0, type, larg->bitselect, FORINPUT))->lock = 1;
		
		uint printregdiscardmsg;
		
		// If the register pointed by r is allocated to a tempvar,
		// it is discarded so as to prevent its value from being flushed
		// by flushanddiscardallreg() if it was dirty; in fact if the
		// lyrical variable larg is a tempvar, it will be freed soon
		// after the branching instruction making use of the register r
		// has been generated.
		if (r->v->name.ptr[0] == '$') {
			
			r->v = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) printregdiscardmsg = 1;
			
			setregtothetop(r);
			
		} else printregdiscardmsg = 0;
		
		snz(rresult, r);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		r->lock = 0;
		
		if (printregdiscardmsg) {
			// For clarity in the output,
			// the comment instruction is
			// generated after the location
			// where the register is last used.
			
			comment(stringfmt("reg %%%d discarded", r->id));
		}
		
		// The register pointed by rresult was locked. Having the register locked will
		// insure that even when the register is discarded it do not get used by allocreg()
		// (Which will lead to its value getting corrupted by another rountine) during
		// flushing, since I need its value for the jz or jnz instruction.
		
		// I flush all registers without discarding them
		// since I am going to use a conditional branching.
		flushanddiscardallreg(DONOTDISCARD);
		
		// Here I generate an instruction which will jump depending on whether the value in
		// the register rresult is true or false. So if (*op == '&'), a jump will occur only if the
		// value in the register rresult is false. On the other hand if (*op == '|'), a jump will occur
		// only if the value in the register rresult is true.
		// The jump instruction will be resolved later to the label coming right after
		// the last instruction generated while evaluating the right argument.
		// In fact, the operators && || behave in a short-circuit manner where the right
		// operand may not be evaluated depending on the value of the left operand.
		// If (*op == '&'), I generate jz rresult, locationtojumpto;
		// If (*op == '|'), I generate jnz rresult, locationtojumpto;
		lyricalinstruction* i = newinstruction(currentfunc, (*op == '&') ? LYRICALOPJZ : LYRICALOPJNZ);
		i->r1 = rresult->id;
		i->imm = mmallocz(sizeof(lyricalimmval));
		i->imm->type = LYRICALIMMOFFSETTOINSTRUCTION;
		
		labelnameafterrightoperand = stringfmt("%d", newgenericlabelid());
		
		// The address where I have to jump to is resolved later after
		// having processed the right operand of the short-circuit operator.
		resolvelabellater(labelnameafterrightoperand, &i->imm->i);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		rresult->lock = 0;
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(larg);
	
	// I recursively call evaluateexpression()
	// to evaluate the right side of the operator.
	rarg = evaluateexpression(OPPRECEDENCE);
	
	if (!rarg || rarg == EXPRWITHNORETVAL) {
		curpos = savedcurpos;
		throwerror("no operand for the operator right argument");
	}
	
	if (rarg->cast.ptr) type = rarg->cast;
	else type = rarg->type;
	
	if (!pamsynmatch2(isnativeorpointertype, type.ptr, stringmmsz(type)).start) {
		curpos = savedcurpos;
		throwerror("arguments to the operator && or || can only be pointers or have native types");
	}
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I lock the allocated register to prevent
		// another call to allocreg() from using it.
		// I also lock it, otherwise it could be lost
		// when insureenoughunusedregisters() is called
		// while creating a new lyricalinstruction.
		(rresult = getregforvar(resultofshortcircuiteval, 0, largeenoughunsignednativetype(sizeofgpr), 0, FOROUTPUT))->lock = 1;
		
		(r = getregforvar(rarg, 0, type, rarg->bitselect, FORINPUT))->lock = 1;
		
		snz(rresult, r);
		
		// Unlock lyricalreg.
		// Locked registers must be unlocked only after
		// the instructions using them have been generated;
		// otherwise they could be lost when insureenoughunusedregisters()
		// is called while creating a new lyricalinstruction.
		rresult->lock = 0;
		r->lock = 0;
		
		// If the register pointed by r is allocated to a tempvar,
		// it is discarded so as to prevent its value from being flushed
		// by flushanddiscardallreg() if it was dirty; in fact if the
		// lyrical variable rarg is a tempvar, it get freed below.
		if (r->v->name.ptr[0] == '$') {
			
			r->v = 0;
			
			if (compileargcompileflag&LYRICALCOMPILECOMMENT) {
				// The lyricalreg pointed by r is not allocated
				// to a lyricalvariable but since I am done using it,
				// I should also produce a comment about it having been
				// discarded to complement the allocation comment that
				// was generated when it was allocated.
				comment(stringfmt("reg %%%d discarded", r->id));
			}
			
			setregtothetop(r);
		}
	}
	
	// If I have a tempvar or a lyricalvariable
	// which depend on a tempvar (dereference variable
	// or variable with its name suffixed with an offset),
	// I should free all those variables because
	// I will no longer need them and it allow
	// the generated code to save stack memory.
	varfreetempvarrelated(rarg);
	
	// Instructions are generated only in the secondpass.
	if (compilepass) {
		// I flush and discard all registers.
		// This is similar to cases where I have to call
		// flushanddiscardallreg() when encountering a closing
		// bracket for the if(), while(), etc... clauses.
		// Whatever come next has to start with un-used registers.
		flushanddiscardallreg(FLUSHANDDISCARDALL);
		
		// I create the label which mark the location
		// after the last instruction that is created for
		// the right operand of the short-circuit operator.
		newlabel(labelnameafterrightoperand);
	}
	
	larg = resultofshortcircuiteval;
	
	// I jump to parsenormalop to process the next normal operator.
	goto parsenormalop;
}
