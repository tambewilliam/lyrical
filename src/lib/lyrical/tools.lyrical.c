
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


// This function is used to free
// a linkedlist of lyricalinstruction.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freeinstructionlinkedlist (lyricalinstruction* linkedlist) {
	
	lyricalinstruction* i = linkedlist;
	
	do {
		if (i->op == LYRICALOPMACHINECODE) mmrefdown(i->opmachinecode.ptr);
		else if (i->op == LYRICALOPCOMMENT) mmrefdown(i->comment.ptr);
		else {
			if (i->imm) {
				// I free the linkedlist of immediate
				// values attached to the instruction.
				
				lyricalimmval* imm = i->imm;
				
				do {
					lyricalimmval* savedimm = imm;
					imm = imm->next;
					mmrefdown(savedimm);
					
				} while (imm);
			}
		}
		
		if (i->unusedregs) mmrefdown(i->unusedregs);
		
		if (i->dbginfo.filepath.ptr) mmrefdown(i->dbginfo.filepath.ptr);
		
		lyricalinstruction* savedi = i;
		i = i->next;
		mmrefdown(savedi);
		
	} while (i != linkedlist);
}


// This function is used to free
// a linkedlist of lyricalvariable.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freevarlinkedlist (lyricalvariable* linkedlist) {
	
	lyricalvariable* v = linkedlist;
	
	do {
		if (v->name.ptr) mmrefdown(v->name.ptr);
		
		if (v->type.ptr) mmrefdown(v->type.ptr);
		if (v->cast.ptr) mmrefdown(v->cast.ptr);
		
		if (v->scope) mmrefdown(v->scope);
		
		lyricalvariable* savedv = v;
		v = v->next;
		mmrefdown(savedv);
		
	} while (v != linkedlist);
}


// Functions used only within lyricalcompile().
#ifdef LYRICALCOMPILE

// Used to compute the length of a string
// which can be terminated by '\n' or 0.
uint slen (u8* ptr) {
	
	u8* savedptr = ptr;
	
	while (*ptr && *ptr != '\n') ++ptr;
	
	return (uint)ptr - (uint)savedptr;
}

// This function return the number
// of lines between the locations given
// by the arguments begin and end.
uint countlines (u8* begin, u8* end) {
	
	uint linecount = 1;
	
	while (--end >= begin) {
		
		if (*end == '\n') ++linecount;
	}
	
	return linecount;
}


