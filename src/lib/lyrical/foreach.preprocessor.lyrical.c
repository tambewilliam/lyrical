
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


u8* savedcurpos = curpos;

while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

string s = readsymbol(UPPERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL);

if (!s.ptr) {
	curpos = savedcurpos;
	throwerror("expecting a macro name that do not use lowercase characters");
}

if (stringiseq2(s, "FILE") || stringiseq2(s, "LINE")) {
	
	curpos = savedcurpos;
	
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
	
	throwerror("reserved macro name");
}

macro* m;

if (m = checkifmacroexist(s)) {
	
	curpos = savedcurpos;
	
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
	
	throwerror(stringfmt("macro was already declared at %s", m->origin).ptr);
}

// I allocate memory for the loop macro.
m = mmalloc(sizeof(macro));

m->chunklocationtsetwhenused = 1;

m->isbeingdefined = 0;

m->name = s;

m->origin = stringfmt("%s:%d", currentfilepath.ptr, countlines(compileargsource, savedcurpos));

m->cannotbeundefined = 1;

m->args = (macro**)0;
m->nbrofargs = 0;

// I create a single chunk for which
// the content will be updated at each
// iteration of the loop.

m->chunks = mmallocz(sizeof(chunk));

// The fields origin, path and linenumber
// will be set on the duplicate of this chunk,
// when the macro owning this chunk is retrieved
// within preprocessor().

// The field content is set at
// the beginning of each iteration.

m->chunks->prev = m->chunks;
m->chunks->next = m->chunks;

// I save the location of the directive `foreach.
u8* savedcurpos2 = savedcurpos -7;

// I parse the loop parameters which are
// one or more double quoted strings.

// Array that will contain pointers to string
// that are the parameters of the loop.
arrayuint loopparameters = arrayuintnull;

while (1) {
	
	savedcurpos = curpos;
	
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
	
	if (*curpos == '\n') {
		// At least one double quoted string must be parsed.
		if (!loopparameters.ptr) {
			curpos = savedcurpos;
			throwerror("expecting a double quoted string");
		}
		
		++curpos;
		
		break;
		
	} else if (*curpos != '\"') {
		
		curpos = savedcurpos;
		throwerror("expecting a double quoted string or a newline");
	}
	
	s = readstringconstant(STRINGUSEDBYPREPROCESSOR);
	
	if (!s.ptr) {
		curpos = savedcurpos;
		throwerror("empty string");
	}
	
	*arrayuintappend1(&loopparameters) = (uint)s.ptr;
}

// I add the loop macro to the top of
// the linkedlist of macro pointed by macros.
LINKEDLISTCIRCULARADDTOTOP(prev, next, m, macros);

// I iteratively parse the body of the loop.

// I save the location where the body of the loop start.
savedcurpos = curpos;

uint loopparameterssz = arrayuintsz(loopparameters);

uint i = 0;

while (1) {
	
	m->chunks->content.ptr = (u8*)loopparameters.ptr[i];
	
	// The recursive call to preprocessor() done below
	// will return when encountering the preprocessor
	// directive: `endfor; it will also return
	// when reaching an unexpected end of file.
	chunk* c = preprocessor(PREPROCESSFOREACHBLOCK);
	
	if (!*curpos) {
		// Set curpos to the location where
		// the directive `foreach was used.
		curpos = savedcurpos2;
		throwerror("corresponding `endfor could not be found");
	}
	
	// I attach the returned linkedlist to
	// the linkedlist pointed by chunks.
	if (c) attachchunks(c);
	
	if (++i < loopparameterssz) curpos = savedcurpos;
	else break;
}

// I throw an error,
// if the for-loop macro
// did not get used.
if (!m->wasused) {
	curpos = savedcurpos2 +7;
	while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
	throwerror("unused for-loop macro");
}

// This will free the array
// as well as the memory used by
// the strings accumulated
// in the array.
arrayuintfree(loopparameters, (void(*)(uint))mmrefdown);

// Set m->chunks->content.ptr to null
// to prevent freechunklinkedlist()
// from invoking mmrefdown() on it.
m->chunks->content.ptr = 0;

// Free the loop macro and remove it from
// the linkedlist pointed by macros.
freemacro(m);

// The recursive call to preprocessor() done above
// would have set curpos at the beginning of the string
// for the preprocessor directive `endfor.
// Here I set curpos right after the directive `endfor.
curpos += 6;
