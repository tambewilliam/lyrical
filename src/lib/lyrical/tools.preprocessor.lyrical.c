
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


// This function is used by the preprocessor
// to check whether I have the directive pointed
// by the argument s, at the current position
// pointed by curpos.
// If true, curpos is set after the directive,
// and 1 is returned, otherwise curpos is left
// untouched and 0 is returned.
// A preprocessor directive is made up of lowercase
// characters and do not use numbers.
// The string pointed by the argument s must
// contain at least a single character.
uint checkfordirective (u8* s) {
	// I save curpos so I can restore it
	// if I didn't match the directive.
	u8* savedcurpos = curpos;
	
	while (*curpos == *s) {
		
		++curpos;
		
		++s;
		
		if (!*s) {
			// I get here if I am done checking all the characters
			// of the preprocessor directive pointed by the argument s;
			// Here I make sure that the character that come next
			// in *curpos is not part of the preprocessor directive
			// otherwise it would mean that what is pointed by
			// the argument s do not match what was initially
			// pointed by curpos.
			if (*curpos < 'a' || *curpos > 'z') return 1;
			
			curpos = savedcurpos;
			
			return 0;
		}
	}
	
	curpos = savedcurpos;
	
	return 0;
}


// Function is used to skip code within a conditional
// preprocessor block until either one of the directives
// `else, `elif, `elifdef, `elifndef, `endif is parsed;
// it will also return when reaching an unexpected end of file.
void skipconditionalblock () {
	// Keep track of the depth of
	// nested conditional blocks.
	uint conditionalblockdepth = 0;
	
	// This loop is similar to the
	// loop found in preprocessor().
	while (1) {
		
		if (*curpos == '#') {
			
			do ++curpos; while (*curpos && *curpos != '\n');
			
		} else if (*curpos == '`') {
			
			++curpos;
			
			if (checkfordirective("if") ||
				checkfordirective("ifdef") ||
				checkfordirective("ifndef")) {
				
				++conditionalblockdepth;
				
			} else if (checkfordirective("else") ||
				checkfordirective("elif") ||
				checkfordirective("elifdef") ||
				checkfordirective("elifndef")) {
				
				if (!conditionalblockdepth) {
					// I search where the directive start.
					do --curpos; while (*curpos != '`');
					
					++curpos;
					
					return;
				}
				
			} else if (checkfordirective("endif")) {
				
				if (!conditionalblockdepth) {
					// This will set curpos at the beginning
					// of the string "endif".
					curpos -= 5;
					
					return;
				}
				
				--conditionalblockdepth;
			}
			
		} else if (*curpos == '"') skipstringconstant(DONOTSKIPSPACEAFTERSTRING);
		else if (*curpos == '\'') skipcharconstant(DONOTSKIPSPACEAFTERCHARCONST);
		else if (!*curpos) {
			// If I get here, I have
			// an unexpected end of file.
			return;
			
		} else ++curpos;
	}
}


// Create a chunk and add it to
// the linkedlist pointed by chunks.
void createchunk () {
	
	chunk* c = mmallocz(sizeof(chunk));
	
	c->origin = stringduplicate1(chunkorigin);
	
	c->path = stringduplicate1(currentfilepath);
	
	c->offset = (uint)startofchunk - (uint)compileargsource;
	
	c->linenumber = countlines(compileargsource, startofchunk);
	
	c->content = stringduplicate3(startofchunk, ((uint)endofchunk - (uint)startofchunk) +1);
	
	LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, c, chunks);
}


// This function attach the linkedlist
// of chunk given as argument to
// the linkedlist pointed by chunks.
void attachchunks (chunk* linkedlist) {
	
	if (chunks) {
		
		chunk* savedchunkprev = chunks->prev;
		
		savedchunkprev->next = linkedlist;
		
		chunks->prev = linkedlist->prev;
		
		linkedlist->prev->next = chunks;
		linkedlist->prev = savedchunkprev;
		
	} else chunks = linkedlist;
}


