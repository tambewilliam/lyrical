
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

string s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL);

if (!s.ptr) {
	curpos = savedcurpos;
	throwerror("expecting a macro name");
}

macro* m = checkifmacroexist(s);

mmrefdown(s.ptr);

skipspace();

if (m) {
	
	if (m->isbeingdefined) {
		curpos = savedcurpos;
		while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;
		throwerror("a macro that is being defined cannot be used");
	}
	
	// skipconditionalblock() will set curpos
	// right before either of the preprocessor
	// directive `else, `elifdef, `elifndef,
	// `endif; it will also return when
	// reaching an unexpected end of file.
	skipconditionalblock();
	
	if (!*curpos) {
		
		curpos = savedcurpos;
		
		// I search where the directive start since
		// it could have been `ifndef or `elifndef.
		do --curpos; while (*curpos != '`');
		
		++curpos;
		
		throwerror("corresponding `endif could not be found");
	}
	
	if (checkfordirective("elifdef")) goto directiveifdef;
	else if (checkfordirective("elifndef")) goto directiveifndef;
	else if (checkfordirective("else")) {
		// The recursive call to preprocessor() done below
		// will return when encountering either of the preprocessor
		// directive: `else, `elifdef, `elifndef, `endif;
		// it will also return when reaching an unexpected
		// end of file.
		chunk* c = preprocessor(PREPROCESSCONDITIONALBLOCK);
		
		if (!*curpos) {
			
			curpos = savedcurpos;
			
			// I search where the directive start since
			// it could have been `ifndef or `elifndef.
			do --curpos; while (*curpos != '`');
			
			++curpos;
			
			throwerror("corresponding `endif could not be found");
		}
		
		// I attach the returned linkedlist
		// to the linkedlist pointed by chunks.
		if (c) attachchunks(c);
		
		// The recursive call to preprocessor() done above
		// would have set curpos right before either of the preprocessor
		// directive `else, `elifdef, `elifndef, `endif.
		
		if (!checkfordirective("endif")) throwerror("expecting `endif");
		
	} else {
		// When I get here, curpos is certainly at
		// the beginning of the directive string "endif";
		// I set curpos after that string.
		curpos += 5;
	}
	
} else {
	// The recursive call to preprocessor() done below
	// will return when encountering either of the preprocessor
	// directive: `else, `elifdef, `elifndef, `endif;
	// it will also return when reaching an unexpected
	// end of file.
	chunk* c = preprocessor(PREPROCESSCONDITIONALBLOCK);
	
	if (!*curpos) {
		
		curpos = savedcurpos;
		
		// I search where the directive start since
		// it could have been `ifndef or `elifndef.
		do --curpos; while (*curpos != '`');
		
		++curpos;
		
		throwerror("corresponding `endif could not be found");
	}
	
	// I attach the returned linkedlist
	// to the linkedlist pointed by chunks.
	if (c) attachchunks(c);
	
	// The recursive call to preprocessor() done above
	// would have set curpos at the beginning of the string
	// for either of the preprocessor directive `else,
	// `elifdef, `elifndef, `endif.
	
	while (!checkfordirective("endif")) {
		// skipconditionalblock() will set curpos
		// right before either of the preprocessor
		// directive `else, `elifdef, `elifndef,
		// `endif; it will also return when
		// reaching an unexpected end of file.
		skipconditionalblock();
		
		if (!*curpos) {
			
			curpos = savedcurpos;
			
			// I search where the directive start since
			// it could have been `ifndef or `elifndef.
			do --curpos; while (*curpos != '`');
			
			++curpos;
			
			throwerror("corresponding `endif could not be found");
		}
	}
}
