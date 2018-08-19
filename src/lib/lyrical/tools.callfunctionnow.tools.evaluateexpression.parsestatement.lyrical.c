
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


// This function generate a call signature
// using the function or operator name given
// as argument, along with the type of currently
// pushed arguments. Example of call signature:
// "funcname|uint|void()|", "++|uint*|", "+|uint|uint|";
// Note that in the signature string, the name of
// the operator or function and its arguments are
// separated by '|' instead of commas, opening and closing
// paranthesis which would create conflict since those
// same characters are used in a pointer to function type.
string generatecallsignature (u8* funcname) {
	
	string s = stringduplicate2(funcname);
	
	stringappend4(&s, '|');
	
	if (funcarg) {
		// funcarg point to the last pushed argument.
		// funcarg->next will give me the address of
		// the first pushed argument.
		lyricalargument* arg = funcarg->next;
		
		do {
			stringappend1(&s, arg->typepushed);
			
			arg = arg->next;
			
			stringappend4(&s, '|');
			
		// Because the linkedlist of pushed argument is a circular linkedlist,
		// if arg == funcarg->next then I came back to the first argument variable.
		} while (arg != funcarg->next);
		
	} else stringappend4(&s, '|');
	
	return s;
}


// This function is used to parse
// the pointer to function type string
// of the lyricalvariable pointed by varptrtofunc.
// This function set typeofresultvar
// and push a null void* argument
// if the function is variadic.
void parsetypeofptrtofunc () {
	// Here below are examples of different pointer to function type:
	// "void()"
	// "uint*[4](uint(uint)(uint)[7], u8)"
	// "void*[4](uint, u8, ...)"
	// "void(uint, u8&)"
	// "void(uint, u8)(uint, void(u8))"
	
	string ptrtofunctype;
	
	if (varptrtofunc->cast.ptr) ptrtofunctype = varptrtofunc->cast;
	else ptrtofunctype = varptrtofunc->type;
	
	// If ptrtofunctype represent
	// a variadic function, I add
	// an additional argument which
	// is a null pointer which will mark
	// the end of the variadic arguments.
	if (ptrtofunctype.ptr[stringmmsz(ptrtofunctype)-2] == '.') {
		// Note that curpos has been set at '(' .
		// It will be used by pushargument() when setting
		// the argument lyricalargumentflag.id .
		pushargument(getvarnumber(0, voidptrstr));
	}
	
	// The ptrtofunctype string is supposed
	// to be a pointer to function type terminated by ')';
	// So I initialize c to the character preceding ')';
	u8* c = ptrtofunctype.ptr + (stringmmsz(ptrtofunctype)-2);
	
	// This function will search
	// where the opening paranthesis
	// of the arguments is located
	// in the pointer to function.
	void searchopeningparanthesis () {
		
		uint paranthesis = 0;
		
		while (1) {
			
			if (*c == '(') {
				
				if (!paranthesis) break;
				
				--paranthesis;
				
			} else if (*c == ')') ++paranthesis;
			
			--c;
		}
	}
	
	searchopeningparanthesis();
	
	// I set the type of the result variable,
	// which is whatever type is coming before
	// the opening paranthesis.
	typeofresultvar = stringduplicate3(ptrtofunctype.ptr, (uint)c - (uint)ptrtofunctype.ptr);
	
	// If I am in the secondpass, I do not need
	// to go onto the following code because
	// doing it in the firstpass was enough.
	if (compilepass) return;
	
	// Set c where the first argument
	// type of the function is located.
	++c;
	
	if ((*c == ')' && funcarg) || (*c != ')' && !funcarg)) throwerror("pointer to function; incorrect use");
	
	if (funcarg) {
		// funcarg point to the last pushed argument.
		// funcarg->next will give me the address of
		// the first pushed argument.
		lyricalargument* arg = funcarg->next;
		
		// Variable used to keep track
		// of the argument position.
		uint argpos = 1;
		
		while (1) {
			
			u8* getargtypestringptr = c;
			
			uint openingparanthesisfound = 0;
			
			// This function return the type string
			// of an argument within the pointer
			// to function type string.
			// This function should never fail because
			// it go through a type string that is
			// supposed to have been correctly parsed.
			while (1) {
				
				if (*c == ')') {
					
					if (openingparanthesisfound) --openingparanthesisfound;
					else break;
					
				} else if (*c == '(') ++openingparanthesisfound;
				else if (*c == ',' || *c == '&') {
					
					if (!openingparanthesisfound) break;
					
				} else if (!*c) break;
				
				++c;
			}
			
			uint getargtypestringsz = (uint)c - (uint)getargtypestringptr;
			
			uint argtypepushedsz = stringmmsz(arg->typepushed);
			
			if (getargtypestringsz != argtypepushedsz ||
				!stringiseq4(getargtypestringptr, arg->typepushed.ptr, argtypepushedsz)) {
				throwerror(stringfmt("pointer to function; incorrect %dth argument type", argpos).ptr);
			}
			
			if (*c == '&') {
				// I get here if the argument is supposed to be passed byref.
				
				// bitselected, address or constant variables
				// cannot be passed byref.
				if (arg->bitselect || isvarreadonly(arg->v)) {
					throwerror(stringfmt("pointer to function; incorrect %dth argument passed by reference", argpos).ptr);
				}
				
				// I get here only when I am in the firstpass,
				// and when that is the case, I notify
				// the secondpass that the argument will be
				// passed byref. That information is used
				// in the secondpass within pushargument().
				arg->flag->istobepassedbyref = 1;
				
				++c;
			}
			
			++argpos;
			
			arg = arg->next;
			
			// The linkedlist of pushed argument
			// is a circular linkedlist, so
			// if arg == funcarg->next then
			// I came back to the first argument.
			
			if (*c == ')') {
				
				if (arg == funcarg->next) return;
				else throwerror("pointer to function; more arguments than needed");
				
			} else if (*c == ',') {
				
				if (arg == funcarg->next) throwerror("pointer to function; less arguments than needed");
				else ++c;
				
				if (*c == '.') return;
			}
		}
	}
}
