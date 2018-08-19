
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


// Variable used only when defining
// a function-like macro, and used to save
// the location of the preprocessor
// directive `define or `locdef .
u8* savedcurpos2 = curpos -6;

while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

u8* savedcurpos = curpos;

string s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL);

if (!s.ptr) {
	
	curpos = savedcurpos;
	
	throwerror("expecting a valid macro name");
}

if (stringiseq2(s, "FILE") || stringiseq2(s, "LINE")) {
	
	curpos = savedcurpos;
	
	throwerror("reserved macro name");
}

macro* m;

if (m = checkifmacroexist(s)) {
	
	curpos = savedcurpos;
	
	s = stringduplicate2("macro was already declared at ");
	
	stringappend1(&s, m->origin);
	
	throwerror(s.ptr);
	
	// The string pointed by s.ptr will be freed
	// when throwerror() throw labelforerror.
}

// I allocate memory for the macro to be defined.
m = mmalloc(sizeof(macro));

// If `locdef was used instead of `define,
// the macro definition is to be visible only
// within the current file and its includes.
m->islocal = (*savedcurpos2 == 'l');

m->chunklocationtsetwhenused = 0;

m->name = s;

m->origin = stringfmt("%s:%d", currentfilepath.ptr, countlines(compileargsource, savedcurpos));

m->cannotbeundefined = 0;

m->args = (macro**)0;

// Pointer to an array of pointers to u8*
// that will be used to save the locations
// where each macro argument was declared.
u8** savedcurposformacroarg = 0;

uint nbrofargs;

if (*curpos == '(') {
	// I get here if I am defining a function-like macro.
	
	nbrofargs = 0;
	
	do {
		++curpos;
		
		savedcurpos = curpos;
		
		while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
		
		++nbrofargs;
		
		// I save the location where the macro argument was declared.
		savedcurposformacroarg = mmrealloc(savedcurposformacroarg, nbrofargs * sizeof(u8*));
		savedcurposformacroarg[nbrofargs-1] = curpos;
		
		s = readsymbol(UPPERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL);
		
		if (s.ptr) {
			
			if (stringiseq1(s, m->name)) {
				curpos = savedcurpos;
				while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
				throwerror("macro argument name has the same name as its owner");
			}
			
			macro* ma = checkifmacroexist(s);
			
			if (ma) {
				curpos = savedcurpos;
				
				while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
				
				s = stringduplicate2("macro argument name was already used to declare a macro at ");
				
				stringappend1(&s, ma->origin);
				
				throwerror(s.ptr);
				
				// The string pointed by s.ptr will be freed
				// when throwerror() throw labelforerror.
			}
			
		} else {
			
			curpos = savedcurpos;
			throwerror("expecting a macro argument name that do not use lowercase characters");
		}
		
		if (stringiseq2(s, "FILE") || stringiseq2(s, "LINE")) {
			
			curpos = savedcurpos;
			
			while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
			
			throwerror("reserved macro name");
		}
		
		if (m->args) {
			
			uint i = nbrofargs -1;
			
			while (i) {
				
				--i;
				
				if (stringiseq1(m->args[i]->name, s)) {
					curpos = savedcurpos;
					throwerror("macro argument name already used");
				}
			}
		}
		
		// Increase the array that contain pointers
		// to macro that are the arguments of
		// the macro being defined.
		m->args = mmrealloc(m->args, nbrofargs * sizeof(macro*));
		
		macro* macroarg = mmallocz(sizeof(macro));
		
		// The origin of a macro argument is the same
		// as the macro for which it is the argument.
		macroarg->origin = stringduplicate1(m->origin);
		
		macroarg->name = s;
		
		macroarg->cannotbeundefined = 1;
		
		// I create a single chunk for which
		// the content is the same as the name
		// of the macro argument to which
		// it will belong.
		// That chunk never make it to the final
		// linkedlist used by postpreprocessing(), as
		// it is substituted within preprocessor() when
		// the name of the function-like macro is parsed.
		macroarg->chunks = mmallocz(sizeof(chunk));
		macroarg->chunks->content = stringduplicate1(s);
		macroarg->chunks->prev = macroarg->chunks;
		macroarg->chunks->next = macroarg->chunks;
		
		m->args[nbrofargs-1] = macroarg;
		
		while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
		
	} while (*curpos == ',');
	
	if (*curpos != ')') {
		curpos = savedcurpos;
		throwerror("expecting ',' or ')'");
	}
	
	do ++curpos; while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t');
	
	if (*curpos != '\n') {
		reverseskipspace();
		throwerror("expecting newline");
	}
	
	++curpos;
	
	m->nbrofargs = nbrofargs;
	
} else {
	// I get here if I am defining an object-like macro.
	
	u8* c = m->name.ptr;
	
	// I check whether lowercase
	// characters were used in the name
	// of the object-like macro.
	// It is enough to check only
	// the first character.
	if (*c >= 'a' && *c <= 'z') {
		
		curpos = savedcurpos;
		
		throwerror("expecting an object-like macro name that do not use lowercase characters");
	}
	
	m->nbrofargs = 0;
	
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
}

