
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


while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

parsemacroname:;

u8* savedcurpos = curpos;

string s = readsymbol(UPPERCASESYMBOL|LOWERCASESYMBOL|DONOTSKIPSPACEAFTERSYMBOL);

if (!s.ptr) {
	curpos = savedcurpos;
	throwerror("expecting a macro name");
}

macro* m;

if (m = checkifmacroexist(s)) {
	
	if (m->cannotbeundefined) {
		
		curpos = savedcurpos;
		
		throwerror("macro cannot be undefined");
	}
	
	if (m->isbeingdefined) {
		
		curpos = savedcurpos;
		
		throwerror("a macro that is being defined cannot be used");
	}
	
} else {
	
	curpos = savedcurpos;
	
	throwerror("macro was not previously defined");
}

mmrefdown(s.ptr);

freemacro(m);

while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

if (*curpos) {
	
	if (*curpos != '\n' &&
		(action != PREPROCESSINCLUDELYX ||
			curpos[0] != '%' || curpos[1] != '>')) {
		goto parsemacroname;
	}
}