// Used to throw an error.
void throwerror (u8* msg) {
	
	if (curpos) {
		// This set curpos so that it be
		// within the last chunk, when it is
		// at the null terminating character,
		// otherwise findchunk() will incorrectly
		// report it as part of the first chunk.
		curpos -= !*curpos;
		
		// String that will contain file and line
		// number where the error occured.
		string origin = stringnull;
		
		if (chunks) {
			// I get here when the error occured
			// after preprocessing.
			
			chunk* c;
			
			uint o;
			
			// This function find the chunk that
			// correspond to the address location
			// in the variable curpos and set its pointer
			// in the variable c; it will also set
			// the variable o with the offset within
			// the chunk content.
			void findchunk () {
				
				o = (uint)curpos - (uint)compileargsource;
				
				c = chunks;
				
				while (1) {
					
					uint ccontentsz = c->contentsz;
					
					if (o < ccontentsz) return;
					
					o -= ccontentsz;
					
					c = c->next;
				}
			}
			
			findchunk();
			
			string s;
			
			u8* pos = curpos;
			
			if (c->origin.ptr) {
				// If I get here, the error was found within a macro.
				
				s = stringfmt("%s:%d: %s\n", c->path,
					countlines(pos - o, pos) + c->linenumber -1,
					c->origin);
				
				stringappend1(&origin, s);
				
				mmrefdown(s.ptr);
				
				s = c->path;
				
				// This loop skip chunks so that the next chunk
				// to process is originating from outside any macro.
				do {
					c = c->prev;
					
					pos -= o;
					
					o = c->contentsz;
					
				} while (c->origin.ptr);
				
			} else s = stringnull;
			
			if (!stringiseq1(c->path, s)) {
				
				s = stringfmt("%s:%d\n",
					c->path,
					countlines(pos - o, pos) + c->linenumber -1);
				
				stringappend1(&origin, s);
				
				mmrefdown(s.ptr);
			}
			
			// I trace all the included file all the way up
			// to the top file where compilation started.
			while (1) {
				// This loop skip chunks for which the string
				// in their field path is the same.
				do {
					if (c == chunks) goto donewithlocatingerror;
					
					s = c->path;
					
					c = c->prev;
					
					pos -= o;
					
					o = c->contentsz;
					
				} while (stringiseq1(c->path, s));
				
				chunk* cfirst = c->first;
				
				if (cfirst) {
					
					cfirstchecked:;
					
					// If I get here, I skip chunks
					// for an included file that
					// did not include the file
					// where the error was found.
					
					// Re-adjust cfirst
					// so that the loop below
					// stop at the chunk that is
					// previous to what was initialy
					// set in cfirst.
					cfirst = cfirst->prev;
					
					do {
						if (c == chunks) goto donewithlocatingerror;
						
						c = c->prev;
						
						pos -= o;
						
						o = c->contentsz;
						
					} while (c != cfirst);
					
					//if (cfirst = c->first) goto cfirstchecked; // TODO: To uncomment once fixed ...
					
					continue;
					
				} else if (c->origin.ptr) {
					// If I get here, I skip chunks
					// for macros from which
					// the error did not originate.
					
					do {
						if (c == chunks) goto donewithlocatingerror;
						
						c = c->prev;
						
						pos -= o;
						
						o = c->contentsz;
						
					} while (c->origin.ptr);
					
					//if (cfirst = c->first) goto cfirstchecked; // TODO: To uncomment once fixed ...
					
					continue;
				}
				
				if (c->path.ptr[0]) {
					
					s = stringfmt("%s:%d\n",
						c->path,
						countlines(pos - o, pos) + c->linenumber -1);
					
					stringappend1(&origin, s);
					
					mmrefdown(s.ptr);
				}
			}
			
		} else {
			// I get here when the error occured
			// during preprocessing.
			
			string s = stringfmt("%s:%d\n",
				currentfilepath,
				countlines(compileargsource, curpos));
			
			stringappend1(&origin, s);
			
			mmrefdown(s.ptr);
		}
		
		donewithlocatingerror:;
		
		// This variable will contain
		// the final error message string.
		string errormsg = stringnull;
		
		// I append the orgin of the error to errormsg.
		stringappend1(&errormsg, origin);
		
		mmrefdown(origin.ptr);
		
		// I append the error message if not empty.
		if (*msg) {
			stringappend2(&errormsg, msg);
			stringappend4(&errormsg, '\n');
		}
		
		// I now generate the actual errouneous line,
		// and the line that show the error within
		// the errouneous line.
		
		u8* savedcurpos = curpos;
		
		// I find where the errouneous line begin.
		
		// Note that if this function is called
		// when *curpos == '\n', the error is
		// certainly before the new-line character.
		if (*curpos == '\n' && curpos > compileargsource) --curpos;
		
		while (*curpos != '\n' && curpos > compileargsource) --curpos;
		
		if (*curpos == '\n') ++curpos;
		
		// This remove any unecessary spaces before
		// the first character of the line; which yield
		// a better and shorter error message.
		while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
		
		// If the line with the error do not contain
		// any non-blank characters, there is no need
		// to display the erroneous line nor the line
		// showing the location of the error within
		// the erroneous line.
		if (*curpos && *curpos != '\n') {
			
			u8* errorlinestart = curpos;
			
			string errorindicatorline = stringnull;
			
			// I search where the errouneous line end,
			// while generating the error indicator line.
			while (1) {
				
				if (curpos == savedcurpos) stringappend4(&errorindicatorline, '^');
				
				if (*curpos == '\t') stringappend4(&errorindicatorline, '\t');
				else stringappend4(&errorindicatorline, ' ');
				
				if (*curpos == '\n' || !*curpos) break;
				
				++curpos;
			}
			
			// I append the errouneous source
			// code line to the error message.
			stringappend3(&errormsg, errorlinestart, (uint)curpos - (uint)errorlinestart);
			
			stringappend4(&errormsg, '\n');
			
			// I append the error indicator line
			// which will show where on the source code line
			// the error occured by pointing it using
			// the character '^' .
			stringappend1(&errormsg, errorindicatorline);
			
			mmrefdown(errorindicatorline.ptr);
			
			stringappend4(&errormsg, '\n');
		}
		
		compilearg->error(errormsg.ptr);
		
		mmrefdown(errormsg.ptr);
		
	} else compilearg->error(msg);
	
	goto labelforerror;
}