// I save the current value of chunkorigin as it will
// be modified below before parsing the macro definition.
string savedchunkorigin = chunkorigin;

// I set chunkorigin with the string that will
// be used by createchunk() to set the field
// origin of struct chunk created while parsing
// the macro definition.
chunkorigin = stringduplicate2("from macro \"");
stringappend1(&chunkorigin, m->name);
stringappend4(&chunkorigin, '"');

m->isbeingdefined = 1;

// I add the newly defined macro to the top
// of the linkedlist of macro pointed by macros.
LINKEDLISTCIRCULARADDTOTOP(prev, next, m, macros);

if (m->args) {
	
	macro* macroarg;
	
	uint i = nbrofargs;
	
	// Whithin this loop, I attach all the macro arguments
	// of the function-like macro pointed by the variable m
	// to the linkedlist of macro pointed by
	// the variable macros.
	do {
		--i;
		
		macroarg = m->args[i];
		
		LINKEDLISTCIRCULARADDTOTOP(prev, next, macroarg, macros);
		
	} while (i);
	
	// The recursive call to preprocessor() done below
	// will return when encountering the preprocessor
	// directive: `enddef; it will also return when reaching
	// an unexpected end of file.
	m->chunks = preprocessor(PREPROCESSFUNCTIONLIKEMACRO);
	
	if (*curpos) {
		// Set curpos right after the directive `enddef.
		curpos += 6;
		
	} else {
		// Set curpos to the location where
		// the directive `define was used.
		curpos = savedcurpos2;
		throwerror("corresponding `enddef could not be found");
	}
	
	// Now that I am done parsing the definition of
	// the function-like macro, I now detach its
	// macro arguments from the linkedlist pointed
	// by the variable macros.
	
	macroarg = m->args[0];
	
	do {
		--nbrofargs;
		
		// If the macro argument did not get used
		// within the function-like macro definition,
		// throw an error.
		if (!macroarg->wasused) {
			curpos = savedcurposformacroarg[nbrofargs];
			throwerror("unused macro argument");
		}
		
		// I free the single chunk that was set in
		// the field chunks of the macro argument
		// as well as its content.
		mmrefdown(macroarg->chunks->content.ptr);
		mmrefdown(macroarg->chunks);
		
		macroarg = macroarg->next;
		
	} while (nbrofargs);
	
	// I free the array that was created to save
	// the location where macro arguments were declared.
	mmrefdown(savedcurposformacroarg);
	
	macro* e = m->args[0];
	
	if (macroarg == e) macros = 0;
	else {
		if (macros == e) macros = macroarg;
		
		e = e->prev;
		
		e->next = macroarg;
		macroarg->prev = e;
	}
	
} else m->chunks = preprocessor(PREPROCESSOBJECTLIKEMACRO);

m->isbeingdefined = 0;

mmrefdown(chunkorigin.ptr);

// I restore the value of chunkorigin.
chunkorigin = savedchunkorigin;
