
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


string initialcwd = filegetcwd();
stringappend4(&initialcwd, '/');

// Path to the directory holding
// the file currently being processed.
string cwd = stringduplicate1(initialcwd);

currentfilepath = stringduplicate2("");

// Array used to store the list of already
// included modules in order to prevent
// inclusion of a module more than once.
arrayuint alreadyincludedmodules = arrayuintnull;

// This variable point to the linkedlist
// of chunk for included modules.
// Their contents is to be at the top
// of the postpreprocessing() result.
chunk* moduleschunks = 0;

// This function attach the linkedlist
// of chunk given as argument to
// the linkedlist pointed by moduleschunks.
void attachmodulechunks (chunk* linkedlist) {
	
	if (moduleschunks) {
		
		chunk* savedchunkprev = moduleschunks->prev;
		
		savedchunkprev->next = linkedlist;
		
		moduleschunks->prev = linkedlist->prev;
		
		linkedlist->prev->next = moduleschunks;
		linkedlist->prev = savedchunkprev;
		
	} else moduleschunks = linkedlist;
}

curpos = compileargsource;

// Enum used by preprocessor().
typedef enum {
	// Used with the first call of preprocessor()
	// that is not a recursive call.
	PREPROCESSBEGIN,
	
	// Used when recursively calling preprocessor()
	// to parse an included file.
	PREPROCESSINCLUDE,
	
	// Used when recursively calling preprocessor()
	// to parse an included .lyx file.
	PREPROCESSINCLUDELYX,
	
	// Used with a recursive call of preprocessor() used
	// to parse code within conditional preprocessor directives.
	PREPROCESSCONDITIONALBLOCK,
	
	// Used with a recursive call of preprocessor() used
	// to parse code within a foreach preprocessor block.
	PREPROCESSFOREACHBLOCK,
	
	// When used, preprocessor() return after
	// parsing the character ')' or ',' if there was
	// no previously '(' that had not been closed
	// by a matching ')' .
	PREPROCESSMACROARGUMENT,
	
	// Used with a recursive call of preprocessor() used
	// to parse the definition of an object-like macro.
	PREPROCESSOBJECTLIKEMACRO,
	
	// Used with a recursive call of preprocessor() used
	// to parse the definition of a function-like macro.
	PREPROCESSFUNCTIONLIKEMACRO,
	
} preprocessoraction;