// This function is used to create a lyricalfunction
// linkedlist of general purpose registers.
// The first lyricalreg of the linkedlist should have
// its field id == 1 as it is assumed within funcdeclaration().
void creategpr (lyricalfunction* funcowner) {
	// Allocate the first general purpose register and set r which will be used
	// to create the other general purpose registers in the linkedlist pointed by firstr.
	lyricalreg* firstr = mmallocz(sizeof(lyricalreg));
	lyricalreg* r = firstr;
	
	// I start allocating lyricalregs
	// with ids from 1 and up.
	// The lyricalreg for the register %0
	// is not part of the linkedlist.
	r->id = 1;
	
	r->funcowner = funcowner;
	
	// Keep track of the register id count
	// of the created general purpose register.
	// I initialize it to 1 because I have
	// already created a gpr right above.
	uint i = 1;
	
	while (i < nbrofgpr) {
		
		r->next = mmallocz(sizeof(lyricalreg));
		
		r->next->prev = r;
		
		r = r->next;
		
		++i;
		
		r->id = i;
		
		r->funcowner = funcowner;
	}
	
	// The linkedlist of registers
	// is a circular linkedlist.
	// Having a circular linkedlist
	// allow to quikly retrieve the last
	// register of the linkedlist when
	// I need to set a register at the bottom
	// of the linkedlist for example.
	r->next = firstr;
	firstr->prev = r;
	
	funcowner->gpr = firstr;
}


// This function is used to free a lyricalfunction
// linkedlist of general purpose registers.
void destroygpr (lyricalfunction* funcowner) {
	
	lyricalreg* linkedlist = funcowner->gpr;
	
	lyricalreg* r = linkedlist;
	
	do {
		lyricalreg* savedr = r;
		r = r->next;
		mmrefdown(savedr);
		
	} while (r != linkedlist);
	
	funcowner->gpr = 0;
}


// This function modify the value in curpos. It skip spaces.
void skipspace () {
	// I skip spaces.
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t' || *curpos == '\n') ++curpos;
}


// This function modify the value in curpos.
// It skip spaces, but stop after any newline found.
void skipspacebutstopafternewline () {
	// I skip spaces.
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
	
	curpos += (*curpos == '\n');
}


// This function work just like skipspace()
// but go backward in the source code
// instead of forward, and set curpos after
// the character where skipping stopped.
void reverseskipspace () {
	
	while (curpos > compileargsource) {
		
		--curpos;
		
		if (*curpos != '\x0d' && *curpos != ' ' && *curpos != '\t' && *curpos != '\n') {
			++curpos;
			return;
		}
	}
}


// Enum used by readstringconstant().
typedef enum {
	
	NORMALSTRINGREADING,
	
	// When used, the string cannot contain a new-line,
	// and skipspace() is not used after having read the string.
	STRINGUSEDBYPREPROCESSOR,
	
} readstringconstantflag;