// This function is used to duplicate
// a single macro chunk into
// the linkedlist pointed by chunks.
void duplicatemacrochunk (chunk* element) {
	
	chunk* c = mmallocz(sizeof(chunk));
	
	c->origin = stringduplicate1(element->origin);
	
	c->path = stringduplicate1(element->path);
	
	c->offset = element->offset;
	
	c->linenumber = element->linenumber;
	
	c->content = stringduplicate1(element->content);
	
	if (chunks) {
		
		c->next = chunks;
		c->prev = chunks->prev;
		chunks->prev->next = c;
		chunks->prev = c;
		
	} else {
		
		c->next = c;
		c->prev = c;
		chunks = c;
	}
}


// This function is used to duplicate
// the linkedlist of chunk of a macro into
// the linkedlist pointed by chunks.
void duplicatemacrochunks (chunk* linkedlist) {
	
	chunk* firstelement = linkedlist;
	
	do {
		chunk* c = mmallocz(sizeof(chunk));
		
		c->origin = stringduplicate1(linkedlist->origin);
		
		c->path = stringduplicate1(linkedlist->path);
		
		c->offset = linkedlist->offset;
		
		c->linenumber = linkedlist->linenumber;
		
		c->content = stringduplicate1(linkedlist->content);
		
		if (chunks) {
			
			c->next = chunks;
			c->prev = chunks->prev;
			chunks->prev->next = c;
			chunks->prev = c;
			
		} else {
			
			c->next = c;
			c->prev = c;
			chunks = c;
		}
		
	} while ((linkedlist = linkedlist->next) != firstelement);
}


// This function check wehther a macro exist
// and return a pointer to it, otherwise return null.
macro* checkifmacroexist (string name) {
	
	if (!macros) return 0;
	
	macro* m = macros;
	
	do {
		if (stringiseq1(m->name, name)) return m;
		
	} while ((m = m->next) != macros);
	
	return 0;
}


// This function is similar to freechunklinkedlist()
// but is used exclusively with macro->chunks where
// the field macro->chunks->content.ptr must be freed.
void freemacrochunklinkedlist (chunk* linkedlist) {
	
	chunk* firstelement = linkedlist;
	
	do {
		u8* ptr = linkedlist->origin.ptr;
		
		if (ptr) mmrefdown(ptr);
		
		ptr = linkedlist->path.ptr;
		
		if (ptr) mmrefdown(ptr);
		
		ptr = linkedlist->content.ptr;
		
		if (ptr) mmrefdown(ptr);
		
		chunk* c = linkedlist;
		
		linkedlist = linkedlist->next;
		
		mmrefdown(c);
		
	} while (linkedlist != firstelement);
}


// This function free the macro given as argument.
// The macro must belong to the linkedlist
// pointed by the variable macros.
void freemacro (macro* m) {
	
	mmrefdown(m->origin.ptr);
	
	mmrefdown(m->name.ptr);
	
	if (m->chunks) freemacrochunklinkedlist(m->chunks);
	
	uint i = m->nbrofargs;
	
	if (i) {
		
		do {
			--i;
			
			macro* macroarg = m->args[i];
			
			mmrefdown(macroarg->origin.ptr);
			
			mmrefdown(macroarg->name.ptr);
			
			// The field m->args[i]->chunks would
			// certainly not be populated with chunk.
			
			mmrefdown(macroarg);
			
		} while (i);
		
		mmrefdown(m->args);
	}
	
	LINKEDLISTCIRCULARREMOVE(prev, next, m, macros);
	
	mmrefdown(m);
}


// This function free the linkedlist
// of macro pointed by macros.
// The variable macros must be non-null
// when this function is called.
void freemacros () {
	
	macro* m = macros;
	
	do {
		mmrefdown(m->origin.ptr);
		
		mmrefdown(m->name.ptr);
		
		if (m->chunks) freemacrochunklinkedlist(m->chunks);
		
		uint i = m->nbrofargs;
		
		if (i) {
			
			do {
				--i;
				
				macro* macroarg = m->args[i];
				
				mmrefdown(macroarg->origin.ptr);
				
				mmrefdown(macroarg->name.ptr);
				
				// The field m->args[i]->chunks would
				// certainly not be populated with chunk.
				
				mmrefdown(macroarg);
				
			} while (i);
			
			mmrefdown(m->args);
		}
		
		macro* msaved = m;
		
		m = m->next;
		
		mmrefdown(msaved);
		
	} while (m != macros);
}
