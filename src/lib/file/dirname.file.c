
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


// The functions filedirname() and filebasename() break
// a null-terminated pathname string into directory and
// filename components respectively. ei:
// path             dirname            basename
// "/usr/lib"       "/usr"             "lib"
// "/usr/"          "/"                "usr"
// "usr"            "."                "usr"
// "/"              "/"                "/"
// "."              "."                "."
// ".."             "."                ".."
// The argument path should always be
// a valid pointer to a null terminated string.

string filedirname (u8* path) {
	
	string s = stringduplicate2(path);
	
	uint ssz = stringmmsz(s);
	
	if (!ssz) {
		
		if (s.ptr) mmfree(s.ptr);
		
		return stringnull;
	}
	
	u8* c = (s.ptr + ssz) -1;
	
	while (*c == '/' && c > s.ptr) --c;
	
	string retvar;
	
	while (1) {
		
		if (c < s.ptr) {
			
			++c;
			
			if (*c == '/') retvar = stringduplicate4('/');
			else retvar = stringduplicate4('.');
			
			mmfree(s.ptr);
			
			return retvar;
		}
		
		if (*c == '/') {
			
			do --c; while (*c == '/' && c > s.ptr);
			
			++c;
			
			if (c == s.ptr) retvar = stringduplicate4('/');
			else retvar = stringduplicate3(s.ptr, (uint)c - (uint)s.ptr);
			
			mmfree(s.ptr);
			
			return retvar;
		}
		
		--c;
	}
}