// This function read a constant string
// between double quote and return it.
// curpos must be located at the opening
// double quote character of the constant
// string when this function is called.
// skipspace() is done before returning
// if (flag == NORMALSTRINGREADING).
string readstringconstant (readstringconstantflag flag) {
	
	string s = stringduplicate2("");
	
	u8* savedcurpos = curpos;
	
	readstring:
	
	++curpos; // Set curpos after the opening double quote.
	
	while (*curpos != '"') {
		// I check whether I have an escaped character.
		// Valid escapes are: '\\', '\n', '\t' and '\xx' where xx is an hexadecimal value.
		if (*curpos == '\\') {
			
			++curpos; // Set curpos after the escaping character.
			
			// Check if the escape was being done using a two digits hexadecimal value.
			if (((curpos[0] >= '0' && curpos[0] <= '9') || (curpos[0] >= 'a' && curpos[0] <= 'f')) &&
				((curpos[1] >= '0' && curpos[1] <= '9') || (curpos[1] >= 'a' && curpos[1] <= 'f'))) {
				
				u8 charval = 0;
				
				// Convert the first digit.
				if (curpos[0] >= '0' && curpos[0] <= '9')
					charval = ((charval<<4) + (curpos[0] - '0')); // <<4 do a multiplication by 16.
					
				else if (curpos[0] >= 'a' && curpos[0] <= 'f')
					charval = ((charval<<4) + (curpos[0] - 'a' + 10)); // <<4 do a multiplication by 16.
				
				// Convert the second digit.
				if (curpos[1] >= '0' && curpos[1] <= '9')
					charval = ((charval<<4) + (curpos[1] - '0')); // <<4 do a multiplication by 16.
					
				else if (curpos[1] >= 'a' && curpos[1] <= 'f')
					charval = ((charval<<4) + (curpos[1] - 'a' + 10)); // <<4 do a multiplication by 16.
				
				stringappend4(&s, charval);
				
				curpos += 2; // Set curpos after the two hexadecimal digits of the escape.
				
			} else if (*curpos == 'n') {
				
				stringappend4(&s, '\n');
				
				++curpos; // Set curpos after the character read.
				
			} else if (*curpos == 't') {
				
				stringappend4(&s, '\t');
				
				++curpos; // Set curpos after the character read.
				
			} else if (*curpos == '"') {
				
				stringappend4(&s, '"');
				
				++curpos; // Set curpos after the character read.
				
			} else if (*curpos == '\\') {
				
				stringappend4(&s, '\\');
				
				++curpos; // Set curpos after the character read.
				
			} else throwerror("invalid escape sequence");
			
		} else if (*curpos == '\n') { // Allow for a string to span more than one line.
			
			if (flag == STRINGUSEDBYPREPROCESSOR) {
				
				curpos = savedcurpos;
				
				throwerror("invalid string");
			}
			
			++curpos; // Set curpos after the character read.
			
			// I skip spaces that start the next line
			// onto which the string is spanning.
			skipspace();
			
		} else if (!*curpos) {
			
			curpos = savedcurpos;
			
			throwerror("invalid string");
			
		} else {
			
			stringappend4(&s, *curpos);
			
			++curpos; // Set curpos after the character read.
		}
	}
	
	++curpos; // Set curpos after the closing double quote.
	
	if (flag != STRINGUSEDBYPREPROCESSOR) {
		
		skipspace();
		
		if (*curpos == '"') goto readstring;
	}
	
	// When I get here, the string read
	// is saved in s; I return it.
	return s;
}


// Enum used by skipstringconstant().
typedef enum {
	// When used, skipspace() is called
	// after the string constant has been skipped.
	SKIPSPACEAFTERSTRING,
	
	// When used, skipspace() is not called after
	// the string constant has been skipped.
	DONOTSKIPSPACEAFTERSTRING,
	
} skipstringconstantflag;