// This function recursively preprocess
// the source code using the variable curpos.
chunk* preprocessor (preprocessoraction action) {
	// This structure represent a macro.
	typedef struct macro {
		// This field is set for a macro created from
		// a lyricalpredeclaredmacro or for a macro
		// created when the directive `foreach is used.
		// When set, it mean that the fields chunks->origin,
		// chunks->path, chunks->offset and chunks->linenumber
		// are determined when the name of this macro
		// is parsed and its single chunk duplicated.
		uint chunklocationtsetwhenused;
		
		// This field is set to 1 while the macro is
		// being defined otherwise it is set to null.
		uint isbeingdefined;
		
		// This field is set to 1 for a macro
		// that was defined using the preprocessor
		// directive `locdef instead of `define .
		uint islocal;
		
		// Set to 1 if the macro was used
		// at least once, otherwise it is null.
		// This field is used only with macro that
		// are arguments of a function-like macro,
		// and with for-loop macro.
		uint wasused;
		
		// This field is used so as to be able
		// to include in an error message the location
		// where the macro was defined.
		// For a macro that was not created from
		// a lyricalpredeclaredmacro, this field
		// is set to a string that is the filepath
		// and linenumber where the macro was defined. ei:
		// filepath.lyc:123.
		// For a macro that was created from an
		// lyricalpredeclaredmacro, this field is set
		// to the string: "creation of predeclared macros".
		string origin;
		
		// Name of the macro.
		string name;
		
		// When set, the macro cannot be undefined
		// by the preprocessor directive `undef.
		uint cannotbeundefined;
		
		// Content of the macro which is
		// a circular linkedlist of chunk.
		chunk* chunks;
		
		// Pointer to an array of macro
		// that are the arguments of this macro.
		// This field is only set for
		// function-like macro.
		struct macro** args;
		
		// Number of arguments used
		// by this macro.
		// This field is only set for
		// function-like macro.
		uint nbrofargs;
		
		// The field prev of the first element of
		// the linkedlist point to the last element
		// of the linkedlist while the field next
		// of the last element of the linkedlist
		// point to the first element of
		// the linkedlist.
		struct macro* prev;
		struct macro* next;
		
	} macro;
	
	// This variable point to the circular
	// linkedlist of macro.
	static macro* macros = 0;
	
	if (action == PREPROCESSBEGIN && compilearg->predeclaredmacros) {
		// If I get here, I create the predeclared macros.
		
		uint i = 0;
		
		while (compilearg->predeclaredmacros[i].name) {
			
			macro* m = mmalloc(sizeof(macro));
			
			m->chunklocationtsetwhenused = 1;
			
			m->isbeingdefined = 0;
			
			// The field m->wasused do not need to be set.
			
			m->origin = stringduplicate2("creation of predeclared macros");
			
			m->name = stringduplicate2(compilearg->predeclaredmacros[i].name);
			
			m->cannotbeundefined = 1;
			
			m->args = (macro**)0;
			m->nbrofargs = 0;
			
			// I create a single chunk for the macro.
			
			m->chunks = mmallocz(sizeof(chunk));
			
			// The fields origin, path and linenumber
			// will be set on the duplicate of this chunk,
			// when the macro owning this chunk is retrieved
			// within preprocessor().
			
			m->chunks->content = stringduplicate2(compilearg->predeclaredmacros[i].content);
			
			m->chunks->prev = m->chunks;
			m->chunks->next = m->chunks;
			
			// I add the newly defined macro to the top of
			// the linkedlist of macro pointed by macros.
			LINKEDLISTCIRCULARADDTOTOP(prev, next, m, macros);
			
			++i;
		}
	}
	
	// This variable point to the first element
	// of the linkedlist of chunk generated
	// within the instance of this function.
	chunk* chunks = 0;
	
	// This variable save the string that is used
	// by createchunk() to set the field origin
	// of the struct chunk.
	static string chunkorigin = stringnull;
	
	// Variables used to save the location where
	// a chunk to create start and end in the file
	// being preprocessed.
	u8* startofchunk;
	u8* endofchunk;
	
	#include "tools.preprocessor.lyrical.c"
	
	// This variable keep track of the number
	// of opened paranthesis when using
	// the action PREPROCESSMACROARGUMENT.
	uint openedparanthesiscount = 0;
	
	// This variable keey track of whether
	// within a code block <% %> when
	// action == PREPROCESSINCLUDELYX.
	uint withinlyxcodeblock = 0;
	
	startofchunk = curpos;
	
	// This loop is similar to the loop
	// found in skipconditionalblock().
	while (1) {
		
		endofchunk = curpos -1;
		
		if (!chunks && (action == PREPROCESSINCLUDE || action == PREPROCESSINCLUDELYX)) {
			// I get here only when including a file.
			// When chunks is null, it mean that
			// the file being included is just getting parsed.
			// I create an empty chunk which for an .lyx file
			// will contain the opening double quote for any text
			// before the first opening code block; for an .lyc file
			// will contain the character sequence opening a scope
			// to infer exporting of functions/operators definitions.
			
			createchunk();
			
			if (action == PREPROCESSINCLUDELYX)
				stringappend4(&chunks->prev->content, '"');
		}
		
		u8* savedcurpos = curpos;
		
		string s;
		
		if (*curpos && action == PREPROCESSINCLUDELYX && !withinlyxcodeblock) {
			
			if ((curpos[0] == '<' && curpos[1] == '%') || curpos[0] == '$') {
				
				s = chunks->prev->content;
				
				uint ssz = stringmmsz(s);
				
				if (s.ptr[ssz-1] != '"' || (ssz > 1 && s.ptr[ssz-2] == '\\')) {
					// If startofchunk <= endofchunk generate a chunk
					// for the text before the opening code block.
					if (startofchunk <= endofchunk) createchunk();
					
					stringappend4(&chunks->prev->content, '"');
					
					if (compilearglyxappend) stringappend2(&chunks->prev->content, compilearglyxappend);
					
				} else {
					// I prevent generating an empty string,
					// chopping-off the opening double quote
					// from the end of the previous chunk.
					stringchop(&chunks->prev->content, 1);
				}
				
				if (*curpos == '$') {
					
					++curpos;
					
					if (*curpos == '{') {
						
						++curpos;
						
						skipspace();
						
						if ((s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL)).ptr) {
							
							stringappend1(&chunks->prev->content, s);
							
							mmrefdown(s.ptr);
							
						} else {
							curpos = savedcurpos+1;
							throwerror("expecting a symbol");
						}
						
						if (*curpos != '}') {
							reverseskipspace();
							throwerror("expecting '}'");
						}
						
						++curpos;
						
					} else if ((s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL)).ptr) {
						
						stringappend1(&chunks->prev->content, s);
						
						mmrefdown(s.ptr);
						
					} else {
						curpos = savedcurpos+1;
						throwerror("expecting a symbol");
					}
					
					if (compilearglyxappend) stringappend2(&chunks->prev->content, compilearglyxappend);
					
				} else {
					
					curpos += 2;
					
					withinlyxcodeblock = 1;
				}
				
				startofchunk = curpos;
				
			} else if (curpos[0] == '<' && curpos[1] == '!') {
				// Skip xml comment block
				// within "<!" and "->" .
				
				curpos += 2;
				
				do {
					if (!*curpos) {
						curpos = savedcurpos;
						throwerror("invalid xml comment");
					}
					
					++curpos;
					
				} while (curpos[0] != '-' || curpos[1] != '>');
				
				// Set curpos after the comment block
				// closing character sequence "->" .
				curpos += 2;
				
			} else if (*curpos == '\n') {
				// I replace the newline character
				// by its corresponding escape sequence
				// otherwise spaces following it would
				// be skipped and not printed out.
				
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the newline character.
				if (startofchunk <= endofchunk) createchunk();
				
				// Append the escape sequence that is
				// to replace the newline character.
				stringappend2(&chunks->prev->content, "\\n");
				
				// Skip the newline character since it is
				// being replaced by its escape sequence.
				++curpos;
				
				startofchunk = curpos;
				
			} else if (*curpos == '"') {
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the double quote character.
				if (startofchunk <= endofchunk) createchunk();
				
				// Escape the opening double quote character.
				stringappend4(&chunks->prev->content, '\\');
				
				startofchunk = curpos;
				
				skipstringconstant(DONOTSKIPSPACEAFTERSTRING);
				
				endofchunk = curpos -2;
				
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the double quote character.
				if (startofchunk <= endofchunk) createchunk();
				
				// Escape the closing double quote character.
				stringappend4(&chunks->prev->content, '\\');
				
				startofchunk = curpos -1;
				
			} else if (*curpos == '\'')
				skipcharconstant(DONOTSKIPSPACEAFTERCHARCONST);
			else if (*curpos == '\\') {
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the backslash character.
				if (startofchunk <= endofchunk) createchunk();
				
				// Escape the backslash character.
				stringappend4(&chunks->prev->content, '\\');
				
				startofchunk = curpos;
				
				++curpos;
				
			} else ++curpos;
			
		} else if (withinlyxcodeblock && curpos[0] == '%' && curpos[1] == '>') {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the closing code block.
			if (startofchunk <= endofchunk) createchunk();
			
			stringappend4(&chunks->prev->content, '"');
			
			curpos += 2;
			
			startofchunk = curpos;
			
			skipspace();
			
			withinlyxcodeblock = 0;
			
		} else if ((s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL)).ptr) {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the macro used.
			if (startofchunk <= endofchunk) createchunk();
			
			// There is no need to set startofchunk
			// as it will be set when this function
			// is recursively called as well as at
			// the end of this block.
			
			// This function create a chunk for which the content
			// is the name of the file currently being preprocessed;
			// and add it to the bottom of the linkedlist pointed by chunks.
			void createchunkforcurrentfilepath () {
				
				chunk* c = mmallocz(sizeof(chunk));
				
				c->path = stringduplicate1(currentfilepath);
				
				c->offset = (uint)savedcurpos - (uint)compileargsource;
				
				c->linenumber = countlines(compileargsource, savedcurpos);
				
				c->content = stringfmt("\"%s\"", currentfilepath.ptr);
				
				LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, c, chunks);
			}
			
			// This function create a chunk for which the content is
			// the line number within the file currently being preprocessed
			// at the address location in curpos; and add it to
			// the linkedlist pointed by chunks.
			void createchunkforcurrentlinenumber () {
				
				chunk* c = mmallocz(sizeof(chunk));
				
				c->path = stringduplicate1(currentfilepath);
				
				c->offset = (uint)savedcurpos - (uint)compileargsource;
				
				c->linenumber = countlines(compileargsource, savedcurpos);
				
				c->content = stringfmt("%d", c->linenumber);
				
				LINKEDLISTCIRCULARADDTOBOTTOM(prev, next, c, chunks);
			}
			
			if (stringiseq2(s, "FILE") && !macros->isbeingdefined) {
				
				mmrefdown(s.ptr);
				
				createchunkforcurrentfilepath();
				
			} else if (stringiseq2(s, "LINE") && !macros->isbeingdefined) {
				
				mmrefdown(s.ptr);
				
				createchunkforcurrentlinenumber();
				
			} else {
				
				macro* m = checkifmacroexist(s);
				
				mmrefdown(s.ptr);
				
				if (m) {
					
					if (m->isbeingdefined) {
						curpos = savedcurpos;
						throwerror("a macro that is being defined cannot be used");
					}
					
					if (m->args) {
						// I get here if a function-like macro was parsed.
						
						skipspace();
						
						if (*curpos != '(') {
							curpos = savedcurpos;
							throwerror("function-like macro not followed by '('");
						}
						
						++curpos;
						skipspace();
						
						uint nbrofargs = m->nbrofargs;
						
						uint i = 0;
						
						macro* macroarg;
						
						// I save the current value of chunkorigin as it will
						// be modified while parsing the function-like macro.
						string savedchunkorigin = chunkorigin;
						
						while (1) {
							// Within this loop, I read the arguments
							// to be used with the function-like macro.
							
							macroarg = m->args[i];
							
							// I set chunkorigin with the string that will
							// be used by createchunk() to set the field
							// origin of struct chunk created while parsing
							// the argument of the function-like macro.
							chunkorigin = stringfmt(
								"from argument \"%s\" of macro \"%s\"",
								macroarg->name.ptr,
								m->name.ptr);
							
							macroarg->chunks = preprocessor(PREPROCESSMACROARGUMENT);
							
							mmrefdown(chunkorigin.ptr);
							
							if (++i < nbrofargs) {
								
								if (*curpos != ',') throwerror("expecting ','");
								
								++curpos;
								skipspace();
								
							} else {
								
								if (*curpos != ')') throwerror("expecting ')'");
								
								++curpos;
								
								break;
							}
						}
						
						// I restore the value of chunkorigin.
						chunkorigin = savedchunkorigin;
						
						// I process the chunks of the function-like macro,
						// substituting the arguments parsed above.
						
						// m->chunks will always be non-null because
						// a function-like macro must use its arguments
						// and that will yield the generation of
						// at least one chunk.
						chunk* macrochunks = m->chunks;
						
						chunk* c = macrochunks;
						
						do {
							i = 0;
							
							while (1) {
								
								macroarg = m->args[i];
								
								// Macros expansion within the definition of a macro
								// is done only when the macro is being defined; after
								// its definition, the only macro expansion done
								// within its defined value is for FILE and LINE or for
								// its arguments when it is a function-like macro; chunks
								// of a function-like macro that can be substituted by
								// their corresponding argument have their field origin.ptr null.
								if (!c->origin.ptr && stringiseq1(c->content, macroarg->name)) {
									// If possible, I duplicate the linkedlist
									// of chunk of the macro argument into
									// the linkedlist pointed by chunks.
									
									chunk* c = macroarg->chunks;
									
									if (c) duplicatemacrochunks(c);
									
									break;
								}
								
								if (++i == nbrofargs) {
									// If I get here, the chunk of the function-like
									// macro do not correspond to any of the arguments
									// of the function-like macro.
									// If the function-like macro was invoked from outside
									// the definition of a function-like or object-like macro,
									// and the content of the chunk was FILE or LINE, I create
									// a chunk for their value into the linkedlist pointed
									// by chunks, otherwise I duplicate the chunk into
									// the linkedlist pointed by chunks.
									if (action != PREPROCESSOBJECTLIKEMACRO && action != PREPROCESSFUNCTIONLIKEMACRO) {
										
										if (stringiseq2(c->content, "FILE")) createchunkforcurrentfilepath();
										else if (stringiseq2(c->content, "LINE")) createchunkforcurrentlinenumber();
										else duplicatemacrochunk(c);
										
									} else duplicatemacrochunk(c);
									
									break;
								}
							}
							
						} while ((c = c->next) != macrochunks);
						
						// I free the linkedlist of chunk generated
						// for the arguments of the function-like macro.
						do {
							c = m->args[--nbrofargs]->chunks;
							
							if (c) freemacrochunklinkedlist(c);
							
						} while (nbrofargs);
						
					} else {
						// I get here if an object-like macro was parsed.
						
						// I set the field wasused of the macro here only
						// because it is used only with for-loop macro and
						// macro that are arguments of a function-like macro.
						m->wasused = 1;
						
						// If possible, I duplicate the linkedlist of chunk of
						// the macro into the linkedlist pointed by chunks.
						
						chunk* macrochunks = m->chunks;
						
						if (macrochunks) {
							
							chunk* c = macrochunks;
							
							do {
								// If the object-like macro was invoked from outside
								// the definition of a function-like or object-like macro,
								// and the content of the chunk was FILE or LINE, I create
								// a chunk for their value into the linkedlist pointed
								// by chunks, otherwise I duplicate the chunk into
								// the linkedlist pointed by chunks.
								if (action != PREPROCESSOBJECTLIKEMACRO && action != PREPROCESSFUNCTIONLIKEMACRO) {
									
									if (stringiseq2(c->content, "FILE")) createchunkforcurrentfilepath();
									else if (stringiseq2(c->content, "LINE")) createchunkforcurrentlinenumber();
									else duplicatemacrochunk(c);
									
								} else duplicatemacrochunk(c);
								
							} while ((c = c->next) != macrochunks);
							
							// If the macro chunk location is to be set
							// when used, I set the fields origin, path, offset
							// and linenumber of its chunk duplicate; note that
							// such macro only has a single chunk; note also
							// that such macro is always an object-like macro.
							if (m->chunklocationtsetwhenused) {
								
								c = chunks->prev;
								
								c->origin = stringduplicate1(chunkorigin);
								
								c->path = stringduplicate1(currentfilepath);
								
								c->offset = (uint)savedcurpos - (uint)compileargsource;
								
								c->linenumber = countlines(compileargsource, savedcurpos);
							}
						}
					}
					
				} else {
					
					startofchunk = endofchunk +1;
					endofchunk = curpos -1;
					
					createchunk();
				}
			}
			
			startofchunk = curpos;
			
		} else if (*curpos == '_') {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the concatening character.
			if (startofchunk <= endofchunk) createchunk();
			
			++curpos;
			
			startofchunk = curpos;
			
		} else if (*curpos == '\n' && action == PREPROCESSOBJECTLIKEMACRO) {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the newline character.
			if (startofchunk <= endofchunk) createchunk();
			
			// There is no need to set startofchunk
			// as it will be set after returning
			// from this function.
			
			return chunks;
			
		} else if (*curpos == '(' && action == PREPROCESSMACROARGUMENT) {
			
			++openedparanthesiscount;
			
			++curpos;
			skipspace();
			
		} else if (*curpos == ')' && action == PREPROCESSMACROARGUMENT) {
			
			if (!openedparanthesiscount) {
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the closing paranthesis.
				if (startofchunk <= endofchunk) createchunk();
				
				// There is no need to set startofchunk
				// as it will be set after returning
				// from this function.
				
				return chunks;
			}
			
			--openedparanthesiscount;
			
			++curpos;
			skipspace();
			
		} else if (*curpos == ',' && action == PREPROCESSMACROARGUMENT) {
			
			if (!openedparanthesiscount) {
				// If startofchunk <= endofchunk generate a chunk
				// for the text before the comma character.
				if (startofchunk <= endofchunk) createchunk();
				
				// There is no need to set startofchunk
				// as it will be set after returning
				// from this function.
				
				return chunks;
			}
			
			++curpos;
			skipspace();
			
		} else if (*curpos == '#' && action != PREPROCESSOBJECTLIKEMACRO) {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the comment.
			if (startofchunk <= endofchunk) createchunk();
			
			++curpos;
			
			if (*curpos) {
				// I check whether it is a block
				// of comments or line of comment.
				if (*curpos == '{') {
					
					u8* commentblockbegin = curpos;
					
					// Keep track of the depth
					// of nested comment blocks.
					uint commentblockdepth = 0;
					
					++curpos;
					
					while (1) {
						
						if (!*curpos) {
							// Set curpos where "#{" was used.
							curpos = commentblockbegin - 1;
							
							throwerror("corresponding \"}#\" could not be found");
							
						} else if (*curpos == '#') {
							
							++curpos;
							
							if (*curpos == '{') {
								
								++curpos;
								
								++commentblockdepth;
								
							} else while (*curpos && *curpos != '\n') ++curpos;
							
						} else if (*curpos == '}') {
							
							++curpos;
							
							if (*curpos == '#') {
								
								++curpos;
								
								if (!commentblockdepth) break;
								
								--commentblockdepth;
							}
							
						} else if (*curpos == '"') skipstringconstant(DONOTSKIPSPACEAFTERSTRING);
						else if (*curpos == '\'') skipcharconstant(DONOTSKIPSPACEAFTERCHARCONST);
						else ++curpos;
					}
					
				} else {
					
					while (*curpos && *curpos != '\n' &&
						(action != PREPROCESSINCLUDELYX ||
							curpos[0] != '%' || curpos[1] != '>')) {
						// I skip characters till
						// the end of the comment line.
						++curpos;
					}
				}
			}
			
			startofchunk = curpos;
			
			skipspace();
			
		} else if (*curpos == '`') {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the preprocessor directive.
			if (startofchunk <= endofchunk) createchunk();
			
			// There is no need to set startofchunk
			// as it will be set when this function
			// is recursively called as well as at
			// the end of this block.
			
			++curpos;
			
			if (checkfordirective("include")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action == PREPROCESSFUNCTIONLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "include".
					curpos -= 7;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				#include "include.preprocessor.lyrical.c"
				
			} else if (checkfordirective("define") || checkfordirective("locdef")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action == PREPROCESSFUNCTIONLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "define" or "locdef" .
					curpos -= 6;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				#include "define.preprocessor.lyrical.c"
				
			} else if (checkfordirective("enddef")) {
				// This will set curpos at the beginning
				// of the string "enddef".
				curpos -= 6;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSFUNCTIONLIKEMACRO)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("undef")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action == PREPROCESSFUNCTIONLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "undef".
					curpos -= 5;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				#include "undef.preprocessor.lyrical.c"
				
			} else if (checkfordirective("ifdef")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "ifdef".
					curpos -= 5;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				directiveifdef:;
				
				#include "ifdef.preprocessor.lyrical.c"
				
			} else if (checkfordirective("ifndef")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "ifndef".
					curpos -= 6;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				directiveifndef:;
				
				#include "ifndef.preprocessor.lyrical.c"
				
			} else if (checkfordirective("else")) {
				// This will set curpos at the beginning
				// of the string "else".
				curpos -= 4;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSCONDITIONALBLOCK)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("elifdef")) {
				// This will set curpos at the beginning
				// of the string "elifdef".
				curpos -= 7;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSCONDITIONALBLOCK)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("elifndef")) {
				// This will set curpos at the beginning
				// of the string "elifndef".
				curpos -= 8;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSCONDITIONALBLOCK)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("endif")) {
				// This will set curpos at the beginning
				// of the string "endif".
				curpos -= 5;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSCONDITIONALBLOCK)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("foreach")) {
				
				if (action == PREPROCESSOBJECTLIKEMACRO) {
					// This will set curpos at the beginning
					// of the string "foreach".
					curpos -= 7;
					
					throwerror("invalid use of preprocessor directive");
				}
				
				#include "foreach.preprocessor.lyrical.c"
				
			} else if (checkfordirective("endfor")) {
				// This will set curpos at the beginning
				// of the string "endfor".
				curpos -= 6;
				
				if (action == PREPROCESSOBJECTLIKEMACRO || action != PREPROCESSFOREACHBLOCK)
					throwerror("invalid use of preprocessor directive");
				
				return chunks;
				
			} else if (checkfordirective("abort")) {
				// This will set curpos at the beginning
				// of the string "abort".
				curpos -= 5;
				
				// The error message is part of
				// the `abort statement itself.
				throwerror("");
				
			} else throwerror("expecting a valid preprocessor directive");
			
			skipspacebutstopafternewline();
			
			startofchunk = curpos;
			
		} else if (*curpos == '"' && action != PREPROCESSOBJECTLIKEMACRO)
			skipstringconstant(DONOTSKIPSPACEAFTERSTRING);
			
		else if (*curpos == '\'' && action != PREPROCESSOBJECTLIKEMACRO)
			skipcharconstant(DONOTSKIPSPACEAFTERCHARCONST);
		
		else if (!*curpos) {
			// If startofchunk <= endofchunk generate a chunk
			// for the text before the end of the source code.
			if (startofchunk <= endofchunk) createchunk();
			
			// There is no need to set startofchunk
			// since I am done preprocessing.
			
			if (action == PREPROCESSINCLUDELYX) {
				
				if (withinlyxcodeblock) {
					
					reverseskipspace();
					
					throwerror("unexpected end of file; expecting %>");
				}
				
				stringappend4(&chunks->prev->content, '"');
				
				if (compilearglyxappend) stringappend2(&chunks->prev->content, compilearglyxappend);
				
				return chunks;
				
			} else if (action != PREPROCESSBEGIN && action != PREPROCESSINCLUDE && action != PREPROCESSINCLUDELYX) {
				// If I get here, I have an unexpected end of file.
				return chunks;
			}
			
			if (openedparanthesiscount) {
				
				reverseskipspace();
				
				throwerror("unexpected end of file while invoking function-like macro");
			}
			
			if (action == PREPROCESSBEGIN && macros) freemacros();
			
			return chunks;
			
		} else ++curpos;
	}
}