// This function skip a constant string
// between double quote.
// curpos must be located at the opening
// double quote character of the constant
// string when this function is called.
void skipstringconstant (skipstringconstantflag flag) {
	
	u8* savedcurpos = curpos;
	
	do {
		++curpos;
		
		if (*curpos == '\\') {
			
			++curpos;
			
			if (*curpos == '"') ++curpos;
		}
		
		if (!*curpos) {
			
			curpos = savedcurpos;
			
			throwerror(stringfmt("internal error: %s: invalid string", __FUNCTION__).ptr);
		}
		
	} while (*curpos != '"');
	
	++curpos; // Set curpos after the closing double quote.
	
	if (flag != DONOTSKIPSPACEAFTERSTRING) skipspace();
}


// Enum used by skipcharconstant().
typedef enum {
	// When used, skipspace() is called
	// after the string constant has been skipped.
	SKIPSPACEAFTERCHARCONST,
	
	// When used, skipspace() is not called after
	// the string constant has been skipped.
	DONOTSKIPSPACEAFTERCHARCONST,
	
} skipcharconstantflag;

// This function skip a character
// or multicharacter constant
// between single quote.
// curpos must be located at the opening
// single quote character of the character
// or multicharacter constant when
// this function is called.
void skipcharconstant (skipcharconstantflag flag) {
	
	u8* savedcurpos = curpos;
	
	do {
		++curpos;
		
		if (*curpos == '\\') {
			
			++curpos;
			
			if (*curpos == '\'') ++curpos;
		}
		
		if (!*curpos) {
			
			curpos = savedcurpos;
			
			throwerror(stringfmt("internal error: %s: invalid character/multicharacter constant", __FUNCTION__).ptr);
		}
		
	} while (*curpos != '\'');
	
	++curpos; // Set curpos after the closing single quote.
	
	if (flag != DONOTSKIPSPACEAFTERCHARCONST) skipspace();
}


// Enum used by readsymbol().
typedef enum {
	// When used, readsymbol() parse a symbol that
	// use only lower case characters and digits.
	LOWERCASESYMBOL = 1,
	
	// When used, readsymbol() parse a symbol that
	// use only upper case characters and digits.
	UPPERCASESYMBOL = 1<<1,
	
	// When used, skipspace() is not called after
	// the symbol has been parsed.
	DONOTSKIPSPACEAFTERSYMBOL = 1<<2,
	
	// The above enum can be ored together to
	// allow readsymbol to exclusively either
	// parse upper or lower case characters.
	
} readsymbolflag;

// This function is used to read a variable,
// function or type name symbol.
// A symbol is made up of alpha-numeric character
// and cannot start with a number.
// This function modify the value in curpos.
string readsymbol (readsymbolflag flag) {
	
	string s = stringnull;
	
	// A symbol cannot start by a numeric character.
	
	if (flag&LOWERCASESYMBOL && *curpos >= 'a' && *curpos <= 'z') {
		
		do {
			stringappend4(&s, *curpos);
			++curpos;
			
		} while ((*curpos >= '0' && *curpos <= '9') || (*curpos >= 'a' && *curpos <= 'z'));
		
	} else if (flag&UPPERCASESYMBOL && *curpos >= 'A' && *curpos <= 'Z') {
		
		do {
			stringappend4(&s, *curpos);
			++curpos;
			
		} while ((*curpos >= '0' && *curpos <= '9') || (*curpos >= 'A' && *curpos <= 'Z'));
	}
	
	if (s.ptr) {
		// This check insure that
		// the symbol exclusively
		// use either uppper or
		// lower case characters.
		if ((*curpos >= 'a' && *curpos <= 'z') || (*curpos >= 'A' && *curpos <= 'Z'))
			throwerror("symbol characters must be exclusively uppercase or lowercase");
		else if (!(flag&DONOTSKIPSPACEAFTERSYMBOL)) skipspace();
	}
	
	// s is null if no valid
	// symbol was read.
	return s;
}


// This function free the linkedlist
// of chunk given as argument.
// Note that this function do not attempt
// to free the field chunk->content.ptr as it has
// already been freed by postpreprocessing().
void freechunklinkedlist (chunk* linkedlist) {
	
	chunk* firstelement = linkedlist;
	
	do {
		u8* ptr = linkedlist->origin.ptr;
		
		if (ptr) mmrefdown(ptr);
		
		ptr = linkedlist->path.ptr;
		
		if (ptr) mmrefdown(ptr);
		
		chunk* c = linkedlist;
		
		linkedlist = linkedlist->next;
		
		mmrefdown(c);
		
	} while (linkedlist != firstelement);
}


// This function is used to free
// a linkedlist of lyricaltype.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freetypelinkedlist (lyricaltype* linkedlist) {
	
	lyricaltype* t = linkedlist;
	
	do {
		mmrefdown(t->name.ptr);
		
		if (t->scope) mmrefdown(t->scope);
		
		if (t->v) freevarlinkedlist(t->v);
		
		lyricaltype* savedt = t;
		t = t->next;
		mmrefdown(savedt);
		
	} while (t != linkedlist);
}


// This function is used to free
// a linkedlist of struct lyricalargumentflag.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freeargumentflaglinkedlist (lyricalargumentflag* linkedlist) {
	
	lyricalargumentflag* a = linkedlist;
	
	do {
		lyricalargumentflag* saveda = a;
		a = a->next;
		mmrefdown(saveda);
		
	} while (a != linkedlist);
}


// This function is used to free a linkedlist
// of propagation elements.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freepropagationlinkedlist (lyricalpropagation* linkedlist) {
	
	do {
		lyricalpropagation* savedptr = linkedlist;
		
		linkedlist = linkedlist->next;
		
		mmrefdown(savedptr);
		
	} while (linkedlist);
}


// This function is used to free
// a linkedlist of lyricalsharedregion.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freesharedregionlinkedlist (lyricalsharedregion* linkedlist) {
	
	do {
		lyricalsharedregion* savedptr = linkedlist;
		
		linkedlist = linkedlist->next;
		
		// This function is used to free
		// a linkedlist of lyricalsharedregionelement.
		// The linkedlist should be valid because
		// there is no check on whether it is valid.
		void freesharedregionelementlinkedlist (lyricalsharedregionelement* linkedlist) {
			
			do {
				lyricalsharedregionelement* savedptr = linkedlist;
				
				linkedlist = linkedlist->next;
				
				mmrefdown(savedptr);
				
			} while (linkedlist);
		}
		
		// savedptr->e will always be non-null.
		freesharedregionelementlinkedlist(savedptr->e);
		
		mmrefdown(savedptr);
		
	} while (linkedlist);
}


// This function is used to free a linkedlist
// of lyricalcachedstackframe elements.
// The linkedlist should be valid because
// there is no check on whether it is valid.
void freestackframelinkedlist (lyricalcachedstackframe* linkedlist) {
	
	do {
		lyricalcachedstackframe* savedptr = linkedlist;
		
		linkedlist = linkedlist->next;
		
		mmrefdown(savedptr);
		
	} while (linkedlist);
}

// This function is very similar to
// the function newinstruction() defined
// in opcodes.tools.parsestatement.lyrical.c,
// except it only create LYRICALOPCOMMENT.
// It is not possible to use newinstruction()
// within lyricalcompile() because
// it is local to parsestatement().
lyricalinstruction* comment (lyricalfunction* f, string s) {
	// I allocate memory for a new lyricalinstruction.
	lyricalinstruction* i = mmallocz(sizeof(lyricalinstruction));
	
	i->op = LYRICALOPCOMMENT;
	
	i->comment = s;
	
	// I attach the newly
	// created lyricalinstruction
	// to the circular linkedlist.
	// Note that "next" is used with
	// the argument FIELDPREV of the macro
	// LINKEDLISTCIRCULARADDTOTOP()
	// because the ordering of the circular
	// linkedlist pointed by f->i is reversed,
	// whereby the linkedlist top is bottom,
	// and the linkedlist bottom is top.
	LINKEDLISTCIRCULARADDTOTOP(next, prev, i, f->i);
	
	return i;
}

#endif


// Functions used only within lyricalfree().
#ifdef LYRICALFREE

#endif