attachmodulechunks(preprocessor(PREPROCESSBEGIN));
chunks = moduleschunks;

if (currentfilepath.ptr) mmrefdown(currentfilepath.ptr);

if (alreadyincludedmodules.ptr) {
	
	uint n = arrayuintsz(alreadyincludedmodules);
	
	while (n) {
		mmrefdown((void*)alreadyincludedmodules.ptr[--n]);
	}
	
	mmrefdown(alreadyincludedmodules.ptr);
}

mmrefdown(cwd.ptr);

mmrefdown(initialcwd.ptr);

// This function combine the content of all
// the chunk in the linkedlist pointed by
// the variable chunks into a single null terminated
// string and set the variable compileargsource
// to that null terminated string.
void postpreprocessing () {
	
	string result = stringnull;
	
	chunk* c = chunks;
	
	do {
		if (c->content.ptr) {
			
			uint ccontentsz = stringmmsz(c->content);
			
			stringappend1(&result, c->content);
			
			// The string content of the chunk
			// is no longer needed; I free it
			// and preserve its size.
			
			mmrefdown(c->content.ptr);
			
			c->contentsz = ccontentsz;
		}
		
	} while ((c = c->next) != chunks);
	
	compileargsource = result.ptr;
}

postpreprocessing();

// ### Enable to dump postprocessed source.
// ### Turn this into a compiler option.
#if 0
fprintf(stderr,
	"----\n"
	"%s\n"
	"----\n",
	compileargsource);
#endif
